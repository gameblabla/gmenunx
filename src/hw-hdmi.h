#ifndef _IHDMI_
#define _IHDMI_

#include <string>

#include "fileutils.h"
#include "debug.h"

class IHdmi {

    private:

        bool exists_ = false;
        std::string path_ = "";

    public:

        IHdmi(std::string path) {
            this->path_ = path;
            this->exists_ = FileUtils::fileExists(this->path_);
        };
        bool featureExists() { return this->exists_; };
        bool enabled() {
            if (this->exists_) {
                std::string raw = FileUtils::fileReader(this->path_);
                int intval = atoi(raw.c_str());
                switch (intval) {
                    case 1:
                        return true;
                        break;
                    default:
                        return false;
                        break;
                };
            } return false;
        };
        void set(bool on) {
            if (this->exists_) {
                std::string val = on ? "1" : "0";
                FileUtils::fileWriter(this->path_, val);
            }
        };
};

class DummyHdmi : IHdmi {
    
    public:

        DummyHdmi() : IHdmi("") {
            TRACE("enter");
        }
};

class Rg350Hdmi : IHdmi {

    public:

        Rg350Hdmi() : IHdmi("/sys/class/hdmi/hdmi") {
            TRACE("enter");
        }
};

#endif