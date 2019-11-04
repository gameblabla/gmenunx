

#include <memory>
#include <stdarg.h>

#include <string>
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

#define sync() sync(); system("sync");

using std::ifstream;
using std::ofstream;
using std::string;

Skin::Skin(string const &prefix, int const &maxX, int const &maxY) {
    DEBUG("Skin::Skin - enter - prefix : %s, maxX : %i, maxY : %i", prefix.c_str(),  maxX, maxY);

    this->assetsPrefix = prefix;
    this->maxX = maxX;
    this->maxY = maxY;

}

Skin::~Skin() {
    TRACE("Skin::~Skin");
}

vector<string> Skin::getSkins(string assetsPath) {

	string skinPath = assetsPath + SKIN_FOLDER;
    vector<string> result;
	TRACE("GMenu2X::getSkins - searching for skins in : %s", skinPath.c_str());
	if (dirExists(skinPath)) {

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
				TRACE("Error: Skin::getSkins - adding directory : %s", folder.c_str());
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

    vec.push_back("# gmenunx skin config file");
    vec.push_back("# lines starting with a # are ignored");
    vec.push_back(string_format("fontSize=%i", fontSize));
    vec.push_back(string_format("fontSizeTitle=%i", fontSizeTitle));
    vec.push_back(string_format("fontSizeSectionTitle=%i", fontSizeSectionTitle));
    vec.push_back(string_format("linkRows=%i", numLinkRows));
    vec.push_back(string_format("linkCols=%i", numLinkCols));
    vec.push_back(string_format("sectionBarSize=%i", sectionBarSize));
    vec.push_back(string_format("bottomBarHeight=%i", bottomBarHeight));
    vec.push_back(string_format("topBarHeight=%i", topBarHeight));
    vec.push_back(string_format("previewWidth=%i", previewWidth));

    vec.push_back(string_format("showLinkIcons=%i", showLinkIcons));
    vec.push_back(string_format("showSectionIcons=%i", showSectionIcons));
    vec.push_back(string_format("skinBackdrops=%i", skinBackdrops));

    vec.push_back(string_format("sectionBar=%i", sectionBar));
    vec.push_back(string_format("wallpaper=\"%s\"", wallpaper.c_str()));

    vec.push_back("# colours section starts");
    vec.push_back(string_format("topBarBg=%s", rgbatostr(colours.topBarBackground).c_str()));
    vec.push_back(string_format("listBg=%s", rgbatostr(colours.listBackground).c_str()));
    vec.push_back(string_format("bottomBarBg=%s", rgbatostr(colours.bottomBarBackground).c_str()));
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
    
bool Skin::save() {
    TRACE("Skin::save - enter");
    string fileName = this->assetsPrefix + SKIN_FOLDER + "/" + this->name + "/" + SKIN_FILE_NAME;
    TRACE("Skin::save - saving to : %s", fileName.c_str());

	ofstream config(fileName.c_str());
	if (config.is_open()) {
		config << this->toString();
		config.close();
		sync();
	}

    TRACE("Skin::save - exit");
    return true;
}

bool Skin::loadSkin(string name) {
    TRACE("Skin::loadSkin - loading skin : %s", name.c_str());
    this->name = name;
    this->reset();
    return this->fromFile();
}

vector<string> Skin::getWallpapers() {
    TRACE("Skin::getWallpapers - enter");
    string path = this->assetsPrefix + SKIN_FOLDER + "/" + this->name + "/wallpapers";
    TRACE("Skin::getWallpapers - searching in : %s", path.c_str());

    vector<string> results;

	DIR *dirp;
	if ((dirp = opendir(path.c_str())) == NULL) {
		ERROR("Error: opendir(%s)", path.c_str());
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
					TRACE("Skin::getWallpapers - adding wallpaper : %s", file.c_str());
					results.push_back(file);
					break;
				}
			}
		}
    }
	closedir(dirp);
	sort(results.begin(), results.end(), case_less());
    TRACE("Skin::getWallpapers - exit - found %i wallpapers for skin %s", results.size(), this->name.c_str());
}

/* Private methods */

void Skin::reset() {

    TRACE("Skin::reset - enter");
    fontSize = 12;
    fontSizeTitle = 20;
    fontSizeSectionTitle = 30;
    numLinkRows = 6;
    numLinkCols = 1;
    sectionBarSize = 40;
    bottomBarHeight = 16;
    topBarHeight = 40;
    previewWidth = 142;

    showLinkIcons = true;
    showSectionIcons = true;
    skinBackdrops = false;
    sectionBar = SB_LEFT;
    wallpaper = "";

	TRACE("Skin::reset - skinFontColors");
    colours.topBarBackground = (RGBAColor){255,255,255,130};
    colours.listBackground = (RGBAColor){255,255,255,0};
    colours.bottomBarBackground = (RGBAColor){255,255,255,130};
    colours.selectionBackground = (RGBAColor){255,255,255,130};
    colours.msgBoxBackground = (RGBAColor){255,255,255,255};
    colours.msgBoxBorder = (RGBAColor){80,80,80,255};
    colours.msgBoxSelection = (RGBAColor){160,160,160,255};
    colours.font = (RGBAColor){255,255,255,255};
    colours.fontOutline = (RGBAColor){0,0,0,200};
    colours.fontAlt = (RGBAColor){253,1,252,0};
    colours.fontAltOutline = (RGBAColor){253,1,252,0};

    TRACE("Skin::reset - exit");
    return;
}

