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

Link::Link(Esoteric *app, LinkAction action)
	: Button(app->ts, true)
	, app(app)
{
	this->action = action;
	this->edited = false;
	this->iconPath = app->skin->getSkinFilePath("icons/generic.png");
	this->padding = 4;
	this->displayTitle = "";
}

void Link::run() {
	TRACE("calling action");
	this->action();
	TRACE("action called");
}

const std::string &Link::getTitle() {
	return this->title;
}

const std::string &Link::getDisplayTitle() {
	return this->displayTitle;
}

void Link::setTitle(const std::string &title) {
	if (title != this->title) {
		this->title = title;
		this->edited = true;

		// Reduce title length to fit the link width
		// TODO :: maybe move to a format function, called after loading in LinkApp
		// and called again after skin column change etc
		std::string temp = std::string(title);
		temp = StringUtils::strReplace(temp, "-", " ");
		std::string::size_type pos = temp.find( "  ", 0 );
		while (pos != std::string::npos) {
			temp = StringUtils::strReplace(temp, "  ", " ");
			pos = temp.find( "  ", 0 );
		};
		int maxWidth = (app->linkWidth);
		if ((int)app->font->getLineWidthSafe(temp) > maxWidth) {
			while ((int)app->font->getLineWidthSafe(temp + "..") > maxWidth) {
				temp = temp.substr(0, temp.length() - 1);
			}
			temp += "..";
		}
		this->displayTitle = temp;
	}
}

const std::string &Link::getDescription() {
	return description;
}

void Link::setDescription(const std::string &description) {
	this->description = description;
	edited = true;
}

const std::string &Link::getIcon() {
	return icon;
}

void Link::setIcon(const std::string &icon) {
	this->icon = icon;

	if (icon.compare(0, 5, "skin:") == 0)
		this->iconPath = app->skin->getSkinFilePath(icon.substr(5, std::string::npos));
	else
		this->iconPath = icon;

	edited = true;
}

const std::string &Link::searchIcon() {
	TRACE("enter");
	if (this->iconPath.empty()) {
		this->iconPath = app->skin->getSkinFilePath("icons/generic.png");
	} else if (!FileUtils::fileExists(this->iconPath)) {
		this->iconPath = app->skin->getSkinFilePath("icons/generic.png");
	} else if (!app->skin->getSkinFilePath(this->iconPath).empty()) {
		this->iconPath = app->skin->getSkinFilePath(this->iconPath);
	} else
		this->iconPath = app->skin->getSkinFilePath("icons/generic.png");
	TRACE("exit : %s", this->iconPath.c_str());
	return this->iconPath;
}

const std::string &Link::getIconPath() {
	if (iconPath.empty()) searchIcon();
	return iconPath;
}

void Link::setIconPath(const std::string &icon) {
	if (FileUtils::fileExists(icon))
		iconPath = icon;
	else
		iconPath = app->skin->getSkinFilePath("icons/generic.png");
}
