
#include <hydrogen/config.h>
#include <hydrogen/helpers/files.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/playlist.h>
#include <hydrogen/basics/song.h>

namespace H2Core
{

	const char* Files::__class_name = "Files";

	QString Files::savePattern( SaveMode mode, const QString& fileName, const Pattern* pPattern, Song* pSong, const QString& drumkitName )
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
				return NULL;
				break;
		}

		if ( mode == SAVE_NEW && Filesystem::file_exists( fileInfo.absoluteFilePath(), false ) ) {
			return NULL;
		}

		if ( !Filesystem::path_usable( fileInfo.path(), true, false ) ) {
			return NULL;
		}

		if ( !pPattern->save_file( drumkitName, pSong->get_author(), pSong->get_license(), fileInfo.absoluteFilePath(), true ) )
			return NULL;

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
				return NULL;
				break;
		}

		if ( mode == SAVE_NEW && Filesystem::file_exists( fileInfo.absoluteFilePath(), false ) ) {
			return NULL;
		}

		if ( !Filesystem::path_usable( fileInfo.path(), true, false ) ) {
			return NULL;
		}

		if ( !playlist->save_file( fileInfo.absoluteFilePath(), fileInfo.fileName(), true, relativePaths) )
			return NULL;

		return fileInfo.absoluteFilePath();
	}
};

/* vim: set softtabstop=4 noexpandtab: */
