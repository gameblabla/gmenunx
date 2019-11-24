#ifndef OPKCACHE_H_
#define OPKCACHE_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include "desktopfile.h"

using namespace std;

static const string OPK_CACHE_DIR = "cache";
static const string OPK_CACHE_IMAGE_DIR = "images";
static const string OPK_EXEC = "/usr/bin/opkrun";

class OpkCache {
    private:
        vector<string> opkDirs_;
        string sectionDir_;
        string cacheDir_;

        //std::unordered_map<string, std::list<DesktopFile>> * sectionCache;
        // key = sections
        // pair <string = hash, DesktopFile = object>
        std::unordered_map<string, std::list<std::pair<std::string, DesktopFile>>> * sectionCache;

        bool loadCache();
        void scanSection(const string & sectionName, string path);
        bool createMissingOpkDesktopFiles();
        bool removeUnlinkedDesktopFiles();
        string hashKey(DesktopFile const & file);
        const string imageCachePath();
        bool ensureCacheDirs();

    public:
        OpkCache(vector<string> opkDirs, const string & sectionsDir);
        ~OpkCache();
        bool update();
};

#endif