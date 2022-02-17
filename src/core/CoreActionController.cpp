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

#include <core/AudioEngine.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Preferences.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Instrument.h>
#include "core/OscServer.h"
#include <core/MidiAction.h>
#include "core/MidiMap.h"
#include <core/Helpers/Xml.h>

#include <core/IO/AlsaMidiDriver.h>
#include <core/IO/MidiOutput.h>
#include <core/IO/JackAudioDriver.h>

#ifdef H2CORE_HAVE_OSC
#include <core/NsmClient.h>
#endif

namespace H2Core
{

const char* CoreActionController::__class_name = "CoreActionController";


CoreActionController::CoreActionController() : Object( __class_name ),
												m_nDefaultMidiFeedbackChannel(0)
{
	//nothing
}

CoreActionController::~CoreActionController() {
	//nothing
}

void CoreActionController::setMasterVolume( float masterVolumeValue )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	pHydrogen->getSong()->setVolume( masterVolumeValue );
	
#ifdef H2CORE_HAVE_OSC
	Action FeedbackAction( "MASTER_VOLUME_ABSOLUTE" );
	FeedbackAction.setParameter2( QString("%1").arg( masterVolumeValue ) );
	OscServer::get_instance()->handleAction( &FeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionType( QString("MASTER_VOLUME_ABSOLUTE"));
	
	handleOutgoingControlChange( ccParamValue, (masterVolumeValue / 1.5) * 127 );
}

void CoreActionController::setStripVolume( int nStrip, float fVolumeValue, bool bSelectStrip )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}
	
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	Instrument *pInstr = pInstrList->get( nStrip );
	pInstr->set_volume( fVolumeValue );
	
#ifdef H2CORE_HAVE_OSC
	Action FeedbackAction( "STRIP_VOLUME_ABSOLUTE" );
	
	FeedbackAction.setParameter1( QString("%1").arg( nStrip + 1 ) );
	FeedbackAction.setParameter2( QString("%1").arg( fVolumeValue ) );
	OscServer::get_instance()->handleAction( &FeedbackAction );
#endif

	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("STRIP_VOLUME_ABSOLUTE"), QString("%1").arg( nStrip ) );
	
	handleOutgoingControlChange( ccParamValue, (fVolumeValue / 1.5) * 127 );
}

void CoreActionController::setMetronomeIsActive( bool isActive )
{
	Preferences::get_instance()->m_bUseMetronome = isActive;
	
#ifdef H2CORE_HAVE_OSC
	Action FeedbackAction( "TOGGLE_METRONOME" );
	
	FeedbackAction.setParameter1( QString("%1").arg( (int) isActive ) );
	OscServer::get_instance()->handleAction( &FeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionType( QString("TOGGLE_METRONOME"));
	
	handleOutgoingControlChange( ccParamValue, (int) isActive * 127 );
}

void CoreActionController::setMasterIsMuted( bool isMuted )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	pHydrogen->getSong()->setIsMuted( isMuted );
	
#ifdef H2CORE_HAVE_OSC
	Action FeedbackAction( "MUTE_TOGGLE" );
	
	FeedbackAction.setParameter1( QString("%1").arg( (int) isMuted ) );
	OscServer::get_instance()->handleAction( &FeedbackAction );
#endif

	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionType( QString("MUTE_TOGGLE") );

	handleOutgoingControlChange( ccParamValue, (int) isMuted * 127 );
}

void CoreActionController::toggleStripIsMuted(int nStrip)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if( pInstrList->is_valid_index( nStrip ))
	{
		Instrument* pInstr = pInstrList->get( nStrip );
		
		if( pInstr ) {
			setStripIsMuted( nStrip , !pInstr->is_muted() );
		}
	}
}

void CoreActionController::setStripIsMuted( int nStrip, bool isMuted )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	Instrument *pInstr = pInstrList->get( nStrip );
	pInstr->set_muted( isMuted );
	
#ifdef H2CORE_HAVE_OSC
	Action FeedbackAction( "STRIP_MUTE_TOGGLE" );
	
	FeedbackAction.setParameter1( QString("%1").arg( nStrip + 1 ) );
	FeedbackAction.setParameter2( QString("%1").arg( (int) isMuted ) );
	OscServer::get_instance()->handleAction( &FeedbackAction );
