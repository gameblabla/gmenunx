#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <string>
#include "gmenu2x.h"
#include "rtc.h"

class Renderer {

private:

    SDL_TimerID timerId_;
    int interval_ = 2000;
    bool finished_;
    bool locked_;
    GMenu2X *gmenu2x;
    RTC rtc;

	string prevBackdrop;
	string currBackdrop;

    int8_t brightnessIcon;
    Surface *iconBrightness[6];
    int8_t batteryIcon;
	Surface *iconBattery[7];
	Surface *iconVolume[3];

	Surface *iconSD;
	Surface *iconManual;
	Surface *iconCPU;

    vector<Surface*> helpers;
    uint8_t currentVolumeMode;

	void layoutHelperIcons(vector<Surface*> icons, Surface *target, int helperHeight, int * rootXPosPtr, int * rootYPosPtr, int iconsPerRow);
    uint8_t getVolumeMode(uint8_t vol);

    void pollHW();

public:

    Renderer(GMenu2X * gmenu2x);
    ~Renderer();
    void render();
    void quit();

    static uint32_t callback(uint32_t interval, void * data);

};

#endif //__RENDERER_H__
