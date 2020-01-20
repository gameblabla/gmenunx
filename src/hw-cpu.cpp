

#include "hw-cpu.h"

const std::string JZ4770Factory::SYSFS_CPU_SCALING_GOVERNOR = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
const std::string JZ4770Factory::SYSFS_CPUFREQ_SET = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed";

ICpu * JZ4770Factory::getCpu() {
    ICpu * result;
    if (FileUtils::fileExists(JZ4770Factory::SYSFS_CPUFREQ_SET)) {
        result = (ICpu *) new JZ4770OverClockedCpu();
    } else if (FileUtils::fileExists(JZ4770Factory::SYSFS_CPU_SCALING_GOVERNOR)) {
        result = (ICpu *) new JZ4770GuvernorCpu();
    } else  {
        result = (ICpu *) new JZ4770BasicCpu();
    }
    return result;
}
