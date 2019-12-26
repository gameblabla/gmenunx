#ifndef __SysClock_H__
#define __SysClock_H__

#include <string>
#include <time.h>

#include "iclock.h"

class SysClock : IClock {

private:

    struct tm myTime;
    void refresh();

public:

    SysClock();
    ~SysClock();
    std::string getClockTime(bool is24hr = false);
    std::string getDateTime();

    int getHours();
    int getMinutes();
    int getYear();
    int getMonth();
    int getDay();
    bool setTime(std::string datetime);

};

#endif //__SysClock_H__
