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

#ifndef CORE_ACTION_CONTROLLER_H
#define CORE_ACTION_CONTROLLER_H

#include <vector>
#include <memory>

#include <core/Object.h>
#include <core/Basics/Song.h>

namespace H2Core
{
	class Drumkit;
	class Instrument;

/** \ingroup docCore docAutomation */
class CoreActionController : public H2Core::Object<CoreActionController> {
	H2_OBJECT(CoreActionController)
	
	public:
		CoreActionController();
		~CoreActionController();
	
		bool setMasterVolume( float masterVolumeValue );
		/**
		 * \param nStrip Instrument which to set the volume for.
		 * \param fVolumeValue New volume.
		 * \param bSelectStrip Whether the corresponding instrument
		 * should be selected.
		 */
		bool setStripVolume( int nStrip, float fVolumeValue, bool bSelectStrip );
		/**
		 * \param nStrip Instrument which to set the pan for.
		 * \param fValue New pan.
		 * \param bSelectStrip Whether the corresponding instrument
		 * should be selected.
		 */
		bool setStripPan( int nStrip, float fValue, bool bSelectStrip );
		/**
		 * \param nStrip Instrument which to set the pan for.
		 * \param fValue New pan. range in [-1;1] => symmetric respect to 0
		 * \param bSelectStrip Whether the corresponding instrument
		 * should be selected.
		 */
		bool setStripPanSym( int nStrip, float fValue, bool bSelectStrip );
		bool setInstrumentPitch( int nInstrument, float fValue );
		bool setMetronomeIsActive( bool isActive );
		bool setMasterIsMuted( bool isMuted );
		
		bool setStripIsMuted( int nStrip, bool isMuted );
		bool toggleStripIsMuted( int nStrip );
		
		bool setStripIsSoloed( int nStrip, bool isSoloed );
		bool toggleStripIsSoloed( int nStrip );
		
		bool initExternalControlInterfaces();
	
		// -----------------------------------------------------------
		// Actions required for session management.
		
		/**
		 * Create an empty #H2Core::Song, which will be stored in @a songPath.
		 *
		 * This will be done immediately and without saving
		 * the current #H2Core::Song. All unsaved changes will be lost! In
		 * addition, the new song won't be saved by this function. You
		 * can do so using saveSong().
		 *
		 * The intended use of this function for session
		 * management. Therefore, the function will *not* store the
		 * provided @a songPath in Preferences::m_lastSongFilename and
		 * Hydrogen won't resume with the corresponding song on
		 * restarting.
		 *
		 * \param songPath Absolute path to the .h2song file to be
		 *    opened.
		 * \return true on success
		 */
		bool newSong( const QString& songPath );
		/**
		 * Opens the #H2Core::Song specified in @a songPath.
		 *
		 * This will be done immediately and without saving
		 * the current #H2Core::Song. All unsaved changes will be lost!
		 *
		 * \param songPath Absolute path to the .h2song file to be
		 *    opened.
		 * \param sRecoverSongPath If set to a value other than "",
		 *    the corresponding path will be used to load the song and
		 *    the latter is assigned @a songPath as Song::m_sFilename
		 *    afterwards. Using this mechanism the GUI can use an
		 *    autosave backup file to load a song without the core
		 *    having to do some string magic to retrieve the original name.
		 * \return true on success
		 */
	bool openSong( const QString& songPath, const QString& sRecoverSongPath = "" );
		/**
		 * Opens the #H2Core::Song specified in @a songPath.
		 *
		 * This will be done immediately and without saving
		 * the current #H2Core::Song. All unsaved changes will be lost!
		 *
		 * \param pSong New Song.
		 * \param bRelinking Whether the drumkit last loaded should be
		 * relinked when under session management. This flag is used
		 * to distinguish between the regular load of a song file
		 * within a session and its replacement by another song (which
		 * requires an update of the linked drumkit).
		 * \return true on success
		 */
		bool openSong( std::shared_ptr<Song> pSong, bool bRelinking = true );
		/**
		 * Saves the current #H2Core::Song.
		 *
		 * \return true on success
		 */
		bool saveSong();
		/**
		 * Saves the current #H2Core::Song to the path provided in @a sNewFilename.
		 *
		 * The intended use of this function for session
		 * management. Therefore, the function will *not* store the
		 * provided @a sNewFilename in
		 * #H2Core::Preferences::m_lastSongFilename and Hydrogen won't
		 * resume with the corresponding song on restarting.
		 *
		 * \param sNewFilename Absolute path to the file to store the
		 *   current #H2Core::Song in.
		 * \return true on success
		 */
		bool saveSongAs( const QString& sNewFilename );
		/**
		 * Saves the current state of the #H2Core::Preferences.
		 *
		 * \return true on success
		 */
		bool savePreferences();
		/**
		 * Triggers the shutdown of Hydrogen.
		 *
		 * This will be done immediately and without saving the
		 * current #H2Core::Song. All unsaved changes will be lost!
		 *
		 * The shutdown will be triggered in both the CLI and the GUI
		 * via the #H2Core::EVENT_QUIT event.
		 *
		 * \return true on success
		 */
		bool quit();

