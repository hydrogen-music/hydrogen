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

#ifndef NSM_CLIENT_H
#define NSM_CLIENT_H

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

#include <core/Object.h>
#include <cassert>

#include "core/Nsm.h"


/**
* @class NsmClient
*
* @brief Non session manager client implementation
*
* Non session management (NSM) is the name for both a standard/API
* allowing for a reproducible, multi-application session handling in
* Linux systems as well as for the actual application - the
* non-session-manager - implementing it.
*
* Hydrogen is compliant with the standard, sends dirty flag to the NSM
* server to indicate unsaved changes, and is able to switch between
* different sessions without restarting the entire
* application. However, Hydrogen does use several files to store all
* options the user is able to customize and not all of them are stored
* inside the folder provided by the NSM server. Both the song and a
* custom copy of the preferences will be stored in the session
* folder. But the drumkit used in the song will only be linked into it.
*
* @author Sebastian Moors
*
*/

class NsmClient : public H2Core::Object<NsmClient>
{
	H2_OBJECT(NsmClient)
	public:
		/**
		 * Object holding the current NsmClient singleton. It
		 * is initialized with nullptr, set with
		 * create_instance(), and accessed with
		 * get_instance().
		 */
		static NsmClient* __instance;
		/** Destructor*/
		~NsmClient();
		/** Thread the NSM client will run in.*/
		pthread_t m_NsmThread;
		/**
		 * If #__instance equals nullptr, a new NsmClient singleton
		 * will be created and stored in it.
		 *
		 * It is called in
		 * H2Core::Hydrogen::create_instance().
		 */
		static void create_instance();
		/**
		 * \return a pointer to the current NsmClient
		 * singleton stored in #__instance.
		 */
		static NsmClient* get_instance() { assert(__instance); return __instance; }

		/**
		 * Informs the NSM server whether the current H2Core::Song is
		 * modified or not.
		 *
		 * This function is triggered within
		 * H2Core::Song::setIsModified().
		 *
		 * \param isDirty true, if the current H2Core::Song was
		 * modified, and false if it wasn't
		 */
		void sendDirtyState( const bool isDirty );
		/**
		 * Actual setup, initialization, and registration of the NSM
		 * client.
		 *
		 * It create a new NSM client, sets the callback functions
		 * nsm_open_cb() and nsm_save_cb(), and registers the newly
		 * created client with the NSM server. It also indicates that
		 * Hydrogen does support the two NSM options "dirty" and
		 * "switch", allowing the server to notice whenever there are
		 * unsaved changes and to switch between Songs without
		 * restarting the whole application.
		 *
		 * This function will performs action if a NSM server is
		 * already running. This will be indicated by a set
		 * environmental variable called "NSM_URL". However, this is
		 * condition is not sufficient and only after receiving a
		 * certain response - handled by the NSM API inside the
		 * nsm_free() function - to the announce message sent by
		 * Hydrogen, the client can truly be considered under session
		 * management. This particular state will be indicated by
		 * setting #m_bUnderSessionManagement to true.
		 */
		void createInitialClient();

		/** Causes the NSM client to not process events anymore.
		 *
		 * Sets #bNsmShutdown to true.*/
		void shutdown();
	/**
	 * Responsible for linking and loading of the drumkit samples.
	 *
	 * Upon first invocation of this function in a new project, a
	 * symbolic link to the folder containing the samples of the
	 * current drumkit will be created in @a sName `drumkit`. In all
	 * following runs of the session the linked samples will be used
	 * over the default ones.
	 *
	 * If the session were archived, the symbolic link would had
	 * been replaced by a folder containing the samples. In such an
	 * occasion the samples located in the folder will be loaded. This
	 * ensure portability of Hydrogen within a session regardless of
	 * the local drumkits present in the user's home.
	 *
	 * \param sName Absolute path to the session folder.
	 * \param bCheckLinkage Whether or not the linked drumkit should
	 * be verified to correspond to @a sName. If set to none, the
	 * drumkit will always be relinked.
	 */
	static void linkDrumkit( const QString& sName, bool bCheckLinkage );
	/** Custom function to print a colored error message.
	 *
	 * Since the OpenCallback() and SaveCallback() functions will be
	 * invoked by the NSM server and not by Hydrogen itself, we can
	 * not use our usual log macros in there.
	 *
	 * \param msg String to print to std::cerr.
	 */
	static void printError( const QString& msg );
	/** Custom function to print a colored message.
	 *
	 * Since the OpenCallback() and SaveCallback() functions will be
	 * invoked by the NSM server and not by Hydrogen itself, we can
	 * not use our usual log macros in there.
	 *
	 * \param msg String to print to std::cout.
	 */
	static void printMessage( const QString& msg );

