#ifndef LINKSCANNERDIALOG_H_
#define LINKSCANNERDIALOG_H_

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <vector>

#include "esoteric.h"
#include "dialog.h"
#include "menu.h"
#include "messagebox.h"

class LinkScannerDialog : protected Dialog {

private:
	std::vector<std::string> messages_;
	bool finished_;
	int maxMessagesOnScreen;

protected:
	int foundFiles = 0;
	int selectedItem = 0;
	std::string title, description, icon;
	void scanPath(std::string path, std::vector<std::string> *files);
	void quit();
	void render();

public:
	LinkScannerDialog(Esoteric *app, const std::string &title, const std::string &description, const std::string &icon);
	~LinkScannerDialog();
	void exec();
	void notify(std::string message);
};

#endif /*LINKSCANNERDIALOG_H_*/
