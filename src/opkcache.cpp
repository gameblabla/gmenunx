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

OpkCache::OpkCache(vector<string> opkDirs, const string & rootDir) {
    TRACE("OpkCache::OpkCache - enter - root : %s", rootDir.c_str());
    this->opkDirs_ = opkDirs;
    this->sectionDir_ = rootDir + "sections";
    this->cacheDir_ = rootDir + OPK_CACHE_DIR;
    this->sectionCache = nullptr;
    TRACE("OpkCache::OpkCache - exit");
}

OpkCache::~OpkCache() {
    TRACE("OpkCache::~OpkCache - enter");
    if (this->sectionCache != nullptr) {
        delete sectionCache;
    }
    TRACE("OpkCache::~OpkCache - exit");
}

bool OpkCache::update() {
    TRACE("OpkCache::update - enter");

    if (!this->ensureCacheDirs())
        return false;

    if (this->sectionCache != nullptr) {
        TRACE("OpkCache::update - deleteing previous cache");
        this->sectionCache->clear();
        delete this->sectionCache;
        TRACE("OpkCache::update - deleted previous cache");
    }
    this->sectionCache = new std::unordered_map<string, std::list<std::pair<std::string, DesktopFile>>>();

    assert(this->sectionCache);

    if (!loadCache()) return false;
    if (!removeUnlinkedDesktopFiles()) return false;
    if (!createMissingOpkDesktopFiles()) return false;
    TRACE("OpkCache::update - exit");
    return true;

}

const string OpkCache::imageCachePath() {
    return this->cacheDir_ + "/" + OPK_CACHE_IMAGE_DIR;
}

bool OpkCache::ensureCacheDirs() {
    TRACE("OpkCache::ensureCacheDirs - enter");

    if (!dirExists(this->cacheDir_)) {
        if (mkdir(this->cacheDir_.c_str(), 0777) == 0) {
            TRACE("OpkCache::ensureCacheDirs - created dir : %s", this->cacheDir_.c_str());
        } else {
            ERROR("OpkCache::ensureCacheDirs - failed to create cache dir : %s", this->cacheDir_.c_str());
            return false;
        }
    }
    string imageDir = this->imageCachePath();
    if (!dirExists(imageDir)) {
        if (mkdir(imageDir.c_str(), 0777) == 0) {
            TRACE("OpkCache::ensureCacheDirs - created dir : %s", imageDir.c_str());
        } else {
            ERROR("OpkCache::ensureCacheDirs - failed to create image cache dir : %s", imageDir.c_str());
            return false;
        }
    }

    TRACE("OpkCache::ensureCacheDirs - exit");
    return true;
}

string OpkCache::hashKey(DesktopFile const & file) {
    return file.providerMetadata() + "-" + file.provider();
}

/*
    Reads all existing desktop files, 
    checks for opkrun, 
        if exists, puts desktop file into section catageory <hash map <list>> structure
*/
bool OpkCache::loadCache() {
    TRACE("OpkCache::loadCache - enter");
    bool success = false;
    if (dirExists(this->sectionDir_)) {
    
        DIR *dirp;
        struct stat st;
        struct dirent *dptr;
        string section;
        string path = this->sectionDir_;
        if (path[path.length()-1]!='/') path += "/";

        if ((dirp = opendir(path.c_str()) ) != NULL) {
            TRACE("OpkCache::loadCache - entered section dir : %s", this->sectionDir_.c_str());
            while ((dptr = readdir(dirp))) {
                if (dptr->d_name[0] == '.')
                    continue;
                section = dptr->d_name;
                string sectionPath = path + section;
                int statRet = stat(sectionPath.c_str(), &st);
                if (S_ISDIR(st.st_mode)) {
                    TRACE("OpkCache::loadCache - found section : %s", section.c_str());
                    scanSection(section, sectionPath);
                }
            }
            closedir(dirp);
            success = true;
        }
    }

    TRACE("OpkCache::loadCache - exit - success : %i", success);
    return success;
}

