#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Bind and activate the framebuffer console on selected platforms.
#include <linux/vt.h>

#include "constants.h"
#include "launcher.h"
#include "debug.h"

using namespace std;

Launcher::Launcher(vector<string> const& commandLine, bool consoleApp)
	: commandLine(commandLine)
	, consoleApp(consoleApp)
{
}

Launcher::Launcher(vector<string> && commandLine, bool consoleApp)
	: commandLine(commandLine)
	, consoleApp(consoleApp)
{
}

void Launcher::exec() {
	TRACE("enter");
	
	if (this->consoleApp) {
		TRACE("console app");

		/* Enable the framebuffer console */
		TRACE("enabling framebuffer");
		char c = '1';
		int fd = open("/sys/devices/virtual/vtconsole/vtcon1/bind", O_WRONLY);
		if (fd < 0) {
			WARNING("Unable to open fbcon handle");
		} else {
			write(fd, &c, 1);
			close(fd);
		}
		TRACE("opening tty handle");
		fd = open(APP_TTY.c_str(), O_RDWR);
		if (fd < 0) {
			WARNING("Unable to open %s handle", APP_TTY.c_str());
		} else {
			if (ioctl(fd, VT_ACTIVATE, 1) < 0)
				WARNING("Unable to activate %s", APP_TTY.c_str());
			close(fd);
		}
		TRACE("opened %s successfully", APP_TTY.c_str());
	}

	TRACE("sorting args out for size : %zu", commandLine.size());
	vector<const char *> args;
	args.reserve(commandLine.size() + 1);
	TRACE("sorting args reserved");
	std::string s;
	for (auto arg : commandLine) {
		TRACE("pushing back arg : %s", arg.c_str());
		args.push_back(arg.c_str());
		s += " " + arg;
	}
	args.push_back(nullptr);
	TRACE("args finished");
	TRACE("exec-ing : %s", s.c_str());

	execvp(commandLine[0].c_str(), (char* const*)&args[0]);
	WARNING("Failed to exec '%s': %s", s.c_str(), strerror(errno));
	
	TRACE("exit");
}
