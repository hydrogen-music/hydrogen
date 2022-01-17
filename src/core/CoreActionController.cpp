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
#include <core/Preferences/Preferences.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Pattern.h>
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


CoreActionController::CoreActionController() : m_nDefaultMidiFeedbackChannel(0)
{
	//nothing
}

CoreActionController::~CoreActionController() {
	//nothing
}

bool CoreActionController::setMasterVolume( float masterVolumeValue )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	pHydrogen->getSong()->setVolume( masterVolumeValue );
	
#ifdef H2CORE_HAVE_OSC
	std::shared_ptr<Action> pFeedbackAction = std::make_shared<Action>( "MASTER_VOLUME_ABSOLUTE" );
	pFeedbackAction->setParameter2( QString("%1").arg( masterVolumeValue ) );
	OscServer::get_instance()->handleAction( pFeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionType( QString("MASTER_VOLUME_ABSOLUTE"));
	
	handleOutgoingControlChange( ccParamValue, (masterVolumeValue / 1.5) * 127 );

	return true;
}

bool CoreActionController::setStripVolume( int nStrip, float fVolumeValue, bool bSelectStrip )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nStrip );
	pInstr->set_volume( fVolumeValue );
	
#ifdef H2CORE_HAVE_OSC
	std::shared_ptr<Action> pFeedbackAction = std::make_shared<Action>( "STRIP_VOLUME_ABSOLUTE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
	pFeedbackAction->setParameter2( QString("%1").arg( fVolumeValue ) );
	OscServer::get_instance()->handleAction( pFeedbackAction );
#endif

	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("STRIP_VOLUME_ABSOLUTE"), QString("%1").arg( nStrip ) );
	
	handleOutgoingControlChange( ccParamValue, (fVolumeValue / 1.5) * 127 );
	pHydrogen->setIsModified( true );

	return true;
}

bool CoreActionController::setMetronomeIsActive( bool isActive )
{
	Preferences::get_instance()->m_bUseMetronome = isActive;
	
#ifdef H2CORE_HAVE_OSC
	std::shared_ptr<Action> pFeedbackAction = std::make_shared<Action>( "TOGGLE_METRONOME" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( (int) isActive ) );
	OscServer::get_instance()->handleAction( pFeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionType( QString("TOGGLE_METRONOME"));
	
	handleOutgoingControlChange( ccParamValue, (int) isActive * 127 );

	return true;
}

bool CoreActionController::setMasterIsMuted( bool isMuted )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	pHydrogen->getSong()->setIsMuted( isMuted );
	
#ifdef H2CORE_HAVE_OSC
	std::shared_ptr<Action> pFeedbackAction = std::make_shared<Action>( "MUTE_TOGGLE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( (int) isMuted ) );
	OscServer::get_instance()->handleAction( pFeedbackAction );
#endif

	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionType( QString("MUTE_TOGGLE") );

	handleOutgoingControlChange( ccParamValue, (int) isMuted * 127 );

	return true;
}

bool CoreActionController::toggleStripIsMuted(int nStrip)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if( pInstrList->is_valid_index( nStrip ))
	{
		auto pInstr = pInstrList->get( nStrip );
		
		if( pInstr ) {
			setStripIsMuted( nStrip , !pInstr->is_muted() );
		}
	}

	return true;
}

bool CoreActionController::setStripIsMuted( int nStrip, bool isMuted )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nStrip );
	pInstr->set_muted( isMuted );
	
#ifdef H2CORE_HAVE_OSC
	std::shared_ptr<Action> pFeedbackAction = std::make_shared<Action>( "STRIP_MUTE_TOGGLE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
	pFeedbackAction->setParameter2( QString("%1").arg( (int) isMuted ) );
	OscServer::get_instance()->handleAction( pFeedbackAction );
#endif

	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("STRIP_MUTE_TOGGLE"), QString("%1").arg( nStrip ) );
	
	handleOutgoingControlChange( ccParamValue, ((int) isMuted) * 127 );
	pHydrogen->setIsModified( true );

	return true;
}

