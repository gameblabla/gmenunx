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
    TRACE("OpkHelper::ToOpkList - enter : %s", path.c_str());

    if (!fileExists(path)) {
        return NULL;
    }

    TRACE("OpkHelper::ToOpkList - opening opk : %s", path.c_str());
    struct OPK *opk = opk_open(path.c_str());
    if (!opk) {
        ERROR("Unable to open OPK %s", path.c_str());
        return NULL;
    }
    TRACE("OpkHelper::ToOpkList - opened opk successfully");

    std::list<OpkHelper::Opk> * results = new std::list<OpkHelper::Opk>();

    //TRACE("OpkHelper::ToOpkList - meta outer loop");
    for (;;) {
        unsigned int i;
        bool has_metadata = false;
        const char *name;

        //TRACE("OpkHelper::ToOpkList - meta inner loop");
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

            TRACE("OpkHelper::ToOpkList - extracted metadata : %s", metadata.c_str());
            pos = metadata.rfind('.');
            platform = metadata.substr(0, pos);

            /* Keep only the platform name */
            pos = platform.rfind('.');
            platform = platform.substr(pos + 1);

            //TRACE("OpkHelper::ToOpkList : resolved meta data to : %s", platform.c_str());
            if (platform == OPK_PLATFORM || platform == "all") {
                TRACE("OpkHelper::ToOpkList : metadata matches platform : %s", OPK_PLATFORM.c_str());

                const char *key, *val;
                size_t lkey, lval;
                int ret;

                OpkHelper::Opk opkFile;
                opkFile.metadata = metadata;
                opkFile.fileName = base_name(path);
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
                        //TRACE("OpkHelper::ToOpkList - opk::category : %s", opkFile.category.c_str());
                    } else if (!strncmp(key, "Name", lkey)) {
                        opkFile.name = buf;
                        //TRACE("OpkHelper::ToOpkList - opk::title : %s", opkFile.name.c_str());
                    } else if (!strncmp(key, "Comment", lkey)) {
                        opkFile.comment = buf;
                        //TRACE("OpkHelper::ToOpkList - opk::description : %s", opkFile.comment.c_str());
                    } else if (!strncmp(key, "Terminal", lkey)) {
                        opkFile.terminal = !strncmp(val, "true", lval);
                        //TRACE("OpkHelper::ToOpkList - opk::consoleapp : %i", opkFile.terminal);
                    } else if (!strncmp(key, "X-OD-Manual", lkey)) {
                        opkFile.manual = buf;
                        //TRACE("OpkHelper::ToOpkList - opk::manual : %s", opkFile.manual.c_str());
                    } else if (!strncmp(key, "Icon", lkey)) {
                        opkFile.icon = (string)buf;
                        //TRACE("OpkHelper::ToOpkList - opk::icon : %s", opkFile.icon.c_str());
                    } else if (!strncmp(key, "Exec", lkey)) {
                        opkFile.exec = buf;
                        //TRACE("OpkHelper::ToOpkList - opk::raw exec : %s", opkFile.exec.c_str());
                        continue;
                    } else if (!strncmp(key, "selectordir", lkey)) {
                        opkFile.selectorDir = buf;
                        //TRACE("OpkHelper::ToOpkList - opk::selector dir : %s", opkFile.selectorDir.c_str());
                    } else if (!strncmp(key, "selectorfilter", lkey)) {
                        opkFile.selectorFilter = buf;
                        //TRACE("OpkHelper::ToOpkList - opk::selector filter : %s", opkFile.selectorFilter.c_str());
                    } else if (!strncmp(key, "Type", lkey)) {
                        opkFile.type = buf;
                        //TRACE("OpkHelper::ToOpkList - opk::Type : %s", opkFile.type.c_str());
                    } else if (!strncmp(key, "StartupNotify", lkey)) {
                        opkFile.startupNotify = !strncmp(val, "true", lval);
                        //TRACE("OpkHelper::ToOpkList - opk::StartupNotify : %i", opkFile.startupNotify);
                    } else if (!strncmp(key, "X-OD-NeedsDownscaling", lkey)) {
                        opkFile.needsDownscaling = !strncmp(val, "true", lval);
                        //TRACE("OpkHelper::ToOpkList - opk::X-OD-NeedsDownscaling : %i", opkFile.needsDownscaling);
                    } else if (!strncmp(key, "MimeType", lkey)) {
                        opkFile.mimeType = buf;
                        //TRACE("OpkHelper::ToOpkList - opk::MimeType : %s", opkFile.mimeType.c_str());
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
    TRACE("OpkHelper::ToOpkLists : closed opk");
    return results;

}