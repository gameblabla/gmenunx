#ifndef _IMANAGER_H_
#define _IMANAGER_H_

#include <SDL.h>
#include <cassert>

#include "../debug.h"

class IManager {

    public:

        IManager(SDL_NewTimerCallback callback_) {
            TRACE("enter");
            this->timer_ = nullptr;
            this->timeout_ = 0;
            this->timeout_startms_ = 0;
            this->staticCallback = callback_;
            TRACE("exit");
        }

        ~IManager() {
            TRACE("enter");
            this->removeTimer();
            TRACE("exit");
        }

        virtual uint32_t callback(uint32_t interval) { return 0; };

        virtual void resetTimer() {
            TRACE("enter");
            this->removeTimer();
            if (this->timeout_ != 0) {
                this->addTimer();
            }
            TRACE("exit");
        }

        virtual void setTimeout(unsigned int seconds) {
            TRACE("enter : %i seconds", seconds);
            this->timeout_ = seconds;
            this->resetTimer();
            TRACE("exit");
        }

    protected:

        SDL_NewTimerCallback staticCallback;
        SDL_TimerID timer_;
        uint32_t timeout_startms_;
        uint32_t timeout_;

        void addTimer() {
            TRACE("enter");
            assert(!this->timer_);
            this->timeout_startms_ = SDL_GetTicks();
            this->timer_ = SDL_AddTimer(this->timeout_ * 1000, this->staticCallback, this);
            if (!this->timer_) {
                ERROR("Could not initialize SDLTimer: %s", SDL_GetError());
            }
            TRACE("exit - timer id : %lu", (long)this->timer_);
        }
        void removeTimer() {
            TRACE("enter");
            if (this->timer_) {
                TRACE("timer id exists : %lu", (long)this->timer_);
                SDL_bool success = SDL_RemoveTimer(this->timer_);
                if (SDL_TRUE == success) {
                    TRACE("timer safely removed");
                } else {
                    TRACE("couldn't remove timer id : %lu", (long)this->timer_);
                }
                // null it anyway, 1.2 seems buggy
                this->timer_ = nullptr;
            }
            TRACE("exit");
        }

};

#endif
