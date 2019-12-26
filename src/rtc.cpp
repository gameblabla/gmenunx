#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h> 
#include <stdlib.h>

#include "rtc.h"
#include "iclock.h"
#include "debug.h"
#include "utilities.h"

RTC::RTC() {
    TRACE("RTC");
    this->refresh();
}
RTC::~RTC() {
    TRACE("~RTC");
}

void RTC::refresh() {
    TRACE("enter");
    int fd;
    fd = open("/dev/rtc", O_RDONLY);
    if (fd) {
        ioctl(fd, RTC_RD_TIME, &this->rt);
        close(fd);
        TRACE("successful read");
        TRACE("hour raw = %i", this->rt.tm_hour);
    }
    TRACE("exit");
}

int RTC::getYear() {
    return this->rt.tm_year + 1900;
}
int RTC::getMonth() {
    return this->rt.tm_mon + 1;
}
int RTC::getDay() {
    return this->rt.tm_mday;
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
    std::string result = std::string(buf);
    TRACE("exit - %s", result.c_str());
	return result;
}

std::string RTC::getDateTime() {
    TRACE("enter");
    this->refresh();
    std::string result = string_format(
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

bool RTC::setTime(std::string datetime) {
    std::string cmd = "/sbin/hwclock --set --localtime --epoch=1900 --date=\"" + datetime + "\";/sbin/hwclock --hctosys";
    TRACE("running : %s", cmd.c_str());
    system(cmd.c_str());
    return true;
}