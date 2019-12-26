/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef NSM_CLIENT_H
#define NSM_CLIENT_H

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

#include <hydrogen/object.h>
#include <cassert>

#include "hydrogen/nsm.h"


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
* Hydrogen is compliant with the standard, send dirty flag to the NSM
* server to indicate unsaved changes, and is able to switch between
* different sessions without restarting the entire
* application. However, Hydrogen does use several files to store all
* options the user is able to customize and not all of them are stored
* inside the folder provided by the NSM server. While the song file
* will be kept there, both the drumkit and preferences are stored
* elsewhere. Altering either of them can very well affect the state of
* the individual session, e.g. by disabling a prior set the per track
* output option of the JACK driver outside of the session and
* restarting it. So, be very careful. 
*
* @author Sebastian Moors
*
*/

class NsmClient : public H2Core::Object
{
	H2_OBJECT
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
		 * H2Core::Song::set_is_modified().
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
		 * Sets #NsmShutdown to true.*/
		void shutdown();

		/**
		 * To determine whether Hydrogen is under Non session management,
		 * it is not sufficient to check whether the NSM_URL environmental
		 * variable is set but also whether the NSM server did respond
		 * to the announce message appropriately. Therefore,
		 * createInitialClient() has to be called first.
		 */
		bool m_bUnderSessionManagement;

		/** Folder all the content of the current session will be
		 * stored in.
		 *
		 * Set at the beginning of each session in
		 * nsm_open_cb().
		 */
		QString m_sSessionFolderPath;

	private:
		/**Constructor*/
		NsmClient();
		
		/**
		 * Stores the current instance of the NSM client.
		 *
		 * Used in sendDirtyState() to establish a communication to
		 * the NSM server.
		 */
		nsm_client_t* m_nsm;
};

#endif /* H2CORE_HAVE_OSC */

#endif // NSM_CLIENT_H
