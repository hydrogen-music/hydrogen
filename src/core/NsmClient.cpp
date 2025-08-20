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

#include <core/CoreActionController.h>
#include "core/Helpers/Filesystem.h"
#include "core/Preferences/Preferences.h"
#include "core/Hydrogen.h"
#include "core/Basics/Drumkit.h"
#include "core/Basics/Song.h"
#include "core/NsmClient.h"
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <pthread.h>
#include <unistd.h>

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

NsmClient * NsmClient::__instance = nullptr;
bool NsmClient::bNsmShutdown = false;


NsmClient::NsmClient()
	: m_pNsm( nullptr ),
	  m_bUnderSessionManagement( false ),
	  m_NsmThread( 0 ),
	  m_sSessionFolderPath( "" ),
	  m_bIsNewSession( false )
{
}

NsmClient::~NsmClient()
{
	__instance = nullptr;
}

void NsmClient::create_instance()
{
	if( __instance == nullptr ) {
		__instance = new NsmClient;
	}
}

int NsmClient::OpenCallback( const char *name,
							 const char *displayName,
							 const char *clientID,
							 char **outMsg,
							 void *userData ) {

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pPref = H2Core::Preferences::get_instance();
	auto pNsmClient = NsmClient::get_instance();

	if ( !name ) {
		NsmClient::printError( "No `name` supplied in NSM open callback!" );
		return ERR_LAUNCH_FAILED;
	}

	// Cause there is no newline in the output of nsmd shown
	// beforehand.
	std::cout << std::endl;
	
	// NSM sends a unique string, like - if the displayName ==
	// Hydrogen - "Hydrogen.nJKUV". In order to make the whole
	// Hydrogen session reproducible, a folder will be created, which
	// will contain the song file, a copy of the current preferences,
	// and a symbolic link to the drumkit.
	QDir sessionFolder( name );
	if ( !sessionFolder.exists() ) {
		if ( !sessionFolder.mkpath( name ) ) {
			NsmClient::printError( "Folder could not created." );
			return ERR_LAUNCH_FAILED;
		}
	}

	NsmClient::copyPreferences( name );

	pNsmClient->setSessionFolderPath( name );
	
	const QFileInfo sessionPath( name );
	const QString sSongPath = QString( "%1/%2%3" )
		.arg( name )
		.arg( sessionPath.fileName() )
		.arg( H2Core::Filesystem::songs_ext );
	
	const QFileInfo songFileInfo = QFileInfo( sSongPath );

	// When restarting the JACK client (during song loading) the
	// clientID will be used as the name of the freshly created
	// instance.
	if ( clientID ){
		// Required for JACK setup, client_id gets the JACK client name
		pNsmClient->setClientId( QString( clientID ) );
	}
	else {
		NsmClient::printError( "No `clientID` supplied in NSM open callback!" );
		return ERR_LAUNCH_FAILED;
	}

	auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
	pSoundLibraryDatabase->registerDrumkitFolder( sessionFolder.absolutePath() );
	pSoundLibraryDatabase->updateDrumkits();

	bool bEmptySongOpened = false;
	std::shared_ptr<H2Core::Song> pSong = nullptr;
	if ( songFileInfo.exists() ) {
		pSong = H2Core::Song::load( sSongPath );
		if ( pSong == nullptr ) {
			NsmClient::printError( QString( "Unable to open existing Song [%1]." )
								   .arg( sSongPath ) );
			return ERR_LAUNCH_FAILED;
		}
	}
	else {

		pSong = H2Core::Song::getEmptySong();
		if ( pSong == nullptr ) {
			NsmClient::printError( "Unable to open new Song." );
			return ERR_LAUNCH_FAILED;
		}
		pSong->setFilename( sSongPath );
		bEmptySongOpened = true;

		// Mark empty song modified in order to emphasis that an
		// initial song save is required to generate the song file and
		// link the associated drumkit in the session folder.
		pSong->setIsModified( true );
		pNsmClient->setIsNewSession( true );
	}

	if ( ! H2Core::CoreActionController::setSong( pSong ) ) {
			NsmClient::printError( "Unable to handle opening action!" );
			return ERR_LAUNCH_FAILED;
	}
	
	NsmClient::printMessage( "Song loaded!" );

	return ERR_OK;
}

void NsmClient::copyPreferences( const char* name ) {
	
	auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();

	QFile preferences( H2Core::Filesystem::usr_config_path() );
	if ( !preferences.exists() ) {
		preferences.setFileName( H2Core::Filesystem::sys_config_path() );
	}
	
	const QString sNewPreferencesPath = QString( "%1/%2" )
		.arg( name )
		.arg( QFileInfo( H2Core::Filesystem::usr_config_path() )
			  .fileName() );
	
	// Store the path in a session variable of the Preferences
	// singleton, which allows overwriting the default path used
	// throughout the application.
	H2Core::Filesystem::setPreferencesOverwritePath( sNewPreferencesPath );

	const QFileInfo newPreferencesFileInfo( sNewPreferencesPath );
	if ( newPreferencesFileInfo.exists() ){
		// If there's already a preference file present from a
		// previous session, we load it instead of overwriting it.
		auto pPref =
			H2Core::CoreActionController::loadPreferences( sNewPreferencesPath );
		if ( pPref != nullptr ) {
			H2Core::CoreActionController::setPreferences( pPref );

			NsmClient::printMessage( "Preferences loaded!" );
		}
		else {
			ERRORLOG( QString( "Loading the previous session's config from [%1] failed!" )
					  .arg( sNewPreferencesPath ) );
		}
	}
	else {
		if ( ! preferences.copy( sNewPreferencesPath ) ) {
			NsmClient::printError( QString( "Unable to copy preferences to [%1]" )
								   .arg( sNewPreferencesPath ) );
		} else {
			NsmClient::printMessage( QString( "Preferences copied to [%1]" )
									 .arg( sNewPreferencesPath ) );
			// The copied preferences file is already loaded.
		}
	}
}

