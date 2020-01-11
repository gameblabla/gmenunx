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

        std::string performanceModeMap(std::string fromInternal);
    
        std::unordered_map<std::string, std::string> performanceModes_;
        std::string performanceMode_ = "ondemand";
        const std::string defaultPerformanceMode = "ondemand";
        SysClock * clock_;

    public:
        HwGeneric();
        ~HwGeneric();

        IClock * Clock();

        bool getTVOutStatus();
        std::string getTVOutMode();
        void setTVOutMode(std::string mode);

        bool supportsPowerGovernors();
        std::string getPerformanceMode();
        void setPerformanceMode(std::string alias = "");
        std::vector<std::string> getPerformanceModes();
        
        bool supportsOverClocking();
        uint32_t getCPUSpeed();
        bool setCPUSpeed(uint32_t mhz);
        uint32_t getCpuDefaultSpeed();

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