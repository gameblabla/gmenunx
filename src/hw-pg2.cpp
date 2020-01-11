#include <math.h>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "constants.h"
#include "debug.h"
#include "hw-pg2.h"
#include "rtc.h"

HwPG2::HwPG2() : IHardware() {
    TRACE("enter");
    this->BLOCK_DEVICE = "/sys/block/mmcblk1/size";
    this->INTERNAL_MOUNT_DEVICE = "/dev/mmcblk0";
    this->EXTERNAL_MOUNT_DEVICE = "/dev/mmcblk1p1";
    this->EXTERNAL_MOUNT_FORMAT = "auto";
    this->EXTERNAL_MOUNT_POINT = EXTERNAL_CARD_PATH;

    this->clock_ = new RTC();
    this->soundcard_ = new AlsaSoundcard("default", "PCM");

    this->ledMaxBrightness_ = fileExists(LED_MAX_BRIGHTNESS_PATH) ? fileReader(LED_MAX_BRIGHTNESS_PATH) : 0;
    this->performanceModes_.insert({"ondemand", "On demand"});
    this->performanceModes_.insert({"performance", "Performance"});

    this->pollBacklight = fileExists(BACKLIGHT_PATH);
    this->pollBatteries = fileExists(BATTERY_CHARGING_PATH) && fileExists(BATTERY_LEVEL_PATH);

    this->supportsOverclocking_ = fileExists(SYSFS_CPUFREQ_SET);
    this->supportsPowerGovernors_ = fileExists(SYSFS_CPU_SCALING_GOVERNOR);
    this->cpuSpeeds_ = { 360, 1080 };

    this->getBacklightLevel();
    this->getKeepAspectRatio();
    this->resetKeymap();

    TRACE(
        "brightness: %i, volume : %i",
        this->getBacklightLevel(),
        this->soundcard_->getVolume());
}
HwPG2::~HwPG2() {
    delete this->clock_;
    delete this->soundcard_;
    this->ledOff();
}

IClock *HwPG2::Clock() {
    return (IClock *)this->clock_;
}
ISoundcard *HwPG2::Soundcard() {
    return (ISoundcard *)this->soundcard_;
}

bool HwPG2::getTVOutStatus() { return 0; };
std::string HwPG2::getTVOutMode() { return "OFF"; }
void HwPG2::setTVOutMode(std::string mode) {
    std::string val = mode;
    if (val != "NTSC" && val != "PAL") val = "OFF";
}

bool HwPG2::supportsPowerGovernors() { return this->supportsPowerGovernors_; }

std::string HwPG2::getPerformanceMode() {
    TRACE("enter");
    this->performanceMode_ = "ondemand";
    if (this->supportsPowerGovernors_) {
        std::string rawValue = fileReader(SYSFS_CPU_SCALING_GOVERNOR);
        this->performanceMode_ = full_trim(rawValue);

    TRACE("read - %s", this->performanceMode_.c_str());
    }
    return this->performanceModeMap(this->performanceMode_);
}
void HwPG2::setPerformanceMode(std::string alias) {
    TRACE("raw desired : %s", alias.c_str());
    if (!this->supportsPowerGovernors_)
        return;

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
        procWriter(SYSFS_CPU_SCALING_GOVERNOR, mode);
        this->performanceMode_ = mode;
    } else {
        TRACE("nothing to do");
    }
    TRACE("exit");
}
std::vector<std::string> HwPG2::getPerformanceModes() {
    std::vector<std::string> results;
    if (this->supportsPowerGovernors_) {
        std::unordered_map<std::string, std::string>::iterator it;
        for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
            results.push_back(it->second);
        }
    }
    return results;
}

bool HwPG2::supportsOverClocking() {
    return this->supportsOverclocking_;
}

