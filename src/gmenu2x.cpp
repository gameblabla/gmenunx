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

#include <iostream>
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <pwd.h>
#include <cassert>
#include <thread>

#include "linkapp.h"
#include "menu.h"
#include "fonthelper.h"
#include "surface.h"
#include "browsedialog.h"
#include "powermanager.h"
#include "gmenu2x.h"
#include "filelister.h"

#include "led.h"
#include "iconbutton.h"
#include "messagebox.h"
#include "screenmanager.h"
#include "inputdialog.h"
#include "settingsdialog.h"
#include "wallpaperdialog.h"
#include "textdialog.h"
#include "menusettingint.h"
#include "menusettingbool.h"
#include "menusettingrgba.h"
#include "menusettingstring.h"
#include "menusettingmultistring.h"
#include "menusettingfile.h"
#include "menusettingimage.h"
#include "menusettingdir.h"
#include "imageviewerdialog.h"
#include "batteryloggerdialog.h"
#include "linkscannerdialog.h"
#include "linkapp.h"
#include "menusettingdatetime.h"
#include "debug.h"
#include "skin.h"
#include "config.h"
#include "rtc.h"
#include "renderer.h"
#include "loader.h"

#ifdef HAVE_LIBOPK
#include "opkcache.h"
#endif

#define sync() sync(); system("sync");
#ifndef __BUILDTIME__
	#define __BUILDTIME__ __DATE__ " " __TIME__ 
#endif

// TODO this should probably point to /media/sdcard
const char *CARD_ROOT = "/media/data/local/home/";
const int MIN_CONFIG_VERSION = 1;
const string RG350_GET_VOLUME_PATH = "/usr/bin/alsa-getvolume default PCM";
const string RG350_SET_VOLUME_PATH = "/usr/bin/alsa-setvolume default PCM "; // keep trailing space
const string RG350_BACKLIGHT_PATH = "/sys/class/backlight/pwm-backlight/brightness";

static GMenu2X *app;

using std::ifstream;
using std::ofstream;
using std::stringstream;
using namespace fastdelegate;

char *ms2hms(uint32_t t, bool mm = true, bool ss = true) {
	static char buf[10];

	t = t / 1000;
	int s = (t % 60);
	int m = (t % 3600) / 60;
	int h = (t % 86400) / 3600;

	if (!ss) sprintf(buf, "%02d:%02d", h, m);
	else if (!mm) sprintf(buf, "%02d", h);
	else sprintf(buf, "%02d:%02d:%02d", h, m, s);
	return buf;
};

void printbin(const char *id, int n) {
	printf("%s: 0x%08x ", id, n);
	for(int i = 31; i >= 0; i--) {
		printf("%d", !!(n & 1 << i));
		if (!(i % 8)) printf(" ");
	}
	printf("\e[0K\n");
}

static void quit_all(int err) {
	delete app;
	exit(err);
}

int memdev = 0;
#ifdef TARGET_RS97
	volatile uint32_t *memregs;
#else
	volatile uint16_t *memregs;
#endif

enum mmc_status {
	MMC_MOUNTED, MMC_UNMOUNTED, MMC_MISSING, MMC_ERROR
};

int16_t tvOutPrev = false, tvOutConnected;
bool getTVOutStatus() {
	if (memdev > 0) return !(memregs[0x10300 >> 2] >> 25 & 0b1);
	return false;
}

GMenu2X::~GMenu2X() {
	TRACE("enter\n\n");
	quit();
	delete menu;
	delete screen;
	delete font;
	delete fontTitle;
	delete fontSectionTitle;
	delete led;
	delete skin;
	delete sc;
	delete config;
	#ifdef HAVE_LIBOPK
	delete opkCache;
	#endif
	TRACE("exit\n\n");
}

void GMenu2X::releaseScreen() {
	SDL_Quit();
}

void GMenu2X::quit() {
	TRACE("enter");
	if (!this->sc->empty()) {
		TRACE("SURFACE EXISTED");
		writeConfig();
		ledOff();
		fflush(NULL);
		this->sc->clear();
		this->screen->free();
		releaseScreen();
	}
	INFO("GMenuNX::quit - exit");
}

int main(int argc, char * argv[]) {
	INFO("GMenuNX starting: Build Date - %s - %s", __DATE__, __TIME__);

	signal(SIGINT, &quit_all);
	signal(SIGSEGV,&quit_all);
	signal(SIGTERM,&quit_all);

	// handle cmd args
    int opt = opterr = 0;
	bool install = false;

    // Retrieve the options:
    while ( (opt = getopt(argc, argv, "ih")) != -1 ) {
        switch ( opt ) {
            case 'i':
				install = true;
				break;
			case 'h':
				cout << "run with -i to install dependencies to writeable path";
				exit(0);
		};
	};

	app = new GMenu2X(install);
	TRACE("Starting app->main()");
	app->main();

	return 0;
}

bool exitMainThread = false;
void* mainThread(void* param) {
	GMenu2X *menu = (GMenu2X*)param;
	while(!exitMainThread) {
		sleep(1);
	}
	return NULL;
}

void GMenu2X::updateAppCache() {
	TRACE("enter");
	#ifdef HAVE_LIBOPK
	if (OPK_USE_CACHE) {
		TRACE("we're using the opk cache");
		string externalPath = this->config->externalAppPath();
		vector<string> opkDirs { OPK_INTERNAL_PATH, externalPath };
		string rootDir = getAssetsPath();
		TRACE("rootDir : %s", rootDir.c_str());
		if (nullptr == this->opkCache) {
			this->opkCache = new OpkCache(opkDirs, rootDir);
		}
		assert(this->opkCache);
		this->opkCache->update();
		sync();
	}
	#endif
	TRACE("exit");
}
GMenu2X::GMenu2X(bool install) : input(screenManager) {

	TRACE("enter - install flag : %i", install);
	TRACE("creating LED instance");
	led = new LED();
	
	TRACE("ledOn");
	ledOn();

	TRACE("getExePath");
	exe_path = getExePath();

	bool firstRun = !dirExists(getAssetsPath());
	string localAssetsPath;
	if (firstRun) {
		localAssetsPath = exe_path;
	} else {
		localAssetsPath = getAssetsPath();
	}
	TRACE("first run : %i, localAssetsPath : %s", firstRun, localAssetsPath.c_str());

	TRACE("init translations");
	tr.setPath(localAssetsPath);

	TRACE("readConfig from : %s", localAssetsPath.c_str());
	this->config = new Config(localAssetsPath);
	readConfig();

	// set this ASAP
	TRACE("backlight");
	setBacklight(config->backlightLevel());

	//Screen
	TRACE("setEnv");
	setenv("SDL_NOMOUSE", "1", 1);
	setenv("SDL_FBCON_DONT_CLEAR", "1", 0);

	TRACE("sdl_init");
	if ( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK) < 0 ) {
		ERROR("Could not initialize SDL: %s", SDL_GetError());
		quit();
	}
	TRACE("disabling the cursor");
	SDL_ShowCursor(SDL_DISABLE);

	TRACE("surface");
	this->screen = new Surface();

	TRACE("SDL_SetVideoMode - x:%i y:%i bpp:%i", config->resolutionX(), config->resolutionY(), config->videoBpp());
	this->screen->raw = SDL_SetVideoMode(config->resolutionX(), config->resolutionY(), config->videoBpp(), SDL_HWSURFACE|SDL_DOUBLEBUF);
	
	this->skin = new Skin(localAssetsPath, config->resolutionX(),  config->resolutionY());
	if (!this->skin->loadSkin( config->skin())) {
		ERROR("GMenu2X::ctor - couldn't load skin, using defaults");
	}
	this->sc = new SurfaceCollection(this->skin, true);

	TRACE("initFont");
	initFont();

	TRACE("screen manager");
	screenManager.setScreenTimeout( config->backlightTimeout());

	TRACE("power mgr");
	powerManager = new PowerManager(this,  config->backlightTimeout(), config->powerTimeout());

	if (firstRun) {
		int exitCode = 0;
		INFO("GMenu2X::ctor - first run, copying data");
		MessageBox mb(this, "Copying data for first run...", localAssetsPath + "gmenunx.png");
		mb.setAutoHide(-1);
		mb.exec();
		if (doInstall()) {
			mb.fadeOut(500);
			ledOff();
			quit();
			exitCode = 0;
		} else {
			ERROR("Fatal, can't copy core files");
			exitCode = 1;
		}
		quit_all(exitCode);
	} else 	if (install || MIN_CONFIG_VERSION > config->version()) {
		// we're doing an upgrade
		int exitCode = 0;
		INFO("GMenu2X::ctor - upgrade requested, config versions are %i and %i", config->version(), MIN_CONFIG_VERSION);
		MessageBox mb(this, "Upgrading new install...", getExePath() + "gmenunx.png");
		mb.setAutoHide(-1);
		mb.exec();
		if (doUpgrade(MIN_CONFIG_VERSION > config->version())) {
			mb.fadeOut(500);
			ledOff();
			quit();
			exitCode = 0;
		} else {
			ERROR("Fatal, can't upgrade core files");
			exitCode = 2;
		}
		quit_all(exitCode);
	}

	TRACE("exit");

}

void GMenu2X::main() {
	TRACE("enter");

	TRACE("kicking off our app cache thread");
	std::thread thread_cache(&GMenu2X::updateAppCache, this);

	this->input.init(this->getAssetsPath() + "input.conf");
	setInputSpeed();
	input.setWakeUpInterval(1000);

	Loader loader(this);
	loader.run();

	setVolume(this->config->globalVolume());
	setWallpaper(this->skin->wallpaper);
	setPerformanceMode();
	checkUDC();
	setCPU(this->config->cpuMenu());

	// we need to re-join before building the menu
	thread_cache.join();
	TRACE("app cache thread has finished");

	initMenu();
	readTmp();
	if (this->lastSelectorElement >- 1 && \
		this->menu->selLinkApp() != NULL && \
		(!this->menu->selLinkApp()->getSelectorDir().empty() || \
		!this->lastSelectorDir.empty())) {

		TRACE("recoverSession happening");
		this->menu->selLinkApp()->selector(
			this->lastSelectorElement, 
			this->lastSelectorDir);
	}

	// turn the blinker off
	ledOff();

	bool quit = false;

	TRACE("pthread");
	pthread_t thread_id;
	if (pthread_create(&thread_id, NULL, mainThread, this)) {
		ERROR("%s, failed to create main thread\n", __func__);
	}

	Renderer *renderer = new Renderer(this);

	while (!quit) {
		try {
			renderer->render();
		} catch (int e) {
			ERROR("render - e : %i", e);
		}

		if (input.isWaiting()) continue;
		bool inputAction = input.update();

		if (inputAction) {
			if ( input[QUIT] ) {
				quit = true;
			} else if ( input[CONFIRM] && menu->selLink() != NULL ) {
				TRACE("******************RUNNING THIS*******************");

				if (menu->selLinkApp() != NULL && menu->selLinkApp()->getSelectorDir().empty()) {
					MessageBox mb(this, tr["Launching "] + menu->selLink()->getTitle().c_str(), menu->selLink()->getIconPath());
					mb.setAutoHide(500);
					mb.exec();
				}

				TRACE("******************RUNNING THIS -- RUN*******************");
				menu->selLink()->run();
			} else if ( input[INC] ) {
				TRACE("******************favouriting an app THIS*******************");
				menu->selLinkApp()->makeFavourite();
			} else if ( input[SETTINGS] ) {
				settings();
			} else if ( input[MENU]     ) {
				contextMenu();
			} else if ( input[LEFT ] && skin->numLinkCols == 1) {
				menu->pageUp();
			} else if ( input[RIGHT] && skin->numLinkCols == 1) {
				menu->pageDown();
			} else if ( input[LEFT ] ) {
				menu->linkLeft();
			} else if ( input[RIGHT] ) {
				menu->linkRight();
			} else if ( input[UP   ] ) {
				menu->linkUp();
			} else if ( input[DOWN ] ) {
				menu->linkDown();
			} else if ( input[SECTION_PREV] ) {
				menu->decSectionIndex();
			} else if ( input[SECTION_NEXT] ) {
				menu->incSectionIndex();
			} else if (input[MANUAL] && menu->selLinkApp() != NULL) {
				showManual();
			}
		}
	}

	delete renderer;

	exitMainThread = true;
	pthread_join(thread_id, NULL);

	TRACE("exit");
}

