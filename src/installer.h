#ifndef INSTALLER_H_
#define INSTALLER_H_

#include <vector>
#include <string>
#include <functional>


class Installer {

    private:

        static const std::string LAUNCHER_PATH;
        static const std::string INSTALLER_MARKER_FILE;

        std::function<void(std::string)> notifiable;

        std::string sourceRootPath;
        std::string destinationRootPath;

        std::vector<std::string> fileManifest = {
            "about.txt", 
            "ChangeLog.md", 
            "COPYING", 
            "gmenunx.conf", 
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

        Installer(std::string const & source, std::string const & destination, std::function<void(std::string)> callback = nullptr);
        ~Installer();

        static const bool isDefaultLauncher(const std::string &opkPath);
        static const bool removeLauncher();
        static const bool deployLauncher();
        static const bool leaveBootMarker();
        static const bool removeBootMarker();

        bool install();
        bool upgrade();



};

#endif