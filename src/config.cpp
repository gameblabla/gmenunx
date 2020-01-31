#include <memory>
#include <stdarg.h>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <dirent.h>
#include <unistd.h>

#include "debug.h"
#include "config.h"
#include "utilities.h"
#include "stringutils.h"

#define sync() sync(); std::system("sync");

Config::Config(std::string const &prefix) {
    TRACE("enter - prefix : %s", prefix.c_str());
    this->prefix = prefix;
    this->isDirty = false;
}

Config::~Config() {
    TRACE("~Config");
}

std::string Config::toString() {

    std::vector<std::string> vec;

    vec.push_back("# " + APP_NAME + " config file");
    vec.push_back("# lines starting with a # are ignored");
    vec.push_back("");

    // strings
    vec.push_back(StringUtils::stringFormat("skin=\"%s\"", this->skin().c_str()));
    vec.push_back(StringUtils::stringFormat("tvOutMode=\"%s\"", this->tvOutMode().c_str()));
    vec.push_back(StringUtils::stringFormat("lang=\"%s\"", this->lang().c_str()));
    vec.push_back(StringUtils::stringFormat("sectionFilter=\"%s\"", this->sectionFilter().c_str()));
    vec.push_back(StringUtils::stringFormat("launcherPath=\"%s\"", this->launcherPath().c_str()));
    vec.push_back(StringUtils::stringFormat("externalAppPath=\"%s\"", this->externalAppPath().c_str()));
    vec.push_back(StringUtils::stringFormat("cpuMenu=\"%s\"", this->cpuMenu().c_str()));
    vec.push_back(StringUtils::stringFormat("defaultCpuSpeed=\"%s\"", this->defaultCpuSpeed().c_str()));

    // ints
    vec.push_back(StringUtils::stringFormat("buttonRepeatRate=%i", this->buttonRepeatRate()));
    vec.push_back(StringUtils::stringFormat("resolutionX=%i", this->resolutionX()));
    vec.push_back(StringUtils::stringFormat("resolutionY=%i", this->resolutionY()));
    vec.push_back(StringUtils::stringFormat("videoBpp=%i", this->videoBpp()));

    vec.push_back(StringUtils::stringFormat("backlightLevel=%i", this->backlightLevel()));
    vec.push_back(StringUtils::stringFormat("backlightTimeout=%i", this->backlightTimeout()));
    vec.push_back(StringUtils::stringFormat("powerTimeout=%i", this->powerTimeout()));

    vec.push_back(StringUtils::stringFormat("globalVolume=%i", this->globalVolume()));
    vec.push_back(StringUtils::stringFormat("aspectRatio=%i", this->aspectRatio()));
    vec.push_back(StringUtils::stringFormat("outputLogs=%i", this->outputLogs()));

    vec.push_back(StringUtils::stringFormat("saveSelection=%i", this->saveSelection()));
    vec.push_back(StringUtils::stringFormat("section=%i", this->section()));
    vec.push_back(StringUtils::stringFormat("link=%i", this->link()));

    vec.push_back(StringUtils::stringFormat("respectHiddenLinks=%i", this->respectHiddenLinks()));
    vec.push_back(StringUtils::stringFormat("setHwLevelsOnBoot=%i", this->setHwLevelsOnBoot()));

    vec.push_back(StringUtils::stringFormat("version=%i", this->version()));
    
    std::string s;
    for (const auto &piece : vec) s += (piece + "\n");
    return s;
}

bool Config::save() {
    TRACE("enter");
    if (this->isDirty) {
        std::string fileName = this->prefix + CONFIG_FILE_NAME;
        TRACE("saving to : %s", fileName.c_str());
        std::ofstream config(fileName.c_str());
        if (config.is_open()) {
            config << this->toString();
            config.close();
            sync();
            this->isDirty = false;
        }
    }
    TRACE("exit");
    return true;
}

