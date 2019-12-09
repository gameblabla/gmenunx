#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <string.h>
#include <unordered_map>
#include <cassert>
#include <opk.h>
#include "imageio.h"

#include "opkcache.h"
#include "opkmonitor.h"
#include "debug.h"
#include "utilities.h"
#include "desktopfile.h"
#include "opkhelper.h"
#include "progressbar.h"

OpkCache::OpkCache(std::vector<std::string> opkDirs, const std::string & rootDir) {
    TRACE("enter - root : %s", rootDir.c_str());
    this->opkDirs_ = opkDirs;
    this->rootDir_ = rootDir;
    this->sectionDir_ = this->rootDir_ + "sections";
    this->cacheDir_ = this->rootDir_ + OPK_CACHE_DIR;
    this->sectionCache = nullptr;
    this->loaded_ = false;
    this->notifiable = nullptr;
    TRACE("exit");
}

OpkCache::~OpkCache() {
    TRACE("enter");
    this->stopMonitors();
    TRACE("clearing cache");
    if (this->sectionCache != nullptr) {
        std::unordered_map<std::string, std::list<std::pair<std::string, DesktopFile>>>::iterator cacheIt;
        cacheIt = this->sectionCache->begin();

        while (cacheIt != this->sectionCache->end()) { 
            std::string sectionName = cacheIt->first;
            TRACE("clearing section : %s", sectionName.c_str());
            std::list<std::pair<std::string, DesktopFile>> list = cacheIt->second;
            TRACE("clearing list");

            std::list<std::pair<std::string, DesktopFile>>::iterator fileIt;
            fileIt = cacheIt->second.begin();
            while (fileIt != cacheIt->second.end()) {
                std::string fileName = fileIt->first;
                TRACE("clearing file : %s", fileName.c_str());
                fileIt = cacheIt->second.erase(fileIt);
            }          
            cacheIt = this->sectionCache->erase(cacheIt);
            TRACE("remaining cache size : %zu", this->sectionCache->size());
        }
        this->sectionCache = nullptr;
    }
    TRACE("exit");
}

void OpkCache::startMonitors() {
    TRACE("adding directory watchers");
    for(std::vector<std::string>::iterator it = this->opkDirs_.begin(); it != this->opkDirs_.end(); it++) {
        std::string dir = (*it);
        if (!dirExists(dir))
            continue;

        TRACE("adding monitor for : %s", dir.c_str());

        OpkMonitor *monitor = new OpkMonitor(
            dir, 
            [&](std::string path){ return this->handleNewOpk(path); }, 
            [&](std::string path){ return this->handleRemovedOpk(path); }
        );
        this->directoryMonitors.push_back(monitor);
    }
    TRACE("we're monitoring %zu directories", this->directoryMonitors.size());
}

void OpkCache::stopMonitors() {
    TRACE("enter");
    std::list<OpkMonitor *>::iterator it;
    for (it = this->directoryMonitors.begin(); it != this->directoryMonitors.end(); it++) {
        (*it)->stop();
        delete (*it);
    }
    this->directoryMonitors.clear();
    TRACE("exit");
}

void OpkCache::setMonitorDirs(std::vector<std::string> dirs) {
    TRACE("enter");
    this->stopMonitors();
    this->opkDirs_ = dirs;
    this->startMonitors();
    TRACE("exit");
}

int OpkCache::size() {
    int result = 0;
    std::unordered_map<string, std::list<std::pair<std::string, DesktopFile>>>::iterator it;
    for (it = this->sectionCache->begin(); it != this->sectionCache->end(); it++) {
        result += it->second.size();
    }
    return result;
}

bool OpkCache::update(std::function<void(std::string)> callback) {
    TRACE("enter");
    this->notifiable = callback;
    this->dirty_ = false;

    this->stopMonitors();
    if (!this->ensureCacheDirs())
        return false;

    if (!this->loaded_) {
        TRACE("we need to load the cache");
        if (!loadCache()) return false;
    }
    // add any new ones first, so we can run the upgrade logic on any unlinked ones
    if (!createMissingOpkDesktopFiles()) return false;
    if (!removeUnlinkedDesktopFiles()) return false;
    sync();
    this->startMonitors();
    this->notify("Cache updated");
    TRACE("exit");
    this->notifiable = nullptr;
    return true;

}

