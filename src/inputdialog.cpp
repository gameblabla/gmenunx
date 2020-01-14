/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo   *
 *   massimiliano.torromeo@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "inputdialog.h"
#include "messagebox.h"
#include "debug.h"

InputDialog::InputDialog(Esoteric *app,
		Touchscreen &ts_, const std::string &text,
		const std::string &startvalue, const std::string &title, const std::string &icon)
	: Dialog(app)
	, ts(ts_) {

	if (title.empty()) {
		this->title = text;
		this->text = "";
	} else {
		this->title = title;
		this->text = text;
	}
	this->icon = "";
	if (!icon.empty() && (*app->sc)[icon] != NULL)
		this->icon = icon;

	this->input = startvalue;
	this->position = input.length();

	this->selCol = 0;
	this->selRow = 0;
	keyboard.resize(7);

	keyboard[0].push_back("qwertyuiop789");
	keyboard[0].push_back(",asdfghjkl456");
	keyboard[0].push_back(".zxcvbnm-0123");

	keyboard[1].push_back("QWERTYUIOP-+=");
	keyboard[1].push_back("@ASDFGHJKL'\"`");
	keyboard[1].push_back("#ZXCVBNM_:;/?");

	// keyboard[0].push_back("abcdefghijklm");
	// keyboard[0].push_back("nopqrstuvwxyz");
	// keyboard[0].push_back("0123456789.  ");

	// keyboard[1].push_back("ABCDEFGHIJKLM");
	// keyboard[1].push_back("NOPQRSTUVWXYZ");
	// keyboard[1].push_back("_\"'`.,:;!?   ");


	keyboard[2].push_back("¡¿*+-/\\&<=>|");
	keyboard[2].push_back("()[]{}@#$%^~");
	keyboard[2].push_back("_\"'`.,:;!?  ");


	keyboard[3].push_back("àáèéìíòóùúýäõ");
	keyboard[3].push_back("ëïöüÿâêîôûåãñ");
	keyboard[3].push_back("čďěľĺňôřŕšťůž");
	keyboard[3].push_back("ąćęłńśżź     ");

	keyboard[4].push_back("ÀÁÈÉÌÍÒÓÙÚÝÄÕ");
	keyboard[4].push_back("ËÏÖÜŸÂÊÎÔÛÅÃÑ");
	keyboard[4].push_back("ČĎĚĽĹŇÔŘŔŠŤŮŽ");
	keyboard[4].push_back("ĄĆĘŁŃŚŻŹ     ");


	keyboard[5].push_back("æçабвгдеёжзий ");
	keyboard[5].push_back("клмнопрстуфхцч");
	keyboard[5].push_back("шщъыьэюяøðßÐÞþ");

	keyboard[6].push_back("ÆÇАБВГДЕЁЖЗИЙ ");
	keyboard[6].push_back("КЛМНОПРСТУФХЦЧ");
	keyboard[6].push_back("ШЩЪЫЬЭЮЯØðßÐÞþ");

	setKeyboard(0);
}

void InputDialog::setKeyboard(int kb) {
	kb = constrain(kb, 0, keyboard.size() - 1);
	curKeyboard = kb;
	this->kb = &(keyboard[kb]);
	kbLength = this->kb->at(0).length();
	for (int x = 0, l = kbLength; x < l; x++)
		if (app->font->utf8Code(this->kb->at(0)[x])) {
			kbLength--;
			x++;
		}

	kbLeft = 160 - kbLength*KEY_WIDTH/2;
	kbWidth = kbLength * KEY_WIDTH + 3;
	kbHeight = (this->kb->size() + 1) * KEY_HEIGHT + 3;

	kbRect.x = kbLeft - 3;
	kbRect.y = KB_TOP - 2;
	kbRect.w = kbWidth;
	kbRect.h = kbHeight;
}

