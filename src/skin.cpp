#include <memory>
#include <stdarg.h>
#include <string.h>
#include <fstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>
#include <unistd.h>

#include "debug.h"
#include "skin.h"
#include "utilities.h"
#include "fileutils.h"
#include "constants.h"

#define sync() sync(); system("sync");

using std::ifstream;
using std::ofstream;
using std::string;

const int SKIN_VERSION = 1;

Skin::Skin(string const &prefix, int const &maxX, int const &maxY) {
    TRACE("enter - prefix : %s, maxX : %i, maxY : %i", prefix.c_str(),  maxX, maxY);

    this->assetsPrefix = prefix;
    this->maxX = maxX;
    this->maxY = maxY;

}

Skin::~Skin() {
    TRACE("~Skin");
}

vector<string> Skin::getSkins(string assetsPath) {

	string skinPath = assetsPath + SKIN_FOLDER;
    vector<string> result;
	TRACE("getSkins - searching for skins in : %s", skinPath.c_str());
	if (FileUtils::dirExists(skinPath)) {

		DIR *dirp;
		if ((dirp = opendir(skinPath.c_str())) == NULL) {
			ERROR("Error: Skin::getSkins - opendir(%s)", skinPath.c_str());
			return result;
		}

		struct stat st;
		struct dirent *dptr;
        string folder, fullPath;

		while ((dptr = readdir(dirp))) {
			folder = dptr->d_name;
			if (folder[0] == '.') continue;
			fullPath = skinPath + "/" + folder;
			int statRet = stat(fullPath.c_str(), &st);
			if (statRet == -1) {
				ERROR("Error: Skin::getSkins - Stat failed on '%s' with error '%s'", fullPath.c_str(), strerror(errno));
				continue;
			}
			if (S_ISDIR(st.st_mode)) {
				TRACE("adding directory : %s", folder.c_str());
				result.push_back(folder);
            }
        }
        closedir(dirp);
        sort(result.begin(), result.end(), case_less());
    }
    return result;
}