const std::string OpkCache::imagesCachePath() {
    return this->cacheDir_ + "/" + OPK_CACHE_IMAGES_DIR;
}
const std::string OpkCache::manualsCachePath() {
    return this->cacheDir_ + "/" + OPK_CACHE_MANUALS_DIR;
}

bool OpkCache::ensureCacheDirs() {
    TRACE("enter");

    this->notify("Checking root directory exists");
    if (!dirExists(this->rootDir_)) {
        if (mkdir(this->rootDir_.c_str(), 0777) == 0) {
            TRACE("created dir : %s", this->rootDir_.c_str());
        } else {
            ERROR("OpkCache::ensureCacheDirs - failed to create root dir : %s", this->rootDir_.c_str());
            return false;
        }
    }
    this->notify("Checking sections directory exists");
    if (!dirExists(this->sectionDir_)) {
        if (mkdir(this->sectionDir_.c_str(), 0777) == 0) {
            TRACE("created dir : %s", this->sectionDir_.c_str());
        } else {
            ERROR("OpkCache::ensureCacheDirs - failed to create root dir : %s", this->sectionDir_.c_str());
            return false;
        }
    }
    this->notify("Checking cache directory exists");
    if (!dirExists(this->cacheDir_)) {
        if (mkdir(this->cacheDir_.c_str(), 0777) == 0) {
            TRACE("created dir : %s", this->cacheDir_.c_str());
        } else {
            ERROR("OpkCache::ensureCacheDirs - failed to create cache dir : %s", this->cacheDir_.c_str());
            return false;
        }
    }
    this->notify("Checking image cache directory exists");
    string imageDir = this->imagesCachePath();
    if (!dirExists(imageDir)) {
        if (mkdir(imageDir.c_str(), 0777) == 0) {
            TRACE("created dir : %s", imageDir.c_str());
        } else {
            ERROR("OpkCache::ensureCacheDirs - failed to create image cache dir : %s", imageDir.c_str());
            return false;
        }
    }

    this->notify("Checking manuals cache directory exists");
    string manualsDir = this->manualsCachePath();
    if (!dirExists(manualsDir)) {
        if (mkdir(manualsDir.c_str(), 0777) == 0) {
            TRACE("created dir : %s", manualsDir.c_str());
        } else {
            ERROR("OpkCache::ensureCacheDirs - failed to create manuals cache dir : %s", manualsDir.c_str());
            return false;
        }
    }
    TRACE("exit");
    return true;
}

std::string OpkCache::hashKey(DesktopFile const & file) {
    return file.providerMetadata() + "-" + file.provider();
}

std::string OpkCache::hashKey(myOpk const & file) {
    return file.metadata() + "-" + file.fullPath();
}

/*
    Reads all existing desktop files, 
    checks for opkrun, 
        if exists, puts desktop file into section catageory <hash map <list>> structure
*/
bool OpkCache::loadCache() {
    TRACE("enter");

    this->notify("Loading cache");
    assert(nullptr == this->sectionCache);
    this->sectionCache = new std::unordered_map<std::string, std::list<std::pair<std::string, DesktopFile>>>();
    assert(this->sectionCache);

    bool success = false;
    if (dirExists(this->sectionDir_)) {
    
        DIR *dirp;
        struct stat st;
        struct dirent *dptr;
        std::string section;
        std::string path = this->sectionDir_;
        if (path[path.length()-1] != '/') path += "/";

        if ((dirp = opendir(path.c_str()) ) != NULL) {
            TRACE("entered section dir : %s", this->sectionDir_.c_str());
            while ((dptr = readdir(dirp))) {
                if (dptr->d_name[0] == '.')
                    continue;
                section = dptr->d_name;
                std::string sectionPath = path + section;
                int statRet = stat(sectionPath.c_str(), &st);
                if (S_ISDIR(st.st_mode)) {
                    TRACE("found section : %s", section.c_str());
                    scanSection(section, sectionPath);
                }
            }
            closedir(dirp);
            success = true;
        }
    }
    this->loaded_ = true;
    this->notify("Cache loaded");
    TRACE("exit - success : %i", success);
    return success;
}

