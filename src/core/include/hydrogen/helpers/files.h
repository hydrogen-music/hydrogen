
#ifndef H2C_FILES_H
#define H2C_FILES_H

#include <hydrogen/object.h>
#include <QtCore/QString>

namespace H2Core
{

class Pattern;
class Song;

/**
 * Files is in charge of writing and reading Patterns, Drumkits, Songs to the filesystem
 */
class Files : public H2Core::Object
{
		H2_OBJECT
	public:
		enum SaveMode {
			SAVE_NEW,				// construct regular path, do not overwrite
			SAVE_OVERWRITE,			// construct regular path, overwrite existing file
			SAVE_PATH,				// given filename is the path
			SAVE_TMP,				// construct the path in save
		};

	static QString savePattern( SaveMode mode, const QString& filename, Pattern* pattern, Song* song, const QString& drumkit_name );
};

};

#endif  // H2C_FILES_H

/* vim: set softtabstop=4 noexpandtab: */
