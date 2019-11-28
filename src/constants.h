#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <string>

#ifdef TARGET_RG350
static const std::string & EXTERNAL_CARD_PATH = "/media/sdcard";
static const std::string USER_PREFIX = "/media/data/local/home/.gmenunx/";
#else
static const std::string & EXTERNAL_CARD_PATH = "/mnt";
// $HOME/ gets prepended to this
static const std::string USER_PREFIX = ".gmenunx/";

#endif

enum mmc_status {
	MMC_MOUNTED, MMC_UNMOUNTED, MMC_MISSING, MMC_ERROR
};
enum vol_mode_t {
	VOLUME_MODE_MUTE, VOLUME_MODE_PHONES, VOLUME_MODE_NORMAL
};

#endif // __CONSTANTS_H__

