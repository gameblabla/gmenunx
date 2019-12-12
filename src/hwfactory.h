#ifndef _HWFACTORY_
#define _HWFACTORY_

// getDiskSize
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

// systemDateTime
#include <ctime>

#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <chrono>
#include <time.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <math.h>

#include "utilities.h"
#include "constants.h"
#include "debug.h"
#include "rtc.h"

class IHardware {

    protected:

        int16_t curMMCStatus;

        std::string BLOCK_DEVICE;
        std::string INTERNAL_MOUNT_DEVICE;
        std::string EXTERNAL_MOUNT_DEVICE;
        std::string EXTERNAL_MOUNT_POINT;
        std::string EXTERNAL_MOUNT_FORMAT;// = "auto";

    public:

        enum CARD_STATUS:int16_t {
            MMC_MOUNTED, MMC_UNMOUNTED, MMC_MISSING, MMC_ERROR
        };

        virtual std::string getSystemdateTime() = 0;
        virtual bool setSystemDateTime(std::string datetime) = 0;

        virtual bool getTVOutStatus() = 0;
        virtual void setTVOutMode(string mode) = 0;
        virtual std::string getTVOutMode() = 0;

        virtual void setPerformanceMode(std::string alias = "") = 0;
        virtual std::string getPerformanceMode() = 0;
        virtual std::vector<std::string>getPerformanceModes() = 0;

        virtual uint32_t setCPUSpeed(uint32_t mhz) = 0;

        virtual void ledOn(int flashSpeed = 250) = 0;
        virtual void ledOff() = 0;

        /*!
        Reads the current battery state and returns a number representing it's level of charge
        @return A number representing battery charge. 0 means fully discharged. 
        5 means fully charged. 6 represents charging.
        */
        virtual int getBatteryLevel() = 0;

        /*!
        Gets or sets the devices volume level, scale 0 - 100
        */
        virtual int getVolumeLevel() = 0;
        virtual int setVolumeLevel(int val) = 0;

        /*!
        Gets or sets the devices backlight level, scale 0 - 100
        */
        virtual int getBacklightLevel() = 0;
        virtual int setBacklightLevel(int val) = 0;

        string mountSd() {
            TRACE("enter");
            string command = "mount -t " + EXTERNAL_MOUNT_FORMAT + " " + EXTERNAL_MOUNT_DEVICE + " " + EXTERNAL_MOUNT_POINT + " 2>&1";
            string result = exec(command.c_str());
            TRACE("result : %s", result.c_str());
            system("sleep 1");
            this->checkUDC();
            return result;
        }

        string umountSd() {
            sync();
            string command = "umount -fl " + EXTERNAL_MOUNT_POINT + " 2>&1";
            string result = exec(command.c_str());
            system("sleep 1");
            this->checkUDC();
            return result;
        }

        void checkUDC() {
            TRACE("enter");
            this->curMMCStatus = MMC_ERROR;
            unsigned long size;
            TRACE("reading block device : %s", BLOCK_DEVICE.c_str());
            std::ifstream fsize(BLOCK_DEVICE.c_str(), std::ios::in | std::ios::binary);
            if (fsize >> size) {
                if (size > 0) {
                    // ok, so we're inserted, are we mounted
                    TRACE("size was : %lu, reading /proc/mounts", size);
                    std::ifstream procmounts( "/proc/mounts" );
                    if (!procmounts) {
                        this->curMMCStatus = MMC_ERROR;
                        WARNING("couldn't open /proc/mounts");
                    } else {
                        std::string line;
                        std::size_t found;
                        this->curMMCStatus = MMC_UNMOUNTED;
                        while (std::getline(procmounts, line)) {
                            if ( !(procmounts.fail() || procmounts.bad()) ) {
                                found = line.find("mcblk1");
                                if (found != std::string::npos) {
                                    this->curMMCStatus = MMC_MOUNTED;
                                    TRACE("inserted && mounted because line : %s", line.c_str());
                                    break;
                                }
                            } else {
                                this->curMMCStatus = MMC_ERROR;
                                WARNING("error reading /proc/mounts");
                                break;
                            }
                        }
                        procmounts.close();
                    }
                } else {
                    this->curMMCStatus = MMC_MISSING;
                    TRACE("not inserted");
                }
            } else {
                this->curMMCStatus = MMC_ERROR;
                WARNING("error, no card present");
            }
            fsize.close();
            TRACE("exit - %i",  this->curMMCStatus);
        }

