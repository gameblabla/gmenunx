#include <math.h>
#include <SDL_timer.h>

#include "ui.h"

int UI::drawButton(Button *btn, int x, int y) {
	if (y < 0) y = this->app->getScreenHeight() + y;
	btn->setPosition(x, y - 7);
	btn->paint();
	return x + btn->getRect().w + 6;
}

int UI::drawButton(Surface *s, const std::string &btn, const std::string &text, int x, int y) {
	if (y < 0) y = this->app->getScreenHeight() + y;
	SDL_Rect re = {x, y, 0, 16};
	int padding = 4;

	if (this->app->sc->skinRes("imgs/buttons/" + btn + ".png") != NULL) {
		int imageWidth = (*this->app->sc)["imgs/buttons/" + btn + ".png"]->raw->w;
		(*this->app->sc)["imgs/buttons/" + btn + ".png"]->blit(
			s, 
			re.x + (imageWidth / 2), 
			re.y, 
			HAlignCenter | VAlignMiddle);
		re.w = imageWidth + padding;

		s->write(
			this->app->font, 
			text, 
			re.x + re.w, 
			re.y,
			VAlignMiddle, 
			this->app->skin->colours.fontAlt, 
			this->app->skin->colours.fontAltOutline);
		re.w += this->app->font->getTextWidth(text);
	}
	return x + re.w + (2 * padding);
}

int UI::drawButtonRight(Surface *s, const std::string &btn, const std::string &text, int x, int y) {
	if (y < 0) {
		y = this->app->getScreenHeight() + y;
	}

	if (this->app->sc->skinRes("imgs/buttons/" + btn + ".png") != NULL) {
		x -= 16;
		(*this->app->sc)["imgs/buttons/" + btn + ".png"]->blit(s, x + 8, y + 2, HAlignCenter | VAlignMiddle);
		x -= 3;
		s->write(
			this->app->font, 
			text, 
			x, 
			y, 
			HAlignRight | VAlignMiddle, 
			this->app->skin->colours.fontAlt, 
			this->app->skin->colours.fontAltOutline);
		return x - 6 - this->app->font->getTextWidth(text);
	}
	return x - 6;
}

void UI::drawScrollBar(uint32_t pagesize, uint32_t totalsize, uint32_t pagepos, SDL_Rect scrollRect) {
	if (totalsize <= pagesize) return;

	//internal bar total height = height-2
	//bar size
	uint32_t bs = (scrollRect.h - 3) * pagesize / totalsize;
	//bar y position
	uint32_t by = (scrollRect.h - 3) * pagepos / totalsize;
	by = scrollRect.y + 3 + by;
	if ( by + bs > scrollRect.y + scrollRect.h - 4) by = scrollRect.y + scrollRect.h - 4 - bs;

	this->app->screen->rectangle(
		scrollRect.x + scrollRect.w - 4, 
		by, 
		4, 
		bs, 
		this->app->skin->colours.listBackground);
	this->app->screen->box(
		scrollRect.x + scrollRect.w - 3, 
		by + 1, 
		2, 
		bs - 2, 
		this->app->skin->colours.selectionBackground);
}

void UI::drawSlider(int val, int min, int max, Surface &icon, Surface &bg) {

	SDL_Rect progress = {52, 32, this->app->getScreenWidth()-84, 8};
	SDL_Rect box = {20, 20, this->app->getScreenWidth()-40, 32};

	val = constrain(val, min, max);

	bg.blit(this->app->screen,0,0);
	this->app->screen->box(box, this->app->skin->colours.msgBoxBackground);
	this->app->screen->rectangle(box.x+2, box.y+2, box.w-4, box.h-4, this->app->skin->colours.msgBoxBorder);

	icon.blit(this->app->screen, 28, 28);

	this->app->screen->box(progress, this->app->skin->colours.msgBoxBackground);
	this->app->screen->box(
		progress.x + 1, 
		progress.y + 1, 
		val * (progress.w - 3) / max + 1, 
		progress.h - 2, 
		this->app->skin->colours.msgBoxSelection);
	this->app->screen->flip();
}

int UI::intTransition(int from, int to, int32_t tickStart, int32_t duration, int32_t tickNow) {
	if (tickNow < 0) tickNow = SDL_GetTicks();
	float elapsed = (float)(tickNow-tickStart)/duration;
	//                    elapsed                 increments
	return min((int)round(elapsed * (to - from)), (int)max(from, to));
}
