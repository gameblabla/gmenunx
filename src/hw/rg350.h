#ifndef _HWRG350_
#define _HWRG350_

#include <string>
#include <unordered_map>
#include <math.h>
#include <vector>
#include <algorithm>
#include <time.h>
#include <sstream>

#include "ihardware.h"
#include "../rtc.h"
#include "../constants.h"

class HwRg350 : IHardware {
    private:

		enum LedAllowedTriggers {
			NONE = 0, 
			TIMER
		};

        RTC *rtc;

        std::unordered_map<std::string, std::string> performanceModes_;
		std::string ledMaxBrightness_;
        std::string performanceMode_ = "ondemand";
        const std::string defaultPerformanceMode = "ondemand";
        int volumeLevel_ = 0;
        int backlightLevel_ = 0;
        bool keepAspectRatio_ = false;
        const std::string SCREEN_BLANK_PATH = "/sys/class/graphics/fb0/blank";
		const std::string LED_PREFIX = "/sys/class/leds/power/";
		const std::string LED_MAX_BRIGHTNESS_PATH = LED_PREFIX + "max_brightness";
		const std::string LED_BRIGHTNESS_PATH = LED_PREFIX + "brightness";
		const std::string LED_DELAY_ON_PATH = LED_PREFIX + "delay_on";
		const std::string LED_DELAY_OFF_PATH = LED_PREFIX + "delay_off";
		const std::string LED_TRIGGER_PATH = LED_PREFIX + "trigger";
        const std::string GET_VOLUME_PATH = "/usr/bin/alsa-getvolume default PCM";
        const std::string SET_VOLUME_PATH = "/usr/bin/alsa-setvolume default PCM "; // keep trailing space
        const std::string BACKLIGHT_PATH = "/sys/class/backlight/pwm-backlight/brightness";
        const std::string ASPECT_RATIO_PATH = "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio";

        std::string performanceModeMap(std::string fromInternal) {
            std::unordered_map<std::string, std::string>::iterator it;
            for (it = this->performanceModes_.begin();it != this->performanceModes_.end(); it++) {
                if (fromInternal == it->first) {
                    return it->second;;
                }
            }
            return "On demand";
        }

		std::string triggerToString(LedAllowedTriggers t) {
            TRACE("mode : %i", t);
            switch(t) {
                case TIMER:
                    return "timer";
                    break;
                default:
                    return "none";
                    break;
            };
        }
    
    protected:

     public:
        HwRg350() {
            TRACE("enter");
            this->BLOCK_DEVICE = "/sys/block/mmcblk1/size";
            this->INTERNAL_MOUNT_DEVICE = "/dev/mmcblk0";
            this->EXTERNAL_MOUNT_DEVICE = "/dev/mmcblk1p1";
            this->EXTERNAL_MOUNT_FORMAT = "auto";
            this->EXTERNAL_MOUNT_POINT = EXTERNAL_CARD_PATH;

            this->rtc = new RTC();

            this->ledMaxBrightness_ = fileReader(LED_MAX_BRIGHTNESS_PATH);
            this->performanceModes_.insert({"ondemand", "On demand"});
            this->performanceModes_.insert({"performance", "Performance"});

            this->getBacklightLevel();
            this->getVolumeLevel();
            this->getKeepAspectRatio();

            TRACE(
                "brightness - max : %s, current : %i, volume : %i", 
                ledMaxBrightness_.c_str(), 
                this->backlightLevel_, 
                this->volumeLevel_
            );
        }
        ~HwRg350() {
            delete rtc;
        }

        std::string getSystemdateTime() {
            this->rtc->refresh();
            return this->rtc->getDateTime();
        };
        bool setSystemDateTime(std::string datetime) {
            return this->rtc->setTime(datetime);
        }

        bool getTVOutStatus() { return 0; };
        std::string getTVOutMode() { return "OFF"; }
        void setTVOutMode(std::string mode) {
            std::string val = mode;
            if (val != "NTSC" && val != "PAL") val = "OFF";
        }