bool CoreActionController::toggleStripIsSoloed( int nStrip )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if( pInstrList->is_valid_index( nStrip ))
	{
		auto pInstr = pInstrList->get( nStrip );
	
		if( pInstr ) {
			setStripIsSoloed( nStrip , !pInstr->is_soloed() );
		}
	}

	return true;
}

bool CoreActionController::setStripIsSoloed( int nStrip, bool isSoloed )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nStrip );
	pInstr->set_soloed( isSoloed );
	
#ifdef H2CORE_HAVE_OSC
	std::shared_ptr<Action> pFeedbackAction = std::make_shared<Action>( "STRIP_SOLO_TOGGLE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
	pFeedbackAction->setParameter2( QString("%1").arg( (int) isSoloed ) );
	OscServer::get_instance()->handleAction( pFeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("STRIP_SOLO_TOGGLE"), QString("%1").arg( nStrip ) );
	
	handleOutgoingControlChange( ccParamValue, ((int) isSoloed) * 127 );
	pHydrogen->setIsModified( true );

	return true;
}



bool CoreActionController::setStripPan( int nStrip, float fValue, bool bSelectStrip )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nStrip );
	pInstr->setPanWithRangeFrom0To1( fValue );

#ifdef H2CORE_HAVE_OSC
	std::shared_ptr<Action> pFeedbackAction = std::make_shared<Action>( "PAN_ABSOLUTE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
	pFeedbackAction->setParameter2( QString("%1").arg( fValue ) );
	OscServer::get_instance()->handleAction( pFeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("PAN_ABSOLUTE"), QString("%1").arg( nStrip ) );

	handleOutgoingControlChange( ccParamValue, fValue * 127 );
	pHydrogen->setIsModified( true );

	return true;
}

bool CoreActionController::setStripPanSym( int nStrip, float fValue, bool bSelectStrip )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}
	
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nStrip );
	pInstr->setPan( fValue );

#ifdef H2CORE_HAVE_OSC
	std::shared_ptr<Action> pFeedbackAction = std::make_shared<Action>( "PAN_ABSOLUTE_SYM" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
	pFeedbackAction->setParameter2( QString("%1").arg( fValue ) );
	OscServer::get_instance()->handleAction( pFeedbackAction );
#endif
	
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("PAN_ABSOLUTE"), QString("%1").arg( nStrip ) );
	handleOutgoingControlChange( ccParamValue, pInstr->getPanWithRangeFrom0To1() * 127 );
	pHydrogen->setIsModified( true );

	return true;
}

bool CoreActionController::handleOutgoingControlChange(int param, int value)
{
	Preferences *pPref = Preferences::get_instance();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	MidiOutput *pMidiDriver = pHydrogen->getMidiOutput();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	if(	pMidiDriver 
		&& pPref->m_bEnableMidiFeedback 
		&& param >= 0 ){
		pMidiDriver->handleOutgoingControlChange( param, value, m_nDefaultMidiFeedbackChannel );
	}

	return true;
}

bool CoreActionController::initExternalControlInterfaces()
{
	/*
	 * Push the current state of Hydrogen to the attached control interfaces (e.g. OSC clients)
	 */
	
	//MASTER_VOLUME_ABSOLUTE
	Hydrogen* pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
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

	return true;
}

bool CoreActionController::newSong( const QString& sSongPath ) {
	
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Playing ) {
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
 
	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Playing ) {
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
 
	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Playing ) {
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

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

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

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
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

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pHydrogen->setIsTimelineActivated( bActivate );
	
	if ( pHydrogen->getJackTimebaseState() == JackAudioDriver::Timebase::Slave ) {
		WARNINGLOG( QString( "Timeline usage was [%1] in the Preferences. But these changes won't have an effect as long as there is still an external JACK timebase master." )
					.arg( bActivate ? "enabled" : "disabled" ) );
	} else if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
		WARNINGLOG( QString( "Timeline usage was [%1] in the Preferences. But these changes won't have an effect as long as Pattern Mode is still activated." )
					.arg( bActivate ? "enabled" : "disabled" ) );
	}
	
	return true;
}

