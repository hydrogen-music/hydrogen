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

#include <random>

#include <QtCore/QDir>
#include <QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QDateTime>
#include <QRegularExpression>

#if QT_VERSION >= QT_VERSION_CHECK( 6, 8, 0 )
#include <QtEnvironmentVariables>
#else
#include <QtGlobal>
#endif

#include <core/Basics/Drumkit.h>
#include <core/config.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#ifdef H2CORE_HAVE_OSC
#include <core/NsmClient.h>
#endif

// directories
#define LOCAL_DATA_PATH "data/"
#define CACHE "cache/"
#define DEMOS "demo_songs/"
#define DOC "doc/"
#define DRUMKITS "drumkits/"
#define DRUMKIT_MAPS "drumkit_maps/"
#define I18N "i18n/"
#define IMG "img/"
#define PATTERNS "patterns/"
#define PLAYLISTS "playlists/"
#define REPOSITORIES "repositories/"
#define SCRIPTS "scripts/"
#define SONGS "songs/"
#define THEMES "themes/"
#define TMP "hydrogen/"

// XDG-compliant paths for Linux systems. Note that we do not use QStandardPaths
// because `hydrogen`, `h2cli`, and `h2player` must use the same folders while
// not sharing the same application name. Just temporarily setting their
// application name to "hydrogen" would be an even dirtier solution than
// hard-coding paths.
#define XDG_LINUX_CACHE ".cache/"
#define XDG_LINUX_CONFIG ".config/"
#define XDG_LINUX_DATA ".local/share/"
#define APPLICATION_NAME "hydrogen"

// files
/** Sound of metronome beat */
#define CLICK_SAMPLE "click.wav"
#define EMPTY_SAMPLE "emptySample.wav"
#define DEFAULT_SONG "DefaultSong"
#define DEFAULT_PLAYLIST "DefaultPlaylist"
#define EMPTY_SONG_BASE "emptySong"
#define EMPTY_PLAYLIST_BASE "emptyPlaylist"
#define USR_CONFIG "hydrogen.conf"
#define SYS_CONFIG "hydrogen.default.conf"
#define LOG_FILE "hydrogen.log"
#define DRUMKIT_XML "drumkit.xml"

#define AUTOSAVE "autosave"

#define UNTITLED_PLAYLIST "untitled.h2playlist"

// filters
#define DRUMKIT_FILTER "*.h2drumkit"
#define PATTERN_FILTER "*.h2pattern"
#define PLAYLIST_FILTER "*.h2playlist"
#define SONG_FILTER "*.h2song"
#define THEME_FILTER "*.h2theme"

namespace H2Core {

Logger* Filesystem::m_pLogger = nullptr;

const QString Filesystem::sScriptSuffix = ".sh";
const QString Filesystem::sSongSuffix = ".h2song";
const QString Filesystem::sThemeSuffix = ".h2theme";
const QString Filesystem::sPatternSuffix = ".h2pattern";
const QString Filesystem::sPlaylistSuffix = ".h2playlist";
const QString Filesystem::sDrumkitSuffix = ".h2drumkit";
const QString Filesystem::sDrumkitMapSuffix = ".h2map";
const QString Filesystem::sScriptFilter = "Hydrogen Scripts (*.sh)";
const QString Filesystem::sDrumkitFilter = "Hydrogen Drumkit (*.h2drumkit)";
const QString Filesystem::sSongFilter = "Hydrogen Songs (*.h2song)";
const QString Filesystem::sThemeFilter = "Hydrogen Theme (*.h2theme)";
const QString Filesystem::sPatternFilter = "Hydrogen Patterns (*.h2pattern)";
const QString Filesystem::sPlaylistFilter = "Hydrogen Playlists (*.h2playlist)";

QString Filesystem::m_sSystemDataPath;
QString Filesystem::m_sUserCachePath;
QString Filesystem::m_sUserConfigPath;
QString Filesystem::m_sUserDataPath;

#ifdef Q_OS_MACX
QString Filesystem::m_sUserLogPath =
	QDir::homePath().append( "/Library/Application Support/Hydrogen/" LOG_FILE
	);
#elif WIN32
QString Filesystem::m_sUserLogPath =
	QDir::homePath().append( "/.hydrogen/" LOG_FILE );
#else
QString Filesystem::m_sUserLogPath =
	QDir::homePath().append( "/" H2_USR_PATH "/" LOG_FILE );
#endif

bool Filesystem::m_bLogPathInitialized = false;

QString Filesystem::m_sPreferencesOverwritePath = "";

static QString getEnvironmentVariable( const QString& sEnvironmentVariable )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 8, 0 )
	return qEnvironmentVariable( sEnvironmentVariable.toLocal8Bit().data() );
#else
	return QString( qgetenv( sEnvironmentVariable.toLocal8Bit().data() ) );
#endif
};

static bool isEnvironmentVariableSet( const QString& sEnvironmentVariable )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 8, 0 )
	return qEnvironmentVariableIsSet( sEnvironmentVariable.toLocal8Bit().data()
	);
#else
	return !qgetenv( sEnvironmentVariable.toLocal8Bit().data() ).isEmpty();
#endif
};

Filesystem::Context Filesystem::DetermineContext( const QString& sPath,
												  Hydrogen* pHydrogen )
{
	if ( !sPath.isEmpty() ) {
		const QString sAbsolutePath = Filesystem::absolutePath( sPath );
		if ( sAbsolutePath.contains( m_sSystemDataPath ) ) {
			return Filesystem::Context::System;
		}
		else if ( sAbsolutePath.contains( m_sUserDataPath ) ) {
			return Filesystem::Context::User;
		}
		else {
			for ( const auto& ssPath :
				  pHydrogen->getPreferences()->getCustomSoundLibraryDirs() ) {
				if ( sAbsolutePath.contains( ssPath ) ) {
					return Filesystem::Context::Custom;
				}
			}
			if ( Filesystem::dirWritable( sAbsolutePath, true ) ) {
				return Filesystem::Context::SessionReadWrite;
			}
			else {
				return Filesystem::Context::SessionReadOnly;
			}
		}
	}
	else {
		return Filesystem::Context::Song;
	}
}

