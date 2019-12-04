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

#include "textdialog.h"
#include "messagebox.h"
#include "utilities.h"
#include "debug.h"
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

TextDialog::TextDialog(Esoteric *app, const string &title, const string &description, const string &icon, const string &backdrop)
	: Dialog(app), title(title), description(description), icon(icon), backdrop(backdrop)
{}

void TextDialog::preProcess() {
	uint32_t i = 0;
	string row;

	split(text, rawText, "\n");

	while (i < text.size()) {
		//clean this row
		row = trim(text.at(i));

		//check if this row is not too long
		if (app->font->getTextWidth(row) > app->config->resolutionX() - 15) {
			vector<string> words;
			split(words, row, " ");

			uint32_t numWords = words.size();
			//find the maximum number of rows that can be printed on screen
			while (app->font->getTextWidth(row) > app->config->resolutionX() - 15 && numWords > 0) {
				numWords--;
				row = "";
				for (uint32_t x = 0; x < numWords; x++)
					row += words[x] + " ";
				row = trim(row);
			}

			//if numWords==0 then the string must be printed as-is, it cannot be split
			if (numWords > 0) {
				//replace with the shorter version
				text.at(i) = row;

				//build the remaining text in another row
				row = "";
				for (uint32_t x = numWords; x < words.size(); x++)
					row += words[x] + " ";
				row = trim(row);

				if (!row.empty())
					text.insert(text.begin() + i + 1, row);
			}
		}
		i++;
	}
}

void TextDialog::drawText(vector<string> *text, uint32_t firstRow, uint32_t rowsPerPage) {
	app->screen->setClipRect(app->listRect);

	for (uint32_t i = firstRow; i < firstRow + rowsPerPage && i < text->size(); i++) {
		int rowY;
		if (text->at(i)=="----") { //draw a line
			rowY = app->listRect.y + (int)((i - firstRow + 0.5) * app->font->getHeight());
			app->screen->box(5, rowY, app->config->resolutionX() - 16, 1, 255, 255, 255, 130);
			app->screen->box(5, rowY + 1, app->config->resolutionX() - 16, 1, 0, 0, 0, 130);
		} else {
			rowY = app->listRect.y + (i - firstRow) * app->font->getHeight();
			app->font->write(app->screen, text->at(i), 5, rowY);
		}
	}

	app->screen->clearClipRect();
	app->ui->drawScrollBar(rowsPerPage, text->size(), firstRow, app->listRect);
}

void TextDialog::exec() {
	if ((*app->sc)[backdrop] != NULL) (*app->sc)[backdrop]->blit(this->bg,0,0);

	preProcess();

	bool close = false, inputAction = false;

	if (this->icon.empty())
		this->icon = "skin:icons/ebook.png";
	drawTopBar(this->bg, this->title, this->description, this->icon);

	drawBottomBar(this->bg);

	app->ui->drawButton(
		this->bg, 
		"down", 
		app->tr["Scroll"],
		app->ui->drawButton(
			this->bg, 
			"up", 
			"",
			app->ui->drawButton(
				this->bg, 
				"start", 
				app->tr["Exit"],
				5)
			)
		-10
	);

	this->bg->box(app->listRect, app->skin->colours.listBackground);

	uint32_t firstRow = 0, rowsPerPage = app->listRect.h/app->font->getHeight();
	while (!close) {
		this->bg->blit(app->screen,0,0);
		drawText(&text, firstRow, rowsPerPage);
		app->screen->flip();

		do {
			inputAction = app->input.update();
			
			if ( app->input[UP  ] && firstRow > 0 ) firstRow--;
			else if ( app->input[DOWN] && firstRow + rowsPerPage < text.size() ) firstRow++;
			else if ( app->input[PAGEUP] || app->input[LEFT]) {
				if (firstRow >= rowsPerPage - 1)
					firstRow -= rowsPerPage - 1;
				else
					firstRow = 0;
			}
			else if ( app->input[PAGEDOWN] || app->input[RIGHT]) {
				if (firstRow + rowsPerPage * 2 - 1 < text.size())
					firstRow += rowsPerPage - 1;
				else
					firstRow = max(0,text.size()-rowsPerPage);
			}
			else if ( app->input[SETTINGS] || app->input[CANCEL] ) close = true;
		} while (!inputAction);
	}
}

void TextDialog::appendText(const string &text) {
	this->rawText += text;
}

void TextDialog::appendFile(const string &file) {
	if (fileExists(file)) {
		ifstream t(file);
		stringstream buf;
		buf << t.rdbuf();
		this->rawText += buf.str();
	}
}

void TextDialog::appendCommand(const string &executable, const string &args) {
	TRACE("enter : running %s %s", executable.c_str(), args.c_str());
	if (fileExists(executable)) {
		TRACE("executable exists");
		char buffer[128];
		std::string result = "";
		std::string final = executable;
		if (args.length() > 0) {
			final += " " + args;
		}
		FILE* pipe = popen(final.c_str(), "r");
		if (!pipe) throw std::runtime_error("popen() failed!");
		try {
			while (fgets(buffer, sizeof buffer, pipe) != NULL) {
				result += buffer;
			}
		} catch (...) {
			pclose(pipe);
			throw;
		}
		pclose(pipe);
		this->rawText += result;
	}
	TRACE("exit");
}
