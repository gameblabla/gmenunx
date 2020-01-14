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
#ifndef LINK_H
#define LINK_H

#include <string>
#include <iostream>

#include "button.h"

// linkaction
#include "FastDelegate.h"
typedef fastdelegate::FastDelegate0<> LinkAction;

class Esoteric;

/**
Base class that represents a link on screen.

	@author Massimiliano Torromeo <massimiliano.torromeo@gmail.com>
*/
class Link : public Button {
private:
	uint32_t padding;
	LinkAction action;

protected:
	Esoteric *app;
	bool edited;
	std::string title, description, icon, iconPath;
	std::string displayTitle;

public:
	// linkaction
	Link(Esoteric *app, LinkAction action);

	virtual ~Link() {};
	const std::string &getTitle();
	const std::string &getDisplayTitle();
	void setTitle(const std::string &title);
	const std::string &getDescription();
	void setDescription(const std::string &description);
	const std::string &getIcon();
	void setIcon(const std::string &icon);
	virtual const std::string &searchIcon();
	const std::string &getIconPath();
	void setIconPath(const std::string &icon);

	virtual void run();
};

#endif
