// RTC includes
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h> 
#include <cstdlib>

#include "debug.h"
#include "stringutils.h"

#include "hw-clock.h"

std::string IClock::getBuildDate() {
    char buildBits[] = {
        BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3,
        '-',
        BUILD_MONTH_CH0, BUILD_MONTH_CH1,
        '-',
        BUILD_DAY_CH0, BUILD_DAY_CH1};
    std::stringstream ss;
    ss << buildBits << " 0:0";
    std::string result;
    ss >> result;
    return result;
}

std::string IClock::getClockTime(bool is24hr) {
    TRACE("enter - is24hr : %i", is24hr);
    int hours = this->getHours();
	bool pm = (hours >= 12);
	if (!is24hr && pm)
		hours -= 12;

	char buf[9];
	sprintf(buf, "%02i:%02i%s", hours, this->getMinutes(), is24hr ? "" : (pm ? " pm" : " am"));
    std::string result = std::string(buf);
    TRACE("exit - %s", result.c_str());
	return result;
}

std::string IClock::getDateTime() {
    TRACE("enter");
    this->refresh();
    std::string result = StringUtils::stringFormat(
        "%i-%i-%i %i:%i", 
        this->getYear(), 
        this->getMonth(), 
        this->getDay(), 
        this->getHours(), 
        this->getMinutes()
    );
    TRACE("exit : %s", result.c_str());
    return result;
}

// RTC section starts

void RTC::refresh() {

    TRACE("enter");
    time_t theTime = time(NULL);
    this->myTime = (*localtime(&theTime));
    TRACE("exit");
    return;

    TRACE("enter");
    int fd;
    fd = open("/dev/rtc", O_RDONLY);
    if (fd) {
        struct rtc_time rt;
        ioctl(fd, RTC_RD_TIME, &rt);
        close(fd);
        TRACE("successful read");
        this->myTime.tm_year = rt.tm_year;
        this->myTime.tm_mon = rt.tm_mon;
        this->myTime.tm_mday = rt.tm_mday;
        this->myTime.tm_hour = rt.tm_hour;
        this->myTime.tm_min = rt.tm_min;
        this->myTime.tm_isdst = rt.tm_isdst;
    }
    TRACE("exit");
}

bool RTC::setTime(std::string datetime) {
    std::string cmd = "/sbin/hwclock --set --localtime --epoch=1900 --date=\"" + datetime + "\";/sbin/hwclock --hctosys";
    TRACE("running : %s", cmd.c_str());
    std::system(cmd.c_str());
    return true;
}

// RTC section ends

// SysClock section starts

void SysClock::refresh() {
    TRACE("enter");
    time_t theTime = time(NULL);
    this->myTime = (*localtime(&theTime));
    TRACE("exit");
}

bool SysClock::setTime(std::string datetime) {
    return true;
}

// SysClock section ends
