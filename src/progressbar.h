#ifndef PROGRESSBAR_H_
#define PROGRESSBAR_H_

#include <string>
#include "gmenu2x.h"

class ProgressBar {
private:
	std::string title_, detail_, icon;
	int bgalpha, boxPadding, boxHeight, titleWidth;
	GMenu2X *gmenu2x;
	string formatText(const std::string & text);
    bool finished_;
    SDL_TimerID timerId_;
    int interval_ = 100;

public:
	ProgressBar(GMenu2X *gmenu2x, const std::string &title, const std::string &icon="");
    ~ProgressBar();
	void exec();
    static uint32_t render(uint32_t interval, void * data);
	void setBgAlpha(bool bgalpha);
	void updateDetail(std::string text);
    void finished(int millis = 0);
	void myCallback(std::string text);
    static void callback(void * this_pointer, string text);
};

#endif /*PROGRESSBAR_H_*/
