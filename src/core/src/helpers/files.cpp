
#include <hydrogen/config.h>
#include <hydrogen/helpers/files.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/song.h>

namespace H2Core
{

	const char* Files::__class_name = "Files";

	QString Files::savePattern( SaveMode mode, const QString& filename, Pattern* pattern, Song* song, const QString& drumkit_name )
	{
		QFileInfo fileInfo;

		switch ( mode ) {
			case SAVE_NEW:
			case SAVE_OVERWRITE:
				fileInfo = Filesystem::pattern_path( drumkit_name, filename );
				break;
			case SAVE_PATH:
				fileInfo = filename;
				break;
			case SAVE_TMP:
				fileInfo = Filesystem::tmp_file( filename );
				break;
			default:
				ERRORLOG( QString( "unknown mode : %1" ).arg( mode ) );
				return;
				break;
		}

		if ( mode == SAVE_NEW && Filesystem::file_exists( fileInfo.absoluteFilePath(), false ) ) {
			return NULL;
		}

		if ( !Filesystem::path_usable( fileInfo.path(), true, false ) ) {
			return NULL;
		}

		if ( !pattern->save_file( drumkit_name, song->get_author(), song->get_license(), fileInfo.absoluteFilePath(), true ) )
			return NULL;

		return fileInfo.absoluteFilePath();
	}

};

/* vim: set softtabstop=4 noexpandtab: */