string Skin::toString() {

    vector<string> vec;
    vec.push_back("#      " + APP_NAME + " skin config file      #");
    vec.push_back("");
    vec.push_back("# ################################### #");
    vec.push_back("# lines starting with a # are ignored #");
    vec.push_back("# ################################### #");
    vec.push_back("");

    vec.push_back("");
    vec.push_back("# assets are searched for in the following order");
    vec.push_back("# - ./skins/{your_skin}/{your_asset}");
    vec.push_back("# - ./skins/Default/{your_asset}");
    vec.push_back("# - ./skins/{your_asset}");
    vec.push_back("");
    vec.push_back("# this means that you can rely on there being");
    vec.push_back("# - a font in the skin root folder");
    vec.push_back("# - basic button icons in the Defaut skin folder");
    vec.push_back("# - basic device icons in the Defaut skin folder");
    vec.push_back("# and you can reduce the size of your skin pack if you use them");
    vec.push_back("");

    vec.push_back("# the version of skin config file format");
    vec.push_back("");
    vec.push_back(string_format("version=%i", version));
    vec.push_back("");

    vec.push_back("# font sizes");
    vec.push_back("");
    vec.push_back(string_format("fontSize=%i", fontSize));
    vec.push_back(string_format("fontSizeTitle=%i", fontSizeTitle));
    vec.push_back(string_format("fontSizeSectionTitle=%i", fontSizeSectionTitle));
    vec.push_back("");

    vec.push_back("# number of rows and columns to show in launcher view");
    vec.push_back("");
    vec.push_back(string_format("linkRows=%i", numLinkRows));
    vec.push_back(string_format("linkCols=%i", numLinkCols));
    vec.push_back("");

    vec.push_back("# section title bar holds the section icons or name, etc");
    vec.push_back("# image is optional, and takes priority over simple coloured box");
    vec.push_back("");
    vec.push_back(string_format("sectionTitleBarSize=%i", sectionTitleBarSize));
    vec.push_back(string_format("sectionTitleBarImage=\"%s\"", sectionTitleBarImage.c_str()));
    vec.push_back("");
    vec.push_back("# controls where to position the section bar....");
    vec.push_back("# off, left, bottom, right, top as integer values from zero");
    vec.push_back("");
    vec.push_back(string_format("sectionBar=%i", sectionBar));
    vec.push_back("");
    vec.push_back("# toggles the section title bar between icon and text mode");
    vec.push_back("# text mode is only valid if section titlebar is on the top or bottom of the screen");
    vec.push_back("");
    vec.push_back(string_format("showSectionIcons=%i", showSectionIcons));
    vec.push_back("");

    vec.push_back("# section info bar holds the key commands in launcher view");
    vec.push_back("# it is only ever visible if section title bar is on the top or bottom of the screen");
    vec.push_back("# and if sectionInfoBarVisible=1");
    vec.push_back("");
    vec.push_back(string_format("sectionInfoBarSize=%i", sectionInfoBarSize));
    vec.push_back(string_format("sectionInfoBarImage=\"%s\"", sectionInfoBarImage.c_str()));
    vec.push_back(string_format("sectionInfoBarVisible=%i", sectionInfoBarVisible));
    vec.push_back("");

    vec.push_back("# menu title and info bar work in the same way as section bars, ");
    vec.push_back("# but are fixed in position, always top and bottom of the screen");
    vec.push_back("");
    vec.push_back(string_format("menuTitleBarHeight=%i", menuTitleBarHeight));
    vec.push_back(string_format("menuInfoBarHeight=%i", menuInfoBarHeight));
    vec.push_back("");

    vec.push_back("# how game previews look");
    vec.push_back("# game previews live in a subfolder called .previews, within a rom folder");
    vec.push_back("# -1 means fullscreen as a background");
    vec.push_back("# 0 means disable, and don't search folders");
    vec.push_back("# positive int means animate in from the right hand side for this many pixels");
    vec.push_back("");
    vec.push_back(string_format("previewWidth=%i", previewWidth));
    vec.push_back("");

    vec.push_back("# how to render a launch link");
    vec.push_back("# 0 = icons and text");
    vec.push_back("# 1 = icons only");
    vec.push_back("# 2 = text only");
    vec.push_back("");
    vec.push_back(string_format("linkDisplayMode=%i", linkDisplayMode));
    vec.push_back("");

    vec.push_back("# display the clock in the section title bar or not");
    vec.push_back("");
    vec.push_back(string_format("showClock=%i", showClock));
    vec.push_back("");

    vec.push_back("# display the loader on first boot, if there is one");
    vec.push_back("");
    vec.push_back(string_format("showLoader=%i", showLoader));
    vec.push_back("");

    vec.push_back("# display skin backgrounds for emulators etc");
    vec.push_back("# backdrop images can be .png or .jpg.");
    vec.push_back("# the most efficient place to put them is in a folder called {your_skin}/backdrops.");
    vec.push_back("# the filename should match one of these three");
    vec.push_back("# - the link name. eg. ReGBA");
    vec.push_back("# - the executable name. eg. regba. Not so good for opk's, as this is always opkrun");
    vec.push_back("# - the directory name. eg. gba");
    vec.push_back("# only turn this on if you really mean it, as it involves a lot of sdcard scans");
    vec.push_back("# which is why it defaults to off");
    vec.push_back("");
    vec.push_back(string_format("skinBackdrops=%i", skinBackdrops));
    vec.push_back("");

    vec.push_back("# if there is a highlight image supplied with the skin, ");
    vec.push_back("# it needs to be located at imgs/iconbg_on.png");
    vec.push_back("# then this flag declares if it is scaleable or not....");
    vec.push_back("# why ?");
    vec.push_back("# because of the limitations of SDL 1.2, software scaling kills the Alpha channel,");
    vec.push_back("# and so we take the colour of pixel (0, 0) AKA top left");
    vec.push_back("# and consider anything else that colour to be transparent");
    vec.push_back("# we also don't keep aspect ration in the scaling, ");
    vec.push_back("# because cell sizes change ratio with row and column count");
    vec.push_back("");
    vec.push_back(string_format("scaleableHighlightImage=%i", scaleableHighlightImage));
    vec.push_back("");

    vec.push_back("# the current selected wallpaper");
    vec.push_back("# when designing a skin, set the filename only, no path, ");
    vec.push_back("# the path is resolved at run time");
    vec.push_back("");
    vec.push_back(string_format("wallpaper=\"%s\"", wallpaper.c_str()));
    vec.push_back("");

    vec.push_back("# force skin aspects to grayscale");
    vec.push_back("# imagesToGrayscale covers wallpapers, previews, backdrops");
    vec.push_back("");
    vec.push_back(string_format("imagesToGrayscale=%i", imagesToGrayscale));
    vec.push_back(string_format("iconsToGrayscale=%i", iconsToGrayscale));
    vec.push_back("");

    vec.push_back("# colours section starts");
    vec.push_back("");
    vec.push_back(string_format("background=%s", rgbatostr(colours.background).c_str()));
    vec.push_back(string_format("listBg=%s", rgbatostr(colours.listBackground).c_str()));
    vec.push_back(string_format("titleBarBg=%s", rgbatostr(colours.titleBarBackground).c_str()));
    vec.push_back(string_format("infoBarBg=%s", rgbatostr(colours.infoBarBackground).c_str()));
    vec.push_back(string_format("selectionBg=%s", rgbatostr(colours.selectionBackground).c_str()));
    vec.push_back(string_format("messageBoxBg=%s", rgbatostr(colours.msgBoxBackground).c_str()));
    vec.push_back(string_format("messageBoxBorder=%s", rgbatostr(colours.msgBoxBorder).c_str()));
    vec.push_back(string_format("messageBoxSelection=%s", rgbatostr(colours.msgBoxSelection).c_str()));
    vec.push_back(string_format("font=%s", rgbatostr(colours.font).c_str()));
    vec.push_back(string_format("fontOutline=%s", rgbatostr(colours.fontOutline).c_str()));
    vec.push_back(string_format("fontAlt=%s", rgbatostr(colours.fontAlt).c_str()));
    vec.push_back(string_format("fontAltOutline=%s", rgbatostr(colours.fontAltOutline).c_str()));

    std::string s;
    for (const auto &piece : vec) s += (piece + "\n");
    return s;
}

