#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <string>

class Esoteric;
class Surface;

class Dialog
{
	
public:
	Dialog(Esoteric *app);

protected:
	~Dialog();

	Surface *bg;
	void drawTitleIcon(const std::string &icon, Surface *s = NULL);
	void writeTitle(const std::string &title, Surface *s = NULL);
	void writeSubTitle(const std::string &subtitle, Surface *s = NULL);
	void drawTopBar(Surface *s, const std::string &title = {}, const std::string &description = {}, const std::string &icon = {});
	void drawBottomBar(Surface *s = NULL);

	Esoteric *app;
private:
	int iconWidth;

};

#endif