#endif

	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("STRIP_MUTE_TOGGLE"), QString("%1").arg( nStrip ) );
	
	handleOutgoingControlChange( ccParamValue, ((int) isMuted) * 127 );
}

void CoreActionController::toggleStripIsSoloed( int nStrip )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if( pInstrList->is_valid_index( nStrip ))
	{
		Instrument* pInstr = pInstrList->get( nStrip );
	
		if( pInstr ) {
			setStripIsSoloed( nStrip , !pInstr->is_soloed() );
		}
	}
}

void CoreActionController::setStripIsSoloed( int nStrip, bool isSoloed )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	Instrument* pInstr = pInstrList->get( nStrip );
	pInstr->set_soloed( isSoloed );
	
#ifdef H2CORE_HAVE_OSC
	Action FeedbackAction( "STRIP_SOLO_TOGGLE" );
	
	FeedbackAction.setParameter1( QString("%1").arg( nStrip + 1 ) );
	FeedbackAction.setParameter2( QString("%1").arg( (int) isSoloed ) );
	OscServer::get_instance()->handleAction( &FeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("STRIP_SOLO_TOGGLE"), QString("%1").arg( nStrip ) );
	
	handleOutgoingControlChange( ccParamValue, ((int) isSoloed) * 127 );
}



void CoreActionController::setStripPan( int nStrip, float fPanValue, bool bSelectStrip )
{
	float	fPan_L;
	float	fPan_R;

	if ( fPanValue >= 0.5 ) {
		fPan_L = (1.0 - fPanValue) * 2;
		fPan_R = 1.0;
	}
	else {
		fPan_L = 1.0;
		fPan_R = fPanValue * 2;
	}

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}
	
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	Instrument *pInstr = pInstrList->get( nStrip );
	pInstr->set_pan_l( fPan_L );
	pInstr->set_pan_r( fPan_R );
	
#ifdef H2CORE_HAVE_OSC
	Action FeedbackAction( "PAN_ABSOLUTE" );
	
	FeedbackAction.setParameter1( QString("%1").arg( nStrip + 1 ) );
	FeedbackAction.setParameter2( QString("%1").arg( fPanValue ) );
	OscServer::get_instance()->handleAction( &FeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("PAN_ABSOLUTE"), QString("%1").arg( nStrip ) );
	

	handleOutgoingControlChange( ccParamValue, fPanValue * 127 );
}

void CoreActionController::handleOutgoingControlChange(int param, int value)
{
	Preferences *pPref = Preferences::get_instance();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	MidiOutput *pMidiDriver = pHydrogen->getMidiOutput();
	
	if(	pMidiDriver 
		&& pPref->m_bEnableMidiFeedback 
		&& param >= 0 ){
		pMidiDriver->handleOutgoingControlChange( param, value, m_nDefaultMidiFeedbackChannel );
	}
}

void CoreActionController::initExternalControlInterfaces()
{
	/*
	 * Push the current state of Hydrogen to the attached control interfaces (e.g. OSC clients)
	 */
	
	//MASTER_VOLUME_ABSOLUTE
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	setMasterVolume( pSong->getVolume() );
	
	//PER-INSTRUMENT/STRIP STATES
	InstrumentList *pInstrList = pSong->getInstrumentList();
	for(int i=0; i < pInstrList->size(); i++){
		
			//STRIP_VOLUME_ABSOLUTE
			Instrument *pInstr = pInstrList->get( i );
			setStripVolume( i, pInstr->get_volume(), false );
			
			float fPan_L = pInstr->get_pan_l();
			float fPan_R = pInstr->get_pan_r();

			//PAN_ABSOLUTE
			float fPanValue = 0.0;
			if (fPan_R == 1.0) {
				fPanValue = 1.0 - (fPan_L / 2.0);
			}
			else {
				fPanValue = fPan_R / 2.0;
			}
		
			setStripPan( i, fPanValue, false );
			
			//STRIP_MUTE_TOGGLE
			setStripIsMuted( i, pInstr->is_muted() );
			
			//SOLO
			if(pInstr->is_soloed()) {
				setStripIsSoloed( i, pInstr->is_soloed() );
			}
	}
	
	//TOGGLE_METRONOME
	setMetronomeIsActive( Preferences::get_instance()->m_bUseMetronome );
	
	//MUTE_TOGGLE
	setMasterIsMuted( Hydrogen::get_instance()->getSong()->getIsMuted() );
}

