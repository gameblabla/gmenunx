#ifndef OPKHELPER_H_
#define OPKHELPER_H_

#include <vector>
#include <list>
#include <array>
#include <opk.h>

#include "desktopfile.h"


class myOpk {
    public:

        myOpk();
        void constrain();
        std::string params();
        bool load(OPK * opk);

        // accessors
        std::string fileName() const;
        void fileName(std::string val);

        std::string fullPath() const;
        void fullPath(std::string val);

        std::string metadata() const;
        void metadata(std::string val);

        std::string category() const;
        void category(std::string val);

        std::string name() const;
        void name(std::string val);

        std::string comment() const;
        void comment(std::string val);

        std::string exec() const;
        void exec(std::string val);

        std::string manual() const;
        void manual(std::string val);

        std::string icon() const;
        void icon(std::string val);

        std::string selectorDir() const;
        void selectorDir(std::string val);

        std::string selectorFilter() const;
        void selectorFilter(std::string val);

        std::string type() const;
        void type(std::string val);

        std::string mimeType() const;
        void mimeType(std::string val);

        std::string version() const;
        void version(std::string val);

        bool needsDownscaling() const;
        void needsDownscaling(bool val);

        bool startupNotify() const;
        void startupNotify(bool val);

        bool terminal() const;
        void terminal(bool val);
        
        bool needsGSensor() const;
        void needsGSensor(bool val);
        
        bool needsJoystick() const;
        void needsJoystick(bool val);

    private:

        const std::vector<std::string> LauncherTokens = { 
            "%f", 
            "%F", 
            "%u", 
            "%U",
        };

        std::string fileName_;
        std::string fullPath_;
        std::string metadata_;
        std::string category_;
        std::string name_;
        std::string comment_;
        std::string exec_;
        std::string manual_;
        std::string icon_;
        std::string selectorDir_;
        std::string selectorFilter_;
        std::string type_;
        std::string mimeType_;
        std::string version_;

        bool needsDownscaling_;
        bool startupNotify_;
        bool terminal_;
        bool needsGSensor_;
        bool needsJoystick_;

};

class OpkHelper {
    private:

    public:
        OpkHelper();
        OpkHelper(const std::string & path);
        ~OpkHelper();
        static std::list<myOpk> * ToOpkList(const std::string & path);

        static int extractFile(const std::string &path, void **buffer, std::size_t &length);

};

#endif