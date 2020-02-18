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
/*	RS97 Key Codes. 2018, pingflood
	BUTTON     GMENU          SDL             NUMERIC
	-------------------------------------------------
	X          MODIFIER       SDLK_SPACE      32
	A          CONFIRM        SDLK_LCTRL      306
	B          CANCEL         SDLK_LALT       308
	Y          MANUAL         SDLK_LSHIFT     304
	L          SECTION_PREV   SDLK_TAB        9
	R          SECTION_NEXT   SDLK_BACKSPACE  8
	START      SETTINGS       SDLK_RETURN     13
	SELECT     MENU)          SDLK_ESCAPE     27
	BACKLIGHT  BACKLIGHT      SDLK_3          51
	POWER      POWER          SDLK_END        279
	
	rg-350 key codes
	
	https://wiki.libsdl.org/SDLKeycodeLookup
	
	D-pad up        KEY_UP,
	D-pad down      KEY_DOWN,
	D-pad left      KEY_LEFT,
	D-pad right     KEY_RIGHT,
	A               KEY_LEFTCTRL,		306
	B               KEY_LEFTALT,		308
	X               KEY_SPACE,			32
	Y               KEY_LEFTSHIFT,		304
	L1              KEY_TAB,			9
	R1              KEY_BACKSPACE,		8
	L2              KEY_PAGEUP,			280
	R2              KEY_PAGEDOWN,		281
	L3              KEY_KPSLASH,
	R3              KEY_KPDOT,
	START           KEY_ENTER,
	SELECT          KEY_ESC,
	POWER           KEY_POWER,			320 (SEND KEY_HOME on press, 278)
	VOL_UP          KEY_VOLUMEUP,
	VOL_DOWN        KEY_VOLUMEDOWN,

	Joysticks
	Axis 0          Left Joy X
	Axis 1          Left Joy Y
	Axis 2          Right Joy X
	Alis 3          Right Joy Y

	Devices
	Backlight       pwm1
	Rumble          pwm4
	Power LED       JZ_GPIO_PORTB(30)

	# pg2 changes L2 and R2

	L2 SDLK_RSHIFT		= 303,
	R2 SDLK_RALT		= 307,
	
*/

#include <iostream>
#include <fstream>

#include "debug.h"
#include "inputmanager.h"
#include "managers/screenmanager.h"
#include "managers/powermanager.h"
#include "fileutils.h"
#include "stringutils.h"

InputManager::InputManager(ScreenManager& screenManager, PowerManager& powerManager) : 
	screenManager(screenManager), 
	powerManager(powerManager) {

	this->powerSet_ = false;
	wakeUpTimer = NULL;
}

InputManager::~InputManager() {
	TRACE("enter");
	for (uint32_t x = 0; x < joysticks.size(); x++) {
		if (SDL_JoystickOpened(x))
			SDL_JoystickClose(joysticks[x]);
	}
	TRACE("exit");
}

void InputManager::init(const std::string &conffile, const int &repeatRate) {
	TRACE("init started");
	this->initJoysticks();
	if (!this->readConfFile(conffile)) {
		ERROR("InputManager initialization from config file failed.");
	}
	this->setButtonRepeat(repeatRate);
	TRACE("init completed");
}

void InputManager::initJoysticks() {
	TRACE("initJoysticks started");
	SDL_Init(SDL_INIT_JOYSTICK);
	SDL_JoystickEventState(SDL_ENABLE);
	
	int numJoy = SDL_NumJoysticks();
	INFO("%d joysticks found", numJoy);
	for (int x = 0; x < numJoy; x++) {
		SDL_Joystick *joy = SDL_JoystickOpen(x);
		if (joy) {
			INFO("Initialized joystick: '%s'", SDL_JoystickName(x));
			joysticks.push_back(joy);
		}
		else WARNING("Failed to initialize joystick: %i", x);
	}
	TRACE("initJoysticks completed");
}

bool InputManager::readConfFile(const std::string &conffile) {
	TRACE("enter : %s", conffile.c_str());
	setActionsCount(NOOP + 1); // track the highest in the enum

	if (!FileUtils::fileExists(conffile)) {
		ERROR("File not found: %s", conffile.c_str());
		return false;
	}

	std::ifstream inf(conffile.c_str(), std::ios_base::in);
	if (!inf.is_open()) {
		ERROR("Could not open %s", conffile.c_str());
		return false;
	}

	bool result = this->readStream(inf);
	TRACE("close stream");
	inf.close();
	TRACE("exit : %i", result);
	return result;
}

