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

class PowerManager;

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

extern const char *CARD_ROOT;

const string USER_PREFIX = "/media/data/local/home/.gmenunx/";


// Note: Keep this in sync with colorNames!
enum color {
	COLOR_TOP_BAR_BG,
	COLOR_LIST_BG,
	COLOR_BOTTOM_BAR_BG,
	COLOR_SELECTION_BG,
	COLOR_MESSAGE_BOX_BG,
	COLOR_MESSAGE_BOX_BORDER,
	COLOR_MESSAGE_BOX_SELECTION,
	COLOR_FONT,
	COLOR_FONT_OUTLINE,
	COLOR_FONT_ALT,
	COLOR_FONT_ALT_OUTLINE,

	NUM_COLORS,
};

/*
enum sb {
	SB_OFF,
	SB_LEFT,
	SB_BOTTOM,
	SB_RIGHT,
	SB_TOP,
};
*/
using std::string;
using std::vector;
using fastdelegate::FastDelegate0;

typedef FastDelegate0<> MenuAction;
typedef unordered_map<string, string, std::hash<string> > ConfStrHash;
typedef unordered_map<string, int, std::hash<string> > ConfIntHash;
// typedef unordered_map<string, RGBAColor, hash<string> > ConfColorHash;

struct MenuOption {
	string text;
	MenuAction action;
};

char *ms2hms(uint32_t t, bool mm, bool ss);

class Menu;

class GMenu2X {
private:
	int getBacklight();
	int getVolume();

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
	// void writeCommonIni();

	void initFont();
	void initMenu();
	void showManual();
	// IconButton *btnContextMenu;

#ifdef TARGET_GP2X
	typedef struct {
		uint16_t batt;
		uint16_t remocon;
	} MMSP2ADC;

	int batteryHandle;
	string ip, defaultgw;
	
	bool inet, //!< Represents the configuration of the basic network services. @see readCommonIni @see usbnet @see samba @see web
		usbnet,
		samba,
		web;
	volatile uint16_t *MEM_REG;
	int cx25874; //tv-out
	void gp2x_tvout_on(bool pal);
	void gp2x_tvout_off();
	void readCommonIni();
	void initServices();

#endif
	
	//void formatSd();
	void checkUDC();
	void umountSdDialog();
	string umountSd();
	void mountSdDialog();
	string mountSd();

	// private Configuration hashes
	ConfStrHash confStr;
	//ConfStrHash skinConfStr;

public:
	GMenu2X();
	~GMenu2X();
	void quit();
	void releaseScreen();

	// public Configuration hashes
	// TODO - move these to private as well....
	//RGBAColor skinConfColors[NUM_COLORS];
	//ConfIntHash skinConfInt;
	ConfIntHash confInt;

	/*
	 * Variables needed for elements disposition
	 */
	uint32_t resX, resY, halfX, halfY;
	// uint32_t bottomBarIconY, bottomBarTextY
	uint32_t linkCols, linkRows, linkWidth, linkHeight, linkSpacing = 4;
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
	const string &getAssetsPath();

	string getCurrentSkinPath();

	bool copyAssets();

	ScreenManager screenManager;
	InputManager input;
	Touchscreen ts;

	LED *led;
	// uint32_t tickSuspend; //, tickPowerOff;

	void setSkin(const string &skin, bool resetWallpaper = true, bool clearSC = true);
	//firmware type and version
	string fwType = ""; //, fwVersion;
	//gp2x type
	bool f200 = true;

	SurfaceCollection sc;
	Translator tr;
	Surface *s, *bg;
	FontHelper *font = NULL, *titlefont = NULL; //, *bottombarfont;

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

	PowerManager *powerManager;

#if defined(TARGET_GP2X)
	void writeConfigOpen2x();
	void readConfigOpen2x();
	void settingsOpen2x();
	// Open2x settings ---------------------------------------------------------
	bool o2x_usb_net_on_boot, o2x_ftp_on_boot, o2x_telnet_on_boot, o2x_gp2xjoy_on_boot, o2x_usb_host_on_boot, o2x_usb_hid_on_boot, o2x_usb_storage_on_boot;
	string o2x_usb_net_ip;
	int savedVolumeMode;		//	just use the const int scale values at top of source

	//  Volume scaling values to store from config files
	int volumeScalerPhones;
	int volumeScalerNormal;
	//--------------------------------------------------------------------------
	void activateSdUsb();
	void activateNandUsb();
	void activateRootUsb();
	void applyRamTimings();
	void applyDefaultTimings();
	void setGamma(int gamma);
	void setVolumeScaler(int scaler);
	int getVolumeScaler();
#endif

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

	void drawSlider(int val, int min, int max, Surface &icon, Surface &bg);
	bool saveScreenshot();
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
	void renameSection();
	void deleteSection();

	void setWallpaper(const string &wallpaper = "");

	int drawButton(Button *btn, int x=5, int y=-10);
	int drawButton(Surface *s, const string &btn, const string &text, int x=5, int y=-10);
	int drawButtonRight(Surface *s, const string &btn, const string &text, int x=5, int y=-10);
	void drawScrollBar(uint32_t pagesize, uint32_t totalsize, uint32_t pagepos, SDL_Rect scrollRect);

	Menu* menu;

	Skin* skin;

};

#endif