bool Config::loadConfig() {
    TRACE("enter");
    this->reset();
    if (this->fromFile()) {
        this->constrain();
        isDirty = false;
        return true;
    }
    return false;
}

/* Private methods */

void Config::reset() {
    TRACE("enter");

     //strings
    this->externalAppPath_ = APP_EXTERNAL_PATH;
    this->skin_ = "Default";
    this->tvOutMode_ = "NTSC";
    this->lang_ = "";
    this->sectionFilter_ = "";
    this->cpuMenu_ = "";
    this->defaultCpuSpeed_ = "";

    if (FileUtils::dirExists(EXTERNAL_LAUNCHER_PATH)) {
        this->launcherPath(EXTERNAL_LAUNCHER_PATH);
    } else this->launcherPath(HOME_DIR);

    // ints
    this->buttonRepeatRate_ = 10;
    this->resolutionX_ = 320;
    this->resolutionY_ = 240;
    this->videoBpp_ = 32;

    this->powerTimeout_ = 10;
    this->backlightTimeout_ = 30;
    this->backlightLevel_ = 70;

    this->globalVolume_ = 60;
    this->aspectRatio_ = 1;
    this->outputLogs_ = 0;

    this->saveSelection_ = 1;
    this->section_ = 1;
    this->link_ = 1;

    this->setHwLevelsOnBoot_ = 0;
    this->respectHiddenLinks_ = true;
    this->version_ = CONFIG_CURRENT_VERSION;

    TRACE("exit");
    return;
}

void Config::constrain() {

	evalIntConf( &this->backlightTimeout_, 30, 10, 300);
    evalIntConf( &this->backlightLevel_, 70, 1, 100);
    evalIntConf( &this->buttonRepeatRate_, 50, 0, 500);
	evalIntConf( &this->powerTimeout_, 10, 0, 300);
	evalIntConf( &this->outputLogs_, 0, 0, 1 );
	evalIntConf( &this->globalVolume_, 60, 1, 100 );
    evalIntConf (&this->aspectRatio_, 1, 0, 1);
	evalIntConf( &this->videoBpp_, 16, 8, 32 );
    evalIntConf( &this->respectHiddenLinks_, 1, 0, 1);
    evalIntConf( &this->setHwLevelsOnBoot_, 0, 0, 1);
	evalIntConf( &this->version_, CONFIG_CURRENT_VERSION, 1, 999);

    if (!this->saveSelection()) {
        this->section(0);
        this->link(0);
    }

    if (this->buttonRepeatRate_ > 0 && this->buttonRepeatRate_ < 25) {
        this->buttonRepeatRate_ = 50;
    }
    if (this->powerTimeout_ > 0 && this->powerTimeout_ < 5) {
        this->powerTimeout_ = 5;
    }
	if (this->tvOutMode() != "PAL") 
		this->tvOutMode("NTSC");
    if (!FileUtils::dirExists(this->launcherPath())) {
        this->launcherPath(HOME_DIR);
    }
	if (!FileUtils::dirExists(this->prefix + "skins/" + this->skin())) {
		this->skin("Default");
	}
}

bool Config::configExistsUnderPath(const std::string & path) {
    std::string filePath = path + CONFIG_FILE_NAME;
    TRACE("checking for config under : %s", filePath.c_str());
    return FileUtils::fileExists(path + CONFIG_FILE_NAME);
}

bool Config::remove() {
    std::string filePath = this->prefix + CONFIG_FILE_NAME;
    return (0 == unlink(filePath.c_str()));
}

