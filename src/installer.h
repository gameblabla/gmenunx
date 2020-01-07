#ifndef INSTALLER_H_
#define INSTALLER_H_

#include <vector>
#include <string>
#include <functional>

#include "constants.h"

class Installer {

    private:

        static const std::string LAUNCHER_PATH;
        static const std::string INSTALLER_MARKER_FILE;

        std::function<void(std::string)> notifiable;

        std::string sourceRootPath;
        std::string destinationRootPath;

        std::vector<std::string> fileManifest = {
            BINARY_NAME, 
            BINARY_NAME + ".conf", 
            BINARY_NAME + ".man.txt",
            "about.txt", 
            "ChangeLog.md", 
            "COPYING", 
            "logo.png"
        };
        std::vector<std::string> folderManifest = {
            "input", 
            "scripts", 
            "sections", 
            "skins", 
            "translations"
        };

        void notify(std::string text);
        bool copyDirs(bool force = false);
        bool copyFiles();
        bool setBinaryPermissions();

    public:

        Installer(std::string const & source, std::string const & destination, std::function<void(std::string)> callback = nullptr);
        ~Installer();

        static const bool isDefaultLauncher(const std::string &opkPath);
        static const bool removeLauncher();
        const bool deployLauncher();
        static const bool leaveBootMarker();
        static const bool removeBootMarker();
        std::string binaryPath() { return this-> destinationRootPath + BINARY_NAME; }
        bool install();
        bool upgrade();

};

#endif