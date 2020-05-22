#ifndef _ILED_
#define _ILED_

#include <string>

#include "constants.h"
#include "fileutils.h"

class ILed {

    public:

        enum LedStates {
            UNKNOWN = 0, 
            OFF, 
            FLASHING, 
            ON
        };
		enum LedAllowedTriggers {
			NONE = 0, 
			TIMER
		};

        ILed() { 
            this->state_ = ILed::LedStates::UNKNOWN; 
        };

        LedStates getState() {
            TRACE("led state :: %i", this->state_);
            return this->state_; 
        }
        virtual void read() = 0;
        virtual bool turnOn(int flashSpeed = 250) = 0;
        virtual bool turnOff() = 0;

		std::string state() {
			switch (this->state_) {
				case LedStates::ON:
					return "On";
					break;
				case LedStates::OFF:
					return "Off";
					break;
				case LedStates::FLASHING:
					return "Flashing";
					break;
				default:
					return "Unknown";
					break;
			};
        }

    protected:

        LedStates state_;

        std::string triggerToString(ILed::LedAllowedTriggers t) {
            TRACE("mode : %i", t);
            switch (t) {
                case TIMER:
                    return "timer";
                    break;
                default:
                    return "none";
                    break;
            };
        }

};

class Rg350Led : public ILed {

    private:

		const std::string LED_PREFIX = "/sys/class/leds/power/";
		const std::string LED_MAX_BRIGHTNESS_PATH = LED_PREFIX + "max_brightness";
		const std::string LED_BRIGHTNESS_PATH = LED_PREFIX + "brightness";
		const std::string LED_DELAY_ON_PATH = LED_PREFIX + "delay_on";
		const std::string LED_DELAY_OFF_PATH = LED_PREFIX + "delay_off";
		const std::string LED_TRIGGER_PATH = LED_PREFIX + "trigger";
        std::string ledMaxBrightness_;
    
    public:

        Rg350Led() : ILed() {
            this->ledMaxBrightness_ = FileUtils::fileExists(LED_MAX_BRIGHTNESS_PATH) ? FileUtils::fileReader(LED_MAX_BRIGHTNESS_PATH) : "0";
            this->read();
        }

        void read() {
            TRACE("enter");
            std::string trigger = this->triggerToString(LedAllowedTriggers::TIMER);
            std::string currentMode = FileUtils::fileReader(LED_TRIGGER_PATH);
            if (trigger.compare(currentMode)) {
                int speed = atoi(FileUtils::fileReader(LED_DELAY_ON_PATH).c_str());
                if (0 == speed) {
                    this->state_ = ILed::LedStates::ON;
                } else {
                    this->state_ = ILed::LedStates::FLASHING;
                }
            } else {
                this->state_ = ILed::LedStates::OFF;
            }
            TRACE("exit :: %i", this->state_);
        }

        bool turnOn(int flashSpeed) {
            TRACE("enter");
            bool result = false;
            try {
                int limited = constrain(flashSpeed, 0, atoi(ledMaxBrightness_.c_str()));
                std::string trigger = this->triggerToString(LedAllowedTriggers::TIMER);
                TRACE("mode : %s - for %i", trigger.c_str(), limited);
                FileUtils::fileWriter(LED_TRIGGER_PATH, trigger);
                FileUtils::fileWriter(LED_DELAY_ON_PATH, limited);
                FileUtils::fileWriter(LED_DELAY_OFF_PATH, limited);
                this->state_ = ILed::LedStates::ON;
                result = true;
            } catch (std::exception e) {
                ERROR("LED error : '%s'", e.what());
            } catch (...) {
                ERROR("Unknown error");
            }
            TRACE("exit :: %i", this->state_);
            return result;
        };

        bool turnOff() {
            TRACE("enter");
            bool result = false;
            try {
                std::string trigger = this->triggerToString(LedAllowedTriggers::NONE);
                TRACE("mode : %s", trigger.c_str());
                FileUtils::fileWriter(LED_TRIGGER_PATH, trigger);
                FileUtils::fileWriter(LED_BRIGHTNESS_PATH, ledMaxBrightness_);
                this->state_ = ILed::LedStates::OFF;
                result = true;
            } catch (std::exception e) {
                ERROR("LED error : '%s'", e.what());
            } catch (...) {
                ERROR("Unknown error");
            }
            TRACE("exit :: %i", this->state_);
            return result;
        };

    protected:

};

class DummyLed : public ILed {

    public:

        DummyLed() : ILed() { 
            this->read();
        }
        void read() {
            TRACE("enter");
            this->state_  = LedStates::OFF;
            TRACE("exit :: %i", this->state_);
        };
        bool turnOn(int flashSpeed) {
            TRACE("enter");
            this->state_ = LedStates::ON;
            TRACE("exit :: %i", this->state_);
            return true; 
        }
        bool turnOff() {
            TRACE("enter");
            this->state_ = LedStates::OFF;
            TRACE("exit :: %i", this->state_);
            return true; 
        }
};

#endif
