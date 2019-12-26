#ifndef _IHARDWARE_
#define _IHARDWARE_

// getDiskSize
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <sys/sysinfo.h>
#include <string>
#include <vector>

#include "../debug.h"
#include "../utilities.h"

class IHardware {

    private:
        
        std::string kernelVersion_;

    protected:

        int16_t curMMCStatus;
        std::string BLOCK_DEVICE;
        std::string INTERNAL_MOUNT_DEVICE;
        std::string EXTERNAL_MOUNT_DEVICE;
        std::string EXTERNAL_MOUNT_POINT;
        std::string EXTERNAL_MOUNT_FORMAT;// = "auto";

    public:

        static const int BATTERY_CHARGING = 6;
        enum CARD_STATUS:int16_t {
            MMC_MOUNTED, MMC_UNMOUNTED, MMC_MISSING, MMC_ERROR
        };

        virtual std::string getSystemdateTime() = 0;
        virtual bool setSystemDateTime(std::string datetime) = 0;

        virtual bool getTVOutStatus() = 0;
        virtual void setTVOutMode(std::string mode) = 0;
        virtual std::string getTVOutMode() = 0;

        virtual void setPerformanceMode(std::string alias = "") = 0;
        virtual std::string getPerformanceMode() = 0;
        virtual std::vector<std::string>getPerformanceModes() = 0;

        virtual bool supportsOverClocking() = 0;
        virtual bool setCPUSpeed(uint32_t mhz) = 0;
        
        virtual std::vector<uint32_t> cpuSpeeds() = 0;
        virtual uint32_t getCpuDefaultSpeed() = 0;

        virtual void ledOn(int flashSpeed = 250) = 0;
        virtual void ledOff() = 0;

        virtual std::string getKernelVersion() {
            TRACE("enter");
            if (this->kernelVersion_.empty()) {
                std::string kernel = exec("/bin/uname -r");
                this->kernelVersion_ = full_trim(kernel);
            }
            TRACE("exit - %s", this->kernelVersion_.c_str());
            return this->kernelVersion_;
        };

        /*!
        Reads the current battery state and returns a number representing it's level of charge
        @return A number representing battery charge. 0 means fully discharged. 
        5 means fully charged. 6 represents charging.
        */
        virtual int getBatteryLevel() = 0;

        /*!
        Gets or sets the devices volume level, scale 0 - 100
        */
        virtual int getVolumeLevel() = 0;
        virtual int setVolumeLevel(int val) = 0;

        /*!
        Gets or sets the devices backlight level, scale 0 - 100
        */
        virtual int getBacklightLevel() = 0;
        virtual int setBacklightLevel(int val) = 0;

        /*!
        Gets or sets if we force scaling
        */
        virtual bool getKeepAspectRatio() = 0;
        virtual bool setKeepAspectRatio(bool val) = 0;

        std::string mountSd() {
            TRACE("enter");
            std::string command = "mount -t " + EXTERNAL_MOUNT_FORMAT + " " + EXTERNAL_MOUNT_DEVICE + " " + EXTERNAL_MOUNT_POINT + " 2>&1";
            std::string result = exec(command.c_str());
            TRACE("result : %s", result.c_str());
            system("sleep 1");
            this->checkUDC();
            return result;
        }

        std::string umountSd() {
            sync();
            std::string command = "umount -fl " + EXTERNAL_MOUNT_POINT + " 2>&1";
            std::string result = exec(command.c_str());
            system("sleep 1");
            this->checkUDC();
            return result;
        }

        void checkUDC() {
            TRACE("enter");
            this->curMMCStatus = MMC_ERROR;
            unsigned long size;
            TRACE("reading block device : %s", BLOCK_DEVICE.c_str());
            std::ifstream fsize(BLOCK_DEVICE.c_str(), std::ios::in | std::ios::binary);
            if (fsize >> size) {
                if (size > 0) {
                    // ok, so we're inserted, are we mounted
                    TRACE("size was : %lu, reading /proc/mounts", size);
                    std::ifstream procmounts( "/proc/mounts" );
                    if (!procmounts) {
                        this->curMMCStatus = MMC_ERROR;
                        WARNING("couldn't open /proc/mounts");
                    } else {
                        std::string line;
                        std::size_t found;
                        this->curMMCStatus = MMC_UNMOUNTED;
                        while (std::getline(procmounts, line)) {
                            if ( !(procmounts.fail() || procmounts.bad()) ) {
                                found = line.find("mcblk1");
                                if (found != std::string::npos) {
                                    this->curMMCStatus = MMC_MOUNTED;
                                    TRACE("inserted && mounted because line : %s", line.c_str());
                                    break;
                                }
                            } else {
                                this->curMMCStatus = MMC_ERROR;
                                WARNING("error reading /proc/mounts");
                                break;
                            }
                        }
                        procmounts.close();
                    }
                } else {
                    this->curMMCStatus = MMC_MISSING;
                    TRACE("not inserted");
                }
            } else {
                this->curMMCStatus = MMC_ERROR;
                WARNING("no card present");
            }
            fsize.close();
            TRACE("exit - %i",  this->curMMCStatus);
        }

