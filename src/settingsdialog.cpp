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

#include "settingsdialog.h"
#include "messagebox.h"

using namespace std;

SettingsDialog::SettingsDialog(
		Esoteric *app, Touchscreen &ts_,
		const string &title, const string &icon)
	: Dialog(app)
	, ts(ts_)
	, title(title)
{
	if (!icon.empty() && (*app->sc)[icon] != NULL)
		this->icon = icon;
	else
		this->icon = "icons/generic.png";
}

SettingsDialog::~SettingsDialog() {
	for (uint32_t i = 0; i < voices.size(); i++)
		delete voices[i];
}

bool SettingsDialog::exec() {
	bool ts_pressed = false, inputAction = false;
	uint32_t i, iY, firstElement = 0, action = SD_NO_ACTION, rowHeight, numRows;
	voices[selected]->adjustInput();

	while (!close) {
		app->initLayout();
		app->font->
			setSize(app->skin->fontSize)->
			setColor(app->skin->colours.font)->
			setOutlineColor(app->skin->colours.fontOutline);

		app->fontTitle->
			setSize(app->skin->fontSizeTitle)->
			setColor(app->skin->colours.fontAlt)->
			setOutlineColor(app->skin->colours.fontAltOutline);

		app->fontSectionTitle->
			setSize(app->skin->fontSizeSectionTitle)->
			setColor(app->skin->colours.fontAlt)->
			setOutlineColor(app->skin->colours.fontAltOutline);

		rowHeight = app->font->getHeight() + 1;
		numRows = (app->listRect.h - 2)/rowHeight - 1;

		this->bg->blit(app->screen,0,0);

		// redraw to due to realtime skin
		drawTopBar(app->screen, title, voices[selected]->getDescription(), icon);
		drawBottomBar(app->screen);
		app->screen->box(app->listRect, app->skin->colours.listBackground);

		//Selection
		if (selected >= firstElement + numRows) firstElement = selected - numRows;
		if (selected < firstElement) firstElement = selected;

		iY = app->listRect.y + 1;
		for (i = firstElement; i < voices.size() && i <= firstElement + numRows; i++, iY += rowHeight) {
			if (i == selected) {
				app->screen->box(app->listRect.x, iY, app->listRect.w, rowHeight, app->skin->colours.selectionBackground);
				voices[selected]->drawSelected(iY);
			}
			voices[i]->draw(iY);
		}

		app->ui->drawScrollBar(
			numRows, 
			voices.size(), 
			firstElement, 
			app->listRect);

		app->screen->flip();
		do {
			inputAction = app->input.update();

			action = SD_NO_ACTION;
			if ( app->input[SETTINGS] ) action = SD_ACTION_SAVE;
			else if ( app->input[CANCEL] && allowCancel) action = SD_ACTION_CLOSE;
			else if ( app->input[UP      ] ) action = SD_ACTION_UP;
			else if ( app->input[DOWN    ] ) action = SD_ACTION_DOWN;
			else if ( app->input[PAGEUP  ] ) action = SD_ACTION_PAGEUP;
			else if ( app->input[PAGEDOWN] ) action = SD_ACTION_PAGEDOWN;
			else action = voices[selected]->manageInput();
			switch (action) {
				case SD_ACTION_SAVE:
					save = true;
					close = true;
					break;
				case SD_ACTION_CLOSE:
					close = true;
					if (allowCancel) {
						if (edited()) {
							MessageBox mb(app, app->tr["Save changes?"], this->icon);
							mb.setButton(CONFIRM, app->tr["Yes"]);
							mb.setButton(CANCEL,  app->tr["No"]);
							save = (mb.exec() == CONFIRM);
						}}
					break;
				case SD_ACTION_UP:
					selected -= 1;
					if (selected < 0) selected = voices.size() - 1;
					app->setInputSpeed();
					voices[selected]->adjustInput();
					break;
				case SD_ACTION_DOWN:
					selected += 1;
					if (selected >= voices.size()) selected = 0;
					app->setInputSpeed();
					voices[selected]->adjustInput();
					break;
				case SD_ACTION_PAGEUP:
					selected -= numRows;
					if (selected < 0) selected = 0;
					break;
				case SD_ACTION_PAGEDOWN:
					selected += numRows;
					if (selected >= voices.size()) selected = voices.size() - 1;
					break;
			}
		} while (!inputAction);
	}

	app->setInputSpeed();

	return true;
}

void SettingsDialog::addSetting(MenuSetting* set) {
	voices.push_back(set);
}

bool SettingsDialog::edited() {
	for (uint32_t i = 0; i < voices.size(); i++)
		if (voices[i]->edited()) return true;
	return false;
}