void OpkCache::scanSection(const string & sectionName, string path) {

    TRACE("OpkCache::scanSection - enter - scanning section '%s' at : %s", 
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
			TRACE("OpkCache::scanSection - found desktop file : '%s'", dptr->d_name);
            DesktopFile file(filepath);
            if (OPK_EXEC == file.exec() && !file.provider().empty()) {
                TRACE("OpkCache::scanSection - it's an opk desktop file");
                //TRACE("DUMP ::\n %s", file.toString().c_str());
                string hash = hashKey(file);
                TRACE("OpkCache::scanSection - opk hash : %s", hash.c_str());
                std::pair<std::string, DesktopFile> myPair {hash, file};
                (*this->sectionCache)[sectionName].push_back(myPair);
                TRACE("OpkCache::scanSection :: section count for '%s' = %zu", 
                    sectionName.c_str(), 
                    (*this->sectionCache)[sectionName].size());
            }
		}
	}
	closedir(dirp);
    TRACE("OpkCache::scanSection - exit");
}

bool OpkCache::createMissingOpkDesktopFiles() {
    TRACE("OpkCache::createMissingOpkDesktopFiles - enter");

    for (std::vector<string>::iterator opkDir = this->opkDirs_.begin(); opkDir != this->opkDirs_.end(); opkDir++) {
        string dir = (*opkDir);
        TRACE("OpkCache::createMissingOpkDesktopFiles - checking opk directory : %s", dir.c_str());
        if (!dirExists(dir)) {
            TRACE("OpkCache::createMissingOpkDesktopFiles - skipping non-existing directory : %s", dir.c_str());
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
        TRACE("OpkCache::createMissingOpkDesktopFiles - reading directory : %s", dir.c_str());
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

            TRACE("OpkCache::createMissingOpkDesktopFiles - found opk : %s", dptr->d_name);
            string opkPath = dir + dptr->d_name;
            // we need to 
            // check all the metadata declarations in the opk
            // and extract the section
            // then check if we have an entry for
            // -> section::[metadata-filename]
            // in our cache
            // and if not, process this opk and 
            // - extract image to ./cache/images/basename(filename)/image.jpg
            // - create desktop file

            list<OpkHelper::Opk> * opks = OpkHelper::ToOpkList(opkPath);
            if (NULL == opks) {
                WARNING("OpkCache::createMissingOpkDesktopFiles - got null back");
                continue;
            }

            TRACE("OpkCache::createMissingOpkDesktopFiles - got opks : %zu", opks->size());
            std::list<OpkHelper::Opk>::iterator categoryIt;
            for (categoryIt = opks->begin(); categoryIt != opks->end(); categoryIt++) {
                OpkHelper::Opk myOpk = (*categoryIt);
                string sectionName = myOpk.category;
                string myHash = myOpk.metadata + "-" + opkPath;
                TRACE("OpkCache::createMissingOpkDesktopFiles - looking in cache for hash : %s", myHash.c_str());
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
                    TRACE("OpkCache::createMissingOpkDesktopFiles - need to create desktop file for : %s",
                         myOpk.name.c_str());

                    string imagePath = this->imageCachePath() + "/" +  fileBaseName(myHash);
                    TRACE("OpkCache::createMissingOpkDesktopFiles - extracting image to : %s", imagePath.c_str());

                    if (!dirExists(imagePath)) {
                        if (mkdir(imagePath.c_str(), 0777) == 0) {
                            TRACE("OpkCache::createMissingOpkDesktopFiles - created dir : %s", imagePath.c_str());
                        } else {
                            ERROR("failed to create cache dir : %s", imagePath.c_str());
                            return false;
                        }
                    }

                    // extract he image and save it
                    string shortIconName = myOpk.icon + ".png";
                    string opkIconName = opkPath + "#" + shortIconName;
                    string outFile = imagePath + "/" + base_name(shortIconName);

                    SDL_Surface *tmpIcon = loadPNG(opkIconName, true);
                    if (tmpIcon) {
                        TRACE("OpkCache::createMissingOpkDesktopFiles - loaded opk icon ok");
                        TRACE("OpkCache::createMissingOpkDesktopFiles - saving icon to : %s", outFile.c_str());
                        if (0 == saveSurfacePng((char*)outFile.c_str(), tmpIcon)) {
                            TRACE("OpkCache::createMissingOpkDesktopFiles - saved image");
                        } else {
                            ERROR("OpkCache::createMissingOpkDesktopFiles - couldn't save icon png");
                            continue;
                        }
                    }

                    // now create a desktop file, referencing the image path
                    string desktopSectionPath = this->sectionDir_ + "/" + sectionName;
                    string desktopFilePath =desktopSectionPath + "/" + myOpk.name + ".desktop";
                    TRACE("OpkCache::createMissingOpkDesktopFiles - saving desktop file to : %s", desktopFilePath.c_str());
                    
                    if (!dirExists(desktopSectionPath)) {
                        if (mkdir(desktopSectionPath.c_str(), 0777) == 0) {
                            TRACE("OpkCache::createMissingOpkDesktopFiles - created dir : %s", desktopSectionPath.c_str());
                        } else {
                            ERROR("failed to create section dir : %s", desktopSectionPath.c_str());
                            continue;
                        }
                    }
                    DesktopFile *finalFile = new DesktopFile();
                    finalFile->icon(outFile);
                    finalFile->title(myOpk.name);
                    finalFile->exec(OPK_EXEC);
                    finalFile->params("-m " + myOpk.metadata + " \"" + opkPath + "\" " + myOpk.exec);
                    finalFile->selectordir(myOpk.selectorDir);
                    finalFile->selectorfilter(myOpk.selectorFilter);
                    finalFile->consoleapp(myOpk.terminal);
                    finalFile->description(myOpk.comment);
                    finalFile->provider(opkPath);
                    finalFile->providerMetadata(myOpk.metadata);

                    // and save it to the right section
                    if (finalFile->save(desktopFilePath)) {
                        INFO("Saved new desktop file to : %s", desktopFilePath.c_str());
                    } else {
                        ERROR("Couldn't save desktop file to : %s", desktopFilePath.c_str());
                    }
                }

            }
        }
        TRACE("OpkCache::createMissingOpkDesktopFiles - closing dir");
        closedir(dirp);
    }

    TRACE("OpkCache::createMissingOpkDesktopFiles - exit");
    return true;
}

