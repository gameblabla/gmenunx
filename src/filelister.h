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

#ifndef FILELISTER_H_
#define FILELISTER_H_

#include <string>
#include <vector>

class FileLister {
private:
	std::string path, filter;
	std::vector<std::string> directories, files, excludes;
	bool showHidden;

public:
	bool showDirectories, showFiles, allowDirUp = true;

	FileLister(const std::string &startPath = "/media/", bool showDirectories = true, bool showFiles = true);
	void browse();

	uint32_t size();
	uint32_t dirCount();
	uint32_t fileCount();
	std::string operator[](uint32_t);
	std::string at(uint32_t);
	bool isFile(uint32_t);
	bool isDirectory(uint32_t);

	const std::string &getPath();
	void setPath(const std::string &path, bool doBrowse=true);
	const std::string &getFilter();
	void setFilter(const std::string &filter);

	const std::vector<std::string> &getDirectories() { return directories; }
	const std::vector<std::string> &getFiles() { return files; }
	void insertFile(const std::string &file);
	void addExclude(const std::string &exclude);

	void toggleHidden();
};

#endif /*FILELISTER_H_*/
