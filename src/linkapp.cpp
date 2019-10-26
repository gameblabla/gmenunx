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

#ifdef HAVE_LIBXDGMIME
#include <xdgmime.h>
#endif

#include "linkapp.h"
#include "launcher.h"
#include "menu.h"
#include "selector.h"
#include "debug.h"

using namespace std;

static array<const char *, 4> tokens = { "%f", "%F", "%u", "%U", };

LinkApp::LinkApp(GMenu2X *gmenu2x_, InputManager &inputMgr_, const char* linkfile, bool deletable_, struct OPK *opk, const char *metadata_) :
	Link(gmenu2x_, MakeDelegate(this, &LinkApp::run)),
	inputMgr(inputMgr_)
{
	TRACE("LinkApp::LinkApp - ctor - enter");
	manual = manualPath = "";
	file = linkfile;

	TRACE("LinkApp::LinkApp - ctor - setCPU");
	setCPU(gmenu2x->confInt["cpuMenu"]);

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
		TRACE("LinkApp::LinkApp - ctor - handling opk");
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

		appTakesFileArg = false;
		category = "applications";

		while ((ret = opk_read_pair(opk, &key, &lkey, &val, &lval))) {
			if (ret < 0) {
				ERROR("Unable to read meta-data\n");
				break;
			}

			char buf[lval + 1];
			sprintf(buf, "%.*s", lval, val);

			if (!strncmp(key, "Categories", lkey)) {
				category = buf;

				pos = category.find(';');
				if (pos != category.npos)
					category = category.substr(0, pos);

			} else if ((!strncmp(key, "Name", lkey) && title.empty())
						|| !strncmp(key, ("Name[" + gmenu2x->tr["Lng"] +
								"]").c_str(), lkey)) {
				title = buf;

			} else if ((!strncmp(key, "Comment", lkey) && description.empty())
						|| !strncmp(key, ("Comment[" +
								gmenu2x->tr["Lng"] + "]").c_str(), lkey)) {
				description = buf;

			} else if (!strncmp(key, "Terminal", lkey)) {
				consoleapp = !strncmp(val, "true", lval);

			} else if (!strncmp(key, "X-OD-Manual", lkey)) {
				manual = buf;

			} else if (!strncmp(key, "Icon", lkey)) {
				// Read the icon from the OPK only
				// if it doesn't exist on the skin
				this->icon = gmenu2x->sc.getSkinFilePath("icons/" + (string) buf + ".png");
				if (this->icon.empty()) {
					this->icon = linkfile + '#' + (string) buf + ".png";
				}
				iconPath = this->icon;
				updateSurfaces();

			} else if (!strncmp(key, "Exec", lkey)) {
				string tmp = buf;

				for (auto token : tokens) {
					if (tmp.find(token) != tmp.npos) {
						selectordir = CARD_ROOT;
						appTakesFileArg = true;
						break;
					}
				}

				continue;
			}

#ifdef HAVE_LIBXDGMIME
			if (!strncmp(key, "MimeType", lkey)) {
				string mimetypes = buf;
				selectorfilter = "";

				while ((pos = mimetypes.find(';')) != mimetypes.npos) {
					int nb = 16;
					char *extensions[nb];
					string mimetype = mimetypes.substr(0, pos);
					mimetypes = mimetypes.substr(pos + 1);

					nb = xdg_mime_get_extensions_from_mime_type(
								mimetype.c_str(), extensions, nb);

					while (nb--) {
						selectorfilter += (string) extensions[nb] + ',';
						free(extensions[nb]);
					}
				}

				// Remove last comma
				if (!selectorfilter.empty()) {
					selectorfilter.erase(selectorfilter.end());
					DEBUG("Compatible extensions: %s\n", selectorfilter.c_str());
				}

				continue;
			}
#endif // HAVE_LIBXDGMIME
		}

		file = gmenu2x->getAssetsPath() + "/sections/" + category + '/' + opkMount;
		opkMount = (string) "/mnt/" + opkMount + '/';
		edited = true;
	}

