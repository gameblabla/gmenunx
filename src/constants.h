#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <string>

static const int APP_MIN_CONFIG_VERSION = 1;
static const std::string APP_NAME = "GMenuNX";

#ifdef TARGET_RG350
static const std::string & EXTERNAL_CARD_PATH = "/media/sdcard";
static const std::string USER_PREFIX = "/media/data/local/home/.gmenunx/";
#else
static const std::string & EXTERNAL_CARD_PATH = "/mnt";
// $HOME/ gets prepended to this
static const std::string USER_PREFIX = ".gmenunx/";
#endif

enum vol_mode_t {
	VOLUME_MODE_MUTE, VOLUME_MODE_PHONES, VOLUME_MODE_NORMAL
};

#endif // __CONSTANTS_H__

