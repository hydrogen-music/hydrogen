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

#include "core/LocalFileMng.h"
#include "core/Helpers/Filesystem.h"
#include "core/Preferences.h"
#include "core/EventQueue.h"
#include "core/Hydrogen.h"
#include "core/Basics/Song.h"
#include "core/AudioEngine.h"
#include "core/NsmClient.h"
#include "core/Nsm.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDomDocument>
#include <pthread.h>
#include <unistd.h>

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

NsmClient * NsmClient::__instance = nullptr;
const char* NsmClient::__class_name = "NsmClient";
bool NsmClient::bNsmShutdown = false;


NsmClient::NsmClient()
	: Object( __class_name )
{
	m_NsmThread = 0;
	m_bUnderSessionManagement = false;
	m_sSessionFolderPath = "";
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
	auto pController = pHydrogen->getCoreActionController();
	
	if ( !name ) {
		NsmClient::printError( "No `name` supplied in NSM open callback!" );
		return ERR_LAUNCH_FAILED;
	}

	// Cause there is no newline in the output of nsmd shown
	// beforehand.
	std::cout << std::endl;
	
	// At this point the GUI can be assumed to have to be fully
	// initialized.
	NsmClient::copyPreferences( name );
	
	// NSM sends a unique string, like - if the displayName ==
	// Hydrogen - "Hydrogen.nJKUV". In order to make the whole
	// Hydrogen session reproducible, a folder will be created, which
	// will contain the song file, a copy of the current preferences,
	// and a symbolic link to the drumkit.
	QDir sessionFolder( name );
	if ( !sessionFolder.exists() ) {
		if ( !sessionFolder.mkpath( name ) ) {
			NsmClient::printError( "Folder could not created." );
		}
	}
	
	NsmClient::get_instance()->m_sSessionFolderPath = name;
	
	const QFileInfo sessionPath( name );
	const QString sSongPath = QString( "%1/%2%3" )
		.arg( name )
		.arg( sessionPath.fileName() )
		.arg( H2Core::Filesystem::songs_ext );
	
	const QFileInfo songFileInfo = QFileInfo( sSongPath );

	// When restarting the JACK client (later in this function) the
	// clientID will be used as the name of the freshly created
	// instance.
	if ( pPref != nullptr ){
		if ( clientID ){
			// Setup JACK here, client_id gets the JACK client name
			pPref->setNsmClientId( QString( clientID ) );
		} else {
			NsmClient::printError( "No `clientID` supplied in NSM open callback!" );
			return ERR_LAUNCH_FAILED;
		}
	} else {
		NsmClient::printError( "Preferences instance is not ready yet!" );
		return ERR_NOT_NOW;
	}
	
	H2Core::Song* pSong = nullptr;
	if ( songFileInfo.exists() ) {

		pSong = H2Core::Song::load( sSongPath );
		if ( pSong == nullptr ) {
			NsmClient::printError( QString( "Unable to open existing Song [%1]." )
								   .arg( sSongPath ) );
			return ERR_LAUNCH_FAILED;
		}
		
	} else {

		pSong = H2Core::Song::get_empty_song();
		if ( pSong == nullptr ) {
			NsmClient::printError( "Unable to open new Song." );
			return ERR_LAUNCH_FAILED;
		}
		pSong->set_filename( sSongPath );
	}

	// When starting Hydrogen with its Qt5 GUI activated, the chosen
	// Song will be set via the GUI. But since it is constructed after
	// the NSM client, using the corresponding OSC message to open a
	// Song won't work in this scenario (since this would set the Song
	// asynchronously using the EventQueue and it is require during
	// the construction of MainForm).
	//
	// Two different scenarios are considered in here:
	// 1. notReady && unavailable:
	//    There is no GUI or there will be a GUI but it is not
	//    initialized yet.
	// 2. > ready:
	//    There is a GUI present and it is fully loaded.
	//
	// Scenario 2. is active when switching between sessions.
	//
	// Loading the Song is a little bit tricky in the first
	// scenario. The much more slim setInitialSong() function is used
	// since setSong() requires the audio driver to be already present
	// which would keep external tools from rewiring the per track
	// outputs of the JACK client. In 2. the Song _must_ the loaded by
	// the GUI or Hydrogen will get out of sync and freeze. The Song
	// will be stored using setNextSong() and an event will be created
	// to tell the GUI to load the Song itself.
	if ( pHydrogen->getGUIState() == H2Core::Hydrogen::GUIState::notReady ||
		 pHydrogen->getGUIState() == H2Core::Hydrogen::GUIState::unavailable ) {
		
		// No GUI. Just load the requested Song and restart the audio
		// driver.
		pHydrogen->setInitialSong( pSong );
		pHydrogen->restartDrivers();
		pHydrogen->restartLadspaFX();
		H2Core::AudioEngine::get_instance()->get_sampler()->reinitializePlaybackTrack();

		// If there will be a GUI but it is not ready yet, wait until
		// the Song was set (asynchronously by the GUI) and the GUI is
		// fully loaded.
		if ( pHydrogen->getGUIState() == H2Core::Hydrogen::GUIState::notReady ) {
			const int nNumberOfChecks = 20;
			int nCheck = 0;
			while ( true ) {
				if ( ( ( pSong == pHydrogen->getSong() ) &&
					   ( pHydrogen->getGUIState() != H2Core::Hydrogen::GUIState::notReady ) ) ||
					 ( nCheck > nNumberOfChecks ) ) {
					break;
				}
				nCheck++;
				sleep(1);
			}
		}
		
	} else {

		// The opening of the Song will be done asynchronously.
		pHydrogen->setNextSong( pSong );
		
		bool bSuccess;
		if ( songFileInfo.exists() ) {
			// Open the existing file.
			bSuccess = pController->openSong( sSongPath, true );
		} else {
			// Create a new file and save it as using the provided path.
			bSuccess = pController->newSong( sSongPath );
		}

		if ( !bSuccess ) {
			NsmClient::printError( "Unable to handle opening action!" );
			return ERR_LAUNCH_FAILED;
		}
	}
	
	NsmClient::printMessage( "Song loaded!" );
	
	NsmClient::linkDrumkit( name );
			
	return ERR_OK;
}

