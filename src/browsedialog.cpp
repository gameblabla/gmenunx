#include "browsedialog.h"
#include "FastDelegate.h"
#include "debug.h"
#include "constants.h"

#include <algorithm>

using namespace fastdelegate;
using namespace std;

BrowseDialog::BrowseDialog(Esoteric *app, const string &title, const string &description, const string &icon)
: Dialog(app), title(title), description(description), icon(icon) {
	
	TRACE("enter");
	string startPath = USER_HOME;
	if (dirExists(app->config->launcherPath())) {
		startPath = app->config->launcherPath();
	} else if (dirExists(EXTERNAL_CARD_PATH)) {
		startPath = EXTERNAL_CARD_PATH;
	}
	fl = new FileLister(startPath, true, false);
}

BrowseDialog::~BrowseDialog() {
	delete fl;
}

bool BrowseDialog::exec() {
	TRACE("enter");
	if (!fl) return false;

	this->bg = new Surface(app->bg); // needed to redraw on child screen return

	Surface *iconGoUp = app->sc->skinRes("imgs/go-up.png");
	Surface *iconFolder = app->sc->skinRes("imgs/folder.png");
	Surface *iconFile = app->sc->skinRes("imgs/file.png");

	string path = fl->getPath();
	if (path.empty() || !dirExists(path)) {
		setPath(EXTERNAL_CARD_PATH);
	}

	fl->browse();

	selected = 0;
	close = false;
	bool inputAction = false;

	uint32_t i, iY, firstElement = 0, animation = 0, padding = 6;
	uint32_t rowHeight = app->font->getHeight() + 1;
	uint32_t numRows = (app->listRect.h - 2)/rowHeight - 1;

	drawTopBar(this->bg, title, description, icon);
	drawBottomBar(this->bg);
	this->bg->box(app->listRect, app->skin->colours.listBackground);

	if (!showFiles && allowSelectDirectory) {
		app->ui->drawButton(this->bg, "start", app->tr["Select"]);
	} else {
		app->ui->drawButton(
			this->bg, "start", app->tr["Exit"],
			app->ui->drawButton(this->bg, "b", app->tr["Up"], 
			app->ui->drawButton(this->bg, "a", app->tr["Select"]))
		);
	}

	uint32_t tickStart = SDL_GetTicks();
	while (!close) {
		this->bg->blit(app->screen,0,0);
		// buttonBox.paint(5);

		//Selection
		if (selected >= firstElement + numRows) firstElement = selected - numRows;
		if (selected < firstElement) firstElement = selected;

		//Files & Directories
		iY = app->listRect.y + 1;
		for (i = firstElement; i < fl->size() && i <= firstElement + numRows; i++, iY += rowHeight) {
			if (i == selected) app->screen->box(app->listRect.x, iY, app->listRect.w, rowHeight, app->skin->colours.selectionBackground);
			if (fl->isDirectory(i)) {
				if ((*fl)[i] == "..")
					iconGoUp->blit(app->screen, app->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);
				else
					iconFolder->blit(app->screen, app->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);
			} else {
				iconFile->blit(app->screen, app->listRect.x + 10, iY + rowHeight/2, HAlignCenter | VAlignMiddle);
			}
			app->screen->write(app->font, (*fl)[i], app->listRect.x + 21, iY + rowHeight/2, VAlignMiddle);
		}

		// preview
		string filename = fl->getPath() + "/" + getFile();
		string ext = getExt();

		if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif") {
			app->screen->box(320 - animation, app->listRect.y, app->skin->previewWidth, app->listRect.h, app->skin->colours.titleBarBackground);

			(*app->sc)[filename]->softStretch(app->skin->previewWidth - 2 * padding, app->listRect.h - 2 * padding, true, false);
			(*app->sc)[filename]->blit(app->screen, {320 - animation + padding, app->listRect.y + padding, app->skin->previewWidth - 2 * padding, app->listRect.h - 2 * padding}, HAlignCenter | VAlignMiddle, 240);

			if (animation < app->skin->previewWidth) {
				animation = intTransition(0, app->skin->previewWidth, tickStart, 110);
				app->screen->flip();
				app->input.setWakeUpInterval(45);
				continue;
			}
		} else {
			if (animation > 0) {
				app->screen->box(320 - animation, app->listRect.y, app->skin->previewWidth, app->listRect.h, app->skin->colours.titleBarBackground);
				animation = app->skin->previewWidth - intTransition(0, app->skin->previewWidth, tickStart, 80);
				app->screen->flip();
				app->input.setWakeUpInterval(45);
				continue;
			}
		}
		app->input.setWakeUpInterval(1000);
		app->ui->drawScrollBar(numRows, fl->size(), firstElement, app->listRect);
		app->screen->flip();

		do {
			inputAction = app->input.update();
			if (inputAction) tickStart = SDL_GetTicks();

			uint32_t action = getAction();

		// if (action == BD_ACTION_SELECT && (*fl)[selected] == "..")
			// action = BD_ACTION_GOUP;
			switch (action) {
				case BD_ACTION_CANCEL:
					cancel();
					break;
				case BD_ACTION_CLOSE:
					if (allowSelectDirectory && fl->isDirectory(selected)) confirm();
					else cancel();
					break;
				case BD_ACTION_UP:
					selected -= 1;
					if (selected < 0) selected = fl->size() - 1;
					break;
				case BD_ACTION_DOWN:
					selected += 1;
					if (selected >= fl->size()) selected = 0;
					break;
				case BD_ACTION_PAGEUP:
					selected -= numRows;
					if (selected < 0) selected = 0;
					break;
				case BD_ACTION_PAGEDOWN:
					selected += numRows;
					if (selected >= fl->size()) selected = fl->size() - 1;
					break;
				case BD_ACTION_GOUP:
					directoryUp();
					break;
				case BD_ACTION_SELECT:
					if (fl->isDirectory(selected)) {
						directoryEnter();
						break;
					}
			/* Falltrough */
				case BD_ACTION_CONFIRM:
					confirm();
					break;
				default:
					break;
			}
		} while (!inputAction);
	}
	return result;
}