		// -----------------------------------------------------------
		// Further OSC commands

		/**
		 * (De)activates the usage of the Timeline.
		 *
		 * Note that this function will fail in the presence of both JACK audio
		 * driver and an external Timebase controller (see
		 * Hydrogen::getJackTimebaseState()).
		 *
		 * @param bActivate If true - activate or if false -
		 * deactivate.
		 *
		 * @return bool true on success
		 */
		bool activateTimeline( bool bActivate );
		/**
		 * Adds a tempo marker to the Timeline.
		 *
		 * @param nPosition Location of the tempo marker in bars.
		 * @param fBpm Speed associated with the tempo marker.
		 *
		 * @return bool true on success
		 */
		bool addTempoMarker( int nPosition, float fBpm );
		/**
		 * Delete a tempo marker from the Timeline.
		 *
		 * If no Tempo marker is present at @a nPosition, the function
		 * will return true as well.
		 *
		 * @param nPosition Location of the tempo marker in bars.
		 *
		 * @return bool true on success
		 */
		bool deleteTempoMarker( int nPosition );
		/**
		 * Adds a tag to the Timeline.
		 *
		 * @param nPosition Location of the tag in bars.
		 * @param sText Message associated with the tag.
		 *
		 * @return bool true on success
		 */
		bool addTag( int nPosition, const QString& sText );
		/**
		 * Delete a tag from the Timeline.
		 *
		 * If no tag is present at @a nPosition, the function
		 * will return true as well.
		 *
		 * @param nPosition Location of the tag in bars.
		 *
		 * @return bool true on success
		 */
		bool deleteTag( int nPosition );
		/**
		 * (De)activates the usage of Jack transport.
		 *
		 * Note that this function will fail if Jack is not used as
		 * audio driver.
		 *
		 * @param bActivate If true - activate or if false -
		 * deactivate.
		 *
		 * @return bool true on success
		 */
		bool activateJackTransport( bool bActivate );
		/**
		 * (Un)registers Hydrogen as JACK Timebaes constroller.
		 *
		 * Note that this function will fail if JACK is not used as audio
		 * driver.
		 *
		 * @param bActivate If true - activate or if false -
		 * deactivate.
		 *
		 * @return bool true on success
		 */
		bool activateJackTimebaseControl( bool bActivate );

		/**
		 * Switches between Song and Pattern mode of playback.
		 *
		 * @param bActivate If true - activates Song mode or if false -
		 * activates Pattern mode.
		 *
		 * @return bool true on success
		 */
		bool activateSongMode( bool bActivate );
	     /**
		 * Toggle loop mode of playback.
		 *
		 * @param bActivate If true - activates loop mode.
		 *
		 * @return bool true on success
		 */
		bool activateLoopMode( bool bActivate );
	/** Wrapper around setDrumkit() that allows loading drumkits by
	 *	name or path.
	 *
	 * The function tries to retrieve the #Drumkit from cache
	 * (#SoundLibraryDatabase) first and loads it from disk in case
	 * this fails.
	 *
	 * @param sDrumkit Can be either the name of a #Drumkit or a
	 * relative or absolute path pointing to it.
	 * \param bConditional Whether to remove all redundant
	 * H2Core::Instrument regardless of their content.
	 */
	bool setDrumkit( const QString& sDrumkit, bool bConditional = true );
	/**
	 * Sets Drumkit @a pDrumkit as the one used in the current #Song.
	 *
	 * The loading will overwrite the #InstrumentList of the current
	 * #Song with the one found in @a pDrumkit (among other things)
	 * and also can be used to reset the parameters of the current
	 * drumkit to its default values.
	 *
	 * \param pDrumkit Full-fledged #H2Core::Drumkit to load.
	 * \param bConditional Whether to remove all redundant
	 * H2Core::Instrument regardless of their content.
	 */
	bool setDrumkit( std::shared_ptr<Drumkit> pDrumkit, bool bConditional = true );
	/** 
	 * Upgrades the drumkit found at absolute path @a sDrumkitPath.
	 *
	 * If @a sNewPath is missing, the drumkit will be upgraded in
	 * place and a backup file will be created in order to not
	 * overwrite the existing state. 
	 */
	bool upgradeDrumkit( const QString& sDrumkitPath, const QString& sNewPath = "" );

