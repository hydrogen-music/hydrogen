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

#include "core/Helpers/Filesystem.h"
#include "core/Preferences/Preferences.h"
#include "core/EventQueue.h"
#include "core/Hydrogen.h"
#include "core/Basics/Drumkit.h"
#include "core/Basics/Instrument.h"
#include "core/Basics/InstrumentComponent.h"
#include "core/Basics/InstrumentLayer.h"
#include "core/Basics/Sample.h"
#include "core/Basics/Song.h"
#include "core/AudioEngine/AudioEngine.h"
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
	auto pController = pHydrogen->getCoreActionController();
	
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
		}
	}

	NsmClient::copyPreferences( name );

	NsmClient::get_instance()->setSessionFolderPath( name );
	
	const QFileInfo sessionPath( name );
	const QString sSongPath = QString( "%1/%2%3" )
		.arg( name )
		.arg( sessionPath.fileName() )
		.arg( H2Core::Filesystem::songs_ext );
	
	const QFileInfo songFileInfo = QFileInfo( sSongPath );

	// When restarting the JACK client (during song loading) the
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

	bool bEmptySongOpened = false;
	std::shared_ptr<H2Core::Song> pSong = nullptr;
	if ( songFileInfo.exists() ) {

		// Song loading itself does not add the drumkit found to the
		// SoundLibraryDatabase (in order to avoid problem with cyclic
		// dependencies between drumkits).
		loadDrumkit();
		
		pSong = H2Core::Song::load( sSongPath );
		if ( pSong == nullptr ) {
			NsmClient::printError( QString( "Unable to open existing Song [%1]." )
								   .arg( sSongPath ) );
			return ERR_LAUNCH_FAILED;
		}
		
	} else {

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
		NsmClient::get_instance()->setIsNewSession( true );

		// The drumkit of the new song will linked into the session
		// folder during the next song save.
		pHydrogen->setSessionDrumkitNeedsRelinking( true );
	}

	if ( ! pController->openSong( pSong, false /*relinking*/ ) ) {
			NsmClient::printError( "Unable to handle opening action!" );
			return ERR_LAUNCH_FAILED;
	}
	
	NsmClient::printMessage( "Song loaded!" );

	return ERR_OK;
}

void NsmClient::copyPreferences( const char* name ) {
	
	auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	
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

	pCoreActionController->updatePreferences();
	
	NsmClient::printMessage( "Preferences loaded!" );
}

void NsmClient::loadDrumkit() {
	
	const auto pHydrogen = H2Core::Hydrogen::get_instance();
	const QString sSessionFolder = NsmClient::get_instance()->getSessionFolderPath();
	const QString sLinkedDrumkitPath = QString( "%1/%2" )
		.arg( sSessionFolder ).arg( "drumkit" );
	const QFileInfo linkedDrumkitPathInfo( sLinkedDrumkitPath );

	// Check whether the linked folder is valid.
	if ( linkedDrumkitPathInfo.isSymLink() || 
		 linkedDrumkitPathInfo.isDir() ) {

		auto pDrumkit =
			pHydrogen->getSoundLibraryDatabase()->getDrumkit( sLinkedDrumkitPath );
		if ( pDrumkit == nullptr ) {
			ERRORLOG( "Unable to load drumkit from session folder" );
		}
	}
	else {
		ERRORLOG( "No valid drumkit found in session folder" );
	}
}

