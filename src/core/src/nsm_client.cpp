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

#include "hydrogen/helpers/filesystem.h"
#include "hydrogen/Preferences.h"
#include "hydrogen/event_queue.h"
#include "hydrogen/hydrogen.h"
#include "hydrogen/basics/song.h"
#include "hydrogen/audio_engine.h"

#include <pthread.h>
#include <unistd.h>

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

#include "hydrogen/nsm_client.h"


NsmClient * NsmClient::__instance = nullptr;
const char* NsmClient::__class_name = "NsmClient";

/** Indicates whether the nsm_processEvent() function should continue
 * processing events.
 *
 * Set to true in NsmClient::shutdown().
 */
bool NsmShutdown = false;

/**
 * Callback function for the NSM server to tell Hydrogen to open a
 * H2Core::Song.
 *
 * This function has two separate purposes: 1. it is used to load the
 * initial H2Core::Song and to set up the audio driver when a session
 * is started via the NSM server. 2. It handles the switching between
 * sessions by loading a new H2Core::Song without the need to restart
 * the whole application.
 *
 * To fulfill the 1. purpose, it is important to know that the core
 * part of H2Core::Hydrogen is already initialized when this function
 * is called but the GUI isn't. But, in order to allow for a rewiring
 * of all per track JACK output ports, the
 * H2Core::JackAudioDriver::init() function _must_ register them
 * alongside the main left and right output ports in the very
 * initialization and not at a later stage. Therefore, the starting of
 * the audio driver is prohibited whenever the "NSM_URL" environmental
 * variable is set, H2Core::Hydrogen::setInitialSong() is used to
 * store the loaded H2Core::Song, and
 * H2Core::Hydrogen::restartDrivers() to start the audio driver and -
 * if JACK is chosen - to create all per track output ports right
 * away. In addition, is also calls
 * H2Core::Hydrogen::restartLadspaFX() and
 * H2Core::Sampler::reinitialize_playback_track() to set up the
 * missing core parts of Hydrogen.
 *
 * In the 2. case of switching between session the function will
 * construct an Action of type "OPEN_SONG" - or "NEW_SONG" if no file
 * exists with the provided file path - triggering
 * MidiActionManager::open_song() or
 * MidiActionManager::new_song(). Then it waits - up to 11 seconds -
 * until the H2Core::Song was asynchronously set by the GUI (as a
 * response to the action). This (regular) procedure is only done if a
 * GUI is present and fully loaded and thus
 * H2Core::Hydrogen::m_iActiveGUI is set to 1. Else, the methods as
 * for 1. will be called.
 *
 * \param name Unique name corresponding to the current session. The
 * suffix ".h2song" will be appended and \a name will be used as file
 * path to store the H2Core::Song associated with the session.
 * \param display_name Name the application will be presented with by
 * the NSM server. It is determined in
 * NsmClient::createInitialClient() and set to "Hydrogen".
 * \param client_id Unique prefix also present in \a name, "nJKUV". It
 * will be stored in H2Core::Preferences::m_sNsmClientId to provide it
 * as a suffix when creating a JACK client in 
 * H2Core::JackAudioDriver::init().
 * \param out_msg Unused argument. Kept for API compatibility.
 * \param userdata Unused argument. Kept for API compatibility.
 *
 *  \return 
 * - ERR_OK (0): indicating that everything worked fine.
 * - ERR_LAUNCH_FAILED (-4): If no \a client_id provided, the H2Core::Song
 * corresponding to the file path of a concatenation of \a name and
 * ".h2song" could not be loaded, or the Action could not be provided
 * to MidiActionManager::handleAction().
 * - ERR_NOT_NOW (-8): If the H2Core::Preferences instance was
 * not initialized.
 */
