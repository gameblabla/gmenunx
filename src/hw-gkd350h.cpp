#include <string>
#include <vector>
#include <math.h>

#include "constants.h"
#include "hw-gkd350h.h"
#include "hw-cpu.h"
#include "hw-power.h"
#include "hw-clock.h"
#include "hw-led.h"

#include "fileutils.h"

HwGkd350h::HwGkd350h() : IHardware() {
    TRACE("enter");

    this->BLOCK_DEVICE = "/sys/block/mmcblk1/size";
    this->INTERNAL_MOUNT_DEVICE = "/dev/mmcblk0";
    this->EXTERNAL_MOUNT_DEVICE = "/dev/mmcblk1p1";
    this->EXTERNAL_MOUNT_FORMAT = "auto";
    this->EXTERNAL_MOUNT_POINT = EXTERNAL_CARD_PATH;

    this->clock_ = (IClock *) new RTC();
    this->soundcard_ = (ISoundcard *) new AlsaSoundcard("default", "Master");
    this->cpu_ = (ICpu *) new X1830Cpu();
    this->power_ = (IPower *)new JzPower();
    this->led_ = (ILed *)new DummyLed();

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
    delete this->power_;
    delete this->led_;
}

bool HwGkd350h::getTVOutStatus() { return false; }

void HwGkd350h::setTVOutMode(std::string mode) { return; }

std::string HwGkd350h::getTVOutMode() { return ""; }

int HwGkd350h::getBacklightLevel() {
    TRACE("enter");
    if (this->pollBacklight) {
        int level = 0;
        //force  scale 0 - 100
        std::string result = FileUtils::fileReader(BACKLIGHT_PATH);
        if (result.length() > 0) {
            level = ceil(atoi(StringUtils::trim(result).c_str()));
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

    if (FileUtils::fileWriter(BACKLIGHT_PATH, val)) {
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