bool CoreActionController::newSong( const QString& sSongPath ) {
	
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getState() == STATE_PLAYING ) {
		// Stops recording, all queued MIDI notes, and the playback of
		// the audio driver.
		pHydrogen->sequencer_stop();
	}
	
	// Remove all BPM tags on the Timeline.
	pHydrogen->getTimeline()->deleteAllTempoMarkers();
	
	// Create an empty Song.
	auto pSong = Song::getEmptySong();
	
	// Check whether the provided path is valid.
	if ( !isSongPathValid( sSongPath ) ) {
		// isSongPathValid takes care of the error log message.

		return false;
	}
	
	pSong->setFilename( sSongPath );
	
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		
		// Store the prepared Song for the GUI to access after the
		// EVENT_UPDATE_SONG event was triggered.
		pHydrogen->setNextSong( pSong );
		
		// If the GUI is active, the Song *must not* be set by the
		// core part itself.
		// Triggers an update of the Qt5 GUI and tells it to update
		// the song itself.
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 0 );
		
	} else {

		// Update the Song.
		pHydrogen->setSong( pSong );
		
	}
	
	return true;
}

bool CoreActionController::openSong( const QString& sSongPath ) {
	
	auto pHydrogen = Hydrogen::get_instance();
 
	if ( pHydrogen->getState() == STATE_PLAYING ) {
		// Stops recording, all queued MIDI notes, and the playback of
		// the audio driver.
		pHydrogen->sequencer_stop();
	}
	
	// Check whether the provided path is valid.
	if ( !isSongPathValid( sSongPath ) ) {
		// isSongPathValid takes care of the error log message.
		return false;
	}
	
	// Create an empty Song.
	auto pSong = Song::load( sSongPath );

	if ( pSong == nullptr ) {
		ERRORLOG( QString( "Unable to open song [%1]." )
				  .arg( sSongPath ) );
		return false;
	}
	
	return setSong( pSong );
}

bool CoreActionController::openSong( Song* pSong ) {
	
	auto pHydrogen = Hydrogen::get_instance();
 
	if ( pHydrogen->getState() == STATE_PLAYING ) {
		// Stops recording, all queued MIDI notes, and the playback of
		// the audio driver.
		pHydrogen->sequencer_stop();
	}
	
	if ( pSong == nullptr ) {
		ERRORLOG( QString( "Unable to open song." ) );
		return false;
	}

	return setSong( pSong );
}

bool CoreActionController::setSong( Song* pSong ) {

	auto pHydrogen = Hydrogen::get_instance();
	
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		
		// Store the prepared Song for the GUI to access after the
		// EVENT_UPDATE_SONG event was triggered.
		pHydrogen->setNextSong( pSong );
		
		int nRestartAudioDriver = 0;

		if ( pHydrogen->isUnderSessionManagement() ) {
			nRestartAudioDriver = 1;
		}
		
		// If the GUI is active, the Song *must not* be set by the
		// core part itself.
		// Triggers an update of the Qt5 GUI and tells it to update
		// the song itself.
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, nRestartAudioDriver );
		
	} else {

		// Update the Song.
		pHydrogen->setSong( pSong );
		
		if ( pHydrogen->isUnderSessionManagement() ) {
			pHydrogen->restartDrivers();
		}
	}
	
	return true;
}

bool CoreActionController::saveSong() {
	
	auto pHydrogen = Hydrogen::get_instance();

	// Get the current Song which is about to be saved.
	auto pSong = pHydrogen->getSong();
	
	// Extract the path to the associate .h2song file.
	QString sSongPath = pSong->getFilename();
	
	if ( sSongPath.isEmpty() ) {
		ERRORLOG( "Unable to save song. Empty filename!" );
		return false;
	}
	
	// Actual saving
	bool saved = pSong->save( sSongPath );
	if ( !saved ) {
		ERRORLOG( QString( "Current song [%1] could not be saved!" )
				  .arg( sSongPath ) );
		return false;
	}
	
	// Update the status bar.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 2 );
	}
	
	return true;
}

