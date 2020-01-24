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
#include "esoteric.h"
#include "debug.h"

FileLister::FileLister(const std::string &startPath, bool showDirectories, bool showFiles)
	: showDirectories(showDirectories), showFiles(showFiles) {

	this->setPath(startPath, false);
}

const std::string &FileLister::getPath() {
	return path;
}
void FileLister::setPath(const std::string &path, bool doBrowse) {
	TRACE("enter - path : %s, browse : %i", path.c_str(), doBrowse);
	this->path = FileUtils::resolvePath(path);
	if (doBrowse)
		this->browse();
}

const std::string &FileLister::getFilter() {
	return this->filter;
}
void FileLister::setFilter(const std::string &filter) {
	TRACE("%s", this->filter.c_str());
	this->filter = trim(filter);
}

void FileLister::browse() {
	TRACE("enter");

	this->directories.clear();
	this->files.clear();

	if (this->showDirectories || this->showFiles) {
		TRACE("we have some work to do because of flags");
		if (FileUtils::dirExists(this->path)) {
			DIR *dirp;
			if ((dirp = opendir(this->path.c_str())) == NULL) {
				ERROR("Error: opendir(%s)", this->path.c_str());
			} else {
				std::vector<std::string> vfilter;
				split(vfilter, this->getFilter(), ",");
				TRACE("we have : %zu filters to deal with", vfilter.size());
				std::string filepath, file;
				struct stat st;
				struct dirent *dptr;
				bool anyExcludes = this->excludes.empty();

				while ((dptr = readdir(dirp))) {
					file = dptr->d_name;
					TRACE("raw result : %s", file.c_str());
					// skip self and hidden dirs
					if (file[0] == '.') continue;
					// checked supressed list
					if (anyExcludes) {
						if (find(this->excludes.begin(), this->excludes.end(), file) != this->excludes.end()) {
							TRACE("skipping beaused of exclusion");
							continue;
						}
					}

					//TRACE("raw result : %s - post excludes", file.c_str());
					filepath = this->path + "/" + file;
					int statRet = stat(filepath.c_str(), &st);
					if (statRet == -1) {
						ERROR("Stat failed on '%s' with error '%s'", filepath.c_str(), strerror(errno));
						continue;
					}
					//TRACE("raw result : %s - post stat", file.c_str());

					if (S_ISDIR(st.st_mode)) {
						if (!this->showDirectories) continue;
						TRACE("adding directory : %s", file.c_str());
						this->directories.push_back(file);
					} else {
						if (!this->showFiles) continue;
						if (vfilter.empty()) {
							TRACE("no filters, so adding file : %s", file.c_str());
							this->files.push_back(file);
							continue;
						} else {
							//loop through each filter and check the end of the file for a match
							//TRACE("raw result : %s - checking filters", file.c_str());
							for (std::vector<std::string>::iterator it = vfilter.begin(); it != vfilter.end(); ++it) {
								// skip any empty filters...
								//TRACE("iterator is : %s", (*it).c_str());
								int filterLength = (*it).length();
								//TRACE("iterator length test : %i", filterLength);

								// skip testing any files shorter than the filter size
								//TRACE("file length test : %i < %i", file.length(), filterLength);
								if (file.length() < filterLength) continue;

								if (0 == filterLength) {
									if (std::string::npos == file.find(".")) {
										//TRACE("empty filter and no dot");
										TRACE("adding file : %s", file.c_str());
										this->files.push_back(file);
										break;
									} else {
										TRACE("skipping : '%s' because of dot", file.c_str());
										continue;
									}
								}
								// the real test, does the end match our filter
								if (file.compare(file.length() - filterLength, filterLength, *it) == 0) {
									TRACE("adding file : %s", file.c_str());
									this->files.push_back(file);
									break;
								}
							}
						}
					}
				}

				closedir(dirp);
				std::sort(this->files.begin(), this->files.end(), case_less());
				std::sort(this->directories.begin(), this->directories.end(), case_less());
			}
		}
		// add a dir up option at the front if not excluded
		if (this->showDirectories && this->path != "/" && this->allowDirUp) {
			TRACE("adding the .. nav option");
			this->directories.insert(this->directories.begin(), "..");
		}
	}
	TRACE("exit");
}

uint32_t FileLister::size() {
	return this->files.size() + this->directories.size();
}
uint32_t FileLister::dirCount() {
	return this->directories.size();
}
uint32_t FileLister::fileCount() {
	return this->files.size();
}

std::string FileLister::operator[](uint32_t x) {
	return this->at(x);
}

std::string FileLister::at(uint32_t x) {
	if (x >= this->size()) return "";
	if (x < this->directories.size())
		return this->directories[x];
	else
		return this->files[x - this->directories.size()];
}

bool FileLister::isFile(uint32_t x) {
	return x >= this->directories.size() && x < size();
}

bool FileLister::isDirectory(uint32_t x) {
	return x < this->directories.size();
}

void FileLister::insertFile(const std::string &file) {
	this->files.insert(this->files.begin(), file);
}

void FileLister::addExclude(const std::string &exclude) {
	if (exclude == "..") this->allowDirUp = false;
	this->excludes.push_back(exclude);
}