	/**
	 * Checks whether the provided drumkit in @a sDrumkitPath can be
	 * found, can be loaded, and does comply with the current XSD
	 * definition.
	 *
	 * @param sDrumkitPath Can be either an absolute path to a folder
	 *   containing a drumkit file (drumkit.xml), an absolute path to a
	 *   drumkit file itself, or an absolute file to a compressed
	 *   drumkit (.h2drumkit).
	 * @param bCheckLegacyVersions Whether just the current XSD
	 *   definition or also all previous versions should be checked.
	 */
	bool validateDrumkit( const QString& sDrumkitPath, bool bCheckLegacyVersions = false );
		/**
		 * Extracts the compressed .h2drumkit file in @a sDrumkitPath into @a
		 * sTargetDir.
		 *
		 * The function does not automatically load the extracted kit into the
		 * current Hydrogen session in case a custom @a sTargetDir was supplied.
		 * To do so, the name of the folder contained in the tarball is required
		 * (might differ from the name of the tarball) and it is not easily
		 * obtained.
		 *
		 * \param sDrumkitPath Tar-compressed drumkit with .h2drumkit extension
		 * \param sTargetDir Folder to extract the drumkit to. If the folder is
		 *   not present yet, it will be created. If left empty, the drumkit
		 *   will be installed to the users drumkit data folder.
		 * \param pInstalledPath Will contain the actual name of the folder the
		 *   kit was installed to. In most cases this will coincide with a
		 *   folder within @a sTargetPath named like the kit itself. But in case
		 *   the system does not support UTF-8 encoding and @a sTargetPath
		 *   contains characters other than those whitelisted in
		 *   #Filesystem::removeUtf8Characters, those might be omitted and the
		 *   directory and files created using `libarchive` might differ.
		 * \param pEncodingIssuesDetected will be set to `true` in case at least
		 *   one filepath of extracted kit had to be altered in order to not run
		 *   into UTF-8 issues.
		 */
		bool extractDrumkit( const QString& sDrumkitPath,
							 const QString& sTargetDir = "",
							 QString* pInstalledPath = nullptr,
							 bool* pEncodingIssuesDetected = nullptr );
		/** Relocates transport to the beginning of a particular
		 * column/Pattern group.
		 * 
		 * @param nPatternGroup Position of the Song provided as the
		 * index of a particular pattern group (starting at zero).
		 *
		 * @return bool true on success
		 */
		bool locateToColumn( int nPatternGroup );
		/** Relocates transport to a particular tick.
		 * 
		 * @param nTick Destination
		 * \param bWithJackBroadcast Relocate not using the AudioEngine
		 * directly but using the JACK server.
		 *
		 * @return bool true on success
		 */
		bool locateToTick( long nTick, bool bWithJackBroadcast = true );

