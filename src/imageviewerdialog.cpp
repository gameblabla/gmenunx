#include "imageviewerdialog.h"
#include "debug.h"

ImageViewerDialog::ImageViewerDialog(Esoteric *app, const string &title, const string &description, const string &icon, const string &path)
: Dialog(app), title(title), description(description), icon(icon), path(path)
{}

void ImageViewerDialog::exec() {
	Surface image(path);

	bool close = false, inputAction = false;
	int offsetX = 0, offsetY = 0;

	drawTopBar(this->bg, title, description, icon);
	drawBottomBar(this->bg);

	app->ui->drawButton(this->bg, "right", app->tr["Pan"],
	app->ui->drawButton(this->bg, "down", "",
	app->ui->drawButton(this->bg, "up", "",
	app->ui->drawButton(this->bg, "left", "",
	app->ui->drawButton(this->bg, "start", app->tr["Exit"],
	5))-12)-14)-12);

	this->bg->blit(app->screen,0,0);

	while (!close) {
			this->bg->blit(app->screen, 0, 0);
			app->screen->setClipRect(app->listRect);
			image.blit(app->screen, app->listRect.x + offsetX, app->listRect.y + offsetY);
			app->screen->flip();
			app->screen->clearClipRect();

		do {
			inputAction = app->inputManager->update();

			if ( (*app->inputManager)[MANUAL] || (*app->inputManager)[CANCEL] || (*app->inputManager)[SETTINGS] ) close = true;
			else if ( (*app->inputManager)[LEFT] && offsetX < 0) {
				offsetX += app->listRect.w/3;
				if (offsetX > 0) offsetX = 0;
			}
			else if ( (*app->inputManager)[RIGHT] && image.raw->w + offsetX > app->listRect.w) {
				offsetX -=  app->listRect.w/3;
				if (image.raw->w + offsetX < app->listRect.w) offsetX = app->listRect.w - image.raw->w;
			}
			else if ( (*app->inputManager)[UP] && offsetY < 0) {
				offsetY +=  app->listRect.h/3;
				if (offsetY > 0) offsetY = 0;
			}
			else if ( (*app->inputManager)[DOWN] && image.raw->w + offsetY > app->listRect.h) {
				offsetY -=  app->listRect.h/3;
				if (image.raw->h + offsetY < app->listRect.h) offsetY = app->listRect.h - image.raw->h;
			}
		} while (!inputAction);
	}
}
