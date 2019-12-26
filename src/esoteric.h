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

#ifndef _ESOTERIC_H_
#define _ESOTERIC_H_

#include "surfacecollection.h"
#include "iconbutton.h"
#include "translator.h"
#include "FastDelegate.h"
#include "utilities.h"
#include "touchscreen.h"
#include "inputmanager.h"
#include "managers/screenmanager.h"
#include "managers/powermanager.h"
#include "surface.h"
#include "fonthelper.h"
#include "skin.h"
#include "config.h"
#include "ui.h"
#include "hw/hwfactory.h"

#ifdef HAVE_LIBOPK
#include "opkcache.h"
#endif

#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tr1/unordered_map>

typedef fastdelegate::FastDelegate0<> MenuAction;
typedef std::tr1::unordered_map<std::string, string, std::hash<std::string> > ConfStrHash;
typedef std::tr1::unordered_map<std::string, int, std::hash<std::string> > ConfIntHash;

struct MenuOption {
	std::string text;
	MenuAction action;
};

class Menu;
class UI;

#ifdef HAVE_LIBOPK
class OpkCache;
#endif

class Esoteric {
private:

	int screenHalfWidth, screenHalfHeight;

	bool needsInstalling;
	std::string exe_path; //!< Contains the working directory of Esoteric
	std::string writeable_path; // Contains the assets path

	/*!
	Starts the scanning of the nand and sd filesystems, searching for gpe and gpu files and creating the links in 2 dedicated sections.
	*/
	void linkScanner();
	/*!
	Performs the actual scan in the given path and populates the files vector with the results. The creation of the links is not performed here.
	@see scanner
	*/
	// void scanPath(string path, vector<string> *files);

	/*!
	Displays a selector and launches the specified executable file
	*/
	void explorer();

	std::string lastSelectorDir;
	int lastSelectorElement;
	void readTmp();

	void initFont();
	void showManual();

	void mountSdDialog();
	void umountSdDialog();

	void doInstall();
	void doUnInstall();
	void doUpgrade();
	bool doInitialSetup();

	void deviceMenu();
	void skinMenu();
	uint32_t onChangeSkin();

	void addLink();
	void editLink();
	void hideLink();
	void deleteLink();
	void addSection();
	void hideSection();
	void renameSection();
	void deleteSection();

public:
	Esoteric();
	~Esoteric();

	static void quit_all(int err);
	void quit();
	void releaseScreen();

	bool f200 = true;
	uint32_t linkWidth, linkHeight, linkSpacing = 4;
	SDL_Rect listRect, linksRect, sectionBarRect;

	const std::string &getExePath();
	const std::string &getWriteablePath();
	const std::string &getReadablePath();

	Touchscreen ts;
	Translator tr;

	SurfaceCollection *sc;
	ScreenManager *screenManager;
	PowerManager *powerManager;
	InputManager *inputManager;
	Surface *screen, *bg;
	FontHelper *font = NULL;
	FontHelper *fontTitle = NULL;
	FontHelper *fontSectionTitle = NULL; 

	//Status functions
	void main();
	void settings();
	void restartDialog(bool showDialog = false);
	void poweroffDialog();
	void resetSettings();
	void cpuSettings();

	void skinColors();
	void initLayout();
	void initMenu();

	void about();
	void viewLog();
	void performanceMenu();
	void contextMenu();
	void changeWallpaper();

	void setInputSpeed();

	void writeConfig();
	void writeSkinConfig();
	void writeTmp(int selelem=-1, const std::string &selectordir = "");


	void setWallpaper(const std::string &wallpaper = "");
	void updateAppCache(std::function<void(std::string)> callback = nullptr);
	void cacheChanged(const DesktopFile &file, const bool &added);

	int getScreenWidth() { return this->screen->raw->w; };
	int getScreenHeight() { return this->screen->raw->h; };
	int getScreenHalfWidth (){ return this->screenHalfWidth; };
	int getScreenHalfHeight() { return this->screenHalfHeight; };

	Menu* menu;
	Skin* skin;
	Config* config;
	UI* ui;
	IHardware * hw;

	#ifdef HAVE_LIBOPK
	OpkCache* cache;
	#endif

};

#endif
