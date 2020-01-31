#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

#include <string>

class FileUtils {

	public:

		// returns a filename minus the dot extension part
		static std::string fileBaseName(const std::string & filename);

		// returns a files dot extension if there is on,e or an empty string
		static std::string fileExtension(const std::string & filename);

		static bool copyFile(std::string from, std::string to);

		static bool rmTree(std::string path);

		static bool fileExists(const std::string &path);

		static bool dirExists(const std::string &path);

		static std::string resolvePath(const std::string &path);

		// returns the path to a file
		static std::string dirName(const std::string &path);

		// returns a file name without any path part
		// or the last directory, essentially, anything after the last slash
		static std::string pathBaseName(const std::string &path);

		static std::string firstExistingDir(const std::string & path);

		static const int processPid(const std::string name);

		static const std::string fileReader(std::string path);
		static bool fileWriter(std::string path, std::string value);
		static bool fileWriter(std::string path, int value);

};


#endif
