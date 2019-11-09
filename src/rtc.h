#ifndef __RTC_H__
#define __RTC_H__

#include <string>

class RTC {
public:

    std::string static getTime();
    bool static setTime(std::string datetime);

private:

};

#endif //__RTC_H__
