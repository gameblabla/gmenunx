#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "constants.h"
#include "hw-generic.h"
#include "hw-power.h"
#include "hw-clock.h"

HwGeneric::HwGeneric() : IHardware() {

    this->clock_ = (IClock *)new SysClock();
    this->soundcard_ = (ISoundcard *)new DummySoundcard();
    this->cpu_ = (ICpu *)new DefaultCpu();
    this->power_ = (IPower *)new GenericPower();
    this->led_ = (ILed *)new DummyLed();

    TRACE(
        "brightness: %i, volume : %i",
        this->getBacklightLevel(),
        this->soundcard_->getVolume());
}
HwGeneric::~HwGeneric() {
    delete this->clock_;
    delete this->cpu_;
    delete this->soundcard_;
    delete this->power_;
    delete this->led_;
}

bool HwGeneric::getTVOutStatus() { return 0; }
std::string HwGeneric::getTVOutMode() { return "OFF"; }
void HwGeneric::setTVOutMode(std::string mode) {
    std::string val = mode;
    if (val != "NTSC" && val != "PAL") val = "OFF";
}

int HwGeneric::getBacklightLevel() { return 100; }
int HwGeneric::setBacklightLevel(int val) { return val; }

bool HwGeneric::getKeepAspectRatio() { return true; }
bool HwGeneric::setKeepAspectRatio(bool val) { return val; }

std::string HwGeneric::getDeviceType() { return "Generic"; }

bool HwGeneric::setScreenState(const bool &enable) { return true; }
