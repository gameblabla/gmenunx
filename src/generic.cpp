#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "constants.h"
#include "generic.h"
#include "sysclock.h"

std::string HwGeneric::performanceModeMap(std::string fromInternal) {
    std::unordered_map<string, string>::iterator it;
    for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
        if (fromInternal == it->first) {
            return it->second;
            ;
        }
    }
    return "On demand";
}

HwGeneric::HwGeneric() {
    this->performanceModes_.insert({"ondemand", "On demand"});
    this->performanceModes_.insert({"performance", "Performance"});

    this->clock_ = new SysClock();
};
HwGeneric::~HwGeneric() {
    delete this->clock_;
};

IClock *HwGeneric::Clock() { return (IClock *)this->clock_; };

bool HwGeneric::getTVOutStatus() { return 0; };
std::string HwGeneric::getTVOutMode() { return "OFF"; }
void HwGeneric::setTVOutMode(std::string mode) {
    std::string val = mode;
    if (val != "NTSC" && val != "PAL") val = "OFF";
}

bool HwGeneric::supportsPowerGovernors() {
    return true;
}
std::string HwGeneric::getPerformanceMode() {
    TRACE("enter");
    return this->performanceModeMap(this->performanceMode_);
}
void HwGeneric::setPerformanceMode(std::string alias) {
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
std::vector<std::string> HwGeneric::getPerformanceModes() {
    std::vector<std::string> results;
    std::unordered_map<std::string, std::string>::iterator it;
    for (it = this->performanceModes_.begin(); it != this->performanceModes_.end(); it++) {
        results.push_back(it->second);
    }
    return results;
}

bool HwGeneric::supportsOverClocking() { return false; }
uint32_t HwGeneric::getCPUSpeed() { return 0; };
bool HwGeneric::setCPUSpeed(uint32_t mhz) { return true; };

uint32_t HwGeneric::getCpuDefaultSpeed() { return 0; };

void HwGeneric::ledOn(int flashSpeed) { return; };
void HwGeneric::ledOff() { return; };

int HwGeneric::getBatteryLevel() { return IHardware::BATTERY_CHARGING; };
int HwGeneric::getVolumeLevel() { return 100; };
int HwGeneric::setVolumeLevel(int val) { return val; };
;

int HwGeneric::getBacklightLevel() { return 100; };
int HwGeneric::setBacklightLevel(int val) { return val; };

bool HwGeneric::getKeepAspectRatio() { return true; };
bool HwGeneric::setKeepAspectRatio(bool val) { return val; };

std::string HwGeneric::getDeviceType() { return "Generic"; }

bool HwGeneric::setScreenState(const bool &enable) { return true; };