        CARD_STATUS getCardStatus() {
            return (CARD_STATUS)this->curMMCStatus;
        }

        bool formatSdCard() {
            this->checkUDC();
            if (this->curMMCStatus == CARD_STATUS::MMC_MOUNTED) {
                this->umountSd();
            }
            if (this->curMMCStatus != CARD_STATUS::MMC_UNMOUNTED) {
                return false;
            }
            // TODO :: implement me
        }

        string getDiskFree(const char *path) {
            TRACE("enter - %s", path);
            string df = "N/A";
            struct statvfs b;

            if (statvfs(path, &b) == 0) {
                TRACE("read statvfs ok");
                // Make sure that the multiplication happens in 64 bits.
                uint32_t freeMiB = ((uint64_t)b.f_bfree * b.f_bsize) / (1024 * 1024);
                uint32_t totalMiB = ((uint64_t)b.f_blocks * b.f_frsize) / (1024 * 1024);
                TRACE("raw numbers - free: %lu, total: %lu, block size: %lu", b.f_bfree, b.f_blocks, b.f_bsize);
                std::stringstream ss;
                if (totalMiB >= 10000) {
                    ss << (freeMiB / 1024) << "." << ((freeMiB % 1024) * 10) / 1024 << " GiB";
                } else {
                    ss << freeMiB << " MiB";
                }
                std::getline(ss, df);
            } else WARNING("statvfs failed with error '%s'.", strerror(errno));
            TRACE("exit");
            return df;
        }

        string getDiskSize(const std::string &mountDevice) {
            TRACE("reading disk size for : %s", mountDevice.c_str());
            string result = "0 GiB";
            if (mountDevice.empty())
                return result;

            unsigned long long size;
            int fd = open(mountDevice.c_str(), O_RDONLY);
            ioctl(fd, BLKGETSIZE64, &size);
            close(fd);
            std::stringstream ss;
            std::uint64_t totalMiB = (size >> 20);
            if (totalMiB >= 10000) {
                ss << (totalMiB / 1024) << "." << ((totalMiB % 1024) * 10) / 1024 << " GiB";
            } else {
                ss << totalMiB << " MiB";
            }
            std::getline(ss, result);
            return result;
        }

        std::string getExternalMountDevice() {
            return this->EXTERNAL_MOUNT_DEVICE;
        }

        std::string getInternalMountDevice() {
            return this->INTERNAL_MOUNT_DEVICE;
        }

