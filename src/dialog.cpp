#include <string>

#include "dialog.h"
#include "esoteric.h"

Dialog::Dialog(Esoteric *app) : app(app) {
	bg = new Surface(app->bg);
}

Dialog::~Dialog() {
	delete bg;
}

void Dialog::drawTitleIcon(const std::string &icon, Surface *s) {
	if (s == NULL) 
		s = app->screen;

	Surface *i = NULL;
	if (!icon.empty()) {
		i = (*app->sc)[icon];
		if (i == NULL) i = app->sc->skinRes(icon);
	}
	if (i == NULL) 
		i = app->sc->skinRes("icons/generic.png");

	i->blit(
		s, 
		{ 4, 4, app->getScreenWidth() - 8, app->skin->menuTitleBarHeight - 8 }, 
		VAlignMiddle);

	this->iconWidth = i->raw->w;

}

void Dialog::writeTitle(const std::string &title, Surface *s) {
	if (s == NULL) s = app->screen;
	s->write(
		app->fontTitle, 
		title, 
		10 + this->iconWidth, 
		app->fontTitle->getHeight()/2, 
		VAlignMiddle, 
		app->skin->colours.fontAlt, 
		app->skin->colours.fontAltOutline);
	// s->box(40, 16 - app->titlefont->getHalfHeight(), 15, app->titlefont->getHeight(), strtorgba("ff00ffff"));
}

void Dialog::writeSubTitle(const std::string &subtitle, Surface *s) {
	if (s == NULL) s = app->screen;
	s->write(
		app->font, 
		subtitle, 
		10 + this->iconWidth, 
		38, 
		VAlignBottom, 
		app->skin->colours.fontAlt, 
		app->skin->colours.fontAltOutline);
	// s->box(40, 32 - app->font->getHalfHeight(), 15, app->font->getHeight(), strtorgba("00ffffff"));
}

void Dialog::drawTopBar(Surface *s = NULL, const std::string &title, const std::string &description, const std::string &icon) {
	if (s == NULL) s = app->screen;
	// Surface *bar = sc.skinRes("imgs/topbar.png");
	// if (bar != NULL) bar->blit(s, 0, 0);
	// else
	s->setClipRect({ 0, 0, app->getScreenWidth(), app->skin->menuTitleBarHeight });
	s->box(
		0, 
		0, 
		app->getScreenWidth(), 
		app->skin->menuTitleBarHeight, 
		app->skin->colours.titleBarBackground);

	if (!icon.empty()) drawTitleIcon(icon, s);
	if (!title.empty()) writeTitle(title, s);
	if (!description.empty()) writeSubTitle(description, s);
	
	s->clearClipRect();
}

void Dialog::drawBottomBar(Surface *s) {
	if (s == NULL) s = app->screen;
	s->box(
		0, 
		app->getScreenHeight() - app->skin->menuInfoBarHeight, 
		app->getScreenWidth(), 
		app->skin->menuInfoBarHeight, 
		app->skin->colours.infoBarBackground);
}
