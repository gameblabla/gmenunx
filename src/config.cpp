#include <memory>
#include <stdarg.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <dirent.h>
#include <unistd.h>

#include "debug.h"
#include "config.h"
#include "utilities.h"

#define sync() sync(); system("sync");

using std::ifstream;
using std::ofstream;
using std::string;

Config::Config(string const &prefix) {
    DEBUG("Config::Skin - enter - prefix : %s", prefix.c_str());
    this->prefix = prefix;
}

Config::~Config() {
    TRACE("Config::~Config");
}

string Config::toString() {

    vector<string> vec;

    vec.push_back("# gmenunx config file");
    vec.push_back("# lines starting with a # are ignored");

    // strings
    vec.push_back(string_format("skin=\"%s\"", this->skin.c_str()));
    vec.push_back(string_format("performance=\"%s\"", this->performance.c_str()));
    vec.push_back(string_format("tvOutMode=\"%s\"", this->tvOutMode.c_str()));
    vec.push_back(string_format("lang=\"%s\"", this->lang.c_str()));
    vec.push_back(string_format("batteryType=\"%s\"", this->batteryType.c_str()));
    vec.push_back(string_format("sectionFilter=\"%s\"", this->sectionFilter.c_str()));

    // ints
    vec.push_back(string_format("buttonRepeatRate=%i", this->buttonRepeatRate));
    vec.push_back(string_format("resolutionX=%i", this->resolutionX));
    vec.push_back(string_format("resolutionY=%i", this->resolutionY));
    vec.push_back(string_format("videoBpp=%i", this->videoBpp));

    vec.push_back(string_format("backlightLevel=%i", this->backlightLevel));
    vec.push_back(string_format("backlightTimeout=%i", this->backlightTimeout));
    vec.push_back(string_format("powerTimeout=%i", this->powerTimeout));

    vec.push_back(string_format("minBattery=%i", this->minBattery));
    vec.push_back(string_format("maxBattery=%i", this->maxBattery));

    vec.push_back(string_format("cpuMin=%i", this->cpuMin));
    vec.push_back(string_format("cpuMax=%i", this->cpuMax));
    vec.push_back(string_format("cpuMenu=%i", this->cpuMenu));

    vec.push_back(string_format("globalVolume=%i", this->globalVolume));
    vec.push_back(string_format("outputLogs=%i", this->outputLogs));

    vec.push_back(string_format("saveSelection=%i", this->saveSelection));
    vec.push_back(string_format("section=%i", this->section));
    vec.push_back(string_format("link=%i", this->link));

    vec.push_back(string_format("version=%i", this->version));
    
    std::string s;
    for (const auto &piece : vec) s += (piece + "\n");
    return s;
}
    
bool Config::save() {
    TRACE("Config::save - enter");
    string fileName = this->prefix + CONFIG_FILE_NAME;
    TRACE("Config::save - saving to : %s", fileName.c_str());

	std::ofstream config(fileName.c_str());
	if (config.is_open()) {
		config << this->toString();
		config.close();
		sync();
	}

    TRACE("Config::save - exit");
    return true;
}

bool Config::loadConfig() {
    TRACE("Config::loadConfig - enter");
    this->reset();
    return this->fromFile();
}

/* Private methods */

void Config::reset() {
    TRACE("Config::reset - enter");

     //strings
    this->skin = "Default";
    this->performance = "On demand";
    this->tvOutMode = "NTSC";
    this->lang = "";
    this->batteryType = "BL-5B";
    this->sectionFilter = "";

    // ints
    this->buttonRepeatRate = 10;
    this->resolutionX = 320;
    this->resolutionY = 240;
    this->videoBpp = 32;

    this->powerTimeout = 10;
    this->backlightTimeout = 30;
    this->backlightLevel = 70;

    this->minBattery = 0;
    this->maxBattery = 5;

    this->cpuMin = 342;
    this->cpuMax = 996;
    this->cpuMenu = 600;

    this->globalVolume = 60;
    this->outputLogs = 0;

    this->saveSelection = 1;
    this->section = 1;
    this->link = 1;

    this->version = CONFIG_CURRENT_VERSION;

    TRACE("Config::reset - exit");
    return;
}

