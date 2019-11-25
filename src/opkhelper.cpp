#include <opk.h>
#include <list>
#include <string.h>

#include "opkhelper.h"
#include "desktopfile.h"
#include "utilities.h"
#include "debug.h"

// TODO :: fix me to a constants file
const string OPK_PLATFORM = "gcw0";

std::list<OpkHelper::Opk> * OpkHelper::ToOpkList(const string & path) {
    TRACE("enter : %s", path.c_str());

    if (!fileExists(path)) {
        return NULL;
    }

    TRACE("opening opk : %s", path.c_str());
    struct OPK *opk = opk_open(path.c_str());
    if (!opk) {
        ERROR("Unable to open OPK %s", path.c_str());
        return NULL;
    }
    TRACE("opened opk successfully");

    std::list<OpkHelper::Opk> * results = new std::list<OpkHelper::Opk>();

    //TRACE("meta outer loop");
    for (;;) {
        unsigned int i;
        bool has_metadata = false;
        const char *name;

        //TRACE("meta inner loop");
        for (;;) {
            string::size_type pos;
            int ret = opk_open_metadata(opk, &name);
            if (ret < 0) {
                ERROR("Error while loading meta-data for %s", path.c_str());
                break;
            } else if (!ret) {
                break;
            }
            /* Strip .desktop */
            string metadata(name);
            string platform;

            TRACE("extracted metadata : %s", metadata.c_str());
            pos = metadata.rfind('.');
            platform = metadata.substr(0, pos);

            /* Keep only the platform name */
            pos = platform.rfind('.');
            platform = platform.substr(pos + 1);

            //TRACE("resolved meta data to : %s", platform.c_str());
            if (platform == OPK_PLATFORM || platform == "all") {
                TRACE("metadata matches platform : %s", OPK_PLATFORM.c_str());

                const char *key, *val;
                size_t lkey, lval;
                int ret;

                OpkHelper::Opk opkFile;
                opkFile.metadata = metadata;
                opkFile.fileName = base_name(path);
                opkFile.fullPath = path;
                while ((ret = opk_read_pair(opk, &key, &lkey, &val, &lval))) {
                    if (ret < 0) {
                        ERROR("Unable to read meta-data");
                        break;
                    }

                    char buf[lval + 1];
                    sprintf(buf, "%.*s", (int)lval, val);
                    string temp;

                    if (!strncmp(key, "Categories", lkey)) {
                        temp = buf;
                        pos = temp.find(';');
                        if (pos != temp.npos)
                            temp = temp.substr(0, pos);
                        opkFile.category = temp;
                        //TRACE("%s", opkFile.category.c_str());
                    } else if (!strncmp(key, "Name", lkey)) {
                        opkFile.name = buf;
                        //TRACE("%s", opkFile.name.c_str());
                    } else if (!strncmp(key, "Comment", lkey)) {
                        opkFile.comment = buf;
                        //TRACE("%s", opkFile.comment.c_str());
                    } else if (!strncmp(key, "Terminal", lkey)) {
                        opkFile.terminal = !strncmp(val, "true", lval);
                        //TRACE("%i", opkFile.terminal);
                    } else if (!strncmp(key, "X-OD-Manual", lkey)) {
                        opkFile.manual = buf;
                        //TRACE("%s", opkFile.manual.c_str());
                    } else if (!strncmp(key, "Icon", lkey)) {
                        opkFile.icon = (string)buf;
                        //TRACE("%s", opkFile.icon.c_str());
                    } else if (!strncmp(key, "Exec", lkey)) {
                        opkFile.exec = buf;
                        //TRACE("raw exec : %s", opkFile.exec.c_str());
                        continue;
                    } else if (!strncmp(key, "selectordir", lkey)) {
                        opkFile.selectorDir = buf;
                        //TRACE("selector dir : %s", opkFile.selectorDir.c_str());
                    } else if (!strncmp(key, "selectorfilter", lkey)) {
                        opkFile.selectorFilter = buf;
                        //TRACE("selector filter : %s", opkFile.selectorFilter.c_str());
                    } else if (!strncmp(key, "Type", lkey)) {
                        opkFile.type = buf;
                        //TRACE("%s", opkFile.type.c_str());
                    } else if (!strncmp(key, "StartupNotify", lkey)) {
                        opkFile.startupNotify = !strncmp(val, "true", lval);
                        //TRACE("%i", opkFile.startupNotify);
                    } else if (!strncmp(key, "X-OD-NeedsDownscaling", lkey)) {
                        opkFile.needsDownscaling = !strncmp(val, "true", lval);
                        //TRACE("X-OD-NeedsDownscaling : %i", opkFile.needsDownscaling);
                    } else if (!strncmp(key, "MimeType", lkey)) {
                        opkFile.mimeType = buf;
                        //TRACE("%s", opkFile.mimeType.c_str());
                    } else {
                        WARNING("Unrecognized OPK link option: '%s'", key);
                    }
                }
                results->push_back(opkFile);
            }
        }

        if (!has_metadata)
             break;
    }

    opk_close(opk);
    TRACE("closed opk");
    return results;

}