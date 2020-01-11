#ifndef _HwPG2_
#define _HwPG2_

#include <string>
#include <unordered_map>
#include <vector>

#include "hw-ihardware.h"
#include "rtc.h"
#include "constants.h"

class HwPG2 : IHardware {

    private:

		enum LedAllowedTriggers {
			NONE = 0, 
			TIMER
		};

        void resetKeymap();

        RTC * clock_;
        std::unordered_map<std::string, std::string> performanceModes_;
		std::string ledMaxBrightness_;
        std::string performanceMode_ = "ondemand";
        const std::string defaultPerformanceMode = "ondemand";
        int backlightLevel_ = 0;
        bool keepAspectRatio_ = false;
        bool pollBacklight = false;
        bool pollBatteries = false;
        bool supportsOverclocking_ = false;
        bool supportsPowerGovernors_ = false;

        const std::string SCREEN_BLANK_PATH = "/sys/class/graphics/fb0/blank";
		const std::string LED_PREFIX = "/sys/class/leds/power/";
		const std::string LED_MAX_BRIGHTNESS_PATH = LED_PREFIX + "max_brightness";
		const std::string LED_BRIGHTNESS_PATH = LED_PREFIX + "brightness";
		const std::string LED_DELAY_ON_PATH = LED_PREFIX + "delay_on";
		const std::string LED_DELAY_OFF_PATH = LED_PREFIX + "delay_off";
		const std::string LED_TRIGGER_PATH = LED_PREFIX + "trigger";
        const std::string BACKLIGHT_PATH = "/sys/class/backlight/pwm-backlight/brightness";
        const std::string ASPECT_RATIO_PATH = "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio";
        const std::string BATTERY_CHARGING_PATH = "/sys/class/power_supply/usb/online";
        const std::string BATTERY_LEVEL_PATH = "/sys/class/power_supply/battery/capacity";
        const std::string ALT_KEYMAP_FILE = "/sys/devices/platform/linkdev/alt_key_map";
        
        const std::string SYSFS_CPUFREQ_PATH = "/sys/devices/system/cpu/cpu0/cpufreq";
        const std::string SYSFS_CPUFREQ_MAX = SYSFS_CPUFREQ_PATH + "/scaling_max_freq";
        const std::string SYSFS_CPUFREQ_SET = SYSFS_CPUFREQ_PATH + "/scaling_setspeed";
        const std::string SYSFS_CPUFREQ_GET = SYSFS_CPUFREQ_PATH + "/scaling_cur_freq";
        const std::string SYSFS_CPU_SCALING_GOVERNOR = SYSFS_CPUFREQ_PATH + "/scaling_governor";

        std::string performanceModeMap(std::string fromInternal);

		std::string triggerToString(LedAllowedTriggers t);
    
    protected:

     public:
        HwPG2();
        ~HwPG2();

        IClock * Clock();

        bool getTVOutStatus();
        std::string getTVOutMode();
        void setTVOutMode(std::string mode);

        bool supportsPowerGovernors();
        std::string getPerformanceMode();
        void setPerformanceMode(std::string alias = "");
        std::vector<std::string> getPerformanceModes();

        bool supportsOverClocking();

        bool setCPUSpeed(uint32_t mhz);
        uint32_t getCPUSpeed();
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

        int defaultScreenWidth() { return 320; }
        int defaultScreenHeight() { return 240; }

        std::string systemInfo();

        std::string inputFile() { return "pg2.input.conf"; };
};

#endif
