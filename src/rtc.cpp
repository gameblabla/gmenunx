#include <string>
#include <time.h>
#include <cstdio>
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
    TRACE("RTC");
    this->refresh();
}
RTC::~RTC() {
    TRACE("~RTC");
}

void RTC::refresh() {
    TRACE("enter");
    #ifdef TARGET_RG350
    int fd;
    fd = open("/dev/rtc", O_RDONLY);
    if (fd) {
        ioctl(fd, RTC_RD_TIME, &this->rt);
        close(fd);
        TRACE("successful read");
        TRACE("hour raw = %i", this->rt.tm_hour);
    }
    #else
        time_t theTime = time(NULL);
        struct tm *aTime = localtime(&theTime);
        this->rt.tm_year = aTime->tm_year;
        this->rt.tm_hour = aTime->tm_hour;
        this->rt.tm_min = aTime->tm_min;
        this->rt.tm_isdst = aTime->tm_isdst;
    #endif
    TRACE("exit");
}

int RTC::getHours() {
    return this->rt.tm_hour + (this->rt.tm_isdst ? 1 : 0);
}
int RTC::getMinutes() {
    return this->rt.tm_min;
}

std::string RTC::getClockTime(bool is24hr) {
    TRACE("enter - is24hr : %i", is24hr);
    int hours = this->getHours();
    TRACE("hours are : %i", hours);

	bool pm = (hours >= 12);
	if (!is24hr && pm)
		hours -= 12;

	char buf[9];
	sprintf(buf, "%02i:%02i%s", hours, this->rt.tm_min, is24hr ? "" : (pm ? " pm" : " am"));
    string result = string(buf);
    TRACE("exit - %s", result.c_str());
	return result;
}

std::string RTC::getDateTime() {
    TRACE("enter");
    string result = string_format(
        "%i-%i-%i %i:%i", 
        1900 + rt.tm_year, 
        1 + rt.tm_mon, 
        rt.tm_mday, 
        this->getHours(), 
        rt.tm_min);
    TRACE("dst : %i", rt.tm_isdst);
    TRACE("exit : %s", result.c_str());
    return result;
}

bool RTC::setTime(string datetime) {
    string cmd = "/sbin/hwclock --set --localtime --epoch=1900 --date=\"" + datetime + "\";/sbin/hwclock --hctosys";
    TRACE("running : %s", cmd.c_str());
    #ifdef TARGET_RG350
    system(cmd.c_str());
    #endif
    return true;
}