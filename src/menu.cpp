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

#include "esoteric.h"
#include "linkapp.h"
#include "menu.h"
#include "filelister.h"
#include "stringutils.h"
#include "debug.h"

Menu::Menu(Esoteric *app) {
	TRACE("enter");
	this->app = app;
	this->iFirstDispSection = 0;

	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	std::string filepath;

	std::vector<std::string> filter;

	try {
		StringUtils::split(filter, this->app->config->sectionFilter(), ",");
		TRACE("got %zu filter sections", filter.size());

		TRACE("opening sections");
		std::string resolvedPath = this->app->getWriteablePath() + "sections/";
		TRACE("looking for section in : %s", resolvedPath.c_str());
		if ((dirp = opendir(resolvedPath.c_str())) == NULL) return;

		//TRACE("readdir : %zu", (long)dirp);
		while ((dptr = readdir(dirp))) {
			if (dptr->d_name[0] == '.') continue;
			std::string dirName = std::string(dptr->d_name);
			//TRACE("reading : %s", dptr->d_name);
			filepath = resolvedPath + dirName;
			TRACE("checking : %s", filepath.c_str());
			int statRet = stat(filepath.c_str(), &st);
			//TRACE("reading stat : %i", statRet);
			if (!S_ISDIR(st.st_mode)) continue;
			if (statRet != -1) {
				// check the filters
				bool filtered = false;
				for (std::vector<std::string>::iterator it = filter.begin(); it != filter.end(); ++it) {
					std::string filterName = (*it);
					if (dirName.compare(filterName) == 0) {
						filtered = true;
						break;
					}
				}
				if (!filtered) {
					TRACE("adding section : %s", dptr->d_name);
					this->sections.push_back((std::string)dptr->d_name);
					linklist ll;
					this->links.push_back(ll);
				}
			}
		}
		TRACE("dirp has been read, closing");
		closedir(dirp);
	} catch(std::exception const& e) {
         ERROR("%s", e.what());
    } catch(...) {
		ERROR("Unknown error");
    }

	TRACE("add compulsary sections: settings, applications");
	this->addSection("settings");
	this->addSection("applications");

	TRACE("sort sections");
	std::sort(this->sections.begin(), this->sections.end(), caseLess());
	TRACE("set index 0");
	this->setSectionIndex(0);
	TRACE("read links");
	this->readLinks();
	TRACE("ordering links");
	this->orderLinks();

	TRACE("exit");
}

Menu::~Menu() {
	this->freeLinks();
}

uint32_t Menu::firstDispRow() {
	return this->iFirstDispRow;
}

void Menu::loadIcons() {
	TRACE("enter");

	try {
		for (uint32_t i = 0; i < sections.size(); i++) {
			std::string sectionIcon = "sections/" + sections[i] + ".png";
			TRACE("section : %s", sections[i].c_str());
			if (!app->skin->getSkinFilePath(sectionIcon).empty()) {
				TRACE("section icon: skin: %s", sectionIcon.c_str());
				app->sc->addIcon("skin:" + sectionIcon);
			}

			//check link's icons
			std::string linkIcon;
			for (uint32_t x = 0; x < sectionLinks(i)->size(); x++) {
				linkIcon = sectionLinks(i)->at(x)->getIcon();
				TRACE("link : %s", sectionLinks(i)->at(x)->getTitle().c_str());
				TRACE("link icon : %s", linkIcon.c_str());

				TRACE("link : casting the link app");
				LinkApp *linkapp = dynamic_cast<LinkApp*>(sectionLinks(i)->at(x));

				if (linkapp != NULL) {
					TRACE("link : searching backdrop and manuals");
					linkapp->searchBackdrop();
					linkapp->searchManual();
				}

				TRACE("link : testing for skin icon vs real icon");
				if (linkIcon.substr(0,5) == "skin:") {
					linkIcon = app->skin->getSkinFilePath(linkIcon.substr(5, linkIcon.length()));
					if (linkapp != NULL && !FileUtils::fileExists(linkIcon))
						linkapp->searchIcon();
					else
						sectionLinks(i)->at(x)->setIconPath(linkIcon);

				} else if (!FileUtils::fileExists(linkIcon)) {
					if (linkapp != NULL) 
						linkapp->searchIcon();
				}
			}
		}
	} catch(std::exception const& e) {
         ERROR("%s", e.what());
    } catch(...) {
		ERROR("Unknown error");
    }
	TRACE("exit");
}

