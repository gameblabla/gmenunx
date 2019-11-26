#ifndef OPKCACHE_H_
#define OPKCACHE_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include "desktopfile.h"
#include "opkhelper.h"

using namespace std;

static const string OPK_CACHE_DIR = "cache";
static const string OPK_CACHE_IMAGE_DIR = "images";

#ifdef TARGET_RG350
static const string OPK_EXEC = "/usr/bin/opkrun";
#else
// just for testing, make sure the target test resolves
static const string OPK_EXEC = "/bin/false";
#endif

class OpkCache {
    private:
        vector<string> opkDirs_;
        string sectionDir_;
        string cacheDir_;
        bool loaded_;

        //std::unordered_map<string, std::list<DesktopFile>> * sectionCache;
        // key = sections
        // pair <string = hash, DesktopFile = object>
        std::unordered_map<string, std::list<std::pair<std::string, DesktopFile>>> * sectionCache;

        bool loadCache();
        void scanSection(const string & sectionName, string path);

        void addToCache(const string & section, const DesktopFile & file);
        void removeFromCache(const string & section, const DesktopFile & file);

        string savePng(Opk const & myOpk);

        bool createMissingOpkDesktopFiles();
        bool removeUnlinkedDesktopFiles();
        DesktopFile * findMatchingProvider(const string & section, const Opk & myOpk);

        string hashKey(DesktopFile const & file);
        string hashKey(Opk const & file);
        const string imageCachePath();
        bool ensureCacheDirs();

    public:
        OpkCache(vector<string> opkDirs, const string & sectionsDir);
        ~OpkCache();
        bool update();
};

#endif