QString Filesystem::ContextToQString( const Context& context )
{
	switch ( context ) {
		case Filesystem::Context::System:
			return "System";
		case Filesystem::Context::User:
			return "User";
		case Filesystem::Context::SessionReadOnly:
			return "SessionReadOnly";
		case Filesystem::Context::SessionReadWrite:
			return "SessionReadWrite";
		case Filesystem::Context::Song:
			return "Song";
		case Filesystem::Context::Custom:
			return "Custom";
		default:
			return QString( "Unknown context [%1]" )
				.arg( static_cast<int>( context ) );
	}
}

std::vector<Filesystem::AudioFormat> Filesystem::m_supportedAudioFormats = {
	AudioFormat::Wav,  AudioFormat::Aif,  AudioFormat::Aifc, AudioFormat::Aiff,
	AudioFormat::Au,   AudioFormat::Caf,  AudioFormat::Voc,
#ifdef H2CORE_HAVE_FLAC_SUPPORT
	AudioFormat::Ogg,  AudioFormat::Flac,
#endif
#ifdef H2CORE_HAVE_OPUS_SUPPORT
	AudioFormat::Opus,
#endif
#ifdef H2CORE_HAVE_MP3_SUPPORT
	AudioFormat::Mp3,
#endif
	AudioFormat::W64 };

QString
Filesystem::AudioFormatToSuffix( const AudioFormat& format, bool bSilent )
{
	switch ( format ) {
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
			if ( !bSilent ) {
				ERRORLOG( "Unknown audio format" );
			}
			return "";
	}
}

Filesystem::AudioFormat
Filesystem::AudioFormatFromSuffix( const QString& sPath, bool bSilent )
{
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
	}
	else {
		if ( !bSilent ) {
			ERRORLOG( QString( "Unknown suffix in [%1]" ).arg( sPath ) );
		}
		return AudioFormat::Unknown;
	}
}

bool Filesystem::bootstrap(
	Logger* logger,
	const QString& sSysDataPath,
	const QString& sUsrDataPath,
	const QString& sUserConfigPath,
	const QString& sLogFile
)
{
	if ( m_pLogger == nullptr && logger != nullptr ) {
		m_pLogger = logger;
	}
	else {
		return false;
	}

	// A QCoreApplication instance is needed for applicationDirPath etc.
	assert( QCoreApplication::instance() != nullptr );

#ifdef Q_OS_MACX
#ifdef H2CORE_HAVE_BUNDLE
	// Bundle: Prepare hydrogen to use path names which are used in app bundles:
	// http://en.wikipedia.org/wiki/Application_Bundle
	m_sSystemDataPath =
		QCoreApplication::applicationDirPath().append( "/../Resources/data/" );
#else
	m_sSystemDataPath =
		QCoreApplication::applicationDirPath().append( "/data/" );
#endif
	m_sUserDataPath =
		QDir::homePath().append( "/Library/Application Support/Hydrogen/data/"
		);
	m_sUserConfigPath = QDir::homePath().append(
		"/Library/Application Support/Hydrogen/" USR_CONFIG
	);
#elif WIN32
	m_sSystemDataPath =
		QCoreApplication::applicationDirPath().append( "/data/" );
	m_sUserDataPath = QDir::homePath().append( "/.hydrogen/data/" );
	m_sUserConfigPath = QDir::homePath().append( "/.hydrogen/" USR_CONFIG );
#else
#ifdef H2CORE_HAVE_APPIMAGE
	m_sSystemDataPath =
		absolutePath( QCoreApplication::applicationDirPath().append(
			"/../share/hydrogen/data/"
		) );
#else
	m_sSystemDataPath = H2_SYS_PATH "/data/";
#endif
	// If the old path exists (e.g. ~/.hydrogen), old path is used; else uses
	// XDG Paths on Linux. We do not use QStandardPaths in order to share the
	// same location amongst all our binaries (bearing different application
	// names). But we still allow the user to overwrite the default paths.
	if ( !QFileInfo::exists( QDir::homePath().append( "/" H2_USR_PATH ) ) ) {
		if ( isEnvironmentVariableSet( "XDG_CONFIG_HOME" ) ) {
			m_sUserConfigPath = getEnvironmentVariable( "XDG_CONFIG_HOME" ) +
								"/" + APPLICATION_NAME + "/" + USR_CONFIG;
		}
		else {
			m_sUserConfigPath =
				QDir::homePath().append( "/" XDG_LINUX_CONFIG APPLICATION_NAME
										 "/" USR_CONFIG );
		}
		if ( isEnvironmentVariableSet( "XDG_DATA_HOME" ) ) {
			m_sUserDataPath = getEnvironmentVariable( "XDG_DATA_HOME" ) + "/" +
							  APPLICATION_NAME + "/";
		}
		else {
			m_sUserDataPath =
				QDir::homePath().append( "/" XDG_LINUX_DATA APPLICATION_NAME "/"
				);
		}
		if ( isEnvironmentVariableSet( "XDG_CACHE_HOME" ) ) {
			m_sUserCachePath = getEnvironmentVariable( "XDG_CACHE_HOME" ) +
							   "/" + APPLICATION_NAME + "/";
		}
		else {
			m_sUserCachePath =
				QDir::homePath().append( "/" XDG_LINUX_CACHE APPLICATION_NAME
										 "/" );
		}
	}
	else {
		m_sUserDataPath = QDir::homePath().append( "/" H2_USR_PATH "/data/" );
		m_sUserCachePath = m_sUserDataPath + CACHE;
		m_sUserConfigPath =
			QDir::homePath().append( "/" H2_USR_PATH "/" USR_CONFIG );
	}
#endif
	if ( !sSysDataPath.isEmpty() ) {
		INFOLOG( QString( "Using custom system data folder [%1]" )
					 .arg( sSysDataPath ) );
		m_sSystemDataPath = sSysDataPath;
		// Sanity check
		if ( !m_sSystemDataPath.endsWith( "/" ) ) {
			m_sSystemDataPath.append( "/" );
		}
	}

	if ( !sUsrDataPath.isEmpty() ) {
		INFOLOG(
			QString( "Using custom user data folder [%1]" ).arg( sUsrDataPath )
		);
		m_sUserDataPath = sUsrDataPath;
		// Sanity check
		if ( !m_sUserDataPath.endsWith( "/" ) ) {
			m_sUserDataPath.append( "/" );
		}
	}

	if ( !sUserConfigPath.isEmpty() ) {
		INFOLOG( QString( "Using custom user-level config file [%1]" )
					 .arg( sUserConfigPath ) );
		m_sUserConfigPath = sUserConfigPath;
	}

	if ( !sLogFile.isEmpty() ) {
		// No need for an info log. This is done within the bootstrap of the
		// logger.
		Filesystem::m_sUserLogPath = sLogFile;
	}

	if ( !dirReadable( m_sSystemDataPath ) ) {
		m_sSystemDataPath =
			QCoreApplication::applicationDirPath().append( "/" LOCAL_DATA_PATH
			);
		ERRORLOG(
			QString( "will use local data path : %1" ).arg( m_sSystemDataPath )
		);
	}

	bool ret = checkSystemPaths();
	ret &= checkUserPaths();
	info();
	return ret;
}

