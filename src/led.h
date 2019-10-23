#ifndef LED_H_
#define LED_H_

#include <string>

using std::string;

class LED {

	public:

		LED();
		void reset();
		void flash(const int &speed = 250);
		~LED();

	private:

		enum AllowedTriggers {
			NONE = 0, 
			TIMER
		};

		string max_brightness;
		const string PREFIX = "/sys/class/leds/power/";
		const string MAX_BRIGHTNESS_PATH = PREFIX + "max_brightness";
		const string BRIGHTNESS_PATH = PREFIX + "brightness";
		const string DELAY_ON_PATH = PREFIX + "delay_on";
		const string DELAY_OFF_PATH = PREFIX + "delay_off";
		const string TRIGGER_PATH = PREFIX + "trigger";

		string triggerToString(AllowedTriggers t);

};

#endif /*LED_H_*/