	    /** Creates an empty pattern and adds it to the pattern list.
		 *
		 * @param sPath Name for the created pattern.
		 *
		 * @return bool true on success
		 */
    	bool newPattern( const QString& sPatternName );
	    /** Opens a pattern from disk and adds it to the pattern list.
		 *
		 * @param sPath Absolute path to an existing .h2pattern file.
		 * @param nPatternNumber Row the pattern will be added to. If
		 * set to -1, the pattern will be appended at the end of the
		 * pattern list.
		 *
		 * @return bool true on success
		 */
    	bool openPattern( const QString& sPath, int nPatternNumber = -1 );
        /** Opens a pattern to the current pattern list.
		 *
		 * @param pPattern pattern to be added.
		 * @param nPatternNumber Row the pattern will be added to.
		 *
		 * @return bool true on success
		 */
		bool setPattern( Pattern* pPattern, int nPatternNumber );
	    /** Removes a pattern from the pattern list.
		 *
		 * @param nPatternNumber Specifies the position/row of the pattern.
		 *
		 * @return bool true on success
		 */
    	bool removePattern( int nPatternNumber );
		/** Deletes all notes for instrument @a pInstrument in a specified
		 * pattern.
		 *
		 * @param nInstrumentNumber target instrument
		 * @param nPatternNumber index of the target pattern in
		 *   Song::m_pPatternList in the current song. If set to -1, the
		 *   currently selected pattern will be used instead.
		 *
		 * @return bool true on success. */
		bool clearInstrumentInPattern( int nInstrumentNumber,
									   int nPatternNumber = -1 );
	    /** Fills or clears a specific grid cell in the SongEditor.
		 *
		 * @param nColumn column of the pattern.
		 * @param nRow row of the pattern.
		 *
		 * @return bool true on success
		 */
    	bool toggleGridCell( int nColumn, int nRow );

		/** Handle an incoming note event, e.g. a MIDI or OSC NOTE_ON or
		 * NOTE_OFF as well as virtual keyboard stroke.
		 *
		 * @param nNote determines which note will be triggered and is defined
		 *   between [36,127] inspired by the General MIDI standard.
		 * @param fVelocity how "hard" the note was triggered.
		 * @param bNoteOff whether note should trigger or stop sound.
		 *
		 * @return bool true on success */
		bool handleNote( int nNote, float fVelocity, bool bNoteOff = false );

	/**
	 * In case a different preferences file was loaded with Hydrogen
	 * already fully set up this function refreshes all corresponding
	 * values and informs the GUI.
	 */
	void updatePreferences();
private:
	bool sendMasterVolumeFeedback();
	bool sendStripVolumeFeedback( int nStrip );
	bool sendMetronomeIsActiveFeedback();
	bool sendMasterIsMutedFeedback();
	bool sendStripIsMutedFeedback( int nStrip );
	bool sendStripIsSoloedFeedback( int nStrip );
	bool sendStripPanFeedback( int nStrip );
	bool sendStripPanSymFeedback( int nStrip );
	
	bool handleOutgoingControlChanges( std::vector<int> params, int nValue);
	std::shared_ptr<Instrument> getStrip( int nStrip ) const;
	
	// -----------------------------------------------------------
	// Actions required for session management.
		
		/**
		 * Sets a #H2Core::Song to be used by Hydrogen.
		 *
		 * This will be done immediately and without saving the
		 * current #H2Core::Song. All unsaved changes will be lost!
		 *
		 * \param pSong Pointer to the #H2Core::Song to set.
		 * \param bRelinking Whether the drumkit last loaded should be
		 * relinked when under session management. This flag is used
		 * to distinguish between the regular load of a song file
		 * within a session and its replacement by another song (which
		 * requires an update of the linked drumkit).
		 * \return true on success
		 */
		bool setSong( std::shared_ptr<Song> pSong, bool bRelinking = true );

	/**
	 * Loads the drumkit specified in @a sDrumkitPath.
	 *
	 * \param sDrumkitPath Can be either an absolute path to a folder
	 * containing a drumkit file (drumkit.xml), an absolute path to a
	 * drumkit file itself, or an absolute file to a compressed
	 * drumkit (.h2drumkit).
	 * \param bIsCompressed Stores whether the drumkit was provided as
	 * a compressed .h2drumkit file
	 * \param sDrumkitDir Stores the folder containing the drumkit
	 * file. If a compressed drumkit was provided, this will point to
	 * a temporary folder.
	 * \param sTemporaryFolder Root path of a temporary folder
	 * containing the extracted drumkit in case @a sDrumkitPath
	 * pointed to a compressed .h2drumkit file.
	 */
	std::shared_ptr<Drumkit> retrieveDrumkit( const QString& sDrumkitPath, bool* bIsCompressed,
											  QString* sDrumkitDir, QString* sTemporaryFolder );
	/**
	 * Add @a sFilename to the list of recent songs in
	 * Preferences::m_recentFiles.
	 *
	 * The function will also take care of removing any duplicates in
	 * the list in case @a sFilename is already present.
	 *
	 * \param sFilename New song to be added on top of the list.
	 */
	void insertRecentFile( const QString sFilename );
		
		const int m_nDefaultMidiFeedbackChannel;
};

}
#endif
