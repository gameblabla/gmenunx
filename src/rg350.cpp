
#include <math.h>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "constants.h"
#include "debug.h"
#include "rg350.h"
#include "rtc.h"

HwRg350::HwRg350() {
    TRACE("enter");
    this->BLOCK_DEVICE = "/sys/block/mmcblk1/size";
    this->INTERNAL_MOUNT_DEVICE = "/dev/mmcblk0";
    this->EXTERNAL_MOUNT_DEVICE = "/dev/mmcblk1p1";
    this->EXTERNAL_MOUNT_FORMAT = "auto";
    this->EXTERNAL_MOUNT_POINT = EXTERNAL_CARD_PATH;

    this->clock_ = new RTC();

    this->ledMaxBrightness_ = fileExists(LED_MAX_BRIGHTNESS_PATH) ? fileReader(LED_MAX_BRIGHTNESS_PATH) : 0;
    this->performanceModes_.insert({"ondemand", "On demand"});
    this->performanceModes_.insert({"performance", "Performance"});

    this->pollBacklight = fileExists(BACKLIGHT_PATH);
    this->pollBatteries = fileExists(BATTERY_CHARGING_PATH) && fileExists(BATTERY_LEVEL_PATH);
    this->pollVolume = fileExists(GET_VOLUME_PATH);

    this->getBacklightLevel();
    this->getVolumeLevel();
    this->getKeepAspectRatio();

    TRACE(
        "brightness - max : %s, current : %i, volume : %i",
        ledMaxBrightness_.c_str(),
        this->backlightLevel_,
        this->volumeLevel_);
}
HwRg350::~HwRg350() {
    delete this->clock_;
    this->ledOff();
}

IClock *HwRg350::Clock() { return (IClock *)this->clock_; };

bool HwRg350::getTVOutStatus() { return 0; };
std::string HwRg350::getTVOutMode() { return "OFF"; }
void HwRg350::setTVOutMode(std::string mode) {
    std::string val = mode;
    if (val != "NTSC" && val != "PAL") val = "OFF";
}

std::string HwRg350::getPerformanceMode() {
    TRACE("enter");
    this->performanceMode_ = "ondemand";
    if (fileExists("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")) {
        string rawValue = fileReader("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        this->performanceMode_ = full_trim(rawValue);
    }
    TRACE("read - %s", this->performanceMode_.c_str());
    return this->performanceModeMap(this->performanceMode_);
}
void HwRg350::setPerformanceMode(std::string alias) {
    TRACE("raw desired : %s", alias.c_str());
    std::string mode = this->defaultPerformanceMode;
    if (!alias.empty()) {
        std::unordered_map<std::string, std::string>::iterator it;
        for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
            TRACE("checking '%s' aginst <%s, %s>", alias.c_str(), it->first.c_str(), it->second.c_str());
            if (alias == it->second) {
                TRACE("matched it as : %s", it->first.c_str());
                mode = it->first;
                break;
            }
        }
    }
    TRACE("internal desired : %s", mode.c_str());
    if (mode != this->performanceMode_) {
        TRACE("update needed : current '%s' vs. desired '%s'", this->performanceMode_.c_str(), mode.c_str());
        if (fileExists("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")) {
            procWriter("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", mode);
            this->performanceMode_ = mode;
        }
    } else {
        TRACE("nothing to do");
    }
    TRACE("exit");
}
std::vector<std::string> HwRg350::getPerformanceModes() {
    std::vector<std::string> results;
    std::unordered_map<std::string, std::string>::iterator it;
    for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
        results.push_back(it->second);
    }
    return results;
}

bool HwRg350::supportsOverClocking() {
    TRACE("enter");
    std::string kver = this->getKernelVersion();
    if (kver.empty())
        return false;
    //TRACE("we have a kernel version : %s", kver.c_str());
    char lastChar = kver[kver.length() - 1];
    //TRACE("checking '+' against : '%c'", lastChar);
    bool result = '+' == lastChar;
    TRACE("exit - %i", result);
    return result;
}