bool InputManager::readStream(std::istream & input) {
	int action, linenum = 0;
	std::string line, name, value;
	std::string::size_type pos;
	std::vector<std::string> values;

	while (std::getline(input, line, '\n')) {
		TRACE("reading : %s", line.c_str());
		linenum++;
		line = StringUtils::fullTrim(line);
		if (0 == line.length())
			continue;
		if ('#' == line[0])
			continue;

		pos = line.find("=");
		if (std::string::npos == pos)
			continue;

		name = StringUtils::trim(line.substr(0,pos));
		value = StringUtils::trim(line.substr(pos + 1,line.length()));

		if (name == "up")                action = UP;
		else if (name == "down")         action = DOWN;
		else if (name == "left")         action = LEFT;
		else if (name == "right")        action = RIGHT;
		else if (name == "modifier")     action = MODIFIER;
		else if (name == "confirm")      action = CONFIRM;
		else if (name == "cancel")       action = CANCEL;
		else if (name == "manual")       action = MANUAL;
		else if (name == "dec")          action = DEC;
		else if (name == "inc")          action = INC;
		else if (name == "section_prev") action = SECTION_PREV;
		else if (name == "section_next") action = SECTION_NEXT;
		else if (name == "pageup")       action = PAGEUP;
		else if (name == "pagedown")     action = PAGEDOWN;
		else if (name == "settings")     action = SETTINGS;
		else if (name == "menu")         action = MENU;
		else if (name == "volup")        action = VOLUP;
		else if (name == "voldown")      action = VOLDOWN;
		else if (name == "backlight")    action = BACKLIGHT;
		else if (name == "power")        action = POWER;
		else if (name == "quit")         action = QUIT;
		else if (name == "speaker") {}
		else {
			ERROR("%d Unknown action '%s'.", linenum, name.c_str());
			continue;
		}

		StringUtils::split(values, value, ",");
		if (values.size() >= 2) {
			if (values[0] == "joystickbutton" && values.size() == 3) {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_BUTTON;
				map.num = atoi(values[1].c_str());
				map.value = atoi(values[2].c_str());
				map.treshold = 0;
				this->actions[action].maplist.push_back(map);
			} else if (values[0] == "joystickaxis" && values.size() == 4) {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_AXIS;
				map.num = atoi(values[1].c_str());
				map.value = atoi(values[2].c_str());
				map.treshold = atoi(values[3].c_str());
				this->actions[action].maplist.push_back(map);
			} else if (values[0] == "keyboard") {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_KEYPRESS;
				map.value = atoi(values[1].c_str());
				this->actions[action].maplist.push_back(map);
				this->allMappedKeys.push_back(map.value);
			} else if (values[0] == "combo") {
				TRACE("got a combo :: '%s'", value.c_str());
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_KEYCOMBO;
				for (int x = 1; x < values.size(); x++) {
					int keyCode = atoi(values[x].c_str());
					TRACE("adding combo key value : %i", keyCode);
					map.combo.push_back(keyCode);
				}
				this->actions[action].maplist.push_back(map);
			} else {
				ERROR("%d Invalid syntax or unsupported mapping type '%s'.", linenum, value.c_str());
				return false;
			}
		} else {
			ERROR("%d Every definition must have at least 2 values (%s).", linenum, value.c_str());
			return false;
		}
	}
	this->allMappedKeys.unique();
	return true;
}

void InputManager::setActionsCount(int count) {
	actions.clear();
	for (int x = 0; x < count; x++) {
		InputManagerAction action;
		action.active = false;
		action.interval = 0;
		// action.last = 0;
		action.timer = NULL;
		actions.push_back(action);
	}
}

