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

#include <core/Basics/DrumkitMap.h>
#include <core/Object.h>

namespace H2Core
{
	class Drumkit;
	class GridPoint;
	class Instrument;
	class Pattern;
	class Playlist;
	struct PlaylistEntry;
	class Preferences;
	class Song;


/** \ingroup docCore docAutomation */
class CoreActionController : public H2Core::Object<CoreActionController> {
	H2_OBJECT(CoreActionController)
	
	public:
		static bool setMasterVolume( float masterVolumeValue );
		/**
		 * \param nStrip Instrument which to set the volume for.
		 * \param fVolumeValue New volume.
		 * \param bSelectStrip Whether the corresponding instrument
		 * should be selected.
		 */
		static bool setStripVolume( int nStrip, float fVolumeValue,
									bool bSelectStrip );
		/**
		 * \param nStrip Instrument which to set the pan for.
		 * \param fValue New pan.
		 * \param bSelectStrip Whether the corresponding instrument
		 * should be selected.
		 */
		static bool setStripPan( int nStrip, float fValue, bool bSelectStrip );
		/**
		 * \param nStrip Instrument which to set the pan for.
		 * \param fValue New pan. range in [-1;1] => symmetric respect to 0
		 * \param bSelectStrip Whether the corresponding instrument
		 * should be selected.
		 */
		static bool setStripPanSym( int nStrip, float fValue, bool bSelectStrip );
		static bool setInstrumentPitch( int nInstrument, float fValue );
		static bool setMetronomeIsActive( bool isActive );
		static bool setMasterIsMuted( bool isMuted );
		static bool setHumanizeTime( float fValue );
		static bool setHumanizeVelocity( float fValue );
		static bool setSwing( float fValue );

		static bool setStripIsMuted( int nStrip, bool isMuted, bool bSelectStrip );
		static bool toggleStripIsMuted( int nStrip );
		
		static bool setStripIsSoloed( int nStrip, bool isSoloed, bool bSelectStrip );
		static bool toggleStripIsSoloed( int nStrip );

		static bool setStripEffectLevel( int nLine, int nEffect, float fValue,
										 bool bSelectStrip );
		
		static bool initExternalControlInterfaces();
	
		// -----------------------------------------------------------
		// Actions required for session management.

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
		 * \return nullptr on failure
		 */
	static std::shared_ptr<Song> loadSong( const QString& sSongPath,
										   const QString& sRecoverSongPath = "" );
		/**
		 * Sets a #H2Core::Song to be used by Hydrogen.
		 *
		 * This will be done immediately and without saving the
		 * current #H2Core::Song. All unsaved changes will be lost!
		 *
		 * \param pSong Pointer to the #H2Core::Song to set.
		 * \return true on success
		 */
		static bool setSong( std::shared_ptr<Song> pSong );
		/**
		 * Saves the current #H2Core::Song.
		 *
		 * \return true on success
		 */
		static bool saveSong();
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
		static bool saveSongAs( const QString& sNewFilename );
		/**
		 * Loads an instance of #H2Core::Preferences from the corresponding XML
		 * file. */
		static std::shared_ptr<Preferences> loadPreferences( const QString& sPath );
		/**
		 * Replaces the current #H2Core::Preferences singleton with the provided
		 * instance. */
		static bool setPreferences( std::shared_ptr<Preferences> pPreferences );
		/**
		 * Saves the current state of the #H2Core::Preferences. */
		static bool savePreferences();
		/**
		 * Triggers the shutdown of Hydrogen.
		 *
		 * This will be done immediately and without saving the
		 * current #H2Core::Song. All unsaved changes will be lost!
		 *
		 * The shutdown will be triggered in both the CLI and the GUI
		 * via the #H2Core::Event::Type::Quit event.
		 *
		 * \return true on success
		 */
		static bool quit();

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
		static bool activateTimeline( bool bActivate );
		static bool toggleTimeline();
		/**
		 * Adds a tempo marker to the Timeline.
		 *
		 * @param nPosition Location of the tempo marker in bars.
		 * @param fBpm Speed associated with the tempo marker.
		 *
		 * @return bool true on success
		 */
		static bool addTempoMarker( int nPosition, float fBpm );
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
		static bool deleteTempoMarker( int nPosition );
		/**
		 * Adds a tag to the Timeline.
		 *
		 * @param nPosition Location of the tag in bars.
		 * @param sText Message associated with the tag.
		 *
		 * @return bool true on success
		 */
		static bool addTag( int nPosition, const QString& sText );
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
		static bool deleteTag( int nPosition );
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
		static bool activateJackTransport( bool bActivate );
	static bool toggleJackTransport();
		/**
		 * (Un)registers Hydrogen as JACK Timebase constroller.
		 *
		 * Note that this function will fail if JACK is not used as audio
		 * driver.
		 *
		 * @param bActivate If true - activate or if false -
		 * deactivate.
		 *
		 * @return bool true on success
		 */
		static bool activateJackTimebaseControl( bool bActivate );
	static bool toggleJackTimebaseControl();