        CARD_STATUS getCardStatus() {
            return (CARD_STATUS)this->curMMCStatus;
        }

        bool formatSdCard() {
            this->checkUDC();
            if (this->curMMCStatus == CARD_STATUS::MMC_MOUNTED) {
                this->umountSd();
            }
            if (this->curMMCStatus != CARD_STATUS::MMC_UNMOUNTED) {
                return false;
            }
            // TODO :: implement me
        }

        std::string getDiskFree(const char *path) {
            TRACE("enter - %s", path);
            std::string df = "N/A";
            struct statvfs b;

            if (statvfs(path, &b) == 0) {
                TRACE("read statvfs ok");
                // Make sure that the multiplication happens in 64 bits.
                uint32_t freeMiB = ((uint64_t)b.f_bfree * b.f_bsize) / (1024 * 1024);
                uint32_t totalMiB = ((uint64_t)b.f_blocks * b.f_frsize) / (1024 * 1024);
                TRACE("raw numbers - free: %lu, total: %lu, block size: %lu", b.f_bfree, b.f_blocks, b.f_bsize);
                std::stringstream ss;
                if (totalMiB >= 10000) {
                    ss << (freeMiB / 1024) << "." << ((freeMiB % 1024) * 10) / 1024 << " GiB";
                } else {
                    ss << freeMiB << " MiB";
                }
                std::getline(ss, df);
            } else WARNING("statvfs failed with error '%s'.", strerror(errno));
            TRACE("exit");
            return df;
        }

        std::string getDiskSize(const std::string &mountDevice) {
            TRACE("reading disk size for : %s", mountDevice.c_str());
            std::string result = "0 GiB";
            if (mountDevice.empty())
                return result;

            unsigned long long size;
            int fd = open(mountDevice.c_str(), O_RDONLY);
            ioctl(fd, BLKGETSIZE64, &size);
            close(fd);
            std::stringstream ss;
            std::uint64_t totalMiB = (size >> 20);
            if (totalMiB >= 10000) {
                ss << (totalMiB / 1024) << "." << ((totalMiB % 1024) * 10) / 1024 << " GiB";
            } else {
                ss << totalMiB << " MiB";
            }
            std::getline(ss, result);
            return result;
        }

        std::string getExternalMountDevice() {
            return this->EXTERNAL_MOUNT_DEVICE;
        }

        std::string getInternalMountDevice() {
            return this->INTERNAL_MOUNT_DEVICE;
        }

        std::string uptime() {
            std::string result = "";
            std::chrono::milliseconds uptime(0u);
            struct sysinfo x;
            if (sysinfo(&x) == 0) {
                uptime = std::chrono::milliseconds(
                    static_cast<unsigned long long>(x.uptime) * 1000ULL
                );

                int secs = uptime.count() / 1000;
                int s = (secs % 60);
                int m = (secs % 3600) / 60;
                int h = (secs % 86400) / 3600;

                static char buf[10];
                sprintf(buf, "%02d:%02d:%02d", h, m, s);
                result = buf;
            }
            return result;
        }

        virtual std::string getDeviceType() = 0;

        virtual void powerOff() {
            TRACE("enter");
            sync();
            system("poweroff");
            TRACE("exit");
        }

        virtual void reboot() {
            sync();
		    system("reboot");
        }

        virtual int defaultScreenWidth() { return 0; }
        virtual int defaultScreenHeight() { return 0; }
        virtual int defaultScreenBPP() { return 32; }

        virtual bool setScreenState(const bool &enable) = 0;

        virtual std::string systemInfo() {
            std::string result = execute("/usr/bin/uname -a");
            result += execute("/usr/bin/lshw -short 2>/dev/null");
            return result;
        }
};

#endif