void NsmClient::copyPreferences( const char* name ) {
	
	auto pPref = H2Core::Preferences::get_instance();
	const auto pHydrogen = H2Core::Hydrogen::get_instance();
	
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
	pPref->setPreferencesOverwritePath( sNewPreferencesPath );
	
	const QFileInfo newPreferencesFileInfo( sNewPreferencesPath );
	if ( newPreferencesFileInfo.exists() ){
		// If there's already a preference file present from a
		// previous session, we load it instead of overwriting it.
		pPref->loadPreferences( false );
		
	} else {
		if ( !preferences.copy( sNewPreferencesPath ) ) {
			NsmClient::printError( QString( "Unable to copy preferences to [%1]" )
								   .arg( sNewPreferencesPath ) );
		} else {
			NsmClient::printMessage( QString( "Preferences copied to [%1]" )
									 .arg( sNewPreferencesPath ) );
			// The copied preferences file is already loaded.
		}
	}

	// If the GUI is active, we have to update it to reflect the
	// changes in the preferences.
	if ( pHydrogen->getGUIState() == H2Core::Hydrogen::GUIState::ready ) {
		H2Core::EventQueue::get_instance()->push_event( H2Core::EVENT_UPDATE_PREFERENCES, 1 );
	}
	
	NsmClient::printMessage( "Preferences loaded!" );
}

