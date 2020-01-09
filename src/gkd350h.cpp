#include <string>
#include <vector>
#include <math.h>

#include "constants.h"
#include "gkd350h.h"
#include "sysclock.h"

HwGkd350h::HwGkd350h() {
    TRACE("enter");

    this->BLOCK_DEVICE = "/sys/block/mmcblk1/size";
    this->INTERNAL_MOUNT_DEVICE = "/dev/mmcblk0";
    this->EXTERNAL_MOUNT_DEVICE = "/dev/mmcblk1p1";
    this->EXTERNAL_MOUNT_FORMAT = "auto";
    this->EXTERNAL_MOUNT_POINT = EXTERNAL_CARD_PATH;

    this->clock_ = new RTC();
    this->pollVolume = fileExists(GET_VOLUME_PATH);
    this->pollBatteries = fileExists(BATTERY_CHARGING_PATH) && fileExists(BATTERY_LEVEL_PATH);

    this->getBacklightLevel();
    this->getVolumeLevel();
    this->getKeepAspectRatio();
    this->resetKeymap();

    TRACE("exit");
}

HwGkd350h::~HwGkd350h() {
    delete this->clock_;
}

IClock *HwGkd350h::Clock() { return (IClock *)this->clock_; };

bool HwGkd350h::getTVOutStatus() { return false; }

void HwGkd350h::setTVOutMode(std::string mode) { return; }

std::string HwGkd350h::getTVOutMode() { return ""; }

bool HwGkd350h::supportsPowerGovernors() { return false; }

void HwGkd350h::setPerformanceMode(std::string alias) { return; }

std::string HwGkd350h::getPerformanceMode() { return ""; }

std::vector<std::string> HwGkd350h::getPerformanceModes() {
    std::vector<std::string> result;
    return result;
}

bool HwGkd350h::supportsOverClocking() { return true; }

uint32_t HwGkd350h::getCPUSpeed() { return 0; };

bool HwGkd350h::setCPUSpeed(uint32_t mhz) { return true; }

uint32_t HwGkd350h::getCpuDefaultSpeed() { return 0; }

void HwGkd350h::ledOn(int flashSpeed) { return; }

void HwGkd350h::ledOff() { return; }

int HwGkd350h::getBatteryLevel() {
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

int HwGkd350h::getVolumeLevel() {
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
}

int HwGkd350h::setVolumeLevel(int val) {
    TRACE("enter - %i", val);
    return val;
}

int HwGkd350h::getBacklightLevel() { return 100; }

int HwGkd350h::setBacklightLevel(int val) { return val; }

bool HwGkd350h::getKeepAspectRatio() { return true; }

bool HwGkd350h::setKeepAspectRatio(bool val) { return val; }

std::string HwGkd350h::getDeviceType() { return "GKD350H"; }

bool HwGkd350h::setScreenState(const bool &enable) {
    TRACE("enter : %s", (enable ? "on" : "off"));
    const char *path = SCREEN_BLANK_PATH.c_str();
    const char *blank = enable ? "0" : "1";
    return this->writeValueToFile(path, blank);
}

void HwGkd350h::resetKeymap() {
    this->writeValueToFile(ALT_KEYMAP_FILE, "N");
}

bool HwGkd350h::writeValueToFile(const std::string & path, const char *content) {
    bool result = false;
    int fd = open(path.c_str(), O_RDWR);
    if (fd == -1) {
        WARNING("Failed to open '%s': %s", path.c_str(), strerror(errno));
    } else {
        ssize_t written = write(fd, content, strlen(content));
        if (written == -1) {
            WARNING("Error writing '%s': %s", path.c_str(), strerror(errno));
        } else {
            result = true;
        }
        close(fd);
    }
    return result;
}
