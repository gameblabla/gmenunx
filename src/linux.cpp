
#include <signal.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "linux.h"
#include "sysclock.h"

HwLinux::HwLinux() {
    this->performanceModes_.insert({"ondemand", "On demand"});
    this->performanceModes_.insert({"performance", "Performance"});

    this->clock_ = new SysClock();
}
HwLinux::~HwLinux() {
    delete this->clock_;
}

IClock *HwLinux::Clock() { return (IClock *)this->clock_; };

bool HwLinux::getTVOutStatus() { return 0; };
std::string HwLinux::getTVOutMode() { return "OFF"; }
void HwLinux::setTVOutMode(std::string mode) {
    std::string val = mode;
    if (val != "NTSC" && val != "PAL") val = "OFF";
}

std::string HwLinux::getPerformanceMode() {
    TRACE("enter");
    return this->performanceModeMap(this->performanceMode_);
}
void HwLinux::setPerformanceMode(std::string alias) {
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
        this->performanceMode_ = mode;
    } else {
        TRACE("nothing to do");
    }
    TRACE("exit");
}
std::vector<std::string> HwLinux::getPerformanceModes() {
    std::vector<std::string> results;
    std::unordered_map<std::string, std::string>::iterator it;
    for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
        results.push_back(it->second);
    }
    return results;
}

bool HwLinux::supportsOverClocking() { return false; }
bool HwLinux::setCPUSpeed(uint32_t mhz) { return true; };
std::vector<uint32_t> HwLinux::cpuSpeeds() { return {}; };
uint32_t HwLinux::getCpuDefaultSpeed() { return 0; };

void HwLinux::ledOn(int flashSpeed) { return; };
void HwLinux::ledOff() { return; };

int HwLinux::getBatteryLevel() { return 100; };
int HwLinux::getVolumeLevel() { return 100; };
int HwLinux::setVolumeLevel(int val) { return val; };
;

int HwLinux::getBacklightLevel() { return 100; };
int HwLinux::setBacklightLevel(int val) { return val; };

bool HwLinux::getKeepAspectRatio() { return true; };
bool HwLinux::setKeepAspectRatio(bool val) { return val; };

std::string HwLinux::getDeviceType() { return "Linux"; }

void HwLinux::powerOff() {
    TRACE("enter - powerOff");
    raise(SIGTERM);
}
void HwLinux::reboot() {
    TRACE("enter");
    return;
}

int HwLinux::defaultScreenWidth() { return 320; }
int HwLinux::defaultScreenHeight() { return 240; }

bool HwLinux::setScreenState(const bool &enable) { return true; };

std::string HwLinux::performanceModeMap(std::string fromInternal) {
    std::unordered_map<string, string>::iterator it;
    for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
        if (fromInternal == it->first) {
            return it->second;
            ;
        }
    }
    return "On demand";
}