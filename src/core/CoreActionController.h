/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Object.h>
#include <core/Basics/Song.h>

namespace H2Core
{

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
		bool setMetronomeIsActive( bool isActive );
		bool setMasterIsMuted( bool isMuted );
		
		bool setStripIsMuted( int nStrip, bool isMuted );
		bool toggleStripIsMuted( int nStrip );
		
		bool setStripIsSoloed( int nStrip, bool isSoloed );
		bool toggleStripIsSoloed( int nStrip );
		
		bool initExternalControlInterfaces();
		bool handleOutgoingControlChange( int param, int value);
	
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
		 * \return true on success
		 */
		bool openSong( std::shared_ptr<Song> pSong );
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
		 * Note that this function will fail in the presence of the
		 * Jack audio driver and an external timebase master (see Hydrogen::getJackTimebaseState()).
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
		 * (De)activates the usage of Jack timebase master.
		 *
		 * Note that this function will fail if Jack is not used as
		 * audio driver.
		 *
		 * @param bActivate If true - activate or if false -
		 * deactivate.
		 *
		 * @return bool true on success
		 */
		bool activateJackTimebaseMaster( bool bActivate );

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
		 * @param bTriggerEvent Setting this variable to true is
		 * intended for its use as a batch function from within
		 * Hydrogen's core, which will inform the GUI via an Event
		 * about the change of mode. When used from the GUI itself,
		 * this parameter has to be set to false.
		 *
		 * @return bool true on success
		 */
		bool activateLoopMode( bool bActivate, bool bTriggerEvent );
	/** Wrapper around loadDrumkit() that allows loading drumkits by
		name. */
	bool loadDrumkit( const QString& sDrumkitName, bool bConditional = true );
	/**
	 * Loads Drumkit @a pDrumkit and stores it unto the current song.
	 *
	 * The loading is _not_ performed lazily as it also can be used to
	 * reset the parameters of the current drumkit to its default
	 * values.
	 *
	 * \param pDrumkit Full-fledged H2Core::Drumkit to load.
	 * \param bConditional Whether to remove all redundant
	 * H2Core::Instrument regardless of their content.
	 */
	bool loadDrumkit( Drumkit* pDrumkit, bool bConditional = true );
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
	    /** Fills or clears a specific grid cell in the SongEditor.
		 *
		 * @param nColumn column of the pattern.
		 * @param nRow row of the pattern.
		 *
		 * @return bool true on success
		 */
    	bool toggleGridCell( int nColumn, int nRow );
	private:
		
		/**
		 * Sets a #H2Core::Song to be used by Hydrogen.
		 *
		 * This will be done immediately and without saving the
		 * current #H2Core::Song. All unsaved changes will be lost!
		 *
		 * \param pSong Pointer to the #H2Core::Song to set.
		 * \return true on success
		 */
		bool setSong( std::shared_ptr<Song> pSong );

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
