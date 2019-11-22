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
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>
#include <math.h>
#include <fstream>
#include <opk.h>

#include "gmenu2x.h"
#include "linkapp.h"
#include "menu.h"
#include "filelister.h"
#include "utilities.h"
#include "debug.h"

using namespace std;

const string OPK_FOLDER_NAME = "apps";
const string OPK_INTERNAL_PATH = "/media/data/" + OPK_FOLDER_NAME;
const string OPK_PLATFORM = "gcw0";

Menu::Menu(GMenu2X *gmenu2x) {
	TRACE("Menu :: ctor - enter");
	this->gmenu2x = gmenu2x;
	iFirstDispSection = 0;

	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath;

	vector<string> filter;
	split(filter, this->gmenu2x->config->sectionFilter(), ",");
	TRACE("Menu :: ctor - got %i filter sections", filter.size());

	TRACE("Menu :: ctor - opening sections");
	string resolvedPath = this->gmenu2x->getAssetsPath() + "sections/";
	TRACE("Menu :: ctor - looking for section in : %s", resolvedPath.c_str());
	if ((dirp = opendir(resolvedPath.c_str())) == NULL) return;

	TRACE("Menu :: ctor - readdir : %i", dirp);
	while ((dptr = readdir(dirp))) {
		if (dptr->d_name[0] == '.') continue;
		string dirName = string(dptr->d_name);
		TRACE("Menu :: ctor - reading : %s", dptr->d_name);
		filepath = resolvedPath + dirName;
		TRACE("Menu :: ctor - checking : %s", filepath.c_str());
		int statRet = stat(filepath.c_str(), &st);
		TRACE("Menu :: ctor - reading stat : %i", statRet);
		if (!S_ISDIR(st.st_mode)) continue;
		if (statRet != -1) {
			// check the filters
			bool filtered = false;
			for (vector<string>::iterator it = filter.begin(); it != filter.end(); ++it) {
				string filterName = (*it);
				if (dirName.compare(filterName) == 0) {
					filtered = true;
					break;
				}
			}
			if (!filtered) {
				TRACE("Menu :: ctor - adding section : %s", dptr->d_name);
				sections.push_back((string)dptr->d_name);
				linklist ll;
				links.push_back(ll);
			}
		}
	}
	TRACE("Menu :: ctor - dirp has been read");
	
	TRACE("Menu :: ctor - add sections");
	addSection("settings");
	TRACE("Menu :: ctor - add applications");
	addSection("applications");

	TRACE("Menu :: ctor - close dirp");
	closedir(dirp);
	TRACE("Menu :: ctor - sort");
	sort(sections.begin(),sections.end(),case_less());
	TRACE("Menu :: ctor - set index 0");
	setSectionIndex(0);
	TRACE("Menu :: ctor - read links");
	readLinks();

	TRACE("Menu :: ctor - read internal OPK links");
	openPackagesFromDir(OPK_INTERNAL_PATH);

	string appPath = gmenu2x->config->externalAppPath();

	TRACE("Menu :: ctor - searching for external OPK links under : %s", appPath.c_str());
	if (dirExists(appPath)) {
		TRACE("Menu :: ctor - external search root dir : %s", appPath.c_str());
		openPackagesFromDir(appPath);
	}

	TRACE("Menu :: ctor - ordering links");
	orderLinks();

	TRACE("Menu :: ctor - exit");
}

Menu::~Menu() {
	freeLinks();
}

uint32_t Menu::firstDispRow() {
	return iFirstDispRow;
}

