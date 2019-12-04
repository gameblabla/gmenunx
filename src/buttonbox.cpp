
#include "button.h"
#include "esoteric.h"

#include "buttonbox.h"

ButtonBox::ButtonBox(Esoteric *app) : app(app)
{
}

ButtonBox::~ButtonBox()
{
	for (ButtonList::const_iterator it = buttons.begin(); it != buttons.end(); ++it)
		delete *it;
}

void ButtonBox::add(Button *button)
{
	buttons.push_back(button);
}

void ButtonBox::paint(uint32_t posX)
{
	for (ButtonList::const_iterator it = buttons.begin(); it != buttons.end(); ++it)
		posX = app->ui->drawButton(*it, posX);
}

void ButtonBox::handleTS()
{
	for (ButtonList::iterator it = buttons.begin(); it != buttons.end(); ++it)
		(*it)->handleTS();
}
