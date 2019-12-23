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
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <memory>
#include <algorithm>

#include <SDL.h>

#include "utilities.h"
#include "debug.h"

using namespace std;

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

bool case_less::operator()(const string &left, const string &right) const {
	return strcasecmp(left.c_str(), right.c_str()) < 0;
}

string& ltrim(string& str, const string& chars) {
    str.erase(0, str.find_first_not_of(chars));
    return str;
}
 
string& rtrim(string& str, const string& chars) {
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

string& full_trim(string& str, const string& chars) {
    return ltrim(rtrim(str, chars), chars);
}

// General tool to strip spaces from both ends:
string trim(const string &s) {
  if (s.length() == 0)
    return s;
  std::size_t b = s.find_first_not_of(" \t\r");
  std::size_t e = s.find_last_not_of(" \t\r");
  if (b == -1) // No non-spaces
    return "";
  return string(s, b, e - b + 1);
}


void string_copy(const string &s, char **cs) {
	*cs = (char*)malloc(s.length());
	strcpy(*cs, s.c_str());
}

char *string_copy(const string &s) {
	char *cs = NULL;
	string_copy(s, &cs);
	return cs;
}

template <typename T>
std::string to_string( const T& value ) {
  std::ostringstream ss;
  ss << value;
  return ss.str();
}

bool dirExists(const string &path) {
	TRACE("dirExists - enter : %s", path.c_str());
	struct stat s;
	bool result = (stat(path.c_str(), &s) == 0 && s.st_mode & S_IFDIR); // exists and is dir
	TRACE("dirExists - result : %i", result);
	return result;
}

bool fileExists(const string &path) {
	struct stat s;
	// check both that it exists and is file
	bool result = ( (stat(path.c_str(), &s) == 0) && s.st_mode & S_IFREG); 
	TRACE("file '%s' exists : %i", path.c_str(), result);
	return result;
}

bool rmtree(string path) {
	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath;

	TRACE("RMTREE: '%s'", path.c_str());

	if ((dirp = opendir(path.c_str())) == NULL) return false;
	if (path[path.length()-1]!='/') path += "/";

	while ((dptr = readdir(dirp))) {
		filepath = dptr->d_name;
		if (filepath=="." || filepath=="..") continue;
		filepath = path+filepath;
		int statRet = stat(filepath.c_str(), &st);
		if (statRet == -1) continue;
		if (S_ISDIR(st.st_mode)) {
			if (!rmtree(filepath)) return false;
		} else {
			if (unlink(filepath.c_str())!=0) return false;
		}
	}

	closedir(dirp);
	return rmdir(path.c_str())==0;
}

int max(int a, int b) {
	return a > b ? a : b;
}

float max(float a, float b) {
	return a > b ? a : b;
}

int min(int a, int b) {
	return a < b ? a : b;
}

float min(float a, float b) {
	return a < b ? a : b;
}

int constrain(int x, int imin, int imax) {
	return min(imax, max(imin, x));
}

float constrain(float x, float imin, float imax) {
	return min(imax, max(imin, x));
}

//Configuration parsing utilities
int evalIntConf(int val, int def, int imin, int imax) {
	return evalIntConf(&val, def, imin, imax);
}

int evalIntConf(int *val, int def, int imin, int imax) {
	if (*val == 0 && (*val < imin || *val > imax))
		*val = def;
	else
		*val = constrain(*val, imin, imax);
	return *val;
}

const string &evalStrConf(const string &val, const string &def) {
	return val.empty() ? def : val;
}

const string &evalStrConf(string *val, const string &def) {
	*val = evalStrConf(*val, def);
	return *val;
}

bool split(vector<string> &vec, const string &str, const string &delim, bool destructive) {
	vec.clear();

	if (str.empty())
		return false;

	if (delim.empty()) {
		vec.push_back(str);
		return false;
	}

	std::string::size_type i = 0;
	std::string::size_type j = 0;

	while(1) {
		j = str.find(delim,i);
		if ( j== std::string::npos) {
			vec.push_back(str.substr(i));
			break;
		}

		if (!destructive)
			j += delim.size();

		vec.push_back(str.substr(i,j-i));

		if (destructive)
			i = j + delim.size();

		if (i == str.size()) {
			vec.push_back(std::string());
			break;
		}
	}

	return true;
}

string strreplace(string orig, const string &search, const string &replace) {
	if (0 == search.compare(replace)) return orig;
	string::size_type pos = orig.find( search, 0 );
	while (pos != string::npos) {
		orig.replace(pos, search.length(), replace);
		pos = orig.find( search, pos + replace.length() );
	}
	return orig;
}

string cmdclean(string cmdline) {
	TRACE("cmdclean - enter : %s", cmdline.c_str());
	string spchars = "\\`$();|{}&'\"*?<>[]!^~-#\n\r ";
	for (uint32_t i = 0; i < spchars.length(); i++) {
		string curchar = spchars.substr(i, 1);
		cmdline = strreplace(cmdline, curchar, "\\" + curchar);
	}
	TRACE("cmdclean - exit : %s", cmdline.c_str());
	return cmdline;
}

int intTransition(int from, int to, int32_t tickStart, int32_t duration, int32_t tickNow) {
	if (tickNow < 0) tickNow = SDL_GetTicks();
	float elapsed = (float)(tickNow-tickStart)/duration;
	//                    elapsed                 increments
	return min((int)round(elapsed * (to - from)), (int)max(from, to));
}

bool copyFile(string from, string to) {
	if (!fileExists(from)) {
		ERROR("Copy file : Source doesn't exist : %s", from.c_str());
		return false;
	}
	if (fileExists(to)) {
		unlink(to.c_str());
	}
	std::ifstream  src(from, std::ios::binary);
    std::ofstream  dst(to,   std::ios::binary);
    dst << src.rdbuf();
	sync();
	return fileExists(to);
}

string exec(const char* cmd) {
	TRACE("exec - enter : %s", cmd);
	FILE* pipe = popen(cmd, "r");
	if (!pipe) {
		TRACE("couldn't get a pipe");
		return "";
	}
	char buffer[128];
	string result = "";
	while (!feof(pipe)) {
		if(fgets(buffer, sizeof buffer, pipe) != NULL) {
			//TRACE("exec - buffer : %s", buffer);
			result += buffer;
		}
	}
	pclose(pipe);
	result = full_trim(result);
	TRACE("exec - exit : %s", result.c_str());
	return result;
}

string real_path(const string &path) {
	char real_path[PATH_MAX];
	string outpath;
	realpath(path.c_str(), real_path);
	if (errno == ENOENT) return path;
	outpath = (string)real_path;
	return outpath;
}

string dir_name(const string &path) {
	string::size_type p = path.rfind("/");
	if (p == path.size() - 1) p = path.rfind("/", p - 1);
	return real_path("/" + path.substr(0, p));
}

string base_name(const string &path) {
	string::size_type p = path.rfind("/");
	if (p == path.size() - 1) p = path.rfind("/", p - 1);
	return path.substr(p + 1, path.length());
}

string fileBaseName(const string & filename) {
	string::size_type i = filename.rfind(".");
	if (i != string::npos) {
		return filename.substr(0,i);
	}
	return filename;
}

string fileExtension(const string & filename) {
	string::size_type i = filename.rfind(".");
	if (i != string::npos) {
		return filename.substr(i, filename.length());
	}
	return "";
}

bool procWriter(string path, string value) {
	TRACE("%s - %s", path.c_str(), value.c_str());
	if (fileExists(path)) {
		TRACE("file exists");
		ofstream str(path);
		str << value;
		str.close();
		TRACE("success");
		return true;
	}
	return false;
}

bool procWriter(string path, int value) {
	stringstream ss;
	string strVal;
	ss << value;
	std::getline(ss, strVal);
	return procWriter(path, strVal);
}

string fileReader(string path) {
	ifstream str(path);
	stringstream buf;
	buf << str.rdbuf();
	return buf.str();
}

string splitInLines(string source, size_t max_width, string whitespace) {
    size_t  currIndex = max_width - 1;
    size_t  sizeToElim;
    while ( currIndex < source.length() ) {
        currIndex = source.find_last_of(whitespace,currIndex + 1); 
        if (currIndex == std::string::npos)
            break;
        currIndex = source.find_last_not_of(whitespace,currIndex);
        if (currIndex == std::string::npos)
            break;
        sizeToElim = source.find_first_not_of(whitespace,currIndex + 1) - currIndex - 1;
        source.replace( currIndex + 1, sizeToElim , "\n");
        currIndex += (max_width + 1); //due to the recently inserted "\n"
    }
    return source;
}

string string_format(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]);
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