bool HwRg350::setCPUSpeed(uint32_t mhz) {
    TRACE("enter : %i", mhz);
    std::vector<uint32_t>::iterator it;
    bool found = false;
    for (it = cpuSpeeds().begin(); it != cpuSpeeds().end(); it++) {
        if ((*it) == mhz) {
            found = true;
            break;
        }
    }
    if (!found)
        return false;

    int finalFreq = mhz * 1000;
    TRACE("finalFreq : %i", finalFreq);
    if (procWriter(SYSFS_CPUFREQ_MAX, finalFreq)) {
        if (procWriter(SYSFS_CPUFREQ_SET, finalFreq)) {
            TRACE("success");
            return true;
        } else {
            ERROR("Couldn't update current cpu freq to : %i", finalFreq);
        }
    } else {
        ERROR("Couldn't update cpu max freq to : %i", finalFreq);
    }
    return false;
};
uint32_t HwRg350::getCPUSpeed() {
    std::string rawCpu = fileReader(SYSFS_CPUFREQ_GET);
    return atoi(rawCpu.c_str()) / 1000;
}
std::vector<uint32_t> HwRg350::cpuSpeeds() { return {360, 1080}; };
uint32_t HwRg350::getCpuDefaultSpeed() { return 1080; };

void HwRg350::ledOn(int flashSpeed) {
    TRACE("enter");
    int limited = constrain(flashSpeed, 0, atoi(ledMaxBrightness_.c_str()));
    std::string trigger = triggerToString(LedAllowedTriggers::TIMER);
    TRACE("mode : %s - for %i", trigger.c_str(), limited);
    procWriter(LED_TRIGGER_PATH, trigger);
    procWriter(LED_DELAY_ON_PATH, limited);
    procWriter(LED_DELAY_OFF_PATH, limited);
    TRACE("exit");
};
void HwRg350::ledOff() {
    TRACE("enter");
    std::string trigger = triggerToString(LedAllowedTriggers::NONE);
    TRACE("mode : %s", trigger.c_str());
    procWriter(LED_TRIGGER_PATH, trigger);
    procWriter(LED_BRIGHTNESS_PATH, ledMaxBrightness_);
    TRACE("exit");
    return;
};

int HwRg350::getBatteryLevel() {
    int online, result = 0;
    if (!this->pollBatteries)
        return result;

    sscanf(fileReader(BATTERY_CHARGING_PATH).c_str(), "%i", &online);
    if (online) {
        result = IHardware::BATTERY_CHARGING;
    } else {
        int battery_level = 0;
        sscanf(fileReader(BATTERY_LEVEL_PATH).c_str(), "%i", &battery_level);
        TRACE("raw battery level - %i", battery_level);
        if (battery_level >= 100)
            result = 5;
        else if (battery_level > 80)
            result = 4;
        else if (battery_level > 60)
            result = 3;
        else if (battery_level > 40)
            result = 2;
        else if (battery_level > 20)
            result = 1;
        else
            result = 0;
    }
    TRACE("scaled battery level : %i", result);
    return result;
};

int HwRg350::getVolumeLevel() {
    TRACE("enter");
    if (this->pollVolume) {
        int vol = -1;
        std::string volPath = GET_VOLUME_PATH + " " + VOLUME_ARGS;
        std::string result = exec(volPath.c_str());
        if (result.length() > 0) {
            vol = atoi(trim(result).c_str());
        }
        // scale 0 - 31, turn to percent
        vol = ceil(vol * 100 / 31);
        this->volumeLevel_ = vol;
    }
    TRACE("exit : %i", this->volumeLevel_);
    return this->volumeLevel_;
};
int HwRg350::setVolumeLevel(int val) {
    TRACE("enter - %i", val);
    if (val < 0)
        val = 100;
    else if (val > 100)
        val = 0;
    if (val == this->volumeLevel_)
        return val;

    if (this->pollVolume) {
        int rg350val = (int)(val * (31.0f / 100));
        TRACE("rg350 value : %i", rg350val);
        std::stringstream ss;
        std::string cmd;
        ss << SET_VOLUME_PATH << " " << VOLUME_ARGS << " " << rg350val;
        std::getline(ss, cmd);
        TRACE("cmd : %s", cmd.c_str());
        std::string result = exec(cmd.c_str());
        TRACE("result : %s", result.c_str());
    }
    this->volumeLevel_ = val;
    return val;
};

