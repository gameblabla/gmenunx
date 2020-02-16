#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

#include "installer.h"
#include "debug.h"
#include "fileutils.h"
#include "constants.h"

#define sync() sync(); std::system("sync");

const std::string Installer::LAUNCHER_PATH="/usr/local/sbin/frontend_start";
const std::string Installer::INSTALLER_MARKER_FILE = "/tmp/" + BINARY_NAME + ".marker";

Installer::Installer(std::string const & source, std::string const & destination, std::function<void(std::string)> callback) {
    this->sourceRootPath = source;
    this->destinationRootPath = destination;
    this->notifiable = callback;
}

Installer::~Installer() {
    TRACE("enter");
}

bool Installer::install(IHardware * hw) {
    TRACE("enter");
    bool result = false;
    if (!FileUtils::dirExists(this->destinationRootPath)) {
        if (mkdir(this->destinationRootPath.c_str(),0777) != 0) {
            ERROR("Couldn't create install root dir : %s", this->destinationRootPath.c_str());
            return false;
        }
        TRACE("created install dir : %s", this->destinationRootPath.c_str());
    }
    if (this->copyFiles()) {
        if (this->copyDirs(true)) {
            result = this->setBinaryPermissions();
        }
    }

    // hardware specific files
    std::string fileName = hw->inputFile();
    this->notify("file: " + fileName);
    std::string source = this->sourceRootPath + "input/" + fileName;
    std::string destination = this->destinationRootPath + "input.conf";
    if (!FileUtils::copyFile(source, destination)) 
        return false;

    TRACE("exit : %i", result);
    return result;
}

bool Installer::setBinaryPermissions() {
    TRACE("enter");
    bool result = false;
    struct stat fstat;
    if ( stat( this->binaryPath().c_str(), &fstat ) == 0 ) {
        struct stat newstat = fstat;
        if ( S_IRUSR != ( fstat.st_mode & S_IRUSR ) ) newstat.st_mode |= S_IRUSR;
        if ( S_IXUSR != ( fstat.st_mode & S_IXUSR ) ) newstat.st_mode |= S_IXUSR;
        if ( fstat.st_mode != newstat.st_mode ) {
            result = (0 == chmod( this->binaryPath().c_str(), newstat.st_mode ));
        } else result = true;
    } else {
        ERROR("Couldn't stat : %s", this->binaryPath().c_str());
    }
    TRACE("exit : %i", result);
    return result;
}

bool Installer::upgrade() {
    TRACE("enter");
    bool result = false;
    if (this->copyFiles()) {
        if (this->copyDirs(false)) {
            result = this->setBinaryPermissions();
        }
    }
    TRACE("exit : %i", result);
    return result;
}

bool Installer::copyFiles() {
    TRACE("enter");
    for (std::vector<std::string>::iterator it = this->fileManifest.begin(); it != this->fileManifest.end(); it++) {
        std::string fileName = (*it);
        std::string source = this->sourceRootPath + fileName;
        std::string destination = this->destinationRootPath + fileName;
        TRACE("copying file from : %s to %s", source.c_str(), destination.c_str());
        this->notify("file: " + fileName);
        if (!FileUtils::copyFile(source, destination)) 
            return false;
    }
    return true;
    TRACE("exit");
}

