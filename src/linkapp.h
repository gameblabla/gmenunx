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
#ifndef LINKAPP_H
#define LINKAPP_H

#include <string>
#include <iostream>

#include "link.h"

using std::string;

class GMenu2X;
class InputManager;

/**
Parses links files.

	@author Massimiliano Torromeo <massimiliano.torromeo@gmail.com>
*/
class LinkApp : public Link {
private:
	InputManager &inputMgr;

	int iclock = 0;

	string exec, params, workdir, manual, manualPath, selectordir, selectorfilter, selectorscreens, backdrop, backdropPath;
	bool selectorbrowser, consoleapp, deletable, editable;

	bool isOPK;
	string opkMount, opkFile, category, metadata;

	string aliasfile;
	string file;

public:

	LinkApp(GMenu2X *gmenu2x, const char* linkfile, bool deletable, struct OPK *opk = NULL, const char *metadata = NULL);

	const std::string &getCategory() { return category; }
	bool isOpk() { return isOPK; }
	const std::string &getOpkFile() { return opkFile; }

	virtual const string &searchIcon();
	virtual const string &searchBackdrop();
	virtual const string &searchManual();

	const string &getExec();
	void setExec(const string &exec);
	const string &getParams();
	void setParams(const string &params);
	const string &getWorkdir();
	const string getRealWorkdir();
	void setWorkdir(const string &workdir);
	const string &getManual();
	const string &getManualPath() { return manualPath; }
	void setManual(const string &manual);
	const string &getSelectorDir();
	void setSelectorDir(const string &selectordir);
	bool getSelectorBrowser();
	void setSelectorBrowser(bool value);

	const string &getSelectorScreens();
	void setSelectorScreens(const string &selectorscreens);
	const string &getSelectorFilter();
	void setSelectorFilter(const string &selectorfilter);
	const string &getAliasFile();
	void setAliasFile(const string &aliasfile);

	int clock();
	// const string &clockStr(int maxClock);
	void setCPU(int mhz = 0);

	bool save();
	void run();
	// void showManual();
	void selector(int startSelection=0, const string &selectorDir="");
	void launch(const string &selectedFile="", const string &selectedDir="");
	bool targetExists();

	const string &getFile() { return file; }
	const string &getBackdrop() { return backdrop; }
	const string &getBackdropPath() { return backdropPath; }
	void setBackdrop(const string selectedFile="");

	void renameFile(const string &name);
	bool isDeletable() { return deletable; }
	bool isEditable() { return editable; }

	std::ostream& operator<<(const LinkApp &a);

};

#endif
