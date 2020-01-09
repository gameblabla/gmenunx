#ifndef _GKD350H_
#define _GKD350H_

#include <string>
#include <vector>

#include "ihardware.h"
#include "rtc.h"
#include "constants.h"

class HwGkd350h : IHardware {

    private:

        bool writeValueToFile(const std::string &path, const char *content);
        void resetKeymap();

        RTC * clock_;

        int volumeLevel_ = 0;
        bool pollVolume = false;
        bool pollBatteries = false;

        const std::string GET_VOLUME_PATH = "/usr/bin/alsa-getvolume";
        const std::string VOLUME_ARGS = "default PCM";
        const std::string BATTERY_CHARGING_PATH = "/sys/class/power_supply/usb/online";
        const std::string BATTERY_LEVEL_PATH = "/sys/class/power_supply/battery/capacity";
        const std::string SCREEN_BLANK_PATH = "/sys/class/graphics/fb0/blank";
        const std::string ALT_KEYMAP_FILE = "/sys/devices/platform/linkdev/alt_key_map";
    
    protected:

    public:

        HwGkd350h();
        ~HwGkd350h();
        IClock * Clock();

        bool getTVOutStatus();

        void setTVOutMode(std::string mode);

        std::string getTVOutMode();

        bool supportsPowerGovernors();

        void setPerformanceMode(std::string alias);

        std::string getPerformanceMode();

        std::vector<std::string> getPerformanceModes();

        bool supportsOverClocking();

        uint32_t getCPUSpeed();

        bool setCPUSpeed(uint32_t mhz);

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

        int defaultScreenWidth() { return 320; }
        int defaultScreenHeight() { return 240; }

        std::string getDeviceType();

        bool setScreenState(const bool &enable);

        void powerOff() {
            sync();
            system("/sbin/poweroff");
        }

        virtual void reboot() {
            sync();
		    system("/sbin/reboot");
        }

        std::string inputFile() { return "gkd350h.input.conf"; };

};

#endif // _GKD350H_