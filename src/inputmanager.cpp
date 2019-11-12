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
	L2              KEY_PAGEUP,
	R2              KEY_PAGEDOWN,
	L3              KEY_KPSLASH,
	R3              KEY_KPDOT,
	START           KEY_ENTER,
	SELECT          KEY_ESC,
	POWER           KEY_POWER,
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
	
*/

#include "debug.h"
#include "inputmanager.h"
#include "screenmanager.h"
#include "utilities.h"

#include <iostream>
#include <fstream>

using namespace std;

InputManager::InputManager(ScreenManager& screenManager) : screenManager(screenManager) {
	wakeUpTimer = NULL;
}

InputManager::~InputManager() {
	for (uint32_t x = 0; x < joysticks.size(); x++)
		if(SDL_JoystickOpened(x))
			SDL_JoystickClose(joysticks[x]);
}

void InputManager::init(const string &conffile) {
	TRACE("InputManager::init started");
	initJoysticks();
	if (!readConfFile(conffile))
		ERROR("InputManager initialization from config file failed.");
	TRACE("InputManager::init completed");
}

void InputManager::initJoysticks() {
	TRACE("InputManager::initJoysticks started");
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
	TRACE("InputManager::initJoysticks completed");
}

bool InputManager::readConfFile(const string &conffile) {
	TRACE("InputManager::readconffile - enter : %s", conffile.c_str());
	setActionsCount(20); // plus 2 for BACKLIGHT and POWER

	if (!fileExists(conffile)) {
		ERROR("File not found: %s", conffile.c_str());
		return false;
	}

	ifstream inf(conffile.c_str(), ios_base::in);
	if (!inf.is_open()) {
		ERROR("Could not open %s", conffile.c_str());
		return false;
	}

	int action, linenum = 0;
	string line, name, value;
	string::size_type pos;
	vector<string> values;

	while (getline(inf, line, '\n')) {
		TRACE("InputManager::readconffile - reading : %s", line.c_str());
		linenum++;
		pos = line.find("=");
		name = trim(line.substr(0,pos));
		value = trim(line.substr(pos + 1,line.length()));

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
		else if (name == "speaker") {}
		else {
			ERROR("%s:%d Unknown action '%s'.", conffile.c_str(), linenum, name.c_str());
			return false;
		}

		split(values, value, ",");
		if (values.size() >= 2) {

			if (values[0] == "joystickbutton" && values.size() == 3) {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_BUTTON;
				map.num = atoi(values[1].c_str());
				map.value = atoi(values[2].c_str());
				map.treshold = 0;
				actions[action].maplist.push_back(map);
			} else if (values[0] == "joystickaxis" && values.size() == 4) {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_AXIS;
				map.num = atoi(values[1].c_str());
				map.value = atoi(values[2].c_str());
				map.treshold = atoi(values[3].c_str());
				actions[action].maplist.push_back(map);
			} else if (values[0] == "keyboard") {
				InputMap map;
				map.type = InputManager::MAPPING_TYPE_KEYPRESS;
				map.value = atoi(values[1].c_str());
				actions[action].maplist.push_back(map);
			} else {
				ERROR("%s:%d Invalid syntax or unsupported mapping type '%s'.", conffile.c_str(), linenum, value.c_str());
				return false;
			}

		} else {
			ERROR("%s:%d Every definition must have at least 2 values (%s).", conffile.c_str(), linenum, value.c_str());
			return false;
		}
	}
	TRACE("InputManager::readConfFile - close stream");
	inf.close();
	TRACE("InputManager::readConfFile - exit");
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
	//TRACE("InputManager::update started : wait = %i", wait);
	bool anyactions = false;
	SDL_JoystickUpdate();

	events.clear();
	SDL_Event event;

	if (wait) {
		SDL_WaitEvent(&event);
		
		if (screenManager.isAsleep() && SDL_WAKEUPEVENT != event.type && SDL_JOYAXISMOTION != event.type) {
			TRACE("InputManager::update - We're asleep, so we're just going to wake up and eat the timer event");
			
				// let's dump out what we can and isolate the wake up event type
				switch(event.type) {
                    case SDL_ACTIVEEVENT:
                        if (event.active.state & SDL_APPMOUSEFOCUS) {
                                if (event.active.gain) {
                                        TRACE("InputManager::update - event.type test - Mouse focus gained\n");
                                } else {
                                        TRACE("InputManager::update - event.type test - Mouse focus lost\n");
                                }
                        }
                        if (event.active.state & SDL_APPINPUTFOCUS) {
                                if (event.active.gain) {
                                        TRACE("InputManager::update - event.type test - Input focus gained\n");
                                } else {
                                        TRACE("InputManager::update - event.type test - Input focus lost\n");
                                }
                        }
                        if (event.active.state & SDL_APPACTIVE) {
                                if (event.active.gain) {
                                        TRACE("InputManager::update - event.type test - Application restored\n");
                                } else {
                                        TRACE("InputManager::update - event.type test - Application iconified\n");
                                }
                        }
                        break;
                    case SDL_KEYDOWN:  /* Handle a KEYDOWN event */
                        TRACE("InputManager::update - event.type test - Key pressed:\n");
                        TRACE("InputManager::update - event.type test -        SDL sim: %i\n",event.key.keysym.sym);
                        TRACE("InputManager::update - event.type test -        modifiers: %i\n",event.key.keysym.mod);
                        TRACE("InputManager::update - event.type test -        unicode: %i (if enabled with SDL_EnableUNICODE)\n",\
                                        event.key.keysym.unicode);
                        break;
                    case SDL_KEYUP:
                        TRACE("InputManager::update - event.type test - Key released:\n");
                        TRACE("InputManager::update - event.type test -        SDL sim: %i\n",event.key.keysym.sym);
                        TRACE("InputManager::update - event.type test -        modifiers: %i\n",event.key.keysym.mod);
                        TRACE("InputManager::update - event.type test -        unicode: %i (if enabled with SDL_EnableUNICODE)\n",\
                                        event.key.keysym.unicode);
                        break;
                    case SDL_MOUSEMOTION:
                        TRACE("InputManager::update - event.type test - Mouse moved to: (%i,%i) ", event.motion.x, event.motion.y);
                        TRACE("InputManager::update - event.type test - change: (%i,%i)\n", event.motion.xrel, event.motion.yrel);
                        TRACE("InputManager::update - event.type test -        button state: %i\n",event.motion.state);
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        TRACE("InputManager::update - event.type test - Mouse button %i ",event.button.button);
                        TRACE("InputManager::update - event.type test - pressed with mouse at (%i,%i)\n",event.button.x,event.button.y);
                        break;
                    case SDL_MOUSEBUTTONUP:
                        TRACE("InputManager::update - event.type test - Mouse button %i ",event.button.button);
                        TRACE("InputManager::update - event.type test - released with mouse at (%i,%i)\n",event.button.x,event.button.y);
                        break;
                    case SDL_JOYAXISMOTION:
                        TRACE("InputManager::update - event.type test - Joystick axis %i ",event.jaxis.axis);
                        TRACE("InputManager::update - event.type test - on joystick %i ", event.jaxis.which);
                        TRACE("InputManager::update - event.type test - moved to %i\n", event.jaxis.value);
                        break;
                    case SDL_JOYBALLMOTION:
                        TRACE("InputManager::update - event.type test - Trackball axis %i ",event.jball.ball);
                        TRACE("InputManager::update - event.type test - on joystick %i ", event.jball.which);
                        TRACE("InputManager::update - event.type test - moved to (%i,%i)\n", event.jball.xrel, event.jball.yrel);
                        break;
                    case SDL_JOYHATMOTION:
                        TRACE("InputManager::update - event.type test - Hat axis %i ",event.jhat.hat);
                        TRACE("InputManager::update - event.type test - on joystick %i ", event.jhat.which);
                        TRACE("InputManager::update - event.type test - moved to %i\n", event.jhat.value);
                        break;
                    case SDL_JOYBUTTONDOWN:
                        TRACE("InputManager::update - event.type test - Joystick button %i ",event.jbutton.button);
                        TRACE("InputManager::update - event.type test - on joystick %i ", event.jbutton.which);
                        TRACE("InputManager::update - event.type test - pressed\n");
                        break;
                    case SDL_JOYBUTTONUP:
                        TRACE("InputManager::update - event.type test - Joystick button %i ",event.jbutton.button);
                        TRACE("InputManager::update - event.type test - on joystick %i ", event.jbutton.which);
                        TRACE("InputManager::update - event.type test - released\n");
                        break;
                    case SDL_VIDEORESIZE:
                        TRACE("InputManager::update - event.type test - Window resized to: (%i,%i)\n",event.resize.w, event.resize.h);
                        break;
                    case SDL_VIDEOEXPOSE:
                        TRACE("InputManager::update - event.type test - Window exposed\n");
                        break;
                    case SDL_QUIT:
                        TRACE("InputManager::update - event.type test - Request to quit\n");
                        break;
                    case SDL_USEREVENT:
                        TRACE("InputManager::update - event.type test - User event:\n");
                        TRACE("InputManager::update - event.type test -        code:  %i\n",event.user.code);
                        TRACE("InputManager::update - event.type test -        data1: %p\n",event.user.data1);
                        TRACE("InputManager::update - event.type test -        data2: %p\n",event.user.data2);
                        break;
                    case SDL_SYSWMEVENT:
                        TRACE("InputManager::update - event.type test - Window manager event\n");
                        break;
                    default: /* Report an unhandled event */
                        TRACE("InputManager::update - event.type test - I don't know what this event is!\n");
                  };
			
			TRACE("InputManager::update - event.type == %i", event.type);
			screenManager.resetScreenTimer();
			return false;
		}

		if (event.type == SDL_KEYUP) anyactions = true;
		SDL_Event evcopy = event;
		events.push_back(evcopy);
		
	}
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYUP) anyactions = true;
		SDL_Event evcopy = event;
		events.push_back(evcopy);
	}

	int32_t now = SDL_GetTicks();
	for (uint32_t x = 0; x < actions.size(); x++) {
		actions[x].active = isActive(x);
		if (actions[x].active) {
			memcpy(input_combo, input_combo + 1, sizeof(input_combo) - 1); // eegg
			input_combo[sizeof(input_combo) - 1] = x; // eegg
			if (actions[x].timer == NULL) actions[x].timer = SDL_AddTimer(actions[x].interval, wakeUp, NULL);
			anyactions = true;
			// actions[x].last = now;
		} else {
			if (actions[x].timer != NULL) {
				SDL_RemoveTimer(actions[x].timer);
				actions[x].timer = NULL;
			}
			// actions[x].last = 0;
		}
	}
	if (anyactions) {
		screenManager.resetScreenTimer();
	}

	//TRACE("InputManager::update completed");
	return anyactions;
}

