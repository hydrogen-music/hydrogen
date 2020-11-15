
#include "hydrogen/config.h"
#include "hydrogen/version.h"

namespace H2Core {

static const std::string version = H2CORE_VERSION;

std::string get_version() {
	return version;
}

bool version_older_than( int major, int minor, int patch ) {
	if ( H2CORE_VERSION_MAJOR > major ) {
		return true;
	} else if ( H2CORE_VERSION_MAJOR < major ) {
		return false;
	} else {
		if ( H2CORE_VERSION_MINOR > minor ) {
			return true;
		} else if ( H2CORE_VERSION_MINOR < minor ) {
			return false;
		} else {
			if ( H2CORE_VERSION_PATCH > patch ) {
				return true;
			} else {
				return false;
			}
		}
	}
}

};

/* vim: set softtabstop=4 noexpandtab: */
