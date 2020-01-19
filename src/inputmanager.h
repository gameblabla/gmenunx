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
#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <string>
#include <list>

#include "managers/screenmanager.h"
#include "managers/powermanager.h"

enum actions {
	UP = 0, 
	DOWN = 1, 
	LEFT = 2, 
	RIGHT = 3,
	CONFIRM = 4, 
	CANCEL = 5, 
	MANUAL = 6, 
	MODIFIER = 7,
	SECTION_PREV = 8, 
	SECTION_NEXT = 9,
	INC = 10, 
	DEC = 11,
	PAGEUP = 12, 
	PAGEDOWN = 13,
	SETTINGS = 14, 
	MENU = 15,
	VOLUP = 16, 
	VOLDOWN = 17,
  	BACKLIGHT = 18, 
	POWER = 19, 
	QUIT = 20, 
	NOOP = 21
};

typedef struct {
	int type;
	uint32_t num;
	int value;
	int treshold;
	std::vector<int> combo;
} InputMap;

typedef std::vector<InputMap> MappingList;

typedef struct {
	bool active;
	int interval;
	long last;
	MappingList maplist;
	SDL_TimerID timer = 0;
} InputManagerAction;

/**
Manages all input peripherals
@author Massimiliano Torromeo <massimiliano.torromeo@gmail.com>
*/
class InputManager {
private:

	bool powerSet_;
	bool readStream(std::istream & input);
	InputMap getInputMapping(int action);
	SDL_TimerID wakeUpTimer;

	std::vector <SDL_Joystick*> joysticks;
	std::vector <InputManagerAction> actions;
	std::list <int> allMappedKeys;

	ScreenManager& screenManager;
	PowerManager& powerManager;

public:
	static const int MAPPING_TYPE_UNDEFINED = -1;
	static const int MAPPING_TYPE_BUTTON = 0;
	static const int MAPPING_TYPE_AXIS = 1;
	static const int MAPPING_TYPE_KEYPRESS = 2;
	static const int MAPPING_TYPE_KEYCOMBO = 3;

	static const int INPUT_KEY_REPEAT_DELAY = 250;

	static const int SDL_WAKEUPEVENT = SDL_USEREVENT + 1;
	static const int SDL_NOOPEVENT = SDL_WAKEUPEVENT + 1;

	InputManager(ScreenManager& screenManager, PowerManager& powerManager);
	~InputManager();
	void init(const std::string &conffile, const int &repeatRate = 50);
	void initJoysticks();
	bool readConfFile(const std::string &conffile = "input.conf");

	bool update(bool wait = true);
	bool combo();
	void dropEvents();
	int count();
	void setActionsCount(int count);
	void setInterval(int ms, int action = -1);
	void noop();
	bool &operator[](int action);
	bool isActive(int action);
	bool isActionKey(int action, int key);
	bool isKeyCombo(const std::vector<int> & comboActions);
	bool isOnlyActive(int action);

	void setButtonRepeat(const int &repeatRate);

};

#endif
