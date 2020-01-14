#ifndef ICONBUTTON_H
#define ICONBUTTON_H

#include <string>
#include "button.h"

class Esoteric;
class Surface;

class IconButton : public Button {
protected:
	Esoteric *app;
	std::string icon, label;
	int labelPosition, labelMargin;
	uint16_t labelHAlign, labelVAlign;
	void recalcSize();
	SDL_Rect iconRect, labelRect;

	Surface *iconSurface;

	void updateSurfaces();

public:
	static const int DISP_RIGHT = 0;
	static const int DISP_LEFT = 1;
	static const int DISP_TOP = 2;
	static const int DISP_BOTTOM = 3;

	IconButton(Esoteric *app, const std::string &icon, const std::string &label = "");
	virtual ~IconButton() {};

	virtual void paint();
	virtual bool paintHover();

	virtual void setPosition(int x, int y);

	const std::string &getLabel();
	void setLabel(const std::string &label);
	void setLabelPosition(int pos, int margin);

	const std::string &getIcon();
	void setIcon(const std::string &icon);

	void setAction(ButtonAction action);
};

#endif
