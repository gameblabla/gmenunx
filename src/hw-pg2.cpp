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
#include "hw-pg2.h"
#include "hw-cpu.h"
#include "hw-power.h"
#include "hw-clock.h"

HwPG2::HwPG2() : IHardware() {
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

    this->reverse_ = true;
    std::string issue = FileUtils::fileReader("/etc/issue");
    if (!issue.empty()) {
        issue = StringUtils::toLower(issue);
        this->rogue_ = (std::string::npos != issue.find("rogue"));
        this->reverse_ = !this->rogue_;
    }

    this->max_ = 255;
    if (FileUtils::fileExists(BACKLIGHT_MAX_PATH)) {
        std::string result = FileUtils::fileReader(BACKLIGHT_MAX_PATH);
        if (!result.empty()) {
            this->max_ = atoi(result.c_str());
        }
    }
    this->pollBacklight_ = FileUtils::fileExists(BACKLIGHT_PATH);

    this->getBacklightLevel();
    this->getKeepAspectRatio();
    this->resetKeymap();

    TRACE(
        "brightness: %i, volume : %i",
        this->getBacklightLevel(),
        this->soundcard_->getVolume()
    );
}

HwPG2::~HwPG2() {
    delete this->clock_;
    delete this->cpu_;
    delete this->soundcard_;
    delete this->power_;
    delete this->led_;
}

bool HwPG2::getTVOutStatus() { return 0; };
std::string HwPG2::getTVOutMode() { return "OFF"; }
void HwPG2::setTVOutMode(std::string mode) {
    std::string val = mode;
    if (val != "NTSC" && val != "PAL") val = "OFF";
}

int HwPG2::getBacklightLevel() {
    TRACE("enter");
    try {
        if (this->pollBacklight_) {
            int level = 0;
            //force  scale 0 - 100
            std::string result = FileUtils::fileReader(BACKLIGHT_PATH);
            if (result.length() > 0) {
                int rawLevel = atoi(StringUtils::trim(result).c_str());
                level = (int)rawLevel / ((float)this->max_ / 100.0f);
                if (this->reverse_) {
                    level = (level * -1) + 100;
                }
                level = constrain(level, 1, 100);
            }
            this->backlightLevel_ = level;
        }
    } catch (std::exception e) {
        ERROR("caught : '%s'", e.what());
    } catch (...) {
        ERROR("unknown error");
    }
    TRACE("exit : %i", this->backlightLevel_);
    return this->backlightLevel_;
}
int HwPG2::setBacklightLevel(int val) {
    TRACE("enter - %i", val);
    try {
        val = constrain(val, 1, 100);
        if (val == this->backlightLevel_)
            return val;

        int localVal = (int)(val * ((float)this->max_ / 100));
        if (this->reverse_) {
            localVal = (localVal * -1) + this->max_;
        }
        localVal = constrain(localVal, 1, this->max_ - 1);
        TRACE("device value : %i", localVal);

        if (FileUtils::fileWriter(BACKLIGHT_PATH, localVal)) {
            TRACE("success");
        } else {
            ERROR("Couldn't update backlight value to : %i", localVal);
        }
    } catch (std::exception e) {
        ERROR("caught : '%s'", e.what());
    } catch (...) {
        ERROR("unknown error");
    }
    this->backlightLevel_ = val;
    return this->backlightLevel_;
}

bool HwPG2::getKeepAspectRatio() {
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
bool HwPG2::setKeepAspectRatio(bool val) {
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

std::string HwPG2::getDeviceType() {
    std::string version = this->rogue_ ? "rogue" : "stock";
    return "PocketGo 2 (" + version + ")"; 
}

bool HwPG2::setScreenState(const bool &enable) {
    TRACE("enter : %s", (enable ? "on" : "off"));
    const char *path = SCREEN_BLANK_PATH.c_str();
    const char *blank = enable ? "0" : "1";
    return this->writeValueToFile(path, blank);
}

std::string HwPG2::systemInfo() {
    TRACE("append - command /usr/bin/system_info");
    if (FileUtils::fileExists("/usr/bin/system_info")) {
        return FileUtils::execute("/usr/bin/system_info") + "\n";
    }
    return IHardware::systemInfo();
}

void HwPG2::resetKeymap() {
    this->writeValueToFile(ALT_KEYMAP_FILE, "N");
}
