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
#include "gmenu2x.h"
#include "filelister.h"

#include "iconbutton.h"
#include "messagebox.h"
#include "progressbar.h"
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
#include "renderer.h"
#include "loader.h"
#include "installer.h"
#include "constants.h"

#ifdef HAVE_LIBOPK
#include "opkcache.h"
const string OPK_FOLDER_NAME = "apps";
const string OPK_INTERNAL_PATH = "/media/data/" + OPK_FOLDER_NAME;
const string OPK_PLATFORM = "gcw0";
#endif

#ifndef __BUILDTIME__
	#define __BUILDTIME__ __DATE__ " - " __TIME__ 
#endif

static GMenu2X *app;

using std::ifstream;
using std::ofstream;
using std::stringstream;
using namespace fastdelegate;

static void quit_all(int err) {
	delete app;
	exit(err);
}

GMenu2X::~GMenu2X() {
	TRACE("enter\n\n");
	quit();
	delete ui;
	delete menu;
	delete screen;
	delete font;
	delete fontTitle;
	delete fontSectionTitle;
	delete hw;
	delete skin;
	delete sc;
	delete config;
	#ifdef HAVE_LIBOPK
	delete opkCache;
	#endif
	TRACE("exit\n\n");
}

void GMenu2X::releaseScreen() {
	TRACE("calling SDL_Quit");
	SDL_Quit();
}

void GMenu2X::quit() {
	TRACE("enter");
	if (!this->sc->empty()) {
		TRACE("SURFACE EXISTED");
		writeConfig();
		this->hw->ledOff();
		fflush(NULL);
		TRACE("clearing the surface collection");
		this->sc->clear();
		TRACE("freeing the screen");
		this->screen->free();
		releaseScreen();
	}
	INFO("GMenuNX::quit - exit");
}