bool CoreActionController::saveSongAs( const QString& sSongPath ) {
	
	auto pHydrogen = Hydrogen::get_instance();
	
	// Get the current Song which is about to be saved.
	auto pSong = pHydrogen->getSong();
	
	// Check whether the provided path is valid.
	if ( !isSongPathValid( sSongPath ) ) {
		// isSongPathValid takes care of the error log message.
		return false;
	}
	
	if ( sSongPath.isEmpty() ) {
		ERRORLOG( "Unable to save song. Empty filename!" );
		return false;
	}
	
	// Actual saving
	bool bSaved = pSong->save( sSongPath );
	if ( !bSaved ) {
		ERRORLOG( QString( "Current song [%1] could not be saved!" )
				  .arg( sSongPath ) );
		return false;
	}
	
	// Update the status bar.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 2 );
	}
	
	return true;
}

bool CoreActionController::savePreferences() {
	
	if ( Hydrogen::get_instance()->getGUIState() != Hydrogen::GUIState::unavailable ) {
		// Update the status bar and let the GUI save the preferences
		// (after writing its current settings to disk).
		EventQueue::get_instance()->push_event( EVENT_UPDATE_PREFERENCES, 0 );
	} else {
		Preferences::get_instance()->savePreferences();
	}
	
	return true;

}
bool CoreActionController::quit() {

	if ( Hydrogen::get_instance()->getGUIState() != Hydrogen::GUIState::unavailable ) {
		EventQueue::get_instance()->push_event( EVENT_QUIT, 0 );
	} else {
		// TODO: Close Hydrogen with no GUI present.
		
		ERRORLOG( QString( "Error: Closing the application via the core part is not supported yet!" ) );
		return false;
		
	}
	
	return true;
}

bool CoreActionController::isSongPathValid( const QString& sSongPath ) {
	
	QFileInfo songFileInfo = QFileInfo( sSongPath );

	if ( !songFileInfo.isAbsolute() ) {
		ERRORLOG( QString( "Error: Unable to handle path [%1]. Please provide an absolute file path!" )
						.arg( sSongPath.toLocal8Bit().data() ));
		return false;
	}
	
	if ( songFileInfo.exists() ) {
		if ( !songFileInfo.isReadable() ) {
			ERRORLOG( QString( "Error: Unable to handle path [%1]. You must have permissions to read the file!" )
						.arg( sSongPath.toLocal8Bit().data() ));
			return false;
		}
		if ( !songFileInfo.isWritable() ) {
			WARNINGLOG( QString( "You don't have permissions to write to the Song found in path [%1]. It will be opened as read-only (no autosave)." )
						.arg( sSongPath.toLocal8Bit().data() ));
			EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 3 );
		}
	}
	
	if ( songFileInfo.suffix() != "h2song" ) {
		ERRORLOG( QString( "Error: Unable to handle path [%1]. The provided file must have the suffix '.h2song'!" )
					.arg( sSongPath.toLocal8Bit().data() ));
		return false;
	}
	
	return true;
}

bool CoreActionController::activateTimeline( bool bActivate ) {
	auto pHydrogen = Hydrogen::get_instance();
	
	if ( pHydrogen->getJackTimebaseState() == JackAudioDriver::Timebase::Slave ) {
		ERRORLOG( "Timeline usage is disabled in the presence of an external JACK timebase master." );
		return false;
	}
	
	Preferences::get_instance()->setUseTimelineBpm( bActivate );

	if ( bActivate && !pHydrogen->haveJackTransport() ) {
		// In case another driver than Jack is used, we have to update
		// the tempo explicitly.
		pHydrogen->setTimelineBpm();
	}
	
	EventQueue::get_instance()->push_event( EVENT_TIMELINE_ACTIVATION, static_cast<int>( bActivate ) );
	
	return true;
}

bool CoreActionController::addTempoMarker( int nPosition, float fBpm ) {
	auto pTimeline = Hydrogen::get_instance()->getTimeline();
	pTimeline->deleteTempoMarker( nPosition );
	pTimeline->addTempoMarker( nPosition, fBpm );

	EventQueue::get_instance()->push_event( EVENT_TIMELINE_UPDATE, 0 );

	return true;
}