uint32_t BrowseDialog::getAction() {
	TRACE("enter");
	uint32_t action = BD_NO_ACTION;

	if (app->input[SETTINGS]) action = BD_ACTION_CLOSE;
	else if (app->input[UP]) action = BD_ACTION_UP;
	else if (app->input[PAGEUP] || app->input[LEFT]) action = BD_ACTION_PAGEUP;
	else if (app->input[DOWN]) action = BD_ACTION_DOWN;
	else if (app->input[PAGEDOWN] || app->input[RIGHT]) action = BD_ACTION_PAGEDOWN;
	else if (app->input[CANCEL]) action = BD_ACTION_GOUP;
	else if (app->input[CONFIRM]) action = BD_ACTION_SELECT;
	else if (app->input[CANCEL] || app->input[MENU]) action = BD_ACTION_CANCEL;
	return action;
}

void BrowseDialog::directoryUp() {
	string path = fl->getPath();
	string::size_type p = path.rfind("/");
	if (p == path.size() - 1) p = path.rfind("/", p - 1);
	selected = 0;
	setPath("/" + path.substr(0, p));
}

void BrowseDialog::directoryEnter() {
	string path = fl->getPath();
	setPath(path + "/" + fl->at(selected));
	selected = 0;
}

void BrowseDialog::confirm() {
	result = true;
	close = true;
}

void BrowseDialog::cancel() {
	result = false;
	close = true;
}

const std::string BrowseDialog::getExt() {
	string filename = (*fl)[selected];
	string ext = "";
	string::size_type pos = filename.rfind(".");
	if (pos != string::npos && pos > 0) {
		ext = filename.substr(pos, filename.length());
		transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int)) tolower);
	}
	return ext;
}

void BrowseDialog::setPath(const string &path) {
	fl->showDirectories = showDirectories;
	fl->showFiles = showFiles;
	fl->setPath(path, true);
	onChangeDir();
}

const std::string &BrowseDialog::getPath() {
	return fl->getPath();
}
std::string BrowseDialog::getFile() {
	return (*fl)[selected];
}
void BrowseDialog::setFilter(const string &filter) {
	fl->setFilter(filter);
}
