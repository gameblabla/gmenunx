#ifndef _HWRG350_
#define _HWRG350_

#include <string>
#include <unordered_map>
#include <vector>

#include "ihardware.h"
#include "rtc.h"
#include "constants.h"

class HwRg350 : IHardware {

    private:

		enum LedAllowedTriggers {
			NONE = 0, 
			TIMER
		};

        RTC * clock_;
        std::unordered_map<std::string, std::string> performanceModes_;
		std::string ledMaxBrightness_;
        std::string performanceMode_ = "ondemand";
        const std::string defaultPerformanceMode = "ondemand";
        int volumeLevel_ = 0;
        int backlightLevel_ = 0;
        bool keepAspectRatio_ = false;
        bool pollBacklight = false;
        bool pollBatteries = false;
        bool pollVolume = false;

        const std::string SCREEN_BLANK_PATH = "/sys/class/graphics/fb0/blank";
		const std::string LED_PREFIX = "/sys/class/leds/power/";
		const std::string LED_MAX_BRIGHTNESS_PATH = LED_PREFIX + "max_brightness";
		const std::string LED_BRIGHTNESS_PATH = LED_PREFIX + "brightness";
		const std::string LED_DELAY_ON_PATH = LED_PREFIX + "delay_on";
		const std::string LED_DELAY_OFF_PATH = LED_PREFIX + "delay_off";
		const std::string LED_TRIGGER_PATH = LED_PREFIX + "trigger";
        const std::string GET_VOLUME_PATH = "/usr/bin/alsa-getvolume";
        const std::string SET_VOLUME_PATH = "/usr/bin/alsa-setvolume";
        const std::string VOLUME_ARGS = "default PCM";
        const std::string BACKLIGHT_PATH = "/sys/class/backlight/pwm-backlight/brightness";
        const std::string ASPECT_RATIO_PATH = "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio";
        const std::string BATTERY_CHARGING_PATH = "/sys/class/power_supply/usb/online";
        const std::string BATTERY_LEVEL_PATH = "/sys/class/power_supply/battery/capacity";
        
        const std::string SYSFS_CPUFREQ_PATH = "/sys/devices/system/cpu/cpu0/cpufreq";
        const std::string SYSFS_CPUFREQ_MAX = SYSFS_CPUFREQ_PATH + "/scaling_max_freq";
        const std::string SYSFS_CPUFREQ_SET = SYSFS_CPUFREQ_PATH + "/scaling_setspeed";
        const std::string SYSFS_CPUFREQ_GET = SYSFS_CPUFREQ_PATH + "/scaling_cur_freq";

        std::string performanceModeMap(std::string fromInternal);

		std::string triggerToString(LedAllowedTriggers t);
    
    protected:

     public:
        HwRg350();
        ~HwRg350();

        IClock * Clock();

        bool getTVOutStatus();
        std::string getTVOutMode();
        void setTVOutMode(std::string mode);

        std::string getPerformanceMode();
        void setPerformanceMode(std::string alias = "");
        std::vector<std::string> getPerformanceModes();

        bool supportsOverClocking();

        bool setCPUSpeed(uint32_t mhz);
        uint32_t getCPUSpeed();
        std::vector<uint32_t> cpuSpeeds();
        uint32_t getCpuDefaultSpeed();

        void ledOn(int flashSpeed = 250);
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

        std::string systemInfo();
};

#endif
