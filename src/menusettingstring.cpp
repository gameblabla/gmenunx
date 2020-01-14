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
#include "menusettingstring.h"
#include "iconbutton.h"
#include "inputdialog.h"

MenuSettingString::MenuSettingString(
		Esoteric *app, const std::string &title,
		const std::string &description, std::string *value,
		const std::string &dialogTitle, const std::string &dialogIcon)
		: MenuSettingStringBase(app, title, description, value)
		, dialogTitle(dialogTitle)
		, dialogIcon(dialogIcon) {

	btn = new IconButton(app, "skin:imgs/buttons/select.png", app->tr["Clear"]);
	btn->setAction(fastdelegate::MakeDelegate(this, &MenuSettingString::clear));
	buttonBox.add(btn);

	btn = new IconButton(app, "skin:imgs/buttons/a.png", app->tr["Edit"]);
	btn->setAction(fastdelegate::MakeDelegate(this, &MenuSettingString::edit));
	buttonBox.add(btn);
}

void MenuSettingString::edit() {
	InputDialog id(app, app->ts, description, value(), dialogTitle, dialogIcon);
	if (id.exec()) setValue(id.getInput());
}
