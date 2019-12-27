#include <string>
#include <vector>

#include "constants.h"
#include "gkd350h.h"
#include "sysclock.h"

HwGkd350h::HwGkd350h() {
    this->clock_ = new SysClock();
}
HwGkd350h::~HwGkd350h() {
    delete this->clock_;
}

IClock *HwGkd350h::Clock() { return (IClock *)this->clock_; };

bool HwGkd350h::getTVOutStatus() { return false; }

void HwGkd350h::setTVOutMode(std::string mode) { return; }

std::string HwGkd350h::getTVOutMode() { return ""; }

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

int HwGkd350h::getBatteryLevel() { return BATTERY_CHARGING; }

int HwGkd350h::getVolumeLevel() { return 100; }

int HwGkd350h::setVolumeLevel(int val) { return val; }

int HwGkd350h::getBacklightLevel() { return 100; }

int HwGkd350h::setBacklightLevel(int val) { return val; }

bool HwGkd350h::getKeepAspectRatio() { return true; }

bool HwGkd350h::setKeepAspectRatio(bool val) { return val; }

std::string HwGkd350h::getDeviceType() { return "GKD-350h"; }

bool HwGkd350h::setScreenState(const bool &enable) { return enable; }
