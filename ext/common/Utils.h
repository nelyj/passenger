/*
 *  Phusion Passenger - http://www.modrails.com/
 *  Copyright (c) 2008, 2009 Phusion
 *
 *  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */
#ifndef _PASSENGER_UTILS_H_
#define _PASSENGER_UTILS_H_

#include <boost/shared_ptr.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <cstdio>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include "Exceptions.h"

typedef struct CachedMultiFileStat CachedMultiFileStat;

namespace Passenger {

using namespace std;
using namespace boost;

/** Enumeration which indicates what kind of file a file is. */
typedef enum {
	/** The file doesn't exist. */
	FT_NONEXISTANT,
	/** A regular file or a symlink to a regular file. */
	FT_REGULAR,
	/** A directory. */
	FT_DIRECTORY,
	/** Something else, e.g. a pipe or a socket. */
	FT_OTHER
} FileType;

/**
 * Convenience shortcut for creating a <tt>shared_ptr</tt>.
 * Instead of:
 * @code
 *    shared_ptr<Foo> foo;
 *    ...
 *    foo = shared_ptr<Foo>(new Foo());
 * @endcode
 * one can write:
 * @code
 *    shared_ptr<Foo> foo;
 *    ...
 *    foo = ptr(new Foo());
 * @endcode
 *
 * @param pointer The item to put in the shared_ptr object.
 * @ingroup Support
 */
template<typename T> shared_ptr<T>
ptr(T *pointer) {
	return shared_ptr<T>(pointer);
}

/**
 * Used internally by toString(). Do not use directly.
 *
 * @internal
 */
template<typename T>
struct AnythingToString {
	string operator()(T something) {
		stringstream s;
		s << something;
		return s.str();
	}
};

/**
 * Used internally by toString(). Do not use directly.
 *
 * @internal
 */
template<>
struct AnythingToString< vector<string> > {
	string operator()(const vector<string> &v) {
		string result("[");
		vector<string>::const_iterator it;
		unsigned int i;
		for (it = v.begin(), i = 0; it != v.end(); it++, i++) {
			result.append("'");
			result.append(*it);
			if (i == v.size() - 1) {
				result.append("'");
			} else {
				result.append("', ");
			}
		}
		result.append("]");
		return result;
	}
};

/**
 * Convert anything to a string.
 *
 * @param something The thing to convert.
 * @ingroup Support
 */
template<typename T> string
toString(T something) {
	return AnythingToString<T>()(something);
}

/**
 * Converts the given string to an integer.
 * @ingroup Support
 */
int atoi(const string &s);

/**
 * Converts the given string to a long integer.
 * @ingroup Support
 */
long atol(const string &s);

/**
 * Split the given string using the given separator.
 *
 * @param str The string to split.
 * @param sep The separator to use.
 * @param output The vector to write the output to.
 * @ingroup Support
 */
void split(const string &str, char sep, vector<string> &output);

/**
 * Check whether the specified file exists.
 *
 * @param filename The filename to check.
 * @param mstat A CachedMultiFileStat object, if you want to use cached statting.
 * @param throttleRate A throttle rate for mstat. Only applicable if mstat is not NULL.
 * @return Whether the file exists.
 * @throws FileSystemException Unable to check because of a filesystem error.
 * @ingroup Support
 */
bool fileExists(const char *filename, CachedMultiFileStat *mstat = 0,
                unsigned int throttleRate = 0);

/**
 * Check whether 'filename' exists and what kind of file it is.
 *
 * @param filename The filename to check.
 * @param mstat A CachedMultiFileStat object, if you want to use cached statting.
 * @param throttleRate A throttle rate for mstat. Only applicable if mstat is not NULL.
 * @return The file type.
 * @throws FileSystemException Unable to check because of a filesystem error.
 * @ingroup Support
 */
FileType getFileType(const char *filename, CachedMultiFileStat *mstat = 0,
                     unsigned int throttleRate = 0);

/**
 * Find the location of the Passenger spawn server script.
 * If passengerRoot is given, t T
 *
 * @param passengerRoot The Passenger root folder. If NULL is given, then
 *      the spawn server is found by scanning $PATH. For security reasons,
 *      only absolute paths are scanned.
 * @return An absolute path to the spawn server script, or
 *         an empty string on error.
 * @throws FileSystemException Unable to access parts of the filesystem.
 * @ingroup Support
 */
string findSpawnServer(const char *passengerRoot = NULL);

/**
 * Find the location of the Passenger ApplicationPool server
 * executable.
 *
 * @param passengerRoot The Passenger root folder.
 * @return An absolute path to the executable.
 * @throws FileSystemException Unable to access parts of the filesystem.
 * @pre passengerRoot != NULL
 * @ingroup Support
 */
string findApplicationPoolServer(const char *passengerRoot);

/**
 * Returns a canonical version of the specified path. All symbolic links
 * and relative path elements are resolved.
 *
 * @throws FileSystemException Something went wrong.
 * @ingroup Support
 */
string canonicalizePath(const string &path);

/**
 * If <em>path</em> refers to a symlink, then this function resolves the
 * symlink for 1 level. That is, if the symlink points to another symlink,
 * then the other symlink will not be resolved. The resolved path is returned.
 *
 * If the symlink doesn't point to an absolute path, then this function will
 * prepend <em>path</em>'s directory to the result.
 *
 * If <em>path</em> doesn't refer to a symlink then this method will return
 * <em>path</em>.
 *
 * @throws FileSystemException Something went wrong.
 * @ingroup Support
 */
string resolveSymlink(const string &path);

/**
 * Given a path, extracts its directory name.
 *
 * @ingroup Support
 */
string extractDirName(const string &path);

/**
 * Escape the given raw string into an XML value.
 *
 * @throws std::bad_alloc Something went wrong.
 * @ingroup Support
 */
string escapeForXml(const string &input);

/**
 * Return the path name for the directory in which temporary files are
 * to be stored.
 *
 * @ensure result != NULL
 * @ingroup Support
 */
const char *getTempDir();

/**
 * Return the path name for the directory in which Phusion Passenger-specific
 * temporary files are to be stored. This directory is unique for this instance
 * of the web server in which Phusion Passenger is running.
 *
 * The result will be cached into the PHUSION_PASSENGER_TMP environment variable,
 * which will be used for future calls to this function. To bypass this cache,
 * set 'bypassCache' to true.
 *
 * @ensure !result.empty()
 * @ingroup Support
 */
string getPassengerTempDir(bool bypassCache = false);

/* Create a temp folder for storing Phusion Passenger-specific temp files,
 * such as temporarily buffered uploads, sockets for backend processes, etc.
 * This method also sets the PHUSION_PASSENGER_TMP environment variable, which
 * allows backend processes to find this temp folder.
 *
 * This method follows the same rules as getPassengerTempDir() for determining
 * the temp folder.
 *
 * Does nothing if this folder already exists.
 *
 * @throws IOException Something went wrong.
 * @throws SystemException Something went wrong.
 */
void createPassengerTempDir();

/**
 * Create the directory at the given path, creating intermediate directories
 * if necessary. The created directories' permissions are as specified by the
 * 'mode' parameter.
 *
 * If 'path' already exists, then nothing will happen.
 *
 * @throws IOException Something went wrong.
 * @throws SystemException Something went wrong.
 */
void makeDirTree(const char *path, const char *mode = "u=rwx,g=,o=");

/**
 * Remove an entire directory tree recursively.
 *
 * @throws SystemException Something went wrong.
 */
void removeDirTree(const char *path);

/**
 * Check whether the specified directory is a valid Ruby on Rails
 * application root directory.
 *
 * @param mstat A CachedMultiFileStat object, if you want to use cached statting.
 * @param throttleRate A throttle rate for mstat. Only applicable if mstat is not NULL.
 * @throws FileSystemException Unable to check because of a system error.
 * @ingroup Support
 */
bool verifyRailsDir(const string &dir, CachedMultiFileStat *mstat = 0,
                    unsigned int throttleRate = 0);

/**
 * Check whether the specified directory is a valid Rack application
 * root directory.
 *
 * @param mstat A CachedMultiFileStat object, if you want to use cached statting.
 * @param throttleRate A throttle rate for mstat. Only applicable if mstat is not NULL.
 * @throws FileSystemException Unable to check because of a filesystem error.
 * @ingroup Support
 */
bool verifyRackDir(const string &dir, CachedMultiFileStat *mstat = 0,
                   unsigned int throttleRate = 0);

/**
 * Check whether the specified directory is a valid WSGI application
 * root directory.
 *
 * @param mstat A CachedMultiFileStat object, if you want to use cached statting.
 * @param throttleRate A throttle rate for mstat. Only applicable if mstat is not NULL.
 * @throws FileSystemException Unable to check because of a filesystem error.
 * @ingroup Support
 */
bool verifyWSGIDir(const string &dir, CachedMultiFileStat *mstat = 0,
                   unsigned int throttleRate = 0);

/**
 * Represents a temporary file. The associated file is automatically
 * deleted upon object destruction.
 *
 * @ingroup Support
 */
class TempFile {
public:
	/** The filename. If this temp file is anonymous, then the filename is an empty string. */
	string filename;
	/** The file handle. */
	FILE *handle;
	
