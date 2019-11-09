#include <string>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> 
#include <stdlib.h>

#include "rtc.h"
#include "debug.h"
#include "utilities.h"

using std::string;

RTC::RTC() {
    TRACE("RTC::RTC");
    this->refresh();
}
RTC::~RTC() {
    TRACE("RTC::~RTC");
}

void RTC::refresh() {
    TRACE("RTC::refresh - enter");

    int fd;
    fd = open("/dev/rtc", O_RDONLY);
    if (fd) {
        ioctl(fd, RTC_RD_TIME, &this->rt);
        close(fd);
        TRACE("RTC::refresh - successful read");
        TRACE("RTC::refresh - hour raw = %i", this->rt.tm_hour);
    }
    TRACE("RTC::refresh - exit");
}

int RTC::getHours() {
    return this->rt.tm_hour + (this->rt.tm_isdst ? 1 : 0);
}
int RTC::getMinutes() {
    return this->rt.tm_min;
}

std::string RTC::getClockTime(bool is24hr) {
    TRACE("RTC::getClockTime - enter - is24hr : %i", is24hr);
    int hours = this->getHours();
    TRACE("RTC::getClockTime - hours are : %i", hours);

	bool pm = (hours >= 12);
	if (!is24hr && pm)
		hours -= 12;

	char buf[9];
	sprintf(buf, "%02i:%02i%s", hours, this->rt.tm_min, is24hr ? "" : (pm ? " pm" : " am"));
    string result = string(buf);
    TRACE("RTC::getClockTime - exit - %s", result.c_str());
	return result;
}

std::string RTC::getDateTime() {
    TRACE("RTC::getTime - enter");
    string result = string_format(
        "%i-%i-%i %i:%i", 
        1900 + rt.tm_year, 
        1 + rt.tm_mon, 
        rt.tm_mday, 
        this->getHours(), 
        rt.tm_min);
    TRACE("RTC::getTime - dst : %i", rt.tm_isdst);
    TRACE("RTC::getTime - exit : %s", result.c_str());
    return result;
}

bool RTC::setTime(string datetime) {
    string cmd = "/sbin/hwclock --set --localtime --epoch=1900 --date=\"" + datetime + "\";/sbin/hwclock --hctosys";
    TRACE("RTC::setTime - running : %s", cmd.c_str());
    system(cmd.c_str());
    return true;
}