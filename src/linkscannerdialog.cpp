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

void LinkScannerDialog::exec() {
	bool close = false;
	this->lineY = gmenu2x->listRect.y;
	this->foundFiles = 0;
	string str = "";
	stringstream ss;
	vector<string> files;

	drawTopBar(this->bg, title, description, icon);
	drawBottomBar(this->bg);
	this->bg->box(gmenu2x->listRect, gmenu2x->skin->colours.listBackground);

	gmenu2x->ui->drawButton(this->bg, "start", gmenu2x->tr["Exit"]);
	this->bg->blit(gmenu2x->screen, 0, 0);
	gmenu2x->screen->write(gmenu2x->font, gmenu2x->tr["Scanning..."], gmenu2x->listRect.x + 4, this->lineY);

	lineY += gmenu2x->font->getHeight();
	gmenu2x->screen->write(gmenu2x->font, gmenu2x->tr["Updating app cache"], gmenu2x->listRect.x + 4, this->lineY);
	gmenu2x->screen->flip();
	gmenu2x->updateAppCache(std::bind( &LinkScannerDialog::foundFile, this, std::placeholders::_1) );

	this->lineY += gmenu2x->font->getHeight();
	gmenu2x->screen->write(gmenu2x->font, EXTERNAL_CARD_PATH, gmenu2x->listRect.x + 4, this->lineY);
	gmenu2x->screen->flip();

	scanPath(EXTERNAL_CARD_PATH, &files);
	ss << this->foundFiles;
	ss >> str;

	this->lineY += gmenu2x->font->getHeight();
	gmenu2x->screen->write(gmenu2x->font, gmenu2x->tr.translate("$1 files found.", str.c_str(), NULL), gmenu2x->listRect.x + 4, this->lineY);
	gmenu2x->screen->flip();

	if (files.size() > 0) {

		lineY += gmenu2x->font->getHeight();
		gmenu2x->screen->write(gmenu2x->font, gmenu2x->tr["Creating links..."], gmenu2x->listRect.x + 4, lineY);
		gmenu2x->screen->flip();

		string path, file;
		string::size_type pos;

		for (size_t i = 0; i < files.size(); ++i) {
			pos = files[i].rfind("/");

			if (pos != string::npos && pos > 0) {
				path = files[i].substr(0, pos + 1);
				file = files[i].substr(pos + 1, files[i].length());
				if (gmenu2x->menu->addLink(path, file, "linkscanner"))
					++this->foundFiles;
			}
		}

		ss.clear();
		ss << this->foundFiles;
		ss >> str;

		this->lineY += gmenu2x->font->getHeight();
		gmenu2x->screen->write(gmenu2x->font, gmenu2x->tr.translate("$1 links created.", str.c_str(), NULL), gmenu2x->listRect.x + 4, lineY);
		gmenu2x->screen->flip();
	}

	if (this->foundFiles > 0) {
		sync();
		gmenu2x->initMenu();
	}

	while (!close) {
		bool inputAction = gmenu2x->input.update();
		if ( gmenu2x->input[SETTINGS] || gmenu2x->input[CANCEL] ) 
			close = true;
	}

}

void LinkScannerDialog::foundFile(string file) {
	++this->foundFiles;
	this->lineY += gmenu2x->font->getHeight();
	gmenu2x->screen->write(gmenu2x->font, file, gmenu2x->listRect.x + 4, this->lineY);
	gmenu2x->screen->flip();
}

void LinkScannerDialog::scanPath(string path, vector<string> *files) {
	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath, ext;

	if (path[path.length()-1]!='/') path += "/";
	if ((dirp = opendir(path.c_str())) == NULL) return;

	while ((dptr = readdir(dirp))) {
		if (dptr->d_name[0] == '.')
			continue;
		filepath = path + dptr->d_name;
		int statRet = stat(filepath.c_str(), &st);
		if (S_ISDIR(st.st_mode))
			scanPath(filepath, files);

		if (statRet != -1) {
			ext = fileExtension(filepath);
			if (ext.empty()) 
				continue;
			if (ext == "dge" || ext == "gpu" || ext == "gpe" || ext == "sh") {
				foundFile(filepath);
				files->push_back(filepath);
			}
		}
	}
	closedir(dirp);
}