bool InputManager::update(bool wait) {
	//TRACE("update started : wait = %i", wait);

	bool anyactions = false;
	SDL_JoystickUpdate();
	SDL_Event event;

	if (wait) {
		SDL_WaitEvent(&event);
		//TRACE("waiting is over, we got an event");
		if (screenManager.isAsleep() && SDL_NOOPEVENT != event.type && SDL_WAKEUPEVENT != event.type && SDL_JOYAXISMOTION != event.type) {
			TRACE("We're asleep, so we're just going to wake up and eat the button press event");
			/*
				// let's dump out what we can and isolate the wake up event type
				switch(event.type) {
                    case SDL_ACTIVEEVENT:
                        if (event.active.state & SDL_APPMOUSEFOCUS) {
                                if (event.active.gain) {
                                        TRACE("event.type test - Mouse focus gained\n");
                                } else {
                                        TRACE("event.type test - Mouse focus lost\n");
                                }
                        }
                        if (event.active.state & SDL_APPINPUTFOCUS) {
                                if (event.active.gain) {
                                        TRACE("event.type test - Input focus gained\n");
                                } else {
                                        TRACE("event.type test - Input focus lost\n");
                                }
                        }
                        if (event.active.state & SDL_APPACTIVE) {
                                if (event.active.gain) {
                                        TRACE("event.type test - Application restored\n");
                                } else {
                                        TRACE("event.type test - Application iconified\n");
                                }
                        }
                        break;
                    case SDL_KEYDOWN:
                        TRACE("event.type test - Key pressed:\n");
                        TRACE("event.type test -        SDL sim: %i\n",event.key.keysym.sym);
                        TRACE("event.type test -        modifiers: %i\n",event.key.keysym.mod);
                        TRACE("event.type test -        unicode: %i (if enabled with SDL_EnableUNICODE)\n",\
                                        event.key.keysym.unicode);
                        break;
                    case SDL_KEYUP:
                        TRACE("event.type test - Key released:\n");
                        TRACE("event.type test -        SDL sim: %i\n",event.key.keysym.sym);
                        TRACE("event.type test -        modifiers: %i\n",event.key.keysym.mod);
                        TRACE("event.type test -        unicode: %i (if enabled with SDL_EnableUNICODE)\n",\
                                        event.key.keysym.unicode);
                        break;
                    case SDL_MOUSEMOTION:
                        TRACE("event.type test - Mouse moved to: (%i,%i) ", event.motion.x, event.motion.y);
                        TRACE("event.type test - change: (%i,%i)\n", event.motion.xrel, event.motion.yrel);
                        TRACE("event.type test -        button state: %i\n",event.motion.state);
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        TRACE("event.type test - Mouse button %i ",event.button.button);
                        TRACE("event.type test - pressed with mouse at (%i,%i)\n",event.button.x,event.button.y);
                        break;
                    case SDL_MOUSEBUTTONUP:
                        TRACE("event.type test - Mouse button %i ",event.button.button);
                        TRACE("event.type test - released with mouse at (%i,%i)\n",event.button.x,event.button.y);
                        break;
                    case SDL_JOYAXISMOTION:
                        TRACE("event.type test - Joystick axis %i ",event.jaxis.axis);
                        TRACE("event.type test - on joystick %i ", event.jaxis.which);
                        TRACE("event.type test - moved to %i\n", event.jaxis.value);
                        break;
                    case SDL_JOYBALLMOTION:
                        TRACE("event.type test - Trackball axis %i ",event.jball.ball);
                        TRACE("event.type test - on joystick %i ", event.jball.which);
                        TRACE("event.type test - moved to (%i,%i)\n", event.jball.xrel, event.jball.yrel);
                        break;
                    case SDL_JOYHATMOTION:
                        TRACE("event.type test - Hat axis %i ",event.jhat.hat);
                        TRACE("event.type test - on joystick %i ", event.jhat.which);
                        TRACE("event.type test - moved to %i\n", event.jhat.value);
                        break;
                    case SDL_JOYBUTTONDOWN:
                        TRACE("event.type test - Joystick button %i ",event.jbutton.button);
                        TRACE("event.type test - on joystick %i ", event.jbutton.which);
                        TRACE("event.type test - pressed\n");
                        break;
                    case SDL_JOYBUTTONUP:
                        TRACE("event.type test - Joystick button %i ",event.jbutton.button);
                        TRACE("event.type test - on joystick %i ", event.jbutton.which);
                        TRACE("event.type test - released\n");
                        break;
                    case SDL_VIDEORESIZE:
                        TRACE("event.type test - Window resized to: (%i,%i)\n",event.resize.w, event.resize.h);
                        break;
                    case SDL_VIDEOEXPOSE:
                        TRACE("event.type test - Window exposed\n");
                        break;
                    case SDL_QUIT:
                        TRACE("event.type test - Request to quit\n");
                        break;
                    case SDL_USEREVENT:
                        TRACE("event.type test - User event:\n");
                        TRACE("event.type test -        code:  %i\n",event.user.code);
                        TRACE("event.type test -        data1: %p\n",event.user.data1);
                        TRACE("event.type test -        data2: %p\n",event.user.data2);
                        break;
                    case SDL_SYSWMEVENT:
                        TRACE("event.type test - Window manager event\n");
                        break;
                    default:
                        TRACE("event.type test - I don't know what this event is!\n");
                  };
			TRACE("event.type == %i", event.type);
			*/
			screenManager.resetTimer();
			powerManager.resetTimer();
			return false;
		}

		if (event.type == SDL_QUIT) {
			TRACE("WAIT QUIT");
			actions[QUIT].active = true;
			return true;
		} else if (event.type == SDL_NOOPEVENT) {
			//TRACE("WAIT NOOP");
			actions[NOOP].active = true;
			// we can return true here, 
			// so that we don't keep the screen awake
			// but if it is awake, we redraw it for hardware changes
			return true;
		} else if (event.type == SDL_KEYDOWN) {
			if (SDLK_UNKNOWN == event.key.keysym.sym) {
				//TRACE("unknown key down");
				return false;
			}
			TRACE("WAIT SDL_KEYDOWN : %i", event.key.keysym.sym);
			if (isActionKey(POWER, event.key.keysym.sym)) {
				this->powerSet_ = true;
			}
			anyactions = true;
		} /* else if (event.type == SDL_KEYUP) {
			TRACE("WAIT SDL_KEYUP : %i", event.key.keysym.sym);
			if (isActionKey(POWER, event.key.keysym.sym)) {
				anyactions = true;
				this->powerSet_ = true;
			}
		} */
	} else {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				anyactions = true;
				TRACE("POLL SDL_KEYDOWN : %i", event.key.keysym.sym);
			}
		}
	}

	for (uint32_t x = 0; x < actions.size(); x++) {
		actions[x].active = isActive(x);
		if (actions[x].active) {
			/*
			if (actions[x].timer == NULL) {
				actions[x].timer = SDL_AddTimer(actions[x].interval, wakeUp, NULL);
			}
			*/
			anyactions = true;
			actions[x].last = SDL_GetTicks();
		} else {
			/*
			if (actions[x].timer != NULL) {
				SDL_RemoveTimer(actions[x].timer);
				actions[x].timer = NULL;
			}
			*/
			actions[x].last = 0;
		}
	}
	if (anyactions) {
		screenManager.resetTimer();
		powerManager.resetTimer();
	}

	//TRACE("update completed");
	return anyactions;
}

