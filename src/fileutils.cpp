
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

#include "debug.h"
#include "fileutils.h"
#include "stringutils.h"
#include "utilities.h"

// returns a filename minus the dot extension part
std::string FileUtils::fileBaseName(const std::string &filename) {
    std::string::size_type i = filename.rfind(".");
    if (i != std::string::npos) {
        return filename.substr(0, i);
    }
    return filename;
}

// returns a files dot extension if there is on,e or an empty string
std::string FileUtils::fileExtension(const std::string &filename) {
    std::string::size_type i = filename.rfind(".");
    if (i != std::string::npos) {
        std::string ext = filename.substr(i, filename.length());
        return StringUtils::toLower(ext);
    }
    return "";
}

bool FileUtils::copyFile(std::string from, std::string to) {
    TRACE("copying from : '%s' to '%s'", from.c_str(), to.c_str());
    if (!FileUtils::fileExists(from)) {
        ERROR("Copy file : Source doesn't exist : %s", from.c_str());
        return false;
    }
    if (FileUtils::fileExists(to)) {
        unlink(to.c_str());
    }
    std::ifstream src(from, std::ios::binary);
    std::ofstream dst(to, std::ios::binary);
    dst << src.rdbuf();
    sync();
    return FileUtils::fileExists(to);
}

bool FileUtils::rmTree(std::string path) {
    TRACE("enter : '%s'", path.c_str());

    DIR *dirp;
    struct stat st;
    struct dirent *dptr;
    std::string filepath;

    if ((dirp = opendir(path.c_str())) == NULL)
        return false;
    if (path[path.length() - 1] != '/')
        path += "/";

    while ((dptr = readdir(dirp))) {
        filepath = dptr->d_name;
        if (filepath == "." || filepath == "..")
            continue;
        filepath = path + filepath;
        int statRet = stat(filepath.c_str(), &st);
        if (statRet == -1)
            continue;
        if (S_ISDIR(st.st_mode)) {
            if (!FileUtils::rmTree(filepath))
                return false;
        } else {
            if (unlink(filepath.c_str()) != 0)
                return false;
        }
    }
    closedir(dirp);
    return rmdir(path.c_str()) == 0;
}

bool FileUtils::fileExists(const std::string &path) {
    TRACE("enter : %s", path.c_str());
    struct stat s;
    // check both that it exists and is a file or a link
    bool result = ((lstat(path.c_str(), &s) == 0) && (S_ISREG(s.st_mode) || S_ISLNK(s.st_mode)));
    TRACE("file '%s' exists : %i", path.c_str(), result);
    return result;
}

bool FileUtils::dirExists(const std::string &path) {
    TRACE("enter : %s", path.c_str());
    struct stat s;
    bool result = (stat(path.c_str(), &s) == 0 && s.st_mode & S_IFDIR); // exists and is dir
    TRACE("dir '%s' exists : %i", path.c_str(), result);
    return result;
}

std::string FileUtils::resolvePath(const std::string &path) {
    char max_path[PATH_MAX];
    std::string outpath;
    errno = 0;
    realpath(path.c_str(), max_path);
    TRACE("in: '%s', resolved: '%s'", path.c_str(), max_path);
    if (errno == ENOENT) {
        TRACE("NOENT came back");
        errno = 0;
        return FileUtils::firstExistingDir(path);
        //return path;
    }
    return (std::string)max_path;
}

// returns the path to a file
std::string FileUtils::dirName(const std::string &path) {
    if (path.empty())
        return "/";
    std::string::size_type p = path.rfind("/");
    if (std::string::npos == p && '.' != path[0]) {
        // no slash and it's not a local path, it's not a dir
        return "";
    }
    if (p == path.size() - 1) {
        // last char is a /, so search for an earlier one
        p = path.rfind("/", p - 1);
    }
    std::string localPath = path.substr(0, p);
    TRACE("local path : '%s'", localPath.c_str());
    return FileUtils::resolvePath(localPath);
}

// returns a file name without any path part
// or the last directory, essentially, anything after the last slash
std::string FileUtils::pathBaseName(const std::string &path) {
    std::string::size_type p = path.rfind("/");
    if (p == path.size() - 1)
        p = path.rfind("/", p - 1);
    return path.substr(p + 1, path.length());
}

std::string FileUtils::firstExistingDir(const std::string &path) {
    TRACE("enter : '%s'", path.c_str());
    if (path.empty() || std::string::npos == path.find("/"))
        return "/";
    if (FileUtils::dirExists(path)) {
        TRACE("raw result is : '%s'", path.c_str());
        std::string result = FileUtils::resolvePath(path);
        TRACE("real path : '%s'", result.c_str());
        return result;
    }
    const std::string &nextPath = FileUtils::dirName(path);
    TRACE("next path : '%s'", nextPath.c_str());
    return FileUtils::firstExistingDir(nextPath);
}


const int FileUtils::processPid(const std::string name) {

    TRACE("enter : '%s'", name.c_str());
    int pid = -1;
    if (name.empty()) {
        return pid;
    }

    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != NULL) {
        // Enumerate all entries in directory until process found
        struct dirent *dirp;
        while (pid < 0 && (dirp = readdir(dp))) {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
            if (id > 0) {
                // Read contents of virtual /proc/{pid}/cmdline file
                std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
                std::ifstream cmdFile(cmdPath.c_str());
                std::string cmdLine;
                std::getline(cmdFile, cmdLine);
                if (!cmdLine.empty()) {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(pos + 1);
                    // Compare against requested process name
                    if (name == cmdLine)
                        pid = id;
                }
            }
        }
    }
    closedir(dp);
    TRACE("exit : %i", pid);
    return pid;
}

const std::string FileUtils::fileReader(std::string path) {
	std::ifstream str(path);
	std::stringstream buf;
	buf << str.rdbuf();
	return buf.str();
}