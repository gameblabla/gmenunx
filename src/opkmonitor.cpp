#ifdef ENABLE_INOTIFY
#include <sys/inotify.h>
#include <SDL/SDL.h>
#include <unistd.h>

#include "debug.h"
#include "inputmanager.h"
#include "opkmonitor.h"
#include "utilities.h"

OpkMonitor::OpkMonitor(
			std::string dir, 
			std::function<void(std::string path)> fileAdded, 
			std::function<void(std::string path)> fileRemoved) :
	Monitor(dir, IN_MOVE | IN_DELETE | IN_CREATE | IN_ONLYDIR) {

		this->fileAdded = fileAdded;
		this->fileRemoved = fileRemoved;

}

bool OpkMonitor::event_accepted(const struct inotify_event &event) {
	// Don't bother about files other than OPKs
	std::size_t len = strlen(event.name);
	if (len > this->extension.length()) {
		std::string eventExtension = ((std::string)event.name).substr(len - this->extension.length());
		TRACE("comparing '%s' to our requested extension filter : '%s'", 
			eventExtension.c_str(), 
			this->extension.c_str());

		return 0 == eventExtension.compare(this->extension);
	}
	return false;
}

void OpkMonitor::inject_event(bool is_add, const std::string &path) {
	/* Sleep for a bit, to ensure that the media will be mounted
	 * on the mountpoint before we start looking for OPKs */
	sleep(1);
	if (is_add) {
		if (nullptr != this->fileAdded) {
			TRACE("notifying fileAdded listener about : %s", path.c_str());
			this->fileAdded(path);
		}
	} else {
		if (nullptr != this->fileRemoved) {
			TRACE("notifying fileRemoved listener about : %s", path.c_str());
			this->fileRemoved(path);
		}
	}
}

#endif /* ENABLE_INOTIFY */
