#include <string>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>

#include "installer.h"
#include "debug.h"
#include "utilities.h"

#define sync() sync(); system("sync");

Installer::Installer(std::string const & source, std::string const & destination, std::function<void(string)> callback) {
    this->sourceRootPath = source;
    this->destinationRootPath = destination;
    this->notifiable = callback;
}

Installer::~Installer() {
    TRACE("enter");
}

bool Installer::install() {
    TRACE("enter");
    bool result = false;
    if (!dirExists(this->destinationRootPath)) {
        if (mkdir(this->destinationRootPath.c_str(),0777) != 0) {
            ERROR("Couldn't create install root dir : %s", this->destinationRootPath.c_str());
            return false;
        }
        TRACE("created install dir : %s", this->destinationRootPath.c_str());
    }
    if (this->copyFiles()) {
        result = this->copyDirs(true);
    }
    TRACE("exit : %i", result);
    return result;
}

bool Installer::upgrade() {
    if (this->copyFiles()) {
        return this->copyDirs(false);
    }
    return false;
}

bool Installer::copyFiles() {
    TRACE("enter");
    for (vector<string>::iterator it = this->fileManifest.begin(); it != this->fileManifest.end(); it++) {
        std::string fileName = (*it);
        std::string source = this->sourceRootPath + fileName;
        std::string destination = this->destinationRootPath + fileName;
        TRACE("copying file from : %s to %s", source.c_str(), destination.c_str());
        this->notify("file: " + fileName);
        if (!copyFile(source, destination)) return false;
        sync();
    }
    return true;
    TRACE("exit");
}

bool Installer::copyDirs(bool force) {
    TRACE("enter");
    string cp = force ? "/bin/cp -arp" : "/bin/cp -arnp";
    for (vector<string>::iterator it = this->folderManifest.begin(); it != this->folderManifest.end(); it++) {
        std::string directory = (*it);
        std::string source = this->sourceRootPath + directory;
        std::string destination = this->destinationRootPath + directory;
        TRACE("copying dir from : %s to %s", source.c_str(), destination.c_str());
        this->notify("directory: " + dir_name(directory));
        if (!dirExists(source)) {
            ERROR("Source directory doesn't exist : %s", source.c_str());
            return false;
        }

        if (!dirExists(destination) || force) {
            std::stringstream ss;
            ss << cp << " \"" << source << "\" " << "\"" << destination << "\"";
            std::string call = ss.str();
            TRACE("running command : %s", call.c_str());
            system(call.c_str());
            sync();
        }
    }
    TRACE("exit");
    return true;
}

void Installer::notify(std::string message) {
    if (nullptr != this->notifiable) {
        this->notifiable(message);
    }
}