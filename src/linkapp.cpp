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

using namespace std;

static array<const char *, 4> tokens = { "%f", "%F", "%u", "%U", };
static const string OPKRUN_PATH = "/usr/bin/opkrun";
const string LinkApp::FAVOURITE_FOLDER = "favourites";

LinkApp::LinkApp(GMenu2X *gmenu2x_, const char* linkfile, bool deletable_, struct OPK *opk, const char *metadata_) :
	Link(gmenu2x_, MakeDelegate(this, &LinkApp::run)),
	inputMgr(gmenu2x->input) {

	TRACE("LinkApp::LinkApp - ctor - enter - linkfile : %s", linkfile);
	manual = manualPath = "";
	file = linkfile;

	TRACE("LinkApp::LinkApp - ctor - setCPU");
	setCPU(gmenu2x->config->cpuMenu());

	selectordir = "";
	selectorfilter = "";
	icon = iconPath = "";
	selectorbrowser = true;
	consoleapp = true;
	workdir = "";
	backdrop = backdropPath = "";

	deletable = deletable_;

	TRACE("LinkApp::LinkApp - ctor - setting opk by testing value of : %i", opk);
	isOPK = (0 != opk);
	TRACE("LinkApp::LinkApp - ctor - setting metadata : %s", metadata_);

	bool appTakesFileArg = true;
/* ---------------------------------------------------------------- */

	if (isOPK) {
		TRACE("LinkApp::LinkApp - ctor - handling opk :%s", file.c_str());
		// let's override these guys for sure
		deletable = editable = false;

		string::size_type pos;
		const char *key, *val;
		size_t lkey, lval;
		int ret;

		metadata.assign(metadata_);
		opkFile = file;
		pos = file.rfind('/');
		opkMount = file.substr(pos+1);
		pos = opkMount.rfind('.');
		opkMount = opkMount.substr(0, pos);
		string metaIcon;

		TRACE("LinkApp::LinkApp - ctor - opkMount : %s", opkMount.c_str());
		appTakesFileArg = false;
		category = "applications";

		TRACE("LinkApp::LinkApp - ctor - reading pairs from meta begins");
		while ((ret = opk_read_pair(opk, &key, &lkey, &val, &lval))) {
			if (ret < 0) {
				ERROR("Unable to read meta-data");
				break;
			}

			char buf[lval + 1];
			sprintf(buf, "%.*s", lval, val);

			if (!strncmp(key, "Categories", lkey)) {
				category = buf;
				pos = category.find(';');
				if (pos != category.npos)
					category = category.substr(0, pos);
				TRACE("LinkApp::LinkApp - ctor - opk::category : %s", category.c_str());
			} else if (!strncmp(key, "Name", lkey)) {
				title = buf;
				TRACE("LinkApp::LinkApp - ctor - opk::title : %s", title.c_str());
			} else if (!strncmp(key, "Comment", lkey) && description.empty()) {
				description = buf;
				TRACE("LinkApp::LinkApp - ctor - opk::description : %s", description.c_str());
			} else if (!strncmp(key, "Terminal", lkey)) {
				consoleapp = !strncmp(val, "true", lval);
				TRACE("LinkApp::LinkApp - ctor - opk::consoleapp : %i", consoleapp);
			} else if (!strncmp(key, "X-OD-Manual", lkey)) {
				manual = buf;
				TRACE("LinkApp::LinkApp - ctor - opk::manual : %s", manual.c_str());
			} else if (!strncmp(key, "Icon", lkey)) {
				metaIcon = (string)buf;
				TRACE("LinkApp::LinkApp - ctor - opk::icon : %s", metaIcon.c_str());
			} else if (!strncmp(key, "Exec", lkey)) {
				exec = buf;
				TRACE("LinkApp::LinkApp - ctor - opk::raw exec : %s", exec.c_str());
				for (auto token : tokens) {
					if (exec.find(token) != exec.npos) {
						TRACE("LinkApp::LinkApp - ctor - opk::exec takes a token");
						selectordir = CARD_ROOT;
						appTakesFileArg = true;
						break;
					}
				}
				continue;
			} else if (!strncmp(key, "selectordir", lkey)) {
				TRACE("LinkApp::LinkApp - ctor - opk::selector dir : %s", buf);
				setSelectorDir(buf);
			} else if (!strncmp(key, "selectorfilter", lkey)) {
				TRACE("LinkApp::LinkApp - ctor - opk::selector filter : %s", buf);
				setSelectorFilter(buf);
			} else {
				WARNING("Unrecognized OPK link option: '%s'", key);
			}
		}

		// let's sort out icons
		string shortFileName = metaIcon + ".png";
		string ip = "icons/" + shortFileName;
		TRACE("LinkApp::LinkApp - ctor - looking for icon at : %s", ip.c_str());
		// Read the icon from the OPK if it doesn't exist on the skin
		searchIcon(metaIcon, false);
		if (!iconPath.empty()) {
			TRACE("LinkApp::LinkApp - ctor - icon found in local skin");
			this->icon = iconPath;
		} else {
			// let's extract the image and use for now, and save it locally for the future
			TRACE("LinkApp::LinkApp - ctor - icon not found, looking in the opk");
			string opkImagePath = file + "#" + shortFileName;
			TRACE("LinkApp::LinkApp - ctor - loading OPK icon from : %s", opkImagePath.c_str());
			SDL_Surface *tmpIcon = loadPNG(opkImagePath, true);
			if (tmpIcon) {
				TRACE("LinkApp::LinkApp - ctor - loaded opk icon ok");
				string outPath = gmenu2x->skin->currentSkinPath() + "/icons";
				if (dirExists(outPath)) {
					string outFile = outPath + "/" + basename(shortFileName.c_str());
					TRACE("LinkApp::LinkApp - ctor - saving icon to : %s", outFile.c_str());
					if (0 == saveSurfacePng((char*)outFile.c_str(), tmpIcon)) {
						string workingName = "skin:" + ip;
						TRACE("LinkApp::LinkApp - ctor - working name is : %s", workingName.c_str());
						this->icon = workingName;
					} else {
						ERROR("LinkApp::LinkApp - ctor - couldn't save icon png");
					}
				} else {
					ERROR("LinkApp::LinkApp - ctor - directory doesn't exist : %s", outPath.c_str());
				}
			} else {
				TRACE("LinkApp::LinkApp - ctor - loaded opk icon failed");
				this->icon = opkImagePath;
			}
		}

		TRACE("LinkApp::LinkApp - ctor - opk::icon set to : %s", this->icon.c_str());

		// end of icons

		// update the filename to point at the opk's section category and friendly name
		file = gmenu2x->getAssetsPath() + "/sections/" + category + '/' + opkMount;
		// define the mount point
		opkMount = (string) "/mnt/" + opkMount + '/';
	}	// isOPK

	else {
		this->editable = this->deletable = deletable;
		TRACE("LinkApp::LinkApp - ctor - handling normal desktop file :%s", linkfile);
		string line;
		TRACE("LinkApp::LinkApp - ctor - creating ifstream");
		ifstream infile (linkfile, ios_base::in);
		TRACE("LinkApp::LinkApp - ctor - iterating thru infile : %s", linkfile);
		while (getline(infile, line, '\n')) {

			line = trim(line);
			if (line.empty()) continue;
			if (line[0] == '#') continue;

			string::size_type position = line.find("=");
			string name = trim(line.substr(0,position));
			string value = trim(line.substr(position+1));

			if (name == "clock") {
				setCPU( atoi(value.c_str()) );
			} else if (name == "selectordir") {
				setSelectorDir( value );
			} else if (name == "selectorbrowser") {
				selectorbrowser = (value == "true" ? true : false);
			} else if (name == "title") {
					title = value;
			} else if (name == "description") {
					description = value;
			} else if (name == "icon") {
					TRACE("LinkApp::LinkApp - ctor - setting icon value to %s", value.c_str());
					setIcon(value);
			} else if (name == "exec") {
					exec = value;
			} else if (name == "params") {
					params = value;
			} else if (name == "workdir") {
					workdir = value;
			} else if (name == "manual") {
					setManual(value);
			} else if (name == "consoleapp") {
					consoleapp = (value == "true" ? true : false);
			} else if (name == "selectorfilter") {
					setSelectorFilter( value );
			} else if (name == "selectorscreens") {
					setSelectorScreens( value );
			} else if (name == "selectoraliases") {
					setAliasFile( value );
			} else if (name == "backdrop") {
					setBackdrop(value);
					// WARNING("BACKDROP: '%s'", backdrop.c_str());
			} else {
				WARNING("Unrecognized native link option: '%s'", name.c_str());
				break;
			}

		}
		TRACE("LinkApp::LinkApp - ctor - closing infile");
		infile.close();

		// try and find an icon 
		if (iconPath.empty()) {
			TRACE("LinkApp::LinkApp - ctor - searching for icon");
			searchIcon(exec, true);
		}

	}	// !opk

	TRACE("LinkApp::LinkApp - ctor exit : %s", this->toString().c_str());
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
	TRACE("LinkApp::searchIcon - enter - fallback : %i", fallBack);

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

	TRACE("LinkApp::searchIcon - exit - iconPath : %s", iconPath.c_str());
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

	TRACE("LinkApp::targetExists - looking for : %s", target.c_str());
	return fileExists(target);
}

