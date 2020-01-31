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
#include "stringutils.h"

#include "debug.h"

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

bool case_less::operator()(const std::string &left, const std::string &right) const {
	return strcasecmp(left.c_str(), right.c_str()) < 0;
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
	result = StringUtils::fullTrim(result);
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

