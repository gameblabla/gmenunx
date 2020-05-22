#ifndef _HWRG350_
#define _HWRG350_

#include <string>
#include <vector>

#include "hw-ihardware.h"
#include "constants.h"

class HwRg350 : IHardware {

    private:

        void resetKeymap();

        IClock * clock_;
        ISoundcard * soundcard_;
        ICpu * cpu_;
        IPower * power_;
        ILed * led_;
        IHdmi * hdmi_;

		std::string ledMaxBrightness_;
        int backlightLevel_ = 0;
        bool keepAspectRatio_ = false;
        bool pollBacklight = false;

        const std::string SCREEN_BLANK_PATH = "/sys/class/graphics/fb0/blank";

        const std::string BACKLIGHT_PATH = "/sys/class/backlight/pwm-backlight/brightness";
        const std::string ASPECT_RATIO_PATH = "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio";
        const std::string ALT_KEYMAP_FILE = "/sys/devices/platform/linkdev/alt_key_map";

    protected:

     public:
        HwRg350();
        ~HwRg350();

        IClock * Clock() { return this->clock_; }
        ISoundcard * Soundcard() { return this->soundcard_; }
        ICpu * Cpu() { return this->cpu_; }
        IPower * Power() { return this->power_; }
        ILed * Led() { return this->led_; }
        IHdmi * Hdmi() { return this->hdmi_; }

        int getBacklightLevel();
        int setBacklightLevel(int val);

        bool getKeepAspectRatio();
        bool setKeepAspectRatio(bool val);
        std::string getDeviceType();

        bool setScreenState(const bool &enable);

        int defaultScreenWidth() { return 320; }
        int defaultScreenHeight() { return 240; }

        std::string systemInfo();
        std::string inputFile() { return "rg350.input.conf"; };
        std::string packageManager() { return "opkrun"; };
};

#endif
