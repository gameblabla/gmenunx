#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <string>
#include <vector>
#include "esoteric.h"
#include "surfacecollection.h"

class Renderer {

private:

    SDL_TimerID timerId_;
    int interval_;
    bool finished_;
    bool locked_;
    bool polling_;
    
    Esoteric *app;

    int8_t brightnessIcon;
    Surface *iconBrightness[6];
    int8_t batteryIcon;
	Surface *iconBattery[7];
	Surface *iconVolume[3];

	Surface *iconSD;
	Surface *iconManual;
	Surface *iconCPU;
    Surface *highlighter;

    std::vector<Surface*> helpers;
    SurfaceCollection *backgrounds;

    uint8_t currentVolumeMode;

	void layoutHelperIcons(std::vector<Surface*> icons, int rootXPos, int rootYPos);
    uint8_t getVolumeMode(uint8_t vol);

    void pollHW();

public:

    Renderer(Esoteric * app);
    ~Renderer();
    void startPolling();
    void stopPolling();
    void render();
    void quit();

    static uint32_t callback(uint32_t interval, void * data);

};

#endif //__RENDERER_H__
