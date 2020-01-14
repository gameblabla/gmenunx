#ifndef SKIN_H
#define SKIN_H

#include <string>
#include <vector>
#include "surface.h"

static const std::string SKIN_FILE_NAME = "skin.conf";
static const std::string SKIN_FOLDER = "skins";

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

    Skin(std::string const &prefix, int const &maxX, int const &maxY);
    ~Skin();

    void changePath(std::string const &prefix) { this->assetsPrefix = prefix; }
    static std::vector<std::string> getSkins(std::string assetsPath) ;

    std::string getSkinFilePath(const std::string &file);
    std::string currentSkinPath();
    bool loadSkin(std::string name);
    std::vector<std::string> getWallpapers();
    std::string toString();
    bool save();
    bool remove();

    std::string name;
    std::string sectionTitleBarImage;
    std::string sectionInfoBarImage;
    std::string menuTitleBarImage;
    std::string menuInfoBarImage;
    std::string wallpaper;

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
    int showLoader;

    LinkDisplayModes linkDisplayMode;

    int showSectionIcons;
    int showClock;
    int scaleableHighlightImage;
    int sectionInfoBarVisible;
    int skinBackdrops;
    SectionBar sectionBar;
    Colours colours;

private:

    std::string assetsPrefix;
    int maxX, maxY;

    void reset();
    bool fromFile();
    void constrain();

};

#endif