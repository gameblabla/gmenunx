#include "browsedialog.h"
#include "FastDelegate.h"
#include "debug.h"
#include "constants.h"

BrowseDialog::BrowseDialog(Esoteric *app, const std::string &title, const std::string &description, const std::string &icon)
: Dialog(app), title(title), description(description), icon(icon) {
	
	TRACE("enter");
	std::string startPath = USER_HOME;
	if (FileUtils::dirExists(app->config->launcherPath())) {
		startPath = app->config->launcherPath();
	} else if (FileUtils::dirExists(EXTERNAL_CARD_PATH)) {
		startPath = EXTERNAL_CARD_PATH;
	}
	fl = new FileLister(startPath, true, false);
}

BrowseDialog::~BrowseDialog() {
	delete this->fl;
}

bool BrowseDialog::exec() {
	TRACE("enter");
	if (!this->fl) return false;

	this->bg = new Surface(this->app->bg); // needed to redraw on child screen return

	Surface *iconGoUp = this->app->sc->skinRes("imgs/go-up.png");
	Surface *iconFolder = this->app->sc->skinRes("imgs/folder.png");
	Surface *iconFile = this->app->sc->skinRes("imgs/file.png");

	std::string path = this->fl->getPath();
	if (path.empty() || !FileUtils::dirExists(path)) {
		setPath(EXTERNAL_CARD_PATH);
	}

	this->fl->browse();

	this->selected = 0;
	this->close = false;
	bool inputAction = false;

	uint32_t i, iY, firstElement = 0, animation = 0, padding = 6;
	uint32_t rowHeight = this->app->font->getHeight() + 1;
	uint32_t numRows = (this->app->listRect.h - 2) / rowHeight - 1;

	this->drawTopBar(this->bg, this->title, this->description, this->icon);
	this->drawBottomBar(this->bg);
	this->bg->box(this->app->listRect, this->app->skin->colours.listBackground);

	if (!this->showFiles && this->allowSelectDirectory) {
		this->app->ui->drawButton(this->bg, "start", app->tr["Select"]);
	} else {
		this->app->ui->drawButton(this->bg, "start", app->tr["Exit"],
			this->app->ui->drawButton(this->bg, "x", app->tr["Hidden"], 
			this->app->ui->drawButton(this->bg, "b", app->tr["Up"], 
			this->app->ui->drawButton(this->bg, "a", app->tr["Select"])))
		);
	}
	
	uint32_t tickStart = SDL_GetTicks();
	while (!this->close) {
		this->bg->blit(this->app->screen,0,0);
		// buttonBox.paint(5);

		//Selection
		if (this->selected >= firstElement + numRows) firstElement = this->selected - numRows;
		if (this->selected < firstElement) firstElement = this->selected;

		//Files & Directories
		iY = this->app->listRect.y + 1;
		for (i = firstElement; i < this->fl->size() && i <= firstElement + numRows; i++, iY += rowHeight) {
			if (i == this->selected) {
				this->app->screen->box(
					this->app->listRect.x, 
					iY, 
					this->app->listRect.w, 
					rowHeight, 
					this->app->skin->colours.selectionBackground);
			}

			if (this->fl->isDirectory(i)) {
				if ((*this->fl)[i] == "..")
					iconGoUp->blit(this->app->screen, this->app->listRect.x + 10, iY + rowHeight / 2, HAlignCenter | VAlignMiddle);
				else
					iconFolder->blit(this->app->screen, this->app->listRect.x + 10, iY + rowHeight / 2, HAlignCenter | VAlignMiddle);
			} else {
				iconFile->blit(this->app->screen, this->app->listRect.x + 10, iY + rowHeight / 2, HAlignCenter | VAlignMiddle);
			}
			this->app->screen->write(this->app->font, (*this->fl)[i], this->app->listRect.x + 21, iY + rowHeight / 2, VAlignMiddle);
		}

		// preview
		std::string ext = this->getExtensionToLower();

		if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif") {

			std::string filename = this->fl->getPath() + "/" + getFile();

			app->screen->box(
				320 - animation, 
				app->listRect.y, 
				app->skin->previewWidth, 
				app->listRect.h, 
				app->skin->colours.titleBarBackground);

			(*app->sc)[filename]->softStretch(
				app->skin->previewWidth - 2 * padding, 
				app->listRect.h - 2 * padding, 
				true, 
				false);

			(*app->sc)[filename]->blit(
				app->screen, 
				{ 
					320 - animation + padding, 
					app->listRect.y + padding, 
					app->skin->previewWidth - 2 * padding, 
					app->listRect.h - 2 * padding 
				}, 
				HAlignCenter | VAlignMiddle, 
				240);

			if (animation < app->skin->previewWidth) {
				animation = UI::intTransition(0, app->skin->previewWidth, tickStart, 110);
				app->screen->flip();
				continue;
			}
		} else {
			if (animation > 0) {
				app->screen->box(
					320 - animation, 
					app->listRect.y, 
					app->skin->previewWidth, 
					app->listRect.h, 
					app->skin->colours.titleBarBackground
				);
				animation = app->skin->previewWidth - UI::intTransition(0, app->skin->previewWidth, tickStart, 80);
				app->screen->flip();
				continue;
			}
		}
		
		this->app->ui->drawScrollBar(numRows, fl->size(), firstElement, app->listRect);
		this->app->screen->flip();

		do {
			inputAction = this->app->inputManager->update();
			if (inputAction) tickStart = SDL_GetTicks();

			uint32_t action = this->getAction();

		// if (action == BD_ACTION_SELECT && (*fl)[selected] == "..")
			// action = BD_ACTION_GOUP;
			switch (action) {
				case BD_ACTION_CANCEL:
					this->cancel();
					break;
				case BD_ACTION_CLOSE:
					if (this->allowSelectDirectory && this->fl->isDirectory(selected)) 
						this->confirm();
					else this->cancel();
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
				case BD_ACTION_TOGGLE_HIDDEN:
					this->toggleHidden();
					break;
				case BD_ACTION_PAGEDOWN:
					selected += numRows;
					if (selected >= fl->size()) selected = fl->size() - 1;
					break;
				case BD_ACTION_GOUP:
					this->directoryUp();
					break;
				case BD_ACTION_SELECT:
					if (this->fl->isDirectory(selected)) {
						this->directoryEnter();
						break;
					}
				// fall-through
				case BD_ACTION_CONFIRM:
					this->confirm();
					break;
				default:
					break;
			}
		} while (!inputAction);
	};
	return result;
}

