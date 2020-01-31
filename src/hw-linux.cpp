
#include <signal.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "hw-linux.h"
#include "hw-power.h"
#include "hw-clock.h"

HwLinux::HwLinux() : IHardware() {

    this->clock_ = (IClock *) new SysClock();
    this->soundcard_ = (ISoundcard *) new AlsaSoundcard("default", "Master");
    this->cpu_ = (ICpu *) new X1830Cpu();
    this->power_ = (IPower *)new GenericPower();

    this->getBacklightLevel();
    this->getKeepAspectRatio();
    TRACE(
        "brightness: %i, volume : %i",
        this->getBacklightLevel(),
        this->soundcard_->getVolume());

}
HwLinux::~HwLinux() {
    delete this->clock_;
    delete this->cpu_;
    delete this->soundcard_;
    delete this->power_;
}

bool HwLinux::getTVOutStatus() { return 0; }
std::string HwLinux::getTVOutMode() { return "OFF"; }
void HwLinux::setTVOutMode(std::string mode) {
    std::string val = mode;
    if (val != "NTSC" && val != "PAL") val = "OFF";
}

void HwLinux::ledOn(int flashSpeed) { return; }
void HwLinux::ledOff() { return; }

int HwLinux::getBacklightLevel() { return 100; };
int HwLinux::setBacklightLevel(int val) { return val; }

bool HwLinux::getKeepAspectRatio() { return true; }
bool HwLinux::setKeepAspectRatio(bool val) { return val; }

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

bool HwLinux::setScreenState(const bool &enable) { return true; }