void OpkCache::scanSection(const std::string & sectionName, std::string path) {

    TRACE("enter - scanning section '%s' at : %s", 
        sectionName.c_str(), 
        path.c_str());

	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	std::string filepath;

	if (path[path.length() -1] != '/') path += "/";
	if ((dirp = opendir(path.c_str())) == NULL) return;

	while ((dptr = readdir(dirp))) {
		if (dptr->d_name[0] == '.')
			continue;
		filepath = path + dptr->d_name;
		int statRet = stat(filepath.c_str(), &st);
		if (S_ISDIR(st.st_mode))
			continue;

		if (statRet != -1) {
			TRACE("found desktop file : '%s'", dptr->d_name);
            DesktopFile file;
            if (!file.fromFile(filepath)) {
                ERROR("Couldn't read desktop file : %s", filepath.c_str());
                continue;
            }
            TRACE("hydrated successfully");
            if (OPK_EXEC == file.exec() && !file.provider().empty()) {
                TRACE("it's a valid opk desktop file, let's handle it");
                this->notify("Loading: " + file.title());
                TRACE("adding to cache");
                this->addToCache(sectionName, file);
                TRACE("added to cache");
            }
		}
	}
	closedir(dirp);
    TRACE("exit");
}

void OpkCache::addToCache(const std::string & section, const DesktopFile & file) {
    TRACE("enter - section count for '%s' = %zu", 
        section.c_str(), 
        (*this->sectionCache)[section].size());

    std::string hash = hashKey(file);
    TRACE("desktop hash : %s", hash.c_str());
    std::pair<std::string, DesktopFile> myPair {hash, file};
    (*this->sectionCache)[section].push_back(myPair);

    this->dirty_ = true;

    TRACE("exit - section count for '%s' = %zu", 
        section.c_str(), 
        (*this->sectionCache)[section].size());

}

void OpkCache::removeFromCache(const std::string & section, const DesktopFile & file) {
    TRACE("enter - section count for '%s' = %zu", 
        section.c_str(), 
        (*this->sectionCache)[section].size());

    std::list<std::pair<std::string, DesktopFile>>::iterator fileIt;
    std::string myHash = hashKey(file);
    for (fileIt = (*this->sectionCache)[section].begin(); fileIt != (*this->sectionCache)[section].end(); fileIt++) {
        if (myHash == fileIt->first) {

            TRACE("removing hash : %s", myHash.c_str());

            (*this->sectionCache)[section].erase(fileIt);

            TRACE("exit - section count for '%s' = %zu", 
                section.c_str(), 
                (*this->sectionCache)[section].size());

            this->dirty_ = true;
            return;
        }
    }
    ERROR("Can't find hash key : %s - %s", section.c_str(), myHash.c_str());
}

/*
    utility function to see if another cached desktop file provides the same functionality, 
    based on name and metadata
*/
DesktopFile * OpkCache::findMatchingProvider(const std::string & section, myOpk const & myOpk) {
    TRACE("enter");
    DesktopFile *result = nullptr;
    std::list<std::pair<std::string, DesktopFile>>::iterator fileIt;
    for (fileIt = (*this->sectionCache)[section].begin(); fileIt != (*this->sectionCache)[section].end(); fileIt++) {
        if (fileIt->second.title() == myOpk.name() && fileIt->second.providerMetadata() == myOpk.metadata()) {
            TRACE("found a matching provider");
            result = &fileIt->second;
            break;
        }
    }
    TRACE("exit");
    return result;
}

