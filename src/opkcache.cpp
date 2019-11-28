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
#include "debug.h"
#include "utilities.h"
#include "desktopfile.h"
#include "opkhelper.h"
#include "progressbar.h"

OpkCache::OpkCache(vector<string> opkDirs, const string & rootDir) {
    TRACE("enter - root : %s", rootDir.c_str());
    this->opkDirs_ = opkDirs;
    this->sectionDir_ = rootDir + "sections";
    this->cacheDir_ = rootDir + OPK_CACHE_DIR;
    this->sectionCache = nullptr;
    this->loaded_ = false;
    this->notifiable = nullptr;
    TRACE("exit");
}

OpkCache::~OpkCache() {
    TRACE("enter");
    if (this->sectionCache != nullptr) {
        delete sectionCache;
    }
    TRACE("exit");
}

bool OpkCache::update(std::function<void(string)> callback) {
    TRACE("enter");
    this->notifiable = callback;

    if (!this->ensureCacheDirs())
        return false;

    if (!this->loaded_) {
        TRACE("we need to load the cache");
        if (!loadCache()) return false;
    }
    // add any new ones first, so we can run the upgrade logic on any unlinked ones
    if (!createMissingOpkDesktopFiles()) return false;
    if (!removeUnlinkedDesktopFiles()) return false;
    this->notify("Cache updated");

    TRACE("exit");
    return true;

}

const string OpkCache::imageCachePath() {
    return this->cacheDir_ + "/" + OPK_CACHE_IMAGE_DIR;
}

bool OpkCache::ensureCacheDirs() {
    TRACE("enter");

    if (!dirExists(this->cacheDir_)) {
        if (mkdir(this->cacheDir_.c_str(), 0777) == 0) {
            TRACE("created dir : %s", this->cacheDir_.c_str());
        } else {
            ERROR("OpkCache::ensureCacheDirs - failed to create cache dir : %s", this->cacheDir_.c_str());
            return false;
        }
    }
    string imageDir = this->imageCachePath();
    if (!dirExists(imageDir)) {
        if (mkdir(imageDir.c_str(), 0777) == 0) {
            TRACE("created dir : %s", imageDir.c_str());
        } else {
            ERROR("OpkCache::ensureCacheDirs - failed to create image cache dir : %s", imageDir.c_str());
            return false;
        }
    }

    TRACE("exit");
    return true;
}

string OpkCache::hashKey(DesktopFile const & file) {
    return file.providerMetadata() + "-" + file.provider();
}

string OpkCache::hashKey(myOpk const & file) {
    return file.metadata() + "-" + file.fullPath();
}

/*
    Reads all existing desktop files, 
    checks for opkrun, 
        if exists, puts desktop file into section catageory <hash map <list>> structure
*/
bool OpkCache::loadCache() {
    TRACE("enter");

    assert(nullptr == this->sectionCache);
    this->sectionCache = new std::unordered_map<string, std::list<std::pair<std::string, DesktopFile>>>();
    assert(this->sectionCache);

    bool success = false;
    if (dirExists(this->sectionDir_)) {
    
        DIR *dirp;
        struct stat st;
        struct dirent *dptr;
        string section;
        string path = this->sectionDir_;
        if (path[path.length()-1] != '/') path += "/";

        if ((dirp = opendir(path.c_str()) ) != NULL) {
            TRACE("entered section dir : %s", this->sectionDir_.c_str());
            while ((dptr = readdir(dirp))) {
                if (dptr->d_name[0] == '.')
                    continue;
                section = dptr->d_name;
                string sectionPath = path + section;
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
    TRACE("exit - success : %i", success);
    return success;
}

void OpkCache::scanSection(const string & sectionName, string path) {

    TRACE("enter - scanning section '%s' at : %s", 
        sectionName.c_str(), 
        path.c_str());

	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath;

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
            DesktopFile file(filepath);
            if (OPK_EXEC == file.exec() && !file.provider().empty()) {
                TRACE("it's an opk desktop file");
                this->notify("loading: " + file.title());

                this->addToCache(sectionName, file);

            }
		}
	}
	closedir(dirp);
    TRACE("exit");
}

void OpkCache::addToCache(const string & section, const DesktopFile & file) {
    TRACE("enter - section count for '%s' = %zu", 
        section.c_str(), 
        (*this->sectionCache)[section].size());

    string hash = hashKey(file);
    TRACE("desktop hash : %s", hash.c_str());
    std::pair<std::string, DesktopFile> myPair {hash, file};
    (*this->sectionCache)[section].push_back(myPair);

    TRACE("exit - section count for '%s' = %zu", 
        section.c_str(), 
        (*this->sectionCache)[section].size());

}

void OpkCache::removeFromCache(const string & section, const DesktopFile & file) {
    TRACE("enter - section count for '%s' = %zu", 
        section.c_str(), 
        (*this->sectionCache)[section].size());

    list<std::pair<std::string, DesktopFile>>::iterator fileIt;
    string myHash = hashKey(file);
    for (fileIt = (*this->sectionCache)[section].begin(); fileIt != (*this->sectionCache)[section].end(); fileIt++) {
        if (myHash == fileIt->first) {

            TRACE("removing hash : %s", myHash.c_str());

            (*this->sectionCache)[section].erase(fileIt);

            TRACE("exit - section count for '%s' = %zu", 
                section.c_str(), 
                (*this->sectionCache)[section].size());

            return;
        }
    }
    ERROR("Can't find hash key : %s - %s", section.c_str(), myHash.c_str());
}

/*
    utility function to see if another cached desktop file provides the same functionality, 
    based on name and metadata
*/
DesktopFile * OpkCache::findMatchingProvider(const string & section, myOpk const & myOpk) {
    TRACE("enter");
    DesktopFile *result = nullptr;
    list<std::pair<std::string, DesktopFile>>::iterator fileIt;
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

    for (std::vector<string>::iterator opkDir = this->opkDirs_.begin(); opkDir != this->opkDirs_.end(); opkDir++) {
        string dir = (*opkDir);
        TRACE("checking opk directory : %s", dir.c_str());
        if (!dirExists(dir)) {
            TRACE("skipping non-existing directory : %s", dir.c_str());
            continue;
        }
        // find the opk files

        DIR *dirp;
        struct dirent *dptr;
        vector<string> linkfiles;

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
            string opkPath = dir + "/" + dptr->d_name;
            // we need to 
            // check all the metadata declarations in the opk
            // and extract the section
            // then check if we have an entry for
            // -> section::[metadata-filename]
            // in our cache
            // and if not, process this opk and 
            // - extract image to ./cache/images/basename(filename)/image.jpg
            // - create desktop file

            list<myOpk> * opks = OpkHelper::ToOpkList(opkPath);
            if (NULL == opks) {
                TRACE("got null back");
                WARNING("got null back");
                continue;
            }

            TRACE("got opks : %zu", opks->size());
            std::list<myOpk>::iterator categoryIt;
            for (categoryIt = opks->begin(); categoryIt != opks->end(); categoryIt++) {
                myOpk theOpk = (*categoryIt);
                string sectionName = theOpk.category();
                string myHash =  hashKey(theOpk);
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
                    this->notify("adding: " + theOpk.name());

                    // now create desktop file paths 
                    string sectionPath = this->sectionDir_ + "/" + sectionName;
                    string desktopFilePath = sectionPath + "/" + theOpk.metadata() + "-" + fileBaseName(theOpk.fileName()) + ".desktop";
                    DesktopFile *finalFile;

                    // first extract the image and get the saved path
                    string imgFile = this->savePng(theOpk);
                    if (imgFile.empty())
                        continue;

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
                    finalFile->selectordir(theOpk.selectorDir());
                    finalFile->selectorfilter(theOpk.selectorFilter());
                    finalFile->consoleapp(theOpk.terminal());
                    finalFile->description(theOpk.comment());
                    finalFile->provider(opkPath);
                    finalFile->providerMetadata(theOpk.metadata());

                    // and save it to the right section
                    if (finalFile->save(desktopFilePath)) {
                        INFO("Saved new desktop file to : %s", desktopFilePath.c_str());
                    } else {
                        ERROR("Couldn't save desktop file to : %s", desktopFilePath.c_str());
                    }
                    // and add it to the cache
                    this->addToCache(sectionName, *finalFile);
                } else {
                    TRACE("found in cache");
                }

            }
        }
        TRACE("closing dir");
        closedir(dirp);
    }

    TRACE("exit");
    return true;
}

