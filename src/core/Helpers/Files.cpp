/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Helpers/Files.h>
#include <core/Helpers/Filesystem.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>

namespace H2Core
{


	QString Files::savePattern( SaveMode mode, const QString& fileName, const Pattern* pPattern, std::shared_ptr<Song> pSong, const QString& drumkitName )
	{
		QFileInfo fileInfo;

		switch ( mode ) {
			case SAVE_NEW:
			case SAVE_OVERWRITE:
				fileInfo = Filesystem::pattern_path( drumkitName, fileName );
				break;
			case SAVE_PATH:
				fileInfo = fileName;
				break;
			case SAVE_TMP:
				fileInfo = Filesystem::tmp_file_path( fileName );
				break;
			default:
				ERRORLOG( QString( "unknown mode : %1" ).arg( mode ) );
				return nullptr;
				break;
		}

		if ( mode == SAVE_NEW && Filesystem::file_exists( fileInfo.absoluteFilePath(), true ) ) {
			return nullptr;
		}

		if ( !Filesystem::path_usable( fileInfo.path(), true, false ) ) {
			return nullptr;
		}

		if ( !pPattern->save_file( drumkitName, pSong->getAuthor(), pSong->getLicense(), fileInfo.absoluteFilePath(), true ) ) {
			return nullptr;
		}

		return fileInfo.absoluteFilePath();
	}

	QString Files::savePlaylist( SaveMode mode, const QString& fileName, Playlist* playlist, bool relativePaths )
	{
		QFileInfo fileInfo;

		switch ( mode ) {
			case SAVE_NEW:
			case SAVE_OVERWRITE:
				fileInfo = Filesystem::playlist_path( fileName );
				break;
			case SAVE_PATH:
				fileInfo = fileName;
				break;
			case SAVE_TMP:
				fileInfo = Filesystem::tmp_file_path( fileName );
				break;
			default:
				ERRORLOG( QString( "unknown mode : %1" ).arg( mode ) );
				return nullptr;
				break;
		}

		if ( mode == SAVE_NEW && Filesystem::file_exists( fileInfo.absoluteFilePath(), false ) ) {
			return nullptr;
		}

		if ( !Filesystem::path_usable( fileInfo.path(), true, false ) ) {
			return nullptr;
		}

		if ( !playlist->save_file( fileInfo.absoluteFilePath(), fileInfo.fileName(), true, relativePaths) ) {
			return nullptr;
		}

		return fileInfo.absoluteFilePath();
	}
};

/* vim: set softtabstop=4 noexpandtab: */
