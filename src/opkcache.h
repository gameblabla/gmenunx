#ifndef OPKCACHE_H_
#define OPKCACHE_H_

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <list>

#include "desktopfile.h"
#include "opkhelper.h"
#include "opkmonitor.h"

static const std::string OPK_CACHE_DIR = "cache";
static const std::string OPK_CACHE_IMAGES_DIR = "images";
static const std::string OPK_CACHE_MANUALS_DIR = "manuals";

#ifdef TARGET_RG350
static const std::string OPK_EXEC = "/usr/bin/opkrun";
#else
// just for testing, make sure the target test resolves
static const std::string OPK_EXEC = "/bin/false";
#endif

class OpkCache {
    private:

        std::function<void(std::string)> notifiable;
        std::vector<std::string> opkDirs_;
        std::string sectionDir_;
        std::string cacheDir_;
        std::string rootDir_;
        bool loaded_;
        bool dirty_;

        // key = sections
        // pair <string = hash, DesktopFile = object>
        std::unordered_map<std::string, std::list<std::pair<std::string, DesktopFile>>> * sectionCache;

        std::list<OpkMonitor *> directoryMonitors;

        bool loadCache();
        void scanSection(const std::string & sectionName, std::string path);

        void addToCache(const std::string & section, const DesktopFile & file);
        void removeFromCache(const std::string & section, const DesktopFile & file);

        std::string savePng(myOpk const & myOpk);

        bool createMissingOpkDesktopFiles();
        bool removeUnlinkedDesktopFiles();
        void handleNewOpk(const std::string & path);
        void handleRemovedOpk(const std::string path);

        DesktopFile * findMatchingProvider(const std::string & section, myOpk const & myOpk);

        std::string hashKey(DesktopFile const & file);
        std::string hashKey(myOpk const & file);
        const std::string imagesCachePath();
        const std::string manualsCachePath();
        bool ensureCacheDirs();

        void notify(std::string text);

    public:

        OpkCache(std::vector<std::string> opkDirs, const std::string & sectionsDir);
        ~OpkCache();
        bool update(std::function<void(std::string)> callback = nullptr);
        int size();
        /*  returns the cache state, 
            AND resets the flag, 
            so you must handle the state in one read */
        bool isDirty() { 
            bool result = this->dirty_; 
            this->dirty_ = false; 
            return result; 
        }
};

#endif