void Menu::loadIcons() {
	TRACE("Menu::loadIcons - enter");

	for (uint32_t i = 0; i < sections.size(); i++) {
		string sectionIcon = "sections/" + sections[i] + ".png";
		TRACE("Menu::loadIcons - section : %s", sections[i].c_str());
		if (!gmenu2x->skin->getSkinFilePath(sectionIcon).empty()) {
			TRACE("Menu::loadIcons - section  icon: skin:%s", sectionIcon.c_str());
			gmenu2x->sc->addIcon("skin:" + sectionIcon);
		}

		//check link's icons
		string linkIcon;
		for (uint32_t x = 0; x < sectionLinks(i)->size(); x++) {
			linkIcon = sectionLinks(i)->at(x)->getIcon();
			TRACE("Menu::loadIcons - link : %s", sectionLinks(i)->at(x)->getTitle().c_str());
			TRACE("Menu::loadIcons - link icon : %s", linkIcon.c_str());

			TRACE("Menu::loadIcons - link : casting the link app");
			LinkApp *linkapp = dynamic_cast<LinkApp*>(sectionLinks(i)->at(x));

			if (linkapp != NULL) {
				TRACE("Menu::loadIcons - link - searching backdrop and manuals");
				linkapp->searchBackdrop();
				linkapp->searchManual();
			}

			TRACE("Menu::loadIcons - link : testing for skin icon vs real icon");
			if (linkIcon.substr(0,5) == "skin:") {
				linkIcon = gmenu2x->skin->getSkinFilePath(linkIcon.substr(5,linkIcon.length()));
				if (linkapp != NULL && !fileExists(linkIcon))
					linkapp->searchIcon();
				else
					sectionLinks(i)->at(x)->setIconPath(linkIcon);

			} else if (!fileExists(linkIcon)) {
				if (linkapp != NULL) 
					linkapp->searchIcon();
			}
		}
	}
	TRACE("Menu::loadIcons - exit");
}

/*====================================
   SECTION MANAGEMENT
  ====================================*/
void Menu::freeLinks() {
	for (vector<linklist>::iterator section = links.begin(); section < links.end(); section++)
		for (linklist::iterator link = section->begin(); link < section->end(); link++)
			delete *link;
}

linklist *Menu::sectionLinks(int i) {
	if (i < 0 || i > (int)links.size())
		i = selSectionIndex();

	if (i < 0 || i > (int)links.size())
		return NULL;

	return &links[i];
}

void Menu::decSectionIndex() {
	setSectionIndex(iSection - 1);
}

void Menu::incSectionIndex() {
	setSectionIndex(iSection + 1);
}

uint32_t Menu::firstDispSection() {
	return iFirstDispSection;
}

int Menu::selSectionIndex() {
	return iSection;
}

const string &Menu::selSection() {
	return sections[iSection];
}

int Menu::sectionNumItems() {
	return gmenu2x->skin->sectionBar == Skin::SB_TOP || gmenu2x->skin->sectionBar == Skin::SB_BOTTOM 
		? (gmenu2x->config->resolutionX() - 40)/gmenu2x->skin->sectionTitleBarSize 
		: (gmenu2x->config->resolutionY() - 40)/gmenu2x->skin->sectionTitleBarSize;
}

void Menu::setSectionIndex(int i) {
	if (i < 0)
		i = sections.size() - 1;
	else if (i >= (int)sections.size())
		i = 0;
	iSection = i;

	int numRows = sectionNumItems();
	numRows -= 1;
	if ( i >= (int)iFirstDispSection + numRows)
		iFirstDispSection = i - numRows;
	else if ( i < (int)iFirstDispSection)
		iFirstDispSection = i;
	iLink = 0;
	iFirstDispRow = 0;
}

string Menu::sectionPath(int section) {
	if (section < 0 || section > (int)sections.size()) section = iSection;
	return "sections/" + sections[section] + "/";
}

/*====================================
   LINKS MANAGEMENT
  ====================================*/
bool Menu::addActionLink(uint32_t section, const string &title, fastdelegate::FastDelegate0<> action, const string &description, const string &icon) {
	if (section >= sections.size()) return false;

	Link *linkact = new Link(gmenu2x, action);
	linkact->setTitle(title);
	linkact->setDescription(description);
	if (gmenu2x->sc->exists(icon) || (icon.substr(0,5) == "skin:" && !gmenu2x->skin->getSkinFilePath(icon.substr(5, icon.length())).empty()) || fileExists(icon))
	linkact->setIcon(icon);

	sectionLinks(section)->push_back(linkact);
	return true;
}