bool InputDialog::exec() {

	SDL_Rect box = {0, 60, 0, app->font->getHeight() + 4};

	uint32_t caretTick = 0, curTick;
	bool caretOn = true;

	uint32_t action;
	this->close = false;
	this->ok = true;

	drawTopBar(this->bg, title, text, icon);
	drawBottomBar(this->bg);

	app->ui->drawButton(this->bg, "a", app->tr["Press"],
	app->ui->drawButton(this->bg, "y", app->tr["Keys"],
	app->ui->drawButton(this->bg, "r", app->tr["Space"],
	app->ui->drawButton(this->bg, "l", app->tr["Backspace"]))));

	this->bg->box(app->listRect, app->skin->colours.listBackground);

	while (!close) {
		app->inputManager->setWakeUpInterval(500);

		this->bg->blit(app->screen, 0, 0);

		box.w = app->font->getTextWidth(input) + 18;
		box.x = 160 - box.w / 2;
		app->screen->box(box.x, box.y, box.w, box.h, app->skin->colours.selectionBackground);
		app->screen->rectangle(box.x, box.y, box.w, box.h, app->skin->colours.selectionBackground);

		std::pair<std::string, std::string> parts = this->inputParts();

		curTick = SDL_GetTicks();
		if (curTick - caretTick >= 600) {
			caretOn = !caretOn;
			caretTick = curTick;
		}

		app->screen->write(
				app->font, 
				parts.first, 
				box.x + 5, 
				box.y + box.h - 4, 
				VAlignBottom);

		int xoffset = app->font->getTextWidth(parts.first) + 2;

		if (caretOn) {
			app->screen->box(
				box.x + xoffset, 
				box.y + 3, 
				8, 
				box.h - 6, 
				app->skin->colours.selectionBackground);
		}

		app->screen->write(
				app->font, 
				parts.second, 
				box.x + xoffset + 8 + 2, 
				box.y + box.h - 4, 
				VAlignBottom);

		if (app->f200) ts.poll();
		action = drawVirtualKeyboard();
		app->screen->flip();

		bool inputAction = app->inputManager->update(true);
		if (!inputAction)
			continue;
		TRACE("input manager updated");
		if ( (*app->inputManager)[CANCEL] || (*app->inputManager)[MENU] ) action = ID_ACTION_CLOSE;
		else if ( (*app->inputManager)[SETTINGS] ) action = ID_ACTION_SAVE;
		else if ( (*app->inputManager)[UP]       ) action = ID_ACTION_UP;
		else if ( (*app->inputManager)[DOWN]     ) action = ID_ACTION_DOWN;
		else if ( (*app->inputManager)[LEFT]     ) action = ID_ACTION_LEFT;
		else if ( (*app->inputManager)[RIGHT]    ) action = ID_ACTION_RIGHT;
		else if ( (*app->inputManager)[CONFIRM]  ) action = ID_ACTION_SELECT;
		else if ( (*app->inputManager)[DEC]   ) action = ID_ACTION_KB_CHANGE;
		else if ( (*app->inputManager)[SECTION_PREV] ) action = ID_ACTION_BACKSPACE;
		else if ( (*app->inputManager)[SECTION_NEXT] ) action = ID_ACTION_SPACE;

		switch (action) {
			case ID_ACTION_SAVE:
				ok = true;
				close = true;
				break;
			case ID_ACTION_CLOSE:
				ok = false;
				close = true;
				break;
			case ID_ACTION_UP:
				selRow--;
				if (selRow == (int)kb->size() - 1) {
					if (selCol == 0) {
						selCol = 0;
					} else if (selCol == 1) {
						selCol = 4;
					} else if (selCol == 2) {
						selCol = 7;
					} else selCol = 10;
				} else if (selRow < 0) {
					selCol = (selCol / 3);
					if (selCol > 3) selCol = 3;
				}
				break;
			case ID_ACTION_DOWN:
				selRow++;
				if (selRow == (int)kb->size()) {
					if (selCol < 4) {
						selCol = 0;
					} else if (selCol < 7) {
						selCol = 1;
					} else if (selCol < 10) {
						selCol = 2;
					} else selCol = 3;
				} else if (selRow > (int)kb->size()) {
					selCol = (selCol * 3) + 1;
				}
				break;
			case ID_ACTION_LEFT:
				selCol--;
				break;
			case ID_ACTION_RIGHT:
				selCol++;
				break;
			case ID_ACTION_BACKSPACE:
				backspace();
				break;
			case ID_ACTION_SPACE:
				space();
				break;
			case ID_ACTION_KB_CHANGE:
				changeKeys();
				break;
			case ID_ACTION_SELECT:
				confirm();
				break;
		}
	}
	// app->input.setWakeUpInterval(0);

	return ok;
}