void InputManager::dropEvents() {
    for (uint32_t x = 0; x < actions.size(); x++) {
        TRACE("resetting action : %i", x);
        actions[x].active = false;
        /*
		if (actions[x].timer != NULL) {
			SDL_RemoveTimer(actions[x].timer);
			actions[x].timer = NULL;
		}
		*/
    }
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
    }
}

int InputManager::count() {
	return actions.size();
}

void InputManager::setInterval(int ms, int action) {
	if (action < 0)
		for (uint32_t x = 0; x < actions.size(); x++)
			actions[x].interval = ms;
	else if ((uint32_t)action < actions.size())
		actions[action].interval = ms;
}

void InputManager::noop() {
	//TRACE("enter");
	SDL_Event event;
	event.type = SDL_NOOPEVENT;
	SDL_PushEvent( &event );
}

bool &InputManager::operator[](int action) {
	return actions[action].active;
}

bool InputManager::isActionKey(int action, int key) {
	TRACE("enter - checking if %i is a key for action %i", key, action);
	bool result = false;
	MappingList mapList = actions[action].maplist;
	for (MappingList::const_iterator it = mapList.begin(); it != mapList.end(); ++it) {
		InputMap map = *it;
		if (map.type == InputManager::MAPPING_TYPE_KEYPRESS) {
			if (key == map.value) {
				TRACE("yes it is");
				result = true;
				break;
			}
		}
	}
	TRACE("exit : %i", result);
	return result;
}