string OpkCache::savePng(myOpk const & theOpk) {
    
    string imagePath = this->imageCachePath() + "/" +  fileBaseName(theOpk.fileName());
    if (fileExists(imagePath)) {
        TRACE("image already exists : %s", imagePath.c_str());
        return imagePath;
    }
    TRACE("extracting image to : %s", imagePath.c_str());
    if (!dirExists(imagePath)) {
        if (mkdir(imagePath.c_str(), 0777) == 0) {
            TRACE("created dir : %s", imagePath.c_str());
        } else {
            ERROR("failed to create cache dir : %s", imagePath.c_str());
            return "";
        }
    }

    // extract the image and save it
    string shortIconName = theOpk.icon() + ".png";
    string opkIconName = theOpk.fullPath() + "#" + shortIconName;
    string outFile = imagePath + "/" + base_name(shortIconName);

    SDL_Surface *tmpIcon = loadPNG(opkIconName, true);
    if (tmpIcon) {
        TRACE("loaded opk icon ok");
        TRACE("saving icon to : %s", outFile.c_str());
        if (0 == saveSurfacePng((char*)outFile.c_str(), tmpIcon)) {
            TRACE("saved image");
            return outFile;
        } else {
            ERROR("OpkCache::savePng - couldn't save icon png");
        }
    }
    return "";
}

bool OpkCache::removeUnlinkedDesktopFiles() {
    TRACE("enter");

    std::unordered_map<string, std::list<std::pair<std::string, DesktopFile>>>::iterator section;
    for (section = sectionCache->begin(); section != sectionCache->end(); section++) {
        TRACE("section : %s", section->first.c_str());
        list<std::pair<std::string, DesktopFile>> fileList = section->second;
        TRACE("checking %zu files", fileList.size());

        list<std::pair<std::string, DesktopFile>>::iterator filePair;
        for (filePair = fileList.begin(); filePair != fileList.end(); filePair++) {
            string key = filePair->first;
            DesktopFile file = filePair->second;
            TRACE("checking : '%s' (%s)", file.title().c_str(), key.c_str());
            string provider = file.provider();
            if (provider.empty())
                continue;
            if (!fileExists(provider)) {
                TRACE("removing '%s' because provider doesn't exist", 
                    file.title().c_str());

                this->notify("removing: " + file.title());

                // TODO :: maybe clean up images as well??

                file.remove();
                // pop from the cache
                this->removeFromCache(section->first, file);
            }
        }
    }
    TRACE("exit");
    return true;
}

void OpkCache::notify(std::string message) {
    if (nullptr != this->notifiable) {
        this->notifiable(message);
    }
}
