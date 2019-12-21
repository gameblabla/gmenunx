#include "screenmanager.h"
#include "debug.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

ScreenManager *ScreenManager::instance = nullptr;

Uint32 screenTimerCallback(Uint32 timeout, void *d) {
	TRACE("enter");
	unsigned int * old_ticks = (unsigned int *) d;
	unsigned int new_ticks = SDL_GetTicks();

	if (new_ticks > *old_ticks + timeout + 1000) {
		INFO("Suspend occured, restarting timer");
		*old_ticks = new_ticks;
		return timeout;
	}

	TRACE("disable backlight event");
	ScreenManager::instance->disableScreen();
	return 0;
}

ScreenManager::ScreenManager(IHardware *hw_) {
	this->screenState = false;
	this->screenTimeout = 0;
	this->screenTimer = nullptr;
	this->hw = hw_;
	this->asleep = false;
	enableScreen();
	assert(!instance);
	instance = this;
}

ScreenManager::~ScreenManager() {
	TRACE("enter");
	removeScreenTimer();
	instance = nullptr;
	enableScreen();
	TRACE("exit");
}

bool ScreenManager::isAsleep() {
	return this->asleep;
}

void ScreenManager::setScreenTimeout(unsigned int seconds) {
	TRACE("enter : %i", seconds);
	screenTimeout = seconds;
	resetScreenTimer();
	TRACE("exit");
}

void ScreenManager::resetScreenTimer() {
	TRACE("enter");
	removeScreenTimer();
	enableScreen();
	if (screenTimeout != 0) {
		//TRACE("screenTimeout : %i > 0", screenTimeout);
		addScreenTimer();
	}
	TRACE("exit");
}

void ScreenManager::addScreenTimer() {
	TRACE("enter");
	assert(!screenTimer);
	TRACE("no screen timer exists");
	timeout_startms = SDL_GetTicks();
	screenTimer = SDL_AddTimer(screenTimeout * 1000, screenTimerCallback, &timeout_startms);
	if (!screenTimer) {
		ERROR("Could not initialize SDLTimer: %s", SDL_GetError());
	}
	TRACE("exit : %lu", (long)screenTimer);
}

void ScreenManager::removeScreenTimer() {
	TRACE("enter");
	if (screenTimer) {
		TRACE("timer exists");
		SDL_RemoveTimer(screenTimer);
		screenTimer = nullptr;
		TRACE("timer safely removed");
	}
	TRACE("exit");
}

void ScreenManager::setScreenBlanking(bool state) {
	TRACE("enter : %s", (state ? "on" : "off"));
	if (this->hw->setScreenState(state)) {
		this->screenState = state;
	}
}

void ScreenManager::enableScreen() {
	TRACE("enter");
	this->asleep = false;
	if (!this->screenState) {
		setScreenBlanking(true);
	}
}

void ScreenManager::disableScreen() {
	TRACE("enter");
	this->asleep = true;
	if (this->screenState) {
		setScreenBlanking(false);
	}
}