/* ---------------------------------------------------------------- */

	string line;
	TRACE("LinkApp::LinkApp - ctor - creating ifstream");
	ifstream infile (linkfile, ios_base::in);
	TRACE("LinkApp::LinkApp - ctor - iterating thru infile : %s", linkfile);
	while (getline(infile, line, '\n')) {

		line = trim(line);
		if (line == "") continue;
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
		} else if (!isOpk()) {
				
			if (name == "title") {
				title = value;
			} else if (name == "description") {
				description = value;
			} else if (name == "icon") {
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

		} else {
			WARNING("Unrecognized OPK link option: '%s'\n", name.c_str());
		}

	}
	TRACE("LinkApp::LinkApp - ctor - closing infile");
	infile.close();

	if (iconPath.empty()) {
		TRACE("LinkApp::LinkApp - ctor - searching for icon");
		searchIcon();
	}

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
	if (!backdropPath.empty() || !gmenu2x->confInt["skinBackdrops"]) return backdropPath;
	string execicon = exec;
	string::size_type pos = exec.rfind(".");
	if (pos != string::npos) execicon = exec.substr(0, pos);
	string exectitle = base_name(execicon);
	string dirtitle = base_name(dir_name(exec));
	string linktitle = base_name(file);
	pos = linktitle.rfind(".");
	if (pos != string::npos) linktitle = linktitle.substr(0, pos);

// auto backdrop
	if (!gmenu2x->sc.getSkinFilePath("backdrops/" + linktitle + ".png").empty())
		backdropPath = gmenu2x->sc.getSkinFilePath("backdrops/" + linktitle + ".png");
	else if (!gmenu2x->sc.getSkinFilePath("backdrops/" + linktitle + ".jpg").empty())
		backdropPath = gmenu2x->sc.getSkinFilePath("backdrops/" + linktitle + ".jpg");
	else if (!gmenu2x->sc.getSkinFilePath("backdrops/" + exectitle + ".png").empty())
		backdropPath = gmenu2x->sc.getSkinFilePath("backdrops/" + exectitle + ".png");
	else if (!gmenu2x->sc.getSkinFilePath("backdrops/" + exectitle + ".jpg").empty())
		backdropPath = gmenu2x->sc.getSkinFilePath("backdrops/" + exectitle + ".jpg");
	else if (!gmenu2x->sc.getSkinFilePath("backdrops/" + dirtitle + ".png").empty())
		backdropPath = gmenu2x->sc.getSkinFilePath("backdrops/" + dirtitle + ".png");
	else if (!gmenu2x->sc.getSkinFilePath("backdrops/" + dirtitle + ".jpg").empty())
		backdropPath = gmenu2x->sc.getSkinFilePath("backdrops/" + dirtitle + ".jpg");

	return backdropPath;
}

const string &LinkApp::searchIcon() {
	string execicon = exec;
	string::size_type pos = exec.rfind(".");
	if (pos != string::npos) execicon = exec.substr(0, pos);
	execicon += ".png";
	string exectitle = base_name(execicon);
	string dirtitle = base_name(dir_name(exec)) + ".png";
	string linktitle = base_name(file);
	pos = linktitle.rfind(".");
	if (pos != string::npos) linktitle = linktitle.substr(0, pos);
	linktitle += ".png";

	if (!gmenu2x->sc.getSkinFilePath("icons/" + linktitle).empty())
		iconPath = gmenu2x->sc.getSkinFilePath("icons/" + linktitle);
	else if (!gmenu2x->sc.getSkinFilePath("icons/" + exectitle).empty())
		iconPath = gmenu2x->sc.getSkinFilePath("icons/" + exectitle);
	else if (!gmenu2x->sc.getSkinFilePath("icons/" + dirtitle).empty())
		iconPath = gmenu2x->sc.getSkinFilePath("icons/" + dirtitle);
	else if (fileExists(execicon))
		iconPath = execicon;
	else
		iconPath = gmenu2x->sc.getSkinFilePath("icons/generic.png");

	return iconPath;
}