bool Filesystem::checkPermissions(
	const QString& sPath,
	const int nFilePermission,
	bool bSilent
)
{
	QFileInfo fi( sPath );
	if ( ( nFilePermission & FilePermission::IsFile ) &&
		 ( nFilePermission & FilePermission::IsWritable ) && !fi.exists() ) {
		QFileInfo folder( sPath.left( sPath.lastIndexOf( "/" ) ) );
		if ( !folder.isDir() ) {
			if ( !bSilent ) {
				ERRORLOG(
					QString( "%1 is not a directory" ).arg( folder.fileName() )
				);
			}
			return false;
		}
		if ( !folder.isWritable() ) {
			if ( !bSilent ) {
				ERRORLOG(
					QString( "%1 is not writable" ).arg( folder.fileName() )
				);
			}
			return false;
		}
		return true;
	}
	if ( ( nFilePermission & FilePermission::IsDir ) && !fi.isDir() ) {
		if ( !bSilent ) {
			ERRORLOG( QString( "%1 is not a directory" ).arg( sPath ) );
		}
		return false;
	}
	if ( ( nFilePermission & FilePermission::IsFile ) && !fi.isFile() ) {
		if ( !bSilent ) {
			ERRORLOG( QString( "%1 is not a file" ).arg( sPath ) );
		}
		return false;
	}
	if ( ( nFilePermission & FilePermission::IsReadable ) &&
		 !fi.isReadable() ) {
		if ( !bSilent ) {
			ERRORLOG( QString( "%1 is not readable" ).arg( sPath ) );
		}
		return false;
	}
	if ( ( nFilePermission & FilePermission::IsWritable ) &&
		 !fi.isWritable() ) {
		if ( !bSilent ) {
			ERRORLOG( QString( "%1 is not writable" ).arg( sPath ) );
		}
		return false;
	}
	if ( ( nFilePermission & FilePermission::IsExecutable ) &&
		 !fi.isExecutable() ) {
		if ( !bSilent ) {
			ERRORLOG( QString( "%1 is not executable" ).arg( sPath ) );
		}
		return false;
	}
	return true;
}

bool Filesystem::fileExists( const QString& sPath, bool bSilent )
{
	return checkPermissions( sPath, FilePermission::IsFile, bSilent );
}
bool Filesystem::fileReadable( const QString& sPath, bool bSilent )
{
	return checkPermissions(
		sPath, FilePermission::IsFile | FilePermission::IsReadable, bSilent
	);
}
bool Filesystem::fileWritable( const QString& sPath, bool bSilent )
{
	return checkPermissions(
		sPath,
		FilePermission::IsFile | FilePermission::IsReadable |
			FilePermission::IsWritable,
		bSilent
	);
}
bool Filesystem::fileExecutable( const QString& sPath, bool bSilent )
{
	return checkPermissions(
		sPath, FilePermission::IsFile | FilePermission::IsExecutable, bSilent
	);
}
bool Filesystem::dirExists( const QString& sPath, bool bSilent )
{
	return checkPermissions( sPath, FilePermission::IsDir, bSilent );
}
bool Filesystem::dirReadable( const QString& sPath, bool bSilent )
{
	return checkPermissions(
		sPath,
		FilePermission::IsDir | FilePermission::IsReadable |
			FilePermission::IsExecutable,
		bSilent
	);
}
bool Filesystem::dirWritable( const QString& sPath, bool bSilent )
{
	return checkPermissions(
		sPath, FilePermission::IsDir | FilePermission::IsWritable, bSilent
	);
}

bool Filesystem::mkdir( const QString& sPath )
{
	if ( !QDir( "/" ).mkpath( QDir( sPath ).absolutePath() ) ) {
		ERRORLOG( QString( "unable to create directory : %1" ).arg( sPath ) );
		return false;
	}
	return true;
}

bool Filesystem::pathUsable( const QString& sPath, bool bCreate, bool bSilent )
{
	if ( !QDir( sPath ).exists() ) {
		if ( !bSilent ) {
			INFOLOG( QString( "Create user directory : %1" ).arg( sPath ) );
		}
		if ( bCreate && !QDir( "/" ).mkpath( sPath ) ) {
			ERRORLOG(
				QString( "unable to create user directory : %1" ).arg( sPath )
			);
			return false;
		}
	}
	return dirReadable( sPath, bSilent ) && dirWritable( sPath, bSilent );
}

