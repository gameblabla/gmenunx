#include <string.h>
#include <algorithm>
#include <memory>
#include <stdarg.h>
#include <sstream>

#include "stringutils.h"
#include "debug.h"

std::string& StringUtils::lTrim(std::string& str, const std::string& chars) {
    str.erase(0, str.find_first_not_of(chars));
    return str;
}
 
std::string& StringUtils::rTrim(std::string& str, const std::string& chars) {
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

std::string& StringUtils::fullTrim(std::string& str, const std::string& chars) {
    return StringUtils::lTrim(StringUtils::rTrim(str, chars), chars);
}

// General tool to strip spaces from both ends:
std::string StringUtils::trim(const std::string &s) {
    if (s.length() == 0)
        return s;
    std::size_t b = s.find_first_not_of(" \t\r");
    std::size_t e = s.find_last_not_of(" \t\r");
    if (b == -1) // No non-spaces
        return "";
    return std::string(s, b, e - b + 1);
}

void StringUtils::stringCopy(const std::string &s, char **cs) {
	*cs = (char*)malloc(s.length());
	strcpy(*cs, s.c_str());
}

char* StringUtils::stringCopy(const std::string &s) {
	char *cs = NULL;
	StringUtils::stringCopy(s, &cs);
	return cs;
}

template <typename T>
std::string StringUtils::toString( const T& value ) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}


bool StringUtils::split(std::vector<std::string> &vec, const std::string &str, const std::string &delim, bool destructive) {

	TRACE("enter - string : '%s', delim : '%s'", str.c_str(), delim.c_str());
	vec.clear();
	if (str.empty())
		return false;

	if (delim.empty()) {
		vec.push_back(str);
		return false;
	}

	std::string::size_type i = 0;
	std::string::size_type j = 0;

	while(1) {
		j = str.find(delim, i);
		if (j == std::string::npos) {
			TRACE("adding : '%s'", str.substr(i).c_str());
			vec.push_back(str.substr(i));
			break;
		}

		if (!destructive)
			j += delim.size();

		TRACE("adding : '%s'", str.substr(i, j - i).c_str());
		vec.push_back(str.substr(i, j - i));

		if (destructive)
			i = j + delim.size();

		if (i == str.size()) {
			TRACE("adding empty string"); 
			vec.push_back(std::string());
			break;
		}
	}
	TRACE("exit");
	return true;
}

std::string StringUtils::strReplace(std::string orig, const std::string &search, const std::string &replace) {
	if (0 == search.compare(replace)) return orig;
	std::string::size_type pos = orig.find( search, 0 );
	while (pos != std::string::npos) {
		orig.replace(pos, search.length(), replace);
		pos = orig.find( search, pos + replace.length() );
	}
	return orig;
}

std::string StringUtils::cmdClean(std::string cmdline) {
	TRACE("cmdclean - enter : %s", cmdline.c_str());
	std::string spchars = "\\`$();|{}&'\"*?<>[]!^~-#\n\r ";
	for (uint32_t i = 0; i < spchars.length(); i++) {
		std::string curchar = spchars.substr(i, 1);
		cmdline = StringUtils::strReplace(cmdline, curchar, "\\" + curchar);
	}
	TRACE("cmdclean - exit : %s", cmdline.c_str());
	return cmdline;
}


std::string StringUtils::splitInLines(std::string source, size_t max_width, std::string whitespace) {
    size_t  currIndex = max_width - 1;
    size_t  sizeToElim;
    while ( currIndex < source.length() ) {
        currIndex = source.find_last_of(whitespace,currIndex + 1); 
        if (currIndex == std::string::npos)
            break;
        currIndex = source.find_last_not_of(whitespace,currIndex);
        if (currIndex == std::string::npos)
            break;
        sizeToElim = source.find_first_not_of(whitespace,currIndex + 1) - currIndex - 1;
        source.replace( currIndex + 1, sizeToElim , "\n");
        currIndex += (max_width + 1); //due to the recently inserted "\n"
    }
    return source;
}

std::string StringUtils::stringFormat(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]);
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

std::string StringUtils::stripQuotes(std::string const &input) {
    std::string result = input;
    if (input.at(0) == '"' && input.at(input.length() - 1) == '"') {
        result = input.substr(1, input.length() - 2);
    }
    return result;
}

std::string StringUtils::toLower(const std::string & input) {
	std::string copy = input;
	transform(copy.begin(), copy.end(), copy.begin(), (int(*)(int)) tolower);
	return copy;
}