void NsmClient::printError( const QString& msg ) {
	const bool bLogColor = __logger->getLogColors();

	if ( bLogColor ) {
		std::cerr << "[\033[30mHydrogen\033[0m]\033[31m ";
	} else {
		std::cerr << "[Hydrogen] ";
	}

	std::cerr << "Error: " << msg.toLocal8Bit().data();

	if ( bLogColor ) {
		std::cerr << "\033[0m";
	}

	std::cerr << std::endl;
}

void NsmClient::printMessage( const QString& msg ) {
	const bool bLogColor = __logger->getLogColors();

	if ( bLogColor ) {
		std::cerr << "[\033[30mHydrogen\033[0m]\033[32m ";
	} else {
		std::cerr << "[Hydrogen] ";
	}

	std::cerr << msg.toLocal8Bit().data();

	if ( bLogColor ) {
		std::cerr << "\033[0m";
	}

	std::cerr << std::endl;
}

int NsmClient::SaveCallback( char** outMsg, void* userData ) {
	if ( ! H2Core::CoreActionController::saveSong() ) {
		NsmClient::printError( "Unable to save Song!" );
		return ERR_GENERAL;
	}
	if ( ! H2Core::CoreActionController::savePreferences() ) {
		NsmClient::printError( "Unable to save Preferences!" );
		return ERR_GENERAL;
	}

	NsmClient::printMessage( "Song and Preferences saved!" );

	return ERR_OK;
}

void* NsmClient::ProcessEvent(void* data) {
	nsm_client_t* pNsm = (nsm_client_t*) data;

	while( !NsmClient::bNsmShutdown && pNsm ){
		nsm_check_wait( pNsm, 1000 );
	}

	return nullptr;
}

void NsmClient::shutdown()
{
	NsmClient::bNsmShutdown = true;
}

void NsmClient::createInitialClient( const QString& sProcessName ) {
	/*
	 * Make first contact with NSM server.
	 */

	nsm_client_t* pNsm = nullptr;

	const auto pPref = H2Core::Preferences::get_instance();
	QByteArray byteArray = sProcessName.toLatin1();

	const char *nsm_url = getenv( "NSM_URL" );

	if ( nsm_url )
	{
		pNsm = nsm_new();
		
		// Store the nsm client in a private member variable for later
		// access.
		m_pNsm = pNsm;

		if ( pNsm )
		{
			nsm_set_open_callback( pNsm, NsmClient::OpenCallback, (void*) nullptr );
			nsm_set_save_callback( pNsm, NsmClient::SaveCallback, (void*) nullptr );

			if ( nsm_init( pNsm, nsm_url ) == 0 )
			{
				// Technically Hydrogen will be under session
				// management after the nsm_send_announce and
				// nsm_check_wait function are called. But since the
				// NsmClient::OpenCallback() will be called by the NSM server
				// immediately after receiving the announce and some
				// of the functions called thereafter do check whether
				// H2 is under session management, the variable will
				// be set here.
				m_bUnderSessionManagement = true;

				nsm_send_announce( pNsm, "Hydrogen", ":dirty:switch:", byteArray.data() );

				if ( pthread_create( &m_NsmThread, nullptr, NsmClient::ProcessEvent, pNsm ) ) {
					___ERRORLOG("Error creating NSM thread\n	");
					m_bUnderSessionManagement = false;
					return;
				}	
				
				// Wait until first the Song and afterwards the audio
				// driver was set (asynchronously by the
				// NsmClient::OpenCallback() function).
				const H2Core::Hydrogen* pHydrogen = H2Core::Hydrogen::get_instance();
				const int nNumberOfChecks = 10;
				int nCheck = 0;
				
				while ( true ) {
					if ( pHydrogen->getSong() != nullptr ) {
						break;
					}
					// Don't wait indefinitely.
					if ( nCheck > nNumberOfChecks ) {
						break;
				   }
					nCheck++;
					sleep( 1 );
				}			

			} else {
				___ERRORLOG("failed, freeing NSM client");
				nsm_free( pNsm );
				pNsm = nullptr;
				m_pNsm = nullptr;
			}
		}
	}
	else
	{
		___WARNINGLOG("No NSM URL available: no NSM management\n");
	}
}

void NsmClient::sendDirtyState( const bool bIsDirty ) {

	if ( m_pNsm != nullptr ) {
		if ( bIsDirty ) {
			nsm_send_is_dirty( m_pNsm );
		} else {
			nsm_send_is_clean( m_pNsm );
		}
	}
}

#endif /* H2CORE_HAVE_OSC */

