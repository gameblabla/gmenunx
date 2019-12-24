#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <string>

static const int APP_MIN_CONFIG_VERSION = 1;
static const std::string APP_NAME = "350teric";
static const std::string BINARY_NAME = "esoteric";
static const std::string TEMP_FILE = "/tmp/" + BINARY_NAME + ".tmp";
static const std::string APP_TTY = "/dev/tty1";

#ifdef TARGET_RG350
static const std::string & EXTERNAL_CARD_PATH = "/media/sdcard";
static const std::string USER_HOME = "/media/data/local/home/";
#else
static const std::string & EXTERNAL_CARD_PATH = "/media";
static const std::string USER_HOME = "~/";
#endif

static const std::string USER_PREFIX = USER_HOME + ".esoteric/";

enum vol_mode_t {
	VOLUME_MODE_MUTE, VOLUME_MODE_PHONES, VOLUME_MODE_NORMAL
};

#endif // __CONSTANTS_H__

