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

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <QString>
#include <vector>

#include <core/Object.h>

namespace H2Core {

/**
 * Filesystem is a thin layer over QDir, QFile and QFileInfo
 */
/** \ingroup docCore*/
class Filesystem : public H2Core::Object<Filesystem> {
	H2_OBJECT( Filesystem )
   public:
	/** flags available for checkPermissions() */
	enum FilePermission {
		IsDir = 0x01,
		IsFile = 0x02,
		IsReadable = 0x04,
		IsWritable = 0x08,
		IsExecutable = 0x10
	};

	/** Indicates usage, storage, and access permissions of a kit.*/
	enum class Context {
		/** Kit is located in the system-level drumkit folder, loaded into the
		 * #H2Core::SoundlibraryDatabase during startup, and is read-only.*/
		System = 0,
		/** Kit is located in the user-level drumkit folder, loaded into the
		 * #H2Core::SoundlibraryDatabase during startup, and can be modified.*/
		User = 1,
		/** Kit is located at an arbitrary location of the host system and was
		 * loaded into Hydrogen during a session using e.g. OSC or its location
		 * was provided during startup. It is transient and located in a place
		 * the user only has read-only access and can not be modified.*/
		SessionReadOnly = 2,
		/** Kit is located at an arbitrary location of the host system and was
		 * loaded into Hydrogen during a session using e.g. OSC or its location
		 * was provided during startup. It is transient and can be modified.*/
		SessionReadWrite = 3,
		/** In contrast to the other contexts this drumkit was not loaded from a
		 * .h2drumkit or a drumkit.xml file within a drumkit folder. Instead, it
		 * is part of a song and loaded with a .h2song or created with a new
		 * song. It is stored with the song when saving the song and can be
		 * converted into a regular kit by saving / exporting the drumkit. All
		 * its metadata, like drumkit image, end up in a cache folder for
		 * Hydrogen.*/
		Song = 4
	};
	static QString ContextToQString( const Context& context );
	static Context DetermineContext( const QString& sPath );

	/** Indicates what type of file - .h2song, .h2playlist, .h2pattern,
	 * drumkit.xml, .h2drumkit - a function as to handle. This covers files
	 * intended to be used and shared the the user herself. (.h2theme,
	 * hydrogen.conf, and .h2map are more considered internal ones for now).
	 */
	enum class Artifact {
		/** A .h2drumkit file. */
		DrumkitBundled,
		/** A drumkit.xml file. */
		DrumkitExtracted,
		/** A .h2pattern file. */
		Pattern,
		/** A .h2playlist file. */
		Playlist,
		/** A .h2song file. */
		Song
	};
	static QString ArtifactToQString( const Artifact& actifact );

	/** All audio file formats supported by Hydrogen */
	enum class AudioFormat {
		/** synonym for Aiff */
		Aif,
		/** synonym for Aiff */
		Aifc,
		Aiff,
		Au,
		Caf,
		Flac,
		Mp3,
		Ogg,
		Opus,
		Unknown,
		Voc,
		W64,
		Wav
	};
	/** Converts @a format to the default lower case suffix of the
	 * format. */
	static QString
	AudioFormatToSuffix( const AudioFormat& format, bool bSilent = false );
	/** Determines the audio format of the provided filename or path
	 * based on its suffix. */
	static AudioFormat
	AudioFormatFromSuffix( const QString& sFile, bool bSilent = false );

	static const QString sDrumkitSuffix;
	static const QString sDrumkitMapSuffix;
	static const QString sPatternSuffix;
	static const QString sPlaylistSuffix;
	static const QString sScriptSuffix;
	static const QString sSongSuffix;
	static const QString sThemeSuffix;

	static const QString sDrumkitFilter;
	static const QString sPatternFilter;
	static const QString sPlaylistFilter;
	static const QString sScriptFilter;
	static const QString sSongFilter;
	static const QString sThemeFilter;

