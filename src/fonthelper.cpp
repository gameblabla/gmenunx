#include <string>

#include "fonthelper.h"
#include "utilities.h"
#include "stringutils.h"
#include "debug.h"

FontHelper::FontHelper(const std::string &fontName, int fontSize, RGBAColor textColor, RGBAColor outlineColor)
	: fontName(fontName),
	  fontSize(fontSize),
	  textColor(textColor),
	  outlineColor(outlineColor) {

	this->font = NULL;
	this->fontOutline = NULL;
	loadFont(fontName, fontSize);
}

FontHelper::~FontHelper() {
	TRACE("enter");
	if (TTF_WasInit()) {
		if (NULL != this->font) {
			TRACE("closing font");
			TTF_CloseFont(this->font);
		}
		if (NULL != this->fontOutline) {
			TRACE("closing fontOutline");
			TTF_CloseFont(this->fontOutline);
		}
	}
	this->font = NULL;
	this->fontOutline = NULL;
	TRACE("exit");
}

void FontHelper::loadFont(const std::string &fontName, int fontSize) {
	TRACE("enter");
	if (!TTF_WasInit()) {
		TRACE("Initializing font");
		if (TTF_Init() == -1) {
			ERROR("TTF_Init: %s", TTF_GetError());
			exit(2);
		}
	}
	this->font = TTF_OpenFont(fontName.c_str(), fontSize);
	if (NULL == this->font) {
		ERROR("TTF_OpenFont %s: %s", fontName.c_str(), TTF_GetError());
		exit(2);
	}
	this->fontOutline = TTF_OpenFont(fontName.c_str(), fontSize);
	if (NULL == this->fontOutline) {
		ERROR("TTF_OpenFont %s: %s", fontName.c_str(), TTF_GetError());
		exit(2);
	}
	TTF_SetFontHinting(this->font, TTF_HINTING_LIGHT);
	TTF_SetFontHinting(this->fontOutline, TTF_HINTING_LIGHT);
	TTF_SetFontOutline(this->fontOutline, 1);
	this->height = 0;
	// Get maximum line height with a sample text
	TTF_SizeUTF8(this->font, "AZ0987654321", NULL, &this->height);
	this->halfHeight = this->height / 2;
	TRACE("exit");
}

bool FontHelper::utf8Code(uint8_t c) {
	return (c >= 194 && c <= 198) || c == 208 || c == 209;
}

FontHelper *FontHelper::setSize(const int size) {
	if (this->fontSize == size) return this;
	TTF_CloseFont(font);
	TTF_CloseFont(fontOutline);
	this->fontSize = size;
	loadFont(this->fontName, this->fontSize);
	return this;
}

FontHelper *FontHelper::setColor(RGBAColor color) {
	this->textColor = color;
	return this;
}

FontHelper *FontHelper::setOutlineColor(RGBAColor color) {
	this->outlineColor = color;
	return this;
}

uint32_t FontHelper::getLineWidthSafe(const std::string &text) {
	std::string localText;
	// use the most common letter
	localText.append(text.length(), 'e');
	return this->getLineWidth(localText);
}

uint32_t FontHelper::getLineWidth(const std::string &text) {
	//TRACE("enter : %s", text.c_str());
	int width = -1;
	try {
		if (text.empty())
			return 0;
		if (!this->fontOutline) {
			ERROR("Calling getLineWidth without a valid fontOutline");
			return -1;
		} else if (0 != TTF_SizeUTF8(this->fontOutline, text.c_str(), &width, NULL)) {
			ERROR("TTF_SizeUTF8 error : %s", TTF_GetError());
			return -2;
		}
	} 
	catch(std::exception& e) { 
		ERROR("Exception : %s", e.what());
		return -3;
	}
	catch (...) {
		WARNING("error getting line width for : %s", text.c_str());
		WARNING("sdl ttf error : %s", TTF_GetError());
		return -4;
	}
	//TRACE("exit : %s = %i px", text.c_str(), width);
	return width;
}

uint32_t FontHelper::getTextWidth(const std::string &text) {
	if (text.find("\n", 0) != std::string::npos) {
		std::vector<std::string> textArr;
		StringUtils::split(textArr, text, "\n");
		return getTextWidth(&textArr);
	} else
		return getLineWidth(text);
}

uint32_t FontHelper::getTextWidth(std::vector<std::string> *text) {
	int w = 0;
	for (uint32_t i = 0; i < text->size(); i++)
		w = max( getLineWidth(text->at(i)), w );
	return w;
}

int FontHelper::getTextHeight(const std::string &text) {
	std::vector<std::string> textArr;
	StringUtils::split(textArr,text,"\n");
	return textArr.size();
}

void FontHelper::write(Surface *surface, std::vector<std::string> *text, int x, int y, const uint8_t align) {
	write(surface, text, x, y, align, textColor, outlineColor);
}

void FontHelper::write(Surface *surface, std::vector<std::string> *text, int x, int y, const uint8_t align, RGBAColor fgColor, RGBAColor bgColor) {
	if (align & VAlignMiddle) {
		y -= getHalfHeight() * text->size();
	} else if (align & VAlignBottom) {
		y -= getHeight() * text->size();
	}

	for (uint32_t i = 0; i < text->size(); i++) {
		int ix = x;
		if (align & HAlignCenter) {
			ix -= getTextWidth(text->at(i)) / 2;
		} else if (align & HAlignRight) {
			ix -= getTextWidth(text->at(i));
		}
		write(surface, text->at(i), ix, y + i * getHeight(), fgColor, bgColor);
	}
}

void FontHelper::write(Surface* surface, const std::string &text, int x, int y, const uint8_t align, RGBAColor fgColor, RGBAColor bgColor) {
	if (text.find("\n", 0) != std::string::npos) {
		std::vector<std::string> textArr;
		StringUtils::split(textArr,text, "\n");
		write(surface, &textArr, x, y, align, fgColor, bgColor);
		return;
	}

	if (align & HAlignCenter) {
		x -= getTextWidth(text)/2;
	} else if (align & HAlignRight) {
		x -= getTextWidth(text);
	}

	if (align & VAlignMiddle) {
		y -= getHalfHeight();
	} else if (align & VAlignBottom) {
		y -= getHeight();
	}

	write(surface, text, x, y, fgColor, bgColor);
}

void FontHelper::write(Surface *surface, const std::string &text, int x, int y, const uint8_t align) {
	write(surface, text, x, y, align, textColor, outlineColor);
}

void FontHelper::write(Surface *surface, const std::string &text, int x, int y, RGBAColor fgColor, RGBAColor bgColor) {
	if (text.empty()) return;

	if (bgColor.a > 0){
		Surface bg;
		bg.raw = TTF_RenderUTF8_Blended(fontOutline, text.c_str(), rgbatosdl(bgColor));
		bg.setAlpha(bgColor.a);
		bg.blit(surface, x - TTF_GetFontOutline(fontOutline), y - TTF_GetFontOutline(fontOutline));
	}

	if (fgColor.a > 0){
		Surface fg;
		fg.raw = TTF_RenderUTF8_Blended(font, text.c_str(), rgbatosdl(fgColor));
		fg.setAlpha(fgColor.a);
		fg.blit(surface, x, y);
	}
}
