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
#include <fstream>
#include <sstream>
#include "link.h"
#include "menu.h"
#include "selector.h"
#include "debug.h"
#include "fonthelper.h"

Link::Link(GMenu2X *gmenu2x_, LinkAction action)
	: Button(gmenu2x_->ts, true)
	, gmenu2x(gmenu2x_)
{
	this->action = action;
	this->edited = false;
	this->iconPath = gmenu2x->skin->getSkinFilePath("icons/generic.png");
	this->padding = 4;
	this->displayTitle = "";

}

void Link::run() {
	TRACE("calling action");
	this->action();
	TRACE("action called");
}

const string &Link::getTitle() {
	return this->title;
}

const string &Link::getDisplayTitle() {
	return this->displayTitle;
}

void Link::setTitle(const string &title) {
	if (title != this->title) {
		this->title = title;
		this->edited = true;

		// Reduce title length to fit the link width
		// TODO :: maybe move to a format function, called after loading in LinkApp
		// and called again after skin column change etc
		string temp = string(title);
		temp = strreplace(temp, "-", " ");
		string::size_type pos = temp.find( "  ", 0 );
		while (pos != string::npos) {
			temp = strreplace(temp, "  ", " ");
			pos = temp.find( "  ", 0 );
		};
		int maxWidth = (gmenu2x->linkWidth);
		if ((int)gmenu2x->font->getTextWidth(temp) > maxWidth) {
			while ((int)gmenu2x->font->getTextWidth(temp + "..") > maxWidth) {
				temp = temp.substr(0, temp.length() - 1);
			}
			temp += "..";
		}
		this->displayTitle = temp;
	}
}

const string &Link::getDescription() {
	return description;
}

void Link::setDescription(const string &description) {
	this->description = description;
	edited = true;
}

const string &Link::getIcon() {
	return icon;
}

void Link::setIcon(const string &icon) {
	this->icon = icon;

	if (icon.compare(0, 5, "skin:") == 0)
		this->iconPath = gmenu2x->skin->getSkinFilePath(icon.substr(5, string::npos));
	else
		this->iconPath = icon;

	edited = true;
}

const string &Link::searchIcon() {
	if (!gmenu2x->skin->getSkinFilePath(iconPath).empty()) {
		iconPath = gmenu2x->skin->getSkinFilePath(iconPath);
	}	else if (!fileExists(iconPath)) {
		iconPath = gmenu2x->skin->getSkinFilePath("icons/generic.png");
	} else
		iconPath = gmenu2x->skin->getSkinFilePath("icons/generic.png");
	return iconPath;
}

const string &Link::getIconPath() {
	if (iconPath.empty()) searchIcon();
	return iconPath;
}

void Link::setIconPath(const string &icon) {
	if (fileExists(icon))
		iconPath = icon;
	else
		iconPath = gmenu2x->skin->getSkinFilePath("icons/generic.png");
}