bool Filesystem::fileCopy(
	const QString& sSourcePath,
	const QString& sDestinationPath,
	bool bOverwrite,
	bool bSilent
)
{
	if ( !bOverwrite && fileExists( sDestinationPath, true ) ) {
		WARNINGLOG( QString( "do not overwrite %1 with %2 as it already exists"
		)
						.arg( sDestinationPath )
						.arg( sSourcePath ) );
		return true;
	}
	if ( !fileReadable( sSourcePath ) ) {
		ERRORLOG( QString( "unable to copy %1 to %2, %1 is not readable" )
					  .arg( sSourcePath )
					  .arg( sDestinationPath ) );
		return false;
	}
	if ( !fileWritable( sDestinationPath ) ) {
		ERRORLOG( QString( "unable to copy %1 to %2, %2 is not writable" )
					  .arg( sSourcePath )
					  .arg( sDestinationPath ) );
		return false;
	}
	if ( !bSilent ) {
		INFOLOG( QString( "copy %1 to %2" )
					 .arg( sSourcePath )
					 .arg( sDestinationPath ) );
	}

	// Since QFile::copy does not overwrite, we have to make sure the
	// destination does not exist.
	if ( bOverwrite && fileExists( sDestinationPath, true ) ) {
		rm( sDestinationPath, true, bSilent );
	}

	bool bOk = QFile::copy( sSourcePath, sDestinationPath );
	if ( !bOk ) {
		ERRORLOG( QString( "Error while copying [%1] to [%2]" )
					  .arg( sSourcePath )
					  .arg( sDestinationPath ) );
	}

	return bOk;
}

bool Filesystem::rm( const QString& sPath, bool bRecursive, bool bSilent )
{
	if ( checkPermissions( sPath, FilePermission::IsFile, true ) ) {
		QFile file( sPath );
		bool ret = file.remove();
		if ( !ret ) {
			ERRORLOG( QString( "unable to remove file %1" ).arg( sPath ) );
		}
		return ret;
	}
	if ( !checkPermissions( sPath, FilePermission::IsDir, true ) ) {
		ERRORLOG(
			QString( "%1 is neither a file nor a directory ?!?!" ).arg( sPath )
		);
		return false;
	}
	if ( !bRecursive ) {
		QDir dir;
		bool ret = dir.rmdir( sPath );
		if ( !ret ) {
			ERRORLOG( QString( "unable to remove dir %1 without recursive "
							   "argument, maybe it is not empty?" )
						  .arg( sPath ) );
		}
		return ret;
	}
	return rmForceRecursive( sPath, bSilent );
}

bool Filesystem::rmForceRecursive( const QString& sPath, bool bSilent )
{
	if ( !bSilent ) {
		INFOLOG( QString( "Removing [%1] recursively" ).arg( sPath ) );
	}

	bool bRet = true;
	QDir dir( sPath );
	QFileInfoList entries =
		dir.entryInfoList( QDir::NoDotAndDotDot | QDir::AllEntries );
	for ( int idx = 0; ( ( idx < entries.size() ) && bRet ); idx++ ) {
		QFileInfo entryInfo = entries[idx];
		if ( entryInfo.isDir() && !entryInfo.isSymLink() ) {
			bRet = rmForceRecursive( entryInfo.absoluteFilePath(), bSilent );
		}
		else {
			QFile file( entryInfo.absoluteFilePath() );
			if ( !file.remove() ) {
				ERRORLOG( QString( "unable to remove %1" )
							  .arg( entryInfo.absoluteFilePath() ) );
				bRet = false;
			}
		}
	}
	if ( !dir.rmdir( dir.absolutePath() ) ) {
		ERRORLOG( QString( "unable to remove %1" ).arg( dir.absolutePath() ) );
		bRet = false;
	}
	return bRet;
}

bool Filesystem::checkSystemPaths()
{
	QStringList dirsReadable = {
		m_sSystemDataPath,
		demosDir(),
		systemDrumkitsDir(),
		systemDrumkitMapsDir(),
		systemThemesDir(),
		systemImageDir(),
		systemInternationalizationDir() };

	QStringList filesReadable = { clickFilePath(),	 emptySamplePath(),
								  systemConfigPath() };

	bool bChecksPassed = true;
	for ( const auto& ssPath : dirsReadable ) {
		if ( !dirReadable( ssPath ) ) {
			bChecksPassed = false;
		}
	}

	for ( const auto& ssFile : filesReadable ) {
		if ( !fileReadable( ssFile ) ) {
			bChecksPassed = false;
		}
	}

	if ( bChecksPassed ) {
		INFOLOG( QString( "system wide data sPath %1 is usable." )
					 .arg( m_sSystemDataPath ) );
	}

	return bChecksPassed;
}

bool Filesystem::checkUserPaths()
{
	QStringList pathsUsable = { tmpDir(),			m_sUserDataPath,
								cacheDir(),			repositoriesCacheDir(),
								userDrumkitsDir(),	userPatternsDir(),
								userPlaylistsDir(),
								userScriptsDir(),	userSongsDir(),
								userThemesDir() };

	QStringList filesWritable = { userConfigPath() };

	bool bChecksPassed = true;
	for ( const auto& ssPath : pathsUsable ) {
		if ( !pathUsable( ssPath ) ) {
			bChecksPassed = false;
		}
	}

	for ( const auto& ssFile : filesWritable ) {
		if ( !fileWritable( ssFile ) ) {
			bChecksPassed = false;
		}
	}

	if ( bChecksPassed ) {
		INFOLOG( QString( "user sPath %1 is usable." ).arg( m_sUserDataPath ) );
	}

	return bChecksPassed;
}

const QString& Filesystem::systemDataPath()
{
	return m_sSystemDataPath;
}
const QString& Filesystem::userDataPath()
{
	return m_sUserDataPath;
}

// FILES
QString Filesystem::systemConfigPath()
{
	return m_sSystemDataPath + SYS_CONFIG;
}
QString Filesystem::userConfigPath()
{
	if ( !m_sPreferencesOverwritePath.isEmpty() ) {
		return m_sPreferencesOverwritePath;
	}
	else {
		return m_sUserConfigPath;
	}
}
QString Filesystem::emptySamplePath()
{
	return m_sSystemDataPath + EMPTY_SAMPLE;
}

QString Filesystem::defaultSongName()
{
	return DEFAULT_SONG;
}

