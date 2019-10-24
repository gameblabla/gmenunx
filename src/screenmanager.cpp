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
	DEBUG("ScreenManager::screenTimerCallback - enter");
	unsigned int * old_ticks = (unsigned int *) d;
	unsigned int new_ticks = SDL_GetTicks();

	if (new_ticks > *old_ticks + timeout + 1000) {
		DEBUG("Suspend occured, restarting timer\n");
		*old_ticks = new_ticks;
		return timeout;
	}

	DEBUG("ScreenManager::screenTimerCallback - Disable Backlight Event\n");
	ScreenManager::instance->disableScreen();
	return 0;
}

ScreenManager::ScreenManager()
	: screenState(false)
	, screenTimeout(0)
	, screenTimer(nullptr)
{
	this->asleep = false;
	enableScreen();
	assert(!instance);
	instance = this;
}

ScreenManager::~ScreenManager() {
	removeScreenTimer();
	instance = nullptr;
	enableScreen();
}

bool ScreenManager::isAsleep() {
	return this->asleep;
}

void ScreenManager::setScreenTimeout(unsigned int seconds) {
	INFO("ScreenManager::setScreenTimeout : %i", seconds);
	screenTimeout = seconds;
	resetScreenTimer();
}

void ScreenManager::resetScreenTimer() {
	DEBUG("ScreenManager::resetScreenTimer - enter");
	removeScreenTimer();
	enableScreen();
	if (screenTimeout != 0) {
		addScreenTimer();
	}
}

void ScreenManager::addScreenTimer() {
	DEBUG("ScreenManager::addScreenTimer - enter");
	assert(!screenTimer);
	DEBUG("ScreenManager::addScreenTimer - no screen timer exists");
	timeout_startms = SDL_GetTicks();
	screenTimer = SDL_AddTimer(
			screenTimeout * 1000, screenTimerCallback, &timeout_startms);
	if (!screenTimer) {
		ERROR("Could not initialize SDLTimer: %s\n", SDL_GetError());
	}
}

void ScreenManager::removeScreenTimer() {
	DEBUG("ScreenManager::removeScreenTimer - enter");
	if (screenTimer) {
		DEBUG("ScreenManager::removeScreenTimer - timer exists");
		SDL_RemoveTimer(screenTimer);
		screenTimer = nullptr;
		DEBUG("ScreenManager::removeScreenTimer - timer safely removed");
	}
}

#define SCREEN_BLANK_PATH "/sys/class/graphics/fb0/blank"
void ScreenManager::setScreenBlanking(bool state) {
	DEBUG("ScreenManager::setScreenBlanking - %s", (state ? "on" : "off"));
	const char *path = SCREEN_BLANK_PATH;
	const char *blank = state ? "0" : "1";

	int fd = open(path, O_RDWR);
	if (fd == -1) {
		WARNING("Failed to open '%s': %s\n", path, strerror(errno));
	} else {
		ssize_t written = write(fd, blank, strlen(blank));
		if (written == -1) {
			WARNING("Error writing '%s': %s\n", path, strerror(errno));
		}
		close(fd);
	}
	screenState = state;
}

void ScreenManager::enableScreen() {
	DEBUG("ScreenManager::enableScreen - enter");
	asleep = false;
	if (!screenState) {
		setScreenBlanking(true);
	}
}

void ScreenManager::disableScreen() {
	DEBUG("ScreenManager::disableScreen - enter");
	asleep = true;
	if (screenState) {
		setScreenBlanking(false);
	}
}
