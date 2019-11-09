#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

using std::string;
using std::vector;

static const string CONFIG_FILE_NAME = "gmenunx.conf";
static const int CONFIG_CURRENT_VERSION = 1;

class Config {

public:

    Config(string const &prefix);
    ~Config();

    string skin; //="Default"
    string performance; //="On demand"
    string tvOutMode; //="NTSC"
    string lang; //=""
    string batteryType; //="BL-5B"
    string sectionFilter; //="applications,foo"

    int buttonRepeatRate; //=10
    int resolutionX; //=320
    int resolutionY; //=240
    int backlightLevel; //=70
    int minBattery; //=0
    int maxBattery; //=5
    int backlightTimeout; //=30
    int videoBpp; //=32
    int cpuMin; //=342
    int cpuMax; //=996
    int cpuMenu; //=600
    int globalVolume; //=60
    int link; //=1
    int section; //=1
    int saveSelection; //=1
    int powerTimeout; //=10
    int outputLogs; //=1
    int version; //=1

    bool loadConfig();
    string toString();
    string now();
    bool save();
    int halfX() { return this->resolutionX / 2; }
    int halfY() { return this->resolutionY / 2; }

private:

    string prefix;

    void reset();
    bool fromFile();
    void constrain();

    std::string stripQuotes(std::string const &input);
    std::string string_format(const std::string fmt_str, ...);

};

#endif