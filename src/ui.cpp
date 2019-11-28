#include "ui.h"

int UI::drawButton(Button *btn, int x, int y) {
	if (y < 0) y = this->gmenu2x->config->resolutionY() + y;
	btn->setPosition(x, y - 7);
	btn->paint();
	return x + btn->getRect().w + 6;
}

int UI::drawButton(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y < 0) y = this->gmenu2x->config->resolutionY() + y;
	SDL_Rect re = {x, y, 0, 16};
	int padding = 4;

	if (this->gmenu2x->sc->skinRes("imgs/buttons/" + btn + ".png") != NULL) {
		int imageWidth = (*this->gmenu2x->sc)["imgs/buttons/" + btn + ".png"]->raw->w;
		(*this->gmenu2x->sc)["imgs/buttons/" + btn + ".png"]->blit(
			s, 
			re.x + (imageWidth / 2), 
			re.y, 
			HAlignCenter | VAlignMiddle);
		re.w = imageWidth + padding;

		s->write(
			this->gmenu2x->font, 
			text, 
			re.x + re.w, 
			re.y,
			VAlignMiddle, 
			this->gmenu2x->skin->colours.fontAlt, 
			this->gmenu2x->skin->colours.fontAltOutline);
		re.w += this->gmenu2x->font->getTextWidth(text);
	}
	return x + re.w + (2 * padding);
}

int UI::drawButtonRight(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y < 0) y = this->gmenu2x->config->resolutionY() + y;
	// y = config->resolutionY - skinConfInt["bottomBarHeight"] / 2;
	if (this->gmenu2x->sc->skinRes("imgs/buttons/" + btn + ".png") != NULL) {
		x -= 16;
		(*this->gmenu2x->sc)["imgs/buttons/" + btn + ".png"]->blit(s, x + 8, y + 2, HAlignCenter | VAlignMiddle);
		x -= 3;
		s->write(
			this->gmenu2x->font, 
			text, 
			x, 
			y, 
			HAlignRight | VAlignMiddle, 
			this->gmenu2x->skin->colours.fontAlt, 
			this->gmenu2x->skin->colours.fontAltOutline);
		return x - 6 - this->gmenu2x->font->getTextWidth(text);
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

	this->gmenu2x->screen->rectangle(
		scrollRect.x + scrollRect.w - 4, 
		by, 
		4, 
		bs, 
		this->gmenu2x->skin->colours.listBackground);
	this->gmenu2x->screen->box(
		scrollRect.x + scrollRect.w - 3, 
		by + 1, 
		2, 
		bs - 2, 
		this->gmenu2x->skin->colours.selectionBackground);
}

void UI::drawSlider(int val, int min, int max, Surface &icon, Surface &bg) {

	SDL_Rect progress = {52, 32, this->gmenu2x->config->resolutionX()-84, 8};
	SDL_Rect box = {20, 20, this->gmenu2x->config->resolutionX()-40, 32};

	val = constrain(val, min, max);

	bg.blit(this->gmenu2x->screen,0,0);
	this->gmenu2x->screen->box(box, this->gmenu2x->skin->colours.msgBoxBackground);
	this->gmenu2x->screen->rectangle(box.x+2, box.y+2, box.w-4, box.h-4, this->gmenu2x->skin->colours.msgBoxBorder);

	icon.blit(this->gmenu2x->screen, 28, 28);

	this->gmenu2x->screen->box(progress, this->gmenu2x->skin->colours.msgBoxBackground);
	this->gmenu2x->screen->box(
		progress.x + 1, 
		progress.y + 1, 
		val * (progress.w - 3) / max + 1, 
		progress.h - 2, 
		this->gmenu2x->skin->colours.msgBoxSelection);
	this->gmenu2x->screen->flip();
}