bool HwPG2::setCPUSpeed(uint32_t mhz) {
    TRACE("enter : %i", mhz);
    if (!this->supportsOverClocking())
        return false;

    std::vector<uint32_t>::const_iterator it;
    bool found = false;
    for (it = this->cpuSpeeds().begin(); it != this->cpuSpeeds().end(); ++it) {
        int val = (*it);
        TRACE("comparing %i to %i", mhz, val);
        if (val == mhz) {
            found = true;
            break;
        }
    }
    if (!found) {
        TRACE("couldn't find speed : %i", mhz);
        return false;
    }

    int finalFreq = mhz * 1000;
    TRACE("finalFreq : %i", finalFreq);
    std::stringstream ss;
    ss << finalFreq;
    std::string value;
    ss >> value;
    if (this->writeValueToFile(SYSFS_CPUFREQ_MAX, value.c_str())) {
        return this->writeValueToFile(SYSFS_CPUFREQ_SET, value.c_str());
    }
    return false;
}
uint32_t HwPG2::getCPUSpeed() {
    if (!this->supportsOverClocking())
        return 0;

    std::string rawCpu = fileReader(SYSFS_CPUFREQ_GET);
    int result = atoi(rawCpu.c_str()) / 1000;
    TRACE("exit : %i",  result);
    return result;
}

uint32_t HwPG2::getCpuDefaultSpeed() { 
    return this->supportsOverClocking() ? 1080 : 1000; 
}

void HwPG2::ledOn(int flashSpeed) {
    TRACE("enter");
    int limited = constrain(flashSpeed, 0, atoi(ledMaxBrightness_.c_str()));
    std::string trigger = triggerToString(LedAllowedTriggers::TIMER);
    TRACE("mode : %s - for %i", trigger.c_str(), limited);
    procWriter(LED_TRIGGER_PATH, trigger);
    procWriter(LED_DELAY_ON_PATH, limited);
    procWriter(LED_DELAY_OFF_PATH, limited);
    TRACE("exit");
}
void HwPG2::ledOff() {
    TRACE("enter");
    std::string trigger = triggerToString(LedAllowedTriggers::NONE);
    TRACE("mode : %s", trigger.c_str());
    procWriter(LED_TRIGGER_PATH, trigger);
    procWriter(LED_BRIGHTNESS_PATH, ledMaxBrightness_);
    TRACE("exit");
    return;
}

int HwPG2::getBatteryLevel() {
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
}

int HwPG2::getBacklightLevel() {
    TRACE("enter");
    if (this->pollBacklight) {
        int level = 0;
        //force  scale 0 - 100
        std::string result = fileReader(BACKLIGHT_PATH);
        if (result.length() > 0) {
            level = ceil(atoi(trim(result).c_str()) / 2.55);
            //level = (level * -1) + 100;
        }
        this->backlightLevel_ = level;
    }
    TRACE("exit : %i", this->backlightLevel_);
    return this->backlightLevel_;
}
int HwPG2::setBacklightLevel(int val) {
    TRACE("enter - %i", val);
    // wrap it
    if (val <= 0)
        val = 100;
    else if (val > 100)
        val = 0;
    if (val == this->backlightLevel_)
        return val;

    int localVal = (int)(val * (255.0f / 100));
    //localVal = (localVal * -1) + 100;
    TRACE("device value : %i", localVal);

    if (procWriter(BACKLIGHT_PATH, localVal)) {
        TRACE("success");
    } else {
        ERROR("Couldn't update backlight value to : %i", localVal);
    }
    this->backlightLevel_ = val;
    return this->backlightLevel_;
}

bool HwPG2::getKeepAspectRatio() {
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
bool HwPG2::setKeepAspectRatio(bool val) {
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

std::string HwPG2::getDeviceType() { return "PocketGo 2"; }

bool HwPG2::setScreenState(const bool &enable) {
    TRACE("enter : %s", (enable ? "on" : "off"));
    const char *path = SCREEN_BLANK_PATH.c_str();
    const char *blank = enable ? "0" : "1";
    return this->writeValueToFile(path, blank);
}

std::string HwPG2::systemInfo() {
    TRACE("append - command /usr/bin/system_info");
    if (fileExists("/usr/bin/system_info")) {
        return execute("/usr/bin/system_info") + "\n";
    }
    return IHardware::systemInfo();
}

std::string HwPG2::performanceModeMap(std::string fromInternal) {
    std::unordered_map<std::string, std::string>::iterator it;
    for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
        if (fromInternal == it->first) {
            return it->second;
            ;
        }
    }
    return "On demand";
}

std::string HwPG2::triggerToString(LedAllowedTriggers t) {
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

void HwPG2::resetKeymap() {
    this->writeValueToFile(ALT_KEYMAP_FILE, "N");
}
