
#ifndef H2C_VERSION_H
#define H2C_VERSION_H

#include <string>

namespace H2Core {

/// Returns the current Hydrogen version string
std::string get_version();

/**
 * return true of the current version is older than the given values
 */
bool version_older_than( int major, int minor, int patch );

#endif // H2C_VERSION

};

/* vim: set softtabstop=4 noexpandtab: */
