#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
//#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <sstream>
#include <fstream>

#include "hw-power.h"
#include "fileutils.h"

IPower::IPower() {
    this->state_ = PowerStates::UNKNOWN;
    this->level_ = 0;
}

JzPower::JzPower () : IPower() {
    this->pollBatteries_ = FileUtils::fileExists(BATTERY_CHARGING_PATH) && FileUtils::fileExists(BATTERY_LEVEL_RAW_PATH);
    this->read();
};
bool JzPower::read() {
    if (!this->pollBatteries_) 
        return true;
    if (this->readLevel()) {
        return this->readState();
    }
    return false;
}

bool JzPower::readState() {
    TRACE("enter");
    bool result = false;
    int online = false;

	std::ifstream str(BATTERY_CHARGING_PATH);
	std::stringstream buf;
	buf << str.rdbuf();
    std::sscanf(buf.str().c_str(), "%i", &online);

    this->state_ = online ? IPower::PowerStates::CHARGING : IPower::PowerStates::DRAINING;

    TRACE("exit - state : %i,", this->state_);
    return result; 
}

bool JzPower::readLevel() {
    TRACE("enter");
    bool result = false;

    FILE* devbatt_;

    devbatt_ = fopen(this->BATTERY_LEVEL_RAW_PATH.c_str(), "r");
    if (NULL != devbatt_) {
        std::size_t n = 4;
        std::size_t t = sizeof(char);
        char* strval = (char*)malloc((std::size_t)10 * sizeof(char));
        
        if (0 == std::fseek (devbatt_, 0, SEEK_SET)) {
            if (0 == std::fread (strval, t, n, devbatt_)) {
                unsigned short currentval = atoi(strval);

                // force fit to jz raw range
                if (currentval > 4000) {
                    currentval = 4000;
                } else if (currentval < 3000) {
                    currentval = 3000;
                }

                // scale to 100
                this->level_ = ((currentval - 3000) / 10);
                result = true;
            }
        }
        std::fclose(devbatt_);
        
    } else {
        ERROR("couldn't open battery device : '%s'", this->BATTERY_LEVEL_RAW_PATH.c_str());
    }

    TRACE("exit - level : %i,", this->level_);
    return result; 
}

GenericPower::GenericPower() : IPower() { 
    this->counter_ = 0;
    this->read();
};

bool GenericPower::read() { 
    TRACE("enter");
    this->counter_ += rand() % 5 + 1;
    if (this->counter_ > 100) {
        this->state_ = IPower::PowerStates::CHARGING;
        if (this->counter_ > 200) {
            this->counter_ = 0;
        }
    } else {
        this->state_ = IPower::PowerStates::DRAINING;
    }
    this->level_ = this->counter_ % 100;
    TRACE("exit - level : %i, state : %i", this->level_, (int)this->state_);
    return true; 
}
