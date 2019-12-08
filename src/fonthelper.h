#ifndef FONTHELPER_H
#define FONTHELPER_H

#include "surface.h"

#include <string>
#include <vector>
#include <SDL_ttf.h>

#ifdef _WIN32
    typedef unsigned int uint32_t;
#endif

class FontHelper {

private:

	int height, halfHeight, fontSize;
	TTF_Font *font, *fontOutline;
	RGBAColor textColor, outlineColor;
	std::string fontName;

public:
	FontHelper(const std::string &fontName, int size, RGBAColor textColor = (RGBAColor){255,255,255}, RGBAColor outlineColor = (RGBAColor){5,5,5});
	~FontHelper();

	bool utf8Code(uint8_t c);

	void write(Surface *surface, const std::string &text, int x, int y, RGBAColor fgColor, RGBAColor bgColor);
	void write(Surface *surface, const std::string& text, int x, int y, const uint8_t align = HAlignLeft | VAlignTop);
	void write(Surface *surface, const std::string& text, int x, int y, const uint8_t align, RGBAColor fgColor, RGBAColor bgColor);

	void write(Surface *surface, std::vector<std::string> *text, int x, int y, const uint8_t align, RGBAColor fgColor, RGBAColor bgColor);
	void write(Surface *surface, std::vector<std::string> *text, int x, int y, const uint8_t align = HAlignLeft | VAlignTop);

	uint32_t getLineWidth(const std::string &text);
	uint32_t getTextWidth(const std::string &text);
	int getTextHeight(const std::string &text);
	uint32_t getTextWidth(std::vector<std::string> *text);
	
	uint32_t getHeight() { return height; };
	uint32_t getHalfHeight() { return halfHeight; };
	uint32_t getSize() { return fontSize; }

	void loadFont(const std::string &fontName, int fontSize);

	FontHelper *setSize(const int size);
	FontHelper *setColor(RGBAColor color);
	FontHelper *setOutlineColor(RGBAColor color);
};

#endif /* FONTHELPER_H */
