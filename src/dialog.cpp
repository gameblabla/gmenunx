#include <string>

#include "dialog.h"
#include "gmenu2x.h"

Dialog::Dialog(GMenu2X *gmenu2x) : gmenu2x(gmenu2x) {
	bg = new Surface(gmenu2x->bg);
}

Dialog::~Dialog() {
	delete bg;
}

void Dialog::drawTitleIcon(const std::string &icon, Surface *s) {
	if (s == NULL) 
		s = gmenu2x->screen;

	Surface *i = NULL;
	if (!icon.empty()) {
		i = (*gmenu2x->sc)[icon];
		if (i == NULL) i = gmenu2x->sc->skinRes(icon);
	}
	if (i == NULL) 
		i = gmenu2x->sc->skinRes("icons/generic.png");

	i->blit(
		s, 
		{ 4, 4, gmenu2x->config->resolutionX() - 8, gmenu2x->skin->menuTitleBarHeight - 8 }, 
		VAlignMiddle);

	this->iconWidth = i->raw->w;

}

void Dialog::writeTitle(const std::string &title, Surface *s) {
	if (s == NULL) s = gmenu2x->screen;
	s->write(
		gmenu2x->fontTitle, 
		title, 
		10 + this->iconWidth, 
		gmenu2x->fontTitle->getHeight()/2, 
		VAlignMiddle, 
		gmenu2x->skin->colours.fontAlt, 
		gmenu2x->skin->colours.fontAltOutline);
	// s->box(40, 16 - gmenu2x->titlefont->getHalfHeight(), 15, gmenu2x->titlefont->getHeight(), strtorgba("ff00ffff"));
}

void Dialog::writeSubTitle(const std::string &subtitle, Surface *s) {
	if (s == NULL) s = gmenu2x->screen;
	s->write(
		gmenu2x->font, 
		subtitle, 
		10 + this->iconWidth, 
		38, 
		VAlignBottom, 
		gmenu2x->skin->colours.fontAlt, 
		gmenu2x->skin->colours.fontAltOutline);
	// s->box(40, 32 - gmenu2x->font->getHalfHeight(), 15, gmenu2x->font->getHeight(), strtorgba("00ffffff"));
}

void Dialog::drawTopBar(Surface *s = NULL, const std::string &title, const std::string &description, const std::string &icon) {
	if (s == NULL) s = gmenu2x->screen;
	// Surface *bar = sc.skinRes("imgs/topbar.png");
	// if (bar != NULL) bar->blit(s, 0, 0);
	// else
	s->setClipRect({ 0, 0, gmenu2x->config->resolutionX(), gmenu2x->skin->menuTitleBarHeight });
	s->box(
		0, 
		0, 
		gmenu2x->config->resolutionX(), 
		gmenu2x->skin->menuTitleBarHeight, 
		gmenu2x->skin->colours.titleBarBackground);

	if (!icon.empty()) drawTitleIcon(icon, s);
	if (!title.empty()) writeTitle(title, s);
	if (!description.empty()) writeSubTitle(description, s);
	
	s->clearClipRect();
}

void Dialog::drawBottomBar(Surface *s) {
	if (s == NULL) s = gmenu2x->screen;
	s->box(
		0, 
		gmenu2x->config->resolutionY() - gmenu2x->skin->menuInfoBarHeight, 
		gmenu2x->config->resolutionX(), 
		gmenu2x->skin->menuInfoBarHeight, 
		gmenu2x->skin->colours.infoBarBackground);
}