QString Filesystem::emptyPath( const Artifact& artifact )
{
	QString sPathBase, sExtension, sDefaultName;

	switch ( artifact ) {
		case Artifact::Song:
			sPathBase = m_sUserDataPath + EMPTY_SONG_BASE;
			sExtension = Filesystem::sSongSuffix;
			sDefaultName = defaultSongName();
			break;

		case Artifact::Playlist:
			sPathBase = m_sUserDataPath + EMPTY_PLAYLIST_BASE;
			sExtension = Filesystem::sPlaylistSuffix;
			sDefaultName = DEFAULT_PLAYLIST;
			break;

		default:
			ERRORLOG( QString( "Unsupported file artifact: [%1]" )
						  .arg( ArtifactToQString( artifact ) ) );
			return "";
	}

	QString sPath( sPathBase + sExtension );

	int nIterations = 0;
	while ( fileExists( sPath, true ) ) {
		sPath = sPathBase + QString::number( nIterations ) + sExtension;
		++nIterations;

		if ( nIterations > 1000 ) {
			ERRORLOG( "That's a bit much. Something is wrong in here." );
			return m_sUserDataPath + SONGS + sDefaultName + sExtension;
		}
	}

	return sPath;
}

QString Filesystem::untitledPlaylistFileName()
{
	return UNTITLED_PLAYLIST;
}
QString Filesystem::clickFilePath()
{
	return m_sSystemDataPath + CLICK_SAMPLE;
}
const QString& Filesystem::logFilePath()
{
	// Called within the Reporter prior to the bootstrap of Filesystem itself.
	// Therefore we need some special treatments.
#if defined( Q_OS_MACX ) || defined( WIN32 )
#else
	if ( !m_bLogPathInitialized ) {
		if ( !QFileInfo::exists( QDir::homePath().append( "/" H2_USR_PATH )
			 ) ) {
			if ( isEnvironmentVariableSet( "XDG_DATA_HOME" ) ) {
				m_sUserLogPath = getEnvironmentVariable( "XDG_DATA_HOME" ) + "/" +
								 APPLICATION_NAME + "/" + LOG_FILE;
			}
			else {
				m_sUserLogPath =
					QDir::homePath().append( "/" XDG_LINUX_DATA APPLICATION_NAME
											 "/" LOG_FILE );
			}
		}
		m_bLogPathInitialized = true;
	}
#endif
	return m_sUserLogPath;
}

// DIRS
QString Filesystem::systemImageDir()
{
	return m_sSystemDataPath + IMG;
}
QString Filesystem::systemDocumentationDir()
{
	return m_sSystemDataPath + DOC;
}
QString Filesystem::systemInternationalizationDir()
{
	return m_sSystemDataPath + I18N;
}
QString Filesystem::userScriptsDir()
{
	return m_sUserDataPath + SCRIPTS;
}
QString Filesystem::userSongsDir()
{
	return m_sUserDataPath + SONGS;
}
QString Filesystem::userThemesDir()
{
	return m_sUserDataPath + THEMES;
}
QString Filesystem::systemThemesDir()
{
	return m_sSystemDataPath + THEMES;
}
QString Filesystem::userPatternsDir()
{
	return m_sUserDataPath + PATTERNS;
}
QString Filesystem::systemPatternsDir()
{
	return m_sSystemDataPath + PATTERNS;
}
QString Filesystem::systemSongsDir()
{
	return m_sSystemDataPath + SONGS;
}
QString Filesystem::systemDrumkitsDir()
{
	return m_sSystemDataPath + DRUMKITS;
}
QString Filesystem::userDrumkitsDir()
{
	return m_sUserDataPath + DRUMKITS;
}
QString Filesystem::systemDrumkitMapsDir()
{
	return m_sSystemDataPath + DRUMKIT_MAPS;
}
QString Filesystem::userPlaylistsDir()
{
	return m_sUserDataPath + PLAYLISTS;
}
QString Filesystem::cacheDir()
{
#if defined(Q_OS_MACX) || defined(WIN32)
	return m_sUserDataPath + CACHE;
#else
	return m_sUserCachePath;
#endif
}
QString Filesystem::repositoriesCacheDir()
{
#if defined(Q_OS_MACX) || defined(WIN32)
	return m_sUserDataPath + CACHE + REPOSITORIES;
#else
	return m_sUserCachePath + REPOSITORIES;
#endif
}
QString Filesystem::demosDir()
{
	return m_sSystemDataPath + DEMOS;
}
QString Filesystem::tmpDir()
{
	return QDir::tempPath() + "/" + TMP;
}
QString Filesystem::tmpFilePath( const QString& sBase )
{
	// Ensure template base will produce a valid filename
	QString validBase = sBase;
	validBase.remove( QRegularExpression(
		"[\\\\|\\/|\\*|\\,|\\$|:|=|@|!|\\^|&|\\?|\"|'|>|<|\\||%|:]+"
	) );

	QFileInfo f( validBase );
	QString sTemplateName( tmpDir() + "/" );
	if ( f.suffix().isEmpty() ) {
		sTemplateName += validBase.left( 20 );
	}
	else {
		sTemplateName +=
			f.completeBaseName().left( 20 ) + "-XXXXXX." + f.suffix();
	}
	QTemporaryFile file( sTemplateName );
	file.setAutoRemove( false );
	file.open();
	file.close();
	return file.fileName();
}

QStringList Filesystem::listContent(
	Artifact artifact,
	Context context,
	const QString& sUserDirOverwrite,
	Hydrogen* pHydrogen
)
{
	QStringList content;

	// In case of session folders, targetDirs can return more than one folder.
	// But in generel there will be only one.
	const auto dirs = Filesystem::targetDirs( artifact, context, pHydrogen );
	if ( dirs.size() == 0 ) {
        // No content in this context.
		return content;
	}

	const auto sFilter = Filesystem::targetFilter( artifact );

	// Recursively traverse all target folders and return all files matching the
	// filter.
	for ( auto ssDir : dirs ) {
		if ( !sUserDirOverwrite.isEmpty() &&
			 ssDir.contains( userDataPath() ) ) {
			ERRORLOG( "sUserDirOverwrite must only be used in unit tests!" );
			ssDir = sUserDirOverwrite;
		}

		QDirIterator it(
			ssDir, QStringList( sFilter ), QDir::Files | QDir::NoDotAndDotDot,
			QDirIterator::Subdirectories
		);

		while ( it.hasNext() ) {
			content << it.next();
		}
	}

	return content;
}

