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

class Esoteric;
class InputManager;

/**
Parses links files.

	@author Massimiliano Torromeo <massimiliano.torromeo@gmail.com>
*/
class LinkApp : public Link {

private:
	InputManager &inputMgr;
	static const string FAVOURITE_FOLDER;

	int iclock = 0;

	string exec, params, workdir, manual, manualPath;
	string selectordir, selectorfilter, selectorscreens, backdrop, backdropPath;
	string provider, providerMetadata;
	bool selectorbrowser, consoleapp, deletable, editable;
	string aliasfile;
	string file;

	void launch(string launchArgs = "");
	string resolveArgs(const string &selectedFile = "", const string &selectedDir = "");
	void favourite(string launchArgs, string supportingFile = "");

public:

	LinkApp(Esoteric *app, const char* linkfile, bool deletable);

	const std::string getFavouriteFolder() { return FAVOURITE_FOLDER; }
	virtual const string &searchIcon();
	virtual const string &searchIcon(string path, bool fallBack = true);
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
	bool getConsoleApp();
	void setConsoleApp(bool value);
	const string &getProvider() { return provider; }
	void setProvider(const string &provider);
	const string &getProviderMetadata() { return providerMetadata; }
	void setProviderMetadata(const string &providerMetadata);

	const string &getSelectorScreens();
	void setSelectorScreens(const string &selectorscreens);
	const string &getSelectorFilter();
	void setSelectorFilter(const string &selectorfilter);
	const string &getAliasFile();
	void setAliasFile(const string &aliasfile);

	int clock();
	void setCPU(int mhz = 0);

	bool save();
	void run();
	void makeFavourite();

	// void showManual();
	void selector(int startSelection=0, const string &selectorDir="");
	bool targetExists();

	const string &getFile() { return file; }
	const string &getBackdrop() { return backdrop; }
	const string &getBackdropPath() { return backdropPath; }
	void setBackdrop(const string selectedFile="");

	void renameFile(const string &name);
	bool isDeletable() { return deletable; }
	bool isEditable() { return editable; }

	std::string toString();

};

#endif