bool Menu::addLink(string path, string file, string section) {
	TRACE("Menu::addLink - enter");
	if (section.empty())
		section = selSection();
	else if (find(sections.begin(), sections.end(), section) == sections.end()) {
		//section directory doesn't exists
		if (!addSection(section))
			return false;
	}

	//if the extension is not equal to gpu or gpe then enable the wrapper by default
	// bool wrapper = true;

	//strip the extension from the filename
	string title = fileBaseName(file);
	string ext = fileExtension(file);

	string linkpath = gmenu2x->getAssetsPath() + "sections/" + section + "/" + title;
	int x = 2;
	while (fileExists(linkpath)) {
		stringstream ss;
		linkpath = "";
		ss << x;
		ss >> linkpath;
		linkpath = gmenu2x->getAssetsPath()  + "sections/" + section + "/" + title + linkpath;
		x++;
	}

	INFO("Adding link: '%s'", linkpath.c_str());

	if (path[path.length() - 1] != '/') path += "/";

	string shorttitle = title, exec = path + file;

	//Reduce title length to fit the link width
	if ((int)gmenu2x->font->getTextWidth(shorttitle) > gmenu2x->linkWidth) {
		while ((int)gmenu2x->font->getTextWidth(shorttitle + "..") > gmenu2x->linkWidth) {
			shorttitle = shorttitle.substr(0, shorttitle.length() - 1);
		}
		shorttitle += "..";
	}
	
	int isection;
	ofstream f(linkpath.c_str());
	if (f.is_open()) {
		f << "title=" << shorttitle << endl;
		f << "exec=" << exec << endl;
		f.close();

		isection = find(sections.begin(), sections.end(), section) - sections.begin();

		if (isection >= 0 && isection < (int)sections.size()) {
			INFO("Section: '%s(%i)'", sections[isection].c_str(), isection);

			LinkApp *link = new LinkApp(gmenu2x, linkpath.c_str(), true);
			if (link->targetExists())
				links[isection].push_back( link );
			else
				delete link;
		}
	} else {
		ERROR("Error while opening the file '%s' for write.", linkpath.c_str());
		return false;
	}

	setLinkIndex(links[isection].size() - 1);
	TRACE("Menu::addLink - exit");
	return true;
}

bool Menu::addSection(const string &sectionName) {
	TRACE("Menu::addSection - enter %s", sectionName.c_str());
	string sectiondir = gmenu2x->getAssetsPath() + "sections/" + sectionName;

	if (mkdir(sectiondir.c_str(),0777) == 0) {
		sections.push_back(sectionName);
		linklist ll;
		links.push_back(ll);
		TRACE("Menu::addSection - exit created dir : %s", sectiondir.c_str());
		return true;
	} else if (errno == EEXIST ) {
		TRACE("Menu::addSection - skipping dir already exists : %s", sectiondir.c_str());
	} else TRACE("Menu::addSection - failed to mkdir");
	return false;
}

void Menu::deleteSelectedLink() {
	string iconpath = selLink()->getIconPath();

	INFO("Deleting link '%s'", selLink()->getTitle().c_str());

	if (selLinkApp() != NULL) unlink(selLinkApp()->getFile().c_str());
	sectionLinks()->erase( sectionLinks()->begin() + selLinkIndex() );
	setLinkIndex(selLinkIndex());

	bool icon_used = false;
	for (uint32_t i = 0; i < sections.size(); i++) {
		for (uint32_t j = 0; j < sectionLinks(i)->size(); j++) {
			if (iconpath == sectionLinks(i)->at(j)->getIconPath()) {
				icon_used = true;
			}
		}
	}
	if (!icon_used) {
		gmenu2x->sc->del(iconpath);
	}
}

void Menu::deleteSelectedSection() {
	INFO("Deleting section '%s'", selSection().c_str());

	gmenu2x->sc->del(gmenu2x->getAssetsPath() + "sections/" + selSection() + ".png");
	links.erase( links.begin() + selSectionIndex() );
	sections.erase( sections.begin() + selSectionIndex() );
	setSectionIndex(0); //reload sections
}