int HwRg350::getBacklightLevel() {
    TRACE("enter");
    if (this->pollBacklight) {
        int level = 0;
        //force  scale 0 - 100
        std::string result = fileReader(BACKLIGHT_PATH);
        if (result.length() > 0) {
            level = ceil(atoi(trim(result).c_str()) / 2.55);
        }
        this->backlightLevel_ = level;
    }
    TRACE("exit : %i", this->backlightLevel_);
    return this->backlightLevel_;
}
int HwRg350::setBacklightLevel(int val) {
    TRACE("enter - %i", val);
    // wrap it
    if (val <= 0)
        val = 100;
    else if (val > 100)
        val = 0;
    int rg350val = (int)(val * (255.0f / 100));
    TRACE("rg350 value : %i", rg350val);
    if (rg350val == this->backlightLevel_)
        return val;

    if (procWriter(BACKLIGHT_PATH, rg350val)) {
        TRACE("success");
    } else {
        ERROR("Couldn't update backlight value to : %i", rg350val);
    }
    this->backlightLevel_ = val;
    return this->backlightLevel_;
}

bool HwRg350::getKeepAspectRatio() {
    TRACE("enter");
    if (fileExists(ASPECT_RATIO_PATH)) {
        std::string result = fileReader(ASPECT_RATIO_PATH);
        TRACE("raw result : '%s'", result.c_str());
        if (result.length() > 0) {
            result = result[0];
            result = toLower(result);
        }
        this->keepAspectRatio_ = ("y" == result);
    }
    TRACE("exit : %i", this->keepAspectRatio_);
    return this->keepAspectRatio_;
}
bool HwRg350::setKeepAspectRatio(bool val) {
    TRACE("enter - %i", val);
    std::string payload = val ? "Y" : "N";
    if (procWriter(ASPECT_RATIO_PATH, payload)) {
        TRACE("success");
    } else {
        ERROR("Couldn't update aspect ratio value to : '%s'", payload.c_str());
    }
    this->keepAspectRatio_ = val;
    return this->keepAspectRatio_;
}

std::string HwRg350::getDeviceType() { return "RG-350"; }

bool HwRg350::setScreenState(const bool &enable) {
    TRACE("enter : %s", (enable ? "on" : "off"));
    const char *path = SCREEN_BLANK_PATH.c_str();
    const char *blank = enable ? "0" : "1";
    bool result = false;
    int fd = open(path, O_RDWR);
    if (fd == -1) {
        WARNING("Failed to open '%s': %s", path, strerror(errno));
    } else {
        ssize_t written = write(fd, blank, strlen(blank));
        if (written == -1) {
            WARNING("Error writing '%s': %s", path, strerror(errno));
        } else
            result = true;
        close(fd);
    }
    return result;
}

std::string HwRg350::systemInfo() {
    TRACE("append - command /usr/bin/system_info");
    if (fileExists("/usr/bin/system_info")) {
        return execute("/usr/bin/system_info") + "\n";
    }
    return IHardware::systemInfo();
}

std::string HwRg350::performanceModeMap(std::string fromInternal) {
    std::unordered_map<std::string, std::string>::iterator it;
    for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
        if (fromInternal == it->first) {
            return it->second;
            ;
        }
    }
    return "On demand";
}

std::string HwRg350::triggerToString(LedAllowedTriggers t) {
    TRACE("mode : %i", t);
    switch (t) {
        case TIMER:
            return "timer";
            break;
        default:
            return "none";
            break;
    };
}