bool OpkCache::removeUnlinkedDesktopFiles() {
    TRACE("OpkCache::removeUnlinkedDesktopFiles - enter");

    std::unordered_map<string, std::list<std::pair<std::string, DesktopFile>>>::iterator section;
    for (section = sectionCache->begin(); section != sectionCache->end(); section++) {
        TRACE("OpkCache::removeUnlinkedDesktopFiles - section : %s", section->first.c_str());
        list<std::pair<std::string, DesktopFile>> fileList = section->second;
        TRACE("OpkCache::removeUnlinkedDesktopFiles - checking %zu files", fileList.size());

        list<std::pair<std::string, DesktopFile>>::iterator filePair;
        for (filePair = fileList.begin(); filePair != fileList.end(); filePair++) {
            string key = filePair->first;
            DesktopFile file = filePair->second;
            TRACE("OpkCache::removeUnlinkedDesktopFiles - checking : '%s' (%s)", file.title().c_str(), key.c_str());
            string provider = file.provider();
            if (provider.empty())
                continue;
            if (!fileExists(provider)) {
                TRACE("OpkCache::removeUnlinkedDesktopFiles - removing '%s' because provider doesn't exist", 
                    file.title().c_str());
                // TODO :: maybe clean up images as well??
                // TODO :: maybe remove from the list too?
                file.remove();
            }
        }
    }
    TRACE("OpkCache::removeUnlinkedDesktopFiles - exit");
    return true;
}
