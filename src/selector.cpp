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

//for browsing the filesystem
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>

#include "messagebox.h"
#include "linkapp.h"
#include "selector.h"
#include "filelister.h"
#include "debug.h"

using namespace std;

const string PREVIEWS_DIR = ".previews";
const string FILTER_FILE = ".filter";

Selector::Selector(GMenu2X *gmenu2x, LinkApp *link, const string &selectorDir) :
Dialog(gmenu2x) {
	TRACE("Selector::Selector - enter : %s", selectorDir.c_str());
	this->link = link;
	loadAliases();
	selRow = 0;
	if (selectorDir.empty())
		dir = link->getSelectorDir();
	else
		dir = selectorDir;

}

int Selector::exec(int startSelection) {
	TRACE("Selector::exec - enter - startSelection : %i", startSelection);

	// does the link have a backdrop that takes precendence over the background
	if (gmenu2x->sc[link->getBackdrop()] != NULL) 
		gmenu2x->sc[link->getBackdrop()]->blit(this->bg,0,0);

	bool close = false, result = true, inputAction = false;
	vector<string> screens, titles;

	TRACE("Selector::exec - starting selector");
	FileLister fl(dir, link->getSelectorBrowser());

	// do we have screen shots?
	// if we do, they will live under this path, or this dir/screenshots
	this->screendir = link->getSelectorScreens();
	this->tickStart = SDL_GetTicks();
	this->animation = 0;
	this->firstElement = 0;

	uint32_t i, iY, padding = 6;
	uint32_t rowHeight = gmenu2x->font->getHeight() + 1;
	uint32_t halfRowHeight = (rowHeight / 2);

	this->numRows = ((gmenu2x->listRect.h - 2) / rowHeight) - 1;

	drawTopBar(this->bg, link->getTitle(), link->getDescription(), link->getIconPath());
	drawBottomBar(this->bg);

	this->bg->box(gmenu2x->listRect, gmenu2x->skin->colours.listBackground);

	gmenu2x->drawButton(this->bg, "a", gmenu2x->tr["Select"],
	gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Exit"]));

	prepare(&fl, &screens, &titles);
	int selected = constrain(startSelection, 0, fl.size() - 1);

	// moved surfaces out to prevent reloading on loop
	Surface *iconGoUp = gmenu2x->sc.skinRes("imgs/go-up.png");
	Surface *iconFolder = gmenu2x->sc.skinRes("imgs/folder.png");
	Surface *iconFile = gmenu2x->sc.skinRes("imgs/file.png");

	// TODO :: this should probably be true for png's
	gmenu2x->sc.defaultAlpha = false;

	// kick off the chooser loop
	while (!close) {

		// bang the background in
		int currentFileIndex = selected - fl.dirCount();
		this->bg->blit(gmenu2x->screen, 0, 0);

		if (!fl.size()) {
			MessageBox mb(gmenu2x, gmenu2x->tr["This directory is empty"]);
			mb.setAutoHide(1);
			mb.setBgAlpha(0);
			mb.exec();
		} else {
			//Selection wrap test
			if (selected >= firstElement + numRows) firstElement = selected - numRows;
			if (selected < firstElement) firstElement = selected;

			if  (-1 == gmenu2x->skin->previewWidth  && currentFileIndex >= 0) {
				// we are doing a full background thing
				string screenPath = screens.at(currentFileIndex);
				if (!screenPath.empty()) {
					// line it up with the text
					SDL_Rect bgArea {
						0, 
						gmenu2x->skin->topBarHeight, 
						gmenu2x->config->resolutionX, 
						gmenu2x->config->resolutionY - gmenu2x->skin->bottomBarHeight - gmenu2x->skin->topBarHeight };

					// only stretch it once if possible
					if (!gmenu2x->sc.exists(screenPath)) {
						TRACE("Selector::prepare - 1st load - stretching screen path : %s", screenPath.c_str());
						gmenu2x->sc[screenPath]->softStretch(
							gmenu2x->config->resolutionX, 
							gmenu2x->config->resolutionY - gmenu2x->skin->bottomBarHeight - gmenu2x->skin->topBarHeight, 
							true, 
							true);
					}

					gmenu2x->sc[screenPath]->blit(
						gmenu2x->screen, 
						bgArea, 
						HAlignCenter | VAlignMiddle, 
						255);
				}
			}
			//Draw files & Directories
			iY = gmenu2x->listRect.y + 1;
			for (i = firstElement; i < fl.size() && i <= firstElement + numRows; i++, iY += rowHeight) {
				if (i == selected) {
					// slected item highlight
					gmenu2x->screen->box(
						gmenu2x->listRect.x, 
						iY, 
						gmenu2x->listRect.w, 
						rowHeight, 
						gmenu2x->skin->colours.selectionBackground);
				}
				if (fl.isDirectory(i)) {
					if (fl[i] == "..")
						iconGoUp->blit(
							gmenu2x->screen, 
							gmenu2x->listRect.x + 10, 
							iY + halfRowHeight, 
							HAlignCenter | VAlignMiddle);
					else
						iconFolder->blit(
							gmenu2x->screen, 
							gmenu2x->listRect.x + 10, 
							iY + halfRowHeight, 
							HAlignCenter | VAlignMiddle);
				} else {
					iconFile->blit(
						gmenu2x->screen, 
						gmenu2x->listRect.x + 10, 
						iY + halfRowHeight, 
						HAlignCenter | VAlignMiddle);
				}
				gmenu2x->screen->write(
					gmenu2x->font, 
					titles[i], 
					gmenu2x->listRect.x + 21, 
					iY + halfRowHeight, 
					VAlignMiddle);
			}

			// screenshot logic
			if (gmenu2x->skin->previewWidth > 0 && currentFileIndex >= 0) {
				// we're in the files section and there's some art to deal with
				if (!screens[currentFileIndex].empty()) {
					gmenu2x->screen->box(
						gmenu2x->config->resolutionX - animation, 
						gmenu2x->listRect.y, 
						gmenu2x->skin->previewWidth, 
						gmenu2x->listRect.h, 
						gmenu2x->skin->colours.topBarBackground);

					gmenu2x->sc[screens[selected - fl.dirCount()]]->blit(
						gmenu2x->screen, 
						{	gmenu2x->config->resolutionX - animation + padding, 
							gmenu2x->listRect.y + padding, 
							gmenu2x->skin->previewWidth - 2 * padding, 
							gmenu2x->listRect.h - 2 * padding
						}, 
						HAlignCenter | VAlignMiddle, 
						220);

					if (animation < gmenu2x->skin->previewWidth) {
						animation = intTransition(0, gmenu2x->skin->previewWidth, tickStart, 110);
						gmenu2x->screen->flip();
						gmenu2x->input.setWakeUpInterval(45);
						continue;
					}

				} else {
					if (animation > 0) {
						// we only come in here if we had a screenshot before
						// and we need to clean it up
						gmenu2x->screen->box(gmenu2x->config->resolutionX - animation, gmenu2x->listRect.y, gmenu2x->skin->previewWidth, gmenu2x->listRect.h, gmenu2x->skin->colours.topBarBackground);
						animation = gmenu2x->skin->previewWidth - intTransition(0, gmenu2x->skin->previewWidth, tickStart, 80);
						gmenu2x->screen->flip();
						gmenu2x->input.setWakeUpInterval(45);
						continue;
					}
				}
			}
			gmenu2x->input.setWakeUpInterval(1000);
			gmenu2x->screen->clearClipRect();
			gmenu2x->drawScrollBar(numRows, fl.size(), firstElement, gmenu2x->listRect);
			gmenu2x->screen->flip();
		}

		// handle input
		do {
			inputAction = gmenu2x->input.update();
			if (inputAction) this->tickStart = SDL_GetTicks();

			if ( gmenu2x->input[UP] ) {
				selected -= 1;
				if (selected < 0) selected = fl.size() - 1;
			} else if ( gmenu2x->input[DOWN] ) {
				selected += 1;
				if (selected >= fl.size()) selected = 0;
			} else if ( gmenu2x->input[LEFT] ) {
				selected -= numRows;
				if (selected < 0) selected = 0;
			} else if ( gmenu2x->input[RIGHT] ) {
				selected += numRows;
				if (selected >= fl.size()) selected = fl.size() - 1;
			} else if (gmenu2x->input[SECTION_PREV]) {
				selected = 0;
			} else if (gmenu2x->input[SECTION_NEXT]) {
				selected = fl.size() -1;
			} else if (gmenu2x->input[PAGEUP]) {
				// loop thru the titles collection until first char doesn't match
				//std::vector<string>::iterator start = titles.begin();
				char currentStartChar = titles.at(selected)[0];
				int offset = 0;
				bool found = false;
				for(std::vector<string>::iterator current = titles.begin() + selected; current != titles.begin(); current--) {
					--offset;
					if (currentStartChar != (*current)[0]) {
						selected += offset + 1;
						found = true;
						break;
					}
				}
				if (!found) selected = fl.size() -1;
			} else if (gmenu2x->input[PAGEDOWN]) {
				// reverse loop thru the titles collection until first char doesn't match
				//std::vector<string>::iterator end = titles.end();
				char currentStartChar = titles.at(selected)[0];
				int offset = 0;
				bool found = false;
				for(std::vector<string>::iterator current = titles.begin() + selected; current != titles.end(); current++) {
					++offset;
					if (currentStartChar != (*current)[0]) {
						selected += offset - 1;
						found = true;
						break;
					}
				}
				if (!found) selected = 0;
			} else if ( gmenu2x->input[SETTINGS] ) {
				close = true;
				result = false;
			} else if ( gmenu2x->input[CANCEL] && link->getSelectorBrowser()) {
				string::size_type p = this->dir.rfind("/", this->dir.size() - 2);
				this->dir = this->dir.substr(0, p + 1);
				selected = 0;
				this->firstElement = 0;
				prepare(&fl, &screens, &titles);
			} else if ( gmenu2x->input[CONFIRM] ) {
				// file selected or dir selected
				if (fl.isFile(selected)) {
					file = fl[selected];
					close = true;
				} else {
					this->dir = real_path(dir + "/" + fl[selected]);
					selected = 0;
					this->firstElement = 0;
					prepare(&fl, &screens, &titles);
				}
			}
		} while (!inputAction);
	}

	// TODO :: Probably loose this if we loose the previous setter
	gmenu2x->sc.defaultAlpha = true;
	freeScreenshots(&screens);

	TRACE("Selector::exec - exit : %i", result ? (int)selected : -1);
	return result ? (int)selected : -1;
}

// checks for screen shots etc
void Selector::prepare(FileLister *fl, vector<string> *screens, vector<string> *titles) {
	TRACE("Selector::prepare - enter");

	if (this->dir.length() > 0) {
		if (0 != this->dir.compare(dir.length() -1, 1, "/")) 
			this->dir += "/";
	} else this->dir = "/";
	fl->setPath(this->dir);

	TRACE("Selector::exec - setting filter");
	string filter = link->getSelectorFilter();
	string filterFile = this->dir + FILTER_FILE;
	TRACE("Selector::exec - looking for a filter file at : %s", filterFile.c_str());
	if (fileExists(filterFile)) {
		TRACE("Selector::exec - found a filter file at : %s", filterFile.c_str());
		if (filter.length() > 0) filter += ",";
		filter += fileReader(filterFile);
	}
	fl->setFilter(filter);
	TRACE("Selector::exec - filter : %s", filter.c_str());

	fl->browse();
	freeScreenshots(screens);
	TRACE("Selector::exec - found %i files and dirs", fl->size());
	this->numDirs = fl->dirCount();
	this->numFiles = fl->fileCount();

	screens->resize(fl->getFiles().size());
	titles->resize(fl->dirCount() + fl->getFiles().size());

	string fname, noext, realdir;
	string::size_type pos;
	string realPath = real_path(fl->getPath());
	if (realPath.length() > 0) {
		if (0 != realPath.compare(realPath.length() -1, 1, "/")) 
			realPath += "/";
	} else realPath = "/";
	bool previewsDirExists = dirExists(realPath + PREVIEWS_DIR);
	TRACE("Selector::prepare - realPath : %s, exists : %i", realPath.c_str(), previewsDirExists);

	// put all the dirs into titles first
	for (uint32_t i = 0; i < fl->dirCount(); i++) {
		titles->at(i) = fl->getDirectories()[i];
	}

	// then loop thru all the files
	for (uint32_t i = 0; i < fl->getFiles().size(); i++) {
		fname = fl->getFiles()[i];
		pos = fname.rfind(".");
		// cache a version of fname without extension
		if (pos != string::npos && pos > 0) 
			noext = fname.substr(0, pos);
		// and push into titles
		titles->at(fl->dirCount() + i) = getAlias(noext, fname);

		// are we looking for screen shots
		if (!screendir.empty() && 0 != gmenu2x->skin->previewWidth) {
			if (screendir[0] == '.') {
				// allow "." as "current directory", therefore, relative paths
				realdir = realPath + screendir + "/";
			} else realdir = real_path(screendir) + "/";

			// INFO("Searching for screen '%s%s.png'", realdir.c_str(), noext.c_str());
			if (fileExists(realdir + noext + ".jpg")) {
				screens->at(i) = realdir + noext + ".jpg";
			} else if (fileExists(realdir + noext + ".png")){
				screens->at(i) = realdir + noext + ".png";
			}
		}
		if (screens->at(i).empty()) {
			// fallback - always search for filename.png and jpg in a .previews folder inside the current path
			if (previewsDirExists && 0 != gmenu2x->skin->previewWidth) {
				if (fileExists(realPath + PREVIEWS_DIR + "/" + noext + ".png"))
					screens->at(i) = realPath + PREVIEWS_DIR + "/" + noext + ".png";
				else if (fileExists(realPath + PREVIEWS_DIR + "/" + noext + ".jpg"))
					screens->at(i) = realPath + PREVIEWS_DIR + "/" + noext + ".jpg";
			}
		}
		if (!screens->at(i).empty()) {
			TRACE("Selector::prepare - adding name: %s, screen : %s", fname.c_str(), screens->at(i).c_str());
		}
	}
	TRACE("Selector::prepare - exit - loaded %i screens", screens->size());
}

void Selector::freeScreenshots(vector<string> *screens) {
	for (uint32_t i = 0; i < screens->size(); i++) {
		if (!screens->at(i).empty())
			gmenu2x->sc.del(screens->at(i));
	}
}

void Selector::loadAliases() {
	TRACE("Selector::loadAliases - enter");
	aliases.clear();
	if (fileExists(link->getAliasFile())) {
		string line;
		ifstream infile (link->getAliasFile().c_str(), ios_base::in);
		while (getline(infile, line, '\n')) {
			string::size_type position = line.find("=");
			string name = trim(line.substr(0,position));
			string value = trim(line.substr(position+1));
			aliases[name] = value;
		}
		infile.close();
	}
	TRACE("Selector::loadAliases - exit : loaded %i aliases", aliases.size());
}

string Selector::getAlias(const string &key, const string &fname) {
	//TRACE("Selector::getAlias - enter");
	if (aliases.empty()) return fname;
	unordered_map<string, string>::iterator i = aliases.find(key);
	if (i == aliases.end())
		return fname;
	else
		return i->second;
}
