#ifndef _ICPU_
#define _ICPU_

#include <string>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <cerrno>
#include <unordered_map>

#include "debug.h"
#include "fileutils.h"
#include "utilities.h"

class ICpu {

    protected:

        static bool writeValueToFile(const std::string & path, const char *content) {
            bool result = false;
            int fd = open(path.c_str(), O_RDWR);
            if (fd == -1) {
                WARNING("Failed to open '%s': %s", path.c_str(), strerror(errno));
            } else {
                ssize_t written = write(fd, content, strlen(content));
                if (written == -1) {
                    WARNING("Error writing '%s': %s", path.c_str(), strerror(errno));
                } else {
                    result = true;
                }
                close(fd);
            }
            return result;
        }

    public: 

        bool overclockingSupported() { return this->getValues().size() > 1; };
        void setDefault() { this->setValue(this->getDefaultValue()); };
        virtual std::vector<std::string> getValues() = 0;
        virtual std::string getDefaultValue() = 0;
        virtual bool setValue(const std::string & val) = 0;
        virtual std::string getValue() = 0;
        virtual std::string getDisplayValue() = 0;
        virtual const std::string getType() = 0;
};

class JZ4770Factory {

    private:

        static const std::string SYSFS_CPUFREQ_PATH;
        static const std::string SYSFS_CPU_SCALING_GOVERNOR;
        static const std::string SYSFS_CPUFREQ_SET;
        static const std::string SYSFS_CPUFREQ_MAX;

    public:

        static ICpu * getCpu();
};

class JZ4770BasicCpu  : public ICpu{

    protected:

        std::unordered_map<std::string, std::string> modes_;
        std::string currentValue_;
        std::string defaultValue_;

        std::string mapModeToDisplay(std::string fromInternal) {
            std::unordered_map<std::string, std::string>::const_iterator it;
            for (it = this->modes_.begin(); it != this->modes_.end(); it++) {
                if (fromInternal == it->first) {
                    return it->second;
                }
            }
            return this->mapModeToDisplay(this->defaultValue_);
        }

        std::string mapDisplayToMode(std::string fromDisplay) {
            std::unordered_map<std::string, std::string>::iterator it;
            for (it = this->modes_.begin(); it != this->modes_.end(); it++) {
                TRACE("checking '%s' aginst <%s, %s>", fromDisplay.c_str(), it->first.c_str(), it->second.c_str());
                if (fromDisplay == it->second) {
                    TRACE("matched it as : %s", it->first.c_str());
                    return it->first;
                }
            }
            return this->defaultValue_;
        }

    public:
    
        JZ4770BasicCpu() : ICpu () {
            TRACE("enter");
            this->modes_.clear();
            this->modes_.insert({"1000", "1000"});
            this->defaultValue_= "1000";
            this->getValue();
        }

        std::string getDefaultValue() { return this->mapModeToDisplay(this->defaultValue_); };

        std::vector<std::string> getValues() {
            std::vector<std::string> results;
            std::unordered_map<std::string, std::string>::iterator it;
            for (it = this->modes_.begin(); it != this->modes_.end(); it++) {
                results.push_back(it->second);
            }
            return results;
        }

        /*
            advanced flavours need to override these
        */

        std::string getValue() { return this->defaultValue_; };
        std::string getDisplayValue() { return this->getValue() + " mhz"; };
        bool setValue(const std::string & val) { return true; };
        
        const std::string getType() { return "JZ4770"; };
};

class JZ4770GuvernorCpu : public JZ4770BasicCpu {

    private:

        const std::string guvernorPath_ = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";

    public:

        JZ4770GuvernorCpu() : JZ4770BasicCpu() {
            TRACE("enter");
            this->currentValue_ = this->defaultValue_ = "ondemand";
            this->modes_.clear();
            this->modes_.insert({"ondemand", "On demand"});
            this->modes_.insert({"performance", "Performance"});
            this->getValue();
            TRACE("exit : '%s'", this->getValue().c_str());
        }

        std::string getValue() {
            std::string rawValue = fileReader(this->guvernorPath_);
            this->currentValue_ = full_trim(rawValue);
            return this->mapModeToDisplay(this->currentValue_);
        }

        std::string getDisplayValue() { return this->getValue() + " mode"; };

        bool setValue(const std::string & val) {
            TRACE("raw desired : %s", val.c_str());
            std::string mode = this->defaultValue_;
            if (!val.empty()) {
                mode = this->mapDisplayToMode(val);
            }
            TRACE("internal desired : %s", mode.c_str());
            if (mode != this->currentValue_) {
                TRACE("update needed : current '%s' vs. desired '%s'", this->currentValue_.c_str(), mode.c_str());
                procWriter(this->guvernorPath_, mode);
                this->currentValue_ = mode;
            } else {
                TRACE("nothing to do");
            }
            TRACE("exit");
        }

        const std::string getType() { return "JZ4770 Guvernor Cpu"; };
};

class JZ4770OverClockedCpu : public JZ4770BasicCpu {

    private:

        const std::string SYSFS_CPUFREQ_PATH = "/sys/devices/system/cpu/cpu0/cpufreq";
        const std::string SYSFS_CPUFREQ_MAX = SYSFS_CPUFREQ_PATH + "/scaling_max_freq";
        const std::string SYSFS_CPUFREQ_SET = SYSFS_CPUFREQ_PATH + "/scaling_setspeed";
        const std::string SYSFS_CPUFREQ_GET = SYSFS_CPUFREQ_PATH + "/scaling_cur_freq";

    public:

        JZ4770OverClockedCpu() : JZ4770BasicCpu() {
            TRACE("enter");
            this->modes_.clear();
            this->modes_.insert({"360", "360"});
            this->modes_.insert({"1080", "1080"});
            this->defaultValue_= "1080";
            this->getValue();
        }

        std::string getValue() {
            std::string rawCpu = fileReader(SYSFS_CPUFREQ_GET);
            int mhz = atoi(rawCpu.c_str()) / 1000;
            std::stringstream ss;
            ss << mhz;
            std::string finalCpu = ss.str();
            this->currentValue_ = finalCpu;
            return this->mapModeToDisplay(this->currentValue_);
        }

        std::string getDisplayValue() { return this->getValue() + " mhz"; };

        bool setValue(const std::string & val) {
            TRACE("enter - raw desired : %s", val.c_str());

            if (val != this->currentValue_) {
                int rawVal = atoi(val.c_str()) * 1000;
                std::stringstream ss;
                ss << rawVal;
                std::string value;
                ss >> value;
                if (ICpu::writeValueToFile(SYSFS_CPUFREQ_MAX, value.c_str())) {
                    if (ICpu::writeValueToFile(SYSFS_CPUFREQ_SET, value.c_str())) {
                        this->currentValue_ = val;
                        return true;
                    }
                }
                return false;
            }
            return true;
        }

        const std::string getType() { return "JZ4770 Overclock Cpu"; };

};

class DefaultCpu : public ICpu {

    public: 
        DefaultCpu() : ICpu() { TRACE("enter"); }

        std::vector<std::string> getValues() { return { "Default"}; };
        std::string getDefaultValue() { return "Default"; };
        bool setValue(const std::string & val) { return true; };
        std::string getValue() { return "Default"; };
        std::string getDisplayValue() { return this->getValue() + " speed"; };
        const std::string getType() { return "Default"; };
};

#endif