/*====================================
   SECTION MANAGEMENT
  ====================================*/
void Menu::freeLinks() {
	for (std::vector<linklist>::iterator section = links.begin(); section < links.end(); section++)
		for (linklist::iterator link = section->begin(); link < section->end(); link++)
			delete *link;
}

linklist *Menu::sectionLinks(int i) {
	if (i < 0 || i > (int)this->links.size())
		i = this->selSectionIndex();

	if (i < 0 || i > (int)this->links.size())
		return NULL;

	return &this->links[i];
}

void Menu::decSectionIndex() {
	this->setSectionIndex(iSection - 1);
}

void Menu::incSectionIndex() {
	this->setSectionIndex(iSection + 1);
}

uint32_t Menu::firstDispSection() {
	return this->iFirstDispSection;
}

int Menu::selSectionIndex() {
	return this->iSection;
}

const std::string &Menu::selSection() {
	return this->sections[iSection];
}

int Menu::sectionNumItems() {
	return app->skin->sectionBar == Skin::SB_TOP || app->skin->sectionBar == Skin::SB_BOTTOM 
		? (app->getScreenWidth() - 40)/app->skin->sectionTitleBarSize 
		: (app->getScreenHeight() - 40)/app->skin->sectionTitleBarSize;
}

void Menu::setSectionIndex(int i) {
	try {
		if (i < 0)
			i = sections.size() - 1;
		else if (i >= (int)this->sections.size())
			i = 0;
		this->iSection = i;

		int numRows = this->sectionNumItems() - 1;
		if ( i >= (int)this->iFirstDispSection + numRows)
			this->iFirstDispSection = i - numRows;
		else if ( i < (int)this->iFirstDispSection)
			this->iFirstDispSection = i;
		this->iLink = 0;
		this->iFirstDispRow = 0;
	} catch(std::exception const& e) {
         ERROR("%s", e.what());
    }
    catch(...) {
		ERROR("Unknown error");
    }
}

std::string Menu::sectionPath(int section) {
	if (section < 0 || section > (int)sections.size()) section = iSection;
	return "sections/" + sections[section] + "/";
}

/*====================================
   LINKS MANAGEMENT
  ====================================*/
bool Menu::addActionLink(uint32_t section, const std::string &title, fastdelegate::FastDelegate0<> action, const std::string &description, const std::string &icon) {
	TRACE("enter");
	try {
		if (section >= this->sections.size()) return false;
		Link *linkact = new Link(this->app, action);
		linkact->setTitle(title);
		linkact->setDescription(description);
		if (	this->app->sc->exists(icon) || 
				(icon.substr(0,5) == "skin:" && !this->app->skin->getSkinFilePath(icon.substr(5, icon.length())).empty()) || 
				FileUtils::fileExists(icon)) {
					linkact->setIcon(icon);
		}
		this->sectionLinks(section)->push_back(linkact);
	} catch(std::exception const& e) {
         ERROR("%s", e.what());
         return false;
    }
    catch(...) {
		ERROR("Unknown error");
        return false;
    }
	TRACE("exit");
	return true;
}

