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

std::string RTC::getTime() {
    TRACE("RTC::getTime - enter");
    int fd;
    struct rtc_time rt;
    fd = open("/dev/rtc", O_RDONLY);
    ioctl(fd, RTC_RD_TIME, &rt);
    close(fd);

/*
    struct       rtc_time {
        int         tm_sec;
        int         tm_min;
        int         tm_hour;
        int         tm_mday;
        int         tm_mon;
        int         tm_year;
        int         tm_wday;
        int         tm_yday;
        int         tm_isdst;
    };
*/

    string result = string_format(
        "%i-%i-%i %i:%i", 
        1900 + rt.tm_year, 
        1 + rt.tm_mon, 
        rt.tm_mday, 
        rt.tm_hour + (rt.tm_isdst ? 1 : 0), 
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