void InputDialog::backspace() {
	TRACE("enter : '%s'", this->input.c_str());
	std::pair<std::string, std::string> parts = this->inputParts();
	if (parts.first.empty())
		return;
	// check for utf8 characters
	bool utf8 = app->font->utf8Code(this->input[this->position - 2]);
	int subtract = utf8 ? 2 : 1;

	TRACE("deleting %s character from : '%s' at position : %i", (utf8 ? "unicode" : "non-unicode"), this->input.c_str(), this->position);
	TRACE("first : '%s', second : '%s'", parts.first.c_str(), parts.second.c_str());
	std::string result;
	result = parts.first.substr(0, parts.first.length() - subtract);
	result += parts.second;

	this->input = result;
	this->position -= subtract;
	TRACE("exit : '%s'", this->input.c_str());
}

void InputDialog::space() {
	this->insertCharacter(" ");
}

void InputDialog::insertCharacter(const std::string & character) {
	if (character.empty())
		return;
	TRACE("inserting character : '%s'", character.c_str());
	bool utf8 = app->font->utf8Code((int)character[0]);
	int offset = utf8 ? 2 : 1;
	std::string original = this->input;
	std::pair<std::string, std::string> parts = this->inputParts();
	std::string result = parts.first + character + parts.second;
	this->input = result;
	this->position += offset;
	TRACE("original : '%s', result : '%s'", original.c_str(), this->input.c_str());
}

std::pair<std::string, std::string> InputDialog::inputParts() {
	std::string front;
	std::string back;

	TRACE("splitting '%s' at position %i", this->input.c_str(), this->position);

	if (this->position > 0)
		front = this->input.substr(0, this->position);
	if (this->position < this->input.length())
		back = this->input.substr(this->position, this->input.length() - this->position);
	
	TRACE("front : %s, back : %s", front.c_str(), back.c_str());
	std::pair<std::string, std::string> result { front, back };
	return result;
}

void InputDialog::stepLeft() {
	this->position--;
	if (this->position < 0)
		this->position = 0;
}

void InputDialog::stepRight() {
	this->position++;
	if (this->position > this->input.length())
		this->position = this->input.length();
}

void InputDialog::confirm() {
	if (selRow == (int)kb->size()) {
		if (selCol == 0) {
			this->stepLeft();
		} else if (selCol == 1) {
			this->ok = false;
			this->close = true;
		} else if (selCol == 2) {
			this->ok = true;
			this->close = true;
		} else if (selCol == 3) {
			this->stepRight();
		}
		
	} else {
		bool utf8;
		int xc = 0;
		for (uint32_t x = 0; x < kb->at(selRow).length(); x++) {
			utf8 = app->font->utf8Code(kb->at(selRow)[x]);
			if (xc == selCol) {
				std::string character = kb->at(selRow).substr(x, utf8 ? 2 : 1);
				this->insertCharacter(character);
				//input += character;//kb->at(selRow).substr(x, utf8 ? 2 : 1);
				break;
			}
			if (utf8) x++;
			xc++;
		}
	}
}

void InputDialog::changeKeys() {
	if (curKeyboard == 6) setKeyboard(0);
	else setKeyboard(curKeyboard + 1);
}