bool Menu::addLink(std::string path, std::string file, std::string section) {
	TRACE("enter");

	try {
		if (section.empty()) {
			section = this->selSection();
		} else if (std::find(this->sections.begin(), this->sections.end(), section) == this->sections.end()) {
			//section directory doesn't exists
			if (!this->addSection(section))
				return false;
		}

		// strip the extension from the filename
		std::string title = FileUtils::fileBaseName(file);
		std::string linkpath = this->app->getWriteablePath() + "sections/" + section + "/" + title;
		int x = 2;
		while (FileUtils::fileExists(linkpath)) {
			std::stringstream ss;
			linkpath = "";
			ss << x;
			ss >> linkpath;
			linkpath = this->app->getWriteablePath()  + "sections/" + section + "/" + title + linkpath;
			x++;
		}

		INFO("Adding link: '%s'", linkpath.c_str());
		path = FileUtils::normalisePath(path);
		std::string shorttitle = title;
		std::string exec = path + file;
		//Reduce title length to fit the link width
		if ((int)this->app->font->getTextWidth(shorttitle) > app->linkWidth) {
			while ((int)this->app->font->getTextWidth(shorttitle + "..") > app->linkWidth) {
				shorttitle = shorttitle.substr(0, shorttitle.length() - 1);
			}
			shorttitle += "..";
		}
		
		int isection;
		std::ofstream f(linkpath.c_str());
		if (f.is_open()) {
			f << "title=" << shorttitle << std::endl;
			f << "exec=" << exec << std::endl;
			f.close();

			isection = std::find(this->sections.begin(), this->sections.end(), section) - this->sections.begin();

			if (isection >= 0 && isection < (int)this->sections.size()) {
				INFO("Section: '%s(%i)'", this->sections[isection].c_str(), isection);

				LinkApp *link = new LinkApp(this->app, linkpath.c_str(), true);
				if (link->targetExists()) {
					this->links[isection].push_back(link);
				} else {
					delete link;
				}
			}
		} else {
			ERROR("Error while opening the file '%s' for write.", linkpath.c_str());
			return false;
		}

		this->setLinkIndex(links[isection].size() - 1);
	} catch(std::exception const& e) {
         ERROR("%s", e.what());
         return false;
    }
    catch(...) {
		ERROR("Unknown error");
        return false;
    }
	TRACE("exit");
	return true;
}

bool Menu::addSection(const std::string &sectionName) {
	TRACE("enter %s", sectionName.c_str());
	std::string sectiondir = this->app->getWriteablePath() + "sections/" + sectionName;
	try {
		if (mkdir(sectiondir.c_str(),0777) == 0) {
			this->sections.push_back(sectionName);
			linklist ll;
			this->links.push_back(ll);
			TRACE("exit created dir : %s", sectiondir.c_str());
			return true;
		} else if (errno == EEXIST ) {
			TRACE("skipping dir already exists : %s", sectiondir.c_str());
			errno = 0;
		} else TRACE("failed to mkdir");
	} catch(std::exception const& e) {
         ERROR("%s", e.what());
         return false;
    } catch(...) {
		ERROR("Unknown error");
        return false;
    }
	return false;
}

void Menu::deleteSelectedLink() {
	std::string iconpath = this->selLink()->getIconPath();

	INFO("Deleting link '%s'", this->selLink()->getTitle().c_str());

	try {
		if (this->selLinkApp() != NULL) unlink(this->selLinkApp()->getFile().c_str());
		this->sectionLinks()->erase( sectionLinks()->begin() + selLinkIndex() );
		this->setLinkIndex(this->selLinkIndex());

		bool icon_used = false;
		for (uint32_t i = 0; i < sections.size(); i++) {
			for (uint32_t j = 0; j < this->sectionLinks(i)->size(); j++) {
				if (iconpath == this->sectionLinks(i)->at(j)->getIconPath()) {
					icon_used = true;
				}
			}
		}
		if (!icon_used) {
			this->app->sc->del(iconpath);
		}
	} catch(std::exception const& e) {
         ERROR("%s", e.what());
    } catch(...) {
		ERROR("Unknown error");
    }
}

