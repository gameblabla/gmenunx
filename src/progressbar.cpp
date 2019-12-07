#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <chrono>

#include "progressbar.h"
#include "debug.h"
#include "utilities.h"

using namespace std;

ProgressBar::ProgressBar(Esoteric *app, const string &title, const string &icon) {
	this->app = app;
	this->title_ = title;
    this->detail_ = "";
	this->icon = icon;
	this->bgalpha = 200;
    this->finished_ = false;
    this->timerId_ = 0;
    this->boxPadding = 24 + ((*this->app->sc)[this->icon] != NULL ? 37 : 0);
    this->titleWidth = this->app->font->getTextWidth(this->title_);
	if (this->titleWidth + this->boxPadding > this->app->config->resolutionX()) {
		this->titleWidth = this->app->config->resolutionX(); 
	}
    this->boxHeight = 2 * (this->app->font->getTextHeight(this->title_) * this->app->font->getHeight()) + this->app->font->getHeight();

}

ProgressBar::~ProgressBar() {
    TRACE("enter");
    if (this->timerId_ > 0) {
        SDL_RemoveTimer(this->timerId_);
    }
    this->app->input.dropEvents(); 
    TRACE("exit");
}

void ProgressBar::setBgAlpha(bool bgalpha) {
	this->bgalpha = bgalpha;
}

string ProgressBar::formatText(const string & text) {
	int wrap_size = ((app->config->resolutionX() - (this->boxPadding / 2)) / app->font->getSize() + 15);
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
        me->app->screen->box(
            (SDL_Rect){ 0, 0, me->app->config->resolutionX(), me->app->config->resolutionY() }, 
            (RGBAColor){0,0,0, me->bgalpha}
        );

        SDL_Rect box;
        box.h = me->boxHeight;
        if ((*me->app->sc)[me->icon] != NULL && box.h < 40) box.h = 48;
        box.w = me->titleWidth + me->boxPadding;
        box.x = me->app->config->halfX() - box.w/2 - 2;
        box.y = me->app->config->halfY() - box.h/2 - 2;

        //outer box
        me->app->screen->box(box, me->app->skin->colours.msgBoxBackground);
        
        //draw inner rectangle
        me->app->screen->rectangle(
            box.x+2, 
            box.y+2, 
            box.w-4, 
            box.h-4, 
            me->app->skin->colours.msgBoxBorder);

        //icon+wrapped_text
        if ((*me->app->sc)[me->icon] != NULL)
            (*me->app->sc)[me->icon]->blit(
                me->app->screen, 
                box.x + 24, 
                box.y + 24 , 
                HAlignCenter | VAlignMiddle);

        string finalText = me->title_ + "\n" + me->detail_;
        me->app->screen->write(
            me->app->font, 
            finalText, 
            box.x + ((*me->app->sc)[me->icon] != NULL ? 47 : 11), 
            me->app->config->halfY() - me->app->font->getHeight() / 5, 
            VAlignMiddle, 
            me->app->skin->colours.fontAlt, 
            me->app->skin->colours.fontAltOutline);

        me->app->screen->flip();
    }

    return(interval);
}

void ProgressBar::exec() {
	TRACE("enter");
	this->finished_ = false;
    this->timerId_ = SDL_AddTimer(this->interval_, render, this);
}

void ProgressBar::finished(int millis) {
    TRACE("finished called : %ims", millis);
    if (millis > 0) {
        ProgressBar::render(this->interval_, this);
    }
    SDL_RemoveTimer(this->timerId_);
    this->timerId_ = 0;
    this->finished_ = true; 
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

void ProgressBar::updateDetail(std::string text) {
    TRACE("enter : %s", text.c_str());
    int textWidth = this->app->font->getTextWidth(text);
    if (textWidth > this->titleWidth && this->titleWidth > 3) {
        while (textWidth > this->titleWidth) {
            TRACE("%s : %i > %i", text.c_str(), textWidth, this->titleWidth);
            text = text.substr(0, text.length() -2);
            textWidth = this->app->font->getTextWidth(text + "...");
        }
        text += "...";
    }
    this->detail_ = text;
    TRACE("exit : %s", text.c_str());
}