int LinkApp::clock() {
	return iclock;
}

void LinkApp::setCPU(int mhz) {
	iclock = mhz;
	if (iclock != 0) iclock = constrain(iclock, gmenu2x->confInt["cpuMin"], gmenu2x->confInt["cpuMax"]);
	edited = true;
}

#if defined(TARGET_GP2X)
//G
int LinkApp::gamma() {
	return igamma;
}

void LinkApp::setGamma(int gamma) {
	igamma = constrain(gamma, 0, 100);
	edited = true;
}
// /G
#endif

void LinkApp::setBackdrop(const string selectedFile) {
	backdrop = backdropPath = selectedFile;
	edited = true;
}

bool LinkApp::targetExists() {
#if defined(TARGET_PC)
	return true; //For displaying elements during testing on pc
#endif

	string target = exec;
	if (!exec.empty() && exec[0] != '/' && !workdir.empty())
		target = workdir + "/" + exec;

	return fileExists(target);
}

bool LinkApp::save() {
	if (!edited) return false;

	ofstream f(file.c_str());
	if (f.is_open()) {
		if (title != ""        ) f << "title="           << title           << endl;
		if (description != ""  ) f << "description="     << description     << endl;
		if (icon != ""         ) f << "icon="            << icon            << endl;
		if (exec != ""         ) f << "exec="            << exec            << endl;
		if (params != ""       ) f << "params="          << params          << endl;
		if (workdir != ""      ) f << "workdir="         << workdir         << endl;
		if (consoleapp         ) f << "consoleapp=true"                     << endl;
		if (!consoleapp        ) f << "consoleapp=false"                    << endl;
		if (manual != ""       ) f << "manual="          << manual          << endl;
		if (iclock != 0        ) f << "clock="           << iclock          << endl;
		// if (useRamTimings      ) f << "useramtimings=true"                  << endl;
		// if (useGinge           ) f << "useginge=true"                       << endl;
		// if (ivolume > 0        ) f << "volume="          << ivolume         << endl;

#if defined(TARGET_GP2X)
		//G
		if (igamma != 0        ) f << "gamma="           << igamma          << endl;
#endif

		if (selectordir != ""    ) f << "selectordir="     << selectordir     << endl;
		if (selectorbrowser      ) f << "selectorbrowser=true"                << endl;
		if (!selectorbrowser     ) f << "selectorbrowser=false"               << endl;
		if (selectorfilter != "" ) f << "selectorfilter="  << selectorfilter  << endl;
		if (selectorscreens != "") f << "selectorscreens=" << selectorscreens << endl;
		if (aliasfile != ""      ) f << "selectoraliases=" << aliasfile       << endl;
		if (backdrop != ""       ) f << "backdrop="        << backdrop        << endl;
		// if (wrapper              ) f << "wrapper=true"                        << endl;
		// if (dontleave            ) f << "dontleave=true"                      << endl;
		f.close();
		return true;
	} else
		ERROR("Error while opening the file '%s' for write.", file.c_str());
	return false;
}

/*
 * entry point for running
 * checks to see if we want a supporting file arg
 * and launches a chooser
 * or
 * just launches
 */

void LinkApp::run() {
	if (selectordir != "") {
		selector();
	} else {
		launch();
	}
}

/*
 * lauches a supporting file selector if needed
 */
void LinkApp::selector(int startSelection, const string &selectorDir) {
	//Run selector interface
	Selector sel(gmenu2x, this, selectorDir);
	int selection = sel.exec(startSelection);
	if (selection != -1) {
		//gmenu2x->writeTmp(selection, sel.getDir());
		launch(sel.getFile(), sel.getDir());
	}
}