	/**
	 * check user and system filesystem usability
	 *
	 * If any argument is not provided or empty, the corresponding default
	 * values will be used.
	 *
	 * @param logger is a pointer to the logger instance which will be used
	 * @param sSysDataPath path to an alternate system data folder
	 * @param sUsrDataPath path to an alternate user data folder
	 * @param sUserConfigFile path to an alternate hydrogen.conf config file
	 * @param sLogFile path to alternate log file
	 */
	static bool bootstrap(
		Logger* logger,
		const QString& sSysDataPath = "",
		const QString& sUsrDataPath = "",
		const QString& sUserConfigFile = "",
		const QString& sLogFile = ""
	);

	/** returns system data path */
	static const QString& systemDataPath();
	/** returns user data path */
	static const QString& userDataPath();

	/** returns user ladspa paths */
	static const QStringList& ladspaPaths();

	/** returns system config path */
	static QString systemConfigPath();
	/** @return user config path. This is either
	 * $HOME/.hydrogen/hydrogen.conf or the alternative path provided via
	 * CLI (see #m_sPreferencesOverwritePath) */
	static QString userConfigPath();
	/** returns system empty sample file path */
	static QString emptySamplePath();
	/**
	 * Provides the full path to the current empty song or playlist.
	 *
	 * The basename consists of a fixed expression and an optional
	 * suffix ensuring the path does not point to an existing
	 * file.
	 *
	 * Empty songs are handled in Hydrogen as follows: Upon
	 * creation, the empty song will be assigned a
	 * Song::m_sFileName identical to the return value of this
	 * function. This triggers autosave files to be generated
	 * corresponding to empty song path. If the user attempts to
	 * save the song from within the GUI, she will be prompted a
	 * "Save As" dialog and is asked to provide a new name. This
	 * way a file using the empty song path does normally not
	 * exists. But since the return value of this function is
	 * reproducible, Hydrogen is able to recover unsaved
	 * modifications applied to an empty song. If the user - by
	 * design or coincidence - picks the empty song path to save a
	 * file or if the OSC API is used to save the empty song,
	 * emptyPath() will use a suffix to return yet again a
	 * path to a non-existing file and allow for the behavior
	 * described above.
	 */
	static QString emptyPath( const Artifact& artifact );
	/** Default option to offer the user when saving an empty song
		to disk.*/
	static QString defaultSongName();
	/** Returns a string containing the path to the
		_click.wav_ file used in the metronome.
		*
		* It is a concatenation of #m_sSystemDataPath and
		* #CLICK_SAMPLE.
		*/
	static QString clickFilePath();
	/** returns the full path (including filename) of the logfile */
	static const QString& logFilePath();

	/** returns gui image path */
	static QString systemImageDir();
	/** returns documentation path */
	static QString systemDocumentationDir();
	/** returns internationalization path */
	static QString systemInternationalizationDir();
	/** returns user scripts path */
	static QString userScriptsDir();
	/** returns system songs path (demos directory) */
	static QString systemSongsDir();
	/** returns user songs path */
	static QString userSongsDir();
	/** returns user patterns path */
	static QString userPatternsDir();
	/** returns system patterns path */
	static QString systemPatternsDir();
	/** returns user plugins path */
	static QString userPluginsDir();
	/** returns system drumkits path */
	static QString systemDrumkitsDir();
	/** returns user drumkits path */
	static QString userDrumkitsDir();
	static QString systemDrumkitMapsDir();
	/** returns user playlist path */
	static QString userPlaylistsDir();
	/** returns untitled playlist file name */
	static QString untitledPlaylistFileName();
	/** returns user cache path */
	static QString cacheDir();
	/** returns user repository cache path */
	static QString repositoriesCacheDir();
	/** returns system demos path */
	static QString demosDir();
	/** returns temp path */
	static QString tmpDir();
	static QString userThemesDir();
	static QString systemThemesDir();
	/**
	 * touch a temporary file under tmpDir() and return it's path.
	 * if base has a suffix it will be preserved, spaces will be replaced by
	 * underscores. @param base part of the path
	 */
	static QString tmpFilePath( const QString& sBase );

