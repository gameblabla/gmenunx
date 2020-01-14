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

#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <vector>
#include <tr1/unordered_map>

using std::tr1::unordered_map;
using std::tr1::hash;
using std::string;
using std::vector;

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif /* NULL */

class case_less {
public:
	bool operator()(const string &left, const string &right) const;
};

string& ltrim(string& str, const string& chars = "\t\n\v\f\r ");
string& rtrim(string& str, const string& chars = "\t\n\v\f\r ");
string& full_trim(string& str, const string& chars = "\t\n\v\f\r ");

string trim(const string& s);
string strreplace (string orig, const string &search, const string &replace);
string cmdclean (string cmdline);

char *string_copy(const string &);
void string_copy(const string &, char **);

template <typename T>
string to_string( const T& value );

int max (int a, int b);
int min (int a, int b);
int constrain(int x, int imin, int imax);

int evalIntConf (int val, int def, int imin, int imax);
int evalIntConf (int *val, int def, int imin, int imax);
const string &evalStrConf (const string &val, const string &def);
const string &evalStrConf (string *val, const string &def);

float max (float a, float b);
float min (float a, float b);
float constrain (float x, float imin, float imax);

bool split (vector<string> &vec, const string &str, const string &delim, bool destructive=true);

int intTransition(int from, int to, int32_t tickStart, int32_t duration = 500, int32_t tickNow = -1);

string exec(const char* cmd);
string execute(const char* cmd);

bool procWriter(string path, string value);
bool procWriter(string path, int value);
string fileReader(string path);

string splitInLines(string source, std::size_t max_width, string whitespace = " \t\r");
string string_format(const std::string fmt_str, ...);
string stripQuotes(std::string const &input);
string toLower(const std::string & input);

std::string getOpkPath();

#include "debug.h"
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>

class FileUtils {

	public:

		// returns a filename minus the dot extension part
		static std::string fileBaseName(const std::string & filename) {
			std::string::size_type i = filename.rfind(".");
			if (i != std::string::npos) {
				return filename.substr(0, i);
			}
			return filename;
		}

		// returns a files dot extension if there is on,e or an empty string
		static std::string fileExtension(const std::string & filename) {
			std::string::size_type i = filename.rfind(".");
			if (i != std::string::npos) {
				return filename.substr(i, filename.length());
			}
			return "";
		}

		static bool copyFile(std::string from, std::string to) {
			TRACE("copying from : '%s' to '%s'", from.c_str(), to.c_str());
			if (!FileUtils::fileExists(from)) {
				ERROR("Copy file : Source doesn't exist : %s", from.c_str());
				return false;
			}
			if (FileUtils::fileExists(to)) {
				unlink(to.c_str());
			}
			std::ifstream  src(from, std::ios::binary);
			std::ofstream  dst(to,   std::ios::binary);
			dst << src.rdbuf();
			sync();
			return FileUtils::fileExists(to);
		}

		static bool rmTree(std::string path) {
			TRACE("enter : '%s'", path.c_str());

			DIR *dirp;
			struct stat st;
			struct dirent *dptr;
			std::string filepath;

			if ((dirp = opendir(path.c_str())) == NULL) return false;
			if (path[path.length()-1]!='/') path += "/";

			while ((dptr = readdir(dirp))) {
				filepath = dptr->d_name;
				if (filepath == "." || filepath == "..") continue;
				filepath = path + filepath;
				int statRet = stat(filepath.c_str(), &st);
				if (statRet == -1) continue;
				if (S_ISDIR(st.st_mode)) {
					if (!FileUtils::rmTree(filepath)) return false;
				} else {
					if (unlink(filepath.c_str())!=0) return false;
				}
			}
			closedir(dirp);
			return rmdir(path.c_str()) == 0;
		}

		static bool fileExists(const std::string &path) {
			TRACE("enter : %s", path.c_str());
			struct stat s;
			// check both that it exists and is a file or a link
			bool result = ( (lstat(path.c_str(), &s) == 0) && (S_ISREG(s.st_mode) || S_ISLNK(s.st_mode)) );
			TRACE("file '%s' exists : %i", path.c_str(), result);
			return result;
		}

		static bool dirExists(const std::string &path) {
			TRACE("enter : %s", path.c_str());
			struct stat s;
			bool result = (stat(path.c_str(), &s) == 0 && s.st_mode & S_IFDIR); // exists and is dir
			TRACE("dir '%s' exists : %i", path.c_str(), result);
			return result;
		}

		static std::string resolvePath(const std::string &path) {
			char max_path[PATH_MAX];
			std::string outpath;
			errno = 0;
			realpath(path.c_str(), max_path);
			TRACE("in: '%s', resolved: '%s'", path.c_str(), max_path);
			if (errno == ENOENT) {
				TRACE("NOENT came back");
				errno = 0;
				return FileUtils::firstFirstExistingDir(path);
				//return path;
			}
			return (std::string)max_path;
		}

		// returns the path to a file
		static std::string dirName(const std::string &path) {
			if (path.empty())
				return "/";
			std::string::size_type p = path.rfind("/");
			if (std::string::npos == p && '.' != path[0]) {
				// no slash and it's not a local path, it's not a dir
				return "";
			}
			if (p == path.size() - 1) {
				// last char is a /, so search for an earlier one
				p = path.rfind("/", p - 1);
			}
			std::string localPath = path.substr(0, p);
			TRACE("local path : '%s'", localPath.c_str());
			return FileUtils::resolvePath(localPath);
		}

		// returns a file name without any path part
		// or the last directory, essentially, anything after the last slash
		static std::string pathBaseName(const std::string &path) {
			std::string::size_type p = path.rfind("/");
			if (p == path.size() - 1) p = path.rfind("/", p - 1);
			return path.substr(p + 1, path.length());
		}

		static std::string firstFirstExistingDir(const std::string & path) {
			TRACE("enter : '%s'", path.c_str());
			if (path.empty() || std::string::npos == path.find("/"))
				return "/";
			if (FileUtils::dirExists(path)) {
				TRACE("raw result is : '%s'", path.c_str());
				std::string result = FileUtils::resolvePath(path);
				TRACE("real path : '%s'", result.c_str());
				return result;
			}
			const std::string & nextPath = FileUtils::dirName(path);
			TRACE("next path : '%s'", nextPath.c_str());
			return FileUtils::firstFirstExistingDir(nextPath);
		}

};

#endif
