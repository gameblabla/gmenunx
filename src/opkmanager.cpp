
#include <cstring> 
//#include <iostream>
//#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
//#include <algorithm>
#include <stdlib.h>
//#include <unistd.h>
#include <math.h>
//#include <signal.h>

#include <errno.h>

//for browsing the filesystem
#include <opk.h>
//#include <sys/statvfs.h>
#include <sys/stat.h>
//#include <sys/types.h>
#include <dirent.h>

#include "opkmanager.h"
#include "debug.h"
#include "gmenu2x.h"
#include "link.h"
#include "linkapp.h"

using namespace std;

/*
 * public section
 */
OpkManager::OpkManager(GMenu2X *gmenu2x) : gmenu2x(gmenu2x) {
	TRACE("OpkManager::OpkManager - enter");

	TRACE("OpkManager::OpkManager - exit");
}

OpkManager::~OpkManager() {
	TRACE("OpkManager::~OpkManager - enter");

	TRACE("OpkManager::~OpkManager - exit");
}

void OpkManager::openPackage(std::string path, bool order) {
	TRACE("OpkManager::openPackage - enter");

	/* First try to remove existing links of the same OPK
	 * (needed for instance when an OPK is modified) */
	removePackageLink(path);

	struct OPK *opk = opk_open(path.c_str());
	if (!opk) {
		ERROR("Unable to open OPK %s\n", path.c_str());
		return;
	}

	for (;;) {
		unsigned int i;
		bool has_metadata = false;
		const char *name;
		LinkApp *link;

		for (;;) {
			string::size_type pos;
			int ret = opk_open_metadata(opk, &name);
			if (ret < 0) {
				ERROR("Error while loading meta-data\n");
				break;
			} else if (!ret)
			  break;

			/* Strip .desktop */
			string metadata(name);
			pos = metadata.rfind('.');
			metadata = metadata.substr(0, pos);

			// let's assume for now we only install stuff for our system!
			pos = metadata.rfind('.');
			metadata = metadata.substr(pos + 1);
			has_metadata = true;
			break;

			/*
			pos = metadata.rfind('.');
			metadata = metadata.substr(pos + 1);

			if (metadata == PLATFORM || metadata == "all") {
				has_metadata = true;
				break;
			}
			*/
		}

		if (!has_metadata)
		  break;

		// Note: OPK links can only be deleted by removing the OPK itself,
		//       but that is not something we want to do in the menu,
		//       so consider this link undeletable.
		
		link = new LinkApp(gmenu2x, path.c_str(), false, opk, name);
		link->setSize(gmenu2x->skinConfInt["linkWidth"], gmenu2x->skinConfInt["linkHeight"]);

/*

here we should just add it to a collection and return it, and run this in the menu class...

*/

/*
		// this needs to just trawl the vector
		addSection(link->getCategory());
		for (i = 0; i < sections.size(); i++) {
			if (sections[i] == link->getCategory()) {
				links[i].push_back(link);
				break;
			}
		}
*/

	}

	opk_close(opk);

	if (order)
		orderLinks();

	TRACE("OpkManager::openPackage - exit");
}

void OpkManager::openPackagesFromDir(std::string path, std::vector<std::vector<Link*>>& links) {
	TRACE("OpkManager::openPackagesFromDir - enter");
	// grab our own local copy for orderin etc
	this->links = links;

	DEBUG("Opening packages from directory: %s\n", path.c_str());
	readPackages(path);

	TRACE("OpkManager::openPackagesFromDir - exit");
	return;
}

void OpkManager::removePackageLink(std::string path) {
	TRACE("OpkManager::removePackageLink - enter");
	
	TRACE("OpkManager::removePackageLink - exit");
}

/*
 * private section
 */

void OpkManager::readPackages(std::string parentDir) {
	TRACE("OpkManager::readPackages - enter");

	DIR *dirp;
	struct dirent *dptr;

	dirp = opendir(parentDir.c_str());
	if (!dirp)
		return;

	while ((dptr = readdir(dirp))) {
		char *c;

		if (dptr->d_type != DT_REG)
			continue;

		c = strrchr(dptr->d_name, '.');
		if (!c) /* File without extension */
			continue;

		if (strcasecmp(c + 1, "opk"))
			continue;

		if (dptr->d_name[0] == '.') {
			// Ignore hidden files.
			continue;
		}

		openPackage(parentDir + '/' + dptr->d_name, false);
	}

	closedir(dirp);
	orderLinks();

	TRACE("OpkManager::readPackages - exit");
}

void OpkManager::orderLinks() {
	for (auto& section : links) {
		// TODO :: fix me
		//sort(section.begin(), section.end(), compare_links);
	}
}