	/** \return m_bUnderSessionManagement*/
	bool getUnderSessionManagement() const;

		/** Folder all the content of the current session will be
		 * stored in.
		 *
		 * Set at the beginning of each session in
		 * NsmClient::OpenCallback().
		 */
		QString m_sSessionFolderPath;

	private:
		/**Private constructor to allow construction only via
		   create_instance().*/
		NsmClient();
		
		/**
		 * Stores the current instance of the NSM client.
		 */
		nsm_client_t* m_pNsm;
	
		/**
		 * To determine whether Hydrogen is under NON session management,
		 * it is not sufficient to check whether the NSM_URL environmental
		 * variable is set but also whether the NSM server did respond
		 * to the announce message appropriately. Therefore,
		 * createInitialClient() has to be called first.
		 */
		bool m_bUnderSessionManagement;
	
	/**
	 * Callback function for the NSM server to tell Hydrogen to open a
	 * H2Core::Song.
	 *
	 * It also handles the switching between sessions by loading the
	 * H2Core::Song, the H2Core::Preferences, and the H2Core::Drumkit
	 * of the new session without the need to restart the whole
	 * application.
	 *
	 * It is important to know that the
	 * core part of H2Core::Hydrogen is already initialized when this
	 * function is called, but the GUI isn't.
	 *
	 * All files and symbolic links will be stored in a folder created
	 * by this function and named according to @a name.
	 *
	 * \param name Unique name corresponding to the current session. A
	 * folder using this particular \a name will be created, which will
	 * contain the H2Core::Song - using \a name appended by ".h2song" as
	 * file name -, the local H2Core::Preferences, and a symbolic link to
	 * the H2Core::Drumkit in use.
	 * \param displayName Name the application will be presented with by
	 * the NSM server. It is determined in
	 * NsmClient::createInitialClient() and set to "Hydrogen".
	 * \param clientID Unique prefix also present in \a name, "nJKUV". It
	 * will be stored in H2Core::Preferences::m_sNsmClientId to provide it
	 * as a suffix when creating a JACK client in 
	 * H2Core::JackAudioDriver::init().
	 * \param outMsg Unused argument. Kept for API compatibility.
	 * \param userData Unused argument. Kept for API compatibility.
	 *
	 *  \return 
	 * - ERR_OK (0): indicating that everything worked fine.
	 * - ERR_LAUNCH_FAILED (-4): If no \a clientID provided, the H2Core::Song
	 * corresponding to the file path of a concatenation of \a name and
	 * ".h2song" could not be loaded, or the Action could not be provided
	 * to MidiActionManager::handleAction().
	 * - ERR_NOT_NOW (-8): If the H2Core::Preferences instance was
	 * not initialized.
	 *
	 * \see copyPreferences()
	 * \see linkDrumkit()
	 */
	static int OpenCallback( const char* name, const char* displayName,
							 const char* clientID, char** outMsg,
							 void* userData );
							   
	/**
	 * Callback function for the NSM server to tell Hydrogen to save the
	 * current session.
	 *
	 * \param outMsg Unused argument. Kept for API compatibility.
	 * \param userData Unused argument. Kept for API compatibility.
	 *
	 *  \return 0 - actually ERR_OK defined in the NSM API - indicating
	 *  that everything worked fine.
	 */
	static int SaveCallback( char** outMsg, void* userData );
	
	/**
	 * Event handling function of the NSM client.
	 *
	 * The event handling can be deactivated by calling
	 * NsmClient::shutdown() which is setting #bNsmShutdown to true.
	 *
	 * \param data NSM client created in NsmClient::createInitialClient().
	 */
	static void* ProcessEvent( void* data );
	
	/**
	 * Part of OpenCallback() responsible for copying and loading the
	 * preferences.
	 *
	 * Then it uses H2Core::Preferences::loadPreferences() in
	 * combination with
	 * H2Core::Filesystem::setPreferencesOverwritePath() to load the
	 * configurations specific to the session. If none hydrogen.conf
	 * file (see #USR_CONFIG) is present in the session folder, the
	 * one of the user is used to create one instead. Next, a
	 * H2Core::EVENT_UPDATE_PREFERENCES event is created to trigger to
	 * ensure the GUI reflects the changes in configuration.
	 *
	 * \param name Absolute path to the session folder.
	 */
	static void copyPreferences( const char* name );
	
	/** Indicates whether the NsmClient::NsmProcessEvent() function
	 * should continue processing events.
	 *
	 * Set to true in NsmClient::shutdown().
	 */
	static bool bNsmShutdown;
};

inline bool NsmClient::getUnderSessionManagement() const {
	return m_bUnderSessionManagement;
}

#endif /* H2CORE_HAVE_OSC */

#endif // NSM_CLIENT_H
