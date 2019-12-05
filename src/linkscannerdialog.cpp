#include <vector>
#include <algorithm>
#include <math.h>

#include "linkscannerdialog.h"
#include "debug.h"
#include "constants.h"

LinkScannerDialog::LinkScannerDialog(Esoteric *app, const string &title, const string &description, const string &icon)
	: Dialog(app)
{
	this->title = title;
	this->description = description;
	this->icon = icon;

}

LinkScannerDialog::~LinkScannerDialog() {
	TRACE("enter");
	this->quit();
}

void LinkScannerDialog::quit() {
    TRACE("enter");
}

void LinkScannerDialog::exec() {
	bool close = false;
	bool cacheChanged = false;
	this->finished_ = false;
	this->foundFiles = 0;
	this->maxMessagesOnScreen = floor((app->listRect.h - app->skin->menuInfoBarHeight) / app->font->getHeight());

	string str = "";
	stringstream ss;
	vector<string> files;

	drawTopBar(this->bg, title, description, icon);
	drawBottomBar(this->bg);

	this->bg->box(app->listRect, app->skin->colours.listBackground);
	//app->ui->drawButton(this->bg, "start", app->tr["Exit"]);
	this->bg->blit(app->screen, 0, 0);

	this->notify("Scanning...");
	this->notify("Updating application cache");
	int preSize = app->cache->size();
	ss.clear();
	ss << preSize;
	ss >> str;
	this->notify("Current cache size : " + str);

	app->updateAppCache([&](std::string message){ return this->notify(message); });

	int postSize = app->cache->size();
	ss.clear();
	ss << postSize;
	ss >> str;
	this->notify("Updated cache size : " + str);
	int numFound = (postSize - preSize);
	cacheChanged = numFound > 0;
	ss.clear();
	ss << numFound;
	ss >> str;
	this->notify("Added " + str + " files to the cache");
	
	this->notify("Finished updating the application cache");
	this->notify("Scanning for binaries under : " + EXTERNAL_CARD_PATH);

	// do the binary scan
	scanPath(EXTERNAL_CARD_PATH, &files);
	ss.clear();
	ss << files.size();
	ss >> str;
	this->notify(str + " files found");

	if (files.size() > 0) {

		this->notify("Creating links...");

		string path, file;
		string::size_type pos;

		this->foundFiles = 0;
		for (size_t i = 0; i < files.size(); ++i) {
			pos = files[i].rfind("/");

			if (pos != string::npos && pos > 0) {
				path = files[i].substr(0, pos + 1);
				file = files[i].substr(pos + 1, files[i].length());
				if (app->menu->addLink(path, file, "linkscanner")) {
					++this->foundFiles;
				}
			}
		}
		ss.clear();
		ss << this->foundFiles;
		ss >> str;
		this->notify(str + " links created");
	}

	if (this->foundFiles > 0 || app->cache->isDirty()) {
		sync();
		this->notify("Updating the menu");
		app->initMenu();
	}
	this->finished_ = true;
	this->notify("Completed scan");

	while (!close) {
		bool inputAction = app->input.update(true);
		if ( app->input[SETTINGS] || app->input[CANCEL] ) {
			close = true;
		} else if (app->input[UP]) {
			--this->selectedItem;
			if (this->selectedItem < maxMessagesOnScreen) {
				this->selectedItem = this->messages_.size() -1;
			}
			this->render();
		} else if (app->input[DOWN]) {
			++this->selectedItem;
			if (this->selectedItem >= this->messages_.size()) {
				this->selectedItem = maxMessagesOnScreen;
			}
			this->render();
		} else if (app->input[SECTION_PREV]  || app->input[LEFT]) {
			this->selectedItem = maxMessagesOnScreen;
			this->render();
		} else if (app->input[SECTION_NEXT] || app->input[RIGHT]) {
			this->selectedItem = this->messages_.size() -1;
			this->render();
		}
	}
}

void LinkScannerDialog::render() {

	int lineY = app->listRect.y;

	this->bg->box(app->listRect, app->skin->colours.listBackground);
	if (this->finished_) {
		app->ui->drawButton(this->bg, "start", app->tr["Exit"]);
	}
	this->bg->blit(app->screen, 0, 0);

	vector<string>::iterator it = this->messages_.begin();
	int numMessages = (int)this->messages_.size();
	
	if (numMessages > this->maxMessagesOnScreen) {
		if (this->selectedItem > this->maxMessagesOnScreen) {
			it += (this->selectedItem - maxMessagesOnScreen);
		}

		app->ui->drawScrollBar(
			this->maxMessagesOnScreen, 
			numMessages, 
			this->selectedItem - this->maxMessagesOnScreen, 
			app->listRect);
	}

	for (int counter = 0; it != this->messages_.end(); it++) {
		app->screen->write(
			app->font, 
			(*it), 
			app->listRect.x + 4, 
			lineY);

		lineY += app->font->getHeight();
		counter++;
		if (counter > this->maxMessagesOnScreen)
			break;
	}
	app->screen->flip();
}

void LinkScannerDialog::notify(string message) {
	this->app->screenManager.resetScreenTimer();
	this->messages_.push_back(message);
	this->selectedItem = this->messages_.size() -1;
	this->render();
}

void LinkScannerDialog::scanPath(string path, vector<string> *files) {
	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	std::string filepath, ext;

	if (path[path.length() - 1] != '/') path += "/";
	if ((dirp = opendir(path.c_str())) == NULL) return;

	while ((dptr = readdir(dirp))) {
		if (dptr->d_name[0] == '.')
			continue;
		
		std::string dirItem = dptr->d_name;
		filepath = path + dirItem;
		int statRet = stat(filepath.c_str(), &st);
		if (S_ISDIR(st.st_mode)) {
			this->notify("Scanning : " + dirItem);
			scanPath(filepath, files);
		} else if (statRet != -1) {
			ext = fileExtension(dirItem);
			//TRACE("got file extension %s for file : %s", ext.c_str(), dirItem.c_str());
			if (ext.empty()) 
				continue;
			if (ext == ".dge" || ext == ".gpu" || ext == ".gpe" || ext == ".sh" || ext == ".exe") {
				TRACE("found executable file : %s", filepath.c_str());
				this->notify("Found file : " + dirItem);
				files->push_back(filepath);
				this->foundFiles++;
			}
		}
	}
	closedir(dirp);
}
