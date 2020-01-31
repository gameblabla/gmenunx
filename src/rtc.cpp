#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h> 
#include <stdlib.h>

#include "rtc.h"
#include "hw-clock.h"
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

bool RTC::setTime(std::string datetime) {
    std::string cmd = "/sbin/hwclock --set --localtime --epoch=1900 --date=\"" + datetime + "\";/sbin/hwclock --hctosys";
    TRACE("running : %s", cmd.c_str());
    system(cmd.c_str());
    return true;
}