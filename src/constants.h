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

#endif

