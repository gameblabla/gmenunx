#ifndef __RTC_H__
#define __RTC_H__

#include <linux/rtc.h>
#include <string>

#include "hw-clock.h"

class RTC : IClock {

private:
    struct rtc_time rt;
    void refresh();

public:

    RTC();
    ~RTC();
    std::string getClockTime(bool is24hr = false);
    std::string getDateTime();
    int getHours();
    int getMinutes();
    int getYear();
    int getMonth();
    int getDay();

    bool setTime(std::string datetime);

};

#endif //__RTC_H__