        std::string getPerformanceMode() { 
            TRACE("enter");
            this->performanceMode_ = "ondemand";
            if (fileExists("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")) {
                string rawValue = fileReader("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                this->performanceMode_ = full_trim(rawValue);
            }
            TRACE("read - %s", this->performanceMode_.c_str());
            return this->performanceModeMap(this->performanceMode_);
        }
        void setPerformanceMode(std::string alias = "") {
            TRACE("raw desired : %s", alias.c_str());
            std::string mode = this->defaultPerformanceMode;
            if (!alias.empty()) {
                std::unordered_map<std::string, std::string>::iterator it;
                for (it = this->performanceModes_.begin();it != this->performanceModes_.end(); it++) {
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
        std::vector<std::string> getPerformanceModes() {
            std::vector<std::string> results;
            std::unordered_map<std::string, std::string>::iterator it;
            for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
                results.push_back(it->second);
            }
            return results;
        }

        uint32_t setCPUSpeed(uint32_t mhz) { return mhz; };

        void ledOn(int flashSpeed = 250) {
            TRACE("enter");
            int limited = constrain(flashSpeed, 0, atoi(ledMaxBrightness_.c_str()));
            std::string trigger = triggerToString(LedAllowedTriggers::TIMER);
            TRACE("mode : %s - for %i", trigger.c_str(), limited);
            procWriter(LED_TRIGGER_PATH, trigger);
            procWriter(LED_DELAY_ON_PATH, limited);
            procWriter(LED_DELAY_OFF_PATH, limited);
            TRACE("exit");
        };
        void ledOff() { 
            TRACE("enter");
            std::string trigger = triggerToString(LedAllowedTriggers::NONE);
            TRACE("mode : %s", trigger.c_str());
            procWriter(LED_TRIGGER_PATH, trigger);
            procWriter(LED_BRIGHTNESS_PATH, ledMaxBrightness_);
            TRACE("exit");
            return;
        };

        int getBatteryLevel() { 
            int online, result = 0;
            sscanf(fileReader("/sys/class/power_supply/usb/online").c_str(), "%i", &online);
            if (online) {
                result = 6;
            } else {
                int battery_level = 0;
                sscanf(fileReader("/sys/class/power_supply/battery/capacity").c_str(), "%i", &battery_level);
                TRACE("raw battery level - %i", battery_level);
                if (battery_level >= 100) result = 5;
                else if (battery_level > 80) result = 4;
                else if (battery_level > 60) result = 3;
                else if (battery_level > 40) result = 2;
                else if (battery_level > 20) result = 1;
                else result = 0;
            }
            TRACE("scaled battery level : %i", result);
            return result;
        };

        int getVolumeLevel() { 
            TRACE("enter");
            int vol = -1;
            std::string result = exec(GET_VOLUME_PATH.c_str());
            if (result.length() > 0) {
                vol = atoi(trim(result).c_str());
            }
            // scale 0 - 31, turn to percent
            vol = ceil(vol * 100 / 31);
            this->volumeLevel_ = vol;
            TRACE("exit : %i", this->volumeLevel_);
            return this->volumeLevel_;
        };
        int setVolumeLevel(int val) { 
            TRACE("enter - %i", val);
            if (val < 0) val = 100;
            else if (val > 100) val = 0;
            if (val == this->volumeLevel_) 
                return val;
            int rg350val = (int)(val * (31.0f/100));
            TRACE("rg350 value : %i", rg350val);
            std::stringstream ss;
            std::string cmd;
            ss << SET_VOLUME_PATH << rg350val;
            std::getline(ss, cmd);
            TRACE("cmd : %s", cmd.c_str());
            std::string result = exec(cmd.c_str());
            TRACE("result : %s", result.c_str());
            this->volumeLevel_ = val;
            return val;
        };

        int getBacklightLevel() { 
            TRACE("enter");
            int level = 0;
            //force  scale 0 - 100
            std::string result = fileReader(BACKLIGHT_PATH);
            if (result.length() > 0) {
                level = ceil(atoi(trim(result).c_str()) / 2.55);
            }
            this->backlightLevel_ = level;
            TRACE("exit : %i", this->backlightLevel_);
            return this->backlightLevel_;
        };
        int setBacklightLevel(int val) { 
            TRACE("enter - %i", val);
            // wrap it
            if (val <= 0) val = 100;
            else if (val > 100) val = 0;
            int rg350val = (int)(val * (255.0f/100));
            TRACE("rg350 value : %i", rg350val);
            // save a write
            if (rg350val == this->backlightLevel_) 
                return val;

            if (procWriter(BACKLIGHT_PATH, rg350val)) {
                TRACE("success");
            } else {
                ERROR("Couldn't update backlight value to : %i", rg350val);
            }
            this->backlightLevel_ = val;
            return this->backlightLevel_;	
        };

        bool getKeepAspectRatio() {
            TRACE("enter");
            std::string result = fileReader(ASPECT_RATIO_PATH);
            TRACE("raw result : '%s'", result.c_str());
            if (result.length() > 0) {
                result = result[0];
                result = toLower(result);
            }
            this->keepAspectRatio_ = ("y" == result);
            TRACE("exit : %i", this->keepAspectRatio_);
            return this->keepAspectRatio_;
        }
        bool setKeepAspectRatio(bool val) {
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
        std::string getDeviceType() { return "RG-350"; }

        bool setScreenState(const bool &enable) {
            TRACE("enter : %s", (state ? "on" : "off"));
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
                } else result = true;
                close(fd);
            }
            return result;
        }
};

#endif