void NsmClient::linkDrumkit( std::shared_ptr<H2Core::Song> pSong ) {
	
	const auto pHydrogen = H2Core::Hydrogen::get_instance();
	
	bool bRelinkDrumkit = true;
	
	const QString sDrumkitName = pSong->getLastLoadedDrumkitName();
	const QString sDrumkitAbsPath = pSong->getLastLoadedDrumkitPath();

	const QString sSessionFolder = NsmClient::get_instance()->getSessionFolderPath();

	// Sanity check in order to avoid circular linking.
	if ( sDrumkitAbsPath.contains( sSessionFolder, Qt::CaseInsensitive ) ) {
		NsmClient::printError( QString( "Last loaded drumkit [%1] with absolute path [%2] is located within the session folder [%3]. Linking skipped." )
							   .arg( sDrumkitName )
							   .arg( sDrumkitAbsPath )
							   .arg( sSessionFolder ) );
		return;
	}
	
	const QString sLinkedDrumkitPath = QString( "%1/%2" )
		.arg( sSessionFolder ).arg( "drumkit" );
	const QFileInfo linkedDrumkitPathInfo( sLinkedDrumkitPath );

	// Check whether the linked folder is still valid.
	if ( linkedDrumkitPathInfo.isSymLink() || 
		 linkedDrumkitPathInfo.isDir() ) {
		
		// In case of a symbolic link, the target it is pointing to
		// has to be resolved. If drumkit is a real folder, we will
		// search for a drumkit.xml therein.
		QString sLinkedDrumkitPath;
		if ( linkedDrumkitPathInfo.isSymLink() ) {
			sLinkedDrumkitPath = QString( "%1" )
				.arg( linkedDrumkitPathInfo.symLinkTarget() );
		} else {
			sLinkedDrumkitPath = QString( "%1" )
				.arg( sLinkedDrumkitPath );
		}
	    
		if ( H2Core::Filesystem::drumkit_valid( sLinkedDrumkitPath ) ) {

			QString sLinkedDrumkitName( "seemsLikeTheKitCouldNotBeRetrievedFromTheDatabase" );
			auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
			if ( pSoundLibraryDatabase != nullptr ) {
				auto pDrumkit = pSoundLibraryDatabase->getDrumkit( sLinkedDrumkitPath );
				if ( pDrumkit != nullptr ) {
					sLinkedDrumkitName = pDrumkit->get_name();
				}
			}
	
			if ( sLinkedDrumkitName == sDrumkitName ) {
				bRelinkDrumkit = false;
			}
		}
		else {
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
												QString( "%1/drumkit_old" )
												.arg( sSessionFolder ) ) ) {
					NsmClient::printError( QString( "Unable to rename drumkit folder [%1]." )
										   .arg( sLinkedDrumkitPath ) );
					return;
				}
			} else {
				if ( ! linkedDrumkitFile.remove() ) {
					NsmClient::printError( QString( "Unable to remove symlink to drumkit [%1]." )
										   .arg( sLinkedDrumkitPath ) );
					return;
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

	// Replace the temporary reference to the "global" drumkit to the
	// (freshly) linked/found one in the session folder.
	NsmClient::replaceDrumkitPath( pSong, "./drumkit" );

	pHydrogen->setSessionDrumkitNeedsRelinking( false );
}

int NsmClient::dereferenceDrumkit( std::shared_ptr<H2Core::Song> pSong ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return -1;
	}

	const QString sLastLoadedDrumkitPath = pSong->getLastLoadedDrumkitPath();
	const QString sLastLoadedDrumkitName = pSong->getLastLoadedDrumkitName();

	if ( ! sLastLoadedDrumkitPath.contains( NsmClient::get_instance()->
											getSessionFolderPath(),
											Qt::CaseInsensitive ) ) {
		// Regular path. We do not have to alter it.
		return 0;
	}

	const QFileInfo lastLoadedDrumkitInfo( sLastLoadedDrumkitPath );
	if ( lastLoadedDrumkitInfo.isSymLink() ) {
		
		QString sDeferencedDrumkit = lastLoadedDrumkitInfo.symLinkTarget();

		NsmClient::printMessage( QString( "Dereferencing linked drumkit to [%1]" )
								 .arg( sDeferencedDrumkit ) );
		NsmClient::replaceDrumkitPath( pSong, sDeferencedDrumkit );
	}
	else if ( lastLoadedDrumkitInfo.isDir() ) {
		// Drumkit is not linked into the session folder but present
		// within a directory (probably because the session was
		// transfered from another device to recovered from a
		// backup).
		//
		// This is a little bit tricky as we do not want to install
		// the kit into the user's data folder on our own (loss of
		// data etc.). If a kit containing the same name is present,
		// we will assume the kits do match. That's nowhere near
		// perfect but we are dealing with an edge-case of an
		// edge-case in here anyway. If it not exists, we will prompt
		// a warning dialog (via the GUI) asking the user to install
		// it herself.
		bool bDrumkitFound = false;
		for ( const auto& pDrumkitEntry :
				  pHydrogen->getSoundLibraryDatabase()->getDrumkitDatabase() ) {

			auto pDrumkit = pDrumkitEntry.second;
			if ( pDrumkit != nullptr ) {
				if ( pDrumkit->get_name() == sLastLoadedDrumkitName ) {
					NsmClient::replaceDrumkitPath( pSong, pDrumkitEntry.first );
					bDrumkitFound = true;
					break;
						
				}
			}
		}

		if ( ! bDrumkitFound ) {
			ERRORLOG( QString( "Drumkit used in session folder [%1] is not present on the current system. It has to be installed first in order to use the exported song" )
					  .arg( sLastLoadedDrumkitName ) );
			NsmClient::replaceDrumkitPath( pSong, "" );
			return -2;
		}
		else {
			INFOLOG( QString( "Drumkit used in session folder [%1] was dereferenced to [%2]" )
					 .arg( sLastLoadedDrumkitName )
					 .arg( pSong->getLastLoadedDrumkitPath() ) );
		}
	}
	else {
		ERRORLOG( "This should not happen" );
		return -1;
	}
	return 0;
}