std::string stripQuotes(std::string const &input) {
    string result = input;
    if (input.at(0) == '"' && input.at(input.length() - 1) == '"') {
        result = input.substr(1, input.length() - 2);
    }
    return result;
}

std::string toLower(const std::string & input) {
	string copy = input;
	transform(copy.begin(), copy.end(), copy.begin(), (int(*)(int)) tolower);
	return copy;
}

std::string getOpkPath() {
	TRACE("enter");
	// this is sucky, but we need to grep the pid
	// because of 52 char width on the term, 
	// and then read /proc/pid/cmdline
	// and parse that
	std::string cmd = "/bin/ps | /bin/grep opkrun";
	std::string response = exec(cmd.c_str());
	TRACE("response : %s", response.c_str());
	std::istringstream stm(response);
	std::string pid = "";
	stm >> pid;
	TRACE("got pid : %s", pid.c_str());
	if (pid.length() == 0)
		return "";
	cmd = "/proc/" + pid + "/cmdline";
	if (!fileExists(cmd)) {
		TRACE("no proc file for pid : %s", pid.c_str());
		return "";
	}

	const int BUFSIZE = 4096;
	unsigned char buffer[BUFSIZE]; 
	int fd = open(cmd.c_str(), O_RDONLY);
	if (0 == fd) {
		TRACE("couldn't open : %s", cmd.c_str());
		return "";
	}
	int nbytesread = read(fd, buffer, BUFSIZE);
	unsigned char *end = buffer + nbytesread;
	string opk = "";
	string search = ".opk";
	for (unsigned char *p = buffer; p < end; ) { 
		TRACE("proc read : %s", p);
		string token = reinterpret_cast<char*>(p);
		if (token.length() > search.length()) {
			TRACE("%s is longer that : %s", token.c_str(), search.c_str());
			std::string::size_type pos = token.find(search);
			if (pos == (token.length() - search.length())) {
				opk = token;
				break;
			}
		}
		while (*p++);
	}
	close(fd);
	if (opk.length() == 0)
		return "";
	if (!fileExists(opk)) {
		ERROR("Opk '%s' doesn't exist", response.c_str());
		return "";
	}
	return opk;
}