bool CoreActionController::deleteTempoMarker( int nPosition ) {
	Hydrogen::get_instance()->getTimeline()->deleteTempoMarker( nPosition );
	EventQueue::get_instance()->push_event( EVENT_TIMELINE_UPDATE, 0 );

	return true;
}

bool CoreActionController::activateJackTransport( bool bActivate ) {
	
#ifdef H2CORE_HAVE_JACK
	if ( !Hydrogen::get_instance()->haveJackAudioDriver() ) {
		ERRORLOG( "Unable to (de)activate Jack transport. Please select the Jack driver first." );
		return false;
	}
	
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	if ( bActivate ) {
		Preferences::get_instance()->m_bJackTransportMode = Preferences::USE_JACK_TRANSPORT;
	} else {
		Preferences::get_instance()->m_bJackTransportMode = Preferences::NO_JACK_TRANSPORT;
	}
	AudioEngine::get_instance()->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_JACK_TRANSPORT_ACTIVATION, static_cast<int>( bActivate ) );
	
	return true;
#else
	ERRORLOG( "Unable to (de)activate Jack transport. Your Hydrogen version was not compiled with jack support." );
	return false;
#endif
}

bool CoreActionController::activateJackTimebaseMaster( bool bActivate ) {

#ifdef H2CORE_HAVE_JACK
	if ( !Hydrogen::get_instance()->haveJackAudioDriver() ) {
		ERRORLOG( "Unable to (de)activate Jack timebase master. Please select the Jack driver first." );
		return false;
	}
	
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	if ( bActivate ) {
		Preferences::get_instance()->m_bJackMasterMode = Preferences::USE_JACK_TIME_MASTER;
		Hydrogen::get_instance()->onJackMaster();
	} else {
		Preferences::get_instance()->m_bJackMasterMode = Preferences::NO_JACK_TIME_MASTER;
		Hydrogen::get_instance()->offJackMaster();
	}
	AudioEngine::get_instance()->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_JACK_TIMEBASE_ACTIVATION, static_cast<int>( bActivate ) );
	
	return true;
#else
	ERRORLOG( "Unable to (de)activate Jack timebase master. Your Hydrogen version was not compiled with jack support." );
	return false;
#endif
}

bool CoreActionController::activateSongMode( bool bActivate, bool bTriggerEvent ) {

	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->sequencer_stop();
	if ( bActivate ) {
		pHydrogen->setPatternPos( 0 );
		pHydrogen->getSong()->setMode( Song::SONG_MODE );
	} else {
		pHydrogen->getSong()->setMode( Song::PATTERN_MODE );
	}
	
	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_SONG_MODE_ACTIVATION, static_cast<int>( bActivate ) );
	}
	
	return true;
}