bool LinkApp::save() {
	TRACE("LinkApp::save - enter : %s", file.c_str());
	if (!edited) {
		TRACE("LinkApp::save - not edited, nothing to save");
		return false;
	}
	if (isOpk()) {
		TRACE("LinkApp::save - OPK, nothing to save");
		return false;
	}

	std::ostringstream out;
	out << this->toString();

	if (out.tellp() > 0) {
		TRACE("LinkApp::save - saving data : %s", out.str().c_str());
		ofstream f(file.c_str());
		if (f.is_open()) {
			f << out.str();
			f.close();
			sync();
			TRACE("LinkApp::save - save success");
			return true;
		} else {
			ERROR("LinkApp::save - Error while opening the file '%s' for write.\n", file.c_str());
			return false;
		}
	} else {
		TRACE("LinkApp::save - Empty app settings: %s\n", file.c_str());
		return unlink(file.c_str()) == 0 || errno == ENOENT;
	}
}

void LinkApp::favourite(string launchArgs, string supportingFile) {
	TRACE("LinkApp::favourite - enter - launchArgs : %s, supportingFile : %s", 
		launchArgs.c_str(), 
		supportingFile.c_str());

	// need to make a new favourite
	string path = this->gmenu2x->getAssetsPath() + "sections/" + FAVOURITE_FOLDER;
	if (!this->gmenu2x->menu->sectionExists(FAVOURITE_FOLDER)) {
		if (!this->gmenu2x->menu->addSection(FAVOURITE_FOLDER)) {
			 ERROR("LinkApp::selector - Couldn't make favourites folder : $s", path.c_str());
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
		TRACE("LinkApp::selector - deleting existing favourite : %s", favePath.c_str());
		unlink(favePath.c_str());
	}
	*/

	LinkApp * fave = new LinkApp(this->gmenu2x, favePath.c_str(), true, NULL, NULL);
	fave->setTitle(cleanTitle);
	fave->setIcon(this->icon);
	fave->setDescription(description);
	fave->consoleapp = this->consoleapp;

	if (this->isOPK) {
		string opkParams = "-m " + this->metadata + " " + cmdclean(this->opkFile) + " " + launchArgs;
		TRACE("LinkApp::selector - saving an opk favourite to : %s", favePath.c_str());
		TRACE("LinkApp::selector - opk exec path : %s", OPKRUN_PATH.c_str());
		TRACE("LinkApp::selector - opk params : %s", opkParams.c_str());
		fave->setExec(OPKRUN_PATH);
		fave->setParams(opkParams);
	} else {
		TRACE("LinkApp::selector - saving a normal favourite to : %s", favePath.c_str());
		TRACE("LinkApp::selector - normal exec path : %s", this->exec.c_str());
		TRACE("LinkApp::selector - normal params : %s", launchArgs.c_str());
		fave->setExec(this->exec);
		fave->setParams(launchArgs);
	}

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
		TRACE("LinkApp::selector - favourite saved");
	} else delete fave;
	
	TRACE("LinkApp::favourite - exit");
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
	TRACE("LinkApp::run - enter");
	if (!selectordir.empty()) {
		selector();
	} else {
		string launchArgs = resolveArgs();
		launch(launchArgs);
	}
}

