
#include <string>
#include <vector>

#include "debug.h"
#include "utilities.h"

#include "hwfactory.h"
#include "ihardware.h"
#include "generic.h"
#include "gkd350h.h"
#include "linux.h"
#include "rg350.h"

IHardware* HwFactory::GetHardware(std::string device) {
    TRACE("enter : %s", device.c_str());
    if (0 == device.compare("gcw0")) {
        return (IHardware*)new HwGeneric();
    } else if (0 == device.compare("generic")) {
        return (IHardware*)new HwGeneric();
    } else if (0 == device.compare("gkd350h")) {
        return (IHardware*)new HwGkd350h();
    } else if (0 == device.compare("linux")) {
        return (IHardware*)new HwLinux();
    } else if (0 == device.compare("pg2")) {
        return (IHardware*)new HwGeneric();
    } else if (0 == device.compare("retrofw")) {
        return (IHardware*)new HwGeneric();
    } else if (0 == device.compare("rg350")) {
        return (IHardware*)new HwRg350();
    } else
        return (IHardware*)new HwGeneric();
}

std::vector<std::string> HwFactory::supportedDevices() {
    return std::vector<std::string>{
        "gcw0",
        "generic",
        "gkd350h",
        "linux",
        "pg2",
        "retrofw"
        "rg350"};
}

std::string HwFactory::readDeviceType() {

    std::string result = "generic";

    std::string cmdLine = fileReader("/proc/cmdline");
    TRACE("cmdLine : %s", cmdLine.c_str());
    std::vector<std::string> cmdParts;
    split(cmdParts, cmdLine, " ");
    for (std::vector<std::string>::iterator it = cmdParts.begin(); it != cmdParts.end(); it++) {
        std::string cmdPart = (*it);

        std::string::size_type pos = cmdPart.find("=");
        if (std::string::npos == pos) continue;
        std::string name = trim(cmdPart.substr(0, pos));
        std::string value = trim(cmdPart.substr(pos + 1, cmdPart.length()));
        if (0 == value.length()) continue;

        if (name == "hwvariant") {
            TRACE("hwvariant : '%s'", value.c_str());
            if (0 == value.compare("rg350")) {
                return "rg350";
            } else if (0 == value.compare("v11_ddr2_256mb")) {
                return "gcw0";
            } else if (0 == value.compare("v20_mddr_512mb")) {
                return "pg2";
            }
            break;
        }
    }

    // gkd 350?
    std::ifstream cpuInput;
    cpuInput.open("/proc/cpuinfo");
    if (cpuInput.is_open()) {
        bool found = false;
        for (std::string rawLine; std::getline(cpuInput, rawLine); ) {
            std::string trimLine = full_trim(rawLine);
            trimLine = toLower(trimLine);
            TRACE("cpu info line : %s", trimLine.c_str());
            if (std::string::npos != trimLine.find("gkd350")) {
                found = true;
                break;
            }
        }
        cpuInput.close();
        if (found) {
            return "gkd350h";
        }
    }

    // is it retro fw?
    if (fileExists("/etc/hostname")) {
        std::string host = fileReader("/etc/hostname");
        host = toLower(full_trim(host));
        TRACE("hostname : '%s'", host.c_str());
        if (0 == host.compare("retrofw")) {
            return "retrofw";
        }
    }

    // ok, is it open dingux?
    if (fileExists("/etc/issue")) {
        std::string issue = fileReader("/etc/issue");
        TRACE("issue : '%s'", issue.c_str());
        bool isDingux = false;
        if (!issue.empty()) {
            std::string lowerIssue = toLower(issue);
            std::vector<std::string> issueParts;
            split(issueParts, lowerIssue, " ");
            for (std::vector<std::string>::iterator it = issueParts.begin(); it != issueParts.end(); it++) {
                TRACE("checking token : '%s'", (*it).c_str());
                if (0 == (*it).compare("opendingux")) {
                    isDingux = true;
                    break;
                }
            }
            if (!isDingux) {
                return "linux";
            }
        }
    }

    return result;
}
