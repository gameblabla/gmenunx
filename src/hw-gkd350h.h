#ifndef _GKD350H_
#define _GKD350H_

#include <string>
#include <vector>

#include "hw-ihardware.h"
#include "hw-clock.h"
#include "constants.h"

class HwGkd350h : IHardware {

    private:

        void resetKeymap();

        IClock * clock_;
        ISoundcard * soundcard_;
        ICpu * cpu_;
        IPower * power_;

        int backlightLevel_ = 0;
        bool pollBacklight = false;

        const std::string SCREEN_BLANK_PATH = "/sys/class/graphics/fb0/blank";
        const std::string ALT_KEYMAP_FILE = "/sys/devices/platform/linkdev/alt_key_map";
        const std::string BACKLIGHT_PATH = "/sys/devices/platform/jz-pwm-dev.0/jz-pwm/pwm0/dutyratio";

    protected:

    public:

        HwGkd350h();
        ~HwGkd350h();

        IClock * Clock() { return this->clock_; }
        ISoundcard * Soundcard() { return this->soundcard_; }
        ICpu * Cpu() { return this->cpu_; }
        IPower * Power() { return this->power_; }

        bool getTVOutStatus();
        void setTVOutMode(std::string mode);
        std::string getTVOutMode();

        void ledOn(int flashSpeed);
        void ledOff();

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
            std::system("/sbin/poweroff");
        }

        virtual void reboot() {
            sync();
		    std::system("/sbin/reboot");
        }

        std::string inputFile() { return "gkd350h.input.conf"; };
        std::string packageManager() { return "opkrun"; };

};

#endif // _GKD350H_