void LinkApp::launch(const string &selectedFile, const string &selectedDir) {
	TRACE("LinkApp::launch - enter : %s - %s", selectedDir.c_str(), selectedFile.c_str());
	
	save();

	//Set correct working directory
	string wd = getRealWorkdir();
	TRACE("LinkApp::launch - real work dir = %s", wd.c_str());
	if (!wd.empty()) {
		chdir(wd.c_str());
		TRACE("LinkApp::launch - changed into wrkdir");
	}

	//selectedFile
	if (selectedFile != "") {
		string selectedFileExtension;
		string selectedFileName;
		string dir;
		string::size_type i = selectedFile.rfind(".");
		if (i != string::npos) {
			selectedFileExtension = selectedFile.substr(i,selectedFile.length());
			selectedFileName = selectedFile.substr(0,i);
		}

		TRACE("LinkApp::launch - name : %s, extension : %s", selectedFileName.c_str(), selectedFileExtension.c_str());
		if (selectedDir == "")
			dir = getSelectorDir();
		else
			dir = selectedDir;
		// force a slash at the end
		if (dir.length() > 0) {
			if (0 != dir.compare(dir.length() -1, 1, "/")) 
				dir += "/";
		} else dir = "/";
		TRACE("LinkApp::launch - dir : %s", dir.c_str());
		
		if (params == "") {
			params = cmdclean(dir + selectedFile);
			TRACE("LinkApp::launch - no params, so cleaned to : %s", params.c_str());
		} else {
			string origParams = params;
			params = strreplace(params, "[selFullPath]", cmdclean(dir + selectedFile));
			params = strreplace(params, "[selPath]", cmdclean(dir));
			params = strreplace(params, "[selFile]", cmdclean(selectedFileName));
			params = strreplace(params, "[selExt]", cmdclean(selectedFileExtension));
			if (params == origParams) params += " " + cmdclean(dir + selectedFile);
		}
	}

	INFO("Executing '%s' (%s %s)", title.c_str(), exec.c_str(), params.c_str());

	//check if we have to quit
	string command = cmdclean(exec);
	TRACE("LinkApp::launch - command : %s", command.c_str());

	// Check to see if permissions are desirable
	struct stat fstat;
	if ( stat( command.c_str(), &fstat ) == 0 ) {
		struct stat newstat = fstat;
		if ( S_IRUSR != ( fstat.st_mode & S_IRUSR ) ) newstat.st_mode |= S_IRUSR;
		if ( S_IXUSR != ( fstat.st_mode & S_IXUSR ) ) newstat.st_mode |= S_IXUSR;
		if ( fstat.st_mode != newstat.st_mode ) chmod( command.c_str(), newstat.st_mode );
	} // else, well.. we are no worse off :)

	if (params != "") command += " " + params;
	TRACE("LinkApp::launch - command + params : %s", command.c_str());
	
	// if (useGinge) {
		// string ginge_prep = gmenu2x->getExePath() + "/ginge/ginge_prep";
		// if (fileExists(ginge_prep)) command = cmdclean(ginge_prep) + " " + command;
	// }
	if (gmenu2x->confInt["outputLogs"]) 
		command += " 2>&1 | tee " + cmdclean(gmenu2x->getAssetsPath()) + "log.txt";
	
	TRACE("LinkApp::launch - after logging checked : %s", command.c_str());


	if (gmenu2x->confInt["saveSelection"] && (gmenu2x->confInt["section"] != gmenu2x->menu->selSectionIndex() || gmenu2x->confInt["link"] != gmenu2x->menu->selLinkIndex())) {
		TRACE("LinkApp::launch - saving selection");
		gmenu2x->writeConfig();
	}

	Launcher *toLaunch = new Launcher(
				vector<string> { "/bin/sh", "-c", command });

	if (toLaunch) {
		//delete menu;
		gmenu2x->quit();
		//gmenu2x->releaseScreen();
		unsetenv("SDL_FBCON_DONT_CLEAR");

		// TODO Blank the screen
		
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
	if (this->selectordir != "") this->selectordir = real_path(this->selectordir);
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
