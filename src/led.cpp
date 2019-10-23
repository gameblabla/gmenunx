
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
	DEBUG("LED::ctor");
	max_brightness = procReader(MAX_BRIGHTNESS_PATH);
	DEBUG("LED::ctor max : %s", max_brightness.c_str());
}
LED::~LED() {

}
string LED::triggerToString(AllowedTriggers t) {
	DEBUG("LED::triggerToString - mode : %i", t);
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
	DEBUG("LED::reset - enter");
	string trigger = triggerToString(AllowedTriggers::NONE);
	DEBUG("LED::reset - mode : %s", trigger.c_str());
	procWriter(TRIGGER_PATH, trigger);
	procWriter(BRIGHTNESS_PATH, max_brightness);
	DEBUG("LED::reset - enter");
	return;
}

void LED::flash(const int &speed) {
	DEBUG("LED::flash - enter");
	int limited = constrain(speed, 0, atoi(max_brightness.c_str()));
	stringstream ss;  
	ss<<limited;  
	string timer;  
	ss>>timer;  
	string trigger = triggerToString(AllowedTriggers::TIMER);
	DEBUG("LED::flash - mode : %s - for %s", trigger.c_str(), timer.c_str());
	
	procWriter(TRIGGER_PATH, trigger);
	procWriter(DELAY_ON_PATH, timer);
	procWriter(DELAY_OFF_PATH, timer);

	DEBUG("LED::flash - exit");
	return;
}