bool OpkCache::createMissingOpkDesktopFiles() {
    TRACE("enter");
    this->notify("Adding missing files");
    for (std::vector<string>::iterator opkDir = this->opkDirs_.begin(); opkDir != this->opkDirs_.end(); opkDir++) {
        std::string dir = (*opkDir);
        TRACE("checking opk directory : %s", dir.c_str());
        if (!dirExists(dir)) {
            TRACE("skipping non-existing directory : %s", dir.c_str());
            continue;
        }

        this->notify("Adding missing files: " + dir);
        // find the opk files

        DIR *dirp;
        struct dirent *dptr;
        std::vector<std::string> linkfiles;

        dirp = opendir(dir.c_str());
        if (!dirp) {
            // try the next in the list
            continue;
        }
        TRACE("reading directory : %s", dir.c_str());
        while ((dptr = readdir(dirp))) {
            if (dptr->d_type != DT_REG) {
                // skip anything that isn't a file
                continue;
            }

            char *c = strrchr(dptr->d_name, '.');
            if (!c) {
                // no suffix
                continue;
            }

            if (strcasecmp(c + 1, "opk")) {
                // not a opk
                continue;
            }

            if (dptr->d_name[0] == '.') {
                // Ignore hidden files.
                continue;
            }

            TRACE("found opk : %s", dptr->d_name);
            std::string opkPath = dir + "/" + dptr->d_name;
            this->handleNewOpk(opkPath);
        }
        TRACE("closing dir");
        closedir(dirp);
    }
    this->notify("Missing files added");
    TRACE("exit");
    return true;
}

std::string OpkCache::savePng(myOpk const & theOpk) {
    
    std::string imageDir = this->imagesCachePath() + "/" +  fileBaseName(theOpk.fileName());

    TRACE("extracting image to : %s", imageDir.c_str());
    if (!dirExists(imageDir)) {
        if (mkdir(imageDir.c_str(), 0777) == 0) {
            TRACE("created dir : %s", imageDir.c_str());
        } else {
            ERROR("failed to create cache dir : %s", imageDir.c_str());
            return "";
        }
    }

    // extract the image and save it
    std::string shortIconName = theOpk.icon() + ".png";
    std::string opkIconName = theOpk.fullPath() + "#" + shortIconName;
    std::string outFile = imageDir + "/" + base_name(shortIconName);
    std::string result = "";

    SDL_Surface *tmpIcon = loadPNG(opkIconName, true);
    if (tmpIcon) {
        TRACE("loaded opk icon ok");
        TRACE("saving icon to : %s", outFile.c_str());
        if (0 == saveSurfacePng((char*)outFile.c_str(), tmpIcon)) {
            TRACE("saved image");
            result = outFile;
        } else {
            ERROR("OpkCache::savePng - couldn't save icon png");
        }
        SDL_FreeSurface(tmpIcon);
    }
    return result;
}

bool OpkCache::removeUnlinkedDesktopFiles() {
    TRACE("enter");
    this->notify("Removing any deleted files");

    std::list<std::pair<std::string, DesktopFile>> actions;
    std::unordered_map<string, std::list<std::pair<std::string, DesktopFile>>>::iterator section;
    for (section = sectionCache->begin(); section != sectionCache->end(); section++) {
        TRACE("section : %s", section->first.c_str());
        std::list<std::pair<std::string, DesktopFile>> fileList = section->second;
        TRACE("checking %zu files", fileList.size());

        std::list<std::pair<std::string, DesktopFile>>::iterator filePair;
        for (filePair = fileList.begin(); filePair != fileList.end(); filePair++) {
            std::string key = filePair->first;
            DesktopFile file = filePair->second;
            TRACE("checking : '%s' (%s)", file.title().c_str(), key.c_str());
            std::string provider = file.provider();

            // let's also not remove a link if there is no provider, 
            // as it's a manually created file
            if (provider.empty())
                continue;

            // let's not remove anything if the whole dir is missing
            // because it probably means we're external card && unmounted
            if (!dirExists(dir_name(provider)))
                continue;

            if (!fileExists(provider)) {
                TRACE("adding '%s' to action list, because provider doesn't exist", 
                    file.title().c_str());

                actions.push_back(std::pair<std::string, DesktopFile>{ section->first, file });

            }
        }
    }

    TRACE("we have got %zu cache items to remove", actions.size());
    std::list<std::pair<std::string, DesktopFile>>::iterator actionIt;
    for (actionIt = actions.begin(); actionIt != actions.end(); actionIt++) {
        this->notify("Removing: " + actionIt->second.title());
        this->removeFromCache(actionIt->first, actionIt->second);
        TRACE("removed from cache : %s", actionIt->second.title().c_str());
        TRACE("deleting desktop file");
        actionIt->second.remove();
        TRACE("deleted desktop file ok");
    }

    this->notify("Missing files removed");
    TRACE("exit");
    return true;
}

