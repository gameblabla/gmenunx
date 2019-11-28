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

bool fileExists(const string &path);
bool dirExists(const string &path);
bool rmtree(string path);

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
string real_path(const string &path);
string dir_name(const string &path);
string base_name(const string &path);
string fileBaseName(const string & filename);
string fileExtension(const string & filename);

bool procWriter(string path, string value);
bool procWriter(string path, int value);
string fileReader(string path);
bool copyFile(string from, string to);

string splitInLines(string source, std::size_t max_width, string whitespace = " \t\r");
string string_format(const std::string fmt_str, ...);
string stripQuotes(std::string const &input);
string toLower(const std::string & input);

char *ms2hms(uint32_t t, bool mm = true, bool ss = true);

#endif