bool Menu::linkChangeSection(uint32_t linkIndex, uint32_t oldSectionIndex, uint32_t newSectionIndex) {
	if (oldSectionIndex < sections.size() && newSectionIndex < sections.size() && linkIndex < sectionLinks(oldSectionIndex)->size()) {
		sectionLinks(newSectionIndex)->push_back( sectionLinks(oldSectionIndex)->at(linkIndex) );
		sectionLinks(oldSectionIndex)->erase( sectionLinks(oldSectionIndex)->begin() + linkIndex );
		//Select the same link in the new position
		setSectionIndex(newSectionIndex);
		setLinkIndex(sectionLinks(newSectionIndex)->size() - 1);
		return true;
	}
	return false;
}

void Menu::pageUp() {
	// PAGEUP with left
	if ((int)(iLink - gmenu2x->skin->numLinkRows - 1) < 0) {
		setLinkIndex(0);
	} else {
		setLinkIndex(iLink - gmenu2x->skin->numLinkRows + 1);
	}
}

void Menu::pageDown() {
	// PAGEDOWN with right
	if (iLink + gmenu2x->skin->numLinkRows > sectionLinks()->size()) {
		setLinkIndex(sectionLinks()->size() - 1);
	} else {
		setLinkIndex(iLink + gmenu2x->skin->numLinkRows - 1);
	}
}

void Menu::linkLeft() {
	// if (iLink % gmenu2x->skin->numLinkCols == 0)
		// setLinkIndex(sectionLinks()->size() > iLink + gmenu2x->skin->numLinkCols - 1 ? iLink + gmenu2x->skin->numLinkCols - 1 : sectionLinks()->size() - 1 );
	// else
		setLinkIndex(iLink - 1);
}

void Menu::linkRight() {
	// if (iLink % gmenu2x->skin->numLinkCols == (gmenu2x->skin->numLinkCols - 1) || iLink == (int)sectionLinks()->size() - 1)
		// setLinkIndex(iLink - iLink % gmenu2x->skin->numLinkCols);
	// else
		setLinkIndex(iLink + 1);
}

void Menu::linkUp() {
	int l = iLink - gmenu2x->skin->numLinkCols;
	if (l < 0) {
		uint32_t rows = (uint32_t)ceil(sectionLinks()->size() / (double)gmenu2x->skin->numLinkCols);
		l += (rows * gmenu2x->skin->numLinkCols);
		if (l >= (int)sectionLinks()->size())
			l -= gmenu2x->skin->numLinkCols;
	}
	setLinkIndex(l);
}

void Menu::linkDown() {
	uint32_t l = iLink + gmenu2x->skin->numLinkCols;
	if (l >= sectionLinks()->size()) {
		uint32_t rows = (uint32_t)ceil(sectionLinks()->size() / (double)gmenu2x->skin->numLinkCols);
		uint32_t curCol = (uint32_t)ceil((iLink+1) / (double)gmenu2x->skin->numLinkCols);
		if (rows > curCol)
			l = sectionLinks()->size() - 1;
		else
			l %= gmenu2x->skin->numLinkCols;
	}
	setLinkIndex(l);
}

int Menu::selLinkIndex() {
	return iLink;
}

Link *Menu::selLink() {
	if (sectionLinks()->size() == 0) return NULL;
	return sectionLinks()->at(iLink);
}

LinkApp *Menu::selLinkApp() {
	return dynamic_cast<LinkApp*>(selLink());
}

void Menu::setLinkIndex(int i) {
	if (i < 0)
		i = sectionLinks()->size() - 1;
	else if (i >= (int)sectionLinks()->size())
		i = 0;

	if (i >= (int)(iFirstDispRow * gmenu2x->skin->numLinkCols + gmenu2x->skin->numLinkCols * gmenu2x->skin->numLinkRows))
		iFirstDispRow = i / gmenu2x->skin->numLinkCols - gmenu2x->skin->numLinkRows + 1;
	else if (i < (int)(iFirstDispRow * gmenu2x->skin->numLinkCols))
		iFirstDispRow = i / gmenu2x->skin->numLinkCols;

	iLink = i;
}

