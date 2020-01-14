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

#include "link.h"
#include "esoteric.h"

/**
Parses links files.

	@author Massimiliano Torromeo <massimiliano.torromeo@gmail.com>
*/
class LinkApp : public Link {

private:

	static const std::string FAVOURITE_FOLDER;

	int iclock = 0;

	std::string exec, params, workdir, manual, manualPath;
	std::string selectordir, selectorfilter, selectorscreens, backdrop, backdropPath;
	std::string provider, providerMetadata;
	bool selectorbrowser, consoleapp, deletable, editable, hidden;
	std::string aliasfile;
	std::string file;

	void launch(std::string launchArgs = "");
	std::string resolveArgs(const std::string &selectedFile = "", const std::string &selectedDir = "");
	void favourite(std::string launchArgs, std::string supportingFile = "", std::string backdrop = "");

public:

	LinkApp(Esoteric *app, const char* linkfile, bool deletable);

	const std::string getFavouriteFolder() { return FAVOURITE_FOLDER; }
	virtual const std::string &searchIcon();
	virtual const std::string &searchIcon(std::string path, bool fallBack = true);
	virtual const std::string &searchBackdrop();
	virtual const std::string &searchManual();

	const std::string &getExec();
	void setExec(const std::string &exec);
	const std::string &getParams();
	void setParams(const std::string &params);
	const std::string &getWorkdir();
	const std::string getRealWorkdir();
	void setWorkdir(const std::string &workdir);
	const std::string &getManual();
	const std::string &getManualPath() { return manualPath; }
	void setManual(const std::string &manual);
	const std::string &getSelectorDir();
	void setSelectorDir(const std::string &selectordir);
	bool getSelectorBrowser();
	void setSelectorBrowser(bool value);
	bool getConsoleApp();
	void setConsoleApp(bool value);
	bool getHidden();
	void setHidden(bool value);
	const std::string &getProvider() { return provider; }
	void setProvider(const std::string &provider);
	const std::string &getProviderMetadata() { return providerMetadata; }
	void setProviderMetadata(const std::string &providerMetadata);

	const std::string &getSelectorScreens();
	void setSelectorScreens(const std::string &selectorscreens);
	const std::string &getSelectorFilter();
	void setSelectorFilter(const std::string &selectorfilter);
	const std::string &getAliasFile();
	void setAliasFile(const std::string &aliasfile);

	int clock();
	void setCPU(int mhz = 0);

	bool save();
	void run();
	void makeFavourite();
	void makeFavourite(std::string dir, std::string file, std::string backdrop = "");

	// void showManual();
	void selector(int startSelection=0, const std::string &selectorDir = "");
	bool targetExists();

	const std::string &getFile() { return file; }
	const std::string &getBackdrop() { return backdrop; }
	const std::string &getBackdropPath() { return backdropPath; }
	void setBackdrop(const std::string selectedFile = "");

	void renameFile(const std::string &name);
	bool isDeletable() { return deletable; }
	bool isEditable() { return editable; }

	std::string toString();

};

#endif
