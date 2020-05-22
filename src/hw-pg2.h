#ifndef _HwPG2_
#define _HwPG2_

#include <string>
#include <unordered_map>
#include <vector>

#include "hw-ihardware.h"
#include "hw-clock.h"
#include "constants.h"

class HwPG2 : IHardware {

    private:

        void resetKeymap();

        IClock * clock_;
        ISoundcard * soundcard_;
        ICpu * cpu_;
        IPower * power_;
        ILed * led_;

		bool keepAspectRatio_ = false;

        bool pollBacklight_ = false;
        bool reverse_ = false;
        bool rogue_ = false;
        int backlightLevel_ = 0;
        int max_ = 255;

        const std::string BACKLIGHT_ROOT_PATH = "/sys/class/backlight/pwm-backlight/";
        const std::string BACKLIGHT_PATH = BACKLIGHT_ROOT_PATH + "brightness";
        const std::string BACKLIGHT_MAX_PATH = BACKLIGHT_ROOT_PATH + "max_brightness";

        const std::string SCREEN_BLANK_PATH = "/sys/class/graphics/fb0/blank";

        const std::string ASPECT_RATIO_PATH = "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio";
        const std::string ALT_KEYMAP_FILE = "/sys/devices/platform/linkdev/alt_key_map";

    protected:

     public:
        HwPG2();
        ~HwPG2();

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

        bool setScreenState(const bool &enable);

        int defaultScreenWidth() { return 320; }
        int defaultScreenHeight() { return 240; }

        std::string systemInfo();

        std::string inputFile() { return "pg2.input.conf"; };
        std::string packageManager() { return "opkrun"; };
};

#endif
