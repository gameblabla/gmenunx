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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <sstream>
#include <opk.h>
#include <array>
#include <cerrno>

#include "messagebox.h"
#include "linkapp.h"
#include "launcher.h"
#include "menu.h"
#include "selector.h"
#include "imageio.h"
#include "debug.h"
#include "constants.h"

using namespace std;

static array<const char *, 4> tokens = { "%f", "%F", "%u", "%U", };
const string LinkApp::FAVOURITE_FOLDER = "favourites";

LinkApp::LinkApp(GMenu2X *gmenu2x_, const char* linkfile, bool deletable_) :
	Link(gmenu2x_, MakeDelegate(this, &LinkApp::run)),
	inputMgr(gmenu2x->input) {

	TRACE("ctor - handling normal desktop file :%s", linkfile);
	this->manual = manualPath = "";
	this->file = linkfile;

	TRACE("ctor - setCPU");
	setCPU(gmenu2x->config->cpuMenu());

	this->selectordir = "";
	this->selectorfilter = "";
	this->icon = iconPath = "";
	this->selectorbrowser = true;
	this->consoleapp = true;
	this->workdir = "";
	this->backdrop = backdropPath = "";
	this->editable = this->deletable = deletable;

	string line;
	TRACE("ctor - creating ifstream");
	std::ifstream infile (linkfile, ios_base::in);
	std::locale loc("");
	infile.imbue(loc);
	if (infile.is_open()) {
		TRACE("ctor - iterating thru desktop file : %s", linkfile);
		try {
			while (std::getline(infile, line, '\n')) {

				TRACE("read raw line : %s", line.c_str());
				line = trim(line);
				if (line.empty()) continue;
				if (line[0] == '#') continue;

				std::size_t position = line.find("=");
				string name = toLower(trim(line.substr(0,position)));
				string value = trim(line.substr(position+1));

				try {
					if (name == "clock") {
						this->setCPU( atoi(value.c_str()) );
					} else if (name == "selectordir") {
						this->setSelectorDir( value );
					} else if (name == "selectorbrowser") {
						this->setSelectorBrowser(value == "true" ? true : false);
					} else if (name == "title") {
						this->setTitle(value);
					} else if (name == "description") {
						this->setDescription(value);
					} else if (name == "icon") {
						this->setIcon(value);
					} else if (name == "exec") {
						this->setExec(value);
					} else if (name == "params") {
						this->setParams(value);
					} else if (name == "workdir") {
						this->setWorkdir(value);
					} else if (name == "manual") {
						this->setManual(value);
					} else if (name == "consoleapp") {
						this->setConsoleApp(value == "true" ? true : false);
					} else if (name == "selectorfilter") {
						this->setSelectorFilter( value );
					} else if (name == "selectorscreens") {
						this->setSelectorScreens( value );
					} else if (name == "selectoraliases") {
						this->setAliasFile( value );
					} else if (name == "backdrop") {
						this->setBackdrop(value);
					} else if (name == "x-provider") {
						this->setProvider(value);
					} else if (name == "x-providermetadata") {
						this->setProviderMetadata(value);
					} else {
						WARNING("Unrecognized native link option: '%s' in %s", name.c_str(), linkfile);
					}
				}                     
				catch (int param) { 
					ERROR("int exception : %i from <%s, %s>", 
						param, name.c_str(), value.c_str()); }
				catch (char *param) { 
					ERROR("char exception : %s from <%s, %s>", 
						param, name.c_str(), value.c_str()); }
				catch (...) { 
						ERROR("unknown error reading value from <%s, %s>", 
							name.c_str(), value.c_str());
				}
			}
		}
		catch (...) {
			ERROR("Error reading desktop file : %s", linkfile);
		}
		TRACE("ctor - closing infile");
		if (infile.is_open())
			infile.close();
	} else {
		WARNING("Couldn't open desktop file : %s for reading", linkfile);
	}

	// try and find an icon 
	if (iconPath.empty()) {
		TRACE("ctor - searching for icon");
		searchIcon(exec, true);
	}

	TRACE("ctor exit : %s", this->toString().c_str());
	edited = false;
}

