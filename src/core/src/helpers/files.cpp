
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
		QString filepath;

		switch ( mode ) {
			case SAVE_NEW:
			case SAVE_OVERWRITE:
				filepath = Filesystem::pattern_path( drumkit_name, filename );
				break;
			case SAVE_PATH:
				filepath = filename;
				break;
			case SAVE_TMP:
				filepath = Filesystem::tmp_file( filename );
			default:
				ERRORLOG( QString( "unknown mode : %1" ).arg( mode ) );
				break;
		}

		INFOLOG( QString( " write to %1" ).arg( filepath ) );

		if ( !pattern->save_file( drumkit_name, song->get_author(), song->get_license(), filepath, (mode != SAVE_NEW ) ) )
			return NULL;

		return filepath;
	}

};

/* vim: set softtabstop=4 noexpandtab: */