bool InputManager::isActive(int action) {
	//TRACE("enter : %i", action);
	if (action >= actions.size() || action < 0) {
		WARNING("action %i is out of ramge of actions size %zu", action, actions.size());
		return false;
	}

	// dingoo work around
	if (action == POWER) {
		if (this->powerSet_ == true) {
			this->powerSet_= false;
			return true;
		}
		this->powerSet_= false;
	}

	MappingList mapList = actions[action].maplist;
	uint8_t *keystate = SDL_GetKeyState(NULL);

	//TRACE("we have got to check %zu mappings for action %i", mapList.size(), action);
	for (MappingList::const_iterator it = mapList.begin(); it != mapList.end(); ++it) {
		InputMap map = *it;
		switch (map.type) {
			case InputManager::MAPPING_TYPE_BUTTON:
				if (map.num < joysticks.size() && SDL_JoystickGetButton(joysticks[map.num], map.value))
					return true;
				break;
			case InputManager::MAPPING_TYPE_AXIS:
				if (map.num < joysticks.size()) {
					int axyspos = SDL_JoystickGetAxis(joysticks[map.num], map.value);
					if (map.treshold < 0 && axyspos < map.treshold) return true;
					if (map.treshold > 0 && axyspos > map.treshold) return true;
				}
				break;
			case InputManager::MAPPING_TYPE_KEYCOMBO:
				//TRACE("key combo test for action %i", action);
				if (this->isKeyCombo(keystate, map.combo)) {
					//TRACE("combo hit for action %i", action);
					return true;
				}
				break;
			case InputManager::MAPPING_TYPE_KEYPRESS:
				//TRACE("key press test for action %i", action);
				
				if (keystate[map.value]) {
					return true;
				}
				break;
		}
	}
	return false;
}

bool InputManager::isKeyCombo(uint8_t *keystate, const std::vector<int> & comboActions) {
	//TRACE("enter  - looking for %zu combo keys", comboActions.size());

	#if (LOG_LEVEL >= INFO_L)
	std::stringstream ss;
	for (std::vector<int>::const_iterator it = comboActions.begin(); it != comboActions.end(); it++) {
		ss << (*it) << ", ";
	}
	std::string search = ss.str();
	TRACE("we're looking for : '%s'", search.c_str());
	#endif

	bool isComboKey;
	std::list<int>::iterator mappedIt;
	for (mappedIt = this->allMappedKeys.begin(); mappedIt != this->allMappedKeys.end(); mappedIt++) {
		int keyCode = (*mappedIt);
		std::vector<int>::const_iterator comboIt;
		isComboKey = false;
		for (comboIt = comboActions.begin(); comboIt != comboActions.end(); comboIt++) {
			if (keyCode  == (*comboIt) ) {
				//TRACE("combo key match '%i'....checking state", keyCode);
				if (!keystate[keyCode]) {
					//TRACE("combo key %i not active, giving up", keyCode);
					return false;
				}
				//TRACE("combo key %i active, carrying on", keyCode);
				isComboKey = true;
				break;
			}
		}
		if (!isComboKey) {
			//TRACE("combo key miss '%i'....checking state", x);
			if (keystate[keyCode]) {
				//TRACE("non combo key %i active, giving up", keyCode);
				return false;
			}
			//TRACE("non combo key %i not active, carrying on", keyCode);
		}
	}
	//TRACE("exit");
	return true;
}

bool InputManager::isOnlyActive(int action) {
	TRACE("enter : %i", action);
	for (int x = 0; x < actions.size(); x++) {
		if (NOOP == x) {
			// skip a NOOP, they are never active
			continue;
		} else if (action == x) {
			// skip ourselves, as a short press might be over by now
			continue;
		} else if (isActive(x)) {
			TRACE("also active : %i", x);
			return false;
		}
	}
	TRACE("exit - true");
	return true;
}

void InputManager::setButtonRepeat(const int &repeatRate) {
	if (repeatRate > 0) {
		SDL_EnableKeyRepeat(INPUT_KEY_REPEAT_DELAY, repeatRate);
	} else {
		SDL_EnableKeyRepeat(0, 0);
	}
}