bool Installer::copyDirs(bool force) {
    TRACE("enter");
    std::string cp = force ? "/bin/cp -af" : "/bin/cp -a";
    for (std::vector<std::string>::iterator it = this->folderManifest.begin(); it != this->folderManifest.end(); it++) {
        std::string directory = (*it);
        std::string source = this->sourceRootPath + directory;
        std::string destination = this->destinationRootPath;// + directory;
        TRACE("copying dir from : %s to %s", source.c_str(), destination.c_str());
        this->notify("directory: " + directory);
        if (!FileUtils::dirExists(source)) {
            ERROR("Source directory doesn't exist : %s", source.c_str());
            return false;
        }
        std::stringstream ss;
        ss << cp << " \"" << source << "\" " << "\"" << destination << "\"";
        std::string call = ss.str();
        TRACE("running command : %s", call.c_str());
        std::system(call.c_str());
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
    if (FileUtils::fileExists(Installer::LAUNCHER_PATH))
        return 0 == unlink(Installer::LAUNCHER_PATH.c_str());
    return true;
}

const bool Installer::deployLauncher() {

    std::string binary = this->destinationRootPath + BINARY_NAME;

    TRACE("checking binary exists : %s", binary.c_str());
    if (!FileUtils::fileExists(binary)) {
        TRACE("can't risk an install of launcher");
        TRACE("binary not found at : %s", binary.c_str());
        this->notify("Couldn't find " + binary);
        return false;
    }

    TRACE("checking for pre-existing launcher");
    this->notify("Checking for pre-existing launcher");
    if (FileUtils::fileExists(Installer::LAUNCHER_PATH)) {
        TRACE("removing pre-existing launcher");
        this->notify("Removing pre-existing launcher");
        if (0 != unlink(Installer::LAUNCHER_PATH.c_str())) {
            TRACE("couldn't delete pre-existing launcher script");
            this->notify("Couldn't delete pre-existing launcher script");
            return false;
        }
    }

    TRACE("writing out launcher script");
    this->notify("Writing out launcher script");
	std::ofstream launcher(Installer::LAUNCHER_PATH.c_str(), std::ofstream::out);
	if (launcher.is_open()) {
		launcher << "#!/bin/sh\n\n";
        launcher << "# launcher script for " << APP_NAME << "\n\n";
        launcher << "BINARY=" << binary << "\n";
        launcher << "MARKER=" << Installer::INSTALLER_MARKER_FILE << "\n";
        launcher << "\n";
        launcher << "if [ -f ${BINARY} ] && [ ! -f ${MARKER} ]; then\n";
        launcher << "\t${BINARY}\n";
        launcher << "else\n";
        launcher << "\tif [ -f ${MARKER} ];then\n";
        launcher << "\t\trm -f ${MARKER}\n";
        launcher << "fi\n";
        launcher << "\t/usr/bin/gmenu2x\n";
        launcher << "fi\n";
		launcher.close();
		sync();
	} else {
        TRACE("couldn't open '%s' for writing", Installer::LAUNCHER_PATH.c_str());
        this->notify("Couldn't install script: \n" + Installer::LAUNCHER_PATH);
        return false;
    }

    // chmod it
    TRACE("checking file permissions");
    this->notify("Checking file permissions");
    struct stat fstat;
    if ( stat( Installer::LAUNCHER_PATH.c_str(), &fstat ) == 0 ) {
        struct stat newstat = fstat;
        if ( S_IRUSR != ( fstat.st_mode & S_IRUSR ) ) newstat.st_mode |= S_IRUSR;
        if ( S_IXUSR != ( fstat.st_mode & S_IXUSR ) ) newstat.st_mode |= S_IXUSR;
        if ( fstat.st_mode != newstat.st_mode ) {
            this->notify("Updating file permissions");
            chmod( Installer::LAUNCHER_PATH.c_str(), newstat.st_mode );
        }
    }
    return true;
}

const bool Installer::isDefaultLauncher(const std::string &path) {

    if (path.empty())
        return false;

    std::string binary = path + BINARY_NAME;
    TRACE("checking if we're the launcher for : %s", binary.c_str());

    std::ifstream file( Installer::LAUNCHER_PATH, std::ifstream::in );
    if (!file) {
        TRACE("couldn't read launcher file : %s", Installer::LAUNCHER_PATH.c_str());
        return false;
    }
    std::string line;
    bool result = false;
    bool appMatches = false;
    bool binaryMatches = false;
    while(getline(file, line)) {
        if (line.find(APP_NAME, 0) != std::string::npos) {
            TRACE("found our app in line : %s", line.c_str());
            appMatches = true;
        } else if (line.find(binary, 0) != std::string::npos) {
            TRACE("found our binary in line : %s", line.c_str());
            binaryMatches = true;
        }
        if (binaryMatches && appMatches) {
            result = true;
            break;
        }
    }
    file.close();
    return result;
}

const bool Installer::setBootMarker() {
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
    if (FileUtils::fileExists(Installer::INSTALLER_MARKER_FILE))
        return 0 == unlink(Installer::INSTALLER_MARKER_FILE.c_str());
    return true;
}