const string &LinkApp::searchManual() {
	if (!manualPath.empty()) return manualPath;
	string filename = exec;
	string::size_type pos = exec.rfind(".");
	if (pos != string::npos) filename = exec.substr(0, pos);
	filename += ".man.txt";

	string dname = dir_name(exec) + "/";

	string dirtitle = dname + base_name(dir_name(exec)) + ".man.txt";
	string linktitle = base_name(file);
	pos = linktitle.rfind(".");
	if (pos != string::npos) linktitle = linktitle.substr(0, pos);
	linktitle = dname + linktitle + ".man.txt";

	if (fileExists(linktitle))
		manualPath = linktitle;
	else if (fileExists(filename))
		manualPath = filename;
	else if (fileExists(dirtitle))
		manualPath = dirtitle;

	return manualPath;
}

const string &LinkApp::searchBackdrop() {
	if (!backdropPath.empty() || !gmenu2x->skin->skinBackdrops) return backdropPath;
	string execicon = exec;
	string::size_type pos = exec.rfind(".");
	if (pos != string::npos) execicon = exec.substr(0, pos);
	string exectitle = base_name(execicon);
	string dirtitle = base_name(dir_name(exec));
	string linktitle = base_name(file);
	pos = linktitle.rfind(".");
	if (pos != string::npos) linktitle = linktitle.substr(0, pos);

	// auto backdrop
	if (!gmenu2x->skin->getSkinFilePath("backdrops/" + linktitle + ".png").empty())
		backdropPath = gmenu2x->skin->getSkinFilePath("backdrops/" + linktitle + ".png");
	else if (!gmenu2x->skin->getSkinFilePath("backdrops/" + linktitle + ".jpg").empty())
		backdropPath = gmenu2x->skin->getSkinFilePath("backdrops/" + linktitle + ".jpg");
	else if (!gmenu2x->skin->getSkinFilePath("backdrops/" + exectitle + ".png").empty())
		backdropPath = gmenu2x->skin->getSkinFilePath("backdrops/" + exectitle + ".png");
	else if (!gmenu2x->skin->getSkinFilePath("backdrops/" + exectitle + ".jpg").empty())
		backdropPath = gmenu2x->skin->getSkinFilePath("backdrops/" + exectitle + ".jpg");
	else if (!gmenu2x->skin->getSkinFilePath("backdrops/" + dirtitle + ".png").empty())
		backdropPath = gmenu2x->skin->getSkinFilePath("backdrops/" + dirtitle + ".png");
	else if (!gmenu2x->skin->getSkinFilePath("backdrops/" + dirtitle + ".jpg").empty())
		backdropPath = gmenu2x->skin->getSkinFilePath("backdrops/" + dirtitle + ".jpg");

	return backdropPath;
}

const string &LinkApp::searchIcon() {
	return searchIcon(exec, true);
}

const string &LinkApp::searchIcon(string path, bool fallBack) {
	TRACE("enter - fallback : %i", fallBack);

	// get every permutation possible from the metadata opts
	string execicon = path;
	string::size_type pos = path.rfind(".");
	if (pos != string::npos) execicon = path.substr(0, pos);
	execicon += ".png";
	string exectitle = base_name(execicon);
	string dirtitle = base_name(dir_name(path)) + ".png";
	string linktitle = base_name(file);
	pos = linktitle.rfind(".");
	if (pos != string::npos) linktitle = linktitle.substr(0, pos);
	linktitle += ".png";

	if (!gmenu2x->skin->getSkinFilePath("icons/" + linktitle).empty())
		iconPath = gmenu2x->skin->getSkinFilePath("icons/" + linktitle);
	else if (!gmenu2x->skin->getSkinFilePath("icons/" + exectitle).empty())
		iconPath = gmenu2x->skin->getSkinFilePath("icons/" + exectitle);
	else if (!gmenu2x->skin->getSkinFilePath("icons/" + dirtitle).empty())
		iconPath = gmenu2x->skin->getSkinFilePath("icons/" + dirtitle);
	else if (fileExists(execicon))
		iconPath = execicon;
	else if (fallBack) {
		iconPath = gmenu2x->skin->getSkinFilePath("icons/generic.png");
	}

	TRACE("exit - iconPath : %s", iconPath.c_str());
	return iconPath;
}

int LinkApp::clock() {
	return iclock;
}

void LinkApp::setCPU(int mhz) {
	iclock = mhz;
	if (iclock != 0) iclock = constrain(iclock, gmenu2x->config->cpuMin(), gmenu2x->config->cpuMax());
	edited = true;
}

void LinkApp::setBackdrop(const string selectedFile) {
	backdrop = backdropPath = selectedFile;
	edited = true;
}

