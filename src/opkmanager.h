#ifndef OPKMANAGER_H
#define OPKMANAGER_H

#include <vector>
#include <string>
#include <memory>

#include "link.h"

#define OPK_PATH "/media/data/apps"

class OpkManager {

public:
	OpkManager(GMenu2X *gmenu2x);
	virtual ~OpkManager();

	void openPackage(std::string path, bool order = true);
	void openPackagesFromDir(std::string path, std::vector<std::vector<Link*>> &links);
	void removePackageLink(std::string path);
	
	std::vector<std::vector<Link*>> links;

private:

	GMenu2X *gmenu2x;
	void readPackages(std::string parentDir);
	void orderLinks();
};

#endif