bool CoreActionController::upgradeDrumkit( const QString& sDrumkitPath, const QString& sNewPath ) {

	if ( sNewPath.isEmpty() ) {
		INFOLOG( QString( "Upgrading kit at [%1] inplace." )
				 .arg( sDrumkitPath ) );
	} else {
		INFOLOG( QString( "Upgrading kit at [%1] into [%2]." )
				 .arg( sDrumkitPath ).arg( sNewPath ) );
	}

	QFileInfo sourceFileInfo( sDrumkitPath );
	if ( ! sNewPath.isEmpty() ) {
		// Check whether there is already a file or directory
		// present. The latter has to be writable. If none is present,
		// create a folder.
		if ( ! Filesystem::path_usable( sNewPath, true, false ) ) {
			return false;
		}
	} else {
		// We have to assure that the source folder is not just
		// readable since an inplace upgrade was requested
		if ( ! Filesystem::dir_writable( sourceFileInfo.dir().absolutePath(),
										 true ) ) {
			ERRORLOG( QString( "Unable to upgrade drumkit [%1] in place: Folder is in read-only mode" )
					  .arg( sDrumkitPath ) );
			return false;
		}
	}

	QString sTemporaryFolder, sDrumkitDir;
	// Whether the drumkit was provided as compressed .h2drumkit file.
	bool bIsCompressed;
	auto pDrumkit = retrieveDrumkit( sDrumkitPath, &bIsCompressed,
									 &sDrumkitDir, &sTemporaryFolder );

	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to load drumkit from source path [%1]" )
				  .arg( sDrumkitPath ) );
		return false;
	}

	// If the drumkit is not updated inplace, we also need to copy
	// all samples and metadata, like images.
	QString sPath;
	if ( ! sNewPath.isEmpty() ) {

		// When dealing with a compressed drumkit, we can just leave
		// it in the temporary folder and copy the compressed content
		// to the destination right away.
		if ( ! bIsCompressed ) {
			// Copy content
			QDir drumkitDir( sDrumkitDir );
			for ( const auto& ssFile : drumkitDir.entryList( QDir::Files ) ) {

				// We handle the drumkit file later
				if ( ssFile.contains( ".xml" ) ) {
					continue;
				}
				Filesystem::file_copy( drumkitDir.absolutePath() + "/" + ssFile,
									   sNewPath + "/" + ssFile, true, true );
			}
			sPath = sNewPath;
		} else {
			sPath = sDrumkitDir;
		}
		
	} else {
		// Upgrade inplace.

		if ( ! bIsCompressed ) {
			// Make a backup of the original file in order to make the
			// upgrade reversible.
			QString sBackupPath =
				Filesystem::drumkit_backup_path( Filesystem::drumkit_file( sDrumkitDir ) );
			if ( ! Filesystem::file_copy( Filesystem::drumkit_file( sDrumkitDir ),
										  sBackupPath, true, true ) ) {
				ERRORLOG( QString( "Unable to backup source drumkit XML file from [%1] to [%2]. We abort instead of overwriting things." )
						  .arg( Filesystem::drumkit_file( sDrumkitDir ) )
						  .arg( sBackupPath ) );
				delete pDrumkit;
				return false;
			}
		} else {
			QString sBackupPath = Filesystem::drumkit_backup_path( sDrumkitPath );
			if ( ! Filesystem::file_copy( sDrumkitPath, sBackupPath, true, true ) ) {
				ERRORLOG( QString( "Unable to backup source .h2drumkit file from [%1] to [%2]. We abort instead of overwriting things." )
						  .arg( sDrumkitPath ).arg( sBackupPath ) );
				delete pDrumkit;
				return false;
			}
		}

		sPath = sDrumkitDir;
	}

	if ( ! pDrumkit->save_file( Filesystem::drumkit_file( sPath ), true, -1, true ) ) {
		ERRORLOG( QString( "Error while saving upgraded kit to [%1]" )
				  .arg( sPath ) );
		delete pDrumkit;
		return false;
	}

	// Compress the updated drumkit again in order to provide the same
	// format handed over as input.
	if ( bIsCompressed ) {
		QString sExportPath;
		if ( ! sNewPath.isEmpty() ) {
			sExportPath = sNewPath;
		} else {
			sExportPath = sourceFileInfo.dir().absolutePath();
		}
		
		if ( ! pDrumkit->exportTo( sExportPath, "", true, false ) ) {
			ERRORLOG( QString( "Unable to export upgrade drumkit to [%1]" )
					  .arg( sExportPath ) );
			delete pDrumkit;
			return false;
		}

		INFOLOG( QString( "Upgraded drumkit exported as [%1]" )
				 .arg( sExportPath + "/" + pDrumkit->get_name() +
					   Filesystem::drumkit_ext ) );
	}

	// Upgrade was successful. Cleanup
	if ( ! sTemporaryFolder.isEmpty() ) {
		// Filesystem::rm( sTemporaryFolder, true, true );
	}

	INFOLOG( QString( "Drumkit [%1] successfully upgraded!" )
			 .arg( sDrumkitPath ) );

	delete pDrumkit;

	return true;
}

