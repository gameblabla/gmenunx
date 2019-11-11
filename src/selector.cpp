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

string screendir;
const string PREVIEWS_DIR = ".previews";

Selector::Selector(GMenu2X *gmenu2x, LinkApp *link, const string &selectorDir) :
Dialog(gmenu2x)
{
	TRACE("Selector::Selector - enter : %s", selectorDir.c_str());
	this->link = link;
	loadAliases();
	selRow = 0;
	if (selectorDir.empty())
		dir = link->getSelectorDir();
	else
		dir = selectorDir;
	if (dir[dir.length() - 1] != '/') dir += "/";
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
	fl.setFilter(link->getSelectorFilter());
	fl.browse();
	TRACE("Selector::exec - found %i files and dirs", fl.size());

	// do we have screen shots?
	// if we do, they will live under this path, or this dir/screenshots
	screendir = link->getSelectorScreens();

	uint32_t i, firstElement = 0, iY, animation = 0, padding = 6;
	uint32_t rowHeight = gmenu2x->font->getHeight() + 1;
	uint32_t halfRowHeight = (rowHeight / 2);
	uint32_t numRows = ((gmenu2x->listRect.h - 2) / rowHeight) - 1;

	drawTopBar(this->bg, link->getTitle(), link->getDescription(), link->getIconPath());
	drawBottomBar(this->bg);

	this->bg->box(gmenu2x->listRect, gmenu2x->skin->colours.listBackground);

	if (link->getSelectorBrowser()) {
		gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Select"],
		gmenu2x->drawButton(this->bg, "a", gmenu2x->tr["Exit"])
		);
	} else {
		gmenu2x->drawButton(this->bg, "b", gmenu2x->tr["Select"],
		gmenu2x->drawButton(this->bg, "a", gmenu2x->tr["Exit"], 5)
	}

	prepare(&fl, &screens, &titles);
	int selected = constrain(startSelection, 0, fl.size() - 1);

	// moved surfaces out to prevent reloading on loop
	Surface *iconGoUp = gmenu2x->sc.skinRes("imgs/go-up.png");
	Surface *iconFolder = gmenu2x->sc.skinRes("imgs/folder.png");
	Surface *iconFile = gmenu2x->sc.skinRes("imgs/file.png");

	gmenu2x->sc.defaultAlpha = false;
	uint32_t tickStart = SDL_GetTicks();

	// kick off the chooser loop
	while (!close) {

		// bang the background in
		this->bg->blit(gmenu2x->s, 0, 0);

		if (!fl.size()) {
			MessageBox mb(gmenu2x, gmenu2x->tr["This directory is empty"]);
			mb.setAutoHide(1);
			mb.setBgAlpha(0);
			mb.exec();
		} else {
			//Selection wrap test
			if (selected >= firstElement + numRows) firstElement = selected - numRows;
			if (selected < firstElement) firstElement = selected;

			//Draw files & Directories
			iY = gmenu2x->listRect.y + 1;
			for (i = firstElement; i < fl.size() && i <= firstElement + numRows; i++, iY += rowHeight) {
				if (i == selected) {
					// slected item highlight
					gmenu2x->s->box(
						gmenu2x->listRect.x, 
						iY, 
						gmenu2x->listRect.w, 
						rowHeight, 
						gmenu2x->skin->colours.selectionBackground);
				}
				if (fl.isDirectory(i)) {
					if (fl[i] == "..")
						iconGoUp->blit(
							gmenu2x->s, 
							gmenu2x->listRect.x + 10, 
							iY + halfRowHeight, 
							HAlignCenter | VAlignMiddle);
					else
						iconFolder->blit(
							gmenu2x->s, 
							gmenu2x->listRect.x + 10, 
							iY + halfRowHeight, 
							HAlignCenter | VAlignMiddle);
				} else {
					iconFile->blit(
						gmenu2x->s, 
						gmenu2x->listRect.x + 10, 
						iY + halfRowHeight, 
						HAlignCenter | VAlignMiddle);
				}
				gmenu2x->s->write(
					gmenu2x->font, 
					titles[i], 
					gmenu2x->listRect.x + 21, 
					iY + halfRowHeight, 
					VAlignMiddle);
			}

			// screenshot logic
			int currentFileIndex = selected - fl.dirCount();
			if (currentFileIndex >= 0) {
				// we're in the files section
				if (!screens[currentFileIndex].empty()) {
					gmenu2x->s->box(
						gmenu2x->config->resolutionX - animation, 
						gmenu2x->listRect.y, 
						gmenu2x->skin->previewWidth, 
						gmenu2x->listRect.h, 
						gmenu2x->skin->colours.topBarBackground);

					gmenu2x->sc[screens[selected - fl.dirCount()]]->blit(
						gmenu2x->s, 
						{	gmenu2x->config->resolutionX - animation + padding, 
							gmenu2x->listRect.y + padding, 
							gmenu2x->skin->previewWidth - 2 * padding, 
							gmenu2x->listRect.h - 2 * padding
						}, 
						HAlignCenter | VAlignMiddle, 
						220);

					if (animation < gmenu2x->skin->previewWidth) {
						animation = intTransition(0, gmenu2x->skin->previewWidth, tickStart, 110);
						gmenu2x->s->flip();
						gmenu2x->input.setWakeUpInterval(45);
						continue;
					}

				} else {
					if (animation > 0) {
						// we only come in here if we had a screenshot before
						// and we need to clean it up
						gmenu2x->s->box(gmenu2x->config->resolutionX - animation, gmenu2x->listRect.y, gmenu2x->skin->previewWidth, gmenu2x->listRect.h, gmenu2x->skin->colours.topBarBackground);
						animation = gmenu2x->skin->previewWidth - intTransition(0, gmenu2x->skin->previewWidth, tickStart, 80);
						gmenu2x->s->flip();
						gmenu2x->input.setWakeUpInterval(45);
						continue;
					}
				}
			}
			gmenu2x->input.setWakeUpInterval(1000);
			gmenu2x->s->clearClipRect();
			gmenu2x->drawScrollBar(numRows, fl.size(), firstElement, gmenu2x->listRect);
			gmenu2x->s->flip();
		}

		// handle input
		do {
			inputAction = gmenu2x->input.update();
			if (inputAction) tickStart = SDL_GetTicks();

			if ( gmenu2x->input[UP] ) {
				selected -= 1;
				if (selected < 0) selected = fl.size() - 1;
			} else if ( gmenu2x->input[DOWN] ) {
				selected += 1;
				if (selected >= fl.size()) selected = 0;
			} else if ( gmenu2x->input[PAGEUP] || gmenu2x->input[LEFT] ) {
				selected -= numRows;
				if (selected < 0) selected = 0;
			} else if ( gmenu2x->input[PAGEDOWN] || gmenu2x->input[RIGHT] ) {
				selected += numRows;
				if (selected >= fl.size()) selected = fl.size() - 1;
			} else if ( gmenu2x->input[SETTINGS] ) {
				close = true;
				result = false;
			} else if ( gmenu2x->input[CANCEL] && link->getSelectorBrowser()) {
				string::size_type p = dir.rfind("/", dir.size() - 2);
				dir = dir.substr(0, p + 1);
				prepare(&fl, &screens, &titles);
			} else if ( gmenu2x->input[CONFIRM] ) {
				// file selected or dir selected
				if (fl.isFile(selected)) {
					file = fl[selected];
					close = true;
				} else {
					dir = real_path(dir + "/" + fl[selected]);
					selected = 0;
					firstElement = 0;
					prepare(&fl, &screens, &titles);
				}
			}
		} while (!inputAction);
	}

	gmenu2x->sc.defaultAlpha = true;
	freeScreenshots(&screens);

	TRACE("Selector::exec - exit : %i", result ? (int)selected : -1);
	return result ? (int)selected : -1;
}

