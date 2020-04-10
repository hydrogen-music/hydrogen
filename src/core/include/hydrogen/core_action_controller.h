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

#include <hydrogen/object.h>

namespace H2Core
{

class CoreActionController : public H2Core::Object {
	H2_OBJECT
	
	public:
		CoreActionController();
		~CoreActionController();
	
		void setMasterVolume( float masterVolumeValue );
		void setStripVolume( int nStrip, float masterVolumeValue );
		void setStripPan( int nStrip, float panValue );
		void setMetronomeIsActive( bool isActive );
		void setMasterIsMuted( bool isMuted );
		void setStripIsMuted( int nStrip, bool isMuted );
		void setStripIsSoloed( int nStrip, bool isSoloed );
		
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
		 * \return true on success
		 */
		bool openSong( const QString& songPath );
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
		
		const int m_nDefaultMidiFeedbackChannel;
};

}
#endif