void GMenu2X::setWallpaper(const string &wallpaper) {
	TRACE("enter : %s", wallpaper.c_str());
	if (bg != NULL) delete bg;

	TRACE("new surface");
	bg = new Surface(screen);
	TRACE("bg box");
	bg->box((SDL_Rect){ 0, 0, config->resolutionX(), config->resolutionY() }, (RGBAColor){0, 0, 0, 0});

	skin->wallpaper = wallpaper;
	TRACE("null test");
	if (wallpaper.empty()) {
		TRACE("simple background mode");
		
		SDL_FillRect(	bg->raw, 
						NULL, 
						SDL_MapRGBA(
							bg->raw->format, 
							skin->colours.background.r, 
							skin->colours.background.g, 
							skin->colours.background.b, 
							skin->colours.background.a));
		
	} else {
		if (this->sc->addImage(wallpaper) == NULL) {
			// try and add a default one
			string relativePath = "skins/" + this->skin->name + "/wallpapers";
			TRACE("searching for wallpaper in :%s", relativePath.c_str());

			FileLister fl(getAssetsPath() + relativePath, false, true);
			fl.setFilter(".png,.jpg,.jpeg,.bmp");
			fl.browse();
			if (fl.getFiles().size() > 0) {
				TRACE("found a wallpaper");
				skin->wallpaper = fl.getPath() + "/" + fl.getFiles()[0];
			}
		}
		if ((*this->sc)[skin->wallpaper]) {
			(*this->sc)[skin->wallpaper]->softStretch(
				this->config->resolutionX(), 
				this->config->resolutionY(), 
				false, 
				true);
			TRACE("blit");
			(*this->sc)[skin->wallpaper]->blit(bg, 0, 0);
		}
	}
	TRACE("exit");
}

void GMenu2X::initLayout() {
	TRACE("enter");
	// LINKS rect
	linksRect = (SDL_Rect){0, 0, config->resolutionX(), config->resolutionY()};
	sectionBarRect = (SDL_Rect){0, 0, config->resolutionX(), config->resolutionY()};

	if (skin->sectionBar) {
		if (skin->sectionBar == Skin::SB_LEFT || skin->sectionBar == Skin::SB_RIGHT) {
			sectionBarRect.x = (skin->sectionBar == Skin::SB_RIGHT)*(config->resolutionX() - skin->sectionTitleBarSize);
			sectionBarRect.w = skin->sectionTitleBarSize;
			linksRect.w = config->resolutionX() - skin->sectionTitleBarSize;

			if (skin->sectionBar == Skin::SB_LEFT) {
				linksRect.x = skin->sectionTitleBarSize;
			}
		} else {
			sectionBarRect.y = (skin->sectionBar == Skin::SB_BOTTOM)*(config->resolutionY() - skin->sectionTitleBarSize);
			sectionBarRect.h = skin->sectionTitleBarSize;
			linksRect.h = config->resolutionY() - skin->sectionTitleBarSize;

			if (skin->sectionBar == Skin::SB_TOP) {
				linksRect.y = skin->sectionTitleBarSize;
			}
		}
	}
	if (skin->sectionInfoBarVisible) {
		if (skin->sectionBar == Skin::SB_TOP || skin->sectionBar == Skin::SB_BOTTOM) {
			linksRect.h -= skin->sectionInfoBarSize;
			if (skin->sectionBar == Skin::SB_BOTTOM) {
				linksRect.y += skin->sectionInfoBarSize;
			}
		}
	}

	listRect = (SDL_Rect){
		0, 
		skin->menuTitleBarHeight, 
		config->resolutionX(), 
		config->resolutionY() - skin->menuInfoBarHeight - skin->menuTitleBarHeight};

	linkWidth  = (linksRect.w - (skin->numLinkCols + 1 ) * linkSpacing) / skin->numLinkCols;
	linkHeight = (linksRect.h - (skin->numLinkCols > 1) * (skin->numLinkRows    + 1 ) * linkSpacing) / skin->numLinkRows;

	TRACE("exit - cols: %i, rows: %i, width: %i, height: %i", skin->numLinkCols, skin->numLinkRows, linkWidth, linkHeight);
}

void GMenu2X::initFont() {
	TRACE("enter");

	string fontPath = this->skin->getSkinFilePath("font.ttf");
	TRACE("getSkinFilePath: %s", fontPath.c_str());

	if (fileExists(fontPath)) {
		TRACE("font file exists");
		if (font != NULL) {
			TRACE("delete font");
			delete font;
		}
		if (fontTitle != NULL) {
			TRACE("delete title font");
			delete fontTitle;
		}
		if (fontSectionTitle != NULL) {
			TRACE("delete title font");
			delete fontSectionTitle;
		}
		TRACE("getFont");
		font = new FontHelper(fontPath, skin->fontSize, skin->colours.font, skin->colours.fontOutline);
		TRACE("getTileFont");
		fontTitle = new FontHelper(fontPath, skin->fontSizeTitle, skin->colours.font, skin->colours.fontOutline);
		TRACE("exit");
		fontSectionTitle = new FontHelper(fontPath, skin->fontSizeSectionTitle, skin->colours.font, skin->colours.fontOutline);
	} else {
		ERROR("GMenu2X::initFont - font file is missing from : %s", fontPath.c_str());
	}
	TRACE("exit");
}

void GMenu2X::initMenu() {
	TRACE("enter");
	if (menu != NULL) delete menu;

	TRACE("initLayout");
	initLayout();

	//Menu structure handler
	TRACE("new menu");
	menu = new Menu(this);

	TRACE("sections loop : %zu", menu->getSections().size());
	for (uint32_t i = 0; i < menu->getSections().size(); i++) {
		//Add virtual links in the applications section
		if (menu->getSections()[i] == "applications") {
			menu->addActionLink(i, 
						tr["Explorer"], 
						MakeDelegate(this, &GMenu2X::explorer), 
						tr["Browse files and launch apps"], 
						"skin:icons/explorer.png");
			
			menu->addActionLink(i, 
						tr["Battery Logger"], 
						MakeDelegate(this, &GMenu2X::batteryLogger), 
						tr["Log battery power to battery.csv"], 
						"skin:icons/ebook.png");
		}

		//Add virtual links in the setting section
		else if (menu->getSections()[i] == "settings") {
			menu->addActionLink(
				i, 
				tr["Settings"], 
				MakeDelegate(this, &GMenu2X::settings), 
				tr["Configure system and choose skin"], 
				"skin:icons/configure.png");

			menu->addActionLink(
				i, 
				tr["Skin - " + skin->name], 
				MakeDelegate(this, &GMenu2X::skinMenu), 
				tr["Adjust skin settings"], 
				"skin:icons/skin.png");

			if (curMMCStatus == MMC_MOUNTED)
				menu->addActionLink(
					i, 
					tr["Umount"], 
					MakeDelegate(this, &GMenu2X::umountSdDialog), 
					tr["Umount external SD"], 
					"skin:icons/eject.png");

			if (curMMCStatus == MMC_UNMOUNTED)
				menu->addActionLink(
					i, 
					tr["Mount"], 
					MakeDelegate(this, &GMenu2X::mountSdDialog), 
					tr["Mount external SD"], 
					"skin:icons/eject.png");

			if (fileExists(getAssetsPath() + "log.txt"))
				menu->addActionLink(
					i, 
					tr["Log Viewer"], 
					MakeDelegate(this, &GMenu2X::viewLog), 
					tr["Displays last launched program's output"], 
					"skin:icons/ebook.png");

			menu->addActionLink(i, tr["About"], MakeDelegate(this, &GMenu2X::about), tr["Info about system"], "skin:icons/about.png");
			menu->addActionLink(i, tr["Power"], MakeDelegate(this, &GMenu2X::poweroffDialog), tr["Power menu"], "skin:icons/exit.png");
		}
	}
	if (config->saveSelection()) {
		TRACE("menu->setSectionIndex : %i", config->section());
		menu->setSectionIndex(config->section());
		TRACE("menu->setLinkIndex : %i", config->link());
		menu->setLinkIndex(config->link());
	} else {
		menu->setSectionIndex(0);
		menu->setLinkIndex(0);
	}
	TRACE("menu->loadIcons");
	menu->loadIcons();
	TRACE("exit");
}

