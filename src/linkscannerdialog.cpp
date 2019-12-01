#include <vector>
#include <algorithm>
#include <math.h>

#include "linkscannerdialog.h"
#include "debug.h"
#include "constants.h"

LinkScannerDialog::LinkScannerDialog(GMenu2X *gmenu2x, const string &title, const string &description, const string &icon)
	: Dialog(gmenu2x)
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
	this->maxMessagesOnScreen = floor((gmenu2x->listRect.h - gmenu2x->skin->menuInfoBarHeight) / gmenu2x->font->getHeight());

	string str = "";
	stringstream ss;
	vector<string> files;

	drawTopBar(this->bg, title, description, icon);
	drawBottomBar(this->bg);

	this->bg->box(gmenu2x->listRect, gmenu2x->skin->colours.listBackground);
	//gmenu2x->ui->drawButton(this->bg, "start", gmenu2x->tr["Exit"]);
	this->bg->blit(gmenu2x->screen, 0, 0);

	this->notify("Scanning...");
	this->notify("Updating application cache");
	int preSize = gmenu2x->cacheSize();
	ss.clear();
	ss << preSize;
	ss >> str;
	this->notify("Current cache size : " + str);
	gmenu2x->updateAppCache(std::bind( &LinkScannerDialog::notify, this, std::placeholders::_1) );
	int postSize = gmenu2x->cacheSize();
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
				if (gmenu2x->menu->addLink(path, file, "linkscanner")) {
					++this->foundFiles;
				}
			}
		}

		ss.clear();
		ss << this->foundFiles;
		ss >> str;
		this->notify(str + " links created");
	}

	if (this->foundFiles > 0 || cacheChanged) {
		sync();
		this->notify("Updating the menu");
		gmenu2x->initMenu();
	}
	this->finished_ = true;
	this->notify("Completed scan");

	while (!close) {
		bool inputAction = gmenu2x->input.update(true);
		if ( gmenu2x->input[SETTINGS] || gmenu2x->input[CANCEL] ) {
			close = true;
		} else if (gmenu2x->input[UP]) {
			--this->selectedItem;
			if (this->selectedItem < maxMessagesOnScreen) {
				this->selectedItem = this->messages_.size() -1;
			}
			this->render();
		} else if (gmenu2x->input[DOWN]) {
			++this->selectedItem;
			if (this->selectedItem >= this->messages_.size()) {
				this->selectedItem = maxMessagesOnScreen;
			}
			this->render();
		} else if (gmenu2x->input[SECTION_PREV]  || gmenu2x->input[LEFT]) {
			this->selectedItem = maxMessagesOnScreen;
			this->render();
		} else if (gmenu2x->input[SECTION_NEXT] || gmenu2x->input[RIGHT]) {
			this->selectedItem = this->messages_.size() -1;
			this->render();
		}
	}
}

void LinkScannerDialog::render() {

	int lineY = gmenu2x->listRect.y;

	this->bg->box(gmenu2x->listRect, gmenu2x->skin->colours.listBackground);
	if (this->finished_) {
		gmenu2x->ui->drawButton(this->bg, "start", gmenu2x->tr["Exit"]);
	}
	this->bg->blit(gmenu2x->screen, 0, 0);

	vector<string>::iterator it = this->messages_.begin();
	int numMessages = (int)this->messages_.size();
	
	if (numMessages > this->maxMessagesOnScreen) {
		if (this->selectedItem > this->maxMessagesOnScreen) {
			it += (this->selectedItem - maxMessagesOnScreen);
		}

		gmenu2x->ui->drawScrollBar(
			this->maxMessagesOnScreen, 
			numMessages, 
			this->selectedItem - this->maxMessagesOnScreen, 
			gmenu2x->listRect);
	}

	for (int counter = 0; it != this->messages_.end(); it++) {
		gmenu2x->screen->write(
			gmenu2x->font, 
			(*it), 
			gmenu2x->listRect.x + 4, 
			lineY);

		lineY += gmenu2x->font->getHeight();
		counter++;
		if (counter > this->maxMessagesOnScreen)
			break;
	}
	gmenu2x->screen->flip();
}

void LinkScannerDialog::notify(string message) {
	this->gmenu2x->screenManager.resetScreenTimer();
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
