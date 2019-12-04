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
#include "menusettingstringbase.h"
#include "esoteric.h"
#include "debug.h"

using std::string;

MenuSettingStringBase::MenuSettingStringBase(
		Esoteric *app, const string &title,
		const string &description, string *value)
	: MenuSetting(app, title, description)
	, originalValue(*value)
	, _value(value)
{
}

MenuSettingStringBase::~MenuSettingStringBase() {
}

void MenuSettingStringBase::draw(int y) {
	MenuSetting::draw(y);
	app->screen->write(app->font, value(), 155, y + app->font->getHalfHeight(), VAlignMiddle);
}

uint32_t MenuSettingStringBase::manageInput() {
	if (app->input[MENU]) clear();
	if (app->input[CONFIRM]) edit();
	return 0; // SD_NO_ACTION
}

void MenuSettingStringBase::clear() {
	setValue("");
}

bool MenuSettingStringBase::edited() {
	return originalValue != value();
}