QString Filesystem::prepareSamplePath(
	const QString& sSamplePath,
	const QString& sDrumkitPath
)
{
	// Normalize paths using QFileInfo. This way we neither have to deal with
	// duplicated separators nor with platform-dependent quirks.
	const QString sSamplePathCleaned =
		QFileInfo( sSamplePath ).absoluteFilePath();
	const QString sDrumkitDirCleaned =
		QFileInfo( Filesystem::drumkitDirFromPath( sDrumkitPath ) )
			.absoluteFilePath();

	// When storing just the file name, the sample will be loaded by
	// concatenating the drumkit sPath associated with an instrument and the
	// sample file name. Thus, we have to make sure to just string paths belong
	// to that very drumkit.
	if ( sSamplePathCleaned.startsWith( sDrumkitDirCleaned ) ) {
		const int nIndexMatch =
			sSamplePathCleaned.indexOf( "/", sDrumkitDirCleaned.size() ) + 1;
		const QString sShortenedPath =
			sSamplePathCleaned.right( sSamplePathCleaned.size() - nIndexMatch );

		return std::move( sShortenedPath );
	}

	return sSamplePath;
}

QString Filesystem::drumkitXml()
{
	return DRUMKIT_XML;
}

QString Filesystem::sanitizeDrumkitPath( const QString& sDrumkitPath )
{
	if ( sDrumkitPath.contains( DRUMKIT_XML ) ) {
		return sDrumkitPath;
	}
	else if ( dirReadable( sDrumkitPath, true ) && fileReadable( sDrumkitPath + "/" + DRUMKIT_XML ) ) {
		return Filesystem::drumkitPathFromDir( sDrumkitPath );
	}
	else {
		return "";
	}
}

QString Filesystem::drumkitDirFromPath( const QString& sDrumkitPath )
{
	if ( !sDrumkitPath.contains( DRUMKIT_XML ) ) {
		ERRORLOG( QString( "Path [%1] is not a valid drumkit path" )
					  .arg( sDrumkitPath ) );
	}
	QString sDir = QFileInfo( sDrumkitPath ).absoluteDir().absolutePath();

#ifdef WIN32
	// Absolute paths in Windows start with a drive prefix, like "C:". This will
	// be created automatically by QFileInfo. In case it is also present in the
	// provided input path, we keep it. If not, we strip it.
	const int nSeparatorPosPath = sDrumkitPath.indexOf( "/" );
	const int nSeparatorPosDir = sDir.indexOf( "/" );
	// A value of -1 indicates that there is no separator present at all, which is
	// also covered.
	if ( nSeparatorPosDir > 0 && nSeparatorPosPath == 0 ) {
		sDir = sDir.mid( nSeparatorPosDir );
	}
#endif
	return sDir;
}

QString Filesystem::drumkitPathFromDir( const QString& sDrumkitDir )
{
	if ( sDrumkitDir.contains( DRUMKIT_XML ) ) {
		WARNINGLOG( QString( "Path [%1] does not look like a drumkit folder" )
						.arg( sDrumkitDir ) )
	}
	return QDir( sDrumkitDir ).filePath( DRUMKIT_XML );
}

QString Filesystem::drumkitBackupPath( const QString& sDrumkitPath )
{
	return sDrumkitPath + "." +
		   QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh-mm-ss" ) +
		   ".bak";
}

bool Filesystem::isPathValid(
	const Artifact& artifact,
	const QString& sPath,
	bool bCheckExistance
)
{
	QString sExtension;
	switch ( artifact ) {
		case Artifact::DrumkitBundled:
			sExtension = Filesystem::sDrumkitSuffix;
			break;
		case Artifact::DrumkitExtracted:
			sExtension = "." + QFileInfo( DRUMKIT_XML ).suffix();
			break;
		case Artifact::Pattern:
			sExtension = Filesystem::sPatternSuffix;
			break;
		case Artifact::Playlist:
			sExtension = Filesystem::sPlaylistSuffix;
			break;
		case Artifact::Song:
			sExtension = Filesystem::sSongSuffix;
			break;

		default:
			ERRORLOG( QString( "Unsupported file type: [%1]" )
						  .arg( ArtifactToQString( artifact ) ) );
			return "";
	}
	QString suffix( sExtension );
	suffix.remove( 0, 1 );

	QFileInfo fileInfo = QFileInfo( sPath );

	if ( !fileInfo.isAbsolute() ) {
		ERRORLOG( QString( "Error: Unable to handle sPath [%1]. Please provide "
						   "an absolute file sPath!" )
					  .arg( sPath ) );
		return false;
	}

	if ( fileInfo.exists() ) {
		if ( !fileInfo.isReadable() ) {
			ERRORLOG( QString( "Unable to handle sPath [%1]. You must have "
							   "permissions to read the file!" )
						  .arg( sPath ) );
			return false;
		}
	}
	else if ( bCheckExistance ) {
		ERRORLOG( QString( "Provided %1 [%2] does not exist" )
					  .arg( ArtifactToQString( artifact ) )
					  .arg( sPath ) );
		return false;
	}

	if ( fileInfo.suffix() != suffix ) {
		ERRORLOG(
			QString( "Unable to handle sPath [%1]. The provided file must "
					 "have the suffix '%2'!" )
				.arg( sPath )
				.arg( sExtension )
		);
		return false;
	}

	if ( artifact == Artifact::DrumkitExtracted &&
		 fileInfo.fileName() != DRUMKIT_XML ) {
		ERRORLOG(
			QString(
				"Provided drumkit definition [%1] must be called [%2] instead"
			)
				.arg( fileInfo.fileName() )
				.arg( DRUMKIT_XML )
		);
		return false;
	}

	return true;
}

QString Filesystem::validateFilePath( const QString& sPath )
{
	// Ensure the name will be a valid filename
	QString sValidName( sPath );
	sValidName.replace( " ", "_" );
	sValidName.remove( QRegularExpression(
		"[\\\\|\\/|\\*|\\,|\\$|:|=|@|!|\\^|&|\\?|\"|'|>|<|\\||%|:]+"
	) );

	return sValidName;
}

