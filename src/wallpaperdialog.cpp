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

WallpaperDialog::WallpaperDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon)
	: Dialog(gmenu2x) {
	this->title = title;
	this->description = description;
	this->icon = icon;
}

bool WallpaperDialog::exec()
{
	TRACE("WallpaperDialog::exec - enter");
	bool close = false, result = true, inputAction = false;

	uint32_t i, iY, firstElement = 0;
	uint32_t rowHeight = gmenu2x->font->getHeight() + 1;
	uint32_t numRows = (gmenu2x->listRect.h - 2)/rowHeight - 1;
	int32_t selected = 0;
	string assetsPath = gmenu2x->getAssetsPath();
	
	FileLister fl(assetsPath + "skins/" + gmenu2x->confStr["skin"] + "/wallpapers");
	fl.setFilter(".png,.jpg,.jpeg,.bmp");
	vector<string> wallpapers;
	
	if (dirExists(assetsPath + "skins/" + gmenu2x->confStr["skin"] + "/wallpapers")) {
		TRACE("WallpaperDialog::exec - dir found, calling fl.browse");
		fl.browse();
		TRACE("WallpaperDialog::exec - dir found, calling fl.getFiles");
		wallpapers = fl.getFiles();
	}
	
	if (gmenu2x->confStr["skin"] != "Default") {
		TRACE("WallpaperDialog::exec - current skin isn't default");
		fl.setPath(assetsPath + "skins/Default/wallpapers", true);
		for (uint32_t i = 0; i < fl.getFiles().size(); i++)
			wallpapers.push_back(fl.getFiles()[i]);
	}

	wallpaper = base_name(gmenu2x->confStr["wallpaper"]);
	TRACE("WallpaperDialog::exec - wallpaper base name resolved :: %s", wallpaper.c_str());

	for (uint32_t i = 0; i < wallpapers.size(); i++) {
		if (wallpaper == wallpapers[i]) selected = i;
	}
	TRACE("WallpaperDialog::exec - selected wallpaper #%i of %i", selected, wallpapers.size());
	
	TRACE("WallpaperDialog::exec - looping on user input");
	while (!close) {
		//Wallpaper
		
		string skinPath;
		if (selected < wallpapers.size() - fl.getFiles().size())
			skinPath = assetsPath + "skins/"+gmenu2x->confStr["skin"] + "/wallpapers/" + wallpapers[selected];
		else
			skinPath = assetsPath + "skins/Default/wallpapers/" + wallpapers[selected];
		TRACE("WallpaperDialog::exec - blitting surface :: %s", skinPath.c_str());
		gmenu2x->sc[skinPath]->blit(gmenu2x->s,0,0);

		TRACE("WallpaperDialog::exec - drawTopBar");
		drawTopBar(gmenu2x->s, title, description, icon);
		TRACE("WallpaperDialog::exec - drawBottomBar");
		drawBottomBar(gmenu2x->s);
		TRACE("WallpaperDialog::exec - box");
		gmenu2x->s->box(gmenu2x->listRect, gmenu2x->skinConfColors[COLOR_LIST_BG]);

		TRACE("WallpaperDialog::exec - drawButtons");
		gmenu2x->drawButton(gmenu2x->s, "a", gmenu2x->tr["Select"],
		gmenu2x->drawButton(gmenu2x->s, "start", gmenu2x->tr["Exit"],5));

		//Selection
		if (selected >= firstElement + numRows) firstElement = selected - numRows;
		if (selected < firstElement) firstElement = selected;

		//Files & Directories
		iY = gmenu2x->listRect.y + 1;
		TRACE("WallpaperDialog::exec - loop dirs");
		for (i = firstElement; i < wallpapers.size() && i <= firstElement + numRows; i++, iY += rowHeight) {
			if (i == selected) gmenu2x->s->box(gmenu2x->listRect.x, iY, gmenu2x->listRect.w, rowHeight, gmenu2x->skinConfColors[COLOR_SELECTION_BG]);
			gmenu2x->s->write(gmenu2x->font, wallpapers[i], gmenu2x->listRect.x + 5, iY + rowHeight/2, VAlignMiddle);
		}

		TRACE("WallpaperDialog::exec - drawScrollBar");
		gmenu2x->drawScrollBar(numRows, wallpapers.size(), firstElement, gmenu2x->listRect);
		TRACE("WallpaperDialog::exec - flip");
		gmenu2x->s->flip();

		TRACE("WallpaperDialog::exec - loop");
		do {
			inputAction = gmenu2x->input.update();

			TRACE("WallpaperDialog::exec - got an action");
			if ( gmenu2x->input[UP] ) {
				selected -= 1;
				if (selected < 0) selected = wallpapers.size() - 1;
			} else if ( gmenu2x->input[DOWN] ) {
				selected += 1;
				if (selected >= wallpapers.size()) selected = 0;
			} else if ( gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT] ) {
				selected -= numRows;
				if (selected < 0) selected = 0;
			} else if ( gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT] ) {
				selected += numRows;
				if (selected >= wallpapers.size()) selected = wallpapers.size() - 1;
			} else if ( gmenu2x->input[SETTINGS] || gmenu2x->input[MENU] || gmenu2x->input[CANCEL] ) {
				close = true;
				result = false;
			} else if ( gmenu2x->input[CONFIRM] ) {
				close = true;
				if (wallpapers.size() > 0) {
					if (selected < wallpapers.size() - fl.getFiles().size())
						wallpaper = assetsPath + "skins/" + gmenu2x->confStr["skin"] + "/wallpapers/" + wallpapers[selected];
					else
						wallpaper = assetsPath + "skins/Default/wallpapers/" + wallpapers[selected];
				} else {
					result = false;
				}
			}
		} while (!inputAction);
	}
	TRACE("WallpaperDialog::exec - end of inputActions");
	
	TRACE("WallpaperDialog::exec - looping through %i wallpapers", wallpapers.size());
	for (uint32_t i = 0; i < wallpapers.size(); i++) {
		string skinPath;
		if (i < wallpapers.size() - fl.getFiles().size())
			skinPath = assetsPath + "skins/" + gmenu2x->confStr["skin"] + "/wallpapers/" + wallpapers[i];
		else
			skinPath = assetsPath + "skins/Default/wallpapers/" + wallpapers[i];
		
		TRACE("WallpaperDialog::exec - deleting surface from collection :: %s", skinPath.c_str());
		gmenu2x->sc.del(skinPath);
	}

	gmenu2x->confStr["wallpaper"] = wallpaper;
	TRACE("WallpaperDialog::exec - exit - %i", result);
	return result;
}
