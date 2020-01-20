#ifndef _HWGENERIC_
#define _HWGENERIC_

#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>

#include "hw-ihardware.h"
#include "sysclock.h"
#include "constants.h"

class HwGeneric : IHardware {
    private:

        SysClock * clock_;
        DummySoundcard * soundcard_;
        DefaultCpu * cpu_;

    public:
        HwGeneric();
        ~HwGeneric();

        IClock * Clock() { return (IClock *)this->clock_; }
        ISoundcard * Soundcard() { return (ISoundcard *)this->soundcard_; }
        ICpu * Cpu() { return (ICpu *)this->cpu_; }

        bool getTVOutStatus();
        std::string getTVOutMode();
        void setTVOutMode(std::string mode);

        void ledOn(int flashSpeed = 250);
        void ledOff();

        int getBatteryLevel();

        int getBacklightLevel();
        int setBacklightLevel(int val);

        bool getKeepAspectRatio();
        bool setKeepAspectRatio(bool val);

        std::string getDeviceType();

        bool setScreenState(const bool &enable);
};

#endif