bool InputManager::combo() { // eegg
	return !memcmp(input_combo, konami, sizeof(input_combo));
}

void InputManager::dropEvents() {
	for (uint32_t x = 0; x < actions.size(); x++) {
		actions[x].active = false;
		if (actions[x].timer != NULL) {
			SDL_RemoveTimer(actions[x].timer);
			actions[x].timer = NULL;
		}
	}
}

uint32_t InputManager::checkRepeat(uint32_t interval, void *_data) {
	RepeatEventData *data = (RepeatEventData *)_data;
	InputManager *im = (class InputManager*)data->im;
	SDL_JoystickUpdate();
	if (im->isActive(data->action)) {
		SDL_PushEvent( im->fakeEventForAction(data->action) );
		return interval;
	} else {
		im->actions[data->action].timer = NULL;
		return 0;
	}
}

SDL_Event *InputManager::fakeEventForAction(int action) {
	MappingList mapList = actions[action].maplist;
	// Take the first mapping. We only need one of them.
	InputMap map = *mapList.begin();

	SDL_Event *event = new SDL_Event();
	switch (map.type) {
		case InputManager::MAPPING_TYPE_BUTTON:
			event->type = SDL_JOYBUTTONDOWN;
			event->jbutton.type = SDL_JOYBUTTONDOWN;
			event->jbutton.which = map.num;
			event->jbutton.button = map.value;
			event->jbutton.state = SDL_PRESSED;
		break;
		case InputManager::MAPPING_TYPE_AXIS:
			event->type = SDL_JOYAXISMOTION;
			event->jaxis.type = SDL_JOYAXISMOTION;
			event->jaxis.which = map.num;
			event->jaxis.axis = map.value;
			event->jaxis.value = map.treshold;
		break;
		case InputManager::MAPPING_TYPE_KEYPRESS:
			event->type = SDL_KEYDOWN;
			event->key.keysym.sym = (SDLKey)map.value;
		break;
	}
	return event;
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

void InputManager::setWakeUpInterval(int ms) {
	//TRACE("InputManager::setWakeUpInterval - enter - %i", ms);
	if (wakeUpTimer != NULL)
		SDL_RemoveTimer(wakeUpTimer);

	if (ms > 0)
		wakeUpTimer = SDL_AddTimer(ms, wakeUp, NULL);
}

uint32_t InputManager::wakeUp(uint32_t interval, void *_data) {
	//TRACE("InputManager::wakeUp - enter");
	SDL_Event *event = new SDL_Event();
	event->type = SDL_WAKEUPEVENT;
	SDL_PushEvent( event );
	return interval;
}

bool &InputManager::operator[](int action) {
	// if (action<0 || (uint32_t)action>=actions.size()) return false;
	return actions[action].active;
}

bool InputManager::isActive(int action) {
	MappingList mapList = actions[action].maplist;
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
			case InputManager::MAPPING_TYPE_KEYPRESS:
				uint8_t *keystate = SDL_GetKeyState(NULL);
				return keystate[map.value];
			break;
		}
	}
	return false;
}
