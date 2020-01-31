#ifndef _ICPU_
#define _ICPU_

#include <string>
#include <sstream>
#include <vector>
#include <sys/mman.h>
#include <assert.h>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <cerrno>
#include <map>

#include "debug.h"
#include "fileutils.h"
#include "utilities.h"

class ICpu {

    protected:

        static const unsigned long regMapSize = 4096;
        static const unsigned long regMapMask = ICpu::regMapSize - 1;

        std::map<std::string, std::string> modes_;
        std::string currentValue_;
        std::string defaultValue_;

        inline unsigned long getRegister(unsigned long target) {
            TRACE("enter : %lx", target);
            unsigned long result = 0;

            volatile uint8_t memdev = open("/dev/mem", O_RDWR);
            if (memdev > 0) {
                TRACE("opened /dev/mem for rw ok");

                TRACE("mapping the memory");
                void *map_base = (unsigned long *)mmap(
                    0, 
                    ICpu::regMapSize, 
                    PROT_READ | PROT_WRITE, 
                    MAP_SHARED, 
                    memdev, 
                    target & ~ICpu::regMapMask);

                if (map_base == MAP_FAILED) {
                    TRACE("couldn't map memory");
                } else {
                    TRACE("memory mapped at location : 0x%p", map_base);
                    unsigned long *virt_addr = (unsigned long *)map_base + (target & ICpu::regMapMask);
                    result = *((unsigned long *) virt_addr);
                    TRACE("register full value : 0x%lx", result);
                    TRACE("un-mapping the memory");
                    munmap(map_base, ICpu::regMapSize);
                }
                TRACE("closing /dev/mem");
                close(memdev);
            } else {
                TRACE("couldn't open /dev/mem : %i", memdev);
            }
            TRACE("exit : 0x%lx", result);
            return result;
        };

        inline bool setRegister(unsigned long target, unsigned long value) {
            TRACE("enter - target : 0x%lx, value : 0x%lx", target, value);
            bool result = false;

            volatile uint8_t memdev = open("/dev/mem", O_RDWR);
            if (memdev > 0) {
                TRACE("opened /dev/mem for rw ok");

                TRACE("mapping the memory");
                void *map_base = (unsigned long *)mmap(
                    0, 
                    ICpu::regMapSize, 
                    PROT_READ | PROT_WRITE, 
                    MAP_SHARED, 
                    memdev, 
                    target & ~ICpu::regMapMask);

                if (map_base == MAP_FAILED) {
                    TRACE("couldn't map memory");
                } else {
                    TRACE("memory mapped at location : 0x%p", map_base);
                    unsigned long *virt_addr = (unsigned long *)map_base + (target & ICpu::regMapMask);

                    *(virt_addr) = value;
                    unsigned long read_result = *((unsigned long *) virt_addr);

                    result = (value == read_result);
                    TRACE("read and write values match : %i", result);

                    TRACE("un-mapping the memory");
                    munmap(map_base, ICpu::regMapSize);
                }
                TRACE("closing /dev/mem");
                close(memdev);
            } else {
                TRACE("couldn't open /dev/mem : %i", memdev);
            }
            TRACE("exit : %i", result);
            return result;
        };

        static bool writeValueToFile(const std::string & path, const char *content) {
            TRACE("enter : writing '%s' to '%s'", content, path.c_str());
            bool result = false;
            int fd = open(path.c_str(), O_RDWR);
            if (fd == -1) {
                WARNING("Failed to open '%s': %s", path.c_str(), strerror(errno));
            } else {
                ssize_t written = write(fd, content, strlen(content));
                if (written == -1) {
                    WARNING("Error writing '%s' '%s': %s", path.c_str(), content, strerror(errno));
                } else {
                    result = true;
                }
                close(fd);
            }
            return result;
        };

        std::string mapModeToDisplay(std::string fromInternal) {
            std::map<std::string, std::string>::const_iterator it;
            for (it = this->modes_.begin(); it != this->modes_.end(); it++) {
                if (fromInternal == it->first) {
                    return it->second;
                }
            }
            return this->mapModeToDisplay(this->defaultValue_);
        };

        std::string mapDisplayToMode(std::string fromDisplay) {
            std::map<std::string, std::string>::iterator it;
            for (it = this->modes_.begin(); it != this->modes_.end(); it++) {
                TRACE("checking '%s' against <%s, %s>", fromDisplay.c_str(), it->first.c_str(), it->second.c_str());
                if (fromDisplay == it->second) {
                    TRACE("matched it as : %s", it->first.c_str());
                    return it->first;
                }
            }
            return this->defaultValue_;
        };

    public: 

        bool overclockingSupported() { return this->getValues().size() > 1; };
        void setDefault() { this->setValue(this->getDefaultValue()); };

        std::vector<std::string> getValues() {
            std::vector<std::string> results;
            std::map<std::string, std::string>::iterator it;
            for (it = this->modes_.begin(); it != this->modes_.end(); it++) {
                results.push_back(it->second);
            }
            return results;
        };

        std::string getDefaultValue() { return this->mapModeToDisplay(this->defaultValue_); };

        virtual bool setValue(const std::string & val) = 0;
        virtual std::string getValue() = 0;
        virtual std::string getDisplayValue() = 0;
        virtual const std::string getType() = 0;
};

class X1830Cpu : public ICpu {

