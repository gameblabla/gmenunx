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

#ifndef GMENU2X_H
#define GMENU2X_H

#include "surfacecollection.h"
#include "iconbutton.h"
#include "translator.h"
#include "FastDelegate.h"
#include "utilities.h"
#include "touchscreen.h"
#include "inputmanager.h"
#include "screenmanager.h"
#include "surface.h"
#include "fonthelper.h"
#include "led.h"
#include "skin.h"
#include "config.h"
#include "ui.h"

#ifdef HAVE_LIBOPK
#include "opkcache.h"
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tr1/unordered_map>

const int MAX_VOLUME_SCALE_FACTOR = 200;
// Default values - going to add settings adjustment, saving, loading and such
const int VOLUME_SCALER_MUTE = 0;
const int VOLUME_SCALER_PHONES = 65;
const int VOLUME_SCALER_NORMAL = 100;
const int BATTERY_READS = 10;

using std::string;
using std::vector;
using fastdelegate::FastDelegate0;

typedef FastDelegate0<> MenuAction;
typedef std::tr1::unordered_map<string, string, std::hash<string> > ConfStrHash;
typedef std::tr1::unordered_map<string, int, std::hash<string> > ConfIntHash;

struct MenuOption {
	string text;
	MenuAction action;
};

char *ms2hms(uint32_t t, bool mm, bool ss);

class PowerManager;
class Menu;
class UI;
#ifdef HAVE_LIBOPK
class OpkCache;
#endif

class GMenu2X {
private:

	string exe_path; //!< Contains the working directory of GMenu2X
	string assets_path; // Contains the assets path

	/*!
	Retrieves the free disk space on the sd
	@return String containing a human readable representation of the free disk space
	*/
	string getDiskFree(const char *path);

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

	string lastSelectorDir;
	int lastSelectorElement;
	void readConfig();
	void readTmp();

	void initFont();
	void showManual();

	//void formatSd();
	void checkUDC();
	void umountSdDialog();
	string umountSd();
	void mountSdDialog();
	string mountSd();
	bool doInstall();
	bool doUpgrade();

	#ifdef HAVE_LIBOPK
	OpkCache* opkCache;
	#endif

public:
	GMenu2X(bool install = false);
	~GMenu2X();
	void quit();
	void releaseScreen();

	int getBacklight();
	int getVolume();
	int16_t curMMCStatus;
	uint32_t linkWidth, linkHeight, linkSpacing = 4;
	SDL_Rect listRect, linksRect, sectionBarRect;

	/*!
	Retrieves the parent directory of GMenu2X.
	This functions is used to initialize the "exe_path" variable.
	@see exe_path
	@return String containing the parent directory
	*/
	const string &getExePath();
	/*!
	Retrieves the directory that gmenunx assets live in.
	This functions is used to initialize the "assets_path" variable.
	@see assets_path
	@return String containing the parent directory
	*/
	string getAssetsPath();

	ScreenManager screenManager;
	InputManager input;
	Touchscreen ts;

	LED *led;

	//firmware type and version
	string fwType = "";
	//gp2x type
	bool f200 = true;

	SurfaceCollection *sc;
	Translator tr;
	Surface *screen, *bg;
	FontHelper *font = NULL, *fontTitle = NULL, *fontSectionTitle = NULL; 

	//Status functions
	void main();
	void settings();
	void restartDialog(bool showDialog = false);
	void poweroffDialog();
	void resetSettings();
	void cpuSettings();

	/*!
	Reads the current battery state and returns a number representing it's level of charge
	@return A number representing battery charge. 0 means fully discharged. 5 means fully charged. 6 represents a gp2x using AC power.
	*/
	int getBatteryLevel();

	void skinMenu();
	void skinColors();
	uint32_t onChangeSkin();
	void initLayout();
	void initMenu();

	PowerManager *powerManager;

	void setTVOut(string _TVOut);
	string TVOut = "OFF";
	void about();
	void viewLog();
	void batteryLogger();
	void performanceMenu();
	void setPerformanceMode();
	string getPerformanceMode();
	void contextMenu();
	void changeWallpaper();

	void setCPU(uint32_t mhz);
	const string getDateTime();
	void setDateTime();
	int setVolume(int val);
	int setBacklight(int val);

	void setInputSpeed();

	void writeConfig();
	void writeSkinConfig();
	void writeTmp(int selelem=-1, const string &selectordir = "");

	void ledOn();
	void ledOff();

	void addLink();
	void editLink();
	void deleteLink();
	void addSection();
	void hideSection();
	void renameSection();
	void deleteSection();

	void setWallpaper(const string &wallpaper = "");
	void updateAppCache(std::function<void(string)> callback = nullptr);

	Menu* menu;
	Skin* skin;
	Config* config;
	UI* ui;

};

#endif
