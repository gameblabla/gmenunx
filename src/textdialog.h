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

#ifndef TEXTDIALOG_H_
#define TEXTDIALOG_H_

#include <string>
#include "esoteric.h"
#include "dialog.h"

class TextDialog : protected Dialog {
protected:
	std::vector<std::string> text;
	std::string title, description, icon, backdrop, rawText = "";

	void preProcess();
	void drawText(std::vector<std::string> *text, uint32_t firstRow, uint32_t rowsPerPage);

public:
	TextDialog(Esoteric *app, const std::string &title, const std::string &description, const std::string &icon, const std::string &backdrop = "");

	void appendText(const std::string &text);
	void appendFile(const std::string &file);
	void appendCommand(const std::string &executable, const std::string &args = "");
	void exec();
};

#endif /*TEXTDIALOG_H_*/