bool LinkApp::targetExists() {
	string target = exec;
	if (!exec.empty() && exec[0] != '/' && !workdir.empty())
		target = workdir + "/" + exec;

	TRACE("looking for : %s", target.c_str());
	return fileExists(target);
}

bool LinkApp::save() {
	TRACE("enter : %s", file.c_str());
	if (!edited) {
		TRACE("not edited, nothing to save");
		return false;
	}
	std::ostringstream out;
	out << this->toString();

	if (out.tellp() > 0) {
		TRACE("saving data : %s", out.str().c_str());
		ofstream f(file.c_str());
		if (f.is_open()) {
			f << out.str();
			f.close();
			sync();
			TRACE("save success");
			return true;
		} else {
			ERROR("LinkApp::save - Error while opening the file '%s' for write.\n", file.c_str());
			return false;
		}
	} else {
		TRACE("Empty app settings: %s\n", file.c_str());
		return unlink(file.c_str()) == 0 || errno == ENOENT;
	}
}

void LinkApp::favourite(string launchArgs, string supportingFile) {
	TRACE("enter - launchArgs : %s, supportingFile : %s", 
		launchArgs.c_str(), 
		supportingFile.c_str());

	// need to make a new favourite
	string path = this->gmenu2x->getWriteablePath() + "sections/" + FAVOURITE_FOLDER;
	if (!this->gmenu2x->menu->sectionExists(FAVOURITE_FOLDER)) {
		if (!this->gmenu2x->menu->addSection(FAVOURITE_FOLDER)) {
			 ERROR("LinkApp::selector - Couldn't make favourites folder : %s", path.c_str());
			 return;
		}
	}

	string cleanTitle = this->title;
	string description = this->description;
	if (!supportingFile.empty()) {
		cleanTitle = fileBaseName(base_name(supportingFile));
		description = this->description + " - " + cleanTitle;
	}
	string favePath = path + "/" + this->title + "-" + cleanTitle + ".desktop";

	uint32_t x = 1;
	while (fileExists(favePath)) {
		string id = "";
		stringstream ss; ss << x; ss >> id;
		favePath = path + "/" + this->title + "-" + cleanTitle + "." + id + ".desktop";
		x++;
	}
	/*
	if (fileExists(favePath)) {
		TRACE("deleting existing favourite : %s", favePath.c_str());
		unlink(favePath.c_str());
	}
	*/

	LinkApp * fave = new LinkApp(this->gmenu2x, favePath.c_str(), true);
	fave->setTitle(cleanTitle);
	fave->setIcon(this->icon);
	fave->setDescription(description);
	fave->consoleapp = this->consoleapp;

	TRACE("saving a normal favourite to : %s", favePath.c_str());
	TRACE("normal exec path : %s", this->exec.c_str());
	TRACE("normal params : %s", launchArgs.c_str());
	fave->setExec(this->exec);
	fave->setParams(launchArgs);

	MessageBox mb(
		this->gmenu2x, 
		"Savinging favourite - " + cleanTitle,  
		this->icon);
	mb.setAutoHide(500);
	mb.exec();
	if (fave->save()) {
		int secIndex = this->gmenu2x->menu->getSectionIndex(FAVOURITE_FOLDER);
		this->gmenu2x->menu->setSectionIndex(secIndex);
		this->gmenu2x->menu->sectionLinks(secIndex)->push_back(fave);
		TRACE("favourite saved");
	} else delete fave;
	
	TRACE("exit");
}

void LinkApp::makeFavourite() {
	string launchArgs = resolveArgs();
	favourite(launchArgs);
}
/*
 * entry point for running
 * checks to see if we want a supporting file arg
 * and launches a chooser
 * or
 * just launches
 */

void LinkApp::run() {
	TRACE("enter");
	if (!selectordir.empty()) {
		selector();
	} else {
		std::string launchArgs = resolveArgs();
		launch(launchArgs);
	}
	TRACE("exit");
}

/*
 * lauches a supporting file selector if needed
 */
void LinkApp::selector(int startSelection, const string &selectorDir) {
	TRACE("enter - startSelection = %i, selectorDir = %s", startSelection, selectorDir.c_str());

	//Run selector interface - this is the order of specificity
	string myDir = selectorDir;
	if (myDir.empty()) {
		myDir = this->selectordir;
		if (myDir.empty()) {
			myDir = this->gmenu2x->config->launcherPath();
		}
	}

	Selector sel(gmenu2x, this, myDir);
	int selection = sel.exec(startSelection);
	// we got a file
	if (selection != -1) {

		string launchArgs = resolveArgs(sel.getFile(), sel.getDir());

		if (sel.isFavourited()) {
			TRACE("we're saving a favourite");
			string romFile = sel.getDir() + sel.getFile();
			TRACE("launchArgs : %s, romFile : %s", launchArgs.c_str(), romFile.c_str());
			favourite(launchArgs, romFile);

		} else {
			gmenu2x->writeTmp(selection, sel.getDir());
			launch(launchArgs);
		}
	}
}

