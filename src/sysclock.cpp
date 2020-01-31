#include "sysclock.h"
#include "hw-clock.h"
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


bool SysClock::setTime(std::string datetime) {
    return true;
}
