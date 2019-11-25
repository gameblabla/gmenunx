#include "debug.h"
#include "desktopfile.h"
#include "utilities.h"
#include <vector>
#include <unistd.h>
#include <fstream>
#include <sstream>

#define sync() sync(); system("sync");

using std::ifstream;
using std::ofstream;
using std::string;

DesktopFile::DesktopFile() {
    this->reset();
}

DesktopFile::DesktopFile(const string & file) {
    this->reset();
    this->fromFile(file);
}

DesktopFile::~DesktopFile() {
    //TRACE("enter");
}

string DesktopFile::toString() {

    vector<string> vec;

    vec.push_back("# gmenunx X-tended desktop file");
    vec.push_back("# lines starting with a # are ignored");

    vec.push_back(string_format("title=%s", this->title().c_str()));
    vec.push_back(string_format("description=%s", this->description().c_str()));
    vec.push_back(string_format("icon=%s", this->icon().c_str()));
    vec.push_back(string_format("exec=%s", this->exec().c_str()));
    vec.push_back(string_format("params=%s", this->params().c_str()));
    vec.push_back(string_format("selectorDir=%s", this->selectordir().c_str()));
    vec.push_back(string_format("selectorFilter=%s", this->selectorfilter().c_str()));
    vec.push_back(string_format("X-Provider=%s", this->provider().c_str()));
    vec.push_back(string_format("X-ProviderMetadata=%s", this->providerMetadata().c_str()));
    vec.push_back(string_format("consoleapp=%s", this->consoleapp() ? "true" : "false"));

    std::string s;
    for (const auto &piece : vec) s += (piece + "\n");
    return s;

}

bool DesktopFile::fromFile(const string & file) {
    TRACE("enter : %s", file.c_str());
    bool success = false;
    if (fileExists(file)) {
        TRACE("found desktop file");
        this->path_ = file;

		std::ifstream confstream(this->path_.c_str(), std::ios_base::in);
		if (confstream.is_open()) {
			string line;
			while (getline(confstream, line, '\n')) {
				line = trim(line);
                if (0 == line.length()) continue;
                if ('#' == line[0]) continue;
				string::size_type pos = line.find("=");
                if (string::npos == pos) continue;
                
				string name = trim(line.substr(0,pos));
				string value = trim(line.substr(pos+1,line.length()));

                if (0 == value.length()) continue;
                name = toLower(name);
                //TRACE("handling kvp - %s = %s", name.c_str(), value.c_str());

                if (name == "title") {
                    this->title(stripQuotes(value));
                } else if (name == "description") {
                    this->description(stripQuotes(value));
                } else if (name == "icon") {
                    this->icon(stripQuotes(value));
                } else if (name == "exec") {
                    this->exec(stripQuotes(value));
                } else if (name == "params") {
                    this->params(stripQuotes(value));
                } else if (name == "selectordir") {
                    this->selectordir(stripQuotes(value));
                } else if (name == "selectorfilter") {
                    this->selectorfilter(stripQuotes(value));
                } else if (name == "x-provider") {
                    this->provider(stripQuotes(value));
                } else if (name == "x-providermetadata") {
                    this->providerMetadata(stripQuotes(value));
                } else if (name == "consoleapp") {
                    bool console = "false" == stripQuotes(value) ? false : true;
                    this->consoleapp(console);
                } else {
                    WARNING("DesktopFile::fromFile - unknown key : %s", name.c_str());
                }
            };
            confstream.close();
            success = true;
            this->isDirty_ = false;
        }
    } else {
        TRACE("desktopfile doesn't exist : %s", file.c_str());
    }
    TRACE("exit - success: %i", success);
    return success;
}

void DesktopFile::remove() {
    TRACE("enter");
    if (!this->path_.empty()) {
        TRACE("deleting file : %s", this->path_.c_str());
        unlink(this->path_.c_str());
    }
    TRACE("exit");
}

bool DesktopFile::save(const string & path) {
    TRACE("enter : %s", path.c_str());
    if (this->isDirty_) {
        if (!path.empty()) {
            if (fileExists(path)) {
                unlink(path.c_str());
            }
            this->path_ = path;
        }
        TRACE("saving to : %s", this->path_.c_str());
        std::ofstream desktop(this->path_.c_str());
        if (desktop.is_open()) {
            desktop << this->toString();
            desktop.close();
            sync();
            this->isDirty_ = false;
        }
    }
    TRACE("exit");
    return true;
}

void DesktopFile::reset() {
    this->path_ = "";
    this->title_ = "";
    this->description_ = "";
    this->icon_ = "";
    this->exec_ = "";
    this->params_ = "";
    this->selectordir_ = "";
    this->selectorfilter_ = "";
    this->provider_ = "";
    this->providerMetadata_ = "default.gcw0.desktop";
    bool consoleapp_ = false;
    this->isDirty_ = false;
}