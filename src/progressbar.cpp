#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <chrono>

#include "progressbar.h"
#include "debug.h"
#include "utilities.h"

using namespace std;

ProgressBar::ProgressBar(Esoteric *app, const std::string &title, const std::string &icon, const int width) {
    TRACE("enter - width : %i", width);
	this->app = app;
	this->title_ = title;
    this->detail_ = "";
	this->icon = icon;
	this->bgalpha = 200;
    this->finished_ = false;
    this->timerId_ = 0;
    this->boxPadding = 24 + ((*this->app->sc)[this->icon] != NULL ? 37 : 0);
    this->maxWidth = this->app->config->resolutionX() - this->boxPadding;

    if (width < 0) {
        this->maxWidth = this->app->config->resolutionX() + width - this->boxPadding;
    } else {
        int textWidth = this->app->font->getLineWidth(this->title_);
        if (textWidth + this->boxPadding > this->app->config->resolutionX()) {
            textWidth = this->app->config->resolutionX();
        }
        if (width > 0) {
            if (width < textWidth) {
                this->maxWidth = textWidth;
            } else {
                if (width > this->app->config->resolutionX()) {
                    this->maxWidth = this->app->config->resolutionX() - this->boxPadding;
                } else this->maxWidth = width;
            }
        } else {
            this->maxWidth = textWidth;
        }
    }
    this->lineHeight = this->app->font->getHeight();
    this->boxHeight = (3.5 * this->lineHeight);
    TRACE("box padding : %i", this->boxPadding);
    TRACE("line height : %i", this->lineHeight);
    TRACE("box height : %i", this->boxHeight);
    TRACE("max width : %i", this->maxWidth);
    TRACE("exit");
}

ProgressBar::~ProgressBar() {
    TRACE("enter");
    this->free();
    this->app->input.dropEvents(); 
    TRACE("exit");
}

void ProgressBar::free() {
    TRACE("enter");
    if (this->timerId_ > 0) {
        SDL_SetTimer(0, NULL);
        SDL_RemoveTimer(this->timerId_);
    }
    TRACE("exit");
}

void ProgressBar::setBgAlpha(bool bgalpha) {
	this->bgalpha = bgalpha;
}

uint32_t ProgressBar::render(uint32_t interval, void * data) {

	ProgressBar * me = static_cast<ProgressBar*>(data);

    if (me->finished_) {
        TRACE("finished");
        me->free();
        return 0;
    }
    //TRACE("rendering");
    me->app->screen->box(
        (SDL_Rect){ 0, 0, me->app->config->resolutionX(), me->app->config->resolutionY() }, 
        (RGBAColor){0, 0, 0, me->bgalpha}
    );

    SDL_Rect box;
    box.h = me->boxHeight;
    if ((*me->app->sc)[me->icon] != NULL && box.h < 40) box.h = 48;
    box.w = me->maxWidth + me->boxPadding;
    box.x = me->app->config->halfX() - box.w / 2 - 2;
    box.y = me->app->config->halfY() - box.h / 2 - 2;

    //outer box
    me->app->screen->box(box, me->app->skin->colours.msgBoxBackground);
        
    //draw inner rectangle
    me->app->screen->rectangle(
        box.x + 2, 
        box.y + 2, 
        box.w - 4, 
        box.h - 4, 
        me->app->skin->colours.msgBoxBorder);

    //icon + wrapped_text
    if ((*me->app->sc)[me->icon] != NULL)
        (*me->app->sc)[me->icon]->blit(
            me->app->screen, 
            box.x + 24, 
            box.y + 24 , 
            HAlignCenter | VAlignMiddle);

    int iY = box.y + me->app->font->getHalfHeight();
    me->app->screen->write(
        me->app->font, 
        me->title_, 
        box.x + ((*me->app->sc)[me->icon] != NULL ? 47 : 11), 
        iY, 
        0, 
        me->app->skin->colours.fontAlt, 
        me->app->skin->colours.fontAltOutline);

    iY += me->lineHeight + me->app->font->getHalfHeight();
    me->app->screen->write(
        me->app->font, 
        me->detail_, 
        box.x + ((*me->app->sc)[me->icon] != NULL ? 47 : 11), 
        iY, 
        0, 
        me->app->skin->colours.fontAlt, 
        me->app->skin->colours.fontAltOutline);

    me->app->screen->flip();
    return(interval);
}

void ProgressBar::exec() {
	TRACE("enter");
    this->free();
	this->finished_ = false;
    this->timerId_ = SDL_AddTimer(this->interval_, render, this);
}

void ProgressBar::finished(int millis) {
    TRACE("finished called : %ims", millis);
    if (millis > 0) {
        ProgressBar::render(this->interval_, this);
    }
    this->finished_ = true; 
    this->free();
    std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

void ProgressBar::updateDetail(const std::string &text) {
    TRACE("enter : %s", text.c_str());
    if(!(this->app || this->app->font)) return;
    if (this->finished_) return;
    std::string localText = std::string(text);
    try {
        TRACE("getting text width for : '%s'",  localText.c_str());
        int textWidth = this->app->font->getLineWidthSafe(localText);
        TRACE("text width : %i, maxWidth : %i", textWidth, this->maxWidth);
        if (textWidth > this->maxWidth) {
            while (textWidth > this->maxWidth) {
                TRACE("%s : %i > %i", localText.c_str(), textWidth, this->maxWidth);
                localText = localText.substr(0, localText.length() -1);
                textWidth = this->app->font->getLineWidthSafe(localText + "...");
            }
            localText += "...";
        }
    } catch(std::exception& e) {
        ERROR("Couldn't get text size for :%s", localText.c_str());
        ERROR("Exception : %s", e.what());
    }
    this->detail_ = localText;
    TRACE("exit : %s", localText.c_str());
}
