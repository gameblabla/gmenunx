#ifndef OPKHELPER_H_
#define OPKHELPER_H_

#include "desktopfile.h"
#include <list>

class Opk {
    public:
        string fileName;
        string fullPath;
        string metadata;
        string category;
        string name;
        string comment;
        string exec;
        string manual;
        string icon;
        string selectorDir;
        string selectorFilter;
        string type;
        string mimeType;
        bool needsDownscaling;
        bool startupNotify;
        bool terminal;
        string params() { return "-m " + this->metadata + " \"" + this->fullPath + "\" " + this->exec; };
};

class OpkHelper {
    private:

    public:

        OpkHelper();
        OpkHelper(const string & path);
        ~OpkHelper();
        static std::list<Opk> * ToOpkList(const string & path);
};

#endif