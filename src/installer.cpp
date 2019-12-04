#include <string>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

#include "installer.h"
#include "debug.h"
#include "utilities.h"
#include "constants.h"

#define sync() sync(); system("sync");

const std::string Installer::LAUNCHER_PATH="/usr/local/sbin/frontend_start";
const std::string Installer::INSTALLER_MARKER_FILE = "/tmp/" + BINARY_NAME + ".marker";

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
    string cp = force ? "/bin/cp -arfp" : "/bin/cp -arp";
    for (vector<string>::iterator it = this->folderManifest.begin(); it != this->folderManifest.end(); it++) {
        std::string directory = (*it);
        std::string source = this->sourceRootPath + directory;
        std::string destination = this->destinationRootPath;// + directory;
        TRACE("copying dir from : %s to %s", source.c_str(), destination.c_str());
        this->notify("directory: " + dir_name(directory));
        if (!dirExists(source)) {
            ERROR("Source directory doesn't exist : %s", source.c_str());
            return false;
        }
        std::stringstream ss;
        ss << cp << " \"" << source << "\" " << "\"" << destination << "\"";
        std::string call = ss.str();
        TRACE("running command : %s", call.c_str());
        system(call.c_str());
        sync();
    }
    TRACE("exit");
    return true;
}

void Installer::notify(std::string message) {
    if (nullptr != this->notifiable) {
        this->notifiable(message);
    }
}

const bool Installer::removeLauncher() {
    if (fileExists(Installer::LAUNCHER_PATH))
        return 0 == unlink(Installer::LAUNCHER_PATH.c_str());
    return true;
}

const bool Installer::deployLauncher() {

    if (fileExists(Installer::LAUNCHER_PATH))
        unlink(Installer::LAUNCHER_PATH.c_str());

    // write the file
    string opk = getOpkPath();
    if (opk.empty()) {
        TRACE("can't risk an install of launcher, opk not found");
        return false;
    }
    TRACE("opk path : %s", opk.c_str());

	std::ofstream launcher(Installer::LAUNCHER_PATH.c_str());
	if (launcher.is_open()) {
		launcher << "#!/bin/sh\n\n";
        launcher << "# launcher script for " << APP_NAME << "\n\n";
        launcher << "OPK_PATH=" << opk << "\n";
        launcher << "MARKER=" << Installer::INSTALLER_MARKER_FILE << "\n";
        launcher << "LOG_FILE=/tmp/" << BINARY_NAME << ".run.log\n";
        launcher << "\n";
        launcher << "if [ -f ${OPK_PATH} ] && [ ! -f ${MARKER} ]; then\n";
        launcher << "\trm -f ${LOG_FILE}\n";
        launcher << "\t/usr/bin/opkrun -m default.gcw0.desktop ${OPK_PATH} 2>&1 >> ${LOG_FILE}\n";
        launcher << "else\n";
        launcher << "\tif [ -f ${MARKER} ];then\n";
        launcher << "\t\trm -f ${MARKER}\n";
        launcher << "fi\n";
        launcher << "\t/usr/bin/app\n";
        launcher << "fi\n";
		launcher.close();
		sync();
	}

    // chmod it
    struct stat fstat;
    if ( stat( Installer::LAUNCHER_PATH.c_str(), &fstat ) == 0 ) {
        struct stat newstat = fstat;
        if ( S_IRUSR != ( fstat.st_mode & S_IRUSR ) ) newstat.st_mode |= S_IRUSR;
        if ( S_IXUSR != ( fstat.st_mode & S_IXUSR ) ) newstat.st_mode |= S_IXUSR;
        if ( fstat.st_mode != newstat.st_mode ) chmod( Installer::LAUNCHER_PATH.c_str(), newstat.st_mode );
    }
    return true;
}

const bool Installer::isDefaultLauncher(const string &opkPath) {
    TRACE("checking if we're the launcher for opk : %s", opkPath.c_str());
    if (opkPath.empty())
        return false;
    std::fstream file( Installer::LAUNCHER_PATH );
    if (!file) {
        TRACE("couldn't read launcher file : %s", Installer::LAUNCHER_PATH.c_str());
        return false;
    }
    std::string line;
    bool result = false;
    bool appMatches = false;
    bool opkMatches = false;
    while(getline(file, line)) {
        if (line.find(APP_NAME, 0) != std::string::npos) {
            TRACE("found out app in line : %s", line.c_str());
            appMatches = true;
        } else if (line.find(opkPath, 0) != std::string::npos) {
            TRACE("found out opk in line : %s", line.c_str());
            opkMatches = true;
        }
        if (opkMatches && appMatches) {
            result = true;
            break;
        }
    }
    file.close();
    return result;
}

const bool Installer::leaveBootMarker() {
    try {
        std::fstream fs;
        fs.open(Installer::INSTALLER_MARKER_FILE, std::ios::out);
        fs.close();
        return true;
    } catch(...) {
        return false;
    }
}

const bool Installer::removeBootMarker() {
    if (fileExists(Installer::INSTALLER_MARKER_FILE))
        return 0 == unlink(Installer::INSTALLER_MARKER_FILE.c_str());
    return true;
}