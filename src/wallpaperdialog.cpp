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

#include "wallpaperdialog.h"
#include "filelister.h"
#include "debug.h"

using namespace std;

WallpaperDialog::WallpaperDialog(Esoteric *app, const string &title, const string &description, const string &icon)
	: Dialog(app) {
	this->title = title;
	this->description = description;
	this->icon = icon;
	this->wallpaper = app->skin->wallpaper;
}

bool WallpaperDialog::exec()
{
	TRACE("enter");
	bool close = false, result = true, inputAction = false;

	uint32_t i, iY, firstElement = 0;
	uint32_t rowHeight = app->font->getHeight() + 1;
	uint32_t numRows = (app->listRect.h - 2)/rowHeight - 1;
	int32_t selected = 0;
	string wallpaperPath = this->app->skin->currentSkinPath() + "/wallpapers/";

	vector<string> wallpapers;
	wallpapers = app->skin->getWallpapers();
	
	wallpaper = base_name(wallpaper);
	TRACE("wallpaper base name resolved : %s", wallpaper.c_str());

	for (uint32_t i = 0; i < wallpapers.size(); i++) {
		if (wallpaper == wallpapers[i]) selected = i;
	}
	TRACE("selected wallpaper #%i of %zu", selected, wallpapers.size());
	
	TRACE("looping on user input");
	while (!close) {

		string skinPath = wallpaperPath + wallpapers[selected];

		TRACE("blitting surface : %s", skinPath.c_str());
		(*app->sc)[skinPath]->blit(app->screen,0,0);

		TRACE("drawTopBar");
		drawTopBar(app->screen, title, description, icon);
		TRACE("drawBottomBar");
		drawBottomBar(app->screen);
		TRACE("box");
		app->screen->box(app->listRect, app->skin->colours.listBackground);

		TRACE("drawButtons");
		app->ui->drawButton(app->screen, "a", app->tr["Select"],
		app->ui->drawButton(app->screen, "start", app->tr["Exit"],5));

		//Selection
		if (selected >= firstElement + numRows) firstElement = selected - numRows;
		if (selected < firstElement) firstElement = selected;

		//Files & Directories
		iY = app->listRect.y + 1;
		TRACE("loop dirs");
		for (i = firstElement; i < wallpapers.size() && i <= firstElement + numRows; i++, iY += rowHeight) {
			if (i == selected) app->screen->box(app->listRect.x, iY, app->listRect.w, rowHeight, app->skin->colours.selectionBackground);
			app->screen->write(app->font, wallpapers[i], app->listRect.x + 5, iY + rowHeight/2, VAlignMiddle);
		}

		TRACE("drawScrollBar");
		app->ui->drawScrollBar(numRows, wallpapers.size(), firstElement, app->listRect);
		TRACE("flip");
		app->screen->flip();

		TRACE("loop");
		do {
			inputAction = app->inputManager->update();

			TRACE("got an action");
			if ( (*app->inputManager)[UP] ) {
				selected -= 1;
				if (selected < 0) selected = wallpapers.size() - 1;
			} else if ( (*app->inputManager)[DOWN] ) {
				selected += 1;
				if (selected >= wallpapers.size()) selected = 0;
			} else if ( (*app->inputManager)[PAGEUP] || (*app->inputManager)[LEFT] ) {
				selected -= numRows;
				if (selected < 0) selected = 0;
			} else if ( (*app->inputManager)[PAGEDOWN] || (*app->inputManager)[RIGHT] ) {
				selected += numRows;
				if (selected >= wallpapers.size()) selected = wallpapers.size() - 1;
			} else if ( (*app->inputManager)[SETTINGS] || (*app->inputManager)[MENU] || (*app->inputManager)[CANCEL] ) {
				close = true;
				result = false;
			} else if ( (*app->inputManager)[CONFIRM] ) {
				close = true;
				if (wallpapers.size() > 0) {
					wallpaper = wallpaperPath + wallpapers[selected];
				} else {
					result = false;
				}
			}
		} while (!inputAction);
	}
	TRACE("end of inputActions");
	
	TRACE("looping through %zu wallpapers", wallpapers.size());
	for (uint32_t i = 0; i < wallpapers.size(); i++) {
		string skinPath = wallpaperPath + wallpapers[i];
		TRACE("deleting surface from collection : %s", skinPath.c_str());
		app->sc->del(skinPath);
	}
	TRACE("exit - %i", result);
	return result;
}