	/** Searches the folder corresponding to @a artifact and @a context
	 * recursively.
	 *
     * @param sUserDirOverwrite Must only be used in unit tests.
	 *
	 * @returns a list of absolute file paths. */
	static QStringList listContent(
		Artifact artifact,
		Context context,
		const QString& sUserDirOverwrite = ""
	);

	/* DRUMKIT */
	/** Returns the basename if the given path is under an instrument of an
	 * existing user or system drumkit path, otherwise the given @a
	 * sSamplePath */
	static QString prepareSamplePath(
		const QString& sSamplePath,
		const QString& sDrumkitPath
	);

	/**
	 * Returns filename and extension of the expected drumkit file.
	 */
	static QString drumkitXml();
	/** Prior to version 2.0 not the absolute path to the drumkit.xml file but
	 * the absolute path to the folder that files was contained in was used as
	 * drumkit path. This method ensure the path does point to the drumkit.xml
	 * file. In case it was not valid at all, an empty string is returned and an
	 * error logged. */
	static QString sanitizeDrumkitPath( const QString& sDrumkitPath );

	/** Expects a path to the drumkit.xml file of a drumkit and returns the
	 * absolute path to the folder it is contained in. The drumkit.xml file or
	 * the folder itself, however, do not have to exist yet. */
	static QString drumkitDirFromPath( const QString& sDrumkitPath );
	/** Expects a path to directory containing a drumkit and returns the
	 * absolute path to the contained drumkit.xml definition. The
	 * drumkit.xml file or the folder itself, however, do not have to exist
	 * yet. */
	static QString drumkitPathFromDir( const QString& sDrumkitDir );

	/**
	 * Create a backup path from a drumkit path. It will contain
	 * the current datetime to both make individual backup names
	 * unique and to make it more easy to handle them.
	 */
	static QString drumkitBackupPath( const QString& sDrumkitPath );

	/**
	 * Checks the path @a sPath.
	 *
	 * It will be checked whether @a sPath
	 * - is absolute
	 * - exists (if @a bCheckExistance is set to true)
	 * - has the suffix corresponding to @a artifact
	 * - is writable (read-only files are considered valid as well
	 *   and the function returns `true`. But it also triggers an
	 *   event informing the GUI to show a read-only warning.)
	 *
	 * @param artifact Whether the file is a .h2song or .h2playlist etc.
	 * @param sPath Absolute path to a file of artifact @a artifact.
	 * @param bCheckExistance Whether the existence of the file is
	 *   checked (should be true for opening and false for creating
	 *   a new song/playlist)
	 * \return true - if valid.
	 */
	static bool isPathValid(
		const Artifact& artifact,
		const QString& sPath,
		bool bCheckExistance = false
	);

	/**
	 * Takes an arbitrary path, replaces white spaces by
	 * underscores and removes all characters apart from latin
	 * characters, arabic numbers, underscores and dashes.
	 */
	static QString validateFilePath( const QString& sPath );

	/** send current settings information to logger with INFO severity */
	static void info();