void Filesystem::info()
{
	INFOLOG( QString( "Tmp dir                    : %1" ).arg( tmpDir() ) );
	// SYS
	INFOLOG( QString( "Click file                 : %1" ).arg( clickFilePath() )
	);
	INFOLOG( QString( "Empty song                 : %1" )
				 .arg( emptyPath( Artifact::Song ) ) );
	INFOLOG( QString( "Empty playlist             : %1" )
				 .arg( emptyPath( Artifact::Playlist ) ) );
	INFOLOG( QString( "Demos dir                  : %1" ).arg( demosDir() ) );
	INFOLOG( QString( "Documentation dir          : %1" )
				 .arg( systemDocumentationDir() )
	);	// FIXME must be created even if no doc deployed
	INFOLOG(
		QString( "System drumkit dir         : %1" ).arg( systemDrumkitsDir() )
	);
	INFOLOG(
		QString( "Empty sample               : %1" ).arg( emptySamplePath() )
	);
	INFOLOG(
		QString( "Default config             : %1" ).arg( systemConfigPath() )
	);
	INFOLOG( QString( "Internationalization dir   : %1" )
				 .arg( systemInternationalizationDir() ) );
	INFOLOG(
		QString( "Images dir                 : %1" ).arg( systemImageDir() )
	);
	// USR
	INFOLOG(
		QString( "User config                : %1" ).arg( userConfigPath() )
	);
	INFOLOG( QString( "Cache dir                  : %1" ).arg( cacheDir() ) );
	INFOLOG( QString( "Reporitories Cache dir     : %1" )
				 .arg( repositoriesCacheDir() ) );
	INFOLOG(
		QString( "User drumkit dir           : %1" ).arg( userDrumkitsDir() )
	);
	INFOLOG(
		QString( "Patterns dir               : %1" ).arg( userPatternsDir() )
	);
	INFOLOG(
		QString( "Playlist dir               : %1" ).arg( userPlaylistsDir() )
	);
	INFOLOG(
		QString( "Scripts dir                : %1" ).arg( userScriptsDir() )
	);
	INFOLOG( QString( "Songs dir                  : %1" ).arg( userSongsDir() )
	);
}

QString Filesystem::absolutePath( const QString& sFileName, bool bSilent )
{
	if ( QFile( sFileName ).exists() ) {
		return QFileInfo( sFileName ).absoluteFilePath();
	}
	else if ( !bSilent ) {
		___ERRORLOG( QString( "File [%1] not found" ).arg( sFileName ) );
	}

	return QString();
}

QString Filesystem::rerouteDrumkitPath( const QString& sDrumkitPath )
{
#ifdef H2CORE_HAVE_APPIMAGE

	if ( sDrumkitPath.isEmpty() ) {
		ERRORLOG( "Can not reroute empty drumkit paths" );
		return "";
	}

	// Since the sPath to a system kits of a previously mounted image
	// does most probably not exist anymore we can _not_ use
	// Filesystem::absolutePath in here.
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
			 !sAbsolutePath.contains( Filesystem::systemDataPath() ) ) {
			bIsForeignSystemKit = true;
		}
	}

	if ( bIsForeignSystemKit ) {
		const QStringList pathComponents = sAbsolutePath.split( "/" );
		if ( pathComponents.size() > 2 ) {
			const QString sNewPath =
				QString( "%1%2/%3" )
					.arg( Filesystem::systemDataPath() )
					.arg( pathComponents[pathComponents.size() - 2] )
					.arg( pathComponents[pathComponents.size() - 1] );

			INFOLOG( QString( "Rerouting system kit: [%1] -> [%2]" )
						 .arg( sDrumkitPath )
						 .arg( Filesystem::absolutePath( sNewPath ) ) );

			sResult = Filesystem::absolutePath( sNewPath );
		}
		else {
			ERRORLOG( QString( "Unable to replace drumkit sPath [%1]" )
						  .arg( sDrumkitPath ) );
		}
	}

	return sResult;
#else
	return sDrumkitPath;
#endif
}

QString Filesystem::getDrumkitMap( const QString& sDrumkitName, bool bSilent )
{
	const QString sMapDir = systemDrumkitMapsDir();

	if ( !dirReadable( sMapDir ) ) {
		ERRORLOG( QString( "Unable to access system drumkit map folder [%1]" )
					  .arg( sMapDir ) );
		return QString();
	}

	QString sTarget =
		QString( "%1%2" ).arg( sDrumkitName ).arg( sDrumkitMapSuffix );

	QDir mapDir( sMapDir );
	// The mapping file must exactly match drumkit name.
	if ( !mapDir.exists( sTarget ) ) {
		WARNINGLOG(
			QString( "No .h2map fallback file for kit [%1] found. Please add "
					 "types in the kit's Properties dialog yourself" )
				.arg( sDrumkitName )
		);
		return QString();
	}

	if ( bSilent ) {
		INFOLOG( QString( "Found map file [%1] for kit [%2]" )
					 .arg( mapDir.filePath( sTarget ) )
					 .arg( sDrumkitName ) );
	}
	return mapDir.filePath( sTarget );
}

QString Filesystem::addUniquePrefix( const QString& sBaseFilePath )
{
	QString sChars(
		"abcdefghijklmnopqrstuvwxuzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
	);

	// a seed source for the random number engine
	std::random_device randomDevice;
	// seeded mersenne_twister engine producing random unsigned integers.
	std::mt19937 randomEngine( randomDevice() );
	// Transformation of the above random number to comply with our constraints.
	std::uniform_int_distribution<int> distr( 0, sChars.size() - 1 );

	// A prefix will be formatted as "tmp-XXXXXX-" with X being a latin
	// character or digit.
	auto createPrefix = [&]() {
		QString sPrefix( "tmp-" );
		for ( int ii = 0; ii < 6; ++ii ) {
			sPrefix.append( sChars.at( distr( randomEngine ) ) );
		}

		return std::move( sPrefix.append( "-" ) );
	};

	QFileInfo baseInfo( sBaseFilePath );

	QString sUniquePath = baseInfo.absoluteDir().absoluteFilePath(
		createPrefix() + baseInfo.fileName()
	);

	int maxTries = 100;
	int ii = 0;
	while ( fileExists( sUniquePath, true ) ) {
		sUniquePath = baseInfo.absoluteDir().absoluteFilePath(
			createPrefix() + baseInfo.fileName()
		);

		ii++;
		if ( ii >= maxTries ) {
			ERRORLOG( QString( "Unable to create unique sPath for [%1]" )
						  .arg( sBaseFilePath ) );
			return "";
		}
	}

	return std::move( sUniquePath );
}

