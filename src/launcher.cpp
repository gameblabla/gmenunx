#include "launcher.h"

#include "debug.h"

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
#define BIND_CONSOLE \
		defined(PLATFORM_A320) || \
		defined(PLATFORM_GCW0)|| \
		defined(PLATFORM_RG350)

#if BIND_CONSOLE
#include <linux/vt.h>
#endif

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
	TRACE("Launcher::exec - enter");
	
	if (consoleApp) {
		TRACE("Launcher::exec - console app");
#if BIND_CONSOLE
		/* Enable the framebuffer console */
		TRACE("Launcher::exec - enabling framebuffer");
		char c = '1';
		int fd = open("/sys/devices/virtual/vtconsole/vtcon1/bind", O_WRONLY);
		if (fd < 0) {
			WARNING("Unable to open fbcon handle\n");
		} else {
			write(fd, &c, 1);
			close(fd);
		}
		TRACE("Launcher::exec - opening tty handle");
		fd = open("/dev/tty1", O_RDWR);
		if (fd < 0) {
			WARNING("Unable to open tty1 handle\n");
		} else {
			if (ioctl(fd, VT_ACTIVATE, 1) < 0)
				WARNING("Unable to activate tty1\n");
			close(fd);
		}
#endif
		TRACE("Launcher::exec - end of console specific work");
	}

	TRACE("Launcher::exec - sorting args out for size : %i", commandLine.size() + 1);
	vector<const char *> args;
	args.reserve(commandLine.size() + 1);
	TRACE("Launcher::exec - sorting args reserved");
	std::string s;
	for (auto arg : commandLine) {
		TRACE("Launcher::exec - pushing back arg : %s", arg.c_str());
		args.push_back(arg.c_str());
		s += " " + arg;
	}
	args.push_back(nullptr);
	TRACE("Launcher::exec - args finished");
	TRACE("Launcher::exec - exec-ing :: %s", s.c_str());

	execvp(commandLine[0].c_str(), (char* const*)&args[0]);
	WARNING("Failed to exec '%s': %s\n",
			commandLine[0].c_str(), strerror(errno));
	
	TRACE("Launcher::exec - exit");
}
