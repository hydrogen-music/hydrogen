/*
 * Hydrogen
 * Copyright(c) 2017 by Sebastian Moors
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef CORE_ACTION_CONTROLLER_H
#define CORE_ACTION_CONTROLLER_H

#include <core/Object.h>
#include <core/Basics/Song.h>

namespace H2Core
{

class CoreActionController : public H2Core::Object {
	H2_OBJECT
	
	public:
		CoreActionController();
		~CoreActionController();
	
		void setMasterVolume( float masterVolumeValue );
		/**
		 * \param nStrip Instrument which to set the volume for.
		 * \param fVolumeValue New volume.
		 * \param bSelectedStrip Whether the corresponding instrument
		 * should be selected.
		 */
		void setStripVolume( int nStrip, float fVolumeValue, bool bSelectStrip );
		/**
		 * \param nStrip Instrument which to set the volume for.
		 * \param fPanValue New pan.
		 * \param bSelectedStrip Whether the corresponding instrument
		 * should be selected.
		 */
		void setStripPan( int nStrip, float fPanValue, bool bSelectStrip );
		void setMetronomeIsActive( bool isActive );
		void setMasterIsMuted( bool isMuted );
		
		void setStripIsMuted( int nStrip, bool isMuted );
		void toggleStripIsMuted( int nStrip );
		
		void setStripIsSoloed( int nStrip, bool isSoloed );
		void toggleStripIsSoloed( int nStrip );
		
		void initExternalControlInterfaces();
		void handleOutgoingControlChange( int param, int value);
	
		// -----------------------------------------------------------
		// Actions required for session management.
		
		/**
		 * Create an empty #Song, which will be stored in @a songPath.
		 *
		 * This will be done immediately and without saving
		 * the current #Song. All unsaved changes will be lost! In
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
		 * Opens the #Song specified in @a songPath.
		 *
		 * This will be done immediately and without saving
		 * the current #Song. All unsaved changes will be lost!
		 *
		 * The intended use of this function for session
		 * management. Therefore, the function will *not* store the
		 * provided @a songPath in Preferences::m_lastSongFilename and
		 * Hydrogen won't resume with the corresponding song on
		 * restarting.
		 *
		 * \param songPath Absolute path to the .h2song file to be
		 *    opened.
		 * \param bRestartDriver Whether or not to restart the audio
		 *    driver after successfully loading the new song.
		 * \return true on success
		 */
		bool openSong( const QString& songPath, bool bRestartDriver );
		/**
		 * Opens the #Song specified in @a songPath.
		 *
		 * This will be done immediately and without saving
		 * the current #Song. All unsaved changes will be lost!
		 *
		 * The intended use of this function for session
		 * management. Therefore, the function will *not* store the
		 * provided @pSong in Preferences::m_lastSongFilename and
		 * Hydrogen won't resume with the corresponding song on
		 * restarting.
		 *
		 * \param pSong New Song.
		 * \param bRestartDriver Whether or not to restart the audio
		 *    driver after successfully loading the new song.
		 * \return true on success
		 */
		bool openSong( Song* pSong, bool bRestartDriver );
		/**
		 * Saves the current #Song.
		 *
		 * \return true on success
		 */
		bool saveSong();
		/**
		 * Saves the current #Song to the path provided in @a songPath.
		 *
		 * The intended use of this function for session
		 * management. Therefore, the function will *not* store the
		 * provided @a songPath in Preferences::m_lastSongFilename and
		 * Hydrogen won't resume with the corresponding song on
		 * restarting.
		 *
		 * \param songPath Absolute path to the file to store the
		 *   current #Song in.
		 * \return true on success
		 */
		bool saveSongAs( const QString& songPath );
		/**
		 * Saves the current state of the #Preferences.
		 *
		 * \return true on success
		 */
		bool savePreferences();
		/**
		 * Triggers the shutdown of Hydrogen.
		 *
		 * This will be done immediately and without saving the
		 * current #Song. All unsaved changes will be lost!
		 *
		 * The shutdown will be triggered in both the CLI and the GUI
		 * via the #H2Core::EVENT_QUIT event.
		 *
		 * \return true on success
		 */
		bool quit();

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
		 * @param bTriggerEvent Setting this variable to true is
		 * intended for its use as a batch function from within
		 * Hydrogen's core, which will inform the GUI via an Event
		 * about the change of mode. When used from the GUI itself,
		 * this parameter has to be set to false.
		 *
		 * @return bool true on success
		 */
		bool activateSongMode( bool bActivate, bool bTriggerEvent );
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
		/** Relocates transport to the beginning of a particular
		 * Pattern.
		 * 
		 * @param nPatternGroup Position of the Song provided as the
		 * index of a particular pattern group (starting at zero).
		 *
		 * @return bool true on success
		 */
		bool relocate( int nPatternGroup );
		
		// -----------------------------------------------------------
		// Helper functions
		
		/**
		 * Checks the path of the .h2song provided via OSC.
		 *
		 * It will be checked whether @a songPath
		 * - is absolute
		 * - has the '.h2song' suffix
		 * - is writable (if it exists)
		 *
		 * \param songPath Absolute path to an .h2song file.
		 * \return true - if valid.
		 */
		bool isSongPathValid( const QString& songPath );
		
	private:
		
		/**
		 * Sets a #Song to be used by Hydrogen.
		 *
		 * This will be done immediately and without saving the
		 * current #Song. All unsaved changes will be lost!
		 *
		 * The intended use of this function for session
		 * management.
		 *
		 * \param pSong Pointer to the #Song to set.
		 * \param bRestartDriver Whether or not to restart the audio
		 *    driver after successfully loading the new song.
		 * \return true on success
		 */
		bool setSong( Song* pSong, bool bRestartDriver );
		
		const int m_nDefaultMidiFeedbackChannel;
};

}
#endif