bool CoreActionController::addTempoMarker( int nPosition, float fBpm ) {
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	auto pTimeline = pHydrogen->getTimeline();
	pTimeline->deleteTempoMarker( nPosition );
	pTimeline->addTempoMarker( nPosition, fBpm );
	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->push_event( EVENT_TIMELINE_UPDATE, 0 );

	return true;
}

bool CoreActionController::deleteTempoMarker( int nPosition ) {
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	pHydrogen->getTimeline()->deleteTempoMarker( nPosition );
	pHydrogen->setIsModified( true );
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
	auto pHydrogen = Hydrogen::get_instance();
	
#ifdef H2CORE_HAVE_JACK
	if ( !pHydrogen->haveJackAudioDriver() ) {
		ERRORLOG( "Unable to (de)activate Jack timebase master. Please select the Jack driver first." );
		return false;
	}
	
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	if ( bActivate ) {
		Preferences::get_instance()->m_bJackMasterMode = Preferences::USE_JACK_TIME_MASTER;
		pHydrogen->onJackMaster();
	} else {
		Preferences::get_instance()->m_bJackMasterMode = Preferences::NO_JACK_TIME_MASTER;
		pHydrogen->offJackMaster();
	}
	pHydrogen->getAudioEngine()->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_JACK_TIMEBASE_STATE_CHANGED,
											static_cast<int>(pHydrogen->getJackTimebaseState()) );
	
	return true;
#else
	ERRORLOG( "Unable to (de)activate Jack timebase master. Your Hydrogen version was not compiled with jack support." );
	return false;
#endif
}

bool CoreActionController::activateSongMode( bool bActivate ) {

	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	pHydrogen->sequencer_stop();
	if ( bActivate && pHydrogen->getMode() != Song::Mode::Song ) {
		locateToColumn( 0 );
		pHydrogen->setMode( Song::Mode::Song );
	} else if ( ! bActivate && pHydrogen->getMode() != Song::Mode::Pattern ) {
		pHydrogen->setMode( Song::Mode::Pattern );
	}
	
	return true;
}

bool CoreActionController::activateLoopMode( bool bActivate, bool bTriggerEvent ) {

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	pSong->setIsLoopEnabled( bActivate );
	
	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_LOOP_MODE_ACTIVATION, static_cast<int>( bActivate ) );
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
	long nTotalTick = pAudioEngine->getTickForColumn( nPatternGroup );
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

	locateToFrame( static_cast<unsigned long>( nTotalTick * pAudioEngine->getTickSize() ) );

	// TODO: replace this by a boolian indicating a relocation in the
	// current cycle of the audio engine.
	// pHydrogen->setTimelineBpm();
	
	return true;
}

bool CoreActionController::locateToFrame( unsigned long nFrame, bool bWithJackBroadcast ) {

	const auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pDriver = pHydrogen->getAudioOutput();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );
	
	if ( pAudioEngine->getState() != AudioEngine::State::Playing ) {
		// Required to move the playhead when clicking e.g. fast
		// forward or the song editor ruler. The variables set in here
		// do not interfere with the realtime audio (playback of MIDI
		// events of virtual keyboard) and all other position
		// variables in the AudioEngine will be set properly with
		// respect to them by then.

		int nTotalTick = static_cast<int>(nFrame / pAudioEngine->getTickSize());
		int nPatternStartTick;
		int nColumn = pAudioEngine->getColumnForTick( nTotalTick, pHydrogen->getSong()->getIsLoopEnabled(), &nPatternStartTick );

		pAudioEngine->setColumn( nColumn );
		pAudioEngine->setPatternTickPosition( nTotalTick - nPatternStartTick );
	}
	pAudioEngine->locate( nFrame, bWithJackBroadcast );
	pAudioEngine->unlock();

#ifdef H2CORE_HAVE_JACK
	if ( pHydrogen->haveJackTransport() &&
		 pAudioEngine->getState() != AudioEngine::State::Playing ) {
		static_cast<JackAudioDriver*>(pDriver)->m_currentPos = nFrame;
	}
#endif
	return true;
}

