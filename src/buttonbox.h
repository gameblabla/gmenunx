#ifndef __BUTTONBOX_H__
#define __BUTTONBOX_H__

#include <vector>
#include <cstdint>

class Esoteric;
class Button;

class ButtonBox
{

public:
	ButtonBox(Esoteric *app);
	~ButtonBox();

	void add(Button *button);

	void paint(uint32_t posX);
	void handleTS();

private:
	typedef std::vector<Button*> ButtonList;

	ButtonList buttons;
	Esoteric *app;
};

#endif