void GMenu2X::settings() {
	TRACE("enter");
	int curGlobalVolume = config->globalVolume();
	int curGlobalBrightness = config->backlightLevel();
	bool unhideSections = false;
	string prevSkin = config->skin();
	vector<string> skinList = Skin::getSkins(getAssetsPath());

	TRACE("getting translations");
	FileLister fl_tr(getAssetsPath() + "translations");
	fl_tr.browse();
	fl_tr.insertFile("English");
	// local vals
	string lang = tr.lang();
	string skin = config->skin();
	string batteryType = config->batteryType();
	string performanceMode = config->performance();
	string appsPath = config->externalAppPath();

	int saveSelection = config->saveSelection();
	int outputLogs = config->outputLogs();
	int backlightTimeout = config->backlightTimeout();
	int powerTimeout = config->powerTimeout();
	int backlightLevel = config->backlightLevel();
	int globalVolume = config->globalVolume();

	TRACE("found %i translations", fl_tr.fileCount());
	if (lang.empty()) {
		lang = "English";
	}
	TRACE("current language : %s", lang.c_str());

	vector<string> encodings;
	encodings.push_back("NTSC");
	encodings.push_back("PAL");

	vector<string> batteryTypes;
	batteryTypes.push_back("BL-5B");
	batteryTypes.push_back("Linear");

	vector<string> opFactory;
	opFactory.push_back(">>");
	string tmp = ">>";

	vector<string> performanceModes;
	performanceModes.push_back("On demand");
	performanceModes.push_back("Performance");

	RTC *rtc = new RTC();
	string currentDatetime = rtc->getDateTime();
	string prevDateTime = currentDatetime;

	SettingsDialog sd(this, ts, tr["Settings"], "skin:icons/configure.png");
	sd.addSetting(new MenuSettingMultiString(this, tr["Language"], tr["Set the language used by GMenuNX"], &lang, &fl_tr.getFiles()));
	sd.addSetting(new MenuSettingDateTime(this, tr["Date & Time"], tr["Set system's date & time"], &currentDatetime));
	sd.addSetting(new MenuSettingMultiString(this, tr["Battery profile"], tr["Set the battery discharge profile"], &batteryType, &batteryTypes));

	sd.addSetting(new MenuSettingMultiString(this, tr["Skin"], tr["Set the skin used by GMenuNX"], &skin, &skinList));

	sd.addSetting(new MenuSettingMultiString(this, tr["Performance mode"], tr["Set the performance mode"], &performanceMode, &performanceModes));

	sd.addSetting(new MenuSettingBool(this, tr["Save last selection"], tr["Save the last selected link and section on exit"], &saveSelection));
	if (!config->sectionFilter().empty()) {
		sd.addSetting(new MenuSettingBool(this, tr["Unhide all sections"], tr["Remove the hide sections filter"], &unhideSections));
	}
	
	sd.addSetting(new MenuSettingDir(
		this, 
		tr["External apps path"], 
		tr["Path to your apps on the external sd card"], 
		&appsPath, 
		config->externalAppPath(), 
		"Apps path", 
		"skin:icons/explorer.png"));

	sd.addSetting(new MenuSettingBool(this, tr["Output logs"], tr["Logs the link's output to read with Log Viewer"], &outputLogs));
	sd.addSetting(new MenuSettingInt(this,tr["Screen timeout"], tr["Set screen's backlight timeout in seconds"], &backlightTimeout, 60, 0, 120));
	
	sd.addSetting(new MenuSettingInt(this,tr["Power timeout"], tr["Minutes to poweroff system if inactive"], &powerTimeout, 10, 1, 300));
	sd.addSetting(new MenuSettingInt(this,tr["Backlight"], tr["Set LCD backlight"], &backlightLevel, 70, 1, 100));
	sd.addSetting(new MenuSettingInt(this, tr["Audio volume"], tr["Set the default audio volume"], &globalVolume, 60, 0, 100));

	#if defined(TARGET_RS97)
	sd.addSetting(new MenuSettingMultiString(this, tr["TV-out"], tr["TV-out signal encoding"], &config->tvOutMode, &encodings));
	sd.addSetting(new MenuSettingMultiString(this, tr["CPU settings"], tr["Define CPU and overclock settings"], &tmp, &opFactory, 0, MakeDelegate(this, &GMenu2X::cpuSettings)));
	#endif
	sd.addSetting(new MenuSettingMultiString(this, tr["Reset settings"], tr["Choose settings to reset back to defaults"], &tmp, &opFactory, 0, MakeDelegate(this, &GMenu2X::resetSettings)));

	if (sd.exec() && sd.edited() && sd.save) {
		bool refreshNeeded = false;

		if (lang == "English") lang = "";
		if (lang != config->lang()) {
			TRACE("updating language : %s", lang.c_str());
			config->lang(lang);
			tr.setLang(lang);
			refreshNeeded = true;
		}
		if (appsPath != config->externalAppPath()) {
			config->externalAppPath(appsPath);
			refreshNeeded = true;
		}
		config->skin(skin);
		config->batteryType(batteryType);
		config->performance(performanceMode);
		config->saveSelection(saveSelection);
		config->outputLogs(outputLogs);
		config->backlightTimeout(backlightTimeout);
		config->powerTimeout(powerTimeout);
		config->backlightLevel(backlightLevel);
		config->globalVolume(globalVolume);

		TRACE("updating the settings");
		if (curGlobalVolume != config->globalVolume()) {
			curGlobalVolume = setVolume(config->globalVolume());
		}
		if (curGlobalBrightness != config->backlightLevel()) {
			curGlobalBrightness = setBacklight(config->backlightLevel());
		}

		bool restartNeeded = prevSkin != config->skin();

		if (unhideSections) {
			config->sectionFilter("");
			config->section(0);
			config->link(0);
			this->menu->setSectionIndex(0);
			this->menu->setLinkIndex(0);
			refreshNeeded = true;
		}
		if (refreshNeeded && !restartNeeded) {
			TRACE("calling init menu");
			initMenu();
		}

		TRACE("setScreenTimeout - %i", config->backlightTimeout());
		screenManager.resetScreenTimer();
		screenManager.setScreenTimeout(config->backlightTimeout());

		this->writeConfig();

		if (prevDateTime != currentDatetime) {
			TRACE("updating datetime");
			MessageBox mb(this, tr["Updating system time"]);
			mb.setAutoHide(500);
			mb.exec();
			rtc->setTime(currentDatetime);
		}
		if (restartNeeded) {
			TRACE("restarting because skins changed");
			restartDialog();
		}
	}
	delete rtc;
	TRACE("exit");
}

void GMenu2X::resetSettings() {
	bool	reset_gmenu = true,
			reset_skin = true,
			reset_icon = false,
			reset_manual = false,
			reset_parameter = false,
			reset_backdrop = false,
			reset_filter = false,
			reset_directory = false,
			reset_preview = false,
			reset_cpu = false;

	string tmppath = "";

	SettingsDialog sd(this, ts, tr["Reset settings"], "skin:icons/configure.png");
	sd.addSetting(new MenuSettingBool(this, tr["GMenuNX"], tr["Reset GMenuNX settings"], &reset_gmenu));
	sd.addSetting(new MenuSettingBool(this, tr["Default skin"], tr["Reset Default skin settings back to default"], &reset_skin));
	sd.addSetting(new MenuSettingBool(this, tr["Icons"], tr["Reset link's icon back to default"], &reset_icon));
	sd.addSetting(new MenuSettingBool(this, tr["Manuals"], tr["Unset link's manual"], &reset_manual));
	sd.addSetting(new MenuSettingBool(this, tr["Parameters"], tr["Unset link's additional parameters"], &reset_parameter));
	sd.addSetting(new MenuSettingBool(this, tr["Backdrops"], tr["Unset link's backdrops"], &reset_backdrop));
	sd.addSetting(new MenuSettingBool(this, tr["Filters"], tr["Unset link's selector file filters"], &reset_filter));
	sd.addSetting(new MenuSettingBool(this, tr["Directories"], tr["Unset link's selector directory"], &reset_directory));
	sd.addSetting(new MenuSettingBool(this, tr["Previews"], tr["Unset link's selector previews path"], &reset_preview));
	sd.addSetting(new MenuSettingBool(this, tr["CPU speed"], tr["Reset link's custom CPU speed back to default"], &reset_cpu));

	if (sd.exec() && sd.edited() && sd.save) {
		MessageBox mb(this, tr["Changes will be applied to ALL\napps and GMenuNX. Are you sure?"], "skin:icons/exit.png");
		mb.setButton(CONFIRM, tr["Cancel"]);
		mb.setButton(SECTION_NEXT,  tr["Confirm"]);
		if (mb.exec() != SECTION_NEXT) return;

		for (uint32_t s = 0; s < menu->getSections().size(); s++) {
			for (uint32_t l = 0; l < menu->sectionLinks(s)->size(); l++) {
				menu->setSectionIndex(s);
				menu->setLinkIndex(l);
				bool islink = menu->selLinkApp() != NULL;
				// WARNING("APP: %d %d %d %s", s, l, islink, menu->sectionLinks(s)->at(l)->getTitle().c_str());
				if (!islink) continue;
				if (reset_cpu)			menu->selLinkApp()->setCPU();
				if (reset_icon)			menu->selLinkApp()->setIcon("");
				if (reset_manual)		menu->selLinkApp()->setManual("");
				if (reset_parameter) 	menu->selLinkApp()->setParams("");
				if (reset_filter) 		menu->selLinkApp()->setSelectorFilter("");
				if (reset_directory) 	menu->selLinkApp()->setSelectorDir("");
				if (reset_preview) 		menu->selLinkApp()->setSelectorScreens("");
				if (reset_backdrop) 	menu->selLinkApp()->setBackdrop("");
				if (reset_icon || reset_manual || reset_parameter || reset_backdrop || reset_filter || reset_directory || reset_preview )
					menu->selLinkApp()->save();
			}
		}
		if (reset_skin) {
			tmppath = getAssetsPath() + "skins/Default/skin.conf";
			unlink(tmppath.c_str());
		}
		if (reset_gmenu) {
			tmppath = getAssetsPath() + "gmenunx.conf";
			unlink(tmppath.c_str());
		}
		restartDialog();
	}
}

void GMenu2X::cpuSettings() {
	TRACE("enter");
	SettingsDialog sd(this, ts, tr["CPU settings"], "skin:icons/configure.png");
	int cpuMenu = config->cpuMenu();
	int cpuMax = config->cpuMax();
	int cpuMin = config->cpuMin();
	sd.addSetting(new MenuSettingInt(this, tr["Default CPU clock"], tr["Set the default working CPU frequency"], &cpuMenu, 528, 528, 600, 6));
	sd.addSetting(new MenuSettingInt(this, tr["Maximum CPU clock "], tr["Maximum overclock for launching links"], &cpuMax, 624, 600, 1200, 6));
	sd.addSetting(new MenuSettingInt(this, tr["Minimum CPU clock "], tr["Minimum underclock used in Suspend mode"], &cpuMin, 342, 200, 528, 6));
	if (sd.exec() && sd.edited() && sd.save) {
		config->cpuMenu(cpuMenu);
		config->cpuMax(cpuMax);
		config->cpuMin(cpuMin);
		setCPU(config->cpuMenu());
		writeConfig();
	}
	TRACE("exit");
}

void GMenu2X::readTmp() {
	TRACE("enter");
	lastSelectorElement = -1;
	if (!fileExists("/tmp/gmenunx.tmp")) return;
	ifstream inf("/tmp/gmenunx.tmp", ios_base::in);
	if (!inf.is_open()) return;
	string line, name, value;

	while (getline(inf, line, '\n')) {
		string::size_type pos = line.find("=");
		name = trim(line.substr(0,pos));
		value = trim(line.substr(pos+1,line.length()));
		if (name == "section") menu->setSectionIndex(atoi(value.c_str()));
		else if (name == "link") menu->setLinkIndex(atoi(value.c_str()));
		else if (name == "selectorelem") lastSelectorElement = atoi(value.c_str());
		else if (name == "selectordir") lastSelectorDir = value;
		else if (name == "TVOut") TVOut = value;
		else if (name == "tvOutPrev") tvOutPrev = atoi(value.c_str());
	}
	if (TVOut != "NTSC" && TVOut != "PAL") TVOut = "OFF";
	//	udcConnectedOnBoot = 0;
	inf.close();
	unlink("/tmp/gmenunx.tmp");
}

void GMenu2X::writeTmp(int selelem, const string &selectordir) {
	string conffile = "/tmp/gmenunx.tmp";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		inf << "section=" << menu->selSectionIndex() << endl;
		inf << "link=" << menu->selLinkIndex() << endl;
		if (selelem >- 1) inf << "selectorelem=" << selelem << endl;
		if (selectordir != "") inf << "selectordir=" << selectordir << endl;
		inf << "tvOutPrev=" << tvOutPrev << endl;
		inf << "TVOut=" << TVOut << endl;
		inf.close();
	}
}

void GMenu2X::readConfig() {
	TRACE("enter");
	config->loadConfig();
	if (!config->lang().empty()) {
		tr.setLang(config->lang());
	}
	if (!dirExists(getAssetsPath() + "skins/" + config->skin())) {
		config->skin("Default");
	}
	TRACE("exit");
}

void GMenu2X::writeConfig() {
	TRACE("enter");
	ledOn();
	// don't try and save to RO file system
	if (getExePath() != getAssetsPath()) {
		if (config->saveSelection() && menu != NULL) {
			config->section(menu->selSectionIndex());
			config->link(menu->selLinkIndex());
		}
		config->save();
	}
	TRACE("ledOff");
	ledOff();
	TRACE("exit");
}

void GMenu2X::writeSkinConfig() {
	TRACE("enter");
	ledOn();
	skin->save();
	ledOff();
	TRACE("exit");
}

uint32_t GMenu2X::onChangeSkin() {
	return 1;
}