bool Config::fromFile() {
    TRACE("enter");
    bool result = false;
    std::string fileName = this->configFile();
    TRACE("loading config file from : %s", fileName.c_str());

	if (FileUtils::fileExists(fileName)) {
		TRACE("config file exists");
		std::ifstream confstream(fileName.c_str(), std::ios_base::in);
		if (confstream.is_open()) {
			std::string line;
			while (std::getline(confstream, line, '\n')) {
                try {
                    line = StringUtils::trim(line);
                    if (0 == line.length()) continue;
                    if ('#' == line[0]) continue;
                    std::string::size_type pos = line.find("=");
                    if (std::string::npos == pos) continue;
                    
                    std::string name = StringUtils::trim(line.substr(0,pos));
                    std::string value = StringUtils::trim(line.substr(pos + 1, line.length()));

                    if (0 == value.length()) continue;
                    name = StringUtils::toLower(name);

                    TRACE("handling kvp - %s = %s", name.c_str(), value.c_str());

                    try {
                        // strings
                        if (name == "externalapppath") {
                            this->externalAppPath(StringUtils::stripQuotes(value));
                        } else if (name == "skin") {
                            this->skin(StringUtils::stripQuotes(value));
                        } else if (name == "tvoutmode") {
                            this->tvOutMode(StringUtils::stripQuotes(value));
                        } else if (name == "lang") {
                            this->lang(StringUtils::stripQuotes(value));
                        } else if (name == "sectionfilter") {
                            this->sectionFilter(StringUtils::stripQuotes(value));
                        } else if (name == "launcherpath") {
                            this->launcherPath(StringUtils::stripQuotes(value));
                        } else if (name == "cpumenu") {
                            this->cpuMenu(StringUtils::stripQuotes(value));
                        } else if (name == "defaultcpuspeed") {
                            this->defaultCpuSpeed(StringUtils::stripQuotes(value));
                        }

                        // ints
                        else if (name == "buttonrepeatrate") {
                            this->buttonRepeatRate(atoi(value.c_str()));
                        } else if (name == "resolutionx") {
                            this->resolutionX(atoi(value.c_str()));
                        } else if (name == "resolutiony") {
                            this->resolutionY(atoi(value.c_str()));
                        } else if (name == "videobpp") {
                            this->videoBpp(atoi(value.c_str()));
                        } else if (name == "backlightlevel") {
                            this->backlightLevel(atoi(value.c_str()));
                        } else if (name == "backlighttimeout") {
                            this->backlightTimeout(atoi(value.c_str()));
                        } else if (name == "powertimeout") {
                            this->powerTimeout(atoi(value.c_str()));
                        } else if (name == "globalvolume") {
                            this->globalVolume(atoi(value.c_str()));
                        } else if (name == "aspectratio") {
                            this->aspectRatio(atoi(value.c_str()));
                        } else if (name == "outputlogs") {
                            this->outputLogs(atoi(value.c_str()));
                        } else if (name == "saveselection") {
                            this->saveSelection(atoi(value.c_str()));
                        } else if (name == "section") {
                            this->section(atoi(value.c_str()));
                        } else if (name == "link") {
                            this->link(atoi(value.c_str()));
                        } else if (name == "sethwlevelsonboot") {
                            this->setHwLevelsOnBoot(atoi(value.c_str()));
                        } else if (name == "respecthiddenlinks") {
                            this->setRespectHiddenLinks(atoi(value.c_str()));
                        } else if (name == "version") {
                            this->version(atoi(value.c_str()));
                        } else {
                            WARNING("unknown config key : %s", name.c_str());
                        }
                    }
                    catch (int param) { 
                        ERROR("int exception : %i from <%s, %s>", 
                            param, name.c_str(), value.c_str()); }
                    catch (char *param) { 
                        ERROR("char exception : %s from <%s, %s>", 
                            param, name.c_str(), value.c_str()); }
                    catch (std::exception &e) { 
                            ERROR("unknown error reading value from <%s, %s> : %s", 
                                name.c_str(), value.c_str(), e.what());
                    }
                } 
                catch (...) {
                    ERROR("Error reading config line : %s", line.c_str());
                }
            };
            confstream.close();
            this->isDirty = false;
            result = true;
        }
    }
    TRACE("exit");
    return result;
}