void Config::constrain() {

	evalIntConf( &this->backlightTimeout, 30, 10, 300);
	evalIntConf( &this->powerTimeout, 10, 1, 300);
	evalIntConf( &this->outputLogs, 0, 0, 1 );
	evalIntConf( &this->cpuMax, 642, 200, 1200 );
	evalIntConf( &this->cpuMin, 342, 200, 1200 );
	evalIntConf( &this->cpuMenu, 600, 200, 1200 );
	evalIntConf( &this->globalVolume, 60, 1, 100 );
	evalIntConf( &this->videoBpp, 16, 8, 32 );
	evalIntConf( &this->backlightLevel, 70, 1, 100);
	evalIntConf( &this->minBattery, 0, 0, 5);
	evalIntConf( &this->maxBattery, 5, 0, 5);
	evalIntConf( &this->version, CONFIG_CURRENT_VERSION, 1, 999);

    if (!this->saveSelection) {
        this->section = 0;
        this->link = 0;
    }

	if (this->performance != "Performance") 
		this->performance = "On demand";
	if (this->tvOutMode != "PAL") 
		this->tvOutMode = "NTSC";

}

bool Config::fromFile() {
    TRACE("Config::fromFile - enter");
    bool result = false;
    string fileName = this->prefix + CONFIG_FILE_NAME;
    TRACE("Config::fromFile - loading config file from : %s", fileName.c_str());

	if (fileExists(fileName)) {
		TRACE("Config::fromFile - config file exists");
		std::ifstream confstream(fileName.c_str(), std::ios_base::in);
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

                TRACE("Config::fromFile - handling kvp - %s = %s", name.c_str(), value.c_str());

                // strings
                if (name == "skin") {
                    this->skin = stripQuotes(value);
                } else if (name == "performance") {
                    this->performance = stripQuotes(value);
                } else if (name == "tvOutMode") {
                    this->tvOutMode = stripQuotes(value);
                } else if (name == "lang") {
                    this->lang = stripQuotes(value);
                } else if (name == "batteryType") {
                    this->batteryType = stripQuotes(value);
                } else if (name == "sectionFilter") {
                    this->sectionFilter = stripQuotes(value);
                } 

                // ints
                else if (name == "buttonRepeatRate") {
                    this->buttonRepeatRate = atoi(value.c_str());
                } else if (name == "resolutionX") {
                    this->resolutionX = atoi(value.c_str());
                } else if (name == "resolutionY") {
                    this->resolutionY = atoi(value.c_str());
                } else if (name == "videoBpp") {
                    this->videoBpp = atoi(value.c_str());
                } else if (name == "backlightLevel") {
                    this->backlightLevel = atoi(value.c_str());
                } else if (name == "backlightTimeout") {
                    this->backlightTimeout = atoi(value.c_str());
                } else if (name == "powerTimeout") {
                    this->powerTimeout = atoi(value.c_str());
                } else if (name == "minBattery") {
                    this->minBattery = atoi(value.c_str());
                } else if (name == "maxBattery") {
                    this->maxBattery = atoi(value.c_str());
                } else if (name == "cpuMin") {
                    this->cpuMin = atoi(value.c_str());
                } else if (name == "cpuMax") {
                    this->cpuMax = atoi(value.c_str());
                } else if (name == "cpuMenu") {
                    this->cpuMenu = atoi(value.c_str());
                } else if (name == "globalVolume") {
                    this->globalVolume = atoi(value.c_str());
                } else if (name == "outputLogs") {
                    this->outputLogs = atoi(value.c_str());
                } else if (name == "saveSelection") {
                    this->saveSelection = atoi(value.c_str());
                } else if (name == "section") {
                    this->section = atoi(value.c_str());
                } else if (name == "link") {
                    this->link = atoi(value.c_str());
                } else if (name == "version") {
                    this->version = atoi(value.c_str());
                }

            };
            confstream.close();
            this->constrain();
            result = true;
        }
    }
    TRACE("Config::fromFile - exit");
    return result;
}

std::string Config::stripQuotes(std::string const &input) {
    string result = input;
    if (input.at(0) == '"' && input.at(input.length() - 1) == '"') {
        result = input.substr(1, input.length() - 2);
    }
    return result;
}

std::string Config::string_format(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}
