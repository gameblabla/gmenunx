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
#include <cstdlib>

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
#include "stringutils.h"

static std::array<const char *, 4> tokens = { "%f", "%F", "%u", "%U", };
const std::string LinkApp::FAVOURITE_FOLDER = "favourites";

LinkApp::LinkApp(Esoteric *app, const char* linkfile, bool deletable_) :
	Link(app, fastdelegate::MakeDelegate(this, &LinkApp::run)) {

	TRACE("ctor - handling file :%s", linkfile);

	TRACE("ctor - set default CPU speed");
	this->setClock(app->hw->Cpu()->getDefaultValue());

	this->file = linkfile;
	this->manual = "";
	this->manualPath = "";
	this->selectordir = "";
	this->selectorfilter = "";
	this->icon = iconPath = "";
	this->selectorbrowser = true;
	this->selectorscreens ="";
	this->consoleapp = true;
	this->hidden = false;
	this->workdir = "";
	this->backdrop = "";
	this->backdropPath = "";
	this->editable = this->deletable = deletable_;
	this->params = "";
	this->exec = "";
	this->provider = "";
	this->providerMetadata = "";

	TRACE("ctor - creating ifstream");
	std::ifstream infile (linkfile, std::ios_base::in);
	std::locale loc("");
	infile.imbue(loc);
	if (infile.is_open()) {
		TRACE("ctor - iterating thru desktop file : %s", linkfile);
		try {
			std::string line;
			while (std::getline(infile, line, '\n')) {

				TRACE("read raw line : %s", line.c_str());
				line = StringUtils::trim(line);
				if (line.empty()) continue;
				if (line[0] == '#') continue;

				std::size_t position = line.find("=");
				std::string name = StringUtils::toLower(StringUtils::trim(line.substr(0, position)));
				std::string value = StringUtils::trim(line.substr(position + 1));

				try {
					if (name == "clock") {
						this->setClock(value);
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
					} else if (name == "hidden") {
						this->setHidden(value == "true" ? true : false);
					} else if (name == "selectorfilter") {
						this->setSelectorFilter( value );
					} else if (name == "selectorscreens") {
						this->setSelectorScreens( value );
					} else if (name == "selectoralias") {
						this->setAliasFile( value );
					} else if (name == "backdrop") {
						this->setBackdrop(value);
					} else if (name == "x-provider") {
						this->setProvider(value);
					} else if (name == "x-providermetadata") {
						this->setProviderMetadata(value);
					} else {
						INFO("Unrecognized native link option: '%s' in %s", name.c_str(), linkfile);
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
		searchIcon(this->exec, true);
	}

	TRACE("ctor exit : %s", this->toString().c_str());
	this->edited = false;
}

const std::string &LinkApp::searchManual() {
	if (!manualPath.empty()) return manualPath;
	std::string filename = exec;
	std::string::size_type pos = exec.rfind(".");
	if (pos != std::string::npos) filename = exec.substr(0, pos);
	filename += ".man.txt";

	std::string dname = FileUtils::dirName(exec) + "/";

	std::string dirtitle = dname + FileUtils::pathBaseName(FileUtils::dirName(exec)) + ".man.txt";
	std::string linktitle = FileUtils::pathBaseName(this->file);
	pos = linktitle.rfind(".");
	if (pos != std::string::npos) linktitle = linktitle.substr(0, pos);
	linktitle = dname + linktitle + ".man.txt";

	if (FileUtils::fileExists(linktitle))
		manualPath = linktitle;
	else if (FileUtils::fileExists(filename))
		manualPath = filename;
	else if (FileUtils::fileExists(dirtitle))
		manualPath = dirtitle;

	return manualPath;
}

const std::string &LinkApp::searchBackdrop() {
	if (!backdropPath.empty() || !this->app->skin->skinBackdrops) 
		return backdropPath;

	std::string execicon = exec;
	std::string::size_type pos = exec.rfind(".");
	if (pos != std::string::npos) execicon = exec.substr(0, pos);
	std::string exectitle = FileUtils::pathBaseName(execicon);
	std::string dirtitle = FileUtils::pathBaseName(FileUtils::dirName(exec));
	std::string linktitle = FileUtils::pathBaseName(file);
	pos = linktitle.rfind(".");
	if (pos != std::string::npos) linktitle = linktitle.substr(0, pos);

	// auto backdrop
	if (!app->skin->getSkinFilePath("backdrops/" + linktitle + ".png").empty())
		backdropPath = app->skin->getSkinFilePath("backdrops/" + linktitle + ".png");
	else if (!app->skin->getSkinFilePath("backdrops/" + linktitle + ".jpg").empty())
		backdropPath = app->skin->getSkinFilePath("backdrops/" + linktitle + ".jpg");
	else if (!app->skin->getSkinFilePath("backdrops/" + exectitle + ".png").empty())
		backdropPath = app->skin->getSkinFilePath("backdrops/" + exectitle + ".png");
	else if (!app->skin->getSkinFilePath("backdrops/" + exectitle + ".jpg").empty())
		backdropPath = app->skin->getSkinFilePath("backdrops/" + exectitle + ".jpg");
	else if (!app->skin->getSkinFilePath("backdrops/" + dirtitle + ".png").empty())
		backdropPath = app->skin->getSkinFilePath("backdrops/" + dirtitle + ".png");
	else if (!app->skin->getSkinFilePath("backdrops/" + dirtitle + ".jpg").empty())
		backdropPath = app->skin->getSkinFilePath("backdrops/" + dirtitle + ".jpg");

	return backdropPath;
}

const std::string &LinkApp::searchIcon() {
	return searchIcon(exec, true);
}

const std::string &LinkApp::searchIcon(std::string path, bool fallBack) {
	TRACE("enter - fallback : %i", fallBack);

	// get every permutation possible from the metadata opts
	std::string execicon = path;
	std::string::size_type pos = path.rfind(".");
	if (pos != std::string::npos) execicon = path.substr(0, pos);
	execicon += ".png";
	std::string exectitle = FileUtils::pathBaseName(execicon);
	std::string dirtitle = FileUtils::pathBaseName(FileUtils::dirName(path)) + ".png";
	std::string linktitle = FileUtils::pathBaseName(file);
	pos = linktitle.rfind(".");
	if (pos != std::string::npos) linktitle = linktitle.substr(0, pos);
	linktitle += ".png";

	if (!app->skin->getSkinFilePath("icons/" + linktitle).empty())
		iconPath = app->skin->getSkinFilePath("icons/" + linktitle);
	else if (!app->skin->getSkinFilePath("icons/" + exectitle).empty())
		iconPath = app->skin->getSkinFilePath("icons/" + exectitle);
	else if (!app->skin->getSkinFilePath("icons/" + dirtitle).empty())
		iconPath = app->skin->getSkinFilePath("icons/" + dirtitle);
	else if (FileUtils::fileExists(execicon))
		iconPath = execicon;
	else if (fallBack) {
		iconPath = app->skin->getSkinFilePath("icons/generic.png");
	}

	TRACE("exit - iconPath : %s", iconPath.c_str());
	return iconPath;
}

std::string LinkApp::getClock() {
	return this->clock;
}

void LinkApp::setClock(std::string val) {
	TRACE("enter : %s", val.c_str());
	if (val != this->clock) {
		this->clock = val;
		edited = true;
	}
}

void LinkApp::setBackdrop(const std::string selectedFile) {
	backdrop = backdropPath = selectedFile;
	edited = true;
}

bool LinkApp::targetExists() {
	std::string target = exec;
	if (!exec.empty() && exec[0] != '/' && !workdir.empty())
		target = workdir + "/" + exec;

	TRACE("looking for : %s", target.c_str());
	return FileUtils::fileExists(target);
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
		std::ofstream f(file.c_str());
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

void LinkApp::favourite(std::string launchArgs, std::string supportingFile, std::string backdrop) {
	TRACE("enter - launchArgs : %s, supportingFile : %s", 
		launchArgs.c_str(), 
		supportingFile.c_str());

	// need to make a new favourite
	std::string path = this->app->getWriteablePath() + "sections/" + FAVOURITE_FOLDER;
	if (!this->app->menu->sectionExists(FAVOURITE_FOLDER)) {
		if (!this->app->menu->addSection(FAVOURITE_FOLDER)) {
			 ERROR("LinkApp::selector - Couldn't make favourites folder : %s", path.c_str());
			 return;
		}
	}

	std::string cleanTitle = this->title;
	std::string desc = this->description;
	if (!supportingFile.empty()) {
		cleanTitle = FileUtils::fileBaseName(FileUtils::pathBaseName(supportingFile));
		desc = this->description + " - " + cleanTitle;
	}
	std::string favePath = path + "/" + this->title + "-" + cleanTitle + ".desktop";

	uint32_t x = 1;
	while (FileUtils::fileExists(favePath)) {
		std::string id = "";
		std::stringstream ss; ss << x; ss >> id;
		favePath = path + "/" + this->title + "-" + cleanTitle + "." + id + ".desktop";
		x++;
	}
	/*
	if (fileExists(favePath)) {
		TRACE("deleting existing favourite : %s", favePath.c_str());
		unlink(favePath.c_str());
	}
	*/

	LinkApp * fave = new LinkApp(this->app, favePath.c_str(), true);
	fave->setTitle(cleanTitle);
	fave->setIcon(this->icon);
	fave->setDescription(desc);
	fave->setManual(this->manual);
	fave->setConsoleApp(this->consoleapp);
	fave->setBackdrop(backdrop);
	TRACE("saving a favourite to : %s", favePath.c_str());
	TRACE("favourite exec path : %s", this->exec.c_str());
	TRACE("favourite params : %s", launchArgs.c_str());
	TRACE("favourite backdrop : %s", backdrop.c_str());
	fave->setExec(this->exec);
	fave->setParams(launchArgs);

	MessageBox mb(
		this->app, 
		"Savinging favourite - " + cleanTitle,  
		this->icon);
	mb.setAutoHide(500);
	mb.exec();
	if (fave->save()) {
		int secIndex = this->app->menu->getSectionIndex(FAVOURITE_FOLDER);
		this->app->menu->setSectionIndex(secIndex);
		this->app->menu->sectionLinks(secIndex)->push_back(fave);
		TRACE("favourite saved");
	} else delete fave;
	
	TRACE("exit");
}

void LinkApp::makeFavourite(std::string dir, std::string file, std::string backdrop) {
	TRACE("enter : '%s' - '%s' : '%s'", dir.c_str(), file.c_str(), backdrop.c_str());
	std::string launchArgs = this->resolveArgs(file, dir);
	this->favourite(launchArgs, file, backdrop);
}

void LinkApp::makeFavourite() {
	TRACE("enter");
	std::string launchArgs = this->resolveArgs();
	this->favourite(launchArgs);
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
void LinkApp::selector(int startSelection, const std::string &selectorDir, const bool &choose) {
	TRACE("enter - startSelection = %i, selectorDir = %s", startSelection, selectorDir.c_str());

	// Run selector interface - this is the order of dir specificity
	// - directly requested in the call args
	// - selector dir is set, and isn't the default that all initially get
	// - system wide last launch path
	// - default :: EXTERNAL_CARD_PATH
	std::string myDir = selectorDir;
	int selection = startSelection;
	if (myDir.empty()) {
		if (0 != this->selectordir.compare(EXTERNAL_CARD_PATH) && !this->selectordir.empty()) {
			myDir = this->selectordir;
		} else if (!this->app->config->launcherPath().empty()) {
			myDir = this->app->config->launcherPath();
		} else myDir = EXTERNAL_CARD_PATH;
	}

	Selector sel(app, this, myDir);
	if (!choose) {
		// we just want to get the file from dir and index
		sel.resolve(selection);
	} else {
		// we are doing a full resolve
		selection = sel.exec(selection);
	}

	// we got a file
	if (selection > -1) {
		std::string launchArgs = this->resolveArgs(sel.getFile(), sel.getDir());
		this->app->config->launcherPath(sel.getDir());
		this->app->writeTmp(selection, sel.getDir());
		this->launch(launchArgs);
	}
	TRACE("exit");
}

std::string LinkApp::resolveArgs(const std::string &selectedFile, const std::string &selectedDir) {
	TRACE("enter file : '%s', dir : '%s'", selectedFile.c_str(), selectedDir.c_str());
	std::string launchArgs = "";

	// selectedFile means a rom or some kind of data file..
	if (!selectedFile.empty()) {
		TRACE("we have a selected file to work with : %s", selectedFile.c_str());
		// break out the dir, filename and extension
		std::string selectedFileExtension;
		std::string selectedFileName;
		std::string dir;

		selectedFileExtension = FileUtils::fileExtension(selectedFile);
		selectedFileName = FileUtils::fileBaseName(selectedFile);

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

		if (this->getParams().empty()) {
			launchArgs = "\"" + dir + selectedFile + "\"";
			TRACE("no params, so cleaned to : %s", launchArgs.c_str());
		} else {
			TRACE("params need handling : %s", this->getParams().c_str());
			launchArgs = StringUtils::strReplace(params, "[selFullPath]", StringUtils::cmdClean(dir + selectedFile));
			launchArgs = StringUtils::strReplace(launchArgs, "[selPath]", StringUtils::cmdClean(dir));
			launchArgs = StringUtils::strReplace(launchArgs, "[selFile]", StringUtils::cmdClean(selectedFileName));
			launchArgs = StringUtils::strReplace(launchArgs, "[selExt]", StringUtils::cmdClean(selectedFileExtension));
			// if this is true, then we've made no subs, so we still need to add the selected file
			if (this->getParams() == launchArgs) {
				launchArgs += " \"" + dir + selectedFile + "\"";
			}

		}
		// save the last dir
		TRACE("setting the launcher path : %s", dir.c_str());
		app->config->launcherPath(dir);
	} else {
		TRACE("no file selected");
		TRACE("assigning params : '%s'", this->getParams().c_str());
		launchArgs = this->getParams();
	}
	TRACE("exit : %s", launchArgs.c_str());
	return launchArgs;
}

void LinkApp::launch(std::string launchArgs) {
	TRACE("enter - args: %si", launchArgs.c_str());

	//Set correct working directory
	std::string wd = getRealWorkdir();
	TRACE("real work dir = %s", wd.c_str());
	if (!wd.empty()) {
		chdir(wd.c_str());
	}

	// Check to see if permissions are desirable
	struct stat fstat;
	if ( stat( this->exec.c_str(), &fstat ) == 0 ) {
		struct stat newstat = fstat;
		if ( S_IRUSR != ( fstat.st_mode & S_IRUSR ) ) newstat.st_mode |= S_IRUSR;
		if ( S_IXUSR != ( fstat.st_mode & S_IXUSR ) ) newstat.st_mode |= S_IXUSR;
		if ( fstat.st_mode != newstat.st_mode ) chmod( exec.c_str(), newstat.st_mode );
	} // else, well.. we are no worse off :)

	std::vector<std::string> commandLine = { "/bin/sh", "-c" };
	std::string execute = this->exec + " " + launchArgs;
	TRACE("standard file cmd lime : %s",  execute.c_str());

	if (app->config->outputLogs()) {
		execute += " 2>&1 | tee " + StringUtils::cmdClean(app->getWriteablePath()) + "log.txt";
		TRACE("adding logging");
	}
	TRACE("final command : %s", execute.c_str());
	
	commandLine.push_back(execute);

	Launcher *toLaunch = new Launcher(commandLine, this->consoleapp);
	if (nullptr != toLaunch) {

		// set the cpu speed ?
		if (this->app->hw->Cpu()->overclockingSupported()) {
			// from desktop file, then app config, then cpu default
			if (!this->getClock().empty()) {
				TRACE("setting cpu based on desktop file : '%s'", this->getClock().c_str());
				this->app->hw->Cpu()->setValue(this->getClock());
			} else if (!this->app->config->defaultCpuSpeed().empty()) {
				TRACE("setting cpu based on config file : '%s'", this->app->config->defaultCpuSpeed().c_str());
				this->app->hw->Cpu()->setValue(this->app->config->defaultCpuSpeed());
			} else {
				TRACE("setting cpu based on hardware : '%s'", this->app->hw->Cpu()->getDefaultValue().c_str());
				this->app->hw->Cpu()->setDefault();
			}
		}
		unsetenv("SDL_FBCON_DONT_CLEAR");

		TRACE("calling : quit");
		TRACE("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
		this->app->quit();
		TRACE("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
		TRACE("quit completed");
		TRACE("calling : release screen");
		TRACE("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
		this->app->releaseScreen();
		TRACE("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
		TRACE("release screen completed");

		TRACE("calling exec");
		toLaunch->exec();
		TRACE("exec called");
		// If control gets here, execution failed. Since we already destructed
		// everything, the easiest solution is to exit and let the system
		// respawn the menu.
		delete toLaunch;
		Esoteric::quit_all(0);
	}

	TRACE("exit");
}

const std::string &LinkApp::getExec() { return exec; }
void LinkApp::setExec(const std::string &exec) {
	this->exec = exec;
	edited = true;
}

const std::string &LinkApp::getParams() { return params; }
void LinkApp::setParams(const std::string &params) {
	this->params = params;
	edited = true;
}

const std::string &LinkApp::getWorkdir() { return workdir; }

const std::string LinkApp::getRealWorkdir() {
	std::string wd = workdir;
	if (wd.empty()) {
		if (exec[0] != '/') {
			wd = app->getExePath();
		} else {
			std::string::size_type pos = exec.rfind("/");
			if (pos != std::string::npos)
				wd = exec.substr(0,pos);
		}
	}
	return wd;
}

void LinkApp::setWorkdir(const std::string &workdir) {
	this->workdir = workdir;
	edited = true;
}

const std::string &LinkApp::getManual() { return manual; }
void LinkApp::setManual(const std::string &manual) {
	this->manual = this->manualPath = manual;
	edited = true;
}

const std::string &LinkApp::getSelectorDir() { return selectordir; }
void LinkApp::setSelectorDir(const std::string &selectordir) {
	this->selectordir = selectordir;
	// if (this->selectordir!="" && this->selectordir[this->selectordir.length()-1]!='/') this->selectordir += "/";
	if (!this->selectordir.empty()) this->selectordir = FileUtils::resolvePath(this->selectordir);
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

bool LinkApp::getHidden() { return this->hidden; }
void LinkApp::setHidden(bool value) {
	this->hidden = value;
	this->edited = true;
}

const std::string &LinkApp::getSelectorFilter() { return this->selectorfilter; }
void LinkApp::setSelectorFilter(const std::string &selectorfilter) {
	this->selectorfilter = selectorfilter;
	edited = true;
}

const std::string &LinkApp::getSelectorScreens() { return selectorscreens; }
void LinkApp::setSelectorScreens(const std::string &selectorscreens) {
	this->selectorscreens = selectorscreens;
	edited = true;
}

const std::string &LinkApp::getAliasFile() { return aliasfile; }
void LinkApp::setAliasFile(const std::string &aliasfile) {
	if (FileUtils::fileExists(aliasfile)) {
		this->aliasfile = aliasfile;
		edited = true;
	}
}

void LinkApp::setProvider(const std::string &provider) {
	this->provider = provider;
	this->edited = true;
}
void LinkApp::setProviderMetadata(const std::string &metadata) {
	this->providerMetadata = metadata;
	this->edited = true;
}

void LinkApp::renameFile(const std::string &name) { file = name; }

std::string LinkApp::toString() {
	
	std::ostringstream out;
	{
		if (!title.empty()            ) out << "title="              << title            << std::endl;
		if (!description.empty()      ) out << "description="        << description      << std::endl;
		if (!icon.empty()             ) out << "icon="               << icon             << std::endl;
		if (!exec.empty()             ) out << "exec="               << exec             << std::endl;
		if (!params.empty()           ) out << "params="             << params           << std::endl;
		if (!workdir.empty()          ) out << "workdir="            << workdir          << std::endl;
		if (consoleapp                ) out << "consoleapp=true"                         << std::endl;
		if (!consoleapp               ) out << "consoleapp=false"                        << std::endl;
		if (hidden                    ) out << "hidden=true"                             << std::endl;
		if (!manual.empty()           ) out << "manual="             << manual           << std::endl;
		if (selectorbrowser           ) out << "selectorBrowser=true"                    << std::endl;
		if (!selectorbrowser          ) out << "selectorbrowser=false"                   << std::endl;
		if (!selectorfilter.empty()   ) out << "selectorFilter="     << selectorfilter   << std::endl;
		if (!selectorscreens.empty()  ) out << "selectorscreens="    << selectorscreens  << std::endl;
		if (!provider.empty()         ) out << "X-Provider="         << provider         << std::endl;
		if (!providerMetadata.empty() ) out << "X-ProviderMetadata=" << providerMetadata << std::endl;
		if (!aliasfile.empty()        ) out << "selectoralias="      << aliasfile        << std::endl;
		if (!backdrop.empty()         ) out << "backdrop="           << backdrop         << std::endl;
		if (!getClock().empty()       ) out << "clock="              << this->getClock() << std::endl;
		if (!selectordir.empty()      ) out << "selectordir="        << selectordir      << std::endl;
		if (!selectorbrowser          ) out << "selectorbrowser=false"                   << std::endl;
	}
	return out.str();

}