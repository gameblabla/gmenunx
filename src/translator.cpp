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

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdarg.h>

#include "translator.h"
#include "debug.h"

Translator::Translator(const std::string &lang) {
	_path = "";
	_lang = "";
	if (!lang.empty())
		setLang(lang);
}

Translator::~Translator() {}

bool Translator::exists(const std::string &term) {
	return translations.find(term) != translations.end();
}

void Translator::setPath(const std::string &path) {
	TRACE("%s", path.c_str());
	_path = path;
	if (!_lang.empty())
		setLang(_lang);
}

void Translator::setLang(const std::string &lang) {
	TRACE("%s", lang.c_str());
	translations.clear();

	std::string line;
	std::string path = _path + "translations/" + lang;
	TRACE("opening - %s", path.c_str());
	std::ifstream infile (path.c_str(), std::ios_base::in);
	if (infile.is_open()) {
		while (getline(infile, line, '\n')) {
			line = trim(line);
			if (line.empty()) continue;
			if (line[0]=='#') continue;

			std::string::size_type position = line.find("=");
			translations[ trim(line.substr(0,position)) ] = trim(line.substr(position+1));
		}
		infile.close();
		TRACE("read completed");
		_lang = lang;
	}
}

std::string Translator::translate(const std::string &term,const char *replacestr,...) {
	std::string result = term;

	if (!_lang.empty()) {
		std::tr1::unordered_map<std::string, std::string>::iterator i = translations.find(term);
		if (i != translations.end()) {
			result = i->second;
		}

		//else WARNING("Untranslated string: '%s'", term.c_str());
	}

	va_list arglist;
	va_start(arglist, replacestr);
	const char *param = replacestr;
	int argnum = 1;
	while (param!=NULL) {
		std::string id;
		std::stringstream ss; ss << argnum; ss >> id;
		result = strreplace(result,"$" + id,param);

		param = va_arg(arglist,const char*);
		argnum++;
	}
	va_end(arglist);

	return result;
}

std::string Translator::operator[](const std::string &term) {
	return translate(term);
}

std::string Translator::lang() {
	return _lang;
}
