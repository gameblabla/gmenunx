#ifndef __OPKMONITOR_H__
#define __OPKMONITOR_H__
#ifdef ENABLE_INOTIFY

#include <functional>
#include <string>

#include "monitor.h"

class OpkMonitor: public Monitor {
	public:
		OpkMonitor(
			std::string dir, 
			std::function<void(std::string path)> fileAdded = nullptr, 
			std::function<void(std::string path)> fileRemoved = nullptr
		);

	protected:
		bool event_accepted(const struct inotify_event &event);
		void inject_event(bool is_add, const std::string &path);

	private:

		const std::string extension = ".opk";
		std::function<void(std::string path)> fileAdded;
		std::function<void(std::string path)> fileRemoved;
};

#endif /* ENABLE_INOTIFY */
#endif /* __OPKMONITOR_H__ */