	/**
	 * returns true if the given path is an existing regular file
	 * @param sPath the path to the file to check
	 * @param bSilent output not messages if set to true
	 */
	static bool fileExists( const QString& sPath, bool bSilent = false );
	/**
	 * returns true if the given path is an existing readable regular file
	 * @param sPath the path to the file to check
	 * @param bSilent output not messages if set to true
	 */
	static bool fileReadable( const QString& sPath, bool bSilent = false );
	/**
	 * @returns true if the given path is a possibly writable file (may exist or
	 *    not)
	 * @param sPath the path to the file to check
	 * @param bSilent output not messages if set to true
	 */
	static bool fileWritable( const QString& sPath, bool bSilent = false );
	/**
	 * @returns true if the given path is an existing executable regular file
	 * @param sPath the path to the file to check
	 * @param bSilent output not messages if set to true
	 */
	static bool fileExecutable( const QString& sPath, bool bSilent = false );
	/**
	 * @returns true if the given path is a regular directory
	 * @param sPath the path to the file to check
	 * @param bSilent output not messages if set to true
	 */
	static bool dirExists( const QString& sPath, bool bSilent = false );
	/**
	 * @returns true if the given path is a readable regular directory
	 * @param sPath the path to the file to check
	 * @param bSilent output not messages if set to true
	 */
	static bool dirReadable( const QString& sPath, bool bSilent = false );
	/**
	 * @returns true if the given path is a writable regular directory
	 * @param sPath the path to the file to check
	 * @param bSilent output not messages if set to true
	 */
	static bool dirWritable( const QString& sPath, bool bSilent = false );
	/**
	 * @returns true if the path is a readable and writable regular directory,
	 *    create if it not exists
	 * @param path the path to the file to check
	 * @param bCreate will try to create path if not exists and set to true
	 * @param bSilent output not messages if set to true
	 */
	static bool pathUsable(
		const QString& sPath,
		bool bCreate = true,
		bool bSilent = false
	);

	/**
	 * Convert a direct to an absolute path.
	 */
	static QString
	absolutePath( const QString& sFileName, bool bSilent = false );
	/**
	 * copy a source file to a destination
	 * @param sSourcePath source file path
	 * @param sDestinationPath destination file path
	 * @param bOverwrite allow to overwrite an existing file if set to true
	 * @param bSilent Whether debug and info messages should be
	 * logged.
	 */
	static bool fileCopy(
		const QString& sSourcePath,
		const QString& sDestinationPath,
		bool bOverwrite = false,
		bool bSilent = false
	);
	/**
	 * remove a path
	 * @param sPath the path to be removed
	 * @param bRecursive perform recursive removal if set to true
	 * @param bSilent Whether debug and info messages should be
	 * logged.
	 */
	static bool
	rm( const QString& sPath, bool bRecursive = false, bool bSilent = false );
	/**
	 * create a path
	 * @param sPath the path to the directory to be created
	 */
	static bool mkdir( const QString& sPath );

	/** \return m_sPreferencesOverwritePath*/
	static const QString& getPreferencesOverwritePath();
	/** @param sPath Sets m_sPreferencesOverwritePath*/
	static void setPreferencesOverwritePath( const QString& sPath );

	/** Retrieves a #H2Core::DrumkitMap file for a kit names @a sDrumkitName
	 *
	 * @param sDrumkitName Name of a drumkit.
	 * @param bSilent Whether to output additional log messages.
	 *
	 * @return an empty string in case no file was found.
	 **/
	static QString
	getDrumkitMap( const QString& sDrumkitName, bool bSilent = false );

	/**
	 * Reroutes stored drumkit paths pointing to a temporary
	 * AppImage system data folder to the current AppImage one.
	 *
	 * Since AppImages are mounted at a random point each time
	 * they are started the absolute path of their system data
	 * folder changes every time. To nevertheless support using
	 * system drumkits consistently, we try to determine whether
	 * the stored kit is a system one and tweak its path. (Without
	 * replacing the random part of the path the lookup would fall
	 * back to a name-based one which _always_ checks user-level
	 * kits first. Having a GMRockKit in ~/.hydrogen/data/drumkits
	 * too, would make the system's one inaccessible).
	 *
	 * @param sDrumkitPath Absolute path that need rerouting.
	 */
	static QString rerouteDrumkitPath( const QString& sDrumkitPath );

