#ifndef _POWERMANAGER_H_
#define _POWERMANAGER_H_

#include "imanager.h"
#include "../debug.h"
#include "../ihardware.h"

class PowerManager : public IManager {

    public:

        static uint32_t staticCallback(uint32_t interval, void * data) {
            TRACE("PowerManager static callback fired");
            PowerManager * me = static_cast<PowerManager *>(data);
            return me->callback(interval);
        }

        PowerManager(IHardware *hw_) : IManager(PowerManager::staticCallback) {
            TRACE("enter");
            this->hw = hw_;
            TRACE("exit");
        }

        uint32_t callback(uint32_t interval) {
            TRACE("PowerManager callback fired");
            unsigned int new_ticks = SDL_GetTicks();
            if (new_ticks > (this->timeout_startms_ + interval + 1000)) {
                INFO("Suspend occured, restarting timer");
                this->timeout_startms_ = new_ticks;
                return interval;
            }
            if (IHardware::BATTERY_CHARGING == this->hw->getBatteryLevel()) {
                TRACE("we're plugged in, not powering down");
                return interval;
            }
            TRACE("power off event");
            this->hw->powerOff();
            TRACE("exit");
            return 0;
        }

        void setTimeout(unsigned int minutes) {
            TRACE("enter : %i minutes", minutes);
            IManager::setTimeout(minutes * 60);
            TRACE("exit");
        }

    private:

        IHardware *hw;

};

#endif

