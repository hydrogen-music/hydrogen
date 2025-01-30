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

#ifndef H2C_FILES_H
#define H2C_FILES_H

#include <core/Object.h>
#include <QtCore/QString>

namespace H2Core
{

class Pattern;
class Playlist;
class Song;

/**
 * Files is in charge of writing and reading Patterns, Drumkits, Songs to the filesystem
 */
/** \ingroup docCore*/
class Files : public H2Core::Object<Files>
{
		H2_OBJECT(Files)
	public:
		enum SaveMode {
			SAVE_NEW,				// construct regular path, do not overwrite
			SAVE_OVERWRITE,			// construct regular path, overwrite existing file
			SAVE_PATH,				// given filename is the path
			SAVE_TMP,				// construct the path in save
		};

		/**
		 * save the given pattern to \<user_data_path\>/pattern/\<drumkitName\>/\<fileName\>.h2pattern
		 * will NOT overwrite an existing file
		 * \param fileName to build filePath from
		 * \param pattern the one to be saved
		 * \param song to access license, author info
		 * \param drumkitName to build filePath from and to access name info
		 * \return filePath on success, NULL on failure
		 */
		static inline QString savePatternNew( const QString& fileName, Pattern* pattern, std::shared_ptr<Song> song, const QString& drumkitName )
		{
			return savePattern( SAVE_NEW, fileName, pattern, song, drumkitName );
		}

		/**
		 * save the given pattern to \<user_data_path\>/pattern/\<drumkitName\>/\<fileName\>.h2pattern
		 * will overwrite an existing file
		 * \param fileName to build filePath from
		 * \param pattern the one to be saved
		 * \param song to access license, author info
		 * \param drumkitName to build filePath from and to access name info
		 * \return filePath on success, NULL on failure
		 */
		static inline QString savePatternOver( const QString& fileName, Pattern* pattern, std::shared_ptr<Song> song, const QString& drumkitName )
		{
			return savePattern( SAVE_OVERWRITE, fileName, pattern, song, drumkitName );
		}

		/**
		 * save the given pattern to @a filePath
		 * will overwrite an existing file
		 * \param filePath to write the pattern to
		 * \param pattern the one to be saved
		 * \param song to access license, author info
		 * \param drumkitName to access name info
		 * \return @a filePath on success, NULL on failure
		 */
		static inline QString savePatternPath( const QString& filePath, Pattern* pattern, std::shared_ptr<Song> song, const QString& drumkitName )
		{
			return savePattern( SAVE_PATH, filePath, pattern, song, drumkitName );
		}

		/**
		 * save the given pattern under \<Tmp_directory\> with a unique filename built from \<fileName\>
		 * will overwrite an existing file
		 * \param fileName to build filePath from
		 * \param pattern the one to be saved
		 * \param song to access license, author info
		 * \param drumkitName to access name info
		 * \return filePath on success, NULL on failure
		 */
		static inline QString savePatternTmp( const QString& fileName, Pattern* pattern, std::shared_ptr<Song> song, const QString& drumkitName )
		{
			return savePattern( SAVE_TMP, fileName, pattern, song, drumkitName );
		}

		/**
		 * save the given playlist to filePath
		 * will overwrite an existing file
		 * \param filePath to write the playlist to
		 * \param playlist the one to be saved
		 * \param relativePaths should the path to the songs be relative to the playlist instead of absolute
		 * \return filePath on success, NULL on failure
		 */
		static inline QString savePlaylistPath( const QString& filePath, Playlist* playlist, bool relativePaths )
		{
			return savePlaylist( SAVE_PATH, filePath, playlist, relativePaths );
		}

	private:
		static QString savePattern( SaveMode mode, const QString& fileName, const Pattern* pattern, std::shared_ptr<Song> song, const QString& drumkitName );
		static QString savePlaylist( SaveMode mode, const QString& fileName, Playlist* playlist, bool relativePaths );
};

};

#endif  // H2C_FILES_H

/* vim: set softtabstop=4 noexpandtab: */