void Menu::deleteSelectedSection() {
	INFO("Deleting section '%s'", selSection().c_str());
	try {
		app->sc->del(app->getWriteablePath() + "sections/" + selSection() + ".png");
		links.erase( links.begin() + selSectionIndex() );
		sections.erase( sections.begin() + selSectionIndex() );
	} catch(std::exception const& e) {
         ERROR("%s", e.what());
    } catch(...) {
		ERROR("Unknown error");
    }
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
	if ((int)(iLink - app->skin->numLinkRows - 1) < 0) {
		setLinkIndex(0);
	} else {
		setLinkIndex(iLink - app->skin->numLinkRows + 1);
	}
}

void Menu::pageDown() {
	// PAGEDOWN with right
	if (iLink + app->skin->numLinkRows > sectionLinks()->size()) {
		setLinkIndex(sectionLinks()->size() - 1);
	} else {
		setLinkIndex(iLink + app->skin->numLinkRows - 1);
	}
}

void Menu::linkLeft() {
	// if (iLink % app->skin->numLinkCols == 0)
		// setLinkIndex(sectionLinks()->size() > iLink + app->skin->numLinkCols - 1 ? iLink + app->skin->numLinkCols - 1 : sectionLinks()->size() - 1 );
	// else
		this->setLinkIndex(iLink - 1);
}

void Menu::linkRight() {
	// if (iLink % app->skin->numLinkCols == (app->skin->numLinkCols - 1) || iLink == (int)sectionLinks()->size() - 1)
		// setLinkIndex(iLink - iLink % app->skin->numLinkCols);
	// else
		this->setLinkIndex(iLink + 1);
}

void Menu::linkUp() {
	int l = iLink - app->skin->numLinkCols;
	if (l < 0) {
		uint32_t rows = (uint32_t)ceil(sectionLinks()->size() / (double)app->skin->numLinkCols);
		l += (rows * app->skin->numLinkCols);
		if (l >= (int)sectionLinks()->size())
			l -= app->skin->numLinkCols;
	}
	setLinkIndex(l);
}

void Menu::linkDown() {
	uint32_t l = iLink + app->skin->numLinkCols;
	if (l >= sectionLinks()->size()) {
		uint32_t rows = (uint32_t)ceil(sectionLinks()->size() / (double)app->skin->numLinkCols);
		uint32_t curCol = (uint32_t)ceil((iLink+1) / (double)app->skin->numLinkCols);
		if (rows > curCol)
			l = sectionLinks()->size() - 1;
		else
			l %= app->skin->numLinkCols;
	}
	setLinkIndex(l);
}

void Menu::letterPrevious() {
	char currentStartChar = this->selLink()->getTitle()[0];
	bool found = false;
	int l = iLink;
	while (l >= 0) {
		if (this->sectionLinks()->at(l)->getTitle()[0] != currentStartChar) {
			this->setLinkIndex(l);
			found = true;
			break;
		}
		--l;
	};
	if (!found) 
		this->setLinkIndex(this->sectionLinks()->size() - 1);
}

void Menu::letterNext() {
	char currentStartChar = this->selLink()->getTitle()[0];
	bool found = false;
	int l = iLink;
	while (l < this->sectionLinks()->size()) {
		if (this->sectionLinks()->at(l)->getTitle()[0] != currentStartChar) {
			this->setLinkIndex(l);
			found = true;
			break;
		}
		++l;
	};
	if (!found) 
		this->setLinkIndex(0);
}

int Menu::selLinkIndex() {
	return this->iLink;
}

Link *Menu::selLink() {
	if (this->sectionLinks()->size() == 0) return NULL;
	return this->sectionLinks()->at(this->iLink);
}

LinkApp *Menu::selLinkApp() {
	return dynamic_cast<LinkApp*>(this->selLink());
}