uint32_t BrowseDialog::getAction() {
	TRACE("enter");
	uint32_t action = BD_NO_ACTION;
	if ((*app->inputManager)[SETTINGS]) action = BD_ACTION_CLOSE;
	else if ((*app->inputManager)[UP]) action = BD_ACTION_UP;
	else if ((*app->inputManager)[PAGEUP] || (*app->inputManager)[LEFT]) action = BD_ACTION_PAGEUP;
	else if ((*app->inputManager)[DOWN]) action = BD_ACTION_DOWN;
	else if ((*app->inputManager)[PAGEDOWN] || (*app->inputManager)[RIGHT]) action = BD_ACTION_PAGEDOWN;
	else if ((*app->inputManager)[CANCEL]) action = BD_ACTION_GOUP;
	else if ((*app->inputManager)[CONFIRM]) action = BD_ACTION_SELECT;
	else if ((*app->inputManager)[CANCEL] || (*app->inputManager)[MENU]) action = BD_ACTION_CANCEL;
	else if ((*app->inputManager)[DEC]) action = BD_ACTION_TOGGLE_HIDDEN;
	return action;
}

void BrowseDialog::directoryUp() {
	std::string path = this->fl->getPath();
	std::string::size_type p = path.rfind("/");
	if (p == path.size() - 1) p = path.rfind("/", p - 1);
	this->selected = 0;
	this->setPath("/" + path.substr(0, p));
}

void BrowseDialog::toggleHidden() {
	this->fl->toggleHidden();
	this->fl->browse();
}

void BrowseDialog::directoryEnter() {
	std::string path = fl->getPath();
	this->setPath(path + "/" + this->fl->at(this->selected));
	this->selected = 0;
}

void BrowseDialog::confirm() {
	this->result = true;
	this->close = true;
}

void BrowseDialog::cancel() {
	this->result = false;
	this->close = true;
}

const std::string BrowseDialog::getExtensionToLower() {
	return FileUtils::fileExtension((*this->fl)[this->selected]);
}

void BrowseDialog::setPath(const std::string &path) {
	this->fl->showDirectories = showDirectories;
	this->fl->showFiles = showFiles;
	this->fl->setPath(path, true);
	this->onChangeDir();
}
const std::string &BrowseDialog::getPath() {
	return this->fl->getPath();
}
std::string BrowseDialog::getFile() {
	return (*this->fl)[selected];
}
void BrowseDialog::setFilter(const std::string &filter) {
	this->fl->setFilter(filter);
}
