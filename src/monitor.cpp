#ifdef ENABLE_INOTIFY
#include "debug.h"

#include <dirent.h>
#include <pthread.h>
#include <SDL.h>
#include <signal.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "inputmanager.h"
#include "monitor.h"
#include "utilities.h"

bool Monitor::event_accepted(const std::string &path) {
	TRACE("accepting event : %s", path.c_str());
	return true;
}

void Monitor::inject_event(bool is_add, const std::string &path) {
	TRACE("injecting event : %i for path : %s", is_add, path.c_str());
	return;
}

int Monitor::run() {

	DEBUG("Starting inotify thread for path %s...\n", this->path.c_str());

	this->fd = inotify_init1(IN_CLOEXEC);
	if (this->fd < 0) {
		ERROR("Unable to start inotify\n");
		return this->fd;
	}

	this->wd = inotify_add_watch(fd, this->path.c_str(), this->mask);
	if (this->wd < 0) {
		ERROR("Unable to add inotify watch\n");
		close(this->fd);
		return this->wd;
	}

	DEBUG("Starting watching directory %s\n", this->path.c_str());

	for (;;) {
		std::size_t len = sizeof(struct inotify_event) + NAME_MAX + 1;
		struct inotify_event event;
		char buf[256];

		read(this->fd, &event, len);
		string eventName = string(event.name);
		TRACE("handling inotify event : %s for wd : %i", eventName.c_str(), event.wd);

		if (event.wd != this->wd) {
			TRACE("skipping event for non matching wd : %i vs. %i", this->wd, event.wd);
			continue;
		}

		TRACE("wd belongs to me, checking masks");
		if (event.mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
			ERROR("We have deleted or removed ourselves, not cool!");
			inject_event(false, this->path);
			break;
		}

		TRACE("masks pass ok for : %s", eventName.c_str());
		std::string fullPath = this->path + "/" + eventName;
		TRACE("seeing if we will accept event name : %s", fullPath.c_str());
		if (!this->event_accepted(fullPath)) {
			TRACE("no we didn't");
			continue;
		}
		TRACE("yes we did, notifying listeners");
		inject_event(event.mask & (IN_MOVED_TO | IN_CLOSE_WRITE | IN_CREATE), fullPath);
	}

	INFO("deleted self or moved self");
	return 0;
}

static void * inotify_thd(void *p) {
	Monitor *monitor = (Monitor *) p;
	monitor->run();
	return NULL;
}

Monitor::Monitor(std::string path, unsigned int flags) {
	this->mask = flags;
	this->path = std::string(path);
	this->wd = this->fd = 0;
	pthread_create(&thd, NULL, inotify_thd, (void *) this);
}

Monitor::~Monitor() {
	if (this->fd > 0 && this->wd > 0) {
		inotify_rm_watch(this->fd, this->wd);
	}
	if (this->fd > 0) {
		close(this->fd);
	}
	pthread_cancel(thd);
	pthread_join(thd, NULL);
	DEBUG("Monitor thread stopped (was watching %s)\n", path.c_str());
}
#endif