    static const unsigned int gkdBaseAddress = 0x10000000;
    static const unsigned int apllAddressOffset = 0x10 >> 2;

    private:

        uint regRead() {
            TRACE("enter");
            
            unsigned long target = X1830Cpu::gkdBaseAddress + X1830Cpu::apllAddressOffset;
            unsigned long regVal = this->getRegister(target);
            int result = 0;
            if (regVal > 0) {
                // drop first 20 bits
                regVal = regVal >> 20;
                // and mask to 8 bits
                regVal &= 0xFF;
                result = (int)regVal;
            }
            TRACE("exit : %i", result);
            return result;
        };
        bool regWrite(uint value) {
            TRACE("enter : %i", value);
            bool result = false;
            unsigned long target = X1830Cpu::gkdBaseAddress + X1830Cpu::apllAddressOffset;
            unsigned long regVal = this->getRegister(target);
            if (regVal > 0) {
                // mask off first 20 bits with a logical &
                unsigned long finalValue = regVal & 0xFFFFF;
                TRACE("masked value : 0x%lx", finalValue);
                // shift value into place
                unsigned long shiftedValue = value << 20;
                TRACE("shifted value : 0x%lx", shiftedValue);
                finalValue = finalValue + shiftedValue;
                TRACE("flagged value : 0x%lx", finalValue);
                result = this->setRegister(target, finalValue);
            }
            TRACE("exit : %i", result);
            return result;
        };

    public:

        X1830Cpu() : ICpu() {
            TRACE("enter");
            this->currentValue_ = this->defaultValue_ = "95";
            this->modes_.clear();
            for (int x = 0; x < 8; x++) {
                int internalValue = (95 + (3 * x));
                std::stringstream ss;
                ss << internalValue;
                std::string internalMode = ss.str();
                ss.str("");
                ss.clear();
                ss << "Level " << x;
                std::string displayMode = (0 == x ? "Bypass" : ss.str());
                this->modes_.insert({internalMode, displayMode});
            }
            TRACE("modes set");
            this->getValue();
            TRACE("exit : '%s'", this->getValue().c_str());
        };

        bool setValue(const std::string & val) {
            TRACE("enter : '%s'", val.c_str());

            std::string internalVal = this->defaultValue_;
            if (!val.empty()) {
                internalVal = this->mapDisplayToMode(val);
            }
            bool result = false;
            uint intVal = (uint)strtoul(internalVal.c_str(), 0, 0);
            TRACE("int val : %i", intVal);

            if (this->regWrite(intVal)) {
                this->currentValue_ = internalVal;
                result = true;
            }
            TRACE("exit : %i", result);
            return result; 
        };
        std::string getValue() {
            TRACE("enter");
            int cpuVal = this->regRead();
            if (cpuVal > 0) {
                std::stringstream ss;
                std::string result;
                ss << cpuVal;
                ss >> result;
                this->currentValue_ = ss.str();
            }
            TRACE("exit : '%s'", this->currentValue_.c_str());
            return this->mapModeToDisplay(this->currentValue_);
        };
        std::string getDisplayValue() {
            TRACE("enter");
            return this->getValue(); 
        };
        const std::string getType() { return "X1830"; };
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

    public:
    
        JZ4770BasicCpu() : ICpu () {
            TRACE("enter");
            this->modes_.clear();
            this->defaultValue_ = "1000";
            this->modes_.insert({this->defaultValue_, this->defaultValue_});
            
            this->getValue();
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
            this->modes_.insert({this->defaultValue_, "On demand"});
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
            bool result = false;
            TRACE("internal desired : %s", mode.c_str());
            if (mode != this->currentValue_) {
                TRACE("update needed : current '%s' vs. desired '%s'", this->currentValue_.c_str(), mode.c_str());
                procWriter(this->guvernorPath_, mode);
                this->currentValue_ = mode;
                result = true;
            } else {
                TRACE("nothing to do");
            }
            TRACE("exit : %i", result);
            return result;
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
            this->defaultValue_ = "1080";
            this->modes_.clear();
            this->modes_.insert({"360", "360"});
            this->modes_.insert({this->defaultValue_, this->defaultValue_});
            
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

            std::string internalVal = this->defaultValue_;
            if (!val.empty()) {
                internalVal = this->mapDisplayToMode(val);
            }

            if (internalVal != this->currentValue_) {
                int rawVal = atoi(internalVal.c_str()) * 1000;
                std::stringstream ss;
                ss << rawVal;
                std::string value;
                ss >> value;
                if (ICpu::writeValueToFile(SYSFS_CPUFREQ_MAX, value.c_str())) {
                    if (ICpu::writeValueToFile(SYSFS_CPUFREQ_SET, value.c_str())) {
                        this->currentValue_ = internalVal;
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
        DefaultCpu() : ICpu() { 
            TRACE("enter");
            this->currentValue_ = this->defaultValue_ = "Default";
            this->modes_.clear();
            this->modes_.insert({this->defaultValue_, this->defaultValue_});
            TRACE("exit : '%s'", this->getValue().c_str());
        }

        bool setValue(const std::string & val) { return true; };
        std::string getValue() { return this->mapModeToDisplay(this->currentValue_); };
        std::string getDisplayValue() { return this->getValue() + " speed"; };
        const std::string getType() { return this->defaultValue_; };
};

#endif
