/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo   *
 *   massimiliano.torromeo@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "menusetting.h"
#include "esoteric.h"

MenuSetting::MenuSetting(Esoteric *app, const std::string &title, const std::string &description)
	: app(app), buttonBox(app), title(title), description(description) {
}

MenuSetting::~MenuSetting() {}

void MenuSetting::draw(int y) {
	app->screen->write( app->font, title, 5, y + app->font->getHalfHeight(), VAlignMiddle );
}

void MenuSetting::handleTS() {
	buttonBox.handleTS();
}

void MenuSetting::adjustInput() {}

void MenuSetting::drawSelected(int /*y*/) {
	buttonBox.paint(5);
}
