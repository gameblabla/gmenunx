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

#ifndef SELECTOR_H_
#define SELECTOR_H_

#include <string>
#include <unordered_map>
#include "esoteric.h"
#include "utilities.h"
#include "dialog.h"

class LinkApp;
class FileLister;

class Selector : protected Dialog {
private:
	int selRow;
	uint32_t animation;
	uint32_t tickStart;
	uint32_t firstElement;
	uint32_t numRows;
	std::string screendir;
	int numDirs;
	int numFiles;
	std::vector<std::string> screens;

	LinkApp *link;

	std::string file, dir;
	std::unordered_map<std::string, std::string> aliases;
	void loadAliases();
	std::string getAlias(const std::string &key, const std::string &fname);
	void prepare(FileLister *fl, std::vector<std::string> *titles, std::vector<std::string> *screens = NULL);
	void freeScreenshots(std::vector<std::string> *screens);

public:
	Selector(Esoteric *app, LinkApp *link, const std::string &selectorDir = "");
	
	void resolve(int selection = 0);
	int exec(int startSelection = 0);
	
	const std::string &getFile() { return file; }
	const std::string &getDir() { return dir; }

};

#endif /*SELECTOR_H_*/