/*
 * lauches a supporting file selector if needed
 */
void LinkApp::selector(int startSelection, const string &selectorDir) {
	TRACE("LinkApp::selector - enter - startSelection = %i, selectorDir = %s", startSelection, selectorDir.c_str());

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
			TRACE("LinkApp::selector - we're saving a favourite");
			string romFile = sel.getDir() + sel.getFile();
			TRACE("LinkApp::selector - launchArgs : %s, romFile : %s", launchArgs.c_str(), romFile.c_str());
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
		TRACE("LinkApp::launch - we have a selected file to work with : %s", selectedFile.c_str());
		// break out the dir, filename and extension
		string selectedFileExtension;
		string selectedFileName;
		string dir;

		selectedFileExtension = fileExtension(selectedFile);
		selectedFileName = fileBaseName(selectedFile);

		TRACE("LinkApp::launch - name : %s, extension : %s", selectedFileName.c_str(), selectedFileExtension.c_str());
		if (selectedDir.empty())
			dir = getSelectorDir();
		else
			dir = selectedDir;

		// force a slash at the end
		if (dir.length() > 0) {
			if (0 != dir.compare(dir.length() -1, 1, "/")) 
				dir += "/";
		} else dir = "/";
		TRACE("LinkApp::launch - dir : %s", dir.c_str());

		if (params.empty()) {
			launchArgs = "\"" + dir + selectedFile + "\"";
			TRACE("LinkApp::launch - no params, so cleaned to : %s", launchArgs.c_str());
		} else {
			
			launchArgs = strreplace(params, "[selFullPath]", cmdclean(dir + selectedFile));
			launchArgs = strreplace(launchArgs, "[selPath]", cmdclean(dir));
			launchArgs = strreplace(launchArgs, "[selFile]", cmdclean(selectedFileName));
			launchArgs = strreplace(launchArgs, "[selExt]", cmdclean(selectedFileExtension));
			// if this is true, then we've made no subs, so we still need to add the selected file
			if (params == launchArgs) 
				launchArgs += " \"" + dir + selectedFile + "\"";

		}
		// save the last dir
		gmenu2x->config->launcherPath(dir);
	} else launchArgs = params;

	return launchArgs;

}