bool Skin::remove() {
    string fileName = this->assetsPrefix + SKIN_FOLDER + "/" + this->name;
    return (0 ==unlink(fileName.c_str()));
}

bool Skin::save() {
    TRACE("enter");
    string fileName = this->assetsPrefix + SKIN_FOLDER + "/" + this->name + "/" + SKIN_FILE_NAME;
    TRACE("saving to : %s", fileName.c_str());

	ofstream config(fileName.c_str());
	if (config.is_open()) {
		config << this->toString();
		config.close();
		sync();
	}

    TRACE("exit");
    return true;
}

bool Skin::loadSkin(string name) {
    TRACE("loading skin : %s", name.c_str());
    this->name = name;
    this->reset();
    return this->fromFile();
}

vector<string> Skin::getWallpapers() {
    TRACE("enter");
    string path = this->currentSkinPath() + "/wallpapers";
    TRACE("searching for wallpapers in : %s", path.c_str());

    vector<string> results;

    if (!FileUtils::dirExists(path)) {
        TRACE("wallpaper directory diesn't exist : %s", path.c_str());
        return results;
    }

	DIR *dirp;
	if ((dirp = opendir(path.c_str())) == NULL) {
		ERROR("Error: couldn't open wallpaper dir : %s", path.c_str());
		return results;
	}

    vector<string> vfilter;
	split(vfilter, ".png,.jpg,.jpeg,.bmp", ",");

	string filepath, file;
	struct stat st;
	struct dirent *dptr;

	while ((dptr = readdir(dirp))) {
		file = dptr->d_name;
		if (file[0] == '.') continue;
		filepath = path + "/" + file;
		int statRet = stat(filepath.c_str(), &st);
		if (statRet == -1) {
			ERROR("Stat failed on '%s' with error '%s'", filepath.c_str(), strerror(errno));
			continue;
		}
		for (vector<string>::iterator it = vfilter.begin(); it != vfilter.end(); ++it) {
			if (vfilter.size() > 1 && it->length() == 0 && (int32_t)file.rfind(".") >= 0) continue;
			if (it->length() <= file.length()) {
				if (file.compare(file.length() - it->length(), it->length(), *it) == 0) {
					TRACE("adding wallpaper : %s", file.c_str());
					results.push_back(file);
					break;
				}
			}
		}
    }
	closedir(dirp);
	sort(results.begin(), results.end(), case_less());
    TRACE("exit - found %zu wallpapers for skin %s", results.size(), this->name.c_str());
    return results;
}