		/**
		 * Switches between Song and Pattern mode of playback.
		 *
		 * @param bActivate If true - activates Song mode or if false -
		 * activates Pattern mode.
		 *
		 * @return bool true on success
		 */
		static bool activateSongMode( bool bActivate );
	static bool toggleSongMode();
	     /**
		 * (De)activates loop mode of playback.
		 *
		 * @param bActivate If true - activates loop mode.
		 *
		 * @return bool true on success
		 */
		static bool activateLoopMode( bool bActivate );
	static bool toggleLoopMode();
		static bool activateRecordMode( bool bActivate );
		static bool toggleRecordMode();
	/** Wrapper around setDrumkit() that allows loading drumkits by
	 *	name or path.
	 *
	 * The function tries to retrieve the #Drumkit from cache
	 * (#SoundLibraryDatabase) first and loads it from disk in case
	 * this fails.
	 *
	 * @param sDrumkit Can be either the name of a #Drumkit or a
	 * relative or absolute path pointing to it.
	 */
	static bool setDrumkit( const QString& sDrumkit );
	/**
	 * Sets Drumkit @a pDrumkit as the one used in the current #Song.
	 *
	 * The loading will overwrite the #InstrumentList of the current
	 * #Song with the one found in @a pDrumkit (among other things)
	 * and also can be used to reset the parameters of the current
	 * drumkit to its default values.
	 *
	 * \param pDrumkit Full-fledged #H2Core::Drumkit to load.
	 */
	static bool setDrumkit( std::shared_ptr<Drumkit> pDrumkit );
	/** 
	 * Upgrades the drumkit found at absolute path @a sDrumkitPath.
	 *
	 * If @a sNewPath is missing, the drumkit will be upgraded in
	 * place and a backup file will be created in order to not
	 * overwrite the existing state. 
	 */
	static bool upgradeDrumkit( const QString& sDrumkitPath, const QString& sNewPath = "" );

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
	static bool validateDrumkit( const QString& sDrumkitPath, bool bCheckLegacyVersions = false );
	/**
	 * Extracts the compressed .h2drumkit file in @a sDrumkitPath into
	 * @a sTargetDir.
	 *
	 * The function does not automatically load the extracted kit into
	 * the current Hydrogen session in case a custom @a sTargetDir was
	 * supplied. To do so, the name of the folder contained in the
	 * tarball is required (might differ from the name of the tarball)
	 * and it is not easily obtained.
	 *
	 * \param sDrumkitPath Tar-compressed drumkit with .h2drumkit extension
	 * \param sTargetDir Folder to extract the drumkit to. If the folder is not
	 *   present yet, it will be created. If left empty, the drumkit will be
	 *   installed to the users drumkit data folder.
	 * \param pInstalledPath Will contain the actual name of the folder the kit
	 *   was installed to. In most cases this will coincide with a folder within
	 *   @a sTargetPath named like the kit itself. But in case the system does
	 *   not support UTF-8 encoding and @a sTargetPath contains characters other
	 *   than those whitelisted in #Filesystem::removeUtf8Characters, those
	 *   might be omitted and the directory and files created using `libarchive`
	 *   might differ.
	 * \param pEncodingIssuesDetected will be set to `true` in case at least one
	 *   filepath of extracted kit had to be altered in order to not run into
	 *   UTF-8 issues.
	 */
	static bool extractDrumkit( const QString& sDrumkitPath,
								const QString& sTargetDir = "",
								QString* pInstalledPath = nullptr,
								bool* pEncodingIssuesDetected = nullptr );

		/** Adds @a pInstrument to the current drumkit.
		 *
		 * In case @a nIndex is `-1` @a pInstrument will be appended to the
		 * instrument list.*/
		static bool addInstrument( std::shared_ptr<Instrument> pInstrument,
								   int nIndex = -1 );
		/** Removes @a pInstrument from the current drumkit and adds it to the
		 * instrument death row. This way it is guarantueed that its samples
		 * stay loaded until the last #H2Core::Note is done rendering it.
		 * Afterwards, its samples will be unloaded. */
		static bool removeInstrument( std::shared_ptr<Instrument> pInstrument );
		/** Replaces @a pOldInstrument by @a pNewInstrument in the current
		 * drumkit without clearing notes, changing the selected instrument
		 * number, etc. */
		static bool replaceInstrument( std::shared_ptr<Instrument> pNewInstrument,
									   std::shared_ptr<Instrument> pOldInstrument );
		/** Moves instrument @a nSourceIndex of the instrument list of the
		 * current drumkit to index @a nTargetIndex.
		 *
		 * Note that both @a nSourceIndex and @a nTargetIndex are the position
		 * within the instrument list and _not_ the ID of the instrument (which
		 * stays the same during the move action). */
		static bool moveInstrument( int nSourceIndex, int nTargetIndex );