        std::string uptime() {
            string result = "";
            std::chrono::milliseconds uptime(0u);
            struct sysinfo x;
            if (sysinfo(&x) == 0) {
                uptime = std::chrono::milliseconds(
                    static_cast<unsigned long long>(x.uptime) * 1000ULL
                );

                int secs = uptime.count() / 1000;
                int s = (secs % 60);
                int m = (secs % 3600) / 60;
                int h = (secs % 86400) / 3600;

                static char buf[10];
                sprintf(buf, "%02d:%02d:%02d", h, m, s);
                result = buf;
            }
            return result;
        }
};

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
		const string LED_PREFIX = "/sys/class/leds/power/";
		const string LED_MAX_BRIGHTNESS_PATH = LED_PREFIX + "max_brightness";
		const string LED_BRIGHTNESS_PATH = LED_PREFIX + "brightness";
		const string LED_DELAY_ON_PATH = LED_PREFIX + "delay_on";
		const string LED_DELAY_OFF_PATH = LED_PREFIX + "delay_off";
		const string LED_TRIGGER_PATH = LED_PREFIX + "trigger";
        const string GET_VOLUME_PATH = "/usr/bin/alsa-getvolume default PCM";
        const string SET_VOLUME_PATH = "/usr/bin/alsa-setvolume default PCM "; // keep trailing space
        const string BACKLIGHT_PATH = "/sys/class/backlight/pwm-backlight/brightness";

        std::string performanceModeMap(std::string fromInternal) {
            std::unordered_map<string, string>::iterator it;
            for (it = this->performanceModes_.begin();it != this->performanceModes_.end(); it++) {
                if (fromInternal == it->first) {
                    return it->second;;
                }
            }
            return "On demand";
        }

		string triggerToString(LedAllowedTriggers t) {
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
            this->getBacklightLevel();
            this->getVolumeLevel();
            this->performanceModes_.insert({"ondemand", "On demand"});
            this->performanceModes_.insert({"performance", "Performance"});
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
            string val = mode;
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
            string mode = this->defaultPerformanceMode;
            if (!alias.empty()) {
                std::unordered_map<string, string>::iterator it;
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
            string trigger = triggerToString(LedAllowedTriggers::TIMER);
            TRACE("mode : %s - for %i", trigger.c_str(), limited);
            procWriter(LED_TRIGGER_PATH, trigger);
            procWriter(LED_DELAY_ON_PATH, limited);
            procWriter(LED_DELAY_OFF_PATH, limited);
            TRACE("exit");
        };
        void ledOff() { 
            TRACE("enter");
            string trigger = triggerToString(LedAllowedTriggers::NONE);
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
            string result = fileReader(BACKLIGHT_PATH);
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
};

class HwGeneric : IHardware {
    private:


        std::string performanceModeMap(std::string fromInternal) {
            std::unordered_map<string, string>::iterator it;
            for (it = this->performanceModes_.begin();it != this->performanceModes_.end(); it++) {
                if (fromInternal == it->first) {
                    return it->second;;
                }
            }
            return "On demand";
        }
    
        std::unordered_map<std::string, std::string> performanceModes_;
        std::string performanceMode_ = "ondemand";
        const std::string defaultPerformanceMode = "ondemand";

    public:
        HwGeneric() {
            this->performanceModes_.insert({"ondemand", "On demand"});
            this->performanceModes_.insert({"performance", "Performance"});
        };

        std::string getSystemdateTime() { 
            time_t now = time(0);
            char* dt = ctime(&now);
            tm *gmtm = gmtime(&now);
            dt = asctime(gmtm);
            return dt;
        };
        bool setSystemDateTime(std::string datetime) { return true; };

        bool getTVOutStatus() { return 0; };
        std::string getTVOutMode() { return "OFF"; }
        void setTVOutMode(std::string mode) {
            string val = mode;
            if (val != "NTSC" && val != "PAL") val = "OFF";
        }

        std::string getPerformanceMode() { 
            TRACE("enter");
            return this->performanceModeMap(this->performanceMode_);
        }
        void setPerformanceMode(std::string alias = "") {
            TRACE("raw desired : %s", alias.c_str());
            string mode = this->defaultPerformanceMode;
            if (!alias.empty()) {
                std::unordered_map<string, string>::iterator it;
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
                this->performanceMode_ = mode;
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
        //std::vector<std::string>getPerformanceModes() { return vector<string> { "Default"}; }
        uint32_t setCPUSpeed(uint32_t mhz) { return mhz; };

        void ledOn(int flashSpeed = 250) { return; };
        void ledOff() { return; };

        int getBatteryLevel() { return 100; };
        int getVolumeLevel() { return 100; };
        int setVolumeLevel(int val) { return val; };;

        int getBacklightLevel() { return 100; };
        int setBacklightLevel(int val) { return val; };
};

class HwFactory {
    public:
        static IHardware * GetHardware() {
            #ifdef TARGET_RG350
            return (IHardware*)new HwRg350();
            #else
            return (IHardware*)new HwGeneric();
            #endif
        }
};

#endif // _HWFACTORY_