void OpkCache::notify(std::string message) {
    if (nullptr != this->notifiable) {
        try {
            TRACE("notifying listener about : %s", message.c_str());
            this->notifiable(message);
        } catch(...) {
            ERROR("Error trying to notify listeners : %s", message.c_str());
        }
    }
}

/*
entry point for any new opk, from disk scan or inotify etc
*/
void OpkCache::handleNewOpk(const std::string & path) {
    TRACE("enter : %s", path.c_str());

    // we need to 
    // check all the metadata declarations in the opk
    // and extract the section
    // then check if we have an entry for
    // -> section::[metadata-filename]
    // in our cache
    // and if not, process this opk and 
    // - extract image to ./cache/images/basename(filename)/image.jpg
    // - create desktop file

    std::list<myOpk> * opks = OpkHelper::ToOpkList(path);
    if (NULL == opks) {
        TRACE("got null back");
        WARNING("Couldn't read opk : %s", path.c_str());
        return;
    }

    TRACE("got opks : %zu", opks->size());
    std::list<myOpk>::iterator categoryIt;
    for (categoryIt = opks->begin(); categoryIt != opks->end(); categoryIt++) {
        myOpk theOpk = (*categoryIt);
        std::string sectionName = theOpk.category();
        std::string myHash =  hashKey(theOpk);
        TRACE("looking in cache for hash : %s", myHash.c_str());
        //TRACE("cache currently has %zu sections", this->sectionCache->size());
        std::list<std::pair<std::string, DesktopFile>> sectionList = (*this->sectionCache)[sectionName];
        std::list<std::pair<std::string, DesktopFile>>::iterator listIt;
        bool exists = false;
        for (listIt = sectionList.begin(); listIt != sectionList.end(); listIt++) {
            if (myHash == (*listIt).first) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            TRACE("opk doesn't exist in cache");

            this->notify("Adding: " + theOpk.name());

            // now create desktop file paths 
            std::string sectionPath = this->sectionDir_ + "/" + sectionName;
            std::string desktopFilePath = sectionPath + "/" + theOpk.metadata() + "-" + fileBaseName(theOpk.fileName()) + ".desktop";
            DesktopFile *finalFile;

            // first extract the image and get the saved path
            std::string imgFile = this->savePng(theOpk);
            if (imgFile.empty()) {
                continue;
            }

            // manual?
            std::string manualPath = "";
            if (!theOpk.manual().empty()) {

                std::string manualDir = this->manualsCachePath() + "/" +  fileBaseName(theOpk.fileName());
                TRACE("extracting manual to : %s", manualDir.c_str());
                if (!dirExists(manualDir)) {
                    if (mkdir(manualDir.c_str(), 0777) == 0) {
                        TRACE("created dir : %s", manualDir.c_str());
                    } else {
                        ERROR("failed to create cache dir : %s", manualDir.c_str());
                        continue;
                    }
                }
                manualPath = manualDir + "/" + base_name(theOpk.manual());
                void *buffer = NULL;
                std::size_t size = 0;
                std::string path = theOpk.fullPath() + "#" + theOpk.manual();
                OpkHelper::extractFile(path, &buffer, size);
                std::ofstream fout(manualPath);
                fout << (char *)buffer;
            }

            TRACE("checking for upgrade");
            DesktopFile * previous = findMatchingProvider(sectionName, theOpk);
            if (nullptr != previous) {
                TRACE("found another version");
                if (!fileExists(previous->provider())) {
                    // do an upgrade to preserve selector filters, dir etc
                    TRACE("upgrading the provider from : %s to : %s", 
                        previous->provider().c_str(), 
                        theOpk.fullPath().c_str());

                    finalFile = previous->clone();
                    finalFile->path(desktopFilePath);
                    finalFile->provider(theOpk.fullPath());
                    finalFile->manual(manualPath);
                    finalFile->params(theOpk.params());
                    finalFile->icon(imgFile);
                    finalFile->save();
                    this->addToCache(sectionName, *finalFile);
                    continue;
                }
            }
            TRACE("need to create desktop file for : %s", theOpk.name().c_str());

            TRACE("saving desktop file to : %s", desktopFilePath.c_str());
            if (!dirExists(sectionPath)) {
                if (mkdir(sectionPath.c_str(), 0777) == 0) {
                    TRACE("created dir : %s", sectionPath.c_str());
                } else {
                    ERROR("failed to create section dir : %s", sectionPath.c_str());
                    continue;
                }
            }
            finalFile = new DesktopFile();
            finalFile->icon(imgFile);
            finalFile->title(theOpk.name());
            finalFile->exec(OPK_EXEC);
            finalFile->params(theOpk.params());
            finalFile->manual(manualPath);
            finalFile->selectordir(theOpk.selectorDir());
            finalFile->selectorfilter(theOpk.selectorFilter());
            finalFile->consoleapp(theOpk.terminal());
            finalFile->description(theOpk.comment());
            finalFile->provider(path);
            finalFile->providerMetadata(theOpk.metadata());

            // and save it to the right section
            if (finalFile->save(desktopFilePath)) {
                INFO("Saved new desktop file to : %s", desktopFilePath.c_str());
                // and add it to the cache
                this->addToCache(sectionName, *finalFile);
            } else {
                ERROR("Couldn't save desktop file to : %s", desktopFilePath.c_str());
                continue;
            }
        } else {
            TRACE("found in cache");
        }
    }
    TRACE("exit");
    return;
}