void LinkApp::launch(string launchArgs) {
	TRACE("LinkApp::launch - enter - args: %si", launchArgs.c_str());

	if (!isOpk()) {
		TRACE("LinkApp::launch - not an opk");
		//Set correct working directory
		string wd = getRealWorkdir();
		TRACE("LinkApp::launch - real work dir = %s", wd.c_str());
		if (!wd.empty()) {
			chdir(wd.c_str());
			TRACE("LinkApp::launch - changed into wrkdir");
		}
	}

	if (gmenu2x->config->saveSelection()) {
		TRACE("LinkApp::launch - updating selections");
		gmenu2x->config->section(gmenu2x->menu->selSectionIndex());
		gmenu2x->config->link(gmenu2x->menu->selLinkIndex());
	}

	vector<string> commandLine;
	commandLine = { "/bin/sh", "-c" };
	string execute;

	if (isOpk()) {
		execute = "/usr/bin/opkrun -m " + metadata + " \"" + opkFile + "\"";
		TRACE("LinkApp::launch - running an opk via : %s", execute.c_str());
		if (!launchArgs.empty()) {
			TRACE("LinkApp::launch - running an opk with extra params : %s", launchArgs.c_str());
			execute += " " + launchArgs;
		}

	} else {
		TRACE("LinkApp::launch - running a standard desktop file");
		// Check to see if permissions are desirable
		struct stat fstat;
		if ( stat( exec.c_str(), &fstat ) == 0 ) {
			struct stat newstat = fstat;
			if ( S_IRUSR != ( fstat.st_mode & S_IRUSR ) ) newstat.st_mode |= S_IRUSR;
			if ( S_IXUSR != ( fstat.st_mode & S_IXUSR ) ) newstat.st_mode |= S_IXUSR;
			if ( fstat.st_mode != newstat.st_mode ) chmod( exec.c_str(), newstat.st_mode );
		} // else, well.. we are no worse off :)

		execute = exec + " " + launchArgs;
		TRACE("LinkApp::launch - standard file cmd lime : %s %s",  execute.c_str());
	}
	if (gmenu2x->config->outputLogs()) {
		execute += " 2>&1 | tee " + cmdclean(gmenu2x->getAssetsPath()) + "log.txt";
		TRACE("LinkApp::launch - adding logging");
	}
	TRACE("LinkApp::launch - Final command : %s", execute.c_str());
	commandLine.push_back(execute);

	Launcher *toLaunch = new Launcher(commandLine, consoleapp);
	if (toLaunch) {
		unsetenv("SDL_FBCON_DONT_CLEAR");

		TRACE("LinkApp::launch - quit");
		gmenu2x->quit();

		TRACE("LinkApp::launch - calling exec");
		toLaunch->exec();
		// If control gets here, execution failed. Since we already destructed
		// everything, the easiest solution is to exit and let the system
		// respawn the menu.
		delete toLaunch;
	}

	TRACE("LinkApp::launch - exit");
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

bool LinkApp::getSelectorBrowser() {
	return selectorbrowser;
}

void LinkApp::setSelectorBrowser(bool value) {
	selectorbrowser = value;
	edited = true;
}

const string &LinkApp::getSelectorFilter() {
	return selectorfilter;
}

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

void LinkApp::renameFile(const string &name) {
	file = name;
}

std::string LinkApp::toString() {
	
	std::ostringstream out;
	if (isOpk()) {
		if (!category.empty()) out << "Categories=" << category           << endl;
		if (!title.empty()) out << "Name=" << title << endl;
		if (!description.empty()) out << "Comment=" << description << endl;
		if (consoleapp           ) out << "Terminal=true"                     << endl;
		if (!consoleapp          ) out << "Terminal=false"                    << endl;
		if (!manual.empty()) out << "X-OD-Manual=" << manual << endl;
		if (!this->icon.empty()) out << "Icon=" << this->icon << endl;
		if (!exec.empty()) out << "Exec=" << exec << endl;
	} else {
		if (title != ""          ) out << "title="           << title           << endl;
		if (description != ""    ) out << "description="     << description     << endl;
		if (icon != ""           ) out << "icon="            << icon            << endl;
		if (exec != ""           ) out << "exec="            << exec            << endl;
		if (params != ""         ) out << "params="          << params          << endl;
		if (workdir != ""        ) out << "workdir="         << workdir         << endl;
		if (consoleapp           ) out << "consoleapp=true"                     << endl;
		if (!consoleapp          ) out << "consoleapp=false"                    << endl;
		if (manual != ""         ) out << "manual="          << manual          << endl;
		if (!selectorbrowser     ) out << "selectorbrowser=false"               << endl;
		if (selectorfilter != "" ) out << "selectorfilter="  << selectorfilter  << endl;
		if (selectorscreens != "") out << "selectorscreens=" << selectorscreens << endl;
		if (aliasfile != ""      ) out << "selectoraliases=" << aliasfile       << endl;
		if (backdrop != ""       ) out << "backdrop="        << backdrop        << endl;
		if (iclock != 0              ) out << "clock="           << iclock          << endl;
		if (!selectordir.empty()     ) out << "selectordir="     << selectordir     << endl;
		if (!selectorbrowser         ) out << "selectorbrowser=false"               << endl;
	}
	return out.str();

}