void GMenu2X::skinMenu() {
	TRACE("enter");
	bool save = false;
	int selected = 0;
	int prevSkinBackdrops = skin->skinBackdrops;

	vector<string> wpLabel;
	wpLabel.push_back(">>");
	string tmp = ">>";

	vector<string> sbStr;
	sbStr.push_back("OFF");
	sbStr.push_back("Left");
	sbStr.push_back("Bottom");
	sbStr.push_back("Right");
	sbStr.push_back("Top");
	string sectionBar = sbStr[skin->sectionBar];
	
	vector<string> linkDisplayModesList;
	linkDisplayModesList.push_back("Icon & text");
	linkDisplayModesList.push_back("Icon");
	linkDisplayModesList.push_back("Text");
    string linkDisplayModeCurrent = linkDisplayModesList[skin->linkDisplayMode];

	vector<string> wallpapers = skin->getWallpapers();
	std::vector<string>::iterator it;
	it = wallpapers.begin();
	wallpapers.insert(it, "None");

	bool restartRequired = false;
	bool currentIconGray = skin->iconsToGrayscale;
	bool currentImageGray = skin->imagesToGrayscale;

	do {
		string wpPrev = base_name(skin->wallpaper);
		string wpCurrent = wpPrev;

		SettingsDialog sd(this, ts, tr["Skin"], "skin:icons/skin.png");
		sd.selected = selected;
		sd.allowCancel = false;

		sd.addSetting(new MenuSettingMultiString(this, tr["Wallpaper"], tr["Select an image to use as a wallpaper"], &wpCurrent, &wallpapers, MakeDelegate(this, &GMenu2X::onChangeSkin), MakeDelegate(this, &GMenu2X::changeWallpaper)));
		sd.addSetting(new MenuSettingMultiString(this, tr["Skin colors"], tr["Customize skin colors"], &tmp, &wpLabel, MakeDelegate(this, &GMenu2X::onChangeSkin), MakeDelegate(this, &GMenu2X::skinColors)));
		sd.addSetting(new MenuSettingBool(this, tr["Skin backdrops"], tr["Automatic load backdrops from skin pack"], &skin->skinBackdrops));
		
		sd.addSetting(new MenuSettingInt(this, tr["Title font size"], tr["Size of title's text font"], &skin->fontSizeTitle, 20, 6, 60));
		sd.addSetting(new MenuSettingInt(this, tr["Font size"], tr["Size of text font"], &skin->fontSize, 12, 6, 60));
		sd.addSetting(new MenuSettingInt(this, tr["Section font size"], tr["Size of section bar font"], &skin->fontSizeSectionTitle, 30, 6, 60));

		sd.addSetting(new MenuSettingInt(this, tr["Section title bar size"], tr["Size of section title bar"], &skin->sectionTitleBarSize, 40, 18, config->resolutionX()));
		sd.addSetting(new MenuSettingInt(this, tr["Section info bar size"], tr["Size of section info bar"], &skin->sectionInfoBarSize, 16, 1, config->resolutionX()));
		sd.addSetting(new MenuSettingBool(this, tr["Section info bar visible"], tr["Show the section info bar in the launcher view"], &skin->sectionInfoBarVisible));
		sd.addSetting(new MenuSettingMultiString(this, tr["Section bar position"], tr["Set the position of the Section Bar"], &sectionBar, &sbStr));
		
		sd.addSetting(new MenuSettingInt(this, tr["Top bar height"], tr["Height of top bar in sub menus"], &skin->menuTitleBarHeight, 40, 18, config->resolutionY()));
		sd.addSetting(new MenuSettingInt(this, tr["Bottom bar height"], tr["Height of bottom bar in sub menus"], &skin->menuInfoBarHeight, 16, 1, config->resolutionY()));

		sd.addSetting(new MenuSettingBool(this, tr["Show section icons"], tr["Toggles Section Bar icons on/off in horizontal"], &skin->showSectionIcons));
		sd.addSetting(new MenuSettingBool(this, tr["Show clock"], tr["Toggles the clock on/off"], &skin->showClock));
		sd.addSetting(new MenuSettingMultiString(this, tr["Link display mode"], tr["Toggles link icons and text on/off"], &linkDisplayModeCurrent, &linkDisplayModesList));

		sd.addSetting(new MenuSettingInt(this, tr["Menu columns"], tr["Number of columns of links in main menu"], &skin->numLinkCols, 1, 1, 8));
		sd.addSetting(new MenuSettingInt(this, tr["Menu rows"], tr["Number of rows of links in main menu"], &skin->numLinkRows, 6, 1, 16));
		
		sd.addSetting(new MenuSettingBool(this, tr["Monochrome icons"], tr["Force all icons to monochrome"], &skin->iconsToGrayscale));
		sd.addSetting(new MenuSettingBool(this, tr["Monochrome images"], tr["Force all images to monochrome"], &skin->imagesToGrayscale));
		
		sd.exec();

		// if wallpaper has changed, get full path and add it to the sc
		// unless we have chosen 'None'
		if (wpCurrent != wpPrev) {
			if (wpCurrent != "None") {
				if (this->sc->addImage(getAssetsPath() + "skins/" + skin->name + "/wallpapers/" + wpCurrent) != NULL)
					skin->wallpaper = getAssetsPath() + "skins/" + skin->name + "/wallpapers/" + wpCurrent;
			} else skin->wallpaper = "";
			setWallpaper(skin->wallpaper);
		}

		selected = sd.selected;
		save = sd.save;

	} while (!save);

	if (sectionBar == "OFF") skin->sectionBar = Skin::SB_OFF;
	else if (sectionBar == "Right") skin->sectionBar = Skin::SB_RIGHT;
	else if (sectionBar == "Top") skin->sectionBar = Skin::SB_TOP;
	else if (sectionBar == "Bottom") skin->sectionBar = Skin::SB_BOTTOM;
	else skin->sectionBar = Skin::SB_LEFT;

	if (linkDisplayModeCurrent == "Icon & text") {
		skin->linkDisplayMode = Skin::ICON_AND_TEXT;
	} else if (linkDisplayModeCurrent == "Icon") {
		skin->linkDisplayMode = Skin::ICON;
	} else skin->linkDisplayMode = Skin::TEXT;

	TRACE("writing config out");
	writeSkinConfig();

	TRACE("checking exit mode");
	if (currentIconGray != skin->iconsToGrayscale) {
		restartRequired = true;
	} else if (currentImageGray != skin->imagesToGrayscale) {
		restartRequired = true;
	} else if (prevSkinBackdrops != skin->skinBackdrops) {
		restartRequired = true;
	}
	if (restartRequired) {
		TRACE("restarting because backdrops or gray scale changed");
		restartDialog(true);
	} else {
		initMenu();
	}
	TRACE("exit");
}

void GMenu2X::skinColors() {
	bool save = false;
	do {
		SettingsDialog sd(this, ts, tr["Skin Colors"], "skin:icons/skin.png");
		sd.allowCancel = false;
		sd.addSetting(new MenuSettingRGBA(this, tr["Background"], tr["Background colour if no wallpaper"], &skin->colours.background));
		sd.addSetting(new MenuSettingRGBA(this, tr["Top/Section Bar"], tr["Color of the top and section bar"], &skin->colours.titleBarBackground));
		sd.addSetting(new MenuSettingRGBA(this, tr["List Body"], tr["Color of the list body"], &skin->colours.listBackground));
		sd.addSetting(new MenuSettingRGBA(this, tr["Bottom Bar"], tr["Color of the bottom bar"], &skin->colours.infoBarBackground));
		sd.addSetting(new MenuSettingRGBA(this, tr["Selection"], tr["Color of the selection and other interface details"], &skin->colours.selectionBackground));
		sd.addSetting(new MenuSettingRGBA(this, tr["Message Box"], tr["Background color of the message box"], &skin->colours.msgBoxBackground));
		sd.addSetting(new MenuSettingRGBA(this, tr["Msg Box Border"], tr["Border color of the message box"], &skin->colours.msgBoxBorder));
		sd.addSetting(new MenuSettingRGBA(this, tr["Msg Box Selection"], tr["Color of the selection of the message box"], &skin->colours.msgBoxSelection));
		sd.addSetting(new MenuSettingRGBA(this, tr["Font"], tr["Color of the font"], &skin->colours.font));
		sd.addSetting(new MenuSettingRGBA(this, tr["Font Outline"], tr["Color of the font's outline"], &skin->colours.fontOutline));
		sd.addSetting(new MenuSettingRGBA(this, tr["Alt Font"], tr["Color of the alternative font"], &skin->colours.fontAlt));
		sd.addSetting(new MenuSettingRGBA(this, tr["Alt Font Outline"], tr["Color of the alternative font outline"], &skin->colours.fontAltOutline));
		sd.exec();
		save = sd.save;
	} while (!save);
	writeSkinConfig();
}

void GMenu2X::about() {
	TRACE("enter");
	vector<string> text;
	string temp;

	char *hms = ms2hms(SDL_GetTicks());
	int battLevel = getBatteryLevel();
	TRACE("batt level : %i", battLevel);
	int battPercent = (battLevel * 20);
	TRACE("batt percent : %i", battPercent);
	
	char buffer[50];
	int n = sprintf (buffer, "%i %%", battPercent);
	string batt(buffer);

	temp = tr["Build date: "] + __DATE__ + "\n";
	temp += tr["Uptime: "] + hms + "\n";
	temp += tr["Battery: "] + ((battLevel == 6) ? tr["Charging"] : batt) + "\n";	
	temp += tr["Storage free:"];
	temp += "\n    " + tr["Internal: "] + getDiskFree("/media/data");

	checkUDC();
	string externalSize;
 	if (curMMCStatus == MMC_MOUNTED) {
		externalSize = getDiskFree("/media/sdcard");
	} else if (curMMCStatus == MMC_UNMOUNTED) {
		externalSize = tr["Inserted, not mounted"];
	} else {
		externalSize = tr["Not inserted"];
	}
	temp += "\n    " + tr["External: "] + externalSize + "\n";
	temp += "----\n";

	TextDialog td(this, "GMenuNX", tr["Info about system"], "skin:icons/about.png");
	td.appendText(temp);
	
	#ifdef TARGET_RG350
	TRACE("append - command /usr/bin/system_info");
	td.appendCommand("/usr/bin/system_info");
	#else
	TRACE("append - command /usr/bin/uname -a");
	td.appendCommand("/usr/bin/uname", "-a");
	TRACE("append - command /usr/bin/lshw -short 2>/dev/null");
	td.appendCommand("/usr/bin/lshw", "-short 2>/dev/null");
	#endif
	td.appendText("----\n");
	
	TRACE("append - file %sabout.txt", getAssetsPath().c_str());
	td.appendFile(getAssetsPath() + "about.txt");
	td.exec();
	TRACE("exit");
}

void GMenu2X::viewLog() {
	string logfile = getAssetsPath() + "log.txt";
	if (!fileExists(logfile)) return;

	TextDialog td(this, tr["Log Viewer"], tr["Last launched program's output"], "skin:icons/ebook.png");
	td.appendFile(getAssetsPath() + "log.txt");
	td.exec();

	MessageBox mb(this, tr["Do you want to delete the log file?"], "skin:icons/ebook.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		ledOn();
		unlink(logfile.c_str());
		sync();
		menu->deleteSelectedLink();
		ledOff();
	}
}

void GMenu2X::batteryLogger() {
	ledOn();
	BatteryLoggerDialog bl(this, tr["Battery Logger"], tr["Log battery power to battery.csv"], "skin:icons/ebook.png");
	bl.exec();
	ledOff();
}

void GMenu2X::linkScanner() {
	LinkScannerDialog ls(this, tr["Link scanner"], tr["Scan for applications and games"], "skin:icons/configure.png");
	ls.exec();
}

void GMenu2X::changeWallpaper() {
	TRACE("enter");
	WallpaperDialog wp(
		this, 
		tr["Wallpaper"], 
		tr["Select an image to use as a wallpaper"], 
		"skin:icons/wallpaper.png");

	if (wp.exec() && skin->wallpaper != wp.wallpaper) {
		TRACE("new wallpaper : %s", wp.wallpaper.c_str());
		setWallpaper(wp.wallpaper);
		TRACE("write config");
		writeSkinConfig();
	}
	TRACE("exit");
}

void GMenu2X::showManual() {
	string linkTitle = menu->selLinkApp()->getTitle();
	string linkDescription = menu->selLinkApp()->getDescription();
	string linkIcon = menu->selLinkApp()->getIcon();
	string linkManual = menu->selLinkApp()->getManualPath();
	string linkBackdrop = menu->selLinkApp()->getBackdropPath();

	if (linkManual.empty() || !fileExists(linkManual)) return;

	string ext = linkManual.substr(linkManual.size() - 4, 4);
	if (ext == ".png" || ext == ".bmp" || ext == ".jpg" || ext == "jpeg") {
		ImageViewerDialog im(this, linkTitle, linkDescription, linkIcon, linkManual);
		im.exec();
		return;
	}

	TextDialog td(this, linkTitle, linkDescription, linkIcon, linkBackdrop);
	td.appendFile(linkManual);
	td.exec();
}

