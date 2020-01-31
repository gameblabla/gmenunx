#include "debug.h"
#include "utilities.h"
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
    std::string result = string_format(
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