int main(int argc, char * argv[]) {
	INFO("GMenuNX starting: Build Date - %s", __BUILDTIME__);

	signal(SIGINT, &quit_all);
	signal(SIGSEGV,&quit_all);
	signal(SIGTERM,&quit_all);

	app = new GMenu2X();
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

void GMenu2X::updateAppCache(std::function<void(string)> callback) {
	TRACE("enter");
	#ifdef HAVE_LIBOPK

	string externalPath = this->config->externalAppPath();
	vector<string> opkDirs { OPK_INTERNAL_PATH, externalPath };
	string rootDir = this->getWriteablePath();
	TRACE("rootDir : %s", rootDir.c_str());
	if (nullptr == this->opkCache) {
		this->opkCache = new OpkCache(opkDirs, rootDir);
	}
	assert(this->opkCache);
	this->opkCache->update(callback);
	sync();

	#endif
	TRACE("exit");
}
int GMenu2X::cacheSize() {
	#ifdef HAVE_LIBOPK
	return this->opkCache->size();
	#else
	return 0;
	#endif
}
GMenu2X::GMenu2X() : input(screenManager) {

	TRACE("enter");

	TRACE("creating hardware layer");
	this->hw = HwFactory::GetHardware();

	TRACE("ledOn");
	this->hw->ledOn();

	TRACE("get paths");
	this->getExePath();
	this->getWriteablePath();
	this->needsInstalling = !Config::configExistsUnderPath(this->getWriteablePath());

	string localAssetsPath = this->getReadablePath();
	TRACE("needs deploying : %i, localAssetsPath : %s", 
		this->needsInstalling, 
		localAssetsPath.c_str());

	TRACE("init translations");
	this->tr.setPath(localAssetsPath);

	TRACE("reading config from : %s", localAssetsPath.c_str());
	this->config = new Config(localAssetsPath);
	this->config->loadConfig();
	if (!config->lang().empty()) {
		this->tr.setLang(this->config->lang());
	}

	TRACE("backlight");
	this->hw->setBacklightLevel(config->backlightLevel());

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

	TRACE("new surface");
	this->screen = new Surface();

	TRACE("SDL_SetVideoMode - x:%i y:%i bpp:%i", 
		config->resolutionX(), 
		config->resolutionY(), 
		config->videoBpp());
	this->screen->raw = SDL_SetVideoMode(
		config->resolutionX(), 
		config->resolutionY(), 
		config->videoBpp(), 
		SDL_HWSURFACE|SDL_DOUBLEBUF);

	TRACE("new ui");
	this->ui = new UI(this);

	TRACE("new skin");
	this->skin = new Skin(localAssetsPath, 
		config->resolutionX(),  
		config->resolutionY());

	if (!this->skin->loadSkin( config->skin())) {
		ERROR("GMenu2X::ctor - couldn't load skin, using defaults");
	}

	TRACE("new surface collection");
	this->sc = new SurfaceCollection(this->skin, true);

	TRACE("initFont");
	initFont();

	TRACE("screen mgr");
	this->screenManager.setScreenTimeout(600);
	TRACE("input");
	this->input.init(this->getReadablePath() + "input.conf");
	setInputSpeed();

	TRACE("exit");

}

void GMenu2X::main() {
	TRACE("enter");

	// has to come before the app cache thread kicks off
	TRACE("checking sd card");
	this->hw->checkUDC();

	// create this early so we can give it to the thread, but don't exec it yet
	ProgressBar * pbLoading = new ProgressBar(
		this, 
		"Please wait, loading the everything...", 
		"skin:icons/device.png");

	std::thread * thread_cache = nullptr;

	if (this->needsInstalling) {
		if(dirExists(this->getWriteablePath())) {
			this->doUpgrade();
		} else this->doInitialSetup();
		pbLoading->exec();
	} else {
		pbLoading->updateDetail("Checking for new applications...");
		TRACE("kicking off our app cache thread");
		thread_cache = new std::thread(
			&GMenu2X::updateAppCache, 
			this, 
			std::bind( &ProgressBar::updateDetail, pbLoading, std::placeholders::_1 ));

		if (this->skin->showLoader) {
			Loader loader(this);
			loader.run();
		}
		pbLoading->exec();
	}
	pbLoading->updateDetail("Initialising hardware");
	this->hw->setVolumeLevel(this->config->globalVolume());
	this->hw->setPerformanceMode(this->config->performance());
	this->hw->setCPUSpeed(this->config->cpuMenu());

	setWallpaper(this->skin->wallpaper);

	if (thread_cache != nullptr) {
		// we need to re-join before building the menu
		thread_cache->join();
		delete thread_cache;
		TRACE("app cache thread has finished");
	}

	TRACE("screen manager");
	screenManager.setScreenTimeout( config->backlightTimeout());

	initMenu();
	readTmp();

	TRACE("pthread");
	pthread_t thread_id;
	if (pthread_create(&thread_id, NULL, mainThread, this)) {
		ERROR("%s, failed to create main thread\n", __func__);
	}

	TRACE("new renderer");
	Renderer *renderer = new Renderer(this);

	pbLoading->finished();
	delete pbLoading;

	input.setWakeUpInterval(1000);
	this->hw->ledOff();

	if (this->lastSelectorElement >- 1 && \
		this->menu->selLinkApp() != NULL && \
		(!this->menu->selLinkApp()->getSelectorDir().empty() || \
		!this->lastSelectorDir.empty())) {

		TRACE("recoverSession happening");
		this->menu->selLinkApp()->selector(
			this->lastSelectorElement, 
			this->lastSelectorDir);
	}

	bool quit = false;
	while (!quit) {
		try {
			TRACE("loop");
			renderer->render();
		} catch (int e) {
			ERROR("render - error : %i", e);
		}

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
				TRACE("Run called");
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
			} else if (this->input[PAGEUP]) {
				menu->letterPrevious();
			} else if (this->input[PAGEDOWN]) {
				menu->letterNext();
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

			FileLister fl(this->getReadablePath() + relativePath, false, true);
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

	bool isDefaultLauncher = Installer::isDefaultLauncher(getOpkPath());

	TRACE("add built in action links");
	int i = menu->getSectionIndex("applications");
	menu->addActionLink(i, 
						tr["Battery Logger"], 
						MakeDelegate(this, &GMenu2X::batteryLogger), 
						tr["Log battery power to battery.csv"], 
						"skin:icons/ebook.png");
	menu->addActionLink(i, 
						tr["Explorer"], 
						MakeDelegate(this, &GMenu2X::explorer), 
						tr["Browse files and launch apps"], 
						"skin:icons/explorer.png");

	i = menu->getSectionIndex("settings");
	menu->addActionLink(
						i, 
						tr["About"], 
						MakeDelegate(this, &GMenu2X::about), 
						tr["Info about system"], 
						"skin:icons/about.png");

	if (!isDefaultLauncher) {
		menu->addActionLink(
							i, 
							tr["Install me"], 
							MakeDelegate(this, &GMenu2X::doInstall), 
							tr["Set " + APP_NAME + " as your launcher"], 
							"skin:icons/device.png");
	}

	if (fileExists(getWriteablePath() + "log.txt"))
		menu->addActionLink(
						i, 
						tr["Log Viewer"], 
						MakeDelegate(this, &GMenu2X::viewLog), 
						tr["Displays last launched program's output"], 
						"skin:icons/ebook.png");

	if (this->hw->getCardStatus() == IHardware::MMC_UNMOUNTED)
		menu->addActionLink(
						i, 
						tr["Mount"], 
						MakeDelegate(this, &GMenu2X::mountSdDialog), 
						tr["Mount external SD"], 
						"skin:icons/eject.png");

	menu->addActionLink(
						i, 
						tr["Power"], 
						MakeDelegate(this, &GMenu2X::poweroffDialog), 
						tr["Power menu"], 
						"skin:icons/exit.png");

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

	if (this->hw->getCardStatus() == IHardware::MMC_MOUNTED)
		menu->addActionLink(
						i, 
						tr["Umount"], 
						MakeDelegate(this, &GMenu2X::umountSdDialog), 
						tr["Umount external SD"], 
						"skin:icons/eject.png");
	if (isDefaultLauncher) {
		menu->addActionLink(
							i, 
							tr["UnInstall me"], 
							MakeDelegate(this, &GMenu2X::doUnInstall), 
							tr["Remove " + APP_NAME + " as your launcher"], 
							"skin:icons/device.png");
	}


	if (this->config->version() < APP_MIN_CONFIG_VERSION || !this->needsInstalling) {
		menu->addActionLink(
							i, 
							tr["Upgrade me"], 
							MakeDelegate(this, &GMenu2X::doUpgrade), 
							tr["Upgrade " + APP_NAME], 
							"skin:icons/device.png");
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
	TRACE("re-order now we've added these guys");
	menu->orderLinks();
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
	vector<string> skinList = Skin::getSkins(getReadablePath());

	TRACE("getting translations");
	FileLister fl_tr(getReadablePath() + "translations");
	fl_tr.browse();
	fl_tr.insertFile("English");
	// local vals
	string lang = tr.lang();
	string skin = config->skin();
	string batteryType = config->batteryType();
	string performanceMode = this->hw->getPerformanceMode();
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

	vector<string> performanceModes = this->hw->getPerformanceModes();
	string currentDatetime = this->hw->getSystemdateTime();
	string prevDateTime = currentDatetime;

	SettingsDialog sd(this, ts, tr["Settings"], "skin:icons/configure.png");
	sd.addSetting(new MenuSettingMultiString(this, tr["Language"], tr["Set the language used by GMenuNX"], &lang, &fl_tr.getFiles()));
	sd.addSetting(new MenuSettingDateTime(this, tr["Date & Time"], tr["Set system's date & time"], &currentDatetime));
	sd.addSetting(new MenuSettingMultiString(this, tr["Battery profile"], tr["Set the battery discharge profile"], &batteryType, &batteryTypes));

	sd.addSetting(new MenuSettingMultiString(this, tr["Skin"], tr["Set the skin used by GMenuNX"], &skin, &skinList));

	if (performanceModes.size() > 1)
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
		if (performanceMode != this->hw->getPerformanceMode()) {
			config->performance(performanceMode);
			this->hw->setPerformanceMode(performanceMode);
		}
		config->skin(skin);
		config->batteryType(batteryType);
		config->saveSelection(saveSelection);
		config->outputLogs(outputLogs);
		config->backlightTimeout(backlightTimeout);
		config->powerTimeout(powerTimeout);
		config->backlightLevel(backlightLevel);
		config->globalVolume(globalVolume);

		TRACE("updating the settings");
		if (curGlobalVolume != config->globalVolume()) {
			curGlobalVolume = this->hw->setVolumeLevel(config->globalVolume());
		}
		if (curGlobalBrightness != config->backlightLevel()) {
			curGlobalBrightness = this->hw->setBacklightLevel(config->backlightLevel());
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

		if (!this->needsInstalling) {
			this->writeConfig();
		} else {
			restartNeeded = false;
		}

		if (prevDateTime != currentDatetime) {
			TRACE("updating datetime");
			MessageBox mb(this, tr["Updating system time"]);
			mb.setAutoHide(500);
			mb.exec();
			this->hw->setSystemDateTime(currentDatetime);
		}
		if (restartNeeded) {
			TRACE("restarting because skins changed");
			restartDialog();
		}
	}
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
		if (reset_skin && !this->needsInstalling) {
			this->skin->remove();
		}
		if (reset_gmenu && !this->needsInstalling) {
			this->config->remove();
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
		this-hw->setCPUSpeed(config->cpuMenu());
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
		else if (name == "TVOut") this->hw->setTVOutMode(value);
		//else if (name == "tvOutPrev") tvOutPrev = atoi(value.c_str());
	}
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
		//inf << "tvOutPrev=" << tvOutPrev << endl;
		inf << "TVOut=" << this->hw->getTVOutMode() << endl;
		inf.close();
	}
}

void GMenu2X::writeConfig() {
	TRACE("enter");
	this->hw->ledOn();
	// don't try and save to RO file system
	if (!this->needsInstalling) {
		if (config->saveSelection() && menu != NULL) {
			config->section(menu->selSectionIndex());
			config->link(menu->selLinkIndex());
		}
		config->save();
	}
	TRACE("ledOff");
	this->hw->ledOff();
	TRACE("exit");
}

void GMenu2X::writeSkinConfig() {
	TRACE("enter");
	this->hw->ledOn();
	skin->save();
	this->hw->ledOff();
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
		sd.addSetting(new MenuSettingBool(this, tr["Show loader"], tr["Show loader animation, if exists"], &skin->showLoader));
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
				if (this->sc->addImage(getReadablePath() + "skins/" + skin->name + "/wallpapers/" + wpCurrent) != NULL)
					skin->wallpaper = getReadablePath() + "skins/" + skin->name + "/wallpapers/" + wpCurrent;
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

	string uptime = this->hw->uptime();
	int battLevel = this->hw->getBatteryLevel();
	TRACE("batt level : %i", battLevel);
	int battPercent = (battLevel * 20);
	TRACE("batt percent : %i", battPercent);
	
	char buffer[50];
	int n = sprintf (buffer, "%i %%", battPercent);
	string batt(buffer);

	string opkPath = getOpkPath();
	if (opkPath.length() == 0) {
		opkPath = "Not found";
	}
	temp = "\n";
	temp += tr["Build date: "] + __BUILDTIME__ + "\n";
	temp += "Opk: " + opkPath + "\n";
	temp += "Config: " + this->config->configFile() + "\n";
	temp += "Skin: " + this->skin->name + "\n";
	temp += tr["Uptime: "] + uptime + "\n";
	temp += tr["Battery: "] + ((battLevel == 6) ? tr["Charging"] : batt) + "\n";
	temp += tr["Internal storage size: "] + this->hw->getDiskSize(this->hw->getInternalMountDevice()) + "\n";
	temp += tr["Internal storage free: "] + this->hw->getDiskFree("/media/data") + "\n";

	this->hw->checkUDC();
	string externalSize;
	switch(this->hw->getCardStatus()) {
		case IHardware::MMC_MOUNTED:
			externalSize = this->hw->getDiskSize(this->hw->getExternalMountDevice());
			break;
		case IHardware::MMC_UNMOUNTED:
			externalSize = tr["Inserted, not mounted"];
			break;
		default:
			externalSize = tr["Not inserted"];
	};

	temp += tr["External storage size: "] + externalSize + "\n";
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
	
	TRACE("append - file %sabout.txt", getReadablePath().c_str());
	td.appendFile(getReadablePath() + "about.txt");
	td.exec();
	TRACE("exit");
}

void GMenu2X::viewLog() {
	string logfile = getWriteablePath() + "log.txt";
	if (!fileExists(logfile)) return;

	TextDialog td(this, tr["Log Viewer"], tr["Last launched program's output"], "skin:icons/ebook.png");
	td.appendFile(getWriteablePath() + "log.txt");
	td.exec();

	MessageBox mb(this, tr["Do you want to delete the log file?"], "skin:icons/ebook.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		this->hw->ledOn();
		unlink(logfile.c_str());
		sync();
		menu->deleteSelectedLink();
		this->hw->ledOff();
	}
}

void GMenu2X::batteryLogger() {
	this->hw->ledOn();
	BatteryLoggerDialog bl(
		this, 
		tr["Battery Logger"], 
		tr["Log battery power to battery.csv"], 
		"skin:icons/ebook.png");
	bl.exec();
	this->hw->ledOff();
}

void GMenu2X::linkScanner() {
	LinkScannerDialog ls(
		this, 
		tr["Link scanner"], 
		tr["Scan for applications and games"], 
		"skin:icons/configure.png");
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
			this->hw->setCPUSpeed(config->cpuMenu());
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

const string &GMenu2X::getWriteablePath() {
	if (this->writeable_path.length())
		return this->writeable_path;
	string result = USER_PREFIX;
	#ifdef TARGET_LINUX
	const char *homedir;
	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}
	TRACE("homedir : %s", homedir);
	result = (string)homedir + "/" + USER_PREFIX;
	#endif
	this->writeable_path = result;
	TRACE("exit : %s", this->writeable_path.c_str());
	return this->writeable_path;
}

const string &GMenu2X::getReadablePath() {
	return (this->needsInstalling ? this->getExePath() : this->getWriteablePath());
}

void GMenu2X::doUpgrade() {
	INFO("GMenu2X::doUpgrade - enter");
	bool success = false;
	// check for the writable home directory existing
	string source = getExePath();
	string destination = getWriteablePath();

	INFO("upgrade from : %s, to : %s", source.c_str(), destination.c_str());
	ProgressBar *pbInstall = new ProgressBar(this, "Copying new data for upgrade...", "skin:icons/device.png");
	pbInstall->exec();
	INFO("doing a full copy");

	Installer *installer = new Installer(
		source, 
		destination, 
		std::bind( &ProgressBar::updateDetail, pbInstall, std::placeholders::_1) );

	if (installer->upgrade()) {
		pbInstall->finished();
	} else {
		pbInstall->updateDetail("Upgrade failed");
		pbInstall->finished(2000);
	}
	delete pbInstall;
	INFO("GMenu2X::doUpgrade - Upgrade complete");
	success = true;
	sync();
	TRACE("exit");
}

void GMenu2X::doInstall() {
	TRACE("enter");
	bool success = false;

	ProgressBar *pbInstall = new ProgressBar(this, "Installing launcher script...", "skin:icons/device.png");
	pbInstall->exec();

	if (Installer::deployLauncher()) {
		pbInstall->updateDetail("Install successful");
		pbInstall->finished(2000);
		success = true;
	} else {
		pbInstall->updateDetail("Install failed");
		pbInstall->finished(2000);
	}
	delete pbInstall;

	if (success) {
		this->initMenu();
		//this->restartDialog(true);
	}
	TRACE("exit");
}

void GMenu2X::doUnInstall() {
	TRACE("enter");
	bool success = false;
	ProgressBar *pbInstall = new ProgressBar(
		this, 
		"UnInstalling launcher script...", 
		"skin:icons/device.png");

	pbInstall->exec();

	if (Installer::removeLauncher()) {
		pbInstall->updateDetail("UnInstall successful");
		pbInstall->finished(2000);
		success = true;
	} else {
		pbInstall->updateDetail("UnInstall failed");
		pbInstall->finished(2000);
	}
	delete pbInstall;
	if (success) {
		this->initMenu();
		//this->restartDialog(true);
	}
	TRACE("exit");
}

bool GMenu2X::doInitialSetup() {
	bool success = false;

	// check for the writable home directory existing
	string source = getExePath();
	string destination = getWriteablePath();

	TRACE("from : %s, to : %s", source.c_str(), destination.c_str());
	INFO("testing for writable home dir : %s", destination.c_str());
	
	ProgressBar *pbInstall = new ProgressBar(
		this, 
		"Copying data to home directory...", 
		"skin:icons/device.png");

	pbInstall->exec();
	INFO("doing a full copy");

	Installer *installer = new Installer(
		source, 
		destination, 
		std::bind( &ProgressBar::updateDetail, pbInstall, std::placeholders::_1) );

	if (installer->install()) {
		pbInstall->finished();
	} else {
		pbInstall->updateDetail("Install failed");
		pbInstall->finished(2000);
	}
	delete pbInstall;

	INFO("setting up the application cache");
	ProgressBar * pbCache = new ProgressBar(
		this, 
		"Updating the application cache...", 
		"skin:icons/device.png");

	pbCache->exec();
	this->updateAppCache(std::bind( &ProgressBar::updateDetail, pbCache, std::placeholders::_1));
	pbCache->updateDetail("Finished creating cache");
	pbCache->finished(200);
	delete pbCache;

	sync();
	success = true;
	TRACE("exit");
	return success;
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
		ProgressBar pbShutdown(this, "Shutting down", "skin:icons/device.png");
		pbShutdown.updateDetail     ("   ~ now ~   ");
		pbShutdown.exec();
		pbShutdown.finished(500);
		this->hw->setBacklightLevel(0);
		sync();
		std::system("poweroff");
	}
	else if (response == SECTION_NEXT) {
		ProgressBar pbReboot(this, " Rebooting", "skin:icons/device.png");
		pbReboot.updateDetail     ("  ~ now ~ ");
		pbReboot.exec();
		pbReboot.finished(500);
		this->hw->setBacklightLevel(0);
		sync();
		std::system("reboot");
	}
}

void GMenu2X::mountSdDialog() {
	MessageBox mb(this, tr["Mount SD card?"], "skin:icons/eject.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);

	if (mb.exec() == CONFIRM) {
		int currentMenuIndex = menu->selSectionIndex();
		int currentLinkIndex = menu->selLinkIndex();
		string result = this->hw->mountSd();
		initMenu();
		menu->setSectionIndex(currentMenuIndex);
		menu->setLinkIndex(currentLinkIndex);

		string msg, icon;
		int wait = 1000;
		bool error = false;
		switch(this->hw->getCardStatus()) {
			case IHardware::MMC_MOUNTED:
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

void GMenu2X::umountSdDialog() {
	MessageBox mb(this, tr["Umount SD card?"], "skin:icons/eject.png");
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		int currentMenuIndex = menu->selSectionIndex();
		int currentLinkIndex = menu->selLinkIndex();
		string result = this->hw->umountSd();
		initMenu();
		menu->setSectionIndex(currentMenuIndex);
		menu->setLinkIndex(currentLinkIndex);

		string msg, icon;
		int wait = 1000;
		switch(this->hw->getCardStatus()) {
			case IHardware::MMC_UNMOUNTED:
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
		this->hw->ledOn();
		if (menu->addLink(fd.getPath(), fd.getFile())) {
			editLink();
		}
		sync();
		this->hw->ledOff();
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
	//bool linkSelBrowser = menu->selLinkApp()->getSelectorBrowser();
	//string linkSelScreens = menu->selLinkApp()->getSelectorScreens();
	string linkSelAliases = menu->selLinkApp()->getAliasFile();
	//int linkClock = menu->selLinkApp()->clock();
	string linkBackdrop = menu->selLinkApp()->getBackdrop();
	string dialogTitle = tr.translate("Edit $1", linkTitle.c_str(), NULL);
	string dialogIcon = menu->selLinkApp()->getIconPath();

	SettingsDialog sd(this, ts, dialogTitle, dialogIcon);
	sd.addSetting(new MenuSettingFile(			this, tr["Executable"],		tr["Application this link points to"], &linkExec, ".dge,.gpu,.gpe,.sh,.bin,.elf,", EXTERNAL_CARD_PATH, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingString(		this, tr["Title"],			tr["Link title"], &linkTitle, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingString(		this, tr["Description"],	tr["Link description"], &linkDescription, dialogTitle, dialogIcon));
	
	if (oldSection != menu->selLinkApp()->getFavouriteFolder()) {
		sd.addSetting(new MenuSettingMultiString(	this, tr["Section"],		tr["The section this link belongs to"], &newSection, &menu->getSections()));
	}
	sd.addSetting(new MenuSettingImage(			this, tr["Icon"],			tr["Select a custom icon for the link"], &linkIcon, ".png,.bmp,.jpg,.jpeg,.gif", dir_name(linkIcon), dialogTitle, dialogIcon, skin->name));
	//sd.addSetting(new MenuSettingInt(			this, tr["CPU Clock"],		tr["CPU clock frequency when launching this link"], &linkClock, config->cpuMenu, config->cpuMin, config->cpuMax, 6));
	sd.addSetting(new MenuSettingString(		this, tr["Parameters"],		tr["Command line arguments to pass to the application"], &linkParams, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingDir(			this, tr["Selector Path"],	tr["Directory to start the selector"], &linkSelDir, EXTERNAL_CARD_PATH, dialogTitle, dialogIcon));
	//sd.addSetting(new MenuSettingBool(			this, tr["Show Folders"],	tr["Allow the selector to change directory"], &linkSelBrowser));
	sd.addSetting(new MenuSettingString(		this, tr["File Filter"],	tr["Filter by file extension (separate with commas)"], &linkSelFilter, dialogTitle, dialogIcon));
	//sd.addSetting(new MenuSettingDir(			this, tr["Screenshots"],	tr["Directory of the screenshots for the selector"], &linkSelScreens, EXTERNAL_CARD_PATH, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingFile(			this, tr["Aliases"],		tr["File containing a list of aliases for the selector"], &linkSelAliases, ".txt,.dat", EXTERNAL_CARD_PATH, dialogTitle, dialogIcon));
	sd.addSetting(new MenuSettingImage(			this, tr["Backdrop"],		tr["Select an image backdrop"], &linkBackdrop, ".png,.bmp,.jpg,.jpeg", EXTERNAL_CARD_PATH, dialogTitle, dialogIcon, skin->name));
	sd.addSetting(new MenuSettingFile(			this, tr["Manual"],   		tr["Select a Manual or Readme file"], &linkManual, ".man.png,.txt,.me", dir_name(linkManual), dialogTitle, dialogIcon));

	if (sd.exec() && sd.edited() && sd.save) {
		this->hw->ledOn();

		menu->selLinkApp()->setExec(linkExec);
		menu->selLinkApp()->setTitle(linkTitle);
		menu->selLinkApp()->setDescription(linkDescription);
		menu->selLinkApp()->setIcon(linkIcon);
		menu->selLinkApp()->setManual(linkManual);
		menu->selLinkApp()->setParams(linkParams);
		menu->selLinkApp()->setSelectorFilter(linkSelFilter);
		menu->selLinkApp()->setSelectorDir(linkSelDir);
		//menu->selLinkApp()->setSelectorBrowser(linkSelBrowser);
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
		this->hw->ledOff();
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
			this->hw->ledOn();
			menu->deleteSelectedLink();
			sync();
			this->hw->ledOff();
		}
	}
}

void GMenu2X::addSection() {
	InputDialog id(this, ts, tr["Insert a name for the new section"], "", tr["Add section"], "skin:icons/section.png");
	if (id.exec()) {
		//only if a section with the same name does not exist
		if (find(menu->getSections().begin(), menu->getSections().end(), id.getInput()) == menu->getSections().end()) {
			//section directory doesn't exists
			this->hw->ledOn();
			if (menu->addSection(id.getInput())) {
				menu->setSectionIndex( menu->getSections().size() - 1 ); //switch to the new section
				sync();
			}
			this->hw->ledOff();
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
			this->hw->ledOn();
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
			this->hw->ledOff();
		}
	}
}

void GMenu2X::deleteSection() {
	MessageBox mb(this, tr["All links in this section will be removed."] + "\n" + tr["Are you sure?"]);
	mb.setButton(CONFIRM, tr["Yes"]);
	mb.setButton(CANCEL,  tr["No"]);
	if (mb.exec() == CONFIRM) {
		this->hw->ledOn();
		if (rmtree(getWriteablePath() + "sections/" + menu->selSection())) {
			menu->deleteSelectedSection();
			sync();
		}
		this->hw->ledOff();
	}
}

void GMenu2X::setInputSpeed() {
	input.setInterval(180);
	input.setInterval(1000, SETTINGS);
	input.setInterval(1000, MENU);
	input.setInterval(1000, CONFIRM);
	input.setInterval(1500, POWER);
}
