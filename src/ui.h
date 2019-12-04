#ifndef _UI_
#define _UI_

#include "surface.h"
#include "esoteric.h"

class UI {
    private:
        Esoteric * app;
    public:
        UI(Esoteric * app) { this->app = app; };
        void drawSlider(int val, int min, int max, Surface &icon, Surface &bg);
        int drawButton(Button *btn, int x=5, int y=-10);
        int drawButton(Surface *s, const string &btn, const string &text, int x=5, int y=-10);
        int drawButtonRight(Surface *s, const string &btn, const string &text, int x=5, int y=-10);
        void drawScrollBar(uint32_t pagesize, uint32_t totalsize, uint32_t pagepos, SDL_Rect scrollRect);
};

#endif
