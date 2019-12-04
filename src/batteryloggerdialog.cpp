#include "batteryloggerdialog.h"

BatteryLoggerDialog::BatteryLoggerDialog(Esoteric *app, const string &title, const string &description, const string &icon)
	: Dialog(app)
{
	this->title = title;
	this->description = description;
	this->icon = icon;
}

char *BatteryLoggerDialog::ms2hms(uint32_t t, bool mm, bool ss) {
	static char buf[10];

	t = t / 1000;
	int s = (t % 60);
	int m = (t % 3600) / 60;
	int h = (t % 86400) / 3600;

	if (!ss) sprintf(buf, "%02d:%02d", h, m);
	else if (!mm) sprintf(buf, "%02d", h);
	else sprintf(buf, "%02d:%02d:%02d", h, m, s);
	return buf;
}

void BatteryLoggerDialog::exec() {
	bool close = false;

	drawTopBar(this->bg, title, description, icon);

	this->bg->box(app->listRect, app->skin->colours.listBackground);

	drawBottomBar(this->bg);
	app->ui->drawButton(this->bg, "start", app->tr["Exit"],
	app->ui->drawButton(this->bg, "select", app->tr["Del battery.csv"],
	app->ui->drawButton(this->bg, "down", app->tr["Scroll"],
	app->ui->drawButton(this->bg, "up", "", 5)-10)));

	this->bg->blit(app->screen,0,0);
	app->hw->setBacklightLevel(100);
	app->screen->flip();

	MessageBox mb(
		app, 
		app->tr["Welcome to the Battery Logger.\nMake sure the battery is fully charged.\nAfter pressing OK, leave the device ON until\nthe battery has been fully discharged.\nThe log will be saved in 'battery.csv'."]);
	mb.exec();

	uint32_t rowsPerPage = app->listRect.h/app->font->getHeight();

	int32_t firstRow = 0, tickNow = 0, tickStart = SDL_GetTicks(), tickBatteryLogger = -1000000;
	string logfile = app->getWriteablePath() + "battery.csv";

	char buf[100];
	sprintf(buf, "echo '----' >> %s/battery.csv; sync", cmdclean(app->getWriteablePath()).c_str());
	system(buf);

	if (!fileExists(logfile)) return;

	ifstream inf(logfile.c_str(), ios_base::in);
	if (!inf.is_open()) return;
	vector<string> log;

	string line;
	while (getline(inf, line, '\n'))
		log.push_back(line);
	inf.close();

	while (!close) {
		tickNow = SDL_GetTicks();
		if ((tickNow - tickBatteryLogger) >= 60000) {
			tickBatteryLogger = tickNow;

			sprintf(
				buf, 
				"echo '%s,%d' >> %s/battery.csv; sync", 
				this->ms2hms(tickNow - tickStart, true, false), 
				app->hw->getBatteryLevel(), 
				cmdclean(app->getWriteablePath()).c_str());

			system(buf);

			ifstream inf(logfile.c_str(), ios_base::in);
			log.clear();

			while (getline(inf, line, '\n'))
				log.push_back(line);
			inf.close();
		}

		this->bg->blit(app->screen,0,0);

		for (uint32_t i = firstRow; i < firstRow + rowsPerPage && i < log.size(); i++) {
			int rowY, j = log.size() - i - 1;
			if (log.at(j) == "----") { // draw a line
				rowY = 42 + (int)((i - firstRow + 0.5) * app->font->getHeight());
				app->screen->box(5, rowY, app->config->resolutionX() - 16, 1, 255, 255, 255, 130);
				app->screen->box(5, rowY + 1, app->config->resolutionX() - 16, 1, 0, 0, 0, 130);
			} else {
				rowY = 42 + (i - firstRow) * app->font->getHeight();
				app->font->write(app->screen, log.at(j), 5, rowY);
			}
		}

		app->ui->drawScrollBar(
			rowsPerPage, 
			log.size(), 
			firstRow, 
			app->listRect);

		app->screen->flip();

		bool inputAction = app->input.update(false);
		
		if ( app->input[UP  ] && firstRow > 0 ) firstRow--;
		else if ( app->input[DOWN] && firstRow + rowsPerPage < log.size() ) firstRow++;
		else if ( app->input[PAGEUP] || app->input[LEFT]) {
			firstRow -= rowsPerPage - 1;
			if (firstRow < 0) firstRow = 0;
		}
		else if ( app->input[PAGEDOWN] || app->input[RIGHT]) {
			firstRow += rowsPerPage - 1;
			if (firstRow + rowsPerPage >= log.size()) firstRow = max(0, log.size() - rowsPerPage);
		}
		else if ( app->input[SETTINGS] || app->input[CANCEL] ) close = true;
		else if (app->input[MENU]) {
			MessageBox mb(
				app, 
				app->tr.translate("Deleting $1", "battery.csv", NULL) + "\n" + app->tr["Are you sure?"]);

			mb.setButton(CONFIRM, app->tr["Yes"]);
			mb.setButton(CANCEL,  app->tr["No"]);
			if (mb.exec() == CONFIRM) {
				string cmd = "rm " + app->getWriteablePath() + "battery.csv";
				system(cmd.c_str());
				log.clear();
			}
		}
	}
	app->hw->setBacklightLevel(app->config->backlightLevel());
}