std::string Skin::currentSkinPath() {
    return this->assetsPrefix + SKIN_FOLDER + "/" + this->name;
}

string Skin::getSkinFilePath(const string &file) {
	TRACE("enter : %s", file.c_str());
	TRACE("prefix : %s, skin : %s", this->assetsPrefix.c_str(), this->name.c_str());
	string result = "";

	if (FileUtils::fileExists(this->currentSkinPath() + "/" + file)) {
		TRACE("found file in current skin");
		result = this->currentSkinPath() + "/" + file;
	} else if (FileUtils::fileExists(this->assetsPrefix + "skins/Default/" + file)) {
		TRACE("found file in default skin");
		result = this->assetsPrefix + "skins/Default/" + file;
	} else if (FileUtils::fileExists(this->assetsPrefix + "skins/" + file)) {
		TRACE("found file in root skin folder");
		result = this->assetsPrefix + "skins/" + file;
	} else {
		TRACE("didn't find file anywhere");
	}
	TRACE("exit : %s", result.c_str());
	return result;
}

/* Private methods */

void Skin::reset() {

    TRACE("enter");
    version = SKIN_VERSION;
    fontSize = 12;
    fontSizeTitle = 20;
    fontSizeSectionTitle = 20;
    numLinkRows = 6;
    numLinkCols = 1;
    sectionInfoBarSize = 16;
    sectionTitleBarSize = 40;
    menuInfoBarHeight = 16;
    menuTitleBarHeight = 40;
    previewWidth = 142;

    linkDisplayMode = LinkDisplayModes::ICON_AND_TEXT;
    showSectionIcons = true;
    sectionInfoBarVisible = false;
    showClock = true;
    showLoader = false;
    skinBackdrops = false;
    scaleableHighlightImage = true;
    sectionBar = SB_LEFT;
    wallpaper = "";
    menuInfoBarImage = "";
    menuTitleBarImage = "";
    sectionInfoBarImage = "";
    sectionTitleBarImage = "";

    iconsToGrayscale = false;
    imagesToGrayscale = false;

	TRACE("skinFontColors");
    colours.background = (RGBAColor){0,0,0,255};
    colours.titleBarBackground = (RGBAColor){255,255,255,130};
    colours.listBackground = (RGBAColor){255,255,255,0};
    colours.infoBarBackground = (RGBAColor){255,255,255,130};
    colours.selectionBackground = (RGBAColor){255,255,255,130};
    colours.msgBoxBackground = (RGBAColor){255,255,255,255};
    colours.msgBoxBorder = (RGBAColor){80,80,80,255};
    colours.msgBoxSelection = (RGBAColor){160,160,160,255};
    colours.font = (RGBAColor){255,255,255,255};
    colours.fontOutline = (RGBAColor){0,0,0,200};
    colours.fontAlt = (RGBAColor){253,1,252,0};
    colours.fontAltOutline = (RGBAColor){253,1,252,0};

    TRACE("exit");
    return;
}