void NsmClient::replaceDrumkitPath( std::shared_ptr<H2Core::Song> pSong, const QString& sDrumkitPath ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();

	// We are only replacing the paths corresponding to the drumkit
	// which is either about to be linked into the session folder or
	// the one which is supposed to replace the linked one.
	const QString sDrumkitToBeReplaced = pSong->getLastLoadedDrumkitPath();
	
	pSong->setLastLoadedDrumkitPath( sDrumkitPath );

	for ( auto pInstrument : *pSong->getInstrumentList() ) {
		if ( pInstrument != nullptr &&
			 pInstrument->get_drumkit_path() == sDrumkitToBeReplaced ) {

			pInstrument->set_drumkit_path( sDrumkitPath );

			// Use full paths in case the drumkit in sDrumkitPath is
			// not located in either the user's or system's drumkit
			// folder or just use the filenames (and load the
			// relatively) otherwise.
			for ( auto pComponent : *pInstrument->get_components() ) {
				if ( pComponent != nullptr ) {
					for ( auto pInstrumentLayer : *pComponent ) {
						if ( pInstrumentLayer != nullptr ) {
							auto pSample = pInstrumentLayer->get_sample();
							if ( pSample != nullptr ) {
								QString sNewPath = QString( "%1/%2" )
									.arg( sDrumkitPath )
									.arg( pSample->get_filename() );

								pSample->set_filepath( H2Core::Filesystem::prepare_sample_path( sNewPath ) );
							}
						}
					}
				}
			}
		}
	}
}

void NsmClient::printError( const QString& msg ) {
	std::cerr << "[\033[30mHydrogen\033[0m]\033[31m "
			  << "Error: " << msg.toLocal8Bit().data() << "\033[0m" << std::endl;
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

void NsmClient::createInitialClient()
{
	/*
	 * Make first contact with NSM server.
	 */

	nsm_client_t* pNsm = nullptr;

	H2Core::Preferences *pPref = H2Core::Preferences::get_instance();
	QString H2ProcessName = pPref->getH2ProcessName();
	QByteArray byteArray = H2ProcessName.toLatin1();

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

