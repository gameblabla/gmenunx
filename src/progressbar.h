#ifndef PROGRESSBAR_H_
#define PROGRESSBAR_H_

#include <string>
#include "gmenu2x.h"

using std::string;
using std::vector;

class ProgressBar {
private:
	string title_, detail_, icon;
	int bgalpha, boxPadding, boxHeight, titleWidth;
	GMenu2X *gmenu2x;
	string formatText(const string & text);
    bool finished_;
    SDL_TimerID timerId_;
    int interval_ = 100;

public:
	ProgressBar(GMenu2X *gmenu2x, const string &title, const string &icon="");
    ~ProgressBar();
	void exec();
    static uint32_t render(uint32_t interval, void * data);
	void setBgAlpha(bool bgalpha);
	void updateDetail(string text);
    void finished(int millis = 0);
    static void callback(void * this_pointer, string text);
};

#endif /*PROGRESSBAR_H_*/