string LinkApp::resolveArgs(const string &selectedFile, const string &selectedDir) {

	string launchArgs;

	// selectedFile means a rom or some kind of data file..
	if (!selectedFile.empty()) {
		TRACE("we have a selected file to work with : %s", selectedFile.c_str());
		// break out the dir, filename and extension
		string selectedFileExtension;
		string selectedFileName;
		string dir;

		selectedFileExtension = fileExtension(selectedFile);
		selectedFileName = fileBaseName(selectedFile);

		TRACE("name : %s, extension : %s", selectedFileName.c_str(), selectedFileExtension.c_str());
		if (selectedDir.empty())
			dir = getSelectorDir();
		else
			dir = selectedDir;

		// force a slash at the end
		if (dir.length() > 0) {
			if (0 != dir.compare(dir.length() -1, 1, "/")) 
				dir += "/";
		} else dir = "/";
		TRACE("dir : %s", dir.c_str());

		if (this->params.empty()) {
			launchArgs = "\"" + dir + selectedFile + "\"";
			TRACE("no params, so cleaned to : %s", launchArgs.c_str());
		} else {
			TRACE("params need handling : %s", params.c_str());
			launchArgs = strreplace(params, "[selFullPath]", cmdclean(dir + selectedFile));
			launchArgs = strreplace(launchArgs, "[selPath]", cmdclean(dir));
			launchArgs = strreplace(launchArgs, "[selFile]", cmdclean(selectedFileName));
			launchArgs = strreplace(launchArgs, "[selExt]", cmdclean(selectedFileExtension));
			// if this is true, then we've made no subs, so we still need to add the selected file
			if (this->params == launchArgs) 
				launchArgs += " \"" + dir + selectedFile + "\"";

		}
		// save the last dir
		TRACE("setting the launcher path : %s", dir.c_str());
		gmenu2x->config->launcherPath(dir);
	} else launchArgs = params;

	TRACE("exit : %s", launchArgs.c_str());
	return launchArgs;

}

void LinkApp::launch(string launchArgs) {
	TRACE("enter - args: %si", launchArgs.c_str());

	//Set correct working directory
	string wd = getRealWorkdir();
	TRACE("real work dir = %s", wd.c_str());
	if (!wd.empty()) {
		chdir(wd.c_str());
	}

	if (gmenu2x->config->saveSelection()) {
		TRACE("updating selections");
		gmenu2x->config->section(gmenu2x->menu->selSectionIndex());
		gmenu2x->config->link(gmenu2x->menu->selLinkIndex());
	}

	// Check to see if permissions are desirable
	struct stat fstat;
	if ( stat( this->exec.c_str(), &fstat ) == 0 ) {
		struct stat newstat = fstat;
		if ( S_IRUSR != ( fstat.st_mode & S_IRUSR ) ) newstat.st_mode |= S_IRUSR;
		if ( S_IXUSR != ( fstat.st_mode & S_IXUSR ) ) newstat.st_mode |= S_IXUSR;
		if ( fstat.st_mode != newstat.st_mode ) chmod( exec.c_str(), newstat.st_mode );
	} // else, well.. we are no worse off :)

	std::string execute = this->exec + " " + launchArgs;
	TRACE("standard file cmd lime : %s",  execute.c_str());

	if (gmenu2x->config->outputLogs()) {
		execute += " 2>&1 | tee " + cmdclean(gmenu2x->getWriteablePath()) + "log.txt";
		TRACE("adding logging");
	}
	TRACE("final command : %s", execute.c_str());
	std::vector<string> commandLine = { "/bin/sh", "-c" };
	commandLine.push_back(execute);

	Launcher *toLaunch = new Launcher(commandLine, this->consoleapp);
	if (toLaunch) {
		unsetenv("SDL_FBCON_DONT_CLEAR");

		TRACE("quit");
		gmenu2x->quit();

		TRACE("calling exec");
		toLaunch->exec();
		// If control gets here, execution failed. Since we already destructed
		// everything, the easiest solution is to exit and let the system
		// respawn the menu.
		delete toLaunch;
	}

	TRACE("exit");
}