void Menu::readLinks() {
	TRACE("Menu::readLinks - enter");
	vector<string> linkfiles;

	iLink = 0;
	iFirstDispRow = 0;

	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath;
	string assets_path = gmenu2x->getAssetsPath();
	
	for (uint32_t i = 0; i < links.size(); i++) {
		links[i].clear();
		linkfiles.clear();
		string full_path = assets_path + sectionPath(i);
		
		TRACE("Menu::readLinks - scanning path : %s", full_path.c_str());
		if ((dirp = opendir(full_path.c_str())) == NULL) continue;
		TRACE("Menu::readLinks - opened path : %s", full_path.c_str());
		while ((dptr = readdir(dirp))) {
			if (dptr->d_name[0] == '.') continue;
			filepath = full_path + dptr->d_name;
			TRACE("Menu::readLinks - filepath : %s", filepath.c_str());
			int statRet = stat(filepath.c_str(), &st);
			if (S_ISDIR(st.st_mode)) continue;
			if (statRet != -1) {
				TRACE("Menu::readLinks - filepath valid : %s", filepath.c_str());
				linkfiles.push_back(filepath);
			}
		}

		TRACE("Menu::readLinks - sorting links");
		sort(linkfiles.begin(), linkfiles.end(), case_less());
		TRACE("Menu::readLinks - links sorted");

		TRACE("Menu::readLinks - validating %i links exist", linkfiles.size());
		for (uint32_t x = 0; x < linkfiles.size(); x++) {
			TRACE("Menu::readLinks - validating link : %s", linkfiles[x].c_str());

			LinkApp *link = new LinkApp(gmenu2x, linkfiles[x].c_str(), true);
			TRACE("Menu::readLinks - link created...");
			if (link->targetExists()) {
				TRACE("Menu::readLinks - target exists");
				links[i].push_back( link );
			} else {
				TRACE("Menu::readLinks - target doesn't exist");
				delete link;
			}
		}

		closedir(dirp);
	}

	TRACE("Menu::readLinks - orderLinks");
	orderLinks();

	TRACE("Menu::readLinks - exit");
}

void Menu::renameSection(int index, const string &name) {
	sections[index] = name;
}

bool Menu::sectionExists(const string &name) {
	return std::find(sections.begin(), sections.end(), name)!=sections.end();
}

int Menu::getSectionIndex(const string &name) {
	return distance(sections.begin(), find(sections.begin(), sections.end(), name));
}

const string Menu::getSectionIcon(int i) {
	string sectionIcon = "skin:sections/" + sections[i] + ".png";
	if (!gmenu2x->sc->exists(sectionIcon)) {
		sectionIcon = "skin:icons/section.png";
	}
	return sectionIcon;
}

static bool compare_links(Link *a, Link *b) {
	LinkApp *app1 = dynamic_cast<LinkApp *>(a);
	LinkApp *app2 = dynamic_cast<LinkApp *>(b);
	bool app1_is_opk = app1 && app1->isOpk(),
		 app2_is_opk = app2 && app2->isOpk();

	if (app1_is_opk && !app2_is_opk)
			return false;
	if (app2_is_opk && !app1_is_opk)
			return true;
	return a->getTitle().compare(b->getTitle()) <= 0;
}

void Menu::orderLinks() {
	TRACE("Menu::orderLinks - enter");
	for (auto& section : links) {
		sort(section.begin(), section.end(), compare_links);
	}
	TRACE("Menu::orderLinks - exit");
}

/* --------------- OPK SECTION ---------------*/

void Menu::openPackagesFromDir(string path) {
	TRACE("Menu::openPackagesFromDir - enter : %s", path.c_str());
	if (dirExists(path))
		readPackages(path);
	TRACE("Menu::openPackagesFromDir - exit");
}

