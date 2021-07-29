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

#include <core/AudioEngine/AudioEngine.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Preferences.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Instrument.h>
#include "core/OscServer.h"
#include <core/MidiAction.h>
#include "core/MidiMap.h"

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
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nStrip );
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if( pInstrList->is_valid_index( nStrip ))
	{
		auto pInstr = pInstrList->get( nStrip );
		
		if( pInstr ) {
			setStripIsMuted( nStrip , !pInstr->is_muted() );
		}
	}
}

void CoreActionController::setStripIsMuted( int nStrip, bool isMuted )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nStrip );
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if( pInstrList->is_valid_index( nStrip ))
	{
		auto pInstr = pInstrList->get( nStrip );
	
		if( pInstr ) {
			setStripIsSoloed( nStrip , !pInstr->is_soloed() );
		}
	}
}

void CoreActionController::setStripIsSoloed( int nStrip, bool isSoloed )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nStrip );
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



void CoreActionController::setStripPan( int nStrip, float fValue, bool bSelectStrip )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nStrip );
	pInstr->setPanWithRangeFrom0To1( fValue );

#ifdef H2CORE_HAVE_OSC
	Action FeedbackAction( "PAN_ABSOLUTE" );
	
	FeedbackAction.setParameter1( QString("%1").arg( nStrip + 1 ) );
	FeedbackAction.setParameter2( QString("%1").arg( fValue ) );
	OscServer::get_instance()->handleAction( &FeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("PAN_ABSOLUTE"), QString("%1").arg( nStrip ) );

	handleOutgoingControlChange( ccParamValue, fValue * 127 );
}

void CoreActionController::setStripPanSym( int nStrip, float fValue, bool bSelectStrip )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nStrip );
	pInstr->setPan( fValue );

#ifdef H2CORE_HAVE_OSC
	Action FeedbackAction( "PAN_ABSOLUTE_SYM" );
	
	FeedbackAction.setParameter1( QString("%1").arg( nStrip + 1 ) );
	FeedbackAction.setParameter2( QString("%1").arg( fValue ) );
	OscServer::get_instance()->handleAction( &FeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("PAN_ABSOLUTE"), QString("%1").arg( nStrip ) );
	handleOutgoingControlChange( ccParamValue, pInstr->getPanWithRangeFrom0To1() * 127 );
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	setMasterVolume( pSong->getVolume() );
	
	//PER-INSTRUMENT/STRIP STATES
	InstrumentList *pInstrList = pSong->getInstrumentList();
	for(int i=0; i < pInstrList->size(); i++){
		
			//STRIP_VOLUME_ABSOLUTE
			auto pInstr = pInstrList->get( i );
			setStripVolume( i, pInstr->get_volume(), false );

			//PAN_ABSOLUTE
			float fValue = pInstr->getPanWithRangeFrom0To1();
			setStripPan( i, fValue, false );
			
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
	
	// Create an empty Song.
	auto pSong = Song::getEmptySong();
	
	// Check whether the provided path is valid.
	if ( !isSongPathValid( sSongPath ) ) {
		// isSongPathValid takes care of the error log message.

		return false;
	}

	// Remove all BPM tags on the Timeline.
	pHydrogen->getTimeline()->deleteAllTempoMarkers();
	pHydrogen->getTimeline()->deleteAllTags();

	if ( pHydrogen->isUnderSessionManagement() ) {
		pHydrogen->restartDrivers();
	}		

	pSong->setFilename( sSongPath );

	pHydrogen->setSong( pSong );
	
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 0 );
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

bool CoreActionController::openSong( std::shared_ptr<Song> pSong ) {
	
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

bool CoreActionController::setSong( std::shared_ptr<Song> pSong ) {

	auto pHydrogen = Hydrogen::get_instance();
	
	// Remove all BPM markers and tags on the Timeline.
	pHydrogen->getTimeline()->deleteAllTempoMarkers();
	pHydrogen->getTimeline()->deleteAllTags();

	// Update the Song.
	pHydrogen->setSong( pSong );
		
	if ( pHydrogen->isUnderSessionManagement() ) {
		pHydrogen->restartDrivers();
	} else {
		// Add the new loaded song in the "last used song" vector.
		// This behavior is prohibited under session management. Only
		// songs open during normal runs will be listed.
		Preferences::get_instance()->insertRecentFile( pSong->getFilename() );
	}

	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 0 );
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
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 1 );
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
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 1 );
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
			EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 2 );
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
	
	Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );
	if ( bActivate ) {
		Preferences::get_instance()->m_bJackTransportMode = Preferences::USE_JACK_TRANSPORT;
	} else {
		Preferences::get_instance()->m_bJackTransportMode = Preferences::NO_JACK_TRANSPORT;
	}
	Hydrogen::get_instance()->getAudioEngine()->unlock();
	
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
	
	Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );
	if ( bActivate ) {
		Preferences::get_instance()->m_bJackMasterMode = Preferences::USE_JACK_TIME_MASTER;
		Hydrogen::get_instance()->onJackMaster();
	} else {
		Preferences::get_instance()->m_bJackMasterMode = Preferences::NO_JACK_TIME_MASTER;
		Hydrogen::get_instance()->offJackMaster();
	}
	Hydrogen::get_instance()->getAudioEngine()->unlock();
	
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
		locateToColumn( 0 );
		pHydrogen->getSong()->setMode( Song::SONG_MODE );
	} else {
		pHydrogen->getSong()->setMode( Song::PATTERN_MODE );
	}
	
	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_SONG_MODE_ACTIVATION, static_cast<int>( bActivate ) );
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

bool CoreActionController::locateToColumn( int nPatternGroup ) {

	if ( nPatternGroup < -1 ) {
		ERRORLOG( QString( "Provided column [%1] too low. Assigning -1 instead." )
				  .arg( nPatternGroup ) );
		nPatternGroup = -1;
	}
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	pAudioEngine->lock( RIGHT_HERE );

	EventQueue::get_instance()->push_event( EVENT_METRONOME, 1 );
	long nTotalTick = pAudioEngine->getTickForColumn( nPatternGroup );
	if ( nTotalTick < 0 ) {
		// TODO: why not locating to the beginning in here?
		DEBUGLOG( QString( "Obtained ticks [%1] are smaller than zero. No relocation done." )
				  .arg( nTotalTick ) );
		pAudioEngine->unlock();
		return false;
	}

	if ( pHydrogen->getState() != STATE_PLAYING ) {
		// find pattern immediately when not playing
		pAudioEngine->setSongPos( nPatternGroup );
		pAudioEngine->setPatternTickPosition( 0 );
	}
	pAudioEngine->unlock();

	locateToFrame( static_cast<unsigned long>( nTotalTick * pAudioEngine->getTickSize() ) );

	pHydrogen->setTimelineBpm();
	
	return true;
}

bool CoreActionController::locateToFrame( unsigned long nFrame ) {

	const auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pDriver = pHydrogen->getAudioOutput();

	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->locate( nFrame );
	pAudioEngine->unlock();

	if ( pHydrogen->haveJackTransport() &&
		 pAudioEngine->getStatus() != TransportInfo::Status::Rolling ) {
		static_cast<JackAudioDriver*>(pDriver)->m_currentPos = nFrame;
	}
	return true;
}
}
