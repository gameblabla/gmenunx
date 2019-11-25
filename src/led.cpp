
#include "led.h"
#include "debug.h"
#include "utilities.h"
#include <stdlib.h>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

LED::LED() {
	TRACE("ctor");
#ifdef TARGET_RG350
	max_brightness = fileReader(MAX_BRIGHTNESS_PATH);
#else
	max_brightness = "10";
#endif
	TRACE("max : %s", max_brightness.c_str());
}
LED::~LED() {

}
string LED::triggerToString(AllowedTriggers t) {
	TRACE("mode : %i", t);
	switch(t) {
		case TIMER:
			return "timer";
			break;
		default:
			return "none";
			break;
	};
};

void LED::reset() {
	TRACE("enter");
	string trigger = triggerToString(AllowedTriggers::NONE);
	TRACE("mode : %s", trigger.c_str());
#ifdef TARGET_RG350
	procWriter(TRIGGER_PATH, trigger);
	procWriter(BRIGHTNESS_PATH, max_brightness);
#endif
	TRACE("exit");
	return;
}

void LED::flash(const int &speed) {
	TRACE("enter");
	int limited = constrain(speed, 0, atoi(max_brightness.c_str()));
	string trigger = triggerToString(AllowedTriggers::TIMER);
	TRACE("mode : %s - for %i", trigger.c_str(), limited);
#ifdef TARGET_RG350
	procWriter(TRIGGER_PATH, trigger);
	procWriter(DELAY_ON_PATH, limited);
	procWriter(DELAY_OFF_PATH, limited);
#endif
	TRACE("exit");
	return;
}