void Menu::setLinkIndex(int i) {

	if ((int)sectionLinks()->size() == 0)
		return;

	if (i < 0)
		i = sectionLinks()->size() - 1;
	else if (i >= (int)sectionLinks()->size())
		i = 0;


	if (i >= (int)(iFirstDispRow * app->skin->numLinkCols + app->skin->numLinkCols * app->skin->numLinkRows))
		iFirstDispRow = i / app->skin->numLinkCols - app->skin->numLinkRows + 1;
	else if (i < (int)(iFirstDispRow * app->skin->numLinkCols))
		iFirstDispRow = i / app->skin->numLinkCols;

	iLink = i;
}

void Menu::readLinks() {
	TRACE("enter");

	std::vector<std::string> linkfiles;
	this->iLink = 0;
	this->iFirstDispRow = 0;

	try {
		DIR *dirp;
		struct stat st;
		struct dirent *dptr;
		std::string filepath;
		std::string assets_path = app->getWriteablePath();
		
		for (uint32_t i = 0; i < links.size(); i++) {
			links[i].clear();
			linkfiles.clear();
			std::string full_path = assets_path + sectionPath(i);
			
			TRACE("scanning path : %s", full_path.c_str());
			if ((dirp = opendir(full_path.c_str())) == NULL) continue;
			TRACE("opened path : %s", full_path.c_str());
			while ((dptr = readdir(dirp))) {
				if (dptr->d_name[0] == '.') continue;
				filepath = full_path + dptr->d_name;
				TRACE("filepath : %s", filepath.c_str());
				int statRet = stat(filepath.c_str(), &st);
				if (S_ISDIR(st.st_mode)) continue;
				if (statRet != -1) {
					TRACE("filepath valid : %s", filepath.c_str());
					linkfiles.push_back(filepath);
				}
			}

			TRACE("sorting links");
			std::sort(linkfiles.begin(), linkfiles.end(), caseLess());
			TRACE("links sorted");

			TRACE("validating %zu links exist", linkfiles.size());
			for (uint32_t x = 0; x < linkfiles.size(); x++) {
				TRACE("validating link : %s", linkfiles[x].c_str());

				LinkApp *link = new LinkApp(this->app, linkfiles[x].c_str(), true);
				TRACE("link created...");
				if (link->targetExists()) {
					TRACE("target exists");
					if (!link->getHidden() || !this->app->config->respectHiddenLinks()) {
						this->links[i].push_back( link );
					}
				} else {
					TRACE("target doesn't exist");
					delete link;
				}
			}
			closedir(dirp);
		}
	} catch(std::exception const& e) {
         ERROR("%s", e.what());
    }
    catch(...) {
		ERROR("Unknown error");
    }
	TRACE("exit");
}

void Menu::renameSection(int index, const std::string &name) {
	this->sections[index] = name;
}

bool Menu::sectionExists(const std::string &name) {
	return std::find(sections.begin(), sections.end(), name)!=sections.end();
}

int Menu::getSectionIndex(const std::string &name) {
	return std::distance(this->sections.begin(), std::find(this->sections.begin(), this->sections.end(), name));
}

const std::string Menu::getSectionIcon(int i) {
	std::string sectionIcon = "skin:sections/" + sections[i] + ".png";
	if (!app->sc->exists(sectionIcon)) {
		sectionIcon = "skin:icons/section.png";
	}
	return sectionIcon;
}

static bool compare_links(Link *a, Link *b) {
	LinkApp *app1 = dynamic_cast<LinkApp *>(a);
	LinkApp *app2 = dynamic_cast<LinkApp *>(b);
	return strcasecmp(a->getTitle().c_str(), b->getTitle().c_str()) < 0;
	//return a->getTitle().compare(b->getTitle()) <= 0;
}

void Menu::orderLinks() {
	TRACE("enter");
	for (auto& section : this->links) {
		std::sort(section.begin(), section.end(), compare_links);
	}
	TRACE("exit");
}
