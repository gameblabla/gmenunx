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

#include "surfacecollection.h"
#include "surface.h"
#include "utilities.h"
#include "skin.h"
#include "debug.h"

using std::endl;
using std::string;

SurfaceCollection::SurfaceCollection(Skin *skin, bool defaultAlpha) {
	this->defaultAlpha = defaultAlpha;
	this->skin = skin;
}

SurfaceCollection::~SurfaceCollection() {}

void SurfaceCollection::debug() {
	SurfaceHash::iterator end = surfaces.end();
	for(SurfaceHash::iterator curr = surfaces.begin(); curr != end; curr++){
		TRACE("key: %s", curr->first.c_str());
	}
}

bool SurfaceCollection::exists(const string &path) {
	return surfaces.find(path) != surfaces.end();
}

Surface *SurfaceCollection::addImage(const string &path, bool alpha) {
	return this->add(path, alpha, this->skin->imagesToGrayscale);
}

Surface *SurfaceCollection::addIcon(const string &path, bool alpha) {
	return this->add(path, alpha, this->skin->iconsToGrayscale);
}

Surface *SurfaceCollection::add(const string &path, bool alpha, bool grayScale) {
	TRACE("SurfaceCollection::add - enter - %s", path.c_str());
	if (path.empty()) return NULL;
	TRACE("SurfaceCollection::add - path exists test");
	if (exists(path)) return surfaces[path];

	string filePath = path;
	if (filePath.substr(0,5)=="skin:") {
		TRACE("SurfaceCollection::add - matched on skin:");
		filePath = this->skin->getSkinFilePath(filePath.substr(5,filePath.length()));
		TRACE("SurfaceCollection::add - filepath - %s", filePath.c_str());
		if (filePath.empty())
			return NULL;
	} else if (!fileExists(filePath)) {
		TRACE("SurfaceCollection::add - file doesn't exist");
		return NULL;
	}

	TRACE("Adding surface: '%s'", path.c_str());
	Surface *s = new Surface(filePath, alpha);
	if (s != NULL) {
		if (grayScale)
			s->toGrayScale();
		TRACE("SurfaceCollection::add - adding surface to collection");
		surfaces[path] = s;
	}
	TRACE("SurfaceCollection::add - exit");
	return s;
}

Surface *SurfaceCollection::addSkinRes(const string &path, bool alpha) {
	if (path.empty()) return NULL;
	if (exists(path)) return surfaces[path];

	string skinpath = this->skin->getSkinFilePath(path);
	if (skinpath.empty())
		return NULL;

	TRACE("Adding skin surface: '%s:%s'", skinpath.c_str(), path.c_str());
	Surface *s = new Surface(skinpath, alpha);
	if (s != NULL) {
		if (this->skin->iconsToGrayscale) {
			s->toGrayScale();
		}
		surfaces[path] = s;
	}
	return s;
}

void SurfaceCollection::del(const string &path) {
	SurfaceHash::iterator i = surfaces.find(path);
	if (i != surfaces.end()) {
		delete i->second;
		surfaces.erase(i);
	}
}

int SurfaceCollection::size() {
	return surfaces.size();
}

bool SurfaceCollection::empty() {
	return surfaces.empty();
}

void SurfaceCollection::clear() {
	while (surfaces.size() > 0) {
		if (surfaces.begin()->second) {
			delete surfaces.begin()->second;
		}
		surfaces.erase(surfaces.begin());
	}
}

void SurfaceCollection::move(const string &from, const string &to) {
	del(to);
	surfaces[to] = surfaces[from];
	surfaces.erase(from);
}

Surface *SurfaceCollection::operator[](const string &key) {
	SurfaceHash::iterator i = surfaces.find(key);
	if (i == surfaces.end())
		return addIcon(key, defaultAlpha);

	return i->second;
}

Surface *SurfaceCollection::skinRes(const string &key) {
	if (key.empty()) return NULL;
	SurfaceHash::iterator i = surfaces.find(key);
	if (i == surfaces.end())
		return addSkinRes(key, defaultAlpha);

	return i->second;
}
