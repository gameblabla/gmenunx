#ifndef _SCREENMANAGER_H_
#define _SCREENMANAGER_H_

#include "imanager.h"
#include "../debug.h"
#include "../hw-ihardware.h"

class ScreenManager : public IManager {

    public:

        static uint32_t staticCallback(uint32_t interval, void * data) {
            TRACE("ScreenManager static callback fired");
            ScreenManager * me = static_cast<ScreenManager *>(data);
            return me->callback(interval);
        }

        ScreenManager(IHardware *hw_) : IManager(ScreenManager::staticCallback) {
            TRACE("enter");
            this->screenState = false;
            this->hw = hw_;
            this->asleep = false;
            TRACE("exit");
        }

        ~ScreenManager() {
            TRACE("enter");
            this->enableScreen();
            TRACE("exit");
        }

        uint32_t callback(uint32_t interval) {
            TRACE("ScreenManager callback fired");
            unsigned int new_ticks = SDL_GetTicks();
            if (new_ticks > (this->timeout_startms_ + interval + 1000)) {
                INFO("Suspend occured, restarting timer");
                this->timeout_startms_ = new_ticks;
                return interval;
            }
            TRACE("disable backlight event");
            this->disableScreen();
            TRACE("exit");
            return 0;
        }

        void resetTimer() {
            TRACE("calling base::resetTimer");
            IManager::resetTimer();
            TRACE("called base::resetTimer");
            // do my work
            this->enableScreen();
        }

        bool isAsleep() {
            return this->asleep;
        }

    private:

        IHardware *hw;
        bool screenState;
        bool asleep;

        void enableScreen() {
            TRACE("enter");
            this->asleep = false;
            if (!this->screenState) {
                this->setScreenBlanking(true);
            }
        }

        void disableScreen() {
            TRACE("enter");
            this->asleep = true;
            if (this->screenState) {
                this->setScreenBlanking(false);
            }
        }

        void setScreenBlanking(bool state) {
            TRACE("enter : %s", (state ? "on" : "off"));
            if (this->hw->setScreenState(state)) {
                this->screenState = state;
            }
        }

};

#endif

