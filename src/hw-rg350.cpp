
#include <math.h>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "constants.h"
#include "fileutils.h"
#include "stringutils.h"
#include "debug.h"
#include "hw-rg350.h"
#include "hw-cpu.h"
#include "hw-power.h"
#include "hw-clock.h"
#include "hw-led.h"

HwRg350::HwRg350() : IHardware() {
    TRACE("enter");
    this->BLOCK_DEVICE = "/sys/block/mmcblk1/size";
    this->INTERNAL_MOUNT_DEVICE = "/dev/mmcblk0";
    this->EXTERNAL_MOUNT_DEVICE = "/dev/mmcblk1p1";
    this->EXTERNAL_MOUNT_FORMAT = "auto";
    this->EXTERNAL_MOUNT_POINT = EXTERNAL_CARD_PATH;

    this->clock_ = (IClock *)new RTC();
    this->soundcard_ = (ISoundcard *)new AlsaSoundcard("default", "PCM");
    this->cpu_ = JZ4770Factory::getCpu();
    this->power_ = (IPower *)new JzPower();
    this->led_ = (ILed *)new Rg350Led();

    this->pollBacklight = FileUtils::fileExists(BACKLIGHT_PATH);

    this->getBacklightLevel();
    this->getKeepAspectRatio();
    this->resetKeymap();

    TRACE(
        "brightness: %i, volume : %i",
        this->getBacklightLevel(),
        this->soundcard_->getVolume());
}
HwRg350::~HwRg350() {
    delete this->clock_;
    delete this->cpu_;
    delete this->soundcard_;
    delete this->power_;
    delete this->led_;
}

int HwRg350::getBacklightLevel() {
    TRACE("enter");
    if (this->pollBacklight) {
        int level = 0;
        //force  scale 0 - 100
        std::string result = FileUtils::fileReader(BACKLIGHT_PATH);
        if (result.length() > 0) {
            level = ceil(atoi(StringUtils::trim(result).c_str()) / 2.55);
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
    if (val == this->backlightLevel_)
        return val;

    int deviceVal = (int)(val * (255.0f / 100));
    TRACE("device value : %i", deviceVal);

    if (FileUtils::fileWriter(BACKLIGHT_PATH, deviceVal)) {
        TRACE("success");
    } else {
        ERROR("Couldn't update backlight value to : %i", deviceVal);
    }
    this->backlightLevel_ = val;
    return this->backlightLevel_;
}

bool HwRg350::getKeepAspectRatio() {
    TRACE("enter");
    if (FileUtils::fileExists(ASPECT_RATIO_PATH)) {
        std::string result = FileUtils::fileReader(ASPECT_RATIO_PATH);
        TRACE("raw result : '%s'", result.c_str());
        if (result.length() > 0) {
            result = result[0];
            result = StringUtils::toLower(result);
        }
        this->keepAspectRatio_ = ("y" == result);
    }
    TRACE("exit : %i", this->keepAspectRatio_);
    return this->keepAspectRatio_;
}
bool HwRg350::setKeepAspectRatio(bool val) {
    TRACE("enter - %i", val);
    std::string payload = val ? "Y" : "N";
    if (FileUtils::fileWriter(ASPECT_RATIO_PATH, payload)) {
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
    return this->writeValueToFile(path, blank);
}

std::string HwRg350::systemInfo() {
    TRACE("append - command /usr/bin/system_info");
    if (FileUtils::fileExists("/usr/bin/system_info")) {
        return FileUtils::execute("/usr/bin/system_info") + "\n";
    }
    return IHardware::systemInfo();
}

void HwRg350::resetKeymap() {
    this->writeValueToFile(ALT_KEYMAP_FILE, "N");
}
