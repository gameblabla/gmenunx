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
#include "menusettingmultistring.h"
#include "esoteric.h"
#include "iconbutton.h"
#include "debug.h"
#include <algorithm>

using std::find;

MenuSettingMultiString::MenuSettingMultiString(
		Esoteric *app, const string &title,
		const string &description, string *value,
		const vector<string> *choices_,
		msms_onchange_t onChange, msms_onselect_t onSelect)
	: MenuSettingStringBase(app, title, description, value), choices(choices_),
	onChange(onChange), onSelect(onSelect)
{
	TRACE("Initialised with value : %s", (*value).c_str());
	setSel(find(choices->begin(), choices->end(), *value) - choices->begin());

	if (choices->size() > 1) {
		btn = new IconButton(app, "skin:imgs/buttons/left.png");
		btn->setAction(MakeDelegate(this, &MenuSettingMultiString::decSel));
		buttonBox.add(btn);

		btn = new IconButton(app, "skin:imgs/buttons/right.png", app->tr["Change"]);
		btn->setAction(MakeDelegate(this, &MenuSettingMultiString::incSel));
		buttonBox.add(btn);
	}

	if (this->onSelect) {
		btn = new IconButton(app, "skin:imgs/buttons/a.png", app->tr["Open"]);
		// btn->setAction(MakeDelegate(this, &MenuSettingMultiString::incSel));
		buttonBox.add(btn);
	}
}

uint32_t MenuSettingMultiString::manageInput() {
	if ((*this->app->inputManager)[LEFT]) {
		decSel();
		return this->onChange && this->onChange();
	}
	else if ((*this->app->inputManager)[RIGHT]) {
		incSel();
		return this->onChange && this->onChange();
	}
	else if ((*this->app->inputManager)[CONFIRM] && this->onSelect) {
		this->onSelect();
		return this->onChange && this->onChange();
	}
	else if ((*this->app->inputManager)[MENU]) {
		setSel(0);
		return this->onChange && this->onChange();
	}
	return 0; // SD_NO_ACTION
}

void MenuSettingMultiString::incSel() {
	setSel(selected + 1);
}

void MenuSettingMultiString::decSel() {
	setSel(selected - 1);
}

void MenuSettingMultiString::setSel(int sel)
{
	if (sel < 0) {
		sel = choices->size()-1;
	} else if (sel >= (int)choices->size()) {
		sel = 0;
	}
	selected = sel;

	setValue((*choices)[sel]);
	TRACE("current value : %s", (*choices)[sel].c_str());
}