void Menu::openPackage(string path, bool order) {
	TRACE("Menu::openPackage - enter : search : %s, sort : %i", path.c_str(), order);

	/* First try to remove existing links of the same OPK
	 * (needed for instance when an OPK is modified) */
	removePackageLink(path);

	struct OPK *opk = opk_open(path.c_str());
	if (!opk) {
		ERROR("Unable to open OPK %s\n", path.c_str());
		return;
	}

	TRACE("Menu::openPackage : meta outer loop");
	for (;;) {
		unsigned int i;
		bool has_metadata = false;
		const char *name;
		LinkApp *link;

		TRACE("Menu::openPackage : meta inner loop");
		for (;;) {
			string::size_type pos;
			int ret = opk_open_metadata(opk, &name);
			if (ret < 0) {
				ERROR("Error while loading meta-data\n");
				break;
			} else if (!ret)
			  break;

			/* Strip .desktop */
			string metadata(name);
			pos = metadata.rfind('.');
			metadata = metadata.substr(0, pos);

			/* Keep only the platform name */
			pos = metadata.rfind('.');
			metadata = metadata.substr(pos + 1);

			TRACE("Menu::openPackage : resolved meta data to : %s", metadata.c_str());
			if (metadata == OPK_PLATFORM || metadata == "all") {
				TRACE("Menu::openPackage : metadata matches platform : %s", OPK_PLATFORM.c_str());
				has_metadata = true;
				break;
			}
		}

		if (!has_metadata)
		  break;

		// Note: OPK links can only be deleted by removing the OPK itself,
		//       but that is not something we want to do in the menu,
		//       so consider this link undeletable.

		TRACE("Menu::openPackage : creating new linkapp");
		link = new LinkApp(gmenu2x, path.c_str(), false, opk, name);
		TRACE("Menu::openPackage : setting sizes");
		link->setSize(gmenu2x->linkWidth, gmenu2x->linkHeight);
		//link->setSize(gmenu2x->skinConfInt["linkWidth"], gmenu2x->skinConfInt["linkHeight"]);

		TRACE("Menu::openPackage : adding category for %S", link->getCategory().c_str());
		addSection(link->getCategory());
		for (i = 0; i < sections.size(); i++) {
			if (sections[i] == link->getCategory()) {
				TRACE("Menu::openPackage : matched category, adding link");
				links[i].push_back(link);
				break;
			}
		}
	}

	TRACE("Menu::openPackage : closed opk");
	opk_close(opk);

	if (order) {
		TRACE("Menu::openPackage : ordering links");
		orderLinks();
	}

	TRACE("Menu::openPackage : exit");
}

void Menu::readPackages(std::string parentDir) {
	TRACE("Menu::readPackage - enter : %s", parentDir.c_str());
	DIR *dirp;
	struct dirent *dptr;
	vector<string> linkfiles;

	dirp = opendir(parentDir.c_str());
	if (!dirp)
		return;

	TRACE("Menu::readPackage - dir opened");
	while ((dptr = readdir(dirp))) {
		TRACE("Menu::readPackage - reading directory");

		if (dptr->d_type != DT_REG) {
			TRACE("Menu::readPackage - skipping non regular dir entry : %s - (%u)", dptr->d_name, dptr->d_type);
			continue;
		}

		char *c;
		c = strrchr(dptr->d_name, '.');
		TRACE("Menu::readPackage - found file : %s", c);
		if (!c) /* File without extension */
			continue;

		if (strcasecmp(c + 1, "opk"))
			continue;

		if (dptr->d_name[0] == '.') {
			// Ignore hidden files.
			// Mac OS X places these on SD cards, probably to store metadata.
			continue;
		}

		TRACE("Menu::readPackage - it's an opk, handling it...");
		openPackage(parentDir + '/' + dptr->d_name, false);
	}
	TRACE("Menu::readPackage - closing dir");
	closedir(dirp);
	TRACE("Menu::readPackage - exit");
}

/* Remove all links that correspond to the given path.
 * If "path" is a directory, it will remove all links that
 * correspond to an OPK present in the directory. */
void Menu::removePackageLink(std::string path) {
	for (vector< vector<Link*> >::iterator section = links.begin();
				section < links.end(); section++) {
		for (vector<Link*>::iterator link = section->begin();
					link < section->end(); link++) {
			LinkApp *app = dynamic_cast<LinkApp *> (*link);
			if (!app || !app->isOpk() || app->getOpkFile().empty())
				continue;

			if (app->getOpkFile().compare(0, path.size(), path) == 0) {
				TRACE("Removing link corresponding to package %s\n",
							app->getOpkFile().c_str());
				section->erase(link);
				if (section - links.begin() == iSection
							&& iLink == (int) section->size())
					setLinkIndex(iLink - 1);
				link--;
			}
		}
	}

}