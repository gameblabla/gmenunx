#ifndef SKIN_H
#define SKIN_H

#include <string>
#include <vector>
#include "surface.h"

using std::string;
using std::vector;

static const string SKIN_FILE_NAME = "skin.conf";
static const string SKIN_FOLDER = "skins";

class Skin {

public:

    struct Colours {
        RGBAColor background;
        RGBAColor listBackground;
        RGBAColor titleBarBackground;
        RGBAColor infoBarBackground;
        RGBAColor selectionBackground;
        RGBAColor msgBoxBackground;
        RGBAColor msgBoxBorder;
        RGBAColor msgBoxSelection;
        RGBAColor font;
        RGBAColor fontOutline;
        RGBAColor fontAlt;
        RGBAColor fontAltOutline;
    };

    enum SectionBar {
        SB_OFF,
        SB_LEFT,
        SB_BOTTOM,
        SB_RIGHT,
        SB_TOP,
    };


    enum LinkDisplayModes {
        ICON_AND_TEXT, 
        ICON, 
        TEXT
    };

    Skin(string const &prefix, int const &maxX, int const &maxY);
    ~Skin();

    static vector<string> getSkins(string assetsPath) ;

    std::string getSkinFilePath(const string &file);
    std::string currentSkinPath();
    bool loadSkin(string name);
    vector<string> getWallpapers();
    std::string toString();
    bool save();

    string name;
    string sectionTitleBarImage;
    string sectionInfoBarImage;
    string menuTitleBarImage;
    string menuInfoBarImage;
    string wallpaper;

    int fontSize;
    int fontSizeTitle;
    int fontSizeSectionTitle;
    int numLinkRows;
    int numLinkCols;
    int sectionTitleBarSize;
    int sectionInfoBarSize;
    int menuTitleBarHeight;
    int menuInfoBarHeight;
    int previewWidth;

    int imagesToGrayscale;
    int iconsToGrayscale;

    int version;

    LinkDisplayModes linkDisplayMode;

    int showSectionIcons;
    int showClock;
    int sectionInfoBarVisible;
    int skinBackdrops;
    SectionBar sectionBar;
    Colours colours;

private:

    string assetsPrefix;
    int maxX, maxY;

    void reset();
    bool fromFile();
    void constrain();

};

#endif