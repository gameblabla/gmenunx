#include "constants.h"
#include "debug.h"
#include "desktopfile.h"
#include "utilities.h"
#include <vector>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <istream>
#include <stdlib.h>

#define sync() sync(); system("sync");

DesktopFile::DesktopFile() {
    this->reset();
}

DesktopFile::DesktopFile(const std::string & file) {
    this->reset();
    this->fromFile(file);
}

DesktopFile::~DesktopFile() {
    //TRACE("enter");
}

DesktopFile * DesktopFile::clone() {
    DesktopFile *result = new DesktopFile();
    std::stringstream ss;
    ss.str (this->toString());
    result->parse(ss);
    return result;
}

std::string DesktopFile::toString() {

    std::vector<std::string> vec;

    vec.push_back("# " + APP_NAME + " X-tended desktop file");
    vec.push_back("# lines starting with a # are ignored");
    vec.push_back("");

    vec.push_back(string_format("title=%s", this->title().c_str()));
    vec.push_back(string_format("description=%s", this->description().c_str()));
    vec.push_back(string_format("icon=%s", this->icon().c_str()));
    vec.push_back(string_format("exec=%s", this->exec().c_str()));
    vec.push_back(string_format("params=%s", this->params().c_str()));
    vec.push_back(string_format("selectorDir=%s", this->selectorDir().c_str()));
    vec.push_back(string_format("selectorFilter=%s", this->selectorFilter().c_str()));
    vec.push_back(string_format("selectorAlias=%s", this->selectorAlias().c_str()));
    vec.push_back(string_format("manual=%s", this->manual().c_str()));
    vec.push_back(string_format("workdir=%s", this->workdir().c_str()));
    vec.push_back(string_format("X-Provider=%s", this->provider().c_str()));
    vec.push_back(string_format("X-ProviderMetadata=%s", this->providerMetadata().c_str()));
    vec.push_back(string_format("consoleapp=%s", this->consoleapp() ? "true" : "false"));

    std::string s;
    for (const auto &piece : vec) s += (piece + "\n");
    return s;

}

void DesktopFile::parse(std::istream & instream) {
    TRACE("enter");
	std::string line;
	while (std::getline(instream, line, '\n')) {
		line = trim(line);
        if (0 == line.length()) continue;
        if ('#' == line[0]) continue;
		std::string::size_type pos = line.find("=");
        if (std::string::npos == pos) continue;

		std::string name = trim(line.substr(0,pos));
		std::string value = trim(line.substr(pos+1,line.length()));

        if (0 == value.length()) continue;
        name = toLower(name);
        TRACE("handling kvp - %s = %s", name.c_str(), value.c_str());

            try {
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
                this->selectorDir(stripQuotes(value));
            } else if (name == "selectorfilter") {
                this->selectorFilter(stripQuotes(value));
            } else if (name == "selectoralias") {
                this->selectorAlias(stripQuotes(value));
            } else if (name == "x-provider") {
                this->provider(stripQuotes(value));
            } else if (name == "x-providermetadata") {
                this->providerMetadata(stripQuotes(value));
            } else if (name == "consoleapp") {
                bool console = "false" == stripQuotes(value) ? false : true;
                this->consoleapp(console);
            } else if (name == "selectorbrowser") {
                // eat it
            } else if (name == "manual") {
                this->manual(stripQuotes(value));
            } else if (name == "workdir") {
                this->workdir(stripQuotes(value));
            } else {
                WARNING("unknown .desktop key : %s", name.c_str());
            }
        } 
        catch (int param) { 
            ERROR("int exception : %i from <%s, %s>", 
                param, name.c_str(), value.c_str()); }
        catch (char *param) { 
            ERROR("char exception : %s from <%s, %s>", 
                param, name.c_str(), value.c_str()); }
        catch (...) { 
            ERROR("unknown error reading value from <%s, %s>", 
                name.c_str(), value.c_str());
        }
    };
}

bool DesktopFile::fromFile(const std::string & file) {
    TRACE("enter : %s", file.c_str());
    bool success = false;
    if (fileExists(file)) {
        TRACE("found desktop file");
        this->path_ = file;

		std::ifstream confstream(this->path_.c_str(), std::ios_base::in);
        std::locale loc("");
        confstream.imbue(loc);
		if (confstream.is_open()) {
            TRACE("parsing the stream");
			this->parse(confstream);
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

bool DesktopFile::save(const std::string & path) {
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
    this->selectorDir_ = "";
    this->selectorFilter_ = "";
    this->selectorAlias_ = "";
    this->provider_ = "";
    this->providerMetadata_ = "default.gcw0.desktop";
    this->manual_ = "";
    this->workdir_ = "";
    this->consoleapp_ = false;
    this->isDirty_ = false;
}