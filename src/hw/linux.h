#ifndef _HWLINUX_
#define _HWLINUX_

#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <signal.h>

#include "ihardware.h"
#include "../constants.h"

class HwLinux : IHardware {
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
        HwLinux() {
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
            std::string val = mode;
            if (val != "NTSC" && val != "PAL") val = "OFF";
        }

        std::string getPerformanceMode() { 
            TRACE("enter");
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

        bool getKeepAspectRatio() { return true; };
        bool setKeepAspectRatio(bool val) { return val; };

        std::string getDeviceType() { return "Linux"; }

        void powerOff() { 
            TRACE("enter - powerOff");
            raise(SIGTERM);
        }
        void reboot() { 
            TRACE("enter");
            return; 
        }

        int defaultScreenWidth() { return 320; }
        int defaultScreenHeight() { return 240; }

        bool setScreenState(const bool &enable) { return true; };
};

#endif