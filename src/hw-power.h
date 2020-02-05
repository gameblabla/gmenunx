#ifndef _IPOWER_
#define _IPOWER_

#include <cstdlib>
#include "debug.h"

class IPower {

    public:

        enum PowerStates {
            UNKNOWN, 
            CHARGING, 
            DRAINING
        };

        IPower();
        IPower::PowerStates state() { return this->state_; }
        int rawLevel() { return this->level_; };
        std::string displayLevel() {
            char intval[20];
            std::sprintf(intval, "%02d%%", this->level_);
            std::string result(intval);
            return (this->state_ == IPower::PowerStates::CHARGING) ? result + "+" : result;
        };
        virtual bool read() = 0;

    protected:

        int level_;
        PowerStates state_;   
};

class JzPower : IPower {

    private:
        bool pollBatteries_ = false;
        const std::string BATTERY_CHARGING_PATH = "/sys/class/power_supply/usb/online";
        const std::string BATTERY_LEVEL_PATH = "/sys/class/power_supply/battery/capacity";
        bool readLevel();
        bool readState();

    public:
        JzPower ();
        bool read();
};

class GenericPower : IPower {
    
    private:
        int counter_;

    public:
        GenericPower();
        bool read();
};

#endif