static int nsm_open_cb (const char *name,
						const char *display_name,
						const char *client_id,
						char **out_msg,
						void *userdata )
{
	MidiActionManager* pActionManager = MidiActionManager::get_instance();
	
	// Handle supplied Song name. Hydrogen sends a unique string, like
	// - if the display_name == Hydrogen - "Hydrogen.nJKUV". We will
	// just append the .h2song and use it as Song path.
	
	if ( !name ) {
		std::cout << std::endl <<
			"\033[1;30m[Hydrogen]\033[31m Error: No `name` supplied in NSM open callback!\033[0m" << std::endl;
		return ERR_LAUNCH_FAILED;
	}

	QString songPath = QString( "%1.h2song" ).arg( name );
	
	std::cout << std::endl << 
		"\033[1;30m[Hydrogen]\033[32m Loading song " << 
		songPath.toLocal8Bit().data() << ".\033[0m" << std::endl;
	
	QFileInfo songFileInfo = QFileInfo( songPath );

	// When restarting the JACK client (later in this function) the
	// client_id will be used as the name of the freshly created
	// instance.
	H2Core::Preferences *pPref = H2Core::Preferences::get_instance();
	if ( pPref ){
		if ( client_id ){
			// Setup JACK here, client_id gets the JACK client name
			pPref->setNsmClientId( QString( client_id ) );
		} else {
			std::cout << "\033[1;30m[Hydrogen]\033[31m Error: No `client_id` supplied in NSM open callback!\033[0m" << std::endl;
			return ERR_LAUNCH_FAILED;
		}
	} else {
		std::cout << "\033[1;30m[Hydrogen]\033[31m Error: Preferences instance is not ready yet!\033[0m" << std::endl;
		return ERR_NOT_NOW;
	}
	
	H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::get_instance();
	H2Core::Song *pSong = nullptr;
	
	// Create a new Song object (and store it for the GUI to load it
	// later on).
	if ( songFileInfo.exists() ) {

		pSong = H2Core::Song::load( songPath );
		if ( pSong == nullptr ) {
			std::cout << "\033[1;30m[Hydrogen]\033[31m Error: Unable to open Song.\033[0m" << std::endl;
			return ERR_LAUNCH_FAILED;
		}
		
	} else {
		
		// There is no file with this name present yet. Fall back
		// to an empty default Song.
		pSong = H2Core::Song::get_empty_song();
		if ( pSong == nullptr ) {
			std::cout << "\033[1;30m[Hydrogen]\033[31m Error: Unable to open new Song.\033[0m" << std::endl;
			return ERR_LAUNCH_FAILED;
		}
		pSong->set_filename( songPath );
	}

	// Usually, when starting Hydrogen with its Qt5 GUI activated, the
	// chosen Song will be set via the GUI. But since it is
	// constructed after the NSM client, using the corresponding OSC
	// message to open a Song won't work in this scenario (since this
	// would set the Song asynchronously using the EventQueue and it
	// is require during the construction of MainForm).
	//
	// Two different scenarios are considered in here:
	// 1. <= 0: There is no GUI or there will be a GUI but it is not
	//         created yet.
	// 2. > 0: There is a GUI present and it is fully loaded.
	//
	// Scenario 2. is active when starting a session and 3. when
	// switching between sessions.
	//
	// Loading the Song is a little bit tricky in both cases. In
	// 1. the much more slim setInitalSong() function is used since
	// setSong() requires the audio driver to be already present which
	// would keep external tools from rewiring the per track outputs
	// of the JACK client. In 2. the Song _must_ the loaded by the GUI
	// or Hydrogen will get out of sync and freeze. The Song will be
	// stored using setNextSong() and an event will be created to tell
	// the GUI to load the Song itself.
	if ( pHydrogen->getActiveGUI() <= 0 ) {
		
		// No GUI. Just load the requested Song and restart the audio
		// driver.
		pHydrogen->setInitialSong( pSong );
		pHydrogen->restartDrivers();
		pHydrogen->restartLadspaFX();
		H2Core::AudioEngine::get_instance()->get_sampler()->reinitialize_playback_track();
		
	} else {

		// The opening of the Song will be done asynchronously using
		// the MidiActionManager.
		pHydrogen->setNextSong( pSong );
		
		// Check whether a file corresponding to the provided path does
		// already exist.
		QString actionType;
		if ( songFileInfo.exists() ) {
			// Open the existing file.
			actionType = "OPEN_SONG";
		} else {
			// Create a new file and save it as using the provided path.
			actionType = "NEW_SONG";
		}

		Action currentAction( actionType );
		
		// Tell the action to load the desired file.
		currentAction.setParameter1( songPath );
		
		// Tell the action to restart the audio engine.
		currentAction.setParameter2( "1" );
		
		H2Core::Song *pCurrentSong = pHydrogen->getSong();
		
		bool ok = pActionManager->handleAction( &currentAction );

		if ( !ok ) {
			std::cout << "\033[1;30m[Hydrogen]\033[31m Error: Unable to handle opening action!\033[0m" << std::endl;
			return ERR_LAUNCH_FAILED;
		}
		
		// Wait until the Song was set (asynchronously by the GUI).
		int numberOfChecks = 10;
		int check = 0;
		while ( true ) {
			if ( pCurrentSong != pHydrogen->getSong() ) {
				break;
			}
			// Don't wait indefinitely.
			if ( check > numberOfChecks ) {
				break;
			}
			check++;
			sleep(1);
		}
	}
	
	std::cout << "\033[1;30m[Hydrogen]\033[32m Song loaded!" << std::endl;
	
	return ERR_OK;
}