void NsmClient::linkDrumkit( const char* name ) {	
	
	const auto pHydrogen = H2Core::Hydrogen::get_instance();
	
	bool bRelinkDrumkit = true;
	
	const QString sDrumkitName = pHydrogen->getCurrentDrumkitname();
	
	const QString sLinkedDrumkitPath = QString( "%1/%2" )
		.arg( name ).arg( "drumkit" );
	const QFileInfo linkedDrumkitPathInfo( sLinkedDrumkitPath );
	
	// Check whether the linked folder is still valid.
	if ( linkedDrumkitPathInfo.isSymLink() || 
		 linkedDrumkitPathInfo.isDir() ) {
		
		// In case of a symbolic link, the target it is pointing to
		// has to be resolved. If drumkit is a real folder, we will
		// search for a drumkit.xml therein.
		QString sDrumkitXMLPath;
		if ( linkedDrumkitPathInfo.isSymLink() ) {
			sDrumkitXMLPath = QString( "%1/%2" )
				.arg( linkedDrumkitPathInfo.symLinkTarget() )
				.arg( "drumkit.xml" );
		} else {
			sDrumkitXMLPath = QString( "%1/%2" )
				.arg( sLinkedDrumkitPath ).arg( "drumkit.xml" );
		}
		
		const QFileInfo drumkitXMLInfo( sDrumkitXMLPath );
		if ( drumkitXMLInfo.exists() ) {
	
			const QDomDocument drumkitXML = H2Core::LocalFileMng::openXmlDocument( sDrumkitXMLPath );
			const QDomNodeList nodeList = drumkitXML.elementsByTagName( "drumkit_info" );
	
			if( nodeList.isEmpty() ) {
				NsmClient::printError( "Linked drumkit does not seem valid." );
			} else {
				const QDomNode drumkitInfoNode = nodeList.at( 0 );
				const QString sDrumkitNameXML = H2Core::LocalFileMng::readXmlString( drumkitInfoNode, "name", "" );
	
				if ( sDrumkitNameXML == sDrumkitName ) {
					bRelinkDrumkit = false;
				} else {
					NsmClient::printError( QString( "Linked [%1] and loaded [%2] drumkit do not match." )
										   .arg( sDrumkitNameXML )
										   .arg( sDrumkitName ) );
				}
			}
		} else {
			NsmClient::printError( "Symlink does not point to valid drumkit." );
		}				   
	}
	
	// The symbolic link either does not exist, is not valid, or does
	// point to the wrong location. Remove it and create a fresh one.
	if ( bRelinkDrumkit ){
		NsmClient::printMessage( "Relinking drumkit" );
		QFile linkedDrumkitFile( sLinkedDrumkitPath );
		
		if ( linkedDrumkitFile.exists() ) {
			if ( linkedDrumkitPathInfo.isDir() &&
				 ! linkedDrumkitPathInfo.isSymLink() ) {
				// Move the folder so we don't use the precious old
				// drumkit. But in order to use it again, it has to be
				// renamed to 'drumkit' manually again.
				QDir oldDrumkitFolder( sLinkedDrumkitPath );
				if ( ! oldDrumkitFolder.rename( sLinkedDrumkitPath,
												QString( "%1/drumkit_old" ).arg( name ) ) ) {
					NsmClient::printError( QString( "Unable to rename drumkit folder [%1]." )
										   .arg( sLinkedDrumkitPath ) );
					return;
				}
			} else {
				if ( !linkedDrumkitFile.remove() ) {
					NsmClient::printError( QString( "Unable to remove symlink to drumkit [%1]." )
										   .arg( sLinkedDrumkitPath ) );
					return;
				}
			}
		}
		
		// Figure out the actual path to the drumkit. We will search
		// the user drumkits first and the system ones second.
		QString sDrumkitAbsPath( "" );
		const QStringList drumkitListUsr = H2Core::Filesystem::usr_drumkit_list();
		for ( auto ssName : drumkitListUsr ) {
			if ( ssName == sDrumkitName ) {
				sDrumkitAbsPath = H2Core::Filesystem::usr_drumkits_dir() + ssName;
			}
		}
		
		if ( sDrumkitAbsPath.isEmpty() ) {
			const QStringList drumkitListSys = H2Core::Filesystem::sys_drumkit_list();
			for ( auto ssName : drumkitListSys ) {
				if ( ssName == sDrumkitName ) {
					sDrumkitAbsPath = H2Core::Filesystem::sys_drumkits_dir() + ssName;
				}
			}
		}
		
		if ( sDrumkitAbsPath.isEmpty() ) {
			// Something went wrong. We skip the linking.
			NsmClient::printError( QString( "No drumkit named [%1] could be found." )
								   .arg( sDrumkitName ) );
		} else {
			
			// Actual linking.
			QFile targetPath( sDrumkitAbsPath );
			if ( !targetPath.link( sLinkedDrumkitPath ) ) {
				NsmClient::printError( QString( "Unable to link drumkit [%1] to [%2]." )
									   .arg( sLinkedDrumkitPath )
									   .arg( sDrumkitAbsPath ) );
			}
		}
	}
}

void NsmClient::printError( const QString& msg ) {
	std::cerr << "[\033[30mHydrogen\033[0m]\033[31m "
			  << "Error " << msg.toLocal8Bit().data() << "\033[0m" << std::endl;
}
void NsmClient::printMessage( const QString& msg ) {
	std::cerr << "[\033[30mHydrogen\033[0m]\033[32m "
			  << msg.toLocal8Bit().data() << "\033[0m" << std::endl;
}

int NsmClient::SaveCallback( char** outMsg, void* userData ) {

	auto pController = H2Core::Hydrogen::get_instance()->getCoreActionController();

	if ( ! pController->saveSong() ) {
		NsmClient::printError( "Unable to save Song!" );
		return ERR_GENERAL;
	}
	if ( ! pController->savePreferences() ) {
		NsmClient::printError( "Unable to save Preferences!" );
		return ERR_GENERAL;
	}

	NsmClient::printMessage( "Song and Preferences saved!" );

	return ERR_OK;
}

void* NsmClient::ProcessEvent(void* data) {
	nsm_client_t* nsm = (nsm_client_t*) data;

	while( !NsmClient::bNsmShutdown && nsm ){
		nsm_check_wait( nsm, 1000 );
	}

	return nullptr;
}

void NsmClient::shutdown()
{
	NsmClient::bNsmShutdown = true;
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
			nsm_set_open_callback( nsm, NsmClient::OpenCallback, (void*) nullptr );
			nsm_set_save_callback( nsm, NsmClient::SaveCallback, (void*) nullptr );

			if ( nsm_init( nsm, nsm_url ) == 0 )
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
				
				nsm_send_announce( nsm, "Hydrogen", ":dirty:switch:", byteArray.data() );
						
				if ( pthread_create( &m_NsmThread, nullptr, NsmClient::ProcessEvent, nsm ) ) {
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
					if ( pHydrogen->getAudioOutput() != nullptr ) {
						break;
					}
					// Don't wait indefinitely.
					if ( nCheck > nNumberOfChecks ) {
						break;
				   }
					nCheck++;
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

void NsmClient::sendDirtyState( const bool bIsDirty ) {
	
	if ( bIsDirty ) {
		nsm_send_is_dirty( m_nsm );
	} else {
		nsm_send_is_clean( m_nsm );
	}
}

#endif /* H2CORE_HAVE_OSC */

