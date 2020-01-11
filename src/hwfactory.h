#ifndef _HWFACTORY_
#define _HWFACTORY_

#include <string>
#include <vector>

#include "hw-ihardware.h"

class HwFactory {
    public:

        static std::vector<std::string> supportedDevices();

        static IHardware * GetHardware(std::string device);

        static std::string readDeviceType();

    protected:

    private:

};

#endif // _HWFACTORY_