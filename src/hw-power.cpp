#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <sstream>
#include <fstream>

#include "hw-power.h"
#include "fileutils.h"
#include "stringutils.h"

IPower::IPower() {
    this->state_ = PowerStates::UNKNOWN;
    this->level_ = 0;
}

JzPower::JzPower () : IPower() {
    TRACE("enter");
    this->pollBatteries_ = FileUtils::fileExists(BATTERY_CHARGING_PATH) && FileUtils::fileExists(BATTERY_LEVEL_PATH);
    this->read();
    TRACE("exit : %i", this->pollBatteries_);
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
    int online = false;

	std::string strval = FileUtils::fileReader(BATTERY_CHARGING_PATH);
    TRACE("raw string value : '%s'", strval.c_str());
    if (strval.length() == 1) {
        std::sscanf(strval.c_str(), "%i", &online);
    }
    TRACE("online : %i", online);
    this->state_ = online ? IPower::PowerStates::CHARGING : IPower::PowerStates::DRAINING;

    TRACE("exit - state : %i,", this->state_);
    return true; 
}

bool JzPower::readLevel() {
    TRACE("enter");
    bool result = false;

    std::string strval = FileUtils::fileReader(this->BATTERY_LEVEL_PATH);
    this->level_ = atoi(strval.c_str());
    result = true;
/*
    FILE* devbatt_;

    devbatt_ = fopen(this->BATTERY_LEVEL_PATH.c_str(), "r");
    if (NULL != devbatt_) {
        std::size_t n = 4;
        std::size_t t = sizeof(char);
        char* strval = (char*)malloc((std::size_t)10 * sizeof(char));
        
        if (0 == std::fseek (devbatt_, 0, SEEK_SET)) {
            if (0 == std::fread (strval, t, n, devbatt_)) {
                unsigned short currentval = atoi(strval);

                // force fit to jz raw range
                if (currentval > 100) {
                    currentval = 100;
                } else if (currentval < 0) {
                    currentval = 0;
                }

                this->level_ = currentval;
                result = true;
            }
        }
        std::fclose(devbatt_);
        
    } else {
        ERROR("couldn't open battery device : '%s'", this->BATTERY_LEVEL_PATH.c_str());
    }
*/

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
