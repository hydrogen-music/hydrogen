/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef SONG_INFO_H
#define SONG_INFO_H

#include <core/Object.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

namespace H2Core
{

class Hydrogen;

/**
 * @class SongInfo
 *
 * Holds meta data about a particular song in the Sound Library.
 *
 * The path #m_sPath serves as the unique identifier for each info item.
 *
 * \ingroup docCore docDataStructure */
class SongInfo : public SoundLibraryInfo, public H2Core::Object<SongInfo> {
	H2_OBJECT( SongInfo )
   public:
	SongInfo();
	~SongInfo();

	/**
	 * Reads the content found in @a sPath.
	 *
	 * @param sPath Path to a .h2song XML file
	 * @return `true` on success
	 */
	bool load( const QString& sPath, Hydrogen* pHydrogen );

	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;
};
};	// namespace H2Core

#endif	// SONG_INFO_H
