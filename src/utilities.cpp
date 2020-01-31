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

#include <string>

#include "utilities.h"

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
