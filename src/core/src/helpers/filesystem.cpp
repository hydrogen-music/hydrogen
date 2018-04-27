
#include <hydrogen/config.h>
#include <hydrogen/helpers/filesystem.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>

// directories
#define LOCAL_DATA_PATH "data/"
#define CACHE           "cache/"
#define DEMOS           "demo_songs/"
#define DOC             "doc/"
#define DRUMKITS        "drumkits/"
#define I18N            "i18n/"
#define IMG             "img/"
#define PATTERNS        "patterns/"
#define PLAYLISTS       "playlists/"
#define PLUGINS         "plugins/"
#define REPOSITORIES    "repositories/"
#define SCRIPTS         "scripts/"
#define SONGS           "songs/"
#define TMP             "hydrogen/"
#define XSD             "xsd/"


// files
#define CLICK_SAMPLE    "click.wav"
#define EMPTY_SAMPLE    "emptySample.wav"
#define EMPTY_SONG      "DefaultSong.h2song"
#define USR_CONFIG		"hydrogen.conf"
#define SYS_CONFIG		"hydrogen.default.conf"
#define DRUMKIT_XML     "drumkit.xml"
#define DRUMKIT_XSD     "drumkit.xsd"
#define DRUMPAT_XSD     "drumkit_pattern.xsd"

#define AUTOSAVE        "autosave"

#define UNTITLED_SONG		"untitled.h2song"
#define UNTITLED_PLAYLIST	"untitled.h2playlist"

// filters
#define PATTERN_FILTER  "*.h2pattern"
#define PLAYLIST_FILTER "*.h2playlist"
#define SONG_FILTER     "*.h2song"

namespace H2Core
{

Logger* Filesystem::__logger = 0;
const char* Filesystem::__class_name = "Filesystem";

const QString Filesystem::scripts_ext = ".sh";
const QString Filesystem::songs_ext = ".h2song";
const QString Filesystem::patterns_ext = ".h2pattern";
const QString Filesystem::playlist_ext = ".h2playlist";
const QString Filesystem::scripts_filter_name = "Hydrogen Scripts (*.sh)";
const QString Filesystem::songs_filter_name = "Hydrogen Songs (*.h2song)";
const QString Filesystem::patterns_filter_name = "Hydrogen Patterns (*.h2pattern)";
const QString Filesystem::playlists_filter_name = "Hydrogen Playlists (*.h2playlist)";

QString Filesystem::__sys_data_path;
QString Filesystem::__usr_data_path;
QString Filesystem::__usr_cfg_path;
QStringList Filesystem::__ladspa_paths;


/* TODO QCoreApplication is not instanciated */
bool Filesystem::bootstrap( Logger* logger, const QString& sys_path )
{
	if( __logger==0 && logger!=0 ) {
		__logger = logger;
	} else {
		return false;
	}

#ifdef Q_OS_MACX
#ifdef H2CORE_HAVE_BUNDLE
	// Bundle: Prepare hydrogen to use path names which are used in app bundles: http://en.wikipedia.org/wiki/Application_Bundle
	__sys_data_path = QCoreApplication::applicationDirPath().append( "/../Resources/data/" ) ;
#else
	__sys_data_path = QCoreApplication::applicationDirPath().append( "/data/" ) ;
#endif
	__usr_data_path = QDir::homePath().append( "/Library/Application Support/Hydrogen/data/" );
	__usr_cfg_path = QDir::homePath().append( "/Library/Application Support/Hydrogen/" USR_CONFIG );
#elif WIN32
	__sys_data_path = QCoreApplication::applicationDirPath().append( "/data/" ) ;
	__usr_data_path = QDir::homePath().append( "/.hydrogen/data/" ) ;
	__usr_cfg_path = QDir::homePath().append( "/.hydrogen/" USR_CONFIG ) ;
#else
	__sys_data_path = H2_SYS_PATH "/data/";
	__usr_data_path = QDir::homePath().append( "/" H2_USR_PATH "/data/" );
	__usr_cfg_path = QDir::homePath().append( "/" H2_USR_PATH "/" USR_CONFIG );
#endif
	if( sys_path!=0 ) __sys_data_path = sys_path;

	if( !dir_readable( __sys_data_path ) ) {
		__sys_data_path = QCoreApplication::applicationDirPath().append( LOCAL_DATA_PATH );
		ERRORLOG( QString( "will use local data path : %1" ).arg( __sys_data_path ) );
	}

	char* ladspaPath = getenv( "LADSPA_PATH" );
	if ( ladspaPath ) {
		INFOLOG( "Found LADSPA_PATH environment variable" );
		QString sLadspaPath = QString::fromLocal8Bit( ladspaPath );
		int pos;
		while ( ( pos = sLadspaPath.indexOf( ":" ) ) != -1 ) {
			QString sPath = sLadspaPath.left( pos );
			__ladspa_paths << QFileInfo(sPath).canonicalFilePath();
			sLadspaPath = sLadspaPath.mid( pos + 1, sLadspaPath.length() );
		}
		__ladspa_paths << QFileInfo( sLadspaPath ).canonicalFilePath();
	} else {
#ifdef Q_OS_MACX
		__ladspa_paths << QFileInfo( QCoreApplication::applicationDirPath(), "/../Resources/plugins" ).canonicalFilePath();
		__ladspa_paths << QFileInfo( "/Library/Audio/Plug-Ins/LADSPA/" ).canonicalFilePath();
		__ladspa_paths << QFileInfo( QDir::homePath(), "/Library/Audio/Plug-Ins/LADSPA" ).canonicalFilePath();
#else
		__ladspa_paths << QFileInfo( "/usr/lib/ladspa" ).canonicalFilePath();
		__ladspa_paths << QFileInfo( "/usr/local/lib/ladspa" ).canonicalFilePath();
		__ladspa_paths << QFileInfo( "/usr/lib64/ladspa" ).canonicalFilePath();
		__ladspa_paths << QFileInfo( "/usr/local/lib64/ladspa" ).canonicalFilePath();
#endif
	}
	__ladspa_paths.sort();
	__ladspa_paths.removeDuplicates();
	if ( !__ladspa_paths.isEmpty() && __ladspa_paths.at( 0 ).isEmpty() )
		__ladspa_paths.removeFirst();
	// we want this first
	__ladspa_paths << Filesystem::plugins_dir();
	__ladspa_paths.removeDuplicates();

	bool ret = check_sys_paths();
	ret &= check_usr_paths();
	info();
	return ret;
}

bool Filesystem::check_permissions( const QString& path, const int perms, bool silent )
{
	QFileInfo fi( path );
	if( ( perms & is_file ) && ( perms & is_writable ) && !fi.exists() ) {
		QFileInfo folder( path.left( path.lastIndexOf( "/" ) ) );
		if( !folder.isDir() ) {
			if( !silent ) ERRORLOG( QString( "%1 is not a directory" ).arg( folder.fileName() ) );
			return false;
		}
		if( !folder.isWritable() ) {
			if( !silent ) ERRORLOG( QString( "%1 is not writable" ).arg( folder.fileName() ) );
			return false;
		}
		return true;
	}
	if( ( perms & is_dir ) && !fi.isDir() ) {
		if( !silent ) ERRORLOG( QString( "%1 is not a directory" ).arg( path ) );
		return false;
	}
	if( ( perms & is_file ) && !fi.isFile() ) {
		if( !silent ) ERRORLOG( QString( "%1 is not a file" ).arg( path ) );
		return false;
	}
	if( ( perms & is_readable ) && !fi.isReadable() ) {
		if( !silent ) ERRORLOG( QString( "%1 is not readable" ).arg( path ) );
		return false;
	}
	if( ( perms & is_writable ) && !fi.isWritable() ) {
		if( !silent ) ERRORLOG( QString( "%1 is not writable" ).arg( path ) );
		return false;
	}
	if( ( perms & is_executable ) && !fi.isExecutable() ) {
		if( !silent ) ERRORLOG( QString( "%1 is not executable" ).arg( path ) );
		return false;
	}
	return true;
}

bool Filesystem::file_exists( const QString& path, bool silent )
{
	return check_permissions( path, is_file, silent );
}
bool Filesystem::file_readable( const QString& path, bool silent )
{
	return check_permissions( path, is_file|is_readable, silent );
}
bool Filesystem::file_writable( const QString& path, bool silent )
{
	return check_permissions( path, is_file|is_readable|is_writable, silent );
}
bool Filesystem::file_executable( const QString& path, bool silent )
{
	return check_permissions( path, is_file|is_executable, silent );
}
bool Filesystem::dir_readable(  const QString& path, bool silent )
{
	return check_permissions( path, is_dir|is_readable|is_executable, silent );
}
bool Filesystem::dir_writable(  const QString& path, bool silent )
{
	return check_permissions( path, is_dir|is_writable, silent );
}

bool Filesystem::mkdir( const QString& path )
{
	if ( !QDir( "/" ).mkpath( QDir( path ).absolutePath() ) ) {
		ERRORLOG( QString( "unable to create directory : %1" ).arg( path ) );
		return false;
	}
	return true;
}

bool Filesystem::path_usable( const QString& path, bool create, bool silent )
{
	if( !QDir( path ).exists() ) {
		if( !silent ) INFOLOG( QString( "create user directory : %1" ).arg( path ) );
		if( create && !QDir( "/" ).mkpath( path ) ) {
			if( !silent ) ERRORLOG( QString( "unable to create user directory : %1" ).arg( path ) );
			return false;
		}
	}
	return dir_readable( path, silent ) && dir_writable( path, silent );
}

bool Filesystem::write_to_file( const QString& dst, const QString& content )
{
	if ( !file_writable( dst ) ) {
		ERRORLOG( QString( "unable to write to %1" ).arg( dst ) );
		return false;
	}
	QFile file( dst );
	if ( !file.open( QIODevice::WriteOnly ) ) {
		ERRORLOG( QString( "unable to write to %1" ).arg( dst ) );
		return false;
	}
	file.write( content.toUtf8().data() );
	file.close();

	return true;
}

bool Filesystem::file_copy( const QString& src, const QString& dst, bool overwrite )
{
	if( !overwrite && file_exists( dst, true ) ) {
		WARNINGLOG( QString( "do not overwrite %1 with %2 as it already exists" ).arg( dst ).arg( src ) );
		return true;
	}
	if ( !file_readable( src ) ) {
		ERRORLOG( QString( "unable to copy %1 to %2, %1 is not readable" ).arg( src ).arg( dst ) );
		return false;
	}
	if ( !file_writable( dst ) ) {
		ERRORLOG( QString( "unable to copy %1 to %2, %2 is not writable" ).arg( src ).arg( dst ) );
		return false;
	}
	INFOLOG( QString( "copy %1 to %2" ).arg( src ).arg( dst ) );
	return QFile::copy( src,dst );
}

bool Filesystem::rm( const QString& path, bool recursive )
{
	if ( check_permissions( path, is_file, true ) ) {
		QFile file( path );
		bool ret = file.remove();
		if( !ret ) {
			ERRORLOG( QString( "unable to remove file %1" ).arg( path ) );
		}
		return ret;
	}
	if ( !check_permissions( path, is_dir, true )  ) {
		ERRORLOG( QString( "%1 is neither a file nor a directory ?!?!" ).arg( path ) );
		return false;
	}
	if ( !recursive ) {
		QDir dir;
		bool ret = dir.rmdir( path );
		if( !ret ) {
			ERRORLOG( QString( "unable to remove dir %1 without recursive argument, maybe it is not empty?" ).arg( path ) );
		}
		return ret;
	}
	return rm_fr( path );
}

bool Filesystem::rm_fr( const QString& path )
{
	bool ret = true;
	QDir dir( path );
	QFileInfoList entries = dir.entryInfoList( QDir::NoDotAndDotDot | QDir::AllEntries );
	for ( int idx = 0; ( ( idx < entries.size() ) && ret ); idx++ ) {
		QFileInfo entryInfo = entries[idx];
		if ( entryInfo.isDir() && !entryInfo.isSymLink() ) {
			ret = rm_fr( entryInfo.absoluteFilePath() );
		} else {
			QFile file( entryInfo.absoluteFilePath() );
			if ( !file.remove() ) {
				ERRORLOG( QString( "unable to remove %1" ).arg( entryInfo.absoluteFilePath() ) );
				ret = false;
			}
		}
	}
	if ( !dir.rmdir( dir.absolutePath() ) ) {
		ERRORLOG( QString( "unable to remove %1" ).arg( dir.absolutePath() ) );
		ret = false;
	}
	return ret;
}

bool Filesystem::check_sys_paths()
{
	bool ret = true;
	if(  !dir_readable( __sys_data_path ) ) ret = false;
	if( !file_readable( click_file_path() ) ) ret = false;
	if( !file_readable( empty_song_path() ) ) ret = false;
	if(  !dir_readable( demos_dir() ) ) ret = false;
	/* if(  !dir_readable( doc_dir() ) ) ret = false; */		// FIXME
	if(  !dir_readable( sys_drumkits_dir() ) ) ret = false;
	if( !file_readable( empty_sample_path() ) ) ret = false;
	if( !file_readable( sys_config_path() ) ) ret = false;
	if(  !dir_readable( i18n_dir() ) ) ret = false;
	if(  !dir_readable( img_dir() ) ) ret = false;
	if(  !dir_readable( xsd_dir() ) ) ret = false;
	if( !file_readable( pattern_xsd_path() ) ) ret = false;
	if( !file_readable( drumkit_xsd_path() ) ) ret = false;

	if ( ret ) INFOLOG( QString( "system wide data path %1 is usable." ).arg( __sys_data_path ) );
	return ret;
}


bool Filesystem::check_usr_paths()
{
	bool ret = true;
	if( !path_usable( tmp_dir() ) ) ret = false;
	if( !path_usable( __usr_data_path ) ) ret = false;
	if( !path_usable( cache_dir() ) ) ret = false;
	if( !path_usable( repositories_cache_dir() ) ) ret = false;
	if( !path_usable( usr_drumkits_dir() ) ) ret = false;
	if( !path_usable( patterns_dir() ) ) ret = false;
	if( !path_usable( playlists_dir() ) ) ret = false;
	if( !path_usable( plugins_dir() ) ) ret = false;
	if( !path_usable( scripts_dir() ) ) ret = false;
	if( !path_usable( songs_dir() ) ) ret = false;
	if( !file_writable( usr_config_path() ) ) ret = false;

	if ( ret ) INFOLOG( QString( "user path %1 is usable." ).arg( __usr_data_path ) );
	return ret;
}

QString Filesystem::sys_data_path()
{
	return __sys_data_path;
}
QString Filesystem::usr_data_path()
{
	return __usr_data_path;
}

QStringList Filesystem::ladspa_paths()
{
	return __ladspa_paths;
}

// FILES
QString Filesystem::sys_config_path()
{
       return __sys_data_path + SYS_CONFIG;
}
QString Filesystem::usr_config_path()
{
       return __usr_cfg_path;
}
QString Filesystem::empty_sample_path()
{
	return __sys_data_path + EMPTY_SAMPLE;
}
QString Filesystem::empty_song_path()
{
	return __sys_data_path + EMPTY_SONG;
}
QString Filesystem::untitled_song_file_name()
{
	return UNTITLED_SONG;
}
QString Filesystem::untitled_playlist_file_name()
{
	return UNTITLED_PLAYLIST;
}
QString Filesystem::click_file_path()
{
	return __sys_data_path + CLICK_SAMPLE;
}
QString Filesystem::usr_click_file_path()
{
	if( file_readable( __usr_data_path + CLICK_SAMPLE, true ) ) return __usr_data_path + CLICK_SAMPLE;
	return click_file_path();
}
QString Filesystem::drumkit_xsd_path( )
{
	return xsd_dir() + DRUMKIT_XSD;
}
QString Filesystem::pattern_xsd_path( )
{
	return xsd_dir() + DRUMPAT_XSD;
}

// DIRS
QString Filesystem::img_dir()
{
	return __sys_data_path + IMG;
}
QString Filesystem::doc_dir()
{
	return __sys_data_path + DOC;
}
QString Filesystem::i18n_dir()
{
	return __sys_data_path + I18N;
}
QString Filesystem::scripts_dir()
{
	return __usr_data_path + SCRIPTS;
}
QString Filesystem::songs_dir()
{
	return __usr_data_path + SONGS;
}
QString Filesystem::song_path( const QString& sg_name )
{
	return QString( songs_dir() + sg_name + songs_ext );
}
QString Filesystem::patterns_dir()
{
	return __usr_data_path + PATTERNS;
}
QString Filesystem::patterns_dir( const QString& dk_name )
{
	return __usr_data_path + PATTERNS + dk_name + "/";
}
QString Filesystem::pattern_path( const QString& dk_name, const QString& p_name )
{
	if ( dk_name.isEmpty() ) {
		return patterns_dir() + p_name + patterns_ext;
	} else {
		return patterns_dir( dk_name ) + p_name + patterns_ext;
	}
}
QString Filesystem::plugins_dir()
{
	return __usr_data_path + PLUGINS;
}
QString Filesystem::sys_drumkits_dir()
{
	return __sys_data_path + DRUMKITS;
}
QString Filesystem::usr_drumkits_dir()
{
	return __usr_data_path + DRUMKITS;
}
QString Filesystem::playlists_dir()
{
	return __usr_data_path + PLAYLISTS;
}
QString Filesystem::playlist_path( const QString& pl_name )
{
	return patterns_dir() + pl_name + playlist_ext;
}
QString Filesystem::cache_dir()
{
	return __usr_data_path + CACHE;
}
QString Filesystem::repositories_cache_dir()
{
	return __usr_data_path + CACHE + REPOSITORIES;
}
QString Filesystem::demos_dir()
{
	return __sys_data_path + DEMOS;
}
QString Filesystem::xsd_dir()
{
	return __sys_data_path + XSD;
}
QString Filesystem::tmp_dir()
{
	return QDir::tempPath() + "/" + TMP;
}
QString Filesystem::tmp_file_path( const QString& base )
{
	QTemporaryFile file( tmp_dir() + base );
	file.setAutoRemove( false );
	file.open();
	file.close();
	return file.fileName();
}

// DRUMKITS
QStringList Filesystem::drumkit_list( const QString& path )
{
	QStringList ok;
	QStringList possible = QDir( path ).entryList( QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot );
	foreach ( const QString& dk, possible ) {
		if ( drumkit_valid( path + dk ) ) {
			ok << dk;
		} else {
			ERRORLOG( QString( "drumkit %1 is not usable" ).arg( dk ) );
		}
	}
	return ok;
}
QStringList Filesystem::sys_drumkit_list( )
{
	return drumkit_list( sys_drumkits_dir() ) ;
}
QStringList Filesystem::usr_drumkit_list( )
{
	return drumkit_list( usr_drumkits_dir() ) ;
}

bool Filesystem::file_is_partof_drumkit( const QString& fname )
{
	if( fname.startsWith( usr_drumkits_dir() ) )
	{
		int start = usr_drumkits_dir().size();
		int index = fname.indexOf( "/", start + 1 );
		QString dkname = fname.midRef( start + 1, index - start - 1 ).toString();
		if(drumkit_exists(dkname))
			return true;
	}


	if( fname.startsWith( sys_drumkits_dir() ) )
	{
		int start = sys_drumkits_dir().size();
		int index = fname.indexOf( "/", start + 1 );
		QString dkname = fname.midRef( start + 1, index - start - 1 ).toString();
		if(drumkit_exists(dkname))
			return true;
	}
	return false;
}


bool Filesystem::drumkit_exists( const QString& dk_name )
{
	if( usr_drumkit_list().contains( dk_name ) ) return true;
	return sys_drumkit_list().contains( dk_name );
}
QString Filesystem::drumkit_usr_path( const QString& dk_name )
{
	return usr_drumkits_dir() + dk_name;
}
QString Filesystem::drumkit_path_search( const QString& dk_name )
{
	if( usr_drumkit_list().contains( dk_name ) ) return usr_drumkits_dir() + dk_name;
	if( sys_drumkit_list().contains( dk_name ) ) return sys_drumkits_dir() + dk_name;
	ERRORLOG( QString( "drumkit %1 not found" ).arg( dk_name ) );
	return "";
}
QString Filesystem::drumkit_dir_search( const QString& dk_name )
{
	if( usr_drumkit_list().contains( dk_name ) ) return usr_drumkits_dir();
	if( sys_drumkit_list().contains( dk_name ) ) return sys_drumkits_dir();
	ERRORLOG( QString( "drumkit %1 not found" ).arg( dk_name ) );
	return "";
}
bool Filesystem::drumkit_valid( const QString& dk_path )
{
	return file_readable( dk_path + "/" + DRUMKIT_XML, true);
}
QString Filesystem::drumkit_file( const QString& dk_path )
{
	return dk_path + "/" + DRUMKIT_XML;
}

// PATTERNS
QStringList Filesystem::pattern_drumkits()
{
	return QDir( patterns_dir() ).entryList( QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot );
}

QStringList Filesystem::pattern_list()
{
	return pattern_list( patterns_dir() );
}

QStringList Filesystem::pattern_list( const QString& path)
{
	return QDir( path ).entryList( QStringList( PATTERN_FILTER ), QDir::Files | QDir::Readable | QDir::NoDotAndDotDot );
}

// SONGS
QStringList Filesystem::song_list( )
{
	return QDir( songs_dir() ).entryList( QStringList( SONG_FILTER ), QDir::Files | QDir::Readable | QDir::NoDotAndDotDot );
}

QStringList Filesystem::song_list_cleared( )
{
	QStringList result;
	foreach ( const QString& str, song_list() ) {
		if ( !str.contains( AUTOSAVE ) )
			result += str;
	}
	return result;
}

bool Filesystem::song_exists( const QString& sg_name )
{
	return QDir( songs_dir() ).exists( sg_name );
}

// PLAYLISTS
QStringList Filesystem::playlist_list( )
{
	return QDir( playlists_dir() ).entryList( QStringList( PLAYLIST_FILTER ), QDir::Files | QDir::Readable | QDir::NoDotAndDotDot );
}

void Filesystem::info()
{
	INFOLOG( QString( "Tmp dir                    : %1" ).arg( tmp_dir() ) );
	// SYS
	INFOLOG( QString( "Click file                 : %1" ).arg( click_file_path() ) );
	INFOLOG( QString( "Empty song                 : %1" ).arg( empty_song_path() ) );
	INFOLOG( QString( "Demos dir                  : %1" ).arg( demos_dir() ) );
	INFOLOG( QString( "Documentation dir          : %1" ).arg( doc_dir() ) );					// FIXME must be created even if no doc deployed
	INFOLOG( QString( "System drumkit dir         : %1" ).arg( sys_drumkits_dir() ) );
	INFOLOG( QString( "Empty sample               : %1" ).arg( empty_sample_path() ) );
	INFOLOG( QString( "Default config             : %1" ).arg( sys_config_path() ) );
	INFOLOG( QString( "Internationalization dir   : %1" ).arg( i18n_dir() ) );
	INFOLOG( QString( "Images dir                 : %1" ).arg( img_dir() ) );
	// new_tutorial
	INFOLOG( QString( "XSD dir                    : %1" ).arg( xsd_dir() ) );
	INFOLOG( QString( "drumkit pattern XSD        : %1" ).arg( pattern_xsd_path() ) );
	INFOLOG( QString( "drumkit XSD                : %1" ).arg( drumkit_xsd_path() ) );
	// USR
	INFOLOG( QString( "User config                : %1" ).arg( usr_config_path() ) );			// FIXME
	INFOLOG( QString( "User Click file            : %1" ).arg( usr_click_file_path() ) );
	INFOLOG( QString( "Cache dir                  : %1" ).arg( cache_dir() ) );
	INFOLOG( QString( "Reporitories Cache dir     : %1" ).arg( repositories_cache_dir() ) );
	INFOLOG( QString( "User drumkit dir           : %1" ).arg( usr_drumkits_dir() ) );
	INFOLOG( QString( "Patterns dir               : %1" ).arg( patterns_dir() ) );
	INFOLOG( QString( "Playlist dir               : %1" ).arg( playlists_dir() ) );
	INFOLOG( QString( "Plugins dir                : %1" ).arg( plugins_dir() ) );
	INFOLOG( QString( "Scripts dir                : %1" ).arg( scripts_dir() ) );
	INFOLOG( QString( "Songs dir                  : %1" ).arg( songs_dir() ) );
}

};

/* vim: set softtabstop=4 noexpandtab: */