void Skin::constrain() {

	evalIntConf( &this->topBarHeight, 40, 1, maxY);
	evalIntConf( &this->sectionBarSize, 40, 18, maxX);
	evalIntConf( &this->bottomBarHeight, 16, 1, maxY);
	evalIntConf( &this->previewWidth, 142, 1, maxX);
	evalIntConf( &this->fontSize, 12, 6, 60);
	evalIntConf( &this->fontSizeTitle, 20, 6, 60);
    evalIntConf( &this->fontSizeSectionTitle, 30, 6, 60);
	evalIntConf( &this->showLinkIcons, 1, 0, 1);
    evalIntConf( &this->showSectionIcons, 1, 0, 1);

}

bool Skin::fromFile() {
    TRACE("Skin::fromFile - enter");
    bool result = false;
    string skinPath = this->assetsPrefix + SKIN_FOLDER + "/" + this->name + "/";
    string fileName = skinPath + SKIN_FILE_NAME;
    TRACE("Skin::fromFile - loading skin from : %s", fileName.c_str());

	if (fileExists(fileName)) {
		TRACE("Skin::fromFile - skin file exists");
		ifstream skinconf(fileName.c_str(), std::ios_base::in);
		if (skinconf.is_open()) {
			string line;
			while (getline(skinconf, line, '\n')) {
				line = trim(line);
                if (0 == line.length()) continue;
                if ('#' == line[0]) continue;
				string::size_type pos = line.find("=");
                if (string::npos == pos) continue;
                
				string name = trim(line.substr(0,pos));
				string value = trim(line.substr(pos+1,line.length()));

                if (0 == value.length()) continue;

                TRACE("Skin::fromFile - handling kvp - %s = %s", name.c_str(), value.c_str());

                if (name == "fontSize") {
                    this->fontSize = atoi(value.c_str());
                } else if (name == "fontSizeTitle") {
                    this->fontSizeTitle = atoi(value.c_str());
                } else if (name == "fontSizeSectionTitle") {
                    this->fontSizeSectionTitle = atoi(value.c_str());
                } else if (name == "linkRows") {
                    numLinkRows = atoi(value.c_str());
                } else if (name == "linkCols") {
                    numLinkCols = atoi(value.c_str());
                } else if (name == "sectionBarSize") {
                    sectionBarSize = atoi(value.c_str());
                } else if (name == "bottomBarHeight") {
                    bottomBarHeight = atoi(value.c_str());
                } else if (name == "topBarHeight") {
                    topBarHeight = atoi(value.c_str());
                } else if (name == "previewWidth") {
                    previewWidth = atoi(value.c_str());
                } else if (name == "showLinkIcons") {
                    showLinkIcons = atoi(value.c_str());
                } else if (name == "showSectionIcons") {
                    showSectionIcons = atoi(value.c_str());
                } else if (name == "skinBackdrops") {
                    skinBackdrops = atoi(value.c_str());
                } else if (name == "sectionBar") {
                    sectionBar = (SectionBar)atoi(value.c_str());
                } else if (name == "wallpaper") {
                    // handle quotes
                    if (value.at(0) == '"' && value.at(value.length() - 1) == '"') {
                        wallpaper = value.substr(1, value.length() - 2);
                    } else wallpaper = value;
                    if (wallpaper == base_name(wallpaper)) {
                        wallpaper = skinPath + "wallpapers/" + wallpaper;
                    }
                } else if (name == "topBarBg") {
                    colours.topBarBackground = strtorgba(value);
                } else if (name == "listBg") {
                    colours.listBackground = strtorgba(value);
                } else if (name == "bottomBarBg") {
                    colours.bottomBarBackground = strtorgba(value);
                } else if (name == "selectionBg") {
                    colours.selectionBackground = strtorgba(value);
                } else if (name == "messageBoxBg") {
                    colours.msgBoxBackground = strtorgba(value);
                } else if (name == "messageBoxBorder") {
                    colours.msgBoxBorder = strtorgba(value);
                } else if (name == "messageBoxSelection") {
                    colours.msgBoxSelection = strtorgba(value);
                } else if (name == "font") {
                    colours.font = strtorgba(value);
                } else if (name == "fontOutline") {
                    colours.fontOutline = strtorgba(value);
                } else if (name == "fontAlt") {
                    colours.fontAlt = strtorgba(value);
                } else if (name == "fontAltOutline") {
                    colours.fontAltOutline = strtorgba(value);
                } else {
                    WARNING("Skin::fromFile - unknown key : %s", name.c_str());
                }

            };
            skinconf.close();
            this->constrain();
            result = true;
        }
    }
    TRACE("Skin::fromFile - exit");
    return result;
}

std::string Skin::string_format(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}