bool CoreActionController::validateDrumkit( const QString& sDrumkitPath ) {

	INFOLOG( QString( "Validating kit [%1]." ).arg( sDrumkitPath ) );

	QString sTemporaryFolder, sDrumkitDir;
	// Whether the drumkit was provided as compressed .h2drumkit file.
	bool bIsCompressed;
	auto pDrumkit = retrieveDrumkit( sDrumkitPath, &bIsCompressed,
									 &sDrumkitDir, &sTemporaryFolder );

	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to load drumkit from source path [%1]" )
				  .arg( sDrumkitPath ) );
		return false;
	}

	if ( ! Filesystem::drumkit_valid( sDrumkitDir ) ) {
		ERRORLOG( QString( "Something went wrong in the drumkit retrieval of [%1]. Unable to load from [%2]" )
				  .arg( sDrumkitPath ).arg( sDrumkitDir ) );
		delete pDrumkit;
		return false;
	}

	XMLDoc doc;
	if ( !doc.read( Filesystem::drumkit_file( sDrumkitDir ),
					Filesystem::drumkit_xsd_path(), true ) ) {
		ERRORLOG( QString( "Drumkit file [%1] does not comply with the current XSD definition" )
				  .arg( Filesystem::drumkit_file( sDrumkitDir ) ) );
		delete pDrumkit;
		return false;
	}
	
	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( QString( "Drumkit file [%1] seems bricked: 'drumkit_info' node not found" )
				  .arg( Filesystem::drumkit_file( sDrumkitDir ) ) );
		delete pDrumkit;
		return false;
	}

	INFOLOG( QString( "Drumkit [%1] is valid!" )
			 .arg( sDrumkitPath ) );
	
	return true;
}

Drumkit* CoreActionController::retrieveDrumkit( const QString& sDrumkitPath, bool* bIsCompressed, QString *sDrumkitDir, QString* sTemporaryFolder ) {

	Drumkit* pDrumkit = nullptr;

	*bIsCompressed = false;
	*sTemporaryFolder = "";
	*sDrumkitDir = "";

	QFileInfo sourceFileInfo( sDrumkitPath );

	if ( Filesystem::dir_readable( sDrumkitPath, true ) ) {

		// Providing the folder containing the drumkit
		pDrumkit = Drumkit::load( sDrumkitPath, false, false, true );
		*sDrumkitDir = sDrumkitPath;
		
	} else if ( sourceFileInfo.fileName() == Filesystem::drumkit_xml() &&
				Filesystem::file_readable( sDrumkitPath, true ) ) {

		// Providing the path of a drumkit.xml file within a drumkit
		// folder.
		pDrumkit = Drumkit::load_file( sDrumkitPath, false, false, true );
		*sDrumkitDir = sourceFileInfo.dir().absolutePath();
			
	} else if ( ( "." + sourceFileInfo.suffix() ) == Filesystem::drumkit_ext &&
				Filesystem::file_readable( sDrumkitPath, true ) ) {

		*bIsCompressed = true;
		
		// Temporary folder used to extract a compressed drumkit (
		// .h2drumkit ).
		QString sTemplateName( Filesystem::tmp_dir() + "/" +
							   sourceFileInfo.baseName() + "_XXXXXX" );
		QTemporaryDir tmpDir( sTemplateName );
		tmpDir.setAutoRemove( false );
		if ( ! tmpDir.isValid() ) {
			ERRORLOG( QString( "Unable to create temporary folder using template name [%1]" )
					  .arg( sTemplateName ) );
			return nullptr;
		}
		
		*sTemporaryFolder = tmpDir.path();

		// Providing the path to a compressed .h2drumkit file. It will
		// be extracted to a temporary folder and loaded from there.
		if ( ! Drumkit::install( sDrumkitPath, tmpDir.path(), true ) ) {
			ERRORLOG( QString( "Unabled to extract provided drumkit [%1] into [%2]" )
					  .arg( sDrumkitPath ).arg( tmpDir.path() ) );
			return nullptr;
		}

		// INFOLOG( QString( "Extracting drumkit [%1] into [%2]" )
		// 		 .arg( sDrumkitPath ).arg( tmpDir.path() ) );

		// The extracted folder is expected to contain a single
		// directory named as the drumkit itself. But some kits
		// deviate from the latter condition. So, we just use the
		// former one.
		QDir extractedDir( tmpDir.path() );
		QStringList extractedContent =
			extractedDir.entryList( QDir::AllEntries | QDir::NoDotAndDotDot );
		QStringList extractedFolders =
			extractedDir.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
		if ( ( extractedContent.size() != extractedFolders.size() ) ||
			 ( extractedFolders.size() != 1 ) ) {
			ERRORLOG( QString( "Unsupported content of [%1]. Expected a single folder within the archive containing all samples, metadata, as well as the drumkit.xml file. Instead:\n" )
					  .arg( sDrumkitPath ) );
			for ( const auto& sFile : extractedContent ) {
				ERRORLOG( sFile );
			}
			return nullptr;
		}

		*sDrumkitDir = tmpDir.path() + "/" + extractedFolders[0];
		
		pDrumkit = Drumkit::load( *sDrumkitDir, false, false, true );
		
	} else {
		ERRORLOG( QString( "Provided source path [%1] does not point to a Hydrogen drumkit" )
				  .arg( sDrumkitPath ) );
		return nullptr;
	}

	return pDrumkit;
}

