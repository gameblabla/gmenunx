#include <string>
#include <vector>
#include <math.h>

#include "constants.h"
#include "hw-gkd350h.h"
#include "sysclock.h"
#include "fileutils.h"

HwGkd350h::HwGkd350h() : IHardware() {
    TRACE("enter");

    this->BLOCK_DEVICE = "/sys/block/mmcblk1/size";
    this->INTERNAL_MOUNT_DEVICE = "/dev/mmcblk0";
    this->EXTERNAL_MOUNT_DEVICE = "/dev/mmcblk1p1";
    this->EXTERNAL_MOUNT_FORMAT = "auto";
    this->EXTERNAL_MOUNT_POINT = EXTERNAL_CARD_PATH;

    this->clock_ = new RTC();
    this->soundcard_ = new AlsaSoundcard("default", "Master");
    this->cpu_ = new DefaultCpu();

    this->pollBatteries = FileUtils::fileExists(BATTERY_CHARGING_PATH) && FileUtils::fileExists(BATTERY_LEVEL_PATH);
    this->pollBacklight = FileUtils::fileExists(BACKLIGHT_PATH);

    this->getBacklightLevel();
    this->getKeepAspectRatio();
    this->resetKeymap();

    TRACE(
        "brightness: %i, volume : %i",
        this->getBacklightLevel(),
        this->soundcard_->getVolume());
    TRACE("exit");
}

HwGkd350h::~HwGkd350h() {
    delete this->clock_;
    delete this->cpu_;
    delete this->soundcard_;
}

bool HwGkd350h::getTVOutStatus() { return false; }

void HwGkd350h::setTVOutMode(std::string mode) { return; }

std::string HwGkd350h::getTVOutMode() { return ""; }

/*
bool HwGkd350h::supportsPowerGovernors() { return false; }

void HwGkd350h::setPerformanceMode(std::string alias) { return; }

std::string HwGkd350h::getPerformanceMode() { return ""; }

std::vector<std::string> HwGkd350h::getPerformanceModes() {
    std::vector<std::string> result;
    return result;
}

bool HwGkd350h::supportsOverClocking() { return true; }

uint32_t HwGkd350h::getCPUSpeed() { 
    return this->getCpuDefaultSpeed(); 
}

bool HwGkd350h::setCPUSpeed(uint32_t mhz) { return true; }

uint32_t HwGkd350h::getCpuDefaultSpeed() { return 1500; }
*/

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

int HwGkd350h::getBacklightLevel() {
    TRACE("enter");
    if (this->pollBacklight) {
        int level = 0;
        //force  scale 0 - 100
        std::string result = fileReader(BACKLIGHT_PATH);
        if (result.length() > 0) {
            level = ceil(atoi(trim(result).c_str()));
        }
        this->backlightLevel_ = level;
    }
    TRACE("exit : %i", this->backlightLevel_);
    return this->backlightLevel_;
}

int HwGkd350h::setBacklightLevel(int val) {
    TRACE("enter - %i", val);
    // wrap it
    if (val <= 0)
        val = 100;
    else if (val > 100)
        val = 0;
    if (val == this->backlightLevel_)
        return val;

    if (procWriter(BACKLIGHT_PATH, val)) {
        TRACE("success");
    } else {
        ERROR("Couldn't update backlight value to : %i", val);
    }
    this->backlightLevel_ = val;
    return this->backlightLevel_;
 }

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
