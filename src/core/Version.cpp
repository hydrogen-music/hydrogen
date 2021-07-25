/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <core/config.h>
#include "Version.h"

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
