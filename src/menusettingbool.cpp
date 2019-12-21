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
#include "menusettingbool.h"
#include "esoteric.h"
using fastdelegate::MakeDelegate;

MenuSettingBool::MenuSettingBool(Esoteric *app, const string &title, const string &description, int *value)
	: MenuSetting(app, title, description)
{
	_ivalue = value;
	_value = NULL;
	originalValue = *value != 0;
	setValue(this->value());
	initButton();
}

MenuSettingBool::MenuSettingBool(Esoteric *app, const string &title, const string &description, bool *value)
	: MenuSetting(app, title, description)
{
	_value = value;
	_ivalue = NULL;
	originalValue = *value;
	setValue(this->value());
	initButton();
}

void MenuSettingBool::initButton()
{
	ButtonAction actionToggle = MakeDelegate(this, &MenuSettingBool::toggle);

	btn = new IconButton(app, "skin:imgs/buttons/left.png");
	btn->setAction(actionToggle);
	buttonBox.add(btn);

	btn = new IconButton(app, "skin:imgs/buttons/right.png");
	btn->setAction(actionToggle);
	buttonBox.add(btn);

	btn = new IconButton(app, "skin:imgs/buttons/a.png", app->tr["Change"]);
	btn->setAction(actionToggle);
	buttonBox.add(btn);
}

void MenuSettingBool::draw(int y)
{
	MenuSetting::draw(y);

	RGBAColor color = (RGBAColor){255, 0, 0, 255};
	if (value()) color = (RGBAColor) {0, 255, 0, 255};

	int w = app->font->getHeight()/2.5;
	app->screen->box(155, y + 1, w, app->font->getHeight() - 2, color);
	app->screen->rectangle(155, y + 1, w, app->font->getHeight() - 2, 0, 0, 0, 255);
	app->screen->write( app->font, strvalue, 155 + w + 2, y + app->font->getHalfHeight(), VAlignMiddle );
}

uint32_t MenuSettingBool::manageInput()
{
	if ( (*app->inputManager)[LEFT] || (*app->inputManager)[RIGHT] || (*app->inputManager)[CONFIRM] ) {
		toggle();
	}
	return 0; // SD_NO_ACTION
}

void MenuSettingBool::toggle()
{
	setValue(!value());
}

void MenuSettingBool::setValue(int value)
{
	setValue(value != 0);
}

void MenuSettingBool::setValue(bool value)
{
	if (_value == NULL)
		*_ivalue = value;
	else
		*_value = value;
	strvalue = value ? "ON" : "OFF";
}

bool MenuSettingBool::value()
{
	if (_value == NULL)
		return *_ivalue != 0;
	else
		return *_value;
}

bool MenuSettingBool::edited()
{
	return originalValue != value();
}
