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

bool Monitor::event_accepted(const struct inotify_event &event) {
	TRACE("accepting event : %s", event.name);
	return true;
}

void Monitor::inject_event(bool is_add, const std::string &path) {
	TRACE("injecting event : %i for path : %s", is_add, path.c_str());
	return;
}

#include <sys/ioctl.h>

int Monitor::run() {

	DEBUG("Starting inotify thread for path %s...\n", this->path.c_str());

	if (this->running_) {
		TRACE("monitor is already running");
		return -1;
	}

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

	this->running_ = true;
	DEBUG("Starting watching directory %s\n", this->path.c_str());

	for (;;) {

		unsigned int avail;
		ioctl(this->fd, FIONREAD, &avail);
		TRACE("inotify has %i bytes to read", avail);
		char buf[avail];
		read(fd, buf, avail);
		int offset = 0;

		while (offset < avail) {

			struct inotify_event *event = (inotify_event*)(buf + offset);

			offset = offset + sizeof(inotify_event) + event->len;

			TRACE("received event : %s", event->name);
			std::string eventName = std::string(strdup(event->name));
			TRACE("handling inotify event : '%s' for wd : %i", eventName.c_str(), event->wd);

			if (event->wd != this->wd) {
				TRACE("skipping event for non matching wd : %i vs. %i", this->wd, event->wd);
				continue;
			}

			TRACE("wd belongs to me, checking masks");
			if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF)) {
				ERROR("We have deleted or removed ourselves, not cool!");
				inject_event(false, this->path);
				break;
			}

			TRACE("masks pass ok for event : '%s'", event->name);
			std::string fullPath = this->path + "/" + eventName;
			TRACE("seeing if we will accept event name : '%s'", fullPath.c_str());

			if (!this->event_accepted((*event))) {
				TRACE("event not accepted");
			} else {
				TRACE("yes we did, notifying listeners");
				inject_event(event->mask & (IN_MOVED_TO | IN_CLOSE_WRITE | IN_CREATE), fullPath);
			}
		}
	}
	INFO("deleted self or moved self");
	return 0;
}

static void * inotify_thd(void *p) {
	Monitor *monitor = (Monitor *) p;
	monitor->run();
	return NULL;
}

void Monitor::stop() {
	TRACE("enter");
	if (!this->running_)
		return;
	if (this->fd > 0 && this->wd > 0) {
		TRACE("removing inotify watch : %i", this->wd);
		if (0 == inotify_rm_watch(this->fd, this->wd)) {
			TRACE("watch removed successfully");
			this->wd = 0;
		}
	}
	if (this->wd == 0 && this->fd > 0) {
		TRACE("closing fd : %i", this->fd);
		close(this->fd);
		this->fd = 0;
	}
	this->running_ = !(this->fd == 0 && this->wd == 0);
	TRACE("exit : %i", this->running_ );
}

Monitor::Monitor(std::string path, unsigned int flags) {
	this->running_ = false;
	this->mask = flags;
	this->path = std::string(path);
	this->wd = this->fd = 0;
	pthread_create(&thd, NULL, inotify_thd, (void *) this);
}

Monitor::~Monitor() {
	this->stop();
	if (this->thd) {
		pthread_cancel(this->thd);
		pthread_join(this->thd, NULL);
	}
	DEBUG("Monitor thread stopped (was watching %s)", path.c_str());
}
#endif
