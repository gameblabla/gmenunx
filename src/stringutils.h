#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

#include <string>
#include <vector>

class StringUtils {

    public:

        static std::string& lTrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
        static std::string& rTrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
        static std::string& fullTrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

        static std::string trim(const std::string& s);
        static std::string strReplace (std::string orig, const std::string &search, const std::string &replace);
        static std::string cmdClean (std::string cmdline);

        static char *stringCopy(const std::string &);
        static void stringCopy(const std::string &, char **);

        template <typename T>
        static std::string toString( const T& value );

        static bool split (std::vector<std::string> &vec, const std::string &str, const std::string &delim, bool destructive = true);

        static std::string splitInLines(std::string source, std::size_t max_width, std::string whitespace = " \t\r");
        static std::string stringFormat(const std::string fmt_str, ...);
        static std::string stripQuotes(std::string const &input);
        static std::string toLower(const std::string & input);

};

#endif
