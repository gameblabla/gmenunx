#include "powermanager.h"
#include "messagebox.h"

PowerManager *PowerManager::instance = NULL;

PowerManager::PowerManager(Esoteric *app, uint32_t suspendTimeout, uint32_t powerTimeout) {
	instance = this;
	this->suspendTimeout = suspendTimeout;
	this->powerTimeout = powerTimeout;
	this->app = app;
	this->suspendActive = false;

	this->powerTimer = NULL;

	resetSuspendTimer();
}

PowerManager::~PowerManager() {
	clearTimer();
	instance = NULL;
}

void PowerManager::setSuspendTimeout(uint32_t suspendTimeout) {
	this->suspendTimeout = suspendTimeout;
	resetSuspendTimer();
};

void PowerManager::setPowerTimeout(uint32_t powerTimeout) {
	this->powerTimeout = powerTimeout;
	resetSuspendTimer();
};

void PowerManager::clearTimer() {
	if (powerTimer != NULL) SDL_RemoveTimer(powerTimer);
	powerTimer = NULL;
};

void PowerManager::resetSuspendTimer() {
	/*clearTimer();
	powerTimer = SDL_AddTimer(this->suspendTimeout * 1e3, doSuspend, NULL);*/
};

void PowerManager::resetPowerTimer() {
	/*clearTimer();
	powerTimer = SDL_AddTimer(this->powerTimeout * 60e3, doPowerOff, NULL);*/
};

uint32_t PowerManager::doSuspend(uint32_t interval, void *param) {
	/*if (interval > 0) {
		MessageBox mb(PowerManager::instance->app, PowerManager::instance->app->tr["Suspend"]);
		mb.setAutoHide(500);
		mb.exec();

		PowerManager::instance->app->setBacklight(0);
		PowerManager::instance->app->setTVOut("OFF");
		PowerManager::instance->app->setCPU(PowerManager::instance->app->confInt["cpuMin"]);
		PowerManager::instance->resetPowerTimer();

		PowerManager::instance->suspendActive = true;

		return interval;
	}

	PowerManager::instance->app->setCPU(PowerManager::instance->app->confInt["cpuMenu"]);
	PowerManager::instance->app->setTVOut(PowerManager::instance->app->TVOut);
	PowerManager::instance->app->setBacklight(max(10, PowerManager::instance->app->confInt["backlight"]));
	PowerManager::instance->suspendActive = false;
	PowerManager::instance->resetSuspendTimer();*/
	return interval;
};

uint32_t PowerManager::doPowerOff(uint32_t interval, void *param) {
#if !defined(TARGET_PC)
	system("sync; poweroff");
#endif
	return interval;
};
