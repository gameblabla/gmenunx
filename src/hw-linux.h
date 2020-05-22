#ifndef _HWLINUX_
#define _HWLINUX_

#include <unordered_map>
#include <string>
#include <vector>

#include "hw-ihardware.h"
#include "hw-clock.h"
#include "constants.h"

class HwLinux : IHardware {

    private:

        IClock * clock_;
        ISoundcard * soundcard_;
        ICpu * cpu_;
        IPower * power_;
        ILed * led_;

    public:
        HwLinux();
        ~HwLinux();

        IClock * Clock() { return this->clock_; }
        ISoundcard * Soundcard() { return this->soundcard_; }
        ICpu * Cpu() { return this->cpu_; }
        IPower * Power() { return this->power_; }
        ILed * Led() { return this->led_; }

        bool getTVOutStatus();
        std::string getTVOutMode();
        void setTVOutMode(std::string mode);

        int getBacklightLevel();
        int setBacklightLevel(int val);

        bool getKeepAspectRatio();
        bool setKeepAspectRatio(bool val);

        std::string getDeviceType();

        void powerOff();
        void reboot();

        int defaultScreenWidth();
        int defaultScreenHeight();

        bool setScreenState(const bool &enable);

        std::string inputFile() { return "linux.input.conf"; };
        std::string packageManager() { return "init"; };
};

#endif