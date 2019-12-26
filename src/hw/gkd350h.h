#ifndef _GKD350H_
#define _GKD350H_

#include <string>
#include <unordered_map>
#include <math.h>
#include <vector>
#include <algorithm>
#include <time.h>
#include <sstream>

#include "ihardware.h"
#include "../rtc.h"
#include "../constants.h"

class HwGkd350h : IHardware {

    private:

    protected:

    public:

        std::string getSystemdateTime() { return ""; };
        
        bool setSystemDateTime(std::string datetime) { return true; }
        
        bool getTVOutStatus() { return false; }
        
        void setTVOutMode(std::string mode) { return; }
        
        std::string getTVOutMode() { return ""; }
        
        void setPerformanceMode(std::string alias) { return; }
        
        std::string getPerformanceMode() { return ""; }
        
        std::vector<std::string> getPerformanceModes() {
            std::vector<std::string> result;
            return result; 
        }
        
        bool supportsOverClocking() { return true; }

        bool setCPUSpeed(uint32_t mhz) { return true; }
        
        std::vector<uint32_t> cpuSpeeds() { return {  }; };
        
        uint32_t getCpuDefaultSpeed() { return 0; }
        
        void ledOn(int flashSpeed) { return; }
        
        void ledOff() { return; }
        
        int getBatteryLevel() { return BATTERY_CHARGING; }
        
        int getVolumeLevel() { return 100; }
        
        int setVolumeLevel(int val) { return val; }
        
        int getBacklightLevel() { return 100; }
        
        int setBacklightLevel(int val) { return val; }
        
        bool getKeepAspectRatio() { return true; }
        
        bool setKeepAspectRatio(bool val) { return val; }
        
        std::string getDeviceType() { return "GKD-350h"; }
        
        bool setScreenState(const bool &enable) { return enable; }

};

#endif // _GKD350H_