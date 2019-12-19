#ifndef _HWFACTORY_
#define _HWFACTORY_

#include "../utilities.h"
#include "../debug.h"

#include "ihardware.h"
#include "generic.h"
#include "rg350.h"

class HwFactory {
    public:
        static IHardware * GetHardware() {
            std::string device = HwFactory::readDeviceType();
            if (0 == device.compare("generic")) {
                return (IHardware*)new HwGeneric();
            } else if (0 == device.compare("rg350")) {
                return (IHardware*)new HwRg350();
            } else if (0 == device.compare("v11_ddr2_256mb")) {
                // GCW Zero
                return (IHardware*)new HwGeneric();
            } else if (0 == device.compare("v20_mddr_512mb")) {
                // pocket go 2, miyoo max
                return (IHardware*)new HwGeneric();
            } else return (IHardware*)new HwGeneric();
        }
    protected:

    private:
        static std::string readDeviceType() {
            std::string cmdLine = fileReader("/proc/cmdline");
            std::vector<std::string> cmdParts;
            split(cmdParts, cmdLine, " ");
            for (std::vector<std::string>::iterator it = cmdParts.begin(); it != cmdParts.end(); it++) {
                std::string cmdPart = (*it);

                std::string::size_type pos = cmdPart.find("=");
                if (std::string::npos == pos) continue;
                std::string name = trim(cmdPart.substr(0, pos));
                std::string value = trim(cmdPart.substr(pos+1,cmdPart.length()));
                if (0 == value.length()) continue;

                if (name == "hwvariant") {
                    return value;
                }
            }
            return "generic";
        }
};

#endif // _HWFACTORY_