	/** In order to store stuff in e.g. our cache folder without neither
	 * loosing their original file names nor risking to overwrite
	 * existing files, this function addes a template prefix to a
	 * path.
	 *
	 * Companion function of deleteUniquePrefix() */
	static QString addUniquePrefix( const QString& sBaseFilePath );
	/** If @a sUniqueFilePath contains a prefix introduced by
	 * addUniquePrefix(), this function removes it and restores the
	 * original base name of the file.*/
	static QString
	removeUniquePrefix( const QString& sUniqueFilePath, bool bSilent = false );

	static QString
	getAutoSavePath( const Artifact& artifact, const QString& sBaseName );
	/** Removes all characters not within the Latin-1 range of @a
	 * sEncodedString. */
	static QString removeUtf8Characters( const QString& sEncodedString );

	/** Which format is supported is determined by the `libsndfile`
	 * version Hydrogen is linked against during compile time (see
	 * https://libsndfile.github.io/libsndfile/api.html#open). */
	static const std::vector<AudioFormat>& supportedAudioFormats();

	/** Adds a '2' separated by a whitespace at the end of the string.
	 * If there is already a number appended, it will be incremented
	 * instead. This can be used when duplicating things. */
	static QString appendNumberOrIncrement( const QString& sString );

   private:
	static bool checkSystemPaths(
	);	///< returns true if the system path is consistent
	static bool checkUserPaths(
	);	///< returns true if the user path is consistent
	static bool rmForceRecursive(
		const QString& sPath,
		bool bSilent = false
	);	///< recursively remove a path

	/**
	 * If this variable is non-empty, its content will be used as
	 * an alternative to store and load the preferences.*/
	static QString m_sPreferencesOverwritePath;
	/**
	 * \return true if all the asked permissions are ok
	 * @param sPath the path to the file to check
	 * @param nFilePermission bit mask of FilePermission
	 * @param bSilent output not messages if set to true
	 */
	static bool checkPermissions(
		const QString& sPath,
		const int nFilePermission,
		bool bSilent
	);
	static QStringList targetDirs( Artifact artifact, Context context );
	static QString targetFilter( Artifact artifact );

	static Logger* m_pLogger;  ///< a pointer to the logger

	/**
	 * Path to the system files set in Filesystem::bootstrap().
	 *
	 * If Q_OSMACX is set, it will be a concatenation of
	 * QCoreApplication::applicationDirPath() and
	 * "/../Resources/data/" (H2CORE_HAVE_BUNDLE defined)
	 * or "/data/" (else). If, instead, WIN32 is set, it
	 * is a concatenation of
	 * QCoreApplication::applicationDirPath() and
	 * "/data/". In case the application is neither run on
	 * Mac or Windows, it is set to a concatenation of
	 * H2_SYS_PATH and "/data/".
	 *
	 * If Filesystem::bootstrap() was called with the @a
	 * sys_path argument preset, it will overwrite all the
	 * choices above.
	 *
	 * Finally, if the variable doesn't point to a
	 * readable directory afterwards, it is set to a
	 * concatenation of
	 * QCoreApplication::applicationDirPath(), "/", and
	 * LOCAL_DATA_PATH.
	 */
	static QString m_sSystemDataPath;  ///< the path to the system files
	static QString m_sUserDataPath;	   ///< the path to the user files
	static QString m_sUserConfigPath;  ///< the path to the user config file
	static QString m_sUserLogPath;	   ///< the path to the log file
	static QStringList m_ladspaPaths;  ///< paths to laspa plugins
	static std::vector<AudioFormat> m_supportedAudioFormats;
};

inline const QString& Filesystem::getPreferencesOverwritePath()
{
	return Filesystem::m_sPreferencesOverwritePath;
}
inline void Filesystem::setPreferencesOverwritePath( const QString& sPath )
{
	Filesystem::m_sPreferencesOverwritePath = sPath;
}

};		// namespace H2Core
#endif	// FILE_SYSTEM_H

/* vim: set softtabstop=4 noexpandtab: */
