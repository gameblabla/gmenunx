#ifndef INSTALLER_H_
#define INSTALLER_H_

#include <vector>
#include <string>

#include "progressbar.h"

class Installer {
    private:

        ProgressBar * progressBar;

        std::string sourceRootPath;
        std::string destinationRootPath;

        std::string const configFile = "gmenunx.conf";
        std::string const defaultSkinPath = "skins/Default";
        std::vector<std::string> fileManifest = {
            "COPYING", 
            "ChangeLog.md", 
            "about.txt", 
            "gmenunx.png", 
            "input.conf"
        };
        std::vector<std::string> folderManifest = {
            "scripts", 
            "sections", 
            "skins", 
            "translations"
        };

        void notify(std::string text);
        bool copyDirs(bool force = false);
        bool copyFiles();

    public:
        Installer(std::string const & source, std::string const & destination, ProgressBar * pb = nullptr);
        ~Installer();
        bool install();
        bool upgrade();
};

#endif