	/**
	 * Create an empty, temporary file, and open it for reading and writing.
	 *
	 * @param anonymous Set to true if this temp file should be unlinked
	 *        immediately. Anonymous temp files are useful if one just wants
	 *        a big not-in-memory buffer to work with.
	 * @throws SystemException Something went wrong.
	 */
	TempFile(const char *identifier = "temp", bool anonymous = true) {
		char templ[PATH_MAX];
		int fd;
		
		snprintf(templ, sizeof(templ), "%s/%s.XXXXXX", getPassengerTempDir().c_str(), identifier);
		templ[sizeof(templ) - 1] = '\0';
		fd = mkstemp(templ);
		if (fd == -1) {
			char message[1024];
			snprintf(message, sizeof(message), "Cannot create a temporary file '%s'", templ);
			message[sizeof(message) - 1] = '\0';
			throw SystemException(message, errno);
		}
		if (anonymous) {
			fchmod(fd, 0000);
			unlink(templ);
		} else {
			filename.assign(templ);
		}
		handle = fdopen(fd, "w+");
	}
	
	~TempFile() {
		fclose(handle);
		if (!filename.empty()) {
			unlink(filename.c_str());
		}
	}
};

} // namespace Passenger

#endif /* _PASSENGER_UTILS_H_ */