int InputDialog::drawVirtualKeyboard() {
	int action = ID_NO_ACTION;

	//keyboard border
	app->screen->rectangle(kbRect, app->skin->colours.selectionBackground);

	// wrap handling
	if (selCol < 0) selCol = selRow == (int)kb->size() ? 3 : kbLength - 1;
	if (selCol >= (int)kbLength) selCol = 0;
	if (selRow < 0) selRow = kb->size();
	if (selRow > (int)kb->size()) selRow = 0;

	//selection
	if (selRow < (int)kb->size())
		app->screen->box(
			kbLeft + selCol * KEY_WIDTH - 1, 
			KB_TOP + selRow * KEY_HEIGHT, 
			KEY_WIDTH - 1, 
			KEY_HEIGHT - 2, 
			app->skin->colours.selectionBackground);
	else {
		if (selCol > 3) selCol = 0;
		if (selCol < 0) selCol = 3;
		app->screen->box(
			kbLeft + selCol * KEY_WIDTH * kbLength / 4 - 1, 
			KB_TOP + kb->size() * KEY_HEIGHT, 
			kbLength * KEY_WIDTH / 4 - 1, 
			KEY_HEIGHT - 1, 
			app->skin->colours.selectionBackground);

	}

	//keys
	for (uint32_t l = 0; l < kb->size(); l++) {
		std::string line = kb->at(l);
		for (uint32_t x = 0, xc = 0; x < line.length(); x++) {
			std::string charX;
			//utf8 characters
			if (app->font->utf8Code(line[x])) {
				charX = line.substr(x, 2);
				x++;
			} else {
				charX = line[x];
			}

			SDL_Rect re = { 
				kbLeft + xc * KEY_WIDTH - 1, 
				KB_TOP + l * KEY_HEIGHT, 
				KEY_WIDTH - 1, 
				KEY_HEIGHT - 2
			};

			//if ts on rect, change selection
			if (app->f200 && ts.pressed() && ts.inRect(re)) {
				selCol = xc;
				selRow = l;
			}

			app->screen->rectangle(re, app->skin->colours.selectionBackground);
			app->screen->write(app->font, charX, kbLeft + xc * KEY_WIDTH + KEY_WIDTH / 2 - 1, KB_TOP + l * KEY_HEIGHT + KEY_HEIGHT / 2 - 2, HAlignCenter | VAlignMiddle);
			xc++;
		}
	}

	int numOpts = 4;
	SDL_Rect re = {
		kbLeft - 1, 
		KB_TOP + kb->size() * KEY_HEIGHT, 
		kbLength * KEY_WIDTH / numOpts - 1, 
		KEY_HEIGHT - 1
	};
	for (int x = 0; x < numOpts; x++) {
		app->screen->rectangle(re, app->skin->colours.selectionBackground);
		std::string behaviour;
		switch (x) {
			case 0:
				behaviour = "<";
				if (app->f200 && ts.pressed() && ts.inRect(re)) {
					this->stepLeft();
				}
				break;
			case 1:
				behaviour = app->tr["Cancel"];
				if (app->f200 && ts.pressed() && ts.inRect(re)) {
					selCol = 0;
					selRow = kb->size();
				}
				break;
			case 2:
				behaviour = app->tr["Ok"];
				if (app->f200 && ts.pressed() && ts.inRect(re)) {
					selCol = 1;
					selRow = kb->size();
				}
				break;
			case 3:
				behaviour = ">";
				if (app->f200 && ts.pressed() && ts.inRect(re)) {
					this->stepLeft();
				}
				break;
			default:
				break;
		};

		int xpos = re.x + ((kbLength * KEY_WIDTH / numOpts) / 2);
		app->screen->write(
			app->font, 
			behaviour, 
			xpos, 
			KB_TOP + kb->size() * KEY_HEIGHT + KEY_HEIGHT / 2, 
			HAlignCenter | VAlignMiddle);

		re.x += kbLength * KEY_WIDTH / numOpts;

	}

	//if ts released
	if (app->f200 && ts.released() && ts.inRect(kbRect)) {
		action = ID_ACTION_SELECT;
	}

	return action;
}
