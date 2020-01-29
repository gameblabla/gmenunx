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

        IClock * clock_;
        ISoundcard * soundcard_;
        ICpu * cpu_;
        IPower * power_;

    public:
        HwGeneric();
        ~HwGeneric();

        IClock * Clock() { return this->clock_; }
        ISoundcard * Soundcard() { return this->soundcard_; }
        ICpu * Cpu() { return this->cpu_; }
        IPower * Power() { return this->power_; }

        bool getTVOutStatus();
        std::string getTVOutMode();
        void setTVOutMode(std::string mode);

        void ledOn(int flashSpeed = 250);
        void ledOff();

        int getBacklightLevel();
        int setBacklightLevel(int val);

        bool getKeepAspectRatio();
        bool setKeepAspectRatio(bool val);

        std::string getDeviceType();

        bool setScreenState(const bool &enable);
};

#endif