void Skin::constrain() {

    evalIntConf( &this->version, SKIN_VERSION, 0, 100);
	evalIntConf( &this->menuTitleBarHeight, 40, 1, maxY);
    evalIntConf( &this->menuInfoBarHeight, 16, 1, maxY);
	evalIntConf( &this->sectionTitleBarSize, 40, 18, maxX);
	evalIntConf( &this->sectionInfoBarSize, 16, 1, maxY);
	evalIntConf( &this->previewWidth, 142, -1, maxX - 60);
	evalIntConf( &this->fontSize, 12, 6, 60);
	evalIntConf( &this->fontSizeTitle, 20, 6, 60);
    evalIntConf( &this->fontSizeSectionTitle, 30, 6, 60);
    evalIntConf( &this->showSectionIcons, 1, 0, 1);
    evalIntConf( &this->sectionInfoBarVisible, 0, 0, 1);
    evalIntConf( &this->scaleableHighlightImage, 0, 0, 1);
    evalIntConf( &this->skinBackdrops, 0, 0, 1);
    evalIntConf( &this->numLinkCols, 1, 1, 10);
    evalIntConf( &this->numLinkRows, 6, 1, 16);
    evalIntConf( (int)*(&this->linkDisplayMode), ICON_AND_TEXT, ICON_AND_TEXT, TEXT);
    evalIntConf( &this->scaleableHighlightImage, 1, 0, 1);

}