void GMenu2X::explorer() {
	TRACE("enter");
	BrowseDialog fd(this, tr["Explorer"], tr["Select a file or application"]);
	fd.showDirectories = true;
	fd.showFiles = true;

	bool loop = true;
	while (fd.exec() && loop) {
		string ext = fd.getExt();
		if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif") {
			ImageViewerDialog im(this, tr["Image viewer"], fd.getFile(), "icons/explorer.png", fd.getPath() + "/" + fd.getFile());
			im.exec();
			continue;
		} else if (ext == ".txt" || ext == ".conf" || ext == ".me" || ext == ".md" || ext == ".xml" || ext == ".log") {
			TextDialog td(this, tr["Text viewer"], fd.getFile(), "skin:icons/ebook.png");
			td.appendFile(fd.getPath() + "/" + fd.getFile());
			td.exec();
		} else {
			if (config->saveSelection() && (config->section() != menu->selSectionIndex() || config->link() != menu->selLinkIndex()))
				writeConfig();

			loop = false;
			string command = cmdclean(fd.getPath() + "/" + fd.getFile());
			chdir(fd.getPath().c_str());
			quit();
			setCPU(config->cpuMenu());
			execlp("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);

			//if execution continues then something went wrong and as we already called SDL_Quit we cannot continue
			//try relaunching gmenunx
			WARNING("Error executing selected application, re-launching gmenunx");
			chdir(getExePath().c_str());
			execlp("./gmenunx", "./gmenunx", NULL);
		}
	}
	TRACE("exit");
}

void GMenu2X::ledOn() {
	TRACE("ledON");
	led->flash();
}

void GMenu2X::ledOff() {
	TRACE("ledOff");
	led->reset();
}

void GMenu2X::restartDialog(bool showDialog) {
	if (showDialog) {
		MessageBox mb(this, tr["GMenuNX will restart to apply\nthe settings. Continue?"], "skin:icons/exit.png");
		mb.setButton(CONFIRM, tr["Restart"]);
		mb.setButton(CANCEL,  tr["Cancel"]);
		if (mb.exec() == CANCEL) return;
	}

	quit();
	WARNING("Re-launching gmenunx");
	chdir(getExePath().c_str());
	execlp("./gmenunx", "./gmenunx", NULL);
}

void GMenu2X::poweroffDialog() {
	MessageBox mb(this, tr["Poweroff or reboot the device?"], "skin:icons/exit.png");
	mb.setButton(SECTION_NEXT, tr["Reboot"]);
	mb.setButton(CONFIRM, tr["Poweroff"]);
	mb.setButton(CANCEL,  tr["Cancel"]);
	int response = mb.exec();
	if (response == CONFIRM) {
		MessageBox mb(this, tr["Poweroff"]);
		mb.setAutoHide(500);
		mb.exec();
		setBacklight(0);
		system("sync; poweroff");
	}
	else if (response == SECTION_NEXT) {
		MessageBox mb(this, tr["Rebooting"]);
		mb.setAutoHide(500);
		mb.exec();
		setBacklight(0);
		system("sync; reboot");
	}
}

void GMenu2X::setTVOut(string TVOut) {
	#if defined(TARGET_RS97)
	system("echo 0 > /proc/jz/tvselect"); // always reset tv out
	if (TVOut == "NTSC")		system("echo 2 > /proc/jz/tvselect");
	else if (TVOut == "PAL")	system("echo 1 > /proc/jz/tvselect");
	#endif
}

string GMenu2X::mountSd() {
	TRACE("enter");
	string result = exec("mount -t auto /dev/mmcblk1p1 /media/sdcard 2>&1");
	TRACE("result : %s", result.c_str());
	system("sleep 1");
	checkUDC();
	return result;
}