// checks for screen shots etc
void Selector::prepare(FileLister *fl, vector<string> *screens, vector<string> *titles) {
	TRACE("Selector::prepare - enter");
	fl->setPath(dir);
	freeScreenshots(screens);
	screens->resize(fl->getFiles().size());
	titles->resize(fl->dirCount() + fl->getFiles().size());

	string fname, noext, realdir;
	string::size_type pos;
	string realPath = real_path(fl->getPath());
	bool previewsDirExists = dirExists(realPath + PREVIEWS_DIR);

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
		if (!screendir.empty()) {
			if (screendir[0] == '.') {
				// allow "." as "current directory", therefore, relative paths
				realdir = realPath + screendir + "/";
			} else realdir = real_path(screendir) + "/";

			// INFO("Searching for screen '%s%s.png'", realdir.c_str(), noext.c_str());
			if (fileExists(realdir + noext + ".jpg")) {
				screens->at(i) = realdir + noext + ".jpg";
				continue;
			} else if (fileExists(realdir + noext + ".png")){
				screens->at(i) = realdir + noext + ".png";
				continue;
			}
		}
		// fallback - always search for filename.png and jpg in a .previews folder inside the current path
		if (previewsDirExists) {
			if (fileExists(realPath + PREVIEWS_DIR + "/" + noext + ".png"))
				screens->at(i) = realPath + PREVIEWS_DIR + "/" + noext + ".png";
			else if (fileExists(realPath + PREVIEWS_DIR + "/" + noext + ".jpg"))
				screens->at(i) = realPath + PREVIEWS_DIR + "/" + noext + ".jpg";
			else
				screens->at(i) = "";
		} else screens->at(i) = "";
		TRACE("Selector::prepare - name: %s, screen : %s", fname.c_str(), screens->at(i).c_str());
	}
	TRACE("Selector::prepare - exit - loaded %i screens", screens->size());
}

void Selector::freeScreenshots(vector<string> *screens) {
	for (uint32_t i = 0; i < screens->size(); i++) {
		if (screens->at(i) != "")
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
	TRACE("Selector::getAlias - enter");
	if (aliases.empty()) return fname;
	unordered_map<string, string>::iterator i = aliases.find(key);
	if (i == aliases.end())
		return fname;
	else
		return i->second;
}