QString
Filesystem::removeUniquePrefix( const QString& sUniqueFilePath, bool bSilent )
{
	QRegularExpression prefix( "tmp-[\\w]{6}-+" );

	if ( sUniqueFilePath.contains( prefix ) ) {
		QFileInfo info( sUniqueFilePath );

		return info.absoluteDir().absoluteFilePath(
			info.fileName().remove( prefix )
		);
	}
	else {
		if ( !bSilent ) {
			WARNINGLOG( QString( "sPath [%1] does not contain unique prefix" )
							.arg( sUniqueFilePath ) );
		}
		return sUniqueFilePath;
	}
}

QString Filesystem::getAutoSavePath(
	const Artifact& artifact,
	const QString& sBaseName
)
{
	QString sDefaultDir, sExtension;
	switch ( artifact ) {
		case Artifact::Song:
			sDefaultDir = userSongsDir();
			sExtension = Filesystem::sSongSuffix;
			break;

		case Artifact::Playlist:
			sDefaultDir = userPlaylistsDir();
			sExtension = Filesystem::sPlaylistSuffix;
			break;

		default:
			ERRORLOG( QString( "Unsupported file type: [%1]" )
						  .arg( ArtifactToQString( artifact ) ) );
			return "";
	}

	if ( !sBaseName.isEmpty() ) {
		QFileInfo fileInfo( sBaseName );

		// In case the user did open a hidden file, the baseName()
		// will be an empty string.
		QString sBaseName( fileInfo.completeBaseName() );
		if ( sBaseName.startsWith( "." ) ) {
			sBaseName.remove( 0, 1 );
		}

		const QString sAbsolutePath =
			QString( "%1/.%2.autosave%3" )
				.arg( fileInfo.absoluteDir().absolutePath() )
				.arg( sBaseName )
				.arg( sExtension );

		if ( !Filesystem::fileWritable( sAbsolutePath, true ) ) {
			QString sNewName = QString( "%1/.%2.autosave%3" )
								   .arg( sDefaultDir )
								   .arg( sBaseName )
								   .arg( sExtension );

			WARNINGLOG( QString( "sPath of current %1 [%2] is not writable. "
								 "Autosave will store it as [%3] instead." )
							.arg( ArtifactToQString( artifact ) )
							.arg( sAbsolutePath )
							.arg( sNewName ) );
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

QString Filesystem::ArtifactToQString( const Artifact& type )
{
	switch ( type ) {
		case Artifact::DrumkitBundled:
			return "Drumkit bundled";
		case Artifact::DrumkitExtracted:
			return "Drumkit definition";
		case Artifact::Pattern:
			return "Pattern";
		case Artifact::Playlist:
			return "Playlist";
		case Artifact::Song:
			return "Song";
		default:
			return QString( "Unknown artifact [%1]" )
				.arg( static_cast<int>( type ) );
	}
}

QString Filesystem::removeUtf8Characters( const QString& sEncodedString )
{
	QString sCleaned( sEncodedString );
	return sCleaned.remove(
		QRegularExpression( "[^a-zA-Z0-9._/\\s()\\[\\]\\&\\+\\-]" )
	);
}
const std::vector<Filesystem::AudioFormat>& Filesystem::supportedAudioFormats()
{
	return m_supportedAudioFormats;
}

QString Filesystem::appendNumberOrIncrement( const QString& sString )
{
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

QStringList Filesystem::targetDirs( Artifact artifact, Context context,
									Hydrogen* pHydrogen )
{
	QStringList results;

	if ( context == Context::Song ) {
		ERRORLOG( "There is no fixed target folder for song-level artifacts" );
		return results;
	}
	if ( ( context == Context::SessionReadOnly ||
		   context == Context::SessionReadWrite ) &&
		 artifact != Artifact::DrumkitExtracted ) {
		ERRORLOG( "Session-based folders are only supported for drumkits." );
		return results;
	}

	if ( context == Context::Custom ) {
		return pHydrogen->getPreferences()->getCustomSoundLibraryDirs();
	}

	switch ( artifact ) {
		case Artifact::DrumkitBundled: {
			ERRORLOG( "Bunled drumkits have to be imported first." );
			return results;
		}
		case Artifact::DrumkitExtracted: {
			if ( context == Context::System ) {
				results << systemDrumkitsDir();
			}
			else if ( context == Context::User ) {
				results << userDrumkitsDir();
			}
			else {
				if ( pHydrogen != nullptr &&
					 pHydrogen->getSoundLibraryDatabase() != nullptr ) {
					results << pHydrogen->getSoundLibraryDatabase()
								   ->getCustomDrumkitFolders();
				}
			}
			break;
		}
		case Artifact::Pattern: {
			if ( context == Context::System ) {
				results << systemPatternsDir();
			}
			else {
				results << userPatternsDir();
			}
			break;
		}
		case Artifact::Playlist: {
			if ( context == Context::System ) {
				ERRORLOG( "There are no system-level playlists yet." );
				return results;
			}
			else {
				results << userPlaylistsDir();
			}
			break;
		}
		case Artifact::Song: {
			if ( context == Context::System ) {
				results << systemSongsDir();
				results << demosDir();
			}
			else {
				results << userSongsDir();
			}
			break;
		}
	}
	return results;
}

QString Filesystem::targetFilter( Artifact artifact )
{
	switch ( artifact ) {
		case Artifact::DrumkitBundled:
			return DRUMKIT_FILTER;
		case Artifact::DrumkitExtracted:
			return DRUMKIT_XML;
		case Artifact::Pattern:
			return PATTERN_FILTER;
		case Artifact::Playlist:
			return PLAYLIST_FILTER;
		case Artifact::Song:
			return SONG_FILTER;
	}

	return "";
}
};	// namespace H2Core

/* vim: set softtabstop=4 noexpandtab: */