/**
 * Callback function for the NSM server to tell Hydrogen to save the
 * current H2Core::Song.
 *
 * It will construct an Action of type "SAVE_SONG" triggering
 * MidiActionManager::save_song().
 *
 * \param out_msg Unused argument. Kept for API compatibility.
 * \param userdata Unused argument. Kept for API compatibility.
 *
 *  \return 0 - actually ERR_OK defined in the NSM API - indicating
 *  that everything worked fine.
 */
static int nsm_save_cb( char **out_msg, void *userdata )
{
	Action currentAction("SAVE_SONG");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(&currentAction);
	
	std::cout << "\033[1;30m[Hydrogen]\033[32m Song saved!\033[0m" << std::endl;

	return ERR_OK;
}

/**
 * Event handling function of the NSM client.
 *
 * The event handling can be deactivated by calling
 * NsmClient::shutdown() which is setting #NsmShutdown to true.
 *
 * \param data NSM client created in NsmClient::createInitialClient().
 */
void* nsm_processEvent(void* data)
{
	nsm_client_t* nsm = (nsm_client_t*) data;

	while(!NsmShutdown && nsm){
		nsm_check_wait( nsm, 1000);
	}

	return nullptr;
}

NsmClient::NsmClient()
	: Object( __class_name )
{
	m_NsmThread = 0;
	m_bUnderSessionManagement = false;
}

void NsmClient::create_instance()
{
	if( __instance == nullptr ) {
		__instance = new NsmClient;
	}
}

NsmClient::~NsmClient()
{
	__instance = nullptr;
}

void NsmClient::shutdown()
{
	NsmShutdown = true;
}

void NsmClient::createInitialClient()
{
	/*
	 * Make first contact with NSM server.
	 */

	nsm_client_t* nsm = nullptr;

	H2Core::Preferences *pPref = H2Core::Preferences::get_instance();
	QString H2ProcessName = pPref->getH2ProcessName();
	QByteArray byteArray = H2ProcessName.toLatin1();

	const char *nsm_url = getenv( "NSM_URL" );

	if ( nsm_url )
	{
		nsm = nsm_new();
		
		// Store the nsm client in a private member variable for later
		// access.
		m_nsm = nsm;

		if ( nsm )
		{
			nsm_set_open_callback( nsm, nsm_open_cb, (void*) nullptr );
			nsm_set_save_callback( nsm, nsm_save_cb, (void*) nullptr );

			if ( nsm_init( nsm, nsm_url ) == 0 )
			{
				// Technically Hydrogen will be under session
				// management after the nsm_send_announce and
				// nsm_check_wait function are called. But since the
				// nsm_open_cb() will be called by the NSM server
				// immediately after receiving the announce and some
				// of the functions called thereafter do check whether
				// H2 is under session management, the variable will
				// be set here.
				m_bUnderSessionManagement = true;
				
				nsm_send_announce( nsm, "Hydrogen", ":dirty:switch:", byteArray.data() );
						
				nsm_check_wait( nsm, 10000 );

				if(pthread_create(&m_NsmThread, nullptr, nsm_processEvent, nsm)) {
					___ERRORLOG("Error creating NSM thread\n	");
					m_bUnderSessionManagement = false;
					return;
				}	
				
				// Wait until first the Song and afterwards the audio
				// driver was set (asynchronously by the nsm_open_cb()
				// function).
				H2Core::Hydrogen* pHydrogen = H2Core::Hydrogen::get_instance();
				int numberOfChecks = 10;
				int check = 0;
				
				while ( true ) {
					if ( pHydrogen->getAudioOutput() != nullptr ) {
						break;
					}
					// Don't wait indefinitely.
					if ( check > numberOfChecks ) {
						break;
				   }
					check++;
					sleep(1);
				}			

			} else {
				___ERRORLOG("failed, freeing NSM client");
				nsm_free( nsm );
				nsm = nullptr;
			}
		}
	}
	else
	{
		___WARNINGLOG("No NSM URL available: no NSM management\n");
	}
}

void NsmClient::sendDirtyState( const bool isDirty ) {
	
	if ( isDirty ) {
		nsm_send_is_dirty( m_nsm );
	} else {
		nsm_send_is_clean( m_nsm );
	}
}

#endif /* H2CORE_HAVE_OSC */

