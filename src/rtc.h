#ifndef __RTC_H__
#define __RTC_H__

#include <linux/rtc.h>
#include <string>

class RTC {

private:
    struct rtc_time rt;

public:

    RTC();
    ~RTC();
    std::string getClockTime(bool is24hr = false);
    std::string getDateTime();
    int getHours();
    int getMinutes();
    bool setTime(std::string datetime);
    void refresh();

};

#endif //__RTC_H__