bool CoreActionController::newPattern( const QString& sPatternName ) {
	auto pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	Pattern* pPattern = new Pattern( sPatternName );
	
	return setPattern( pPattern, pPatternList->size() );
}
bool CoreActionController::openPattern( const QString& sPath, int nPatternPosition ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	auto pPatternList = pSong->getPatternList();
	Pattern* pNewPattern = Pattern::load_file( sPath, pSong->getInstrumentList() );

	if ( pNewPattern == nullptr ) {
		ERRORLOG( QString( "Unable to loading the pattern [%1]" ).arg( sPath ) );
		return false;
	}

	if ( nPatternPosition == -1 ) {
		nPatternPosition = pPatternList->size();
	}

	return setPattern( pNewPattern, nPatternPosition );
}

bool CoreActionController::setPattern( Pattern* pPattern, int nPatternPosition ) {
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	auto pPatternList = pHydrogen->getSong()->getPatternList();

	// Check whether the name of the new pattern is unique.
	if ( !pPatternList->check_name( pPattern->get_name() ) ){
		pPattern->set_name( pPatternList->find_unused_pattern_name( pPattern->get_name() ) );
	}

	pPatternList->insert( nPatternPosition, pPattern );
	pHydrogen->setSelectedPatternNumber( nPatternPosition );
	pHydrogen->setIsModified( true );
	
	// Update the SongEditor.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG_EDITOR, 0 );
	}
	return true;
}

bool CoreActionController::removePattern( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	auto pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	int nPreviousPatternNumber = pHydrogen->getSelectedPatternNumber();
	auto pPattern = pPatternList->get( nPatternNumber );

	if ( nPatternNumber == nPreviousPatternNumber ) {
		pHydrogen->setSelectedPatternNumber( std::max( 0, nPatternNumber - 1 ) );
	}

	pPatternList->del( pPattern );
	delete pPattern;
	pHydrogen->setIsModified( true );

	// Update the SongEditor.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG_EDITOR, 0 );
	}
	return true;
}

bool CoreActionController::toggleGridCell( int nColumn, int nRow ){
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	auto pSong = pHydrogen->getSong();
	auto pPatternList = pSong->getPatternList();
	std::vector<PatternList*>* pColumns = pSong->getPatternGroupVector();

	if ( nRow < 0 || nRow > pPatternList->size() ) {
		ERRORLOG( QString( "Provided row [%1] is out of bound [0,%2]" )
				  .arg( nRow ).arg( pPatternList->size() ) );
		return false;
	}
	
	auto pNewPattern = pPatternList->get( nRow );
	if ( pNewPattern == nullptr ) {
		ERRORLOG( QString( "Unable to obtain Pattern in row [%1]." )
				  .arg( nRow ) );

		return false;
	}

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	if ( nColumn >= 0 && nColumn < pColumns->size() ) {
		PatternList *pColumn = ( *pColumns )[ nColumn ];
		auto pPattern = pColumn->del( pNewPattern );
		if ( pPattern == nullptr ) {
			// No pattern in this row. Let's add it.
			pColumn->add( pNewPattern );
		} else {
			// There was already a pattern present and we removed it.
			// Ensure that there are no empty columns at the end of
			// the song.
			for ( int ii = pColumns->size() - 1; ii >= 0; ii-- ) {
				PatternList *pColumn = ( *pColumns )[ ii ];
				if ( pColumn->size() == 0 ) {
					pColumns->erase( pColumns->begin() + ii );
					delete pColumn;
				} else {
					break;
				}
			}
		}
	} else if ( nColumn >= pColumns->size() ) {
		// We need to add some new columns..
		PatternList *pColumn;

		for ( int ii = 0; nColumn - pColumns->size() + 1; ii++ ) {
			pColumn = new PatternList();
			pColumns->push_back( pColumn );
		}
		pColumn->add( pNewPattern );
	} else {
		// nColumn < 0
		ERRORLOG( QString( "Provided column [%1] is out of bound [0,%2]" )
				  .arg( nColumn ).arg( pColumns->size() ) );
		return false;
	}
	
	pHydrogen->setIsModified( true );
	pHydrogen->getAudioEngine()->unlock();

	// Update the SongEditor.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG_EDITOR, 0 );
	}

	return true;
}

}