void OpkCache::handleRemovedOpk(const std::string path) {
    TRACE("enter : %s", path.c_str());
    // we need to 
    // go through each section
    // - each desktop file
    //   - store any matching providers
    // go through the store doing the remove
    // because we can't modify what we're iterating on
    std::list<std::pair<std::string, DesktopFile>> actions;

    std::unordered_map<std::string, std::list<std::pair<std::string, DesktopFile>>>::iterator sectionIt;
    for (sectionIt = this->sectionCache->begin(); sectionIt != this->sectionCache->end(); sectionIt++) {
        TRACE("checking section '%s' for references to opk : %s", sectionIt->first.c_str(), path.c_str());
        std::list<std::pair<std::string, DesktopFile>>::iterator fileIt;
        for (fileIt = sectionIt->second.begin(); fileIt != sectionIt->second.end(); fileIt++) {
            if (path == fileIt->second.provider()) {
                TRACE("found a matching provider : %s", fileIt->second.title().c_str());
                actions.push_back(std::pair<std::string, DesktopFile>{ sectionIt->first, fileIt->second });
            }
        }
    }
    TRACE("we have got %zu cache items to remove", actions.size());
    std::list<std::pair<std::string, DesktopFile>>::iterator actionIt;
    for (actionIt = actions.begin(); actionIt != actions.end(); actionIt++) {
        this->notify("Removing: " + actionIt->second.title());
        this->removeFromCache(actionIt->first, actionIt->second);
        TRACE("removed from cache : %s", actionIt->second.title().c_str());
        TRACE("deleting desktop file");
        actionIt->second.remove();
        TRACE("deleted desktop file ok");
    }
    TRACE("exit");
}