		/** Changing the type of an instrument - specified using its ID - of the
		 * current drumkit.
		 *
		 * This one should be used over setDrumkit() since it is able to
		 * properly handling setting the initial type or removing a type string
		 * while honoring JACK per-track output ports. */
		static bool setInstrumentType( int nInstrumentId,
									   const DrumkitMap::Type& sType );

		/** Relocates transport to the beginning of a particular
		 * column/Pattern group.
		 * 
		 * @param nPatternGroup Position of the Song provided as the
		 * index of a particular pattern group (starting at zero).
		 *
		 * @return bool true on success
		 */
		static bool locateToColumn( int nPatternGroup );
		/** Relocates transport to a particular tick.
		 * 
		 * @param nTick Destination
		 * \param bWithJackBroadcast Relocate not using the AudioEngine
		 * directly but using the JACK server.
		 *
		 * @return bool true on success
		 */
		static bool locateToTick( long nTick, bool bWithJackBroadcast = true );

	    /** Creates an empty pattern and adds it to the pattern list.
		 *
		 * @param sPath Name for the created pattern.
		 *
		 * @return bool true on success
		 */
    	static bool newPattern( const QString& sPatternName );
	    /** Opens a pattern from disk and adds it to the pattern list.
		 *
		 * @param sPath Absolute path to an existing .h2pattern file.
		 * @param nPatternNumber Row the pattern will be added to. If
		 * set to -1, the pattern will be appended at the end of the
		 * pattern list.
		 *
		 * @return bool true on success
		 */
    	static bool openPattern( const QString& sPath, int nPatternNumber = -1 );
        /** Opens a pattern to the current pattern list.
		 *
		 * @param pPattern pattern to be added.
		 * @param nPatternNumber Row the pattern will be added to.
		 *
		 * @return bool true on success
		 */
		static bool setPattern( std::shared_ptr<Pattern> pPattern,
								int nPatternNumber );
		/** Selects a pattern from the current pattern list while taking into
		 * account whether the pattern editor is currently locked.
		 *
		 * @param nPatternNumber Row the pattern will be added to.
		 *
		 * @return bool true on success
		 */
		static bool selectPattern( int nPatternNumber );
		/** Removes a pattern from the pattern list.
		 *
		 * @param nPatternNumber Specifies the position/row of the pattern.
		 *
		 * @return bool true on success
		 */
    	static bool removePattern( int nPatternNumber );
		/** Deletes all notes for instrument @a pInstrument in a specified
		 * pattern.
		 *
		 * @param nInstrumentNumber target instrument
		 * @param nPatternNumber index of the target pattern in
		 *   Song::m_pPatternList in the current song. If set to -1, the
		 *   currently selected pattern will be used instead.
		 *
		 * @return bool true on success. */
		static bool clearInstrumentInPattern( int nInstrumentNumber,
											  int nPatternNumber = -1 );
	    /** Fills or clears a specific grid cell in the SongEditor.
		 *
		 * @param gridPoint position on the #SongEditor grid.
		 *
		 * @return bool true on success
		 */
    	static bool toggleGridCell( const GridPoint& gridPoint );

		/** Handle an incoming note event, e.g. a MIDI or OSC NOTE_ON or
		 * NOTE_OFF as well as virtual keyboard stroke.
		 *
		 * @param nNote determines which note will be triggered and is defined
		 *   between [36,127] inspired by the General MIDI standard.
		 * @param fVelocity how "hard" the note was triggered.
		 * @param bNoteOff whether note should trigger or stop sound.
		 * @param pMappedInstrument if provided, will hold the names of all
		 *   instruments the note was mapped to.
		 *
		 * @return bool true on success */
		static bool handleNote( int nNote, float fVelocity, bool bNoteOff = false,
								QStringList* pMappedInstruments = nullptr );

	/**
	 * Loads the drumkit specified in @a sDrumkitPath.
	 *
	 * Methods from within Hydrogen should _never_ call this function
	 * directly but, instead, use
	 * #SoundLibrarydatabase::getDrumkit(). It is only exposed
	 * publicly to be used within the unit tests.
	 *
	 * \param sDrumkitPath Can be either an absolute path to a folder
	 *   containing a drumkit file (drumkit.xml), an absolute path to a
	 *   drumkit file itself, or an absolute file to a compressed
	 *   drumkit (.h2drumkit).
	 * \param bIsCompressed Stores whether the drumkit was provided as
	 *   a compressed .h2drumkit file
	 * \param sDrumkitDir Stores the folder containing the drumkit
	 *   file. If a compressed drumkit was provided, this will point to
	 *   a temporary folder.
	 * \param sTemporaryFolder Root path of a temporary folder
	 *   containing the extracted drumkit in case @a sDrumkitPath
	 *   pointed to a compressed .h2drumkit file.
	 * \param pLegacyFormatEncountered will be set to `true` is any of the
	 *   XML elements requires legacy format support and left untouched
	 *   otherwise.
	 */
	static std::shared_ptr<Drumkit> retrieveDrumkit( const QString& sDrumkitPath,
													 bool* bIsCompressed,
													 QString* sDrumkitDir,
													 QString* sTemporaryFolder,
													 bool* pLegacyFormatEncountered );

	/**
	 * Set's song-level tempo of the #AudioEngine and stores the value
	 * in the current #Song.
	 */
	static bool setBpm( float fBpm );

		/**
		 * Opens the #H2Core::Playlist specified in @a sPath.
		 *
		 * This will be done immediately and without saving
		 * the current #H2Core::Playlist. All unsaved changes will be lost!
		 *
		 * \param sPath Absolute path to the .h2playlist file to be
		 *    opened.
		 * \param sRecoverPath If set to a value other than "",
		 *    the corresponding path will be used to load the playlist and
		 *    the latter is assigned @a sPath as Playlist::m_sFilename
		 *    afterwards. Using this mechanism the GUI can use an
		 *    autosave backup file to load a playlist without the core
		 *    having to do some string magic to retrieve the original name.
		 * \return nullptr on failure
		 */
		static std::shared_ptr<Playlist> loadPlaylist( const QString& sPath,
													   const QString& sRecoverPath = "" );
		/** Replaces the current #Playlist with @a Playlist. */
		static bool setPlaylist( std::shared_ptr<Playlist> pPlaylist );
		/** Saves changes of the current #Playlist to disk. */
		static bool savePlaylist();
		/** Saves the current #Playlist to @a sPath.*/
		static bool savePlaylistAs( const QString& sPath );
		/** Adds a new song/ entry to the current playlist.
		 *
		 * If @a nIndex is set to a value of -1, @a pEntry will be appended at
		 * the end of the playlist. */
		static bool addToPlaylist( std::shared_ptr<PlaylistEntry> pEntry,
							int nIndex = -1 );
		/** Removes a song from the current playlist.
		 *
		 * If @a nIndex is set to a value of -1, the first occurrance of @a
		 * pEntry will be deleted. */
		static bool removeFromPlaylist( std::shared_ptr<PlaylistEntry> pEntry,
								 int nIndex = -1 );
		/** Does not load the corresponding song! Only marks it active in the
		 * playlist.
		 *
		 * Song loading was split off to allow the GUI to show error dialogs in
		 * case something went wrong. */
		static bool activatePlaylistSong( int nSongNumber );

		/** Sends NoteOff MIDI messages for all instruments of the current
		 * drumkit. */
		static bool sendAllNoteOffMessages();

		/** Enable or disable tempo control using MIDI clock. */
		static bool setMidiClockInputHandling( bool bHandle );

		/** Enable or disable sending MIDI clock messages. */
		static bool setMidiClockOutputSend( bool bHandle );

private:
	static bool sendMasterVolumeFeedback();
	static bool sendStripVolumeFeedback( int nStrip );
	static bool sendMetronomeIsActiveFeedback();
	static bool sendMasterIsMutedFeedback();
	static bool sendStripIsMutedFeedback( int nStrip );
	static bool sendStripIsSoloedFeedback( int nStrip );
	static bool sendStripPanFeedback( int nStrip );
	static bool sendStripPanSymFeedback( int nStrip );
	
	static bool handleOutgoingControlChanges( const std::vector<int>& params, int nValue);
	static std::shared_ptr<Instrument> getStrip( int nStrip );
	
	// -----------------------------------------------------------
	// Actions required for session management.
		


	/**
	 * Add @a sFilename to the list of recent songs in
	 * Preferences::m_recentFiles.
	 *
	 * The function will also take care of removing any duplicates in
	 * the list in case @a sFilename is already present.
	 *
	 * \param sFilename New song to be added on top of the list.
	 */
	static void insertRecentFile( const QString& sFilename );
};

}
#endif
