#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <string>

#ifdef TARGET_RG350
static const std::string & EXTERNAL_CARD_PATH = "/media/sdcard";

#else
static const std::string & EXTERNAL_CARD_PATH = "/mnt";

#endif

#endif
