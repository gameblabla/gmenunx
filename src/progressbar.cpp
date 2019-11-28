#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <chrono>

#include "progressbar.h"
#include "powermanager.h"
#include "debug.h"
#include "utilities.h"

using namespace std;


ProgressBar::ProgressBar(GMenu2X *gmenu2x, const string &title, const string &icon) {
	this->gmenu2x = gmenu2x;
	this->title_ = title;
    this->detail_ = "";
	this->icon = icon;
	this->bgalpha = 200;
    this->finished_ = false;
    this->timerId_ = 0;
    this->boxPadding = 24 + ((*this->gmenu2x->sc)[this->icon] != NULL ? 37 : 0);
    this->titleWidth = this->gmenu2x->font->getTextWidth(this->title_);
	if (this->titleWidth + this->boxPadding > this->gmenu2x->config->resolutionX()) {
		this->titleWidth = this->gmenu2x->config->resolutionX(); 
	}
    this->boxHeight = 2 * (this->gmenu2x->font->getTextHeight(this->title_) * this->gmenu2x->font->getHeight()) + this->gmenu2x->font->getHeight();

}

ProgressBar::~ProgressBar() {
    TRACE("enter");
    if (this->timerId_ > 0) {
        SDL_RemoveTimer(this->timerId_);
    }
    this->gmenu2x->input.dropEvents(); 
	this->gmenu2x->powerManager->resetSuspendTimer();
    TRACE("exit");
}

void ProgressBar::setBgAlpha(bool bgalpha) {
	this->bgalpha = bgalpha;
}

string ProgressBar::formatText(const string & text) {
	int wrap_size = ((gmenu2x->config->resolutionX() - (this->boxPadding / 2)) / gmenu2x->font->getSize() + 15);
	TRACE("final wrap size : %i", wrap_size);
	string wrappedText = splitInLines(text, wrap_size);
	TRACE("wrap text : %s", wrappedText.c_str());
	return wrappedText;
}

uint32_t ProgressBar::render(uint32_t interval, void * data) {

	ProgressBar * me = static_cast<ProgressBar*>(data);

    if (me->finished_) {
        TRACE("finished");
        SDL_SetTimer(0, NULL);
        SDL_RemoveTimer(me->timerId_);
        interval = 0;
    } else {
        //TRACE("rendering");
        me->gmenu2x->screen->box(
            (SDL_Rect){ 0, 0, me->gmenu2x->config->resolutionX(), me->gmenu2x->config->resolutionY() }, 
            (RGBAColor){0,0,0, me->bgalpha}
        );

        SDL_Rect box;
        box.h = me->boxHeight;
        if ((*me->gmenu2x->sc)[me->icon] != NULL && box.h < 40) box.h = 48;
        box.w = me->titleWidth + me->boxPadding;
        box.x = me->gmenu2x->config->halfX() - box.w/2 - 2;
        box.y = me->gmenu2x->config->halfY() - box.h/2 - 2;

        //outer box
        me->gmenu2x->screen->box(box, me->gmenu2x->skin->colours.msgBoxBackground);
        
        //draw inner rectangle
        me->gmenu2x->screen->rectangle(
            box.x+2, 
            box.y+2, 
            box.w-4, 
            box.h-4, 
            me->gmenu2x->skin->colours.msgBoxBorder);

        //icon+wrapped_text
        if ((*me->gmenu2x->sc)[me->icon] != NULL)
            (*me->gmenu2x->sc)[me->icon]->blit(
                me->gmenu2x->screen, 
                box.x + 24, 
                box.y + 24 , 
                HAlignCenter | VAlignMiddle);

        string finalText = me->title_ + "\n" + me->detail_;
        me->gmenu2x->screen->write(
            me->gmenu2x->font, 
            finalText, 
            box.x + ((*me->gmenu2x->sc)[me->icon] != NULL ? 47 : 11), 
            me->gmenu2x->config->halfY() - me->gmenu2x->font->getHeight() / 5, 
            VAlignMiddle, 
            me->gmenu2x->skin->colours.fontAlt, 
            me->gmenu2x->skin->colours.fontAltOutline);

        me->gmenu2x->screen->flip();
    }

    return(interval);
}

void ProgressBar::exec() {
	TRACE("enter");
	this->finished_ = false;
	gmenu2x->powerManager->clearTimer();
    this->timerId_ = SDL_AddTimer(this->interval_, render, this);
}

void ProgressBar::finished(int millis) {
    TRACE("finished called : %ims", millis);
    SDL_RemoveTimer(this->timerId_);
    this->timerId_ = 0;
    this->finished_ = true; 
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

void ProgressBar::updateDetail(std::string text) {
    this->detail_ = text;
}