bool CoreActionController::extractDrumkit( const QString& sDrumkitPath, const QString& sTargetDir ) {

	QString sTarget;
	if ( sTargetDir.isEmpty() ) {
		INFOLOG( QString( "Installing drumkit [%1]" ).arg( sDrumkitPath ) );
		sTarget = Filesystem::usr_drumkits_dir();
	} else {
		INFOLOG( QString( "Extracting drumkit [%1] to [%2]" )
				 .arg( sDrumkitPath ).arg( sTargetDir ) );
		sTarget = sTargetDir;
	}

	if ( ! Filesystem::path_usable( sTarget, true, false ) ) {
		ERRORLOG( QString( "Target dir [%1] is neither a writable folder nor can it be created." )
				  .arg( sTarget ) );
		return false;
	}

	QFileInfo sKitInfo( sDrumkitPath );
	if ( ! Filesystem::file_readable( sDrumkitPath, true ) ||
		 "." + sKitInfo.suffix() != Filesystem::drumkit_ext ) {
		ERRORLOG( QString( "Invalid drumkit path [%1]. Please provide an absolute path to a .h2drumkit file." )
				  .arg( sDrumkitPath ) );
		return false;
	}

	if ( ! Drumkit::install( sDrumkitPath, sTarget, true ) ) {
		ERRORLOG( QString( "Unabled to extract provided drumkit [%1] into [%2]" )
				  .arg( sDrumkitPath ).arg( sTarget ) );
		return false;
	}

	return true;
}
	
bool CoreActionController::locateToColumn( int nPatternGroup ) {

	if ( nPatternGroup < -1 ) {
		ERRORLOG( QString( "Provided column [%1] too low. Assigning -1 (indicating the beginning of a song without showing a cursor in the SongEditorPositionRuler) instead." )
				  .arg( nPatternGroup ) );
		nPatternGroup = -1;
	}
	
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	EventQueue::get_instance()->push_event( EVENT_METRONOME, 1 );
	long nTotalTick = pHydrogen->getTickForColumn( nPatternGroup );
	if ( nTotalTick < 0 ) {
		// There is no pattern inserted in the SongEditor.
		if ( pHydrogen->getMode() == Song::Mode::Song ) {
			DEBUGLOG( QString( "Obtained ticks [%1] are smaller than zero. No relocation done." )
					  .arg( nTotalTick ) );
			return false;
		} else {
			// In case of Pattern mode this is not a problem and we
			// will treat this case as the beginning of the song.
			nTotalTick = 0;
		}
	}

	return locateToTick( nTotalTick );
}

bool CoreActionController::locateToTick( long nTick, bool bWithJackBroadcast ) {

	const auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	return true;
}

bool CoreActionController::activateLoopMode( bool bActivate, bool bTriggerEvent ) {

	auto pSong = Hydrogen::get_instance()->getSong();
	pSong->setIsLoopEnabled( bActivate );
	pSong->setIsModified( true );
	
	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_LOOP_MODE_ACTIVATION, static_cast<int>( bActivate ) );
	}
	
	return true;
}

bool CoreActionController::relocate( int nPatternGroup ) {

	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->setPatternPos( nPatternGroup );
	pHydrogen->setTimelineBpm();
	
#ifdef H2CORE_HAVE_JACK
	auto pDriver = pHydrogen->getAudioOutput();

	if ( pHydrogen->haveJackTransport() &&
		 pDriver->m_transport.m_status != TransportInfo::ROLLING ) {
	long totalTick = pHydrogen->getTickForPosition( nPatternGroup );
	static_cast<JackAudioDriver*>(pDriver)->m_currentPos = 
		totalTick * pDriver->m_transport.m_fTickSize;
	}
#endif
	return true;
}
}
