#ifndef _GKD350H_
#define _GKD350H_

#include <string>
#include <vector>

#include "ihardware.h"
#include "sysclock.h"
#include "constants.h"

class HwGkd350h : IHardware {

    private:

        SysClock * clock_;

    protected:

    public:

        HwGkd350h();
        ~HwGkd350h();
        IClock * Clock();

        bool getTVOutStatus();
        
        void setTVOutMode(std::string mode);
        
        std::string getTVOutMode();
        
        void setPerformanceMode(std::string alias);
        
        std::string getPerformanceMode();
        
        std::vector<std::string> getPerformanceModes();
        
        bool supportsOverClocking();

        bool setCPUSpeed(uint32_t mhz);
        
        std::vector<uint32_t> cpuSpeeds();
        
        uint32_t getCpuDefaultSpeed();
        
        void ledOn(int flashSpeed);
        
        void ledOff();
        
        int getBatteryLevel();
        
        int getVolumeLevel();
        
        int setVolumeLevel(int val);
        
        int getBacklightLevel();
        
        int setBacklightLevel(int val);
        
        bool getKeepAspectRatio();
        
        bool setKeepAspectRatio(bool val);
        
        std::string getDeviceType();
        
        bool setScreenState(const bool &enable);

};

#endif // _GKD350H_