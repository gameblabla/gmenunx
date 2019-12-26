#include "sysclock.h"
#include "iclock.h"
#include "debug.h"
#include "utilities.h"

SysClock::SysClock() {
    TRACE("SysClock");
    this->refresh();
}
SysClock::~SysClock() {
    TRACE("~SysClock");
}

void SysClock::refresh() {
    TRACE("enter");

    time_t theTime = time(NULL);
    this->myTime = (*localtime(&theTime));
    TRACE("exit");
}

int SysClock::getYear() {
    return this->myTime.tm_year + 1900;
}
int SysClock::getMonth() {
    return this->myTime.tm_mon + 1;
}
int SysClock::getDay() {
    return this->myTime.tm_mday;
}

int SysClock::getHours() {
    return this->myTime.tm_hour + (this->myTime.tm_isdst ? 1 : 0);
}
int SysClock::getMinutes() {
    return this->myTime.tm_min;
}

std::string SysClock::getClockTime(bool is24hr) {
    TRACE("enter - is24hr : %i", is24hr);
    int hours = this->getHours();
    TRACE("hours are : %i", hours);

	bool pm = (hours >= 12);
	if (!is24hr && pm)
		hours -= 12;

	char buf[9];
	sprintf(buf, "%02i:%02i%s", hours, this->myTime.tm_min, is24hr ? "" : (pm ? " pm" : " am"));
    std::string result = std::string(buf);
    TRACE("exit - %s", result.c_str());
	return result;
}

std::string SysClock::getDateTime() {
    TRACE("enter");
    this->refresh();
    std::string result = string_format(
        "%i-%i-%i %i:%i", 
        1900 + myTime.tm_year, 
        1 + myTime.tm_mon, 
        myTime.tm_mday, 
        this->getHours(), 
        myTime.tm_min);
    TRACE("dst : %i", myTime.tm_isdst);
    TRACE("exit : %s", result.c_str());
    return result;
}

bool SysClock::setTime(std::string datetime) {
    return true;
}