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

#include <random>

#include <core/Basics/Drumkit.h>
#include <core/config.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QDateTime>
#include <QRegularExpression>

#ifdef H2CORE_HAVE_OSC
#include <core/NsmClient.h>
#endif

// directories
#define LOCAL_DATA_PATH "data/"
#define CACHE           "cache/"
#define DEMOS           "demo_songs/"
#define DOC             "doc/"
#define DRUMKITS        "drumkits/"
#define DRUMKIT_MAPS    "drumkit_maps/"
#define I18N            "i18n/"
#define IMG             "img/"
#define PATTERNS        "patterns/"
#define PLAYLISTS       "playlists/"
#define PLUGINS         "plugins/"
#define REPOSITORIES    "repositories/"
#define SCRIPTS         "scripts/"
#define SONGS           "songs/"
#define THEMES          "themes/"
#define TMP             "hydrogen/"
#define XSD             "xsd/"


// files
/** Sound of metronome beat */
#define CLICK_SAMPLE    "click.wav"
#define EMPTY_SAMPLE    "emptySample.wav"
#define DEFAULT_SONG    "DefaultSong"
#define DEFAULT_PLAYLIST "DefaultPlaylist"
#define EMPTY_SONG_BASE "emptySong"
#define EMPTY_PLAYLIST_BASE "emptyPlaylist"
#define USR_CONFIG		"hydrogen.conf"
#define SYS_CONFIG		"hydrogen.default.conf"
#define LOG_FILE		"hydrogen.log"
#define DRUMKIT_XML     "drumkit.xml"
#define DRUMKIT_XSD     "drumkit.xsd"
#define DRUMKIT_MAP_XSD "drumkit_map.xsd"
#define DRUMPAT_XSD     "drumkit_pattern.xsd"
#define DRUMKIT_DEFAULT_KIT "GMRockKit"
#define PLAYLIST_XSD     "playlist.xsd"

#define AUTOSAVE        "autosave"

#define UNTITLED_SONG		"Untitled Song"
#define UNTITLED_PLAYLIST	"untitled.h2playlist"

// filters
#define PATTERN_FILTER  "*.h2pattern"
#define PLAYLIST_FILTER "*.h2playlist"
#define SONG_FILTER     "*.h2song"
#define THEME_FILTER    "*.h2theme"

namespace H2Core
{

Logger* Filesystem::__logger = nullptr;

const QString Filesystem::scripts_ext = ".sh";
const QString Filesystem::songs_ext = ".h2song";
const QString Filesystem::themes_ext = ".h2theme";
const QString Filesystem::patterns_ext = ".h2pattern";
const QString Filesystem::playlist_ext = ".h2playlist";
const QString Filesystem::drumkit_ext = ".h2drumkit";
const QString Filesystem::drumkit_map_ext = ".h2map";
const QString Filesystem::scripts_filter_name = "Hydrogen Scripts (*.sh)";
const QString Filesystem::drumkit_filter_name = "Hydrogen Drumkit (*.h2drumkit)";
const QString Filesystem::songs_filter_name = "Hydrogen Songs (*.h2song)";
const QString Filesystem::themes_filter_name = "Hydrogen Theme (*.h2theme)";
const QString Filesystem::patterns_filter_name = "Hydrogen Patterns (*.h2pattern)";
const QString Filesystem::playlists_filter_name = "Hydrogen Playlists (*.h2playlist)";

QString Filesystem::__sys_data_path;
QString Filesystem::__usr_data_path;
QString Filesystem::__usr_cfg_path;

#ifdef Q_OS_MACX
	QString Filesystem::__usr_log_path =QDir::homePath().append( "/Library/Application Support/Hydrogen/" LOG_FILE );
#elif WIN32
	QString Filesystem::__usr_log_path = QDir::homePath().append( "/.hydrogen/" LOG_FILE );
#else
	QString Filesystem::__usr_log_path = QDir::homePath().append( "/" H2_USR_PATH "/" LOG_FILE);
#endif


QStringList Filesystem::__ladspa_paths;

QString Filesystem::m_sPreferencesOverwritePath = "";

std::vector<Filesystem::AudioFormat> Filesystem::m_supportedAudioFormats = {
	AudioFormat::Wav,
	AudioFormat::Aif,
	AudioFormat::Aifc,
	AudioFormat::Aiff,
	AudioFormat::Au,
	AudioFormat::Caf,
	AudioFormat::Voc,
#ifdef H2CORE_HAVE_FLAC_SUPPORT
	AudioFormat::Ogg,
	AudioFormat::Flac,
#endif
#ifdef H2CORE_HAVE_OPUS_SUPPORT
	AudioFormat::Opus,
#endif
#ifdef H2CORE_HAVE_MP3_SUPPORT
	AudioFormat::Mp3,
#endif
	AudioFormat::W64
};

QString Filesystem::AudioFormatToSuffix( const AudioFormat& format, bool bSilent ) {
	switch( format ) {
		case AudioFormat::Aif:
		case AudioFormat::Aifc:
		case AudioFormat::Aiff:
			return "aiff";
		case AudioFormat::Au:
			return "au";
		case AudioFormat::Caf:
			return "caf";
		case AudioFormat::Flac:
			return "flac";
		case AudioFormat::Mp3:
			return "mp3";
		case AudioFormat::Ogg:
			return "ogg";
		case AudioFormat::Opus:
			return "opus";
		case AudioFormat::Voc:
			return "voc";
		case AudioFormat::W64:
			return "w64";
		case AudioFormat::Wav:
			return "wav";
		case AudioFormat::Unknown:
		default:
			if ( ! bSilent ) {
				ERRORLOG( "Unknown audio format" );
			}
			return "";
	}
}

Filesystem::AudioFormat Filesystem::AudioFormatFromSuffix( const QString& sPath,
														   bool bSilent ) {
	const QString sPathLower = sPath.toLower();
	if ( sPathLower.endsWith( "aiff" ) ) {
		return AudioFormat::Aif;
	}
	else if ( sPathLower.endsWith( "au" ) ) {
		return AudioFormat::Au;
	}
	else if ( sPathLower.endsWith( "caf" ) ) {
		return AudioFormat::Caf;
	}
	else if ( sPathLower.endsWith( "flac" ) ) {
		return AudioFormat::Flac;
	}
	else if ( sPathLower.endsWith( "mp3" ) ) {
		return AudioFormat::Mp3;
	}
	else if ( sPathLower.endsWith( "ogg" ) ) {
		return AudioFormat::Ogg;
	}
	else if ( sPathLower.endsWith( "opus" ) ) {
		return AudioFormat::Opus;
	}
	else if ( sPathLower.endsWith( "voc" ) ) {
		return AudioFormat::Voc;
	}
	else if ( sPathLower.endsWith( "w64" ) ) {
		return AudioFormat::W64;
	}
	else if ( sPathLower.endsWith( "wav" ) ) {
		return AudioFormat::Wav;
	} else {
		if ( ! bSilent ) {
			ERRORLOG( QString( "Unknown suffix in [%1]" ).arg( sPath ) );
		}
		return AudioFormat::Unknown;
	}
}

bool Filesystem::bootstrap( Logger* logger, const QString& sSysDataPath,
							const QString& sUsrDataPath,
							const QString& sUserConfigPath,
							const QString& sLogFile )
{
	if( __logger==nullptr && logger!=nullptr ) {
		__logger = logger;
	} else {
		return false;
	}

	// A QCoreApplication instance is needed for applicationDirPath etc.
	assert( QCoreApplication::instance() != nullptr );

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
#ifdef H2CORE_HAVE_APPIMAGE
	__sys_data_path = absolute_path( QCoreApplication::applicationDirPath().append( "/../share/hydrogen/data/" ) ) ;
#else
	__sys_data_path = H2_SYS_PATH "/data/";
#endif
	__usr_data_path = QDir::homePath().append( "/" H2_USR_PATH "/data/" );
	__usr_cfg_path = QDir::homePath().append( "/" H2_USR_PATH "/" USR_CONFIG );
#endif
	if ( ! sSysDataPath.isEmpty() ) {
		INFOLOG( QString( "Using custom system data folder [%1]" )
				 .arg( sSysDataPath ) );
		__sys_data_path = sSysDataPath;
		// Sanity check
		if ( ! __sys_data_path.endsWith( QDir::separator() ) ) {
			__sys_data_path.append( QDir::separator() );
		}
	}

	if ( ! sUsrDataPath.isEmpty() ) {
		INFOLOG( QString( "Using custom user data folder [%1]" )
				 .arg( sUsrDataPath ) );
		__usr_data_path = sUsrDataPath;
		// Sanity check
		if ( ! __usr_data_path.endsWith( QDir::separator() ) ) {
			__usr_data_path.append( QDir::separator() );
		}
	}

	if ( ! sUserConfigPath.isEmpty() ) {
		INFOLOG( QString( "Using custom user-level config file [%1]" )
				 .arg( sUserConfigPath ) );
		__usr_cfg_path = sUserConfigPath;
	}

	if ( ! sLogFile.isEmpty() ) {
		// No need for an info log. This is done within the bootstrap of the
		// logger.
		Filesystem::__usr_log_path = sLogFile;
	}

	if( !dir_readable( __sys_data_path ) ) {
		__sys_data_path = QCoreApplication::applicationDirPath().append( "/" LOCAL_DATA_PATH );
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
	if ( !__ladspa_paths.isEmpty() && __ladspa_paths.at( 0 ).isEmpty() ) {
		__ladspa_paths.removeFirst();
	}
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
			if( !silent ) {
				ERRORLOG( QString( "%1 is not a directory" ).arg( folder.fileName() ) );
			}
			return false;
		}
		if( !folder.isWritable() ) {
			if( !silent ) {
				ERRORLOG( QString( "%1 is not writable" ).arg( folder.fileName() ) );
			}
			return false;
		}
		return true;
	}
	if( ( perms & is_dir ) && !fi.isDir() ) {
		if( !silent ) {
			ERRORLOG( QString( "%1 is not a directory" ).arg( path ) );
		}
		return false;
	}
	if( ( perms & is_file ) && !fi.isFile() ) {
		if( !silent ) {
			ERRORLOG( QString( "%1 is not a file" ).arg( path ) );
		}
		return false;
	}
	if( ( perms & is_readable ) && !fi.isReadable() ) {
		if( !silent ) {
			ERRORLOG( QString( "%1 is not readable" ).arg( path ) );
		}
		return false;
	}
	if( ( perms & is_writable ) && !fi.isWritable() ) {
		if( !silent ) {
			ERRORLOG( QString( "%1 is not writable" ).arg( path ) );
		}
		return false;
	}
	if( ( perms & is_executable ) && !fi.isExecutable() ) {
		if( !silent ) {
			ERRORLOG( QString( "%1 is not executable" ).arg( path ) );
		}
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
bool Filesystem::dir_exists(  const QString& path, bool silent )
{
	return check_permissions( path, is_dir, silent );
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
	if ( !QDir( path ).exists() ) {
		if ( !silent ) {
			INFOLOG( QString( "create user directory : %1" ).arg( path ) );
		}
		if ( create && !QDir( "/" ).mkpath( path ) ) {
			ERRORLOG( QString( "unable to create user directory : %1" ).arg( path ) );
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
	const auto contentUtf8 = content.toUtf8();
	file.write( contentUtf8.data() );
	file.close();

	return true;
}

bool Filesystem::file_copy( const QString& src, const QString& dst, bool overwrite, bool bSilent )
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
	if ( ! bSilent ) {
		INFOLOG( QString( "copy %1 to %2" ).arg( src ).arg( dst ) );
	}
	
	// Since QFile::copy does not overwrite, we have to make sure the
	// destination does not exist.
	if ( overwrite && file_exists( dst, true ) ) {
		rm( dst, true, bSilent );
	}

	bool bOk = QFile::copy( src, dst );
	if ( ! bOk ) {
		ERRORLOG( QString( "Error while copying [%1] to [%2]" )
				  .arg( src ).arg( dst ) );
	}

	return bOk;
}

bool Filesystem::rm( const QString& path, bool recursive, bool bSilent )
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
	return rm_fr( path, bSilent );
}

bool Filesystem::rm_fr( const QString& path, bool bSilent )
{
	if ( ! bSilent ) {
		INFOLOG( QString( "Removing [%1] recursively" ).arg( path ) );
	}
	
	bool ret = true;
	QDir dir( path );
	QFileInfoList entries = dir.entryInfoList( QDir::NoDotAndDotDot | QDir::AllEntries );
	for ( int idx = 0; ( ( idx < entries.size() ) && ret ); idx++ ) {
		QFileInfo entryInfo = entries[idx];
		if ( entryInfo.isDir() && !entryInfo.isSymLink() ) {
			ret = rm_fr( entryInfo.absoluteFilePath(), bSilent );
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
	QStringList dirsReadable = { __sys_data_path,	 demos_dir(),
								 sys_drumkits_dir(), sys_drumkit_maps_dir(),
								 xsd_dir(),			 sys_theme_dir(),
								 img_dir(),			 i18n_dir() };

	QStringList filesReadable = { click_file_path(),   empty_sample_path(),
								  playlist_xsd_path(), drumkit_map_xsd_path(),
								  drumkit_xsd_path(),  pattern_xsd_path(),
								  sys_config_path() };

	bool bChecksPassed = true;
	for ( const auto& ssPath : dirsReadable ) {
		if ( ! dir_readable( ssPath ) ) {
			bChecksPassed = false;
		}
	}

	for ( const auto& ssFile : filesReadable ) {
		if ( ! file_readable( ssFile ) ) {
			bChecksPassed = false;
		}
	}

	if ( bChecksPassed ) {
		INFOLOG( QString( "system wide data path %1 is usable." )
				 .arg( __sys_data_path ) );
	}

	return bChecksPassed;
}

bool Filesystem::check_usr_paths() {
	QStringList pathsUsable = { tmp_dir(),			__usr_data_path,
								cache_dir(),		repositories_cache_dir(),
								usr_drumkits_dir(), patterns_dir(),
								playlists_dir(),    plugins_dir(),
								scripts_dir(), songs_dir(),	usr_theme_dir() };

	QStringList filesWritable = { usr_config_path() };
	
	bool bChecksPassed = true;
	for ( const auto& ssPath : pathsUsable ) {
		if ( ! path_usable( ssPath ) ) {
			bChecksPassed = false;
		}
	}

	for ( const auto& ssFile : filesWritable ) {
		if ( ! file_writable( ssFile ) ) {
			bChecksPassed = false;
		}
	}

	if ( bChecksPassed ) {
		INFOLOG( QString( "user path %1 is usable." ).arg( __usr_data_path ) );
	}
	
	return bChecksPassed;
}

const QString& Filesystem::sys_data_path()
{
	return __sys_data_path;
}
const QString& Filesystem::usr_data_path()
{
	return __usr_data_path;
}

const QStringList& Filesystem::ladspa_paths()
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
	if ( ! m_sPreferencesOverwritePath.isEmpty() ) {
		return m_sPreferencesOverwritePath;
	} else {
		return __usr_cfg_path;
	}
}
QString Filesystem::empty_sample_path()
{
	return __sys_data_path + EMPTY_SAMPLE;
}

QString Filesystem::default_song_name() {
	return DEFAULT_SONG;
}

QString Filesystem::empty_path( const Type& type ) {
	QString sPathBase, sExtension, sDefaultName;

	switch ( type ) {
	case Type::Song:
		sPathBase = __usr_data_path + EMPTY_SONG_BASE;
		sExtension = Filesystem::songs_ext;
		sDefaultName = default_song_name();
		break;

	case Type::Playlist:
		sPathBase = __usr_data_path + EMPTY_PLAYLIST_BASE;
		sExtension = Filesystem::playlist_ext;
		sDefaultName = DEFAULT_PLAYLIST;
		break;

	default:
		ERRORLOG( QString( "Unsupported file type: [%1]" )
				  .arg( TypeToQString( type ) ) );
		return "";
	}

	QString sPath( sPathBase + sExtension );

	int nIterations = 0;
	while ( file_exists( sPath, true ) ) {
		sPath = sPathBase + QString::number( nIterations ) + sExtension;
		++nIterations;

		if ( nIterations > 1000 ) {
			ERRORLOG( "That's a bit much. Something is wrong in here." );
			return __usr_data_path + SONGS + sDefaultName + sExtension;
		}
	}

	return sPath;
}

QString Filesystem::untitled_song_name()
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
QString Filesystem::drumkit_xsd( )
{
	return DRUMKIT_XSD;
}
QString Filesystem::drumkit_xsd_path( )
{
	return xsd_dir() + DRUMKIT_XSD;
}
QString Filesystem::drumkit_map_xsd_path( )
{
	return xsd_dir() + DRUMKIT_MAP_XSD;
}
QString Filesystem::pattern_xsd_path( )
{
	return xsd_dir() + DRUMPAT_XSD;
}
QString Filesystem::playlist_xsd_path( )
{
	return xsd_dir() + PLAYLIST_XSD;
}
const QString& Filesystem::log_file_path()
{
	return __usr_log_path;
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
QString Filesystem::usr_theme_dir()
{
	return __usr_data_path + THEMES;
}
QString Filesystem::sys_theme_dir()
{
	return __sys_data_path + THEMES;
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
QString Filesystem::sys_drumkit_maps_dir()
{
	return __sys_data_path + DRUMKIT_MAPS;
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
QString Filesystem::xsd_legacy_dir()
{
	return xsd_dir() + "legacy";
}
QString Filesystem::tmp_dir()
{
	return QDir::tempPath() + "/" + TMP;
}
QString Filesystem::tmp_file_path( const QString &base )
{
	// Ensure template base will produce a valid filename
	QString validBase = base;
	validBase.remove(
		QRegularExpression( "[\\\\|\\/|\\*|\\,|\\$|:|=|@|!|\\^|&|\\?|\"|'|>|<|\\||%|:]+" ) );

	QFileInfo f( validBase );
	QString templateName( tmp_dir() + "/" );
	if ( f.suffix().isEmpty() ) {
		templateName += validBase.left( 20 );
	} else {
		templateName += f.completeBaseName().left( 20 ) + "-XXXXXX." + f.suffix();
	}
	QTemporaryFile file( templateName);
	file.setAutoRemove( false );
	file.open();
	file.close();
	return file.fileName();
}

// DRUMKITS
QStringList Filesystem::drumkit_list( const QString& sPath )
{
	QDir dir( sPath );
	QStringList ok;
	QStringList possible = dir.entryList( QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot );
	for ( const QString& ssSubFolder : possible ) {
		if ( drumkit_valid( dir.absoluteFilePath( ssSubFolder ) ) ) {
			ok << ssSubFolder;
		} else {
			ERRORLOG( QString( "drumkit [%1] is not usable" ).arg( ssSubFolder ) );
		}
	}
	return ok;
}
QString Filesystem::drumkit_default_kit() {
	QString sDefaultPath = sys_drumkits_dir() + DRUMKIT_DEFAULT_KIT;

	// GMRockKit does not exist at system-level? Let's pick another
	// one.
	if ( ! drumkit_valid( sDefaultPath ) ) {
		for ( const auto& sDrumkitName : Filesystem::sys_drumkit_list() ) {
			if ( drumkit_valid( Filesystem::sys_drumkits_dir() + sDrumkitName ) ) {
				sDefaultPath = Filesystem::sys_drumkits_dir() + sDrumkitName;
				break;
			}
		}
	}

	// There is no drumkit at system-level? Let's pick one from user-space.
	if ( ! drumkit_valid( sDefaultPath ) ) {
		for ( const auto& sDrumkitName : Filesystem::usr_drumkit_list() ) {
			if ( drumkit_valid( Filesystem::usr_drumkits_dir() + sDrumkitName ) ) {
				sDefaultPath = Filesystem::usr_drumkits_dir() + sDrumkitName;
				break;
			}
		}
	}

	return sDefaultPath;
}

QStringList Filesystem::sys_drumkit_list( )
{
	return drumkit_list( sys_drumkits_dir() ) ;
}
QStringList Filesystem::usr_drumkit_list( )
{
	return drumkit_list( usr_drumkits_dir() ) ;
}

QString Filesystem::prepare_sample_path( const QString& sSamplePath )
{
	// Check whether the provided absolute sample path is located within a
	// known drumkit directory.
	int nIndexMatch = -1;
	const QStringList drumkitFolders =
		Hydrogen::get_instance()->getSoundLibraryDatabase()->getDrumkitFolders();

	QString sSamplePathCleaned( sSamplePath );
#ifdef WIN32
	// Qt uses posix separators `/` internally but things can easily mix up
	// (maybe due to our code) and we end up with something like
	// C:\projects\hydrogen/data/\drumkits/GMRockKit/Kick-Softest.wav .
	// We have to ensure to work on a single separator.
	sSamplePathCleaned = QString( sSamplePathCleaned ).replace( "\\", "/" );
#endif

	// When composing paths by combining different elements, two file separators
	// can be used in a row. This is no problem in file access itself but would
	// mess up our index-based approach in here.
	sSamplePathCleaned = QString( sSamplePathCleaned ).replace( "//", "/" );

	for ( const auto& ssFolder : drumkitFolders ) {
		if ( sSamplePathCleaned.startsWith( ssFolder ) ) {
			nIndexMatch = sSamplePathCleaned.indexOf( "/", ssFolder.size() ) + 1;
			break;
		}
	}

	if ( nIndexMatch >= 0 ) {
		// Sample is located in a drumkit folder. Just return basename.
		QString sShortenedPath = sSamplePathCleaned.right(
			sSamplePathCleaned.size() - nIndexMatch );

		return std::move( sShortenedPath );
	}

	return sSamplePath;
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
QString Filesystem::drumkit_path_search( const QString& dk_name, const Lookup& lookup, bool bSilent )
{
	if ( lookup == Lookup::stacked || lookup == Lookup::user ) {
		if ( usr_drumkit_list().contains( dk_name ) ){
			return usr_drumkits_dir() + dk_name;
		}
	}

	if ( lookup == Lookup::stacked || lookup == Lookup::system ) {
		if( sys_drumkit_list().contains( dk_name ) ){
			return sys_drumkits_dir() + dk_name;
		}
	}

	if ( ! bSilent ) {
		ERRORLOG( QString( "drumkit [%1] not found using lookup type [%2]" )
				  .arg( dk_name )
				  .arg( static_cast<int>(lookup)));
	}
	
	return QString("");
}
	
QString Filesystem::drumkit_dir_search( const QString& dk_name, const Lookup& lookup )
{
	if ( lookup == Lookup::user || lookup == Lookup::stacked ) {
		if ( usr_drumkit_list().contains( dk_name ) ) {
			return usr_drumkits_dir();
		}
	}
	if ( lookup == Lookup::system || lookup == Lookup::stacked ) {
		if( sys_drumkit_list().contains( dk_name ) ) {
			return sys_drumkits_dir();
		}
	}
	ERRORLOG( QString( "drumkit %1 not found with lookup mode [%2]" )
			  .arg( dk_name ).arg( static_cast<int>(lookup) ) );
	return "";
}
bool Filesystem::drumkit_valid( const QString& sFolderPath )
{
	return file_readable( QDir( sFolderPath ).absoluteFilePath( DRUMKIT_XML ),
						  true );
}
QString Filesystem::drumkit_file( const QString& dk_path )
{
	return dk_path + "/" + DRUMKIT_XML;
}

QString Filesystem::drumkit_xml() {
	return DRUMKIT_XML;
}

QString Filesystem::drumkit_backup_path( const QString& dk_path ) {
	return dk_path + "." +
		QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh-mm-ss" ) + ".bak";
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
		if ( !str.contains( AUTOSAVE ) ) {
			result += str;
		}
	}
	return result;
}

bool Filesystem::song_exists( const QString& sg_name )
{
	return QDir( songs_dir() ).exists( sg_name );
}

bool Filesystem::isPathValid( const Type& type, const QString& sPath,
							  bool bCheckExistance ) {
	QString sExtension, sType;
	switch ( type ) {
	case Type::Song:
		sExtension = Filesystem::songs_ext;
		sType = "song";
		break;

	case Type::Playlist:
		sExtension = Filesystem::playlist_ext;
		sType = "playlist";
		break;

	default:
		ERRORLOG( QString( "Unsupported file type: [%1]" )
				  .arg( TypeToQString( type ) ) );
		return "";
	}
	QString suffix( sExtension );
	suffix.remove( 0, 1 );
	
	QFileInfo fileInfo = QFileInfo( sPath );

	if ( !fileInfo.isAbsolute() ) {
		ERRORLOG( QString( "Error: Unable to handle path [%1]. Please provide an absolute file path!" )
						.arg( sPath ));
		return false;
	}
	
	if ( fileInfo.exists() ) {
		if ( !fileInfo.isReadable() ) {
			ERRORLOG( QString( "Unable to handle path [%1]. You must have permissions to read the file!" )
						.arg( sPath ));
			return false;
		}
	} else if ( bCheckExistance ) {
		ERRORLOG( QString( "Provided %1 [%2] does not exist" )
				  .arg( sType ).arg( sPath ) );
		return false;
	}
	
	if ( fileInfo.suffix() != suffix ) {
		ERRORLOG( QString( "Unable to handle path [%1]. The provided file must have the suffix '%2'!" )
					.arg( sPath ).arg( sExtension ));
		return false;
	}
	
	return true;
}

QString Filesystem::validateFilePath( const QString& sPath ) {

	// Ensure the name will be a valid filename
	QString sValidName( sPath );
	sValidName.replace( " ", "_" );
	sValidName.remove(
		QRegularExpression( "[\\\\|\\/|\\*|\\,|\\$|:|=|@|!|\\^|&|\\?|\"|'|>|<|\\||%|:]+" ) );

	return sValidName;
}

QStringList Filesystem::theme_list( )
{
	return QDir( sys_theme_dir() ).entryList( QStringList( THEME_FILTER ), QDir::Files | QDir::Readable | QDir::NoDotAndDotDot ) +
		QDir( usr_theme_dir() ).entryList( QStringList( THEME_FILTER ), QDir::Files | QDir::Readable | QDir::NoDotAndDotDot );
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
	INFOLOG( QString( "Empty song                 : %1" )
			 .arg( empty_path( Type::Song ) ) );
	INFOLOG( QString( "Empty playlist             : %1" )
			 .arg( empty_path( Type::Playlist ) ) );
	INFOLOG( QString( "Demos dir                  : %1" ).arg( demos_dir() ) );
	INFOLOG( QString( "Documentation dir          : %1" ).arg( doc_dir() ) );					// FIXME must be created even if no doc deployed
	INFOLOG( QString( "System drumkit dir         : %1" ).arg( sys_drumkits_dir() ) );
	INFOLOG( QString( "Empty sample               : %1" ).arg( empty_sample_path() ) );
	INFOLOG( QString( "Default config             : %1" ).arg( sys_config_path() ) );
	INFOLOG( QString( "Internationalization dir   : %1" ).arg( i18n_dir() ) );
	INFOLOG( QString( "Images dir                 : %1" ).arg( img_dir() ) );
	INFOLOG( QString( "XSD dir                    : %1" ).arg( xsd_dir() ) );
	INFOLOG( QString( "Pattern XSD                : %1" ).arg( pattern_xsd_path() ) );
	INFOLOG( QString( "Drumkit XSD                : %1" ).arg( drumkit_xsd_path() ) );
	INFOLOG( QString( "Playlist XSD               : %1" ).arg( playlist_xsd_path() ) );
	INFOLOG( QString( "Drumkit Map XSD            : %1" )
			 .arg( drumkit_map_xsd_path() ) );
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

QString Filesystem::absolute_path( const QString& sFilename, bool bSilent ) {
	if ( QFile( sFilename ).exists() ) {
		return QFileInfo( sFilename ).absoluteFilePath();
	}
	else if ( ! bSilent ) {
		___ERRORLOG( QString( "File [%1] not found" ).arg( sFilename ) );
	}

	return QString();
}

QStringList Filesystem::drumkit_xsd_legacy_paths() {
	const QDir legacyDir( xsd_legacy_dir() );

	const QStringList legacyDirSubfolders =
		legacyDir.entryList( QDir::Dirs | QDir::NoDotAndDotDot,
							 QDir::Name | QDir::Reversed );

	QStringList drumkitXSDs;
	for ( const auto& ffolder : legacyDirSubfolders ) {
		const QDir folder( legacyDir.filePath( ffolder ) );
		
		if ( folder.exists( drumkit_xsd() ) ) {
			drumkitXSDs << folder.filePath( drumkit_xsd() );
		}
	}

	return std::move( drumkitXSDs );
}

QString Filesystem::rerouteDrumkitPath( const QString& sDrumkitPath ) {
#ifdef H2CORE_HAVE_APPIMAGE

	if ( sDrumkitPath.isEmpty() ) {
		ERRORLOG( "Can not reroute empty drumkit paths" );
		return "";
	}

	// Since the path to a system kits of a previously mounted image
	// does most probably not exist anymore we can _not_ use
	// Filesystem::absolute_path in here.
	const QString sAbsolutePath = QDir( sDrumkitPath ).absolutePath();
	QString sResult = sAbsolutePath;

	// Might be different ones depending on the mounting point of the
	// system.
	const QStringList systemPrefixes = { "/tmp" };

	// Check whether the kit is a system drumkit from a previous
	// AppImage session.
	bool bIsForeignSystemKit = false;
	for ( const auto& ssPrefix : systemPrefixes ) {
		if ( sAbsolutePath.startsWith( ssPrefix ) &&
			 ! sAbsolutePath.contains( Filesystem::sys_data_path() ) ) {
			bIsForeignSystemKit = true;
		}
	}

	if ( bIsForeignSystemKit ) {
		const QStringList pathComponents = sAbsolutePath.split( "/" );
		if ( pathComponents.size() > 2 ) {
			const QString sNewPath = QString( "%1%2/%3" )
				.arg( Filesystem::sys_data_path() )
				.arg( pathComponents[ pathComponents.size() - 2 ] )
				.arg( pathComponents[ pathComponents.size() - 1 ] );

			INFOLOG( QString( "Rerouting system kit: [%1] -> [%2]" )
					 .arg( sDrumkitPath )
					 .arg( Filesystem::absolute_path( sNewPath ) ) );

			sResult = Filesystem::absolute_path( sNewPath );
		}
		else {
			ERRORLOG( QString( "Unable to replace drumkit path [%1]" )
					  .arg( sDrumkitPath ) );
		}
	}

	return sResult;
#else
	return sDrumkitPath;
#endif
}

QString Filesystem::getDrumkitMap( const QString& sDrumkitName,
								   bool bSilent ) {
	const QString sMapDir = sys_drumkit_maps_dir();

	if ( ! dir_readable( sMapDir ) ) {
		ERRORLOG( QString( "Unable to access system drumkit map folder [%1]" )
					  .arg( sMapDir ) );
		return QString();
	}

	QString sTarget = QString( "%1%2" ).arg( sDrumkitName )
		.arg( drumkit_map_ext );
	
	QDir mapDir( sMapDir );
	// The mapping file must exactly match drumkit name.
	if ( ! mapDir.exists( sTarget ) ) {
		WARNINGLOG( QString( "No .h2map fallback file for kit [%1] found. Please add types in the kit's Properties dialog yourself" )
					.arg( sDrumkitName ) );
		return QString();
	}

	if ( bSilent ) {
		INFOLOG(QString( "Found map file [%1] for kit [%2]" )
				 .arg( mapDir.filePath( sTarget ) ).arg( sDrumkitName ) );
	}
	return mapDir.filePath( sTarget );
}

QString Filesystem::addUniquePrefix( const QString& sBaseFilePath ) {

	QString sChars( "abcdefghijklmnopqrstuvwxuzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" );

	// a seed source for the random number engine
	std::random_device randomDevice;
	// seeded mersenne_twister engine producing random unsigned integers.
	std::mt19937 randomEngine( randomDevice() );
	// Transformation of the above random number to comply with our constraints.
	std::uniform_int_distribution<int> distr( 0, sChars.size() - 1 );

	// A prefix will be formatted as "tmp-XXXXXX-" with X being a latin
	// character or digit.
	auto createPrefix = [&](){
		QString sPrefix( "tmp-" );
		for ( int ii = 0; ii < 6; ++ii ) {
			sPrefix.append( sChars.at( distr( randomEngine ) ) );
		}

		return std::move( sPrefix.append( "-" ) );
	};

	QFileInfo baseInfo( sBaseFilePath );

	QString sUniquePath = baseInfo.absoluteDir()
		.absoluteFilePath( createPrefix() + baseInfo.fileName() );

	int maxTries = 100;
	int ii = 0;
	while ( file_exists( sUniquePath, true ) ) {
		sUniquePath = baseInfo.absoluteDir()
			.absoluteFilePath( createPrefix() + baseInfo.fileName() );

		ii++;
		if ( ii >= maxTries ) {
			ERRORLOG( QString( "Unable to create unique path for [%1]" )
					  .arg( sBaseFilePath ) );
			return "";
		}
	}

	return std::move( sUniquePath );
}

QString Filesystem::removeUniquePrefix( const QString& sUniqueFilePath,
										bool bSilent ) {
	QRegularExpression prefix( "tmp-[\\w]{6}-+" );

	if ( sUniqueFilePath.contains( prefix ) ) {
		QFileInfo info( sUniqueFilePath );

		return info.absoluteDir().
			absoluteFilePath( info.fileName().remove( prefix ) );
	}
	else {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Path [%1] does not contain unique prefix" )
						.arg( sUniqueFilePath ) );
		}
		return sUniqueFilePath;
	}
}

QString Filesystem::getAutoSaveFilename( const Type& type, const QString& sBaseName ) {
	QString sDefaultDir, sExtension, sType;
	switch ( type ) {
	case Type::Song:
		sDefaultDir = songs_dir();
		sExtension = Filesystem::songs_ext;
		sType = "song";
		break;

	case Type::Playlist:
		sDefaultDir = playlists_dir();
		sExtension = Filesystem::playlist_ext;
		sType = "playlist";
		break;

	default:
		ERRORLOG( QString( "Unsupported file type: [%1]" )
				  .arg( TypeToQString( type ) ) );
		return "";
	}

	if ( ! sBaseName.isEmpty() ) {
		QFileInfo fileInfo( sBaseName );

		// In case the user did open a hidden file, the baseName()
		// will be an empty string.
		QString sBaseName( fileInfo.completeBaseName() );
		if ( sBaseName.startsWith( "." ) ) {
			sBaseName.remove( 0, 1 );
		}

		const QString sAbsolutePath = QString( "%1/.%2.autosave%3" )
			.arg( fileInfo.absoluteDir().absolutePath() )
			.arg( sBaseName ).arg( sExtension );

		if ( ! Filesystem::file_writable( sAbsolutePath, true ) ) {
			QString sNewName = QString( "%1.%2.autosave%3" )
				.arg( sDefaultDir ).arg( sBaseName ).arg( sExtension );

			WARNINGLOG( QString( "Path of current %1 [%2] is not writable. Autosave will store it as [%3] instead." )
						.arg( sType ).arg( sAbsolutePath ).arg( sNewName ) );
			return sNewName;
		}
		else {
			return sAbsolutePath;
		}
	}

	// Store the default autosave file in the user's song data
	// folder to not clutter their working directory.
	return QString( "%1.autosave%2" ).arg( sDefaultDir ).arg( sExtension );
}

QString Filesystem::TypeToQString( const Type& type ) {
	switch( type ) {
	case Type::Song:
		return "Song";

	case Type::Playlist:
		return "Playlist";

	default:
		return "Unknown";
	}
}

QString Filesystem::removeUtf8Characters( const QString &sEncodedString ) {
	QString sCleaned( sEncodedString );
	return sCleaned.remove(
		QRegularExpression( "[^a-zA-Z0-9._/\\s()\\[\\]\\&\\+\\-]" ) );
}
const std::vector<Filesystem::AudioFormat>& Filesystem::supportedAudioFormats() {
	return m_supportedAudioFormats;
}

QString Filesystem::appendNumberOrIncrement( const QString& sString ) {
	auto parts = sString.split( " " );

	bool bOk;
	int nNumber = parts.last().toInt( &bOk, 10 );
	if ( bOk ) {
		parts.removeLast();
		parts.append( QString::number( ++nNumber ) );
	}
	else {
		parts.append( QString::number( 2 ) );
	}

	return parts.join( " " );
}
};

/* vim: set softtabstop=4 noexpandtab: */
