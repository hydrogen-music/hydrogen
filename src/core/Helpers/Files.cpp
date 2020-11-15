
#include <core/config.h>
#include <core/Helpers/Files.h>
#include <core/Helpers/Filesystem.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>

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
				return nullptr;
				break;
		}

		if ( mode == SAVE_NEW && Filesystem::file_exists( fileInfo.absoluteFilePath(), false ) ) {
			return nullptr;
		}

		if ( !Filesystem::path_usable( fileInfo.path(), true, false ) ) {
			return nullptr;
		}

		if ( !pPattern->save_file( drumkitName, pSong->get_author(), pSong->get_license(), fileInfo.absoluteFilePath(), true ) ) {
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
