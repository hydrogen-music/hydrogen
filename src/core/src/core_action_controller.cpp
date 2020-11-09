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

#include <hydrogen/audio_engine.h>
#include <hydrogen/core_action_controller.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/osc_server.h>
#include <hydrogen/midi_action.h>
#include <hydrogen/midi_map.h>

#include <hydrogen/IO/AlsaMidiDriver.h>
#include <hydrogen/IO/MidiOutput.h>
#include <hydrogen/IO/jack_audio_driver.h>

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
	pHydrogen->getSong()->set_volume( masterVolumeValue );
	
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
	InstrumentList *pInstrList = pSong->get_instrument_list();

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
	pHydrogen->getSong()->__is_muted = isMuted;
	
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
	InstrumentList *pInstrList = pSong->get_instrument_list();
	
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
	InstrumentList *pInstrList = pSong->get_instrument_list();

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
	InstrumentList *pInstrList = pSong->get_instrument_list();
	
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
	InstrumentList *pInstrList = pSong->get_instrument_list();
	
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
	InstrumentList *pInstrList = pSong->get_instrument_list();

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
	setMasterVolume( pSong->get_volume() );
	
	//PER-INSTRUMENT/STRIP STATES
	InstrumentList *pInstrList = pSong->get_instrument_list();
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
	setMasterIsMuted( Hydrogen::get_instance()->getSong()->__is_muted );
}

bool CoreActionController::newSong( const QString& songPath ) {
	
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getState() == STATE_PLAYING ) {
		// Stops recording, all queued MIDI notes, and the playback of
		// the audio driver.
		pHydrogen->sequencer_stop();
	}
	
	// Remove all BPM tags on the Timeline.
	pHydrogen->getTimeline()->deleteAllTempoMarkers();
	
	// Create an empty Song.
	auto pSong = Song::get_empty_song();
	
	// Check whether the provided path is valid.
	if ( !isSongPathValid( songPath ) ) {
		// isSongPathValid takes care of the error log message.

		return false;
	}
	
	pSong->set_filename( songPath );
	
	if ( pHydrogen->getActiveGUI() ) {
		
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

bool CoreActionController::openSong (const QString& songPath ) {
	
	auto pHydrogen = Hydrogen::get_instance();
 
	if ( pHydrogen->getState() == STATE_PLAYING ) {
		// Stops recording, all queued MIDI notes, and the playback of
		// the audio driver.
		pHydrogen->sequencer_stop();
	}
	
	// Remove all BPM tags on the Timeline.
	pHydrogen->getTimeline()->deleteAllTempoMarkers();
	
	// Check whether the provided path is valid.
	if ( !isSongPathValid( songPath ) ) {
		// isSongPathValid takes care of the error log message.
		return false;
	}
	
	QFileInfo songFileInfo = QFileInfo( songPath );
	if ( !songFileInfo.exists() ) {
		ERRORLOG( QString( "Selected song [%1] does not exist." )
				 .arg( songPath ) );
		return false;
	}
	
	// Create an empty Song.
	auto pSong = Song::load( songPath );
	
	if ( pSong == nullptr ) {
		ERRORLOG( QString( "Unable to open song [%1]." )
				  .arg( songPath ) );
		
		return false;
	}
	
	if ( pHydrogen->getActiveGUI() ) {
		
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

bool CoreActionController::saveSong() {
	
	auto pHydrogen = Hydrogen::get_instance();

	// Get the current Song which is about to be saved.
	auto pSong = pHydrogen->getSong();
	
	// Extract the path to the associate .h2song file.
	QString songPath = pSong->get_filename();
	
	if ( songPath.isEmpty() ) {
		ERRORLOG( "Unable to save song. Empty filename!" );
		return false;
	}
	
	// Actual saving
	bool saved = pSong->save( songPath );
	if ( !saved ) {
		ERRORLOG( QString( "Current song [%1] could not be saved!" )
				  .arg( songPath ) );
		return false;
	}
	
	// Update the status bar.
	if ( pHydrogen->getActiveGUI() ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 1 );
	}
	
	return true;
}

bool CoreActionController::saveSongAs( const QString& songPath ) {
	
	auto pHydrogen = Hydrogen::get_instance();
	
	// Get the current Song which is about to be saved.
	auto pSong = pHydrogen->getSong();
	
	// Check whether the provided path is valid.
	if ( !isSongPathValid( songPath ) ) {
		// isSongPathValid takes care of the error log message.
		return false;
	}
	
	if ( songPath.isEmpty() ) {
		ERRORLOG( "Unable to save song. Empty filename!" );
		return false;
	}
	
	// Actual saving
	bool saved = pSong->save( songPath );
	if ( !saved ) {
		ERRORLOG( QString( "Current song [%1] could not be saved!" )
				  .arg( songPath ) );
		return false;
	}
	
	// Update the status bar.
	if ( pHydrogen->getActiveGUI() ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 1 );
	}
	
	return true;
}

bool CoreActionController::quit() {
	EventQueue::get_instance()->push_event( EVENT_QUIT, 0 );
	
	return true;
}

bool CoreActionController::isSongPathValid( const QString& songPath ) {
	
	QFileInfo songFileInfo = QFileInfo( songPath );

	if ( !songFileInfo.isAbsolute() ) {
		ERRORLOG( QString( "Error: Unable to handle path [%1]. Please provide an absolute file path!" )
						.arg( songPath.toLocal8Bit().data() ));
		return false;
	}
	
	if ( songFileInfo.exists() ) {
		if ( !songFileInfo.isWritable() ) {
			ERRORLOG( QString( "Error: Unable to handle path [%1]. You must have permissions to write the file!" )
						.arg( songPath.toLocal8Bit().data() ));
			return false;
		}
	}
	
	if ( songFileInfo.suffix() != "h2song" ) {
		ERRORLOG( QString( "Error: Unable to handle path [%1]. The provided file must have the suffix '.h2song'!" )
					.arg( songPath.toLocal8Bit().data() ));
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
		pHydrogen->getSong()->set_mode( Song::SONG_MODE );
	} else {
		pHydrogen->getSong()->set_mode( Song::PATTERN_MODE );
	}
	
	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_SONG_MODE_ACTIVATION, static_cast<int>( bActivate ) );
	}
	
	return true;
}

bool CoreActionController::activateLoopMode( bool bActivate, bool bTriggerEvent ) {

	auto pSong = Hydrogen::get_instance()->getSong();
	pSong->set_loop_enabled( bActivate );
	pSong->set_is_modified( true );
	
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
