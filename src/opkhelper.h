#ifndef OPKHELPER_H_
#define OPKHELPER_H_

#include "desktopfile.h"
#include <list>

class OpkHelper {
    private:

    public:

        struct Opk {
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
        };

        OpkHelper();
        OpkHelper(const string & path);
        ~OpkHelper();
        static std::list<OpkHelper::Opk> * ToOpkList(const string & path);
};

#endif