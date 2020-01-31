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
#include "fileutils.h"
#include "debug.h"

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

bool case_less::operator()(const std::string &left, const std::string &right) const {
	return strcasecmp(left.c_str(), right.c_str()) < 0;
}

std::string& ltrim(std::string& str, const std::string& chars) {
    str.erase(0, str.find_first_not_of(chars));
    return str;
}
 
std::string& rtrim(std::string& str, const std::string& chars) {
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

std::string& full_trim(std::string& str, const std::string& chars) {
    return ltrim(rtrim(str, chars), chars);
}

// General tool to strip spaces from both ends:
std::string trim(const std::string &s) {
  if (s.length() == 0)
    return s;
  std::size_t b = s.find_first_not_of(" \t\r");
  std::size_t e = s.find_last_not_of(" \t\r");
  if (b == -1) // No non-spaces
    return "";
  return std::string(s, b, e - b + 1);
}


void string_copy(const std::string &s, char **cs) {
	*cs = (char*)malloc(s.length());
	strcpy(*cs, s.c_str());
}

char *string_copy(const std::string &s) {
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

const std::string &evalStrConf(const std::string &val, const std::string &def) {
	return val.empty() ? def : val;
}

const std::string &evalStrConf(std::string *val, const std::string &def) {
	*val = evalStrConf(*val, def);
	return *val;
}

bool split(std::vector<std::string> &vec, const std::string &str, const std::string &delim, bool destructive) {

	TRACE("enter - string : '%s', delim : '%s'", str.c_str(), delim.c_str());
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
		j = str.find(delim, i);
		if (j == std::string::npos) {
			TRACE("adding : '%s'", str.substr(i).c_str());
			vec.push_back(str.substr(i));
			break;
		}

		if (!destructive)
			j += delim.size();

		TRACE("adding : '%s'", str.substr(i, j - i).c_str());
		vec.push_back(str.substr(i, j - i));

		if (destructive)
			i = j + delim.size();

		if (i == str.size()) {
			TRACE("adding empty string"); 
			vec.push_back(std::string());
			break;
		}
	}
	TRACE("exit");
	return true;
}

std::string strreplace(std::string orig, const std::string &search, const std::string &replace) {
	if (0 == search.compare(replace)) return orig;
	std::string::size_type pos = orig.find( search, 0 );
	while (pos != std::string::npos) {
		orig.replace(pos, search.length(), replace);
		pos = orig.find( search, pos + replace.length() );
	}
	return orig;
}

std::string cmdclean(std::string cmdline) {
	TRACE("cmdclean - enter : %s", cmdline.c_str());
	std::string spchars = "\\`$();|{}&'\"*?<>[]!^~-#\n\r ";
	for (uint32_t i = 0; i < spchars.length(); i++) {
		std::string curchar = spchars.substr(i, 1);
		cmdline = strreplace(cmdline, curchar, "\\" + curchar);
	}
	TRACE("cmdclean - exit : %s", cmdline.c_str());
	return cmdline;
}

std::string exec(const char* cmd) {
	TRACE("exec - enter : %s", cmd);
	FILE* pipe = popen(cmd, "r");
	if (!pipe) {
		TRACE("couldn't get a pipe");
		return "";
	}
	char buffer[128];
	std::string result = "";
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

std::string execute(const char* cmd) { 
	return exec(cmd); 
}

bool procWriter(std::string path, std::string value) {
	TRACE("%s - %s", path.c_str(), value.c_str());
	if (FileUtils::fileExists(path)) {
		TRACE("file exists");
		std::ofstream str(path);
		str << value;
		str.close();
		TRACE("success");
		return true;
	}
	return false;
}

bool procWriter(std::string path, int value) {
	std::stringstream ss;
	std::string strVal;
	ss << value;
	std::getline(ss, strVal);
	return procWriter(path, strVal);
}

std::string fileReader(std::string path) {
	std::ifstream str(path);
	std::stringstream buf;
	buf << str.rdbuf();
	return buf.str();
}

std::string splitInLines(std::string source, size_t max_width, std::string whitespace) {
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

std::string string_format(const std::string fmt_str, ...) {
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
    std::string result = input;
    if (input.at(0) == '"' && input.at(input.length() - 1) == '"') {
        result = input.substr(1, input.length() - 2);
    }
    return result;
}

std::string toLower(const std::string & input) {
	std::string copy = input;
	transform(copy.begin(), copy.end(), copy.begin(), (int(*)(int)) tolower);
	return copy;
}