const string &LinkApp::getExec() {
	return exec;
}

void LinkApp::setExec(const string &exec) {
	this->exec = exec;
	edited = true;
}

const string &LinkApp::getParams() {
	return params;
}

void LinkApp::setParams(const string &params) {
	this->params = params;
	edited = true;
}

const string &LinkApp::getWorkdir() {
	return workdir;
}

const string LinkApp::getRealWorkdir() {
	string wd = workdir;
	if (wd.empty()) {
		if (exec[0] != '/') {
			wd = gmenu2x->getExePath();
		} else {
			string::size_type pos = exec.rfind("/");
			if (pos != string::npos)
				wd = exec.substr(0,pos);
		}
	}
	return wd;
}

void LinkApp::setWorkdir(const string &workdir) {
	this->workdir = workdir;
	edited = true;
}

const string &LinkApp::getManual() {
	return manual;
}

void LinkApp::setManual(const string &manual) {
	this->manual = manualPath = manual;
	edited = true;
}

const string &LinkApp::getSelectorDir() {
	return selectordir;
}

void LinkApp::setSelectorDir(const string &selectordir) {
	this->selectordir = selectordir;
	// if (this->selectordir!="" && this->selectordir[this->selectordir.length()-1]!='/') this->selectordir += "/";
	if (!this->selectordir.empty()) this->selectordir = real_path(this->selectordir);
	edited = true;
}

bool LinkApp::getSelectorBrowser() { return this->selectorbrowser; }
void LinkApp::setSelectorBrowser(bool value) {
	selectorbrowser = value;
	edited = true;
}

bool LinkApp::getConsoleApp() { return this->consoleapp; }
void LinkApp::setConsoleApp(bool value) {
	this->consoleapp = value;
	this->edited = true;
}

const string &LinkApp::getSelectorFilter() { return this->selectorfilter; }
void LinkApp::setSelectorFilter(const string &selectorfilter) {
	this->selectorfilter = selectorfilter;
	edited = true;
}

const string &LinkApp::getSelectorScreens() {
	return selectorscreens;
}

void LinkApp::setSelectorScreens(const string &selectorscreens) {
	this->selectorscreens = selectorscreens;
	edited = true;
}

const string &LinkApp::getAliasFile() {
	return aliasfile;
}

void LinkApp::setAliasFile(const string &aliasfile) {
	if (fileExists(aliasfile)) {
		this->aliasfile = aliasfile;
		edited = true;
	}
}

void LinkApp::setProvider(const string &provider) {
	this->provider = provider;
	this->edited = true;
}

void LinkApp::setProviderMetadata(const string &metadata) {
	this->providerMetadata = metadata;
	this->edited = true;
}

void LinkApp::renameFile(const string &name) {
	file = name;
}

std::string LinkApp::toString() {
	
	std::ostringstream out;
	{
		if (title != ""          ) out << "title="           << title           << endl;
		if (description != ""    ) out << "description="     << description     << endl;
		if (icon != ""           ) out << "icon="            << icon            << endl;
		if (exec != ""           ) out << "exec="            << exec            << endl;
		if (params != ""         ) out << "params="          << params          << endl;
		if (workdir != ""        ) out << "workdir="         << workdir         << endl;
		if (consoleapp           ) out << "consoleapp=true"                     << endl;
		if (!consoleapp          ) out << "consoleapp=false"                    << endl;
		if (manual != ""         ) out << "manual="          << manual          << endl;
		if (!selectorbrowser     ) out << "selectorBrowser=false"               << endl;
		if (selectorfilter != "" ) out << "selectorFilter="  << selectorfilter  << endl;
		if (selectorscreens != "") out << "selectorscreens=" << selectorscreens << endl;
		if (!provider.empty())     out << "X-Provider="      << provider        << endl;
		if (!providerMetadata.empty())     out << "X-ProviderMetadata=" << providerMetadata        << endl;
		if (aliasfile != ""      ) out << "selectoraliases=" << aliasfile       << endl;
		if (backdrop != ""       ) out << "backdrop="        << backdrop        << endl;
		if (iclock != 0              ) out << "clock="           << iclock          << endl;
		if (!selectordir.empty()     ) out << "selectordir="     << selectordir     << endl;
		if (!selectorbrowser         ) out << "selectorbrowser=false"               << endl;
	}
	return out.str();

}