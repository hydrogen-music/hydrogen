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

#ifndef H2C_LEGACY_H
#define H2C_LEGACY_H

#include <core/Object.h>

namespace H2Core {

class Drumkit;
class Playlist;
class Pattern;
class InstrumentList;

/**
 * Legacy is a container for legacy code which should be once removed
 */
class Legacy : public H2Core::Object {
		H2_OBJECT
	public:
		/**
		 * load drumkit information from a file
		 * \param dk_path is a path to an xml file
		 * \return a Drumkit on success, 0 otherwise
		 */
		static Drumkit* load_drumkit( const QString& dk_path );
		/**
		 * load pattern from a file
		 * \param pattern_path is a path to an xml file
		 * \param instrList
		 * \return a Pattern on success, 0 otherwise
		 */
		static Pattern* load_drumkit_pattern( const QString& pattern_path, InstrumentList* instrList );
		/**
		 * load playlist from a file
		 * \param pl the playlist to feed
		 * \param pl_path is a path to an xml file
		 * \return a Playlist on success, 0 otherwise
		 */
		static Playlist* load_playlist( Playlist* pl, const QString& pl_path );
};

};

#endif  // H2C_LEGACY_H

/* vim: set softtabstop=4 noexpandtab: */