void GMenu2X::mountSdDialog() {
	MessageBox mb(this, tr["Mount SD card?"], "skin:icons/eject.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);

	if (mb.exec() == CONFIRM) {
		int currentMenuIndex = menu->selSectionIndex();
		int currentLinkIndex = menu->selLinkIndex();
		string result = mountSd();
		initMenu();
		menu->setSectionIndex(currentMenuIndex);
		menu->setLinkIndex(currentLinkIndex);

		string msg, icon;
		int wait = 1000;
		bool error = false;
		switch(curMMCStatus) {
			case MMC_MOUNTED:
				msg = "SD card mounted";
				icon = "skin:icons/eject.png";
				break;
			default:
				msg = "Failed to mount sd card\n" + result;
				icon = "skin:icons/error.png";
				error = true;
				break;
		};
		MessageBox mb(this, tr[msg], icon);
		if (error) {
			mb.setButton(CONFIRM, tr["Close"]);
		} else {
			mb.setAutoHide(wait);
		}
		mb.exec();
	}
}

string GMenu2X::umountSd() {
	system("sync");
	string result = exec("umount -fl /media/sdcard 2>&1");
	system("sleep 1");
	checkUDC();
	return result;
}

void GMenu2X::umountSdDialog() {
	MessageBox mb(this, tr["Umount SD card?"], "skin:icons/eject.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		int currentMenuIndex = menu->selSectionIndex();
		int currentLinkIndex = menu->selLinkIndex();
		string result = umountSd();
		initMenu();
		menu->setSectionIndex(currentMenuIndex);
		menu->setLinkIndex(currentLinkIndex);

		string msg, icon;
		int wait = 1000;
		switch(curMMCStatus) {
			case MMC_UNMOUNTED:
				msg = "SD card umounted";
				icon = "skin:icons/eject.png";
				break;
			default:
				msg = "Failed to unmount sd card\n" + result;
				icon = "skin:icons/error.png";
				wait = 3000;
				break;
		};
		MessageBox mb(this, tr[msg], icon);
		mb.setAutoHide(wait);
		mb.exec();
	}
}

void GMenu2X::checkUDC() {
	TRACE("enter");
	curMMCStatus = MMC_ERROR;
	unsigned long size;
	TRACE("reading /sys/block/mmcblk1/size");
	std::ifstream fsize("/sys/block/mmcblk1/size", ios::in | ios::binary);
	if (fsize >> size) {
		if (size > 0) {
			// ok, so we're inserted, are we mounted
			TRACE("size was : %lu, reading /proc/mounts", size);
			std::ifstream procmounts( "/proc/mounts" );
			if (!procmounts) {
				curMMCStatus = MMC_ERROR;
				WARNING("GMenu2X::checkUDC - couldn't open /proc/mounts");
			} else {
				string line;
				size_t found;
				curMMCStatus = MMC_UNMOUNTED;
				while (std::getline(procmounts, line)) {
					if ( !(procmounts.fail() || procmounts.bad()) ) {
						found = line.find("mcblk1");
						if (found != std::string::npos) {
							curMMCStatus = MMC_MOUNTED;
							TRACE("inserted && mounted because line : %s", line.c_str());
							break;
						}
					} else {
						curMMCStatus = MMC_ERROR;
						WARNING("GMenu2X::checkUDC - error reading /proc/mounts");
						break;
					}
				}
				procmounts.close();
			}
		} else {
			curMMCStatus = MMC_MISSING;
			TRACE("not inserted");
		}
	} else {
		curMMCStatus = MMC_ERROR;
		WARNING("GMenu2X::checkUDC - error, no card");
	}
	fsize.close();
	TRACE("exit - %i",  curMMCStatus);
}

/*
void GMenu2X::formatSd() {
	MessageBox mb(this, tr["Format internal SD card?"], "skin:icons/format.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		MessageBox mb(this, tr["Formatting internal SD card..."], "skin:icons/format.png");
		mb.setAutoHide(100);
		mb.exec();

		system("/usr/bin/format_int_sd.sh");
		{ // new mb scope
			MessageBox mb(this, tr["Complete!"]);
			mb.setAutoHide(0);
			mb.exec();
		}
	}
}
*/
void GMenu2X::setPerformanceMode() {
	TRACE("enter - %s", config->performance().c_str());
	string current = getPerformanceMode();
	string desired = "ondemand";
	if ("Performance" == config->performance()) {
		desired = "performance";
	}
	if (current.compare(desired) != 0) {
		TRACE("update needed : current %s vs. desired %s", current.c_str(), desired.c_str());
		if (fileExists("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")) {
			procWriter("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", desired);
			initMenu();
		}
	} else {
		TRACE("nothing to do");
	}
	TRACE("exit");	
}

string GMenu2X::getPerformanceMode() {
	TRACE("enter");
	string result = "ondemand";
	if (fileExists("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")) {
		result = fileReader("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
	}
	TRACE("exit - %s", result.c_str());
	return full_trim(result);
}

void GMenu2X::contextMenu() {
	
	TRACE("enter");
	vector<MenuOption> voices;
	if (menu->selLinkApp() != NULL) {
		if (menu->selLinkApp()->isEditable() ){
			voices.push_back(
				(MenuOption){
					tr.translate("Edit $1", menu->selLink()->getTitle().c_str(), NULL), 
					MakeDelegate(this, &GMenu2X::editLink)
				});
		}
		if (menu->selLinkApp()->isDeletable()) {
			voices.push_back(
				(MenuOption){
					tr.translate("Delete $1", menu->selLink()->getTitle().c_str(), NULL), 
					MakeDelegate(this, &GMenu2X::deleteLink)
				});
		}
	}
	voices.push_back((MenuOption){tr["Add link"], 		MakeDelegate(this, &GMenu2X::addLink)});
	voices.push_back((MenuOption){tr["Add section"],	MakeDelegate(this, &GMenu2X::addSection)});
	voices.push_back((MenuOption){tr["Rename section"],	MakeDelegate(this, &GMenu2X::renameSection)});
	voices.push_back((MenuOption){tr["Hide section"],	MakeDelegate(this, &GMenu2X::hideSection)});
	voices.push_back((MenuOption){tr["Delete section"],	MakeDelegate(this, &GMenu2X::deleteSection)});
	voices.push_back((MenuOption){tr["App scanner"],	MakeDelegate(this, &GMenu2X::linkScanner)});

	Surface bg(screen);
	bool close = false, inputAction = false;
	int sel = 0;
	uint32_t i, fadeAlpha = 0, h = font->getHeight(), h2 = font->getHalfHeight();

	SDL_Rect box;
	box.h = h * voices.size() + 8;
	box.w = 0;
	for (i = 0; i < voices.size(); i++) {
		int w = font->getTextWidth(voices[i].text);
		if (w > box.w) box.w = w;
	}
	box.w += 23;
	box.x = this->config->halfX() - box.w / 2;
	box.y = this->config->halfY() - box.h / 2;

	TRACE("box - x: %i, y: %i, w: %i, h: %i", box.x, box.y, box.w, box.h);
	TRACE("screen - x: %i, y: %i, halfx: %i, halfy: %i",  config->resolutionX(), config->resolutionY(), this->config->halfX(), this->config->halfY());
	
	uint32_t tickStart = SDL_GetTicks();
	input.setWakeUpInterval(1000);
	while (!close) {
		bg.blit(screen, 0, 0);

		screen->box(0, 0, config->resolutionX(), config->resolutionY(), 0,0,0, fadeAlpha);
		screen->box(box.x, box.y, box.w, box.h, skin->colours.msgBoxBackground);
		screen->rectangle( box.x + 2, box.y + 2, box.w - 4, box.h - 4, skin->colours.msgBoxBorder);

		//draw selection rect
		screen->box( box.x + 4, box.y + 4 + h * sel, box.w - 8, h, skin->colours.msgBoxSelection);
		for (i = 0; i < voices.size(); i++)
			screen->write( 
				font, 
				voices[i].text, 
				box.x + 12, 
				box.y + h2 + 3 + h * i, 
				VAlignMiddle, 
				skin->colours.fontAlt, 
				skin->colours.fontAltOutline);

		screen->flip();

		if (fadeAlpha < 200) {
			fadeAlpha = intTransition(0, 200, tickStart, 200);
			continue;
		}
		do {
			inputAction = input.update();
			if (inputAction) {
				if ( input[MENU] || input[CANCEL]) close = true;
				else if ( input[UP] ) sel = (sel - 1 < 0) ? (int)voices.size() - 1 : sel - 1 ;
				else if ( input[DOWN] ) sel = (sel + 1 > (int)voices.size() - 1) ? 0 : sel + 1;
				else if ( input[LEFT] || input[PAGEUP] ) sel = 0;
				else if ( input[RIGHT] || input[PAGEDOWN] ) sel = (int)voices.size() - 1;
				else if ( input[SETTINGS] || input[CONFIRM] ) { voices[sel].action(); close = true; }
			}
		} while (!inputAction);
	}
	TRACE("exit");
}

void GMenu2X::addLink() {
	BrowseDialog fd(this, tr["Add link"], tr["Select an application"]);
	fd.showDirectories = true;
	fd.showFiles = true;
	fd.setFilter(".dge,.gpu,.gpe,.sh,.bin,.elf,");
	if (fd.exec()) {
		ledOn();
		if (menu->addLink(fd.getPath(), fd.getFile())) {
			editLink();
		}
		sync();
		ledOff();
	}
}

void GMenu2X::editLink() {
	if (menu->selLinkApp() == NULL) return;

	vector<string> pathV;
	// ERROR("FILE: %s", menu->selLinkApp()->getFile().c_str());
	split(pathV, menu->selLinkApp()->getFile(), "/");
	string oldSection = "";
	if (pathV.size() > 1) oldSection = pathV[pathV.size()-2];
	string newSection = oldSection;

	string linkExec = menu->selLinkApp()->getExec();
	string linkTitle = menu->selLinkApp()->getTitle();
	string linkDescription = menu->selLinkApp()->getDescription();
	string linkIcon = menu->selLinkApp()->getIcon();
	string linkManual = menu->selLinkApp()->getManual();
	string linkParams = menu->selLinkApp()->getParams();
	string linkSelFilter = menu->selLinkApp()->getSelectorFilter();
	string linkSelDir = menu->selLinkApp()->getSelectorDir();
	bool linkSelBrowser = menu->selLinkApp()->getSelectorBrowser();
	//string linkSelScreens = menu->selLinkApp()->getSelectorScreens();
	string linkSelAliases = menu->selLinkApp()->getAliasFile();
	//int linkClock = menu->selLinkApp()->clock();
	string linkBackdrop = menu->selLinkApp()->getBackdrop();
	string dialogTitle = tr.translate("Edit $1", linkTitle.c_str(), NULL);
	string dialogIcon = menu->selLinkApp()->getIconPath();

	SettingsDialog sd(this, ts, dialogTitle, dialogIcon);
	sd.addSetting(new MenuSettingFile(			this, tr["Executable"],		tr["Application this link points to"], &linkExec, ".dge,.gpu,.gpe,.sh,.bin,.elf,", CARD_ROOT, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingString(		this, tr["Title"],			tr["Link title"], &linkTitle, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingString(		this, tr["Description"],	tr["Link description"], &linkDescription, dialogTitle, dialogIcon));
	
	if (oldSection != menu->selLinkApp()->getFavouriteFolder()) {
		sd.addSetting(new MenuSettingMultiString(	this, tr["Section"],		tr["The section this link belongs to"], &newSection, &menu->getSections()));
	}
	sd.addSetting(new MenuSettingImage(			this, tr["Icon"],			tr["Select a custom icon for the link"], &linkIcon, ".png,.bmp,.jpg,.jpeg,.gif", dir_name(linkIcon), dialogTitle, dialogIcon, skin->name));
	//sd.addSetting(new MenuSettingInt(			this, tr["CPU Clock"],		tr["CPU clock frequency when launching this link"], &linkClock, config->cpuMenu, config->cpuMin, config->cpuMax, 6));
	sd.addSetting(new MenuSettingString(		this, tr["Parameters"],		tr["Command line arguments to pass to the application"], &linkParams, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingDir(			this, tr["Selector Path"],	tr["Directory to start the selector"], &linkSelDir, CARD_ROOT, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingBool(			this, tr["Show Folders"],	tr["Allow the selector to change directory"], &linkSelBrowser));
	sd.addSetting(new MenuSettingString(		this, tr["File Filter"],	tr["Filter by file extension (separate with commas)"], &linkSelFilter, dialogTitle, dialogIcon));
	//sd.addSetting(new MenuSettingDir(			this, tr["Screenshots"],	tr["Directory of the screenshots for the selector"], &linkSelScreens, CARD_ROOT, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingFile(			this, tr["Aliases"],		tr["File containing a list of aliases for the selector"], &linkSelAliases, ".txt,.dat", CARD_ROOT, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingImage(			this, tr["Backdrop"],		tr["Select an image backdrop"], &linkBackdrop, ".png,.bmp,.jpg,.jpeg", CARD_ROOT, dialogTitle, dialogIcon, skin->name));
	sd.addSetting(new MenuSettingFile(			this, tr["Manual"],   		tr["Select a Manual or Readme file"], &linkManual, ".man.png,.txt,.me", dir_name(linkManual), dialogTitle, dialogIcon));

	if (sd.exec() && sd.edited() && sd.save) {
		ledOn();

		menu->selLinkApp()->setExec(linkExec);
		menu->selLinkApp()->setTitle(linkTitle);
		menu->selLinkApp()->setDescription(linkDescription);
		menu->selLinkApp()->setIcon(linkIcon);
		menu->selLinkApp()->setManual(linkManual);
		menu->selLinkApp()->setParams(linkParams);
		menu->selLinkApp()->setSelectorFilter(linkSelFilter);
		menu->selLinkApp()->setSelectorDir(linkSelDir);
		menu->selLinkApp()->setSelectorBrowser(linkSelBrowser);
		//menu->selLinkApp()->setSelectorScreens(linkSelScreens);
		menu->selLinkApp()->setAliasFile(linkSelAliases);
		menu->selLinkApp()->setBackdrop(linkBackdrop);
		//menu->selLinkApp()->setCPU(linkClock);

		//if section changed move file and update link->file
		if (oldSection != newSection) {
			vector<string>::const_iterator newSectionIndex = find(menu->getSections().begin(), menu->getSections().end(), newSection);
			if (newSectionIndex == menu->getSections().end()) return;
			string newFileName = "sections/" + newSection + "/" + linkTitle;
			uint32_t x = 2;
			while (fileExists(newFileName)) {
				string id = "";
				stringstream ss; ss << x; ss >> id;
				newFileName = "sections/" + newSection + "/" + linkTitle + id;
				x++;
			}
			rename(menu->selLinkApp()->getFile().c_str(), newFileName.c_str());
			menu->selLinkApp()->renameFile(newFileName);

			INFO("New section: %s", newSection.c_str());

			menu->linkChangeSection(menu->selLinkIndex(), menu->selSectionIndex(), newSectionIndex - menu->getSections().begin());
		}
		menu->selLinkApp()->save();
		sync();
		ledOff();
	}
	config->section(menu->selSectionIndex());
	config->link(menu->selLinkIndex());
	initMenu();
}

void GMenu2X::deleteLink() {
	if (menu->selLinkApp() != NULL) {
		MessageBox mb(this, tr.translate("Delete $1", menu->selLink()->getTitle().c_str(), NULL) + "\n" + tr["Are you sure?"], menu->selLink()->getIconPath());
		mb.setButton(CONFIRM, tr["Yes"]);
		mb.setButton(CANCEL,  tr["No"]);
		if (mb.exec() == CONFIRM) {
			ledOn();
			menu->deleteSelectedLink();
			sync();
			ledOff();
		}
	}
}

void GMenu2X::addSection() {
	InputDialog id(this, ts, tr["Insert a name for the new section"], "", tr["Add section"], "skin:icons/section.png");
	if (id.exec()) {
		//only if a section with the same name does not exist
		if (find(menu->getSections().begin(), menu->getSections().end(), id.getInput()) == menu->getSections().end()) {
			//section directory doesn't exists
			ledOn();
			if (menu->addSection(id.getInput())) {
				menu->setSectionIndex( menu->getSections().size() - 1 ); //switch to the new section
				sync();
			}
			ledOff();
		}
	}
}

void GMenu2X::hideSection() {
	string section = menu->selSection();
	if (this->config->sectionFilter().empty()) {
		this->config->sectionFilter(section);
	} else {
		this->config->sectionFilter(this->config->sectionFilter() + "," + section);
	}
	initMenu();
}

void GMenu2X::renameSection() {
	InputDialog id(this, ts, tr["Insert a new name for this section"], menu->selSection(), tr["Rename section"], menu->getSectionIcon(menu->selSectionIndex()));
	if (id.exec()) {
		//only if a section with the same name does not exist & !samename
		if (menu->selSection() != id.getInput() && find(menu->getSections().begin(),menu->getSections().end(), id.getInput()) == menu->getSections().end()) {
			//section directory doesn't exists
			string newsectiondir = "sections/" + id.getInput();
			string sectiondir = "sections/" + menu->selSection();
			ledOn();
			if (rename(sectiondir.c_str(), "tmpsection")==0 && rename("tmpsection", newsectiondir.c_str())==0) {
				string oldpng = sectiondir + ".png", newpng = newsectiondir+".png";
				string oldicon = this->skin->getSkinFilePath(oldpng), newicon = this->skin->getSkinFilePath(newpng);
				if (!oldicon.empty() && newicon.empty()) {
					newicon = oldicon;
					newicon.replace(newicon.find(oldpng), oldpng.length(), newpng);

					if (!fileExists(newicon)) {
						rename(oldicon.c_str(), "tmpsectionicon");
						rename("tmpsectionicon", newicon.c_str());
						this->sc->move("skin:" + oldpng, "skin:" + newpng);
					}
				}
				menu->renameSection(menu->selSectionIndex(), id.getInput());
				sync();
			}
			ledOff();
		}
	}
}

void GMenu2X::deleteSection() {
	MessageBox mb(this, tr["All links in this section will be removed."] + "\n" + tr["Are you sure?"]);
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		ledOn();
		if (rmtree(getAssetsPath() + "sections/" + menu->selSection())) {
			menu->deleteSelectedSection();
			sync();
		}
		ledOff();
	}
}

int GMenu2X::getBatteryLevel() {
	//TRACE("enter");

	int online, result = 0;
	#ifdef TARGET_RG350
	sscanf(fileReader("/sys/class/power_supply/usb/online").c_str(), "%i", &online);
	if (online) {
		result = 6;
	} else {
		int battery_level = 0;
		sscanf(fileReader("/sys/class/power_supply/battery/capacity").c_str(), "%i", &battery_level);
		TRACE("raw battery level - %i", battery_level);
		if (battery_level >= 100) result = 5;
		else if (battery_level > 80) result = 4;
		else if (battery_level > 60) result = 3;
		else if (battery_level > 40) result = 2;
		else if (battery_level > 20) result = 1;
		result = 0;
	}
	#else
	result = 6;
	#endif
	TRACE("scaled battery level : %i", result);
	return result;
}

void GMenu2X::setInputSpeed() {
	input.setInterval(180);
	input.setInterval(1000, SETTINGS);
	input.setInterval(1000, MENU);
	input.setInterval(1000, CONFIRM);
	input.setInterval(1500, POWER);
}

void GMenu2X::setCPU(uint32_t mhz) {
	// mhz = constrain(mhz, CPU_CLK_MIN, CPU_CLK_MAX);
	if (memdev > 0) {
		TRACE("Setting clock to %d", mhz);

	#if defined(TARGET_GP2X)
		uint32_t v, mdiv, pdiv=3, scale=0;

		#define SYS_CLK_FREQ 7372800
		mhz *= 1000000;
		mdiv = (mhz * pdiv) / SYS_CLK_FREQ;
		mdiv = ((mdiv-8)<<8) & 0xff00;
		pdiv = ((pdiv-2)<<2) & 0xfc;
		scale &= 3;
		v = mdiv | pdiv | scale;
		MEM_REG[0x910>>1] = v;

	#elif defined(TARGET_CAANOO) || defined(TARGET_WIZ)
		volatile uint32_t *memregl = static_cast<volatile uint32_t*>((volatile void*)memregs);
		int mdiv, pdiv = 9, sdiv = 0;
		uint32_t v;

		#define SYS_CLK_FREQ 27
		#define PLLSETREG0   (memregl[0xF004>>2])
		#define PWRMODE      (memregl[0xF07C>>2])
		mdiv = (mhz * pdiv) / SYS_CLK_FREQ;
		if (mdiv & ~0x3ff) return;
		v = pdiv<<18 | mdiv<<8 | sdiv;

		PLLSETREG0 = v;
		PWRMODE |= 0x8000;
		for (int i = 0; (PWRMODE & 0x8000) && i < 0x100000; i++);

	#elif defined(TARGET_RS97)
		uint32_t m = mhz / 6;
		memregs[0x10 >> 2] = (m << 24) | 0x090520;
		INFO("Set CPU clock: %d", mhz);
	#endif
		setTVOut(TVOut);
	}
}

int GMenu2X::getVolume() {
	TRACE("enter");
	int vol = -1;
	#ifdef TARGET_RG350
	string result = exec(RG350_GET_VOLUME_PATH.c_str());
	if (result.length() > 0) {
		vol = atoi(trim(result).c_str());
	}
	// scale 0 - 31, turn to percent
	vol = vol * 100 / 31;
	TRACE("exit : %i", vol);
	#else
	vol = 100;
	#endif
	return vol;
}

int GMenu2X::setVolume(int val) {
	TRACE("enter - %i", val);
	if (val < 0) val = 100;
	else if (val > 100) val = 0;
	if (val == getVolume()) 
		return val;
	#ifdef TARGET_RG350
	int rg350val = (int)(val * (31.0f/100));
	TRACE("rg350 value : %i", rg350val);
	stringstream ss;
	string cmd;
	ss << RG350_SET_VOLUME_PATH << rg350val;
	std::getline(ss, cmd);
	TRACE("cmd : %s", cmd.c_str());
	string result = exec(cmd.c_str());
	TRACE("result : %s", result.c_str());
	#endif
	return val;
}

int GMenu2X::getBacklight() {
	TRACE("enter");
	int level = 0;
	#ifdef TARGET_RG350
	//force  scale 0 - 5
	string result = fileReader(RG350_BACKLIGHT_PATH);
	if (result.length() > 0) {
		level = atoi(trim(result).c_str()) / 51;
	}
	#else
	level = 5;
	#endif
	TRACE("exit : %i", level);
	return level;
}

int GMenu2X::setBacklight(int val) {
	TRACE("enter - %i", val);
	if (val <= 0) val = 100;
	else if (val > 100) val = 0;
	#ifdef TARGET_RG350
	int rg350val = (int)(val * (255.0f/100));
	TRACE("rg350 value : %i", rg350val);
	// save a write
	if (rg350val == getBacklight()) 
		return val;

	if (procWriter(RG350_BACKLIGHT_PATH, rg350val)) {
		TRACE("success");
	} else {
		ERROR("Couldn't update backlight value to : %i", rg350val);
	}
	#endif
	return val;	
}

const string &GMenu2X::getExePath() {
	TRACE("enter path: %s", exe_path.c_str());
	if (exe_path.empty()) {
		char buf[255];
		memset(buf, 0, 255);
		int l = readlink("/proc/self/exe", buf, 255);

		exe_path = buf;
		exe_path = exe_path.substr(0, l);
		l = exe_path.rfind("/");
		exe_path = exe_path.substr(0, l + 1);
	}
	TRACE("exit path:%s", exe_path.c_str());
	return exe_path;
}

string GMenu2X::getAssetsPath() {
	if (assets_path.length())
		return assets_path;
	string result = USER_PREFIX;
	#ifdef TARGET_LINUX
	const char *homedir;
	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}
	TRACE("homedir : %s", homedir);
	result = (string)homedir + "/" + USER_PREFIX;
	#endif
	TRACE("exit : %s", result.c_str());
	assets_path = result;
	return result;
}

bool GMenu2X::doUpgrade(bool upgradeConfig) {
	INFO("GMenu2X::doUpgrade - enter");
	bool success = false;
	// check for the writable home directory existing
	string source = getExePath();
	string destination = getAssetsPath();

	INFO("GMenu2X::doUpgrade - from : %s, to : %s", source.c_str(), destination.c_str());

	if (!copyFile(source + "COPYING", destination + "COPYING")) return false;
	if (!copyFile(source + "ChangeLog.md", destination + "ChangeLog.md")) return false;
	if (!copyFile(source + "about.txt", destination + "about.txt")) return false;
	if (!copyFile(source + "gmenunx.png", destination + "gmenunx.png")) return false;
	if (!copyFile(source + "input.conf", destination + "input.conf")) return false;
	if (upgradeConfig) {
		if (!copyFile(source + "gmenunx.conf", destination + "gmenunx.conf")) return false;
	}

	stringstream ss;
	if (!dirExists(destination + "scripts")) {
		ss << "cp -arp \"" << source + "scripts"  << "\" " << destination << " && sync";
		string call = ss.str();
		system(call.c_str());
	}
	if (!dirExists(destination + "sections")) {
		ss << "cp -arp \"" << source + "sections"  << "\" " << destination << " && sync";
		string call = ss.str();
		system(call.c_str());
	}
	if (!dirExists(destination + "skins")) {
		ss << "cp -arp \"" << source + "skins"  << "\" " << destination << " && sync";
		string call = ss.str();
		system(call.c_str());
	}
	if (!dirExists(destination + "skins/Default")) {
		ss << "cp -arp \"" << source + "skins/Default"  << "\" " << destination + "skins/" << " && sync";
		string call = ss.str();
		system(call.c_str());
	}
	if (!dirExists(destination + "translations")) {
		ss << "cp -arp \"" << source + "translations"  << "\" " << destination << " && sync";
		string call = ss.str();
		system(call.c_str());
	}
	INFO("GMenu2X::doUpgrade - Upgrade complete");
	success = true;
	sync();
	TRACE("exit");
	return success;
}

bool GMenu2X::doInstall() {
	TRACE("enter");
	bool success = false;
	// check for the writable home directory existing
	string source = getExePath();
	string destination = getAssetsPath();

	TRACE("from : %s, to : %s", source.c_str(), destination.c_str());
	INFO("testing for writable home dir : %s", destination.c_str());
	
	bool fullCopy = !dirExists(destination);
	if (fullCopy) {
		INFO("doing a full copy");
		stringstream ss;
		ss << "cp -arp \"" << source << "\" " << destination << " && sync";
		string call = ss.str();
		TRACE("going to run :: %s", call.c_str());
		system(call.c_str());
		INFO("successful copy of assets");

		INFO("setting up the application cache");
		this->updateAppCache();

		success = true;
	}

	sync();
	TRACE("exit");
	return success;
}

string GMenu2X::getDiskFree(const char *path) {
	TRACE("enter - %s", path);
	string df = "N/A";
	struct statvfs b;

	if (statvfs(path, &b) == 0) {
		TRACE("read statvfs ok");
		// Make sure that the multiplication happens in 64 bits.
		uint32_t freeMiB = ((uint64_t)b.f_bfree * b.f_bsize) / (1024 * 1024);
		uint32_t totalMiB = ((uint64_t)b.f_blocks * b.f_frsize) / (1024 * 1024);
		TRACE("raw numbers - free: %lu, total: %lu, block size: %lu", b.f_bfree, b.f_blocks, b.f_bsize);
		stringstream ss;
		if (totalMiB >= 10000) {
			ss << (freeMiB / 1024) << "." << ((freeMiB % 1024) * 10) / 1024 << " / "
			   << (totalMiB / 1024) << "." << ((totalMiB % 1024) * 10) / 1024 << " GiB";
		} else {
			ss << freeMiB << " / " << totalMiB << " MiB";
		}
		std::getline(ss, df);
	} else WARNING("statvfs failed with error '%s'.\n", strerror(errno));
	TRACE("exit");
	return df;
}

int GMenu2X::drawButton(Button *btn, int x, int y) {
	if (y < 0) y = config->resolutionY() + y;
	// y = config->resolutionY - 8 - skinConfInt["bottomBarHeight"] / 2;
	btn->setPosition(x, y - 7);
	btn->paint();
	return x + btn->getRect().w + 6;
}

int GMenu2X::drawButton(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y < 0) y = config->resolutionY() + y;
	SDL_Rect re = {x, y, 0, 16};
	int padding = 4;

	if (this->sc->skinRes("imgs/buttons/" + btn + ".png") != NULL) {
		int imageWidth = (*this->sc)["imgs/buttons/" + btn + ".png"]->raw->w;
		(*this->sc)["imgs/buttons/" + btn + ".png"]->blit(
			s, 
			re.x + (imageWidth / 2), 
			re.y, 
			HAlignCenter | VAlignMiddle);
		re.w = imageWidth + padding;

		s->write(
			font, 
			text, 
			re.x + re.w, 
			re.y,
			VAlignMiddle, 
			skin->colours.fontAlt, 
			skin->colours.fontAltOutline);
		re.w += font->getTextWidth(text);
	}
	return x + re.w + (2 * padding);
}

int GMenu2X::drawButtonRight(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y < 0) y = config->resolutionY() + y;
	// y = config->resolutionY - skinConfInt["bottomBarHeight"] / 2;
	if (this->sc->skinRes("imgs/buttons/" + btn + ".png") != NULL) {
		x -= 16;
		(*this->sc)["imgs/buttons/" + btn + ".png"]->blit(s, x + 8, y + 2, HAlignCenter | VAlignMiddle);
		x -= 3;
		s->write(
			font, 
			text, 
			x, 
			y, 
			HAlignRight | VAlignMiddle, 
			skin->colours.fontAlt, 
			skin->colours.fontAltOutline);
		return x - 6 - font->getTextWidth(text);
	}
	return x - 6;
}


void GMenu2X::drawScrollBar(uint32_t pagesize, uint32_t totalsize, uint32_t pagepos, SDL_Rect scrollRect) {
	if (totalsize <= pagesize) return;

	//internal bar total height = height-2
	//bar size
	uint32_t bs = (scrollRect.h - 3) * pagesize / totalsize;
	//bar y position
	uint32_t by = (scrollRect.h - 3) * pagepos / totalsize;
	by = scrollRect.y + 3 + by;
	if ( by + bs > scrollRect.y + scrollRect.h - 4) by = scrollRect.y + scrollRect.h - 4 - bs;

	screen->rectangle(scrollRect.x + scrollRect.w - 4, by, 4, bs, skin->colours.listBackground);
	screen->box(scrollRect.x + scrollRect.w - 3, by + 1, 2, bs - 2, skin->colours.selectionBackground);
}

void GMenu2X::drawSlider(int val, int min, int max, Surface &icon, Surface &bg) {
	SDL_Rect progress = {52, 32, config->resolutionX()-84, 8};
	SDL_Rect box = {20, 20, config->resolutionX()-40, 32};

	val = constrain(val, min, max);

	bg.blit(screen,0,0);
	screen->box(box, skin->colours.msgBoxBackground);
	screen->rectangle(box.x+2, box.y+2, box.w-4, box.h-4, skin->colours.msgBoxBorder);

	icon.blit(screen, 28, 28);

	screen->box(progress, skin->colours.msgBoxBackground);
	screen->box(
		progress.x + 1, 
		progress.y + 1, 
		val * (progress.w - 3) / max + 1, 
		progress.h - 2, 
		skin->colours.msgBoxSelection);
	screen->flip();
}

#if defined(TARGET_GP2X)

void GMenu2X::gp2x_tvout_on(bool pal) {
	if (memdev != 0) {
		/*Ioctl_Dummy_t *msg;
		#define FBMMSP2CTRL 0x4619
		int TVHandle = ioctl(SDL_videofd, FBMMSP2CTRL, msg);*/
		if (cx25874!=0) gp2x_tvout_off();
		//if tv-out is enabled without cx25874 open, stop
		//if (memregs[0x2800 >> 1]&0x100) return;
		cx25874 = open("/dev/cx25874",O_RDWR);
		ioctl(cx25874, _IOW('v', 0x02, uint8_t), pal ? 4 : 3);
		memregs[0x2906 >> 1] = 512;
		memregs[0x28E4 >> 1] = memregs[0x290C >> 1];
		memregs[0x28E8 >> 1] = 239;
	}
}

void GMenu2X::gp2x_tvout_off() {
	if (memdev != 0) {
		close(cx25874);
		cx25874 = 0;
		memregs[0x2906 >> 1] = 1024;
	}
}

void GMenu2X::settingsOpen2x() {
	SettingsDialog sd(this, ts, tr["Open2x Settings"]);
	sd.addSetting(new MenuSettingBool(this, tr["USB net on boot"], tr["Allow USB networking to be started at boot time"],&o2x_usb_net_on_boot));
	sd.addSetting(new MenuSettingString(this, tr["USB net IP"], tr["IP address to be used for USB networking"],&o2x_usb_net_ip));
	sd.addSetting(new MenuSettingBool(this, tr["Telnet on boot"], tr["Allow telnet to be started at boot time"],&o2x_telnet_on_boot));
	sd.addSetting(new MenuSettingBool(this, tr["FTP on boot"], tr["Allow FTP to be started at boot time"],&o2x_ftp_on_boot));
	sd.addSetting(new MenuSettingBool(this, tr["GP2XJOY on boot"], tr["Create a js0 device for GP2X controls"],&o2x_gp2xjoy_on_boot));
	sd.addSetting(new MenuSettingBool(this, tr["USB host on boot"], tr["Allow USB host to be started at boot time"],&o2x_usb_host_on_boot));
	sd.addSetting(new MenuSettingBool(this, tr["USB HID on boot"], tr["Allow USB HID to be started at boot time"],&o2x_usb_hid_on_boot));
	sd.addSetting(new MenuSettingBool(this, tr["USB storage on boot"], tr["Allow USB storage to be started at boot time"],&o2x_usb_storage_on_boot));
//sd.addSetting(new MenuSettingInt(this, tr["Speaker Mode on boot"], tr["Set Speaker mode. 0 = Mute, 1 = Phones, 2 = Speaker"],&volumeMode,0,2,1));
	sd.addSetting(new MenuSettingInt(this, tr["Speaker Scaler"], tr["Set the Speaker Mode scaling 0-150\% (default is 100\%)"],&volumeScalerNormal,100, 0,150));
	sd.addSetting(new MenuSettingInt(this, tr["Headphones Scaler"], tr["Set the Headphones Mode scaling 0-100\% (default is 65\%)"],&volumeScalerPhones,65, 0,100));

	if (sd.exec() && sd.edited()) {
		writeConfigOpen2x();
		switch(volumeMode) {
			case VOLUME_MODE_MUTE:   setVolumeScaler(VOLUME_SCALER_MUTE); break;
			case VOLUME_MODE_PHONES: setVolumeScaler(volumeScalerPhones); break;
			case VOLUME_MODE_NORMAL: setVolumeScaler(volumeScalerNormal); break;
		}
		setVolume(confInt["globalVolume"]);
	}
}

void GMenu2X::readConfigOpen2x() {
	string conffile = "/etc/config/open2x.conf";
	if (!fileExists(conffile)) return;
	ifstream inf(conffile.c_str(), ios_base::in);
	if (!inf.is_open()) return;
	string line;
	while (getline(inf, line, '\n')) {
		string::size_type pos = line.find("=");
		string name = trim(line.substr(0,pos));
		string value = trim(line.substr(pos+1,line.length()));

		if (name=="USB_NET_ON_BOOT") o2x_usb_net_on_boot = value == "y" ? true : false;
		else if (name=="USB_NET_IP") o2x_usb_net_ip = value;
		else if (name=="TELNET_ON_BOOT") o2x_telnet_on_boot = value == "y" ? true : false;
		else if (name=="FTP_ON_BOOT") o2x_ftp_on_boot = value == "y" ? true : false;
		else if (name=="GP2XJOY_ON_BOOT") o2x_gp2xjoy_on_boot = value == "y" ? true : false;
		else if (name=="USB_HOST_ON_BOOT") o2x_usb_host_on_boot = value == "y" ? true : false;
		else if (name=="USB_HID_ON_BOOT") o2x_usb_hid_on_boot = value == "y" ? true : false;
		else if (name=="USB_STORAGE_ON_BOOT") o2x_usb_storage_on_boot = value == "y" ? true : false;
		else if (name=="VOLUME_MODE") volumeMode = savedVolumeMode = constrain( atoi(value.c_str()), 0, 2);
		else if (name=="PHONES_VALUE") volumeScalerPhones = constrain( atoi(value.c_str()), 0, 100);
		else if (name=="NORMAL_VALUE") volumeScalerNormal = constrain( atoi(value.c_str()), 0, 150);
	}
	inf.close();
}

void GMenu2X::writeConfigOpen2x() {
	ledOn();
	string conffile = "/etc/config/open2x.conf";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		inf << "USB_NET_ON_BOOT=" << ( o2x_usb_net_on_boot ? "y" : "n" ) << endl;
		inf << "USB_NET_IP=" << o2x_usb_net_ip << endl;
		inf << "TELNET_ON_BOOT=" << ( o2x_telnet_on_boot ? "y" : "n" ) << endl;
		inf << "FTP_ON_BOOT=" << ( o2x_ftp_on_boot ? "y" : "n" ) << endl;
		inf << "GP2XJOY_ON_BOOT=" << ( o2x_gp2xjoy_on_boot ? "y" : "n" ) << endl;
		inf << "USB_HOST_ON_BOOT=" << ( (o2x_usb_host_on_boot || o2x_usb_hid_on_boot || o2x_usb_storage_on_boot) ? "y" : "n" ) << endl;
		inf << "USB_HID_ON_BOOT=" << ( o2x_usb_hid_on_boot ? "y" : "n" ) << endl;
		inf << "USB_STORAGE_ON_BOOT=" << ( o2x_usb_storage_on_boot ? "y" : "n" ) << endl;
		inf << "VOLUME_MODE=" << volumeMode << endl;
		if (volumeScalerPhones != VOLUME_SCALER_PHONES) inf << "PHONES_VALUE=" << volumeScalerPhones << endl;
		if (volumeScalerNormal != VOLUME_SCALER_NORMAL) inf << "NORMAL_VALUE=" << volumeScalerNormal << endl;
		inf.close();
		sync();
	}
	ledOff();
}

void GMenu2X::activateSdUsb() {
	if (usbnet) {
		MessageBox mb(this, tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		MessageBox mb(this, tr["USB Enabled (SD)"],"skin:icons/usb.png");
		mb.setButton(CONFIRM, tr["Turn off"]);
		mb.exec();
		system("scripts/usbon.sh nand");
	}
}

void GMenu2X::activateNandUsb() {
	if (usbnet) {
		MessageBox mb(this, tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		system("scripts/usbon.sh nand");
		MessageBox mb(this, tr["USB Enabled (Nand)"],"skin:icons/usb.png");
		mb.setButton(CONFIRM, tr["Turn off"]);
		mb.exec();
		system("scripts/usboff.sh nand");
	}
}

void GMenu2X::activateRootUsb() {
	if (usbnet) {
		MessageBox mb(this,tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		system("scripts/usbon.sh root");
		MessageBox mb(this,tr["USB Enabled (Root)"],"skin:icons/usb.png");
		mb.setButton(CONFIRM, tr["Turn off"]);
		mb.exec();
		system("scripts/usboff.sh root");
	}
}

void GMenu2X::applyRamTimings() {
	// 6 4 1 1 1 2 2
	if (memdev!=0) {
		int tRC = 5, tRAS = 3, tWR = 0, tMRD = 0, tRFC = 0, tRP = 1, tRCD = 1;
		memregs[0x3802>>1] = ((tMRD & 0xF) << 12) | ((tRFC & 0xF) << 8) | ((tRP & 0xF) << 4) | (tRCD & 0xF);
		memregs[0x3804>>1] = ((tRC & 0xF) << 8) | ((tRAS & 0xF) << 4) | (tWR & 0xF);
	}
}

void GMenu2X::applyDefaultTimings() {
	// 8 16 3 8 8 8 8
	if (memdev!=0) {
		int tRC = 7, tRAS = 15, tWR = 2, tMRD = 7, tRFC = 7, tRP = 7, tRCD = 7;
		memregs[0x3802>>1] = ((tMRD & 0xF) << 12) | ((tRFC & 0xF) << 8) | ((tRP & 0xF) << 4) | (tRCD & 0xF);
		memregs[0x3804>>1] = ((tRC & 0xF) << 8) | ((tRAS & 0xF) << 4) | (tWR & 0xF);
	}
}

void GMenu2X::setGamma(int gamma) {
	float fgamma = (float)constrain(gamma,1,100)/10;
	fgamma = 1 / fgamma;
	MEM_REG[0x2880>>1] &= ~(1<<12);
	MEM_REG[0x295C>>1] = 0;

	for (int i = 0; i < 256; i++) {
		uint8_t g = (uint8_t)(255.0*pow(i/255.0,fgamma));
		uint16_t s = (g << 8) | g;
		MEM_REG[0x295E >> 1] = s;
		MEM_REG[0x295E >> 1] = g;
	}
}

void GMenu2X::setVolumeScaler(int scale) {
	scale = constrain(scale,0,MAX_VOLUME_SCALE_FACTOR);
	uint32_t soundDev = open("/dev/mixer", O_WRONLY);
	if (soundDev) {
		ioctl(soundDev, SOUND_MIXER_PRIVATE2, &scale);
		close(soundDev);
	}
}

int GMenu2X::getVolumeScaler() {
	int currentscalefactor = -1;
	uint32_t soundDev = open("/dev/mixer", O_RDONLY);
	if (soundDev) {
		ioctl(soundDev, SOUND_MIXER_PRIVATE1, &currentscalefactor);
		close(soundDev);
	}
	return currentscalefactor;
}

void GMenu2X::readCommonIni() {
	if (!fileExists("/usr/gp2x/common.ini")) return;
	ifstream inf("/usr/gp2x/common.ini", ios_base::in);
	if (!inf.is_open()) return;
	string line;
	string section = "";
	while (getline(inf, line, '\n')) {
		line = trim(line);
		if (line[0]=='[' && line[line.length()-1]==']') {
			section = line.substr(1,line.length()-2);
		} else {
			string::size_type pos = line.find("=");
			string name = trim(line.substr(0,pos));
			string value = trim(line.substr(pos+1,line.length()));

			if (section=="usbnet") {
				if (name=="enable")
					usbnet = value=="true" ? true : false;
				else if (name=="ip")
					ip = value;

			} else if (section=="server") {
				if (name=="inet")
					inet = value=="true" ? true : false;
				else if (name=="samba")
					samba = value=="true" ? true : false;
				else if (name=="web")
					web = value=="true" ? true : false;
			}
		}
	}
	inf.close();
}

void GMenu2X::initServices() {
	if (usbnet) {
		string services = "scripts/services.sh "+ip+" "+(inet?"on":"off")+" "+(samba?"on":"off")+" "+(web?"on":"off")+" &";
		system(services.c_str());
	}
}

#endif