bool Skin::fromFile() {
    TRACE("enter");
    bool result = false;
    string skinPath = this->assetsPrefix + SKIN_FOLDER + "/" + this->name + "/";
    string fileName = skinPath + SKIN_FILE_NAME;
    TRACE("loading skin from : %s", fileName.c_str());

	if (FileUtils::fileExists(fileName)) {
		TRACE("skin file exists");
		ifstream skinconf(fileName.c_str(), std::ios_base::in);
		if (skinconf.is_open()) {
			string line;
			while (getline(skinconf, line, '\n')) {
                try {
                    line = trim(line);
                    if (0 == line.length()) continue;
                    if ('#' == line[0]) continue;
                    string::size_type pos = line.find("=");
                    if (string::npos == pos) continue;
                    
                    string name = trim(line.substr(0, pos));
                    string value = trim(line.substr(pos + 1,line.length()));

                    if (0 == value.length()) continue;
                    name = toLower(name);
                    TRACE("handling kvp - %s = %s", name.c_str(), value.c_str());

                    try {
                        if (name == "version") {
                            version = atoi(value.c_str());
                        } else if (name == "fontsize") {
                            this->fontSize = atoi(value.c_str());
                        } else if (name == "fontsizetitle") {
                            this->fontSizeTitle = atoi(value.c_str());
                        } else if (name == "fontsizesectiontitle") {
                            this->fontSizeSectionTitle = atoi(value.c_str());
                        } else if (name == "linkrows") {
                            numLinkRows = atoi(value.c_str());
                        } else if (name == "linkcols") {
                            numLinkCols = atoi(value.c_str());
                        } else if (name == "sectionbarsize") {
                            sectionTitleBarSize = atoi(value.c_str());
                        } else if (name == "sectiontitlebarsize") {
                            sectionTitleBarSize = atoi(value.c_str());
                        } else if (name == "sectiontitlebarimage") {
                            sectionTitleBarImage = stripQuotes(value);
                            if (!sectionTitleBarImage.empty() && sectionTitleBarImage == FileUtils::pathBaseName(sectionTitleBarImage)) {
                                sectionTitleBarImage = skinPath + "imgs/" + sectionTitleBarImage;
                            }
                            sectionTitleBarImage = trim(sectionTitleBarImage);
                        } else if (name == "sectioninfobarsize") {
                            sectionInfoBarSize = atoi(value.c_str());
                        } else if (name == "sectioninfobarimage") {
                            sectionInfoBarImage = stripQuotes(value);
                            if (!sectionInfoBarImage.empty() && sectionInfoBarImage == FileUtils::pathBaseName(sectionInfoBarImage)) {
                                sectionInfoBarImage = skinPath + "imgs/" + sectionInfoBarImage;
                            }
                            sectionInfoBarImage = trim(sectionInfoBarImage);
                        } else if (name == "bottombarheight") {
                            menuInfoBarHeight = atoi(value.c_str());
                        } else if (name == "topbarheight") {
                            menuTitleBarHeight = atoi(value.c_str());
                        } else if (name == "menuinfobarheight") {
                            menuInfoBarHeight = atoi(value.c_str());
                        } else if (name == "menuinfobarimage") {
                            menuInfoBarImage = stripQuotes(value);
                            if (!menuInfoBarImage.empty() && menuInfoBarImage == FileUtils::pathBaseName(menuInfoBarImage)) {
                                menuInfoBarImage = skinPath + "imgs/" + menuInfoBarImage;
                            }
                            menuInfoBarImage = trim(menuInfoBarImage);
                        } else if (name == "menutitlebarheight") {
                            menuTitleBarHeight = atoi(value.c_str());
                        } else if (name == "menutitlebarimage") {
                            menuTitleBarImage = stripQuotes(value);
                            if (!menuTitleBarImage.empty() && menuTitleBarImage == FileUtils::pathBaseName(menuTitleBarImage)) {
                                menuTitleBarImage = skinPath + "imgs/" + menuTitleBarImage;
                            }
                            menuTitleBarImage = trim(menuTitleBarImage);
                        } else if (name == "previewwidth") {
                            previewWidth = atoi(value.c_str());
                        } else if (name == "linkdisplaymode") {
                            linkDisplayMode = (LinkDisplayModes)atoi(value.c_str());
                        } else if (name == "showsectionicons") {
                            showSectionIcons = atoi(value.c_str());
                        } else if (name == "showclock") {
                            showClock = atoi(value.c_str());
                        } else if (name == "showloader") {
                            showLoader = atoi(value.c_str());
                        } else if (name == "sectioninfobarvisible") {
                            sectionInfoBarVisible = atoi(value.c_str());
                        } else if (name == "skinbackdrops") {
                            skinBackdrops = atoi(value.c_str());
                        } else if (name == "sectionbar") {
                            sectionBar = (SectionBar)atoi(value.c_str());
                        } else if (name == "wallpaper") {
                            wallpaper = stripQuotes(value);
                            if (!wallpaper.empty() && wallpaper == FileUtils::pathBaseName(wallpaper)) {
                                wallpaper = skinPath + "wallpapers/" + wallpaper;
                            }
                        } else if (name == "background") {
                            colours.background = strtorgba(value);
                        } else if (name == "topbarbg") {
                            colours.titleBarBackground = strtorgba(value);
                        } else if (name == "titlebarbg") {
                            colours.titleBarBackground = strtorgba(value);
                        } else if (name == "listbg") {
                            colours.listBackground = strtorgba(value);
                        } else if (name == "bottombarbg") {
                            colours.infoBarBackground = strtorgba(value);
                        } else if (name == "infobarbg") {
                            colours.infoBarBackground = strtorgba(value);
                        } else if (name == "selectionbg") {
                            colours.selectionBackground = strtorgba(value);
                        } else if (name == "messageboxbg") {
                            colours.msgBoxBackground = strtorgba(value);
                        } else if (name == "messageboxborder") {
                            colours.msgBoxBorder = strtorgba(value);
                        } else if (name == "messageboxselection") {
                            colours.msgBoxSelection = strtorgba(value);
                        } else if (name == "font") {
                            colours.font = strtorgba(value);
                        } else if (name == "fontoutline") {
                            colours.fontOutline = strtorgba(value);
                        } else if (name == "fontalt") {
                            colours.fontAlt = strtorgba(value);
                        } else if (name == "fontaltoutline") {
                            colours.fontAltOutline = strtorgba(value);
                        } else if (name == "iconstograyscale") {
                            this->iconsToGrayscale = atoi(value.c_str());
                        } else if (name == "imagestograyscale") {
                            this->imagesToGrayscale = atoi(value.c_str());
                        } else if (name == "scaleablehighlightimage") {
                            this->scaleableHighlightImage = atoi(value.c_str());
                        } else {
                            WARNING("unknown skin key : %s", name.c_str());
                        }
                    }
                    catch (int param) { 
                        ERROR("int exception : %i from <%s, %s>", 
                            param, name.c_str(), value.c_str()); }
                    catch (char *param) { 
                        ERROR("char exception : %s from <%s, %s>", 
                            param, name.c_str(), value.c_str()); }
                    catch (...) { 
                            ERROR("unknown error reading value from <%s, %s>", 
                                name.c_str(), value.c_str());
                    }
                } 
                catch (...) {
                    ERROR("Error reading skin config line : %s", line.c_str());
                }
            };
            skinconf.close();
            this->constrain();
            result = true;
        }
    }
    TRACE("exit");
    return result;
}
