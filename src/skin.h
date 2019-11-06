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
        RGBAColor topBarBackground;
        RGBAColor listBackground;
        RGBAColor bottomBarBackground;
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

    bool loadSkin(string name);
    vector<string> getWallpapers();
    string toString();
    bool save();

    string name;

    int fontSize;
    int fontSizeTitle;
    int fontSizeSectionTitle;
    int numLinkRows;
    int numLinkCols;
    int sectionBarSize;
    int bottomBarHeight;
    int topBarHeight;
    int previewWidth;

    LinkDisplayModes linkDisplayMode;

    int showSectionIcons;
    int skinBackdrops;
    SectionBar sectionBar;
    string wallpaper;

    Colours colours;

private:

    string assetsPrefix;
    int maxX, maxY;

    void reset();
    bool fromFile();
    void constrain();

    std::string string_format(const std::string fmt_str, ...);

};

#endif