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

        IClock * clock_;
        ISoundcard * soundcard_;
        ICpu * cpu_;

		std::string ledMaxBrightness_;
        int backlightLevel_ = 0;
        bool keepAspectRatio_ = false;
        bool pollBacklight = false;
        bool pollBatteries = false;

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

		std::string triggerToString(LedAllowedTriggers t);
    
    protected:

     public:
        HwPG2();
        ~HwPG2();

        IClock * Clock() { return this->clock_; }
        ISoundcard * Soundcard() { return this->soundcard_; }
        ICpu * Cpu() { return this->cpu_; }

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

        int defaultScreenWidth() { return 320; }
        int defaultScreenHeight() { return 240; }

        std::string systemInfo();

        std::string inputFile() { return "pg2.input.conf"; };
};

#endif
