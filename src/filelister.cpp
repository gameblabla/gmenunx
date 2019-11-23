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
#include <dirent.h>
#include <algorithm>

#include "filelister.h"
#include "utilities.h"
#include "gmenu2x.h"
#include "debug.h"

using namespace std;

FileLister::FileLister(const string &startPath, bool showDirectories, bool showFiles)
	: showDirectories(showDirectories), showFiles(showFiles) {
	setPath(startPath, false);
}

const string &FileLister::getPath() {
	return path;
}
void FileLister::setPath(const string &path, bool doBrowse) {
	TRACE("FileLister::setpath - enter - path : %s, browse : %i", path.c_str(), doBrowse);
	this->path = real_path(path);
	if (doBrowse)
		browse();
}

const string &FileLister::getFilter() {
	return filter;
}
void FileLister::setFilter(const string &filter) {
	TRACE("FileLister::setFilter - %s", filter.c_str());
	this->filter = trim(filter);
}

void FileLister::browse() {
	TRACE("FileLister::browse - enter");
	directories.clear();
	files.clear();

	if (showDirectories || showFiles) {
		DIR *dirp;
		if ((dirp = opendir(path.c_str())) == NULL) {
			ERROR("Error: opendir(%s)", path.c_str());
			return;
		}

		vector<string> vfilter;
		split(vfilter, this->getFilter(), ",");

		string filepath, file;
		struct stat st;
		struct dirent *dptr;
		bool anyExcludes = excludes.empty();

		while ((dptr = readdir(dirp))) {
			file = dptr->d_name;
			TRACE("FileLister::browse - raw result : %s", file.c_str());
			// skip self and hidden dirs
			if (file[0] == '.') continue;
			// checked supressed list
			if (anyExcludes) {
				if (find(excludes.begin(), excludes.end(), file) != excludes.end())
					continue;
			}

			//TRACE("FileLister::browse - raw result : %s - post excludes", file.c_str());
			filepath = path + "/" + file;
			int statRet = stat(filepath.c_str(), &st);
			if (statRet == -1) {
				ERROR("Stat failed on '%s' with error '%s'", filepath.c_str(), strerror(errno));
				continue;
			}
			//TRACE("FileLister::browse - raw result : %s - post stat", file.c_str());

			if (S_ISDIR(st.st_mode)) {
				if (!showDirectories) continue;
				//TRACE("FileLister::browse - adding directory : %s", file.c_str());
				directories.push_back(file);
			} else {
				if (!showFiles) continue;
				if (vfilter.empty()) {
					//TRACE("FileLister::browse - no filters, so adding file : %s", file.c_str());
					files.push_back(file);
					continue;
				} else {
					//loop through each filter and check the end of the file for a match
					//TRACE("FileLister::browse - raw result : %s - checking filters", file.c_str());
					for (vector<string>::iterator it = vfilter.begin(); it != vfilter.end(); ++it) {
						// skip any empty filters...
						//TRACE("FileLister::browse - itterator is : %s", (*it).c_str());
						int filterLength = (*it).length();
						//TRACE("FileLister::browse - itterator length test : %i", filterLength);
						if (0 == filterLength) continue;
						// skip testing any files shorter than the filter size
						//TRACE("FileLister::browse - file length test : %i < %i", file.length(), filterLength);
						if (file.length() < filterLength) continue;

						//TRACE("FileLister::browse - ends with test");
						// the real test, does the end match our filter
						if (file.compare(file.length() - filterLength, filterLength, *it) == 0) {
							//TRACE("FileLister::browse - adding file : %s", file.c_str());
							files.push_back(file);
							break;
						}
					}
				}
			}
		}
		closedir(dirp);
		//TRACE("FileLister::browse - sort starts");
		std::sort(files.begin(), files.end(), case_less());
		std::sort(directories.begin(), directories.end(), case_less());
		//TRACE("FileLister::browse - sort ended");
		// add a dir up option at the front if not excluded
		if (showDirectories && path != "/" && allowDirUp) 
			directories.insert(directories.begin(), "..");
	}

	TRACE("FileLister::browse - exit");
}

uint32_t FileLister::size() {
	return files.size() + directories.size();
}
uint32_t FileLister::dirCount() {
	return directories.size();
}
uint32_t FileLister::fileCount() {
	return files.size();
}

string FileLister::operator[](uint32_t x) {
	return at(x);
}

string FileLister::at(uint32_t x) {
	if (x >= size()) return "";
	if (x < directories.size())
		return directories[x];
	else
		return files[x - directories.size()];
}

bool FileLister::isFile(uint32_t x) {
	return x >= directories.size() && x < size();
}

bool FileLister::isDirectory(uint32_t x) {
	return x < directories.size();
}

void FileLister::insertFile(const string &file) {
	files.insert(files.begin(), file);
}

void FileLister::addExclude(const string &exclude) {
	if (exclude == "..") allowDirUp = false;
	excludes.push_back(exclude);
}
