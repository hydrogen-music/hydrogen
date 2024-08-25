/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <QDir>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include "core/OscServer.h"
#include <core/MidiAction.h>
#include "core/MidiMap.h"
#include <core/Helpers/Xml.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <core/IO/AlsaMidiDriver.h>
#include <core/IO/MidiOutput.h>
#include <core/IO/JackAudioDriver.h>

#ifdef H2CORE_HAVE_OSC
#include <core/NsmClient.h>
#endif

namespace H2Core
{

#define ASSERT_HYDROGEN assert( pHydrogen ); \
	if ( pHydrogen == nullptr ) {            \
		ERRORLOG( "Core not ready yet!" );   \
		return false;                        \
	}

bool CoreActionController::setMasterVolume( float fMasterVolumeValue )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	pSong->setVolume( fMasterVolumeValue );
	
	return sendMasterVolumeFeedback();
}

bool CoreActionController::setStripVolume( int nStrip, float fVolumeValue, bool bSelectStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {
	
		pInstr->set_volume( fVolumeValue );
	
		if ( bSelectStrip ) {
			pHydrogen->setSelectedInstrumentNumber( nStrip );
		}
	
		pHydrogen->setIsModified( true );

		return sendStripVolumeFeedback( nStrip );
	}

	return false;
}

bool CoreActionController::setInstrumentPitch( int nInstrument, float fValue ){
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "no drumkit" );
		return false;
	}
	auto pInstrumentList = pDrumkit->getInstruments();
	auto pInstrument = pInstrumentList->get( nInstrument );
	if( pInstrument == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" )
				  .arg( nInstrument ) );
		return false;
	}

	pInstrument->set_pitch_offset( fValue );
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nInstrument );
	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED,
											nInstrument );

	return true;
}

bool CoreActionController::setMetronomeIsActive( bool isActive )
{
	Preferences::get_instance()->m_bUseMetronome = isActive;

	return sendMetronomeIsActiveFeedback();
}

bool CoreActionController::setMasterIsMuted( bool bIsMuted )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	pSong->setIsMuted( bIsMuted );
	
	pHydrogen->setIsModified( true );

	return sendMasterIsMutedFeedback();
}

bool CoreActionController::toggleStripIsMuted( int nStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}
	
	return setStripIsMuted( nStrip, !pInstr->is_muted() );
}

bool CoreActionController::setStripIsMuted( int nStrip, bool bIsMuted )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {
		pInstr->set_muted( bIsMuted );

		EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nStrip );
	
		pHydrogen->setIsModified( true );

		return sendStripIsMutedFeedback( nStrip );
	}

	return false;
}

bool CoreActionController::toggleStripIsSoloed( int nStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
		auto pInstr = getStrip( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}
	
	return setStripIsSoloed( nStrip, !pInstr->is_soloed() );
}

bool CoreActionController::setStripIsSoloed( int nStrip, bool isSoloed )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {
	
		pInstr->set_soloed( isSoloed );

		EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nStrip );
	
		pHydrogen->setIsModified( true );

		return sendStripIsSoloedFeedback( nStrip );
	}

	return false;
}

bool CoreActionController::setStripPan( int nStrip, float fValue, bool bSelectStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {
	
		pInstr->setPanWithRangeFrom0To1( fValue );
		
		EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nStrip );
		
		pHydrogen->setIsModified( true );
		
		if ( bSelectStrip ) {
			pHydrogen->setSelectedInstrumentNumber( nStrip );
		}

		return sendStripPanFeedback( nStrip );
	}

	return false;
}


bool CoreActionController::setStripPanSym( int nStrip, float fValue, bool bSelectStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {
	
		pInstr->setPan( fValue );
		
		EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nStrip );
		
		pHydrogen->setIsModified( true );
		
		if ( bSelectStrip ) {
			pHydrogen->setSelectedInstrumentNumber( nStrip );
		}

		return sendStripPanFeedback( nStrip );
	}

	return false;
}

bool CoreActionController::sendMasterVolumeFeedback() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
		
	float fMasterVolume = pSong->getVolume();
	
#ifdef H2CORE_HAVE_OSC
	if ( Preferences::get_instance()->getOscFeedbackEnabled() ) {
		
		std::shared_ptr<Action> pFeedbackAction =
			std::make_shared<Action>( "MASTER_VOLUME_ABSOLUTE" );
		
		pFeedbackAction->setValue( QString("%1")
								   .arg( fMasterVolume ) );
		OscServer::get_instance()->handleAction( pFeedbackAction );
	}
#endif
	
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();
	
	auto ccParamValues = pMidiMap->findCCValuesByActionType( QString("MASTER_VOLUME_ABSOLUTE"));
	
	return handleOutgoingControlChanges( ccParamValues, (fMasterVolume / 1.5) * 127 );
}

bool CoreActionController::sendStripVolumeFeedback( int nStrip ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {

		float fStripVolume = pInstr->get_volume();
		
#ifdef H2CORE_HAVE_OSC
		if ( Preferences::get_instance()->getOscFeedbackEnabled() ) {
		
			std::shared_ptr<Action> pFeedbackAction =
				std::make_shared<Action>( "STRIP_VOLUME_ABSOLUTE" );
		
			pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
			pFeedbackAction->setValue( QString("%1").arg( fStripVolume ) );
			OscServer::get_instance()->handleAction( pFeedbackAction );
		}
#endif

		const auto pMidiMap = Preferences::get_instance()->getMidiMap();
	
		auto ccParamValues = pMidiMap->findCCValuesByActionParam1( QString("STRIP_VOLUME_ABSOLUTE"),
																   QString("%1").arg( nStrip ) );
	
		return handleOutgoingControlChanges( ccParamValues, (fStripVolume / 1.5) * 127 );
	}
	
	return false;
}

bool CoreActionController::sendMetronomeIsActiveFeedback() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	const auto pPref = Preferences::get_instance();
	
#ifdef H2CORE_HAVE_OSC
	if ( pPref->getOscFeedbackEnabled() ) {
		std::shared_ptr<Action> pFeedbackAction =
			std::make_shared<Action>( "TOGGLE_METRONOME" );
		
		pFeedbackAction->setParameter1( QString("%1")
										.arg( static_cast<int>(pPref->m_bUseMetronome) ) );
		OscServer::get_instance()->handleAction( pFeedbackAction );
	}
#endif
	
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();
	
	auto ccParamValues = pMidiMap->findCCValuesByActionType( QString("TOGGLE_METRONOME"));
	
	return handleOutgoingControlChanges( ccParamValues,
										 static_cast<int>(pPref->m_bUseMetronome) * 127 );
}

bool CoreActionController::sendMasterIsMutedFeedback() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
#ifdef H2CORE_HAVE_OSC
	if ( Preferences::get_instance()->getOscFeedbackEnabled() ) {
		std::shared_ptr<Action> pFeedbackAction =
			std::make_shared<Action>( "MUTE_TOGGLE" );
		
		pFeedbackAction->setParameter1( QString("%1")
										.arg( static_cast<int>(pSong->getIsMuted()) ) );
		OscServer::get_instance()->handleAction( pFeedbackAction );
	}
#endif

	const auto pMidiMap = Preferences::get_instance()->getMidiMap();

	auto ccParamValues = pMidiMap->findCCValuesByActionType( QString("MUTE_TOGGLE") );

	return handleOutgoingControlChanges( ccParamValues,
										 static_cast<int>(pSong->getIsMuted()) * 127 );
}

bool CoreActionController::sendStripIsMutedFeedback( int nStrip ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {
	
#ifdef H2CORE_HAVE_OSC
		if ( Preferences::get_instance()->getOscFeedbackEnabled() ) {
			std::shared_ptr<Action> pFeedbackAction =
				std::make_shared<Action>( "STRIP_MUTE_TOGGLE" );
		
			pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
			pFeedbackAction->setValue( QString("%1")
											.arg( static_cast<int>(pInstr->is_muted()) ) );
			OscServer::get_instance()->handleAction( pFeedbackAction );
		}
#endif

		const auto pMidiMap = Preferences::get_instance()->getMidiMap();
	
		auto ccParamValues = pMidiMap->findCCValuesByActionParam1( QString("STRIP_MUTE_TOGGLE"),
																   QString("%1").arg( nStrip ) );
	
		return handleOutgoingControlChanges( ccParamValues,
											 static_cast<int>(pInstr->is_muted()) * 127 );
	}
	
	return false;
}

bool CoreActionController::sendStripIsSoloedFeedback( int nStrip ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {
	
#ifdef H2CORE_HAVE_OSC
		if ( Preferences::get_instance()->getOscFeedbackEnabled() ) {
			std::shared_ptr<Action> pFeedbackAction =
				std::make_shared<Action>( "STRIP_SOLO_TOGGLE" );
		
			pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
			pFeedbackAction->setValue( QString("%1")
									   .arg( static_cast<int>(pInstr->is_soloed()) ) );
			OscServer::get_instance()->handleAction( pFeedbackAction );
		}
#endif

		const auto pMidiMap = Preferences::get_instance()->getMidiMap();
		auto ccParamValues = pMidiMap->findCCValuesByActionParam1( QString("STRIP_SOLO_TOGGLE"),
																   QString("%1").arg( nStrip ) );
	
		return handleOutgoingControlChanges( ccParamValues,
											 static_cast<int>(pInstr->is_soloed()) * 127 );
	}

	return false;
}

bool CoreActionController::sendStripPanFeedback( int nStrip ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {

#ifdef H2CORE_HAVE_OSC
		if ( Preferences::get_instance()->getOscFeedbackEnabled() ) {
			std::shared_ptr<Action> pFeedbackAction =
				std::make_shared<Action>( "PAN_ABSOLUTE" );
		
			pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
			pFeedbackAction->setValue( QString("%1")
									   .arg( pInstr->getPanWithRangeFrom0To1() ) );
			OscServer::get_instance()->handleAction( pFeedbackAction );
		}
#endif
	
		const auto pMidiMap = Preferences::get_instance()->getMidiMap();
		auto ccParamValues = pMidiMap->findCCValuesByActionParam1( QString("PAN_ABSOLUTE"),
																   QString("%1").arg( nStrip ) );

		return handleOutgoingControlChanges( ccParamValues,
											 pInstr->getPanWithRangeFrom0To1() * 127 );
	}

	return false;
}

bool CoreActionController::handleOutgoingControlChanges( const std::vector<int>& params,
														 int nValue)
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	const auto pPref = Preferences::get_instance();
	MidiOutput *pMidiDriver = pHydrogen->getMidiOutput();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	for ( const auto& param : params ) {
		if ( pMidiDriver != nullptr &&
			 pPref->m_bEnableMidiFeedback && param >= 0 ){
			// For now the MIDI feedback channel is always 0.
			pMidiDriver->handleOutgoingControlChange( param, nValue, 0 );
		}
	}

	return true;
}

std::shared_ptr<Instrument> CoreActionController::getStrip( int nStrip ) {
	auto pHydrogen = Hydrogen::get_instance();
	assert( pHydrogen );
	if ( pHydrogen == nullptr ) {
		ERRORLOG( "Core not ready yet!" );
		return nullptr;
	}

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return nullptr;
	}

	auto pInstr = pSong->getDrumkit()->getInstruments()->get( nStrip );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" ).arg( nStrip ) );
		return nullptr;
	}

	return pInstr;
}

bool CoreActionController::initExternalControlInterfaces()
{
	/*
	 * Push the current state of Hydrogen to the attached control interfaces (e.g. OSC clients)
	 */
	
	//MASTER_VOLUME_ABSOLUTE
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	sendMasterVolumeFeedback();
	
	//PER-INSTRUMENT/STRIP STATES
	auto pInstrList = pSong->getDrumkit()->getInstruments();
	for ( int ii = 0; ii < pInstrList->size(); ii++){
		auto pInstr = pInstrList->get( ii );
		if ( pInstr != nullptr ) {
		
			//STRIP_VOLUME_ABSOLUTE
			sendStripVolumeFeedback( ii );

			//PAN_ABSOLUTE
			sendStripPanFeedback( ii );
			
			//STRIP_MUTE_TOGGLE
			sendStripIsMutedFeedback( ii );
			
			//SOLO
			sendStripIsSoloedFeedback( ii );
		}
	}
	
	//TOGGLE_METRONOME
	sendMetronomeIsActiveFeedback();
	
	//MUTE_TOGGLE
	sendMasterIsMutedFeedback();
	
	return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

std::shared_ptr<Song> CoreActionController::loadSong( const QString& sPath,
													  const QString& sRecoverPath ) {
	auto pHydrogen = Hydrogen::get_instance();
	assert( pHydrogen );
	if ( pHydrogen == nullptr ) {
		ERRORLOG( "Core not ready yet!" );
		return nullptr;
	}

	// Check whether the provided path is valid.
	if ( sPath != Filesystem::empty_path( Filesystem::Type::Song ) &&
		 ! Filesystem::isPathValid( Filesystem::Type::Song, sPath, true ) ) {
		// Filesystem::isPathValid takes care of the error log message.
		return nullptr;
	}

	std::shared_ptr<Song> pSong;
	if ( ! sRecoverPath.isEmpty() && Filesystem::isPathValid(
			 Filesystem::Type::Song, sRecoverPath, true ) ) {
		// Use an autosave file to load the playlist
		pSong = Song::load( sRecoverPath );
		if ( pSong != nullptr ) {
			pSong->setFilename( sPath );
		} else {
			ERRORLOG( QString( "Unable to recover changes from [%1]. Loading [%2] instead." )
					  .arg( sRecoverPath ).arg( sPath ) );
		}
	}

	if ( pSong == nullptr ) {
		pSong = Song::load( sPath );
	}

	if ( pSong == nullptr ) {
		ERRORLOG( QString( "Unable to open song [%1]." ).arg( sPath ) );
		return nullptr;
	}
	
	return pSong;
}

bool CoreActionController::setSong( std::shared_ptr<Song> pSong ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return false;
	}

	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Playing ) {
		// Stops recording, all queued MIDI notes, and the playback of
		// the audio driver.
		pHydrogen->sequencerStop();
	}

	// Update the Song.
	pHydrogen->setSong( pSong );
		
	if ( pHydrogen->isUnderSessionManagement() ) {
		pHydrogen->restartDrivers();
	}
	else {
		// Add the new loaded song in the "last used song" vector.
		// This behavior is prohibited under session management. Only
		// songs open during normal runs will be listed. In addition,
		// empty songs - created and set when hitting "New Song" in
		// the main menu - aren't listed either.

		if ( pSong->getFilename() ==
			 Filesystem::empty_path( Filesystem::Type::Song ) ) {
			// To indicate that the user closed the previous song in favor of a
			// new one, we store an empty string. This way the changes from the
			// empty song can be recovered.
			Preferences::get_instance()->setLastSongFilename( "" );
		}
		else {
			insertRecentFile( pSong->getFilename() );
			Preferences::get_instance()->setLastSongFilename( pSong->getFilename() );
		}
	}
		
	EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 0 );

	// In case the song is read-only, autosave won't work.
	if ( ! Filesystem::file_writable( pSong->getFilename() ) ) {
		WARNINGLOG( QString( "You don't have permissions to write to the song found in path [%1]. It will be opened as read-only (no autosave)." )
					.arg( pSong->getFilename() ));
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 2 );
	}

	// As we just set a fresh song, we can mark it not modified
	pHydrogen->setIsModified( false );
	
	return true;
}

bool CoreActionController::saveSong() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	// Extract the path to the associate .h2song file.
	QString sSongPath = pSong->getFilename();
	
	if ( sSongPath.isEmpty() ) {
		ERRORLOG( "Unable to save song. Empty filename!" );
		return false;
	}

	// Actual saving
	bool bSaved = pSong->save( sSongPath );
	if ( ! bSaved ) {
		ERRORLOG( QString( "Current song [%1] could not be saved!" )
				  .arg( sSongPath ) );
		return false;
	}
	
	// Update the status bar.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::headless ) {
		EventQueue::get_instance()->push_event( EVENT_UPDATE_SONG, 1 );
	}
	
	return true;
}

bool CoreActionController::saveSongAs( const QString& sNewFilename ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	// Check whether the provided path is valid.
	if ( !Filesystem::isPathValid(
			 Filesystem::Type::Song, sNewFilename ) ) {
		// Filesystem::isPathValid takes care of the error log message.
		return false;
	}
	if ( ! Filesystem::file_writable( sNewFilename ) ) {
		ERRORLOG( QString( "Song can not be written to read-only location [%1]" )
				  .arg( sNewFilename ) );
		return false;
	}

	QString sPreviousFilename( pSong->getFilename() );
	pSong->setFilename( sNewFilename );
	
	// Actual saving
	if ( ! saveSong() ) {
		return false;
	}

	// Update the recentFiles list by replacing the former file name
	// with the new one.
	insertRecentFile( sNewFilename );
	if ( ! pHydrogen->isUnderSessionManagement() ) {
		Preferences::get_instance()->setLastSongFilename( pSong->getFilename() );
	}
	
	return true;
}

std::shared_ptr<Preferences> CoreActionController::loadPreferences( const QString& sPath ) {
	return Preferences::load( sPath, false );
}

bool CoreActionController::setPreferences( std::shared_ptr<Preferences> pPreferences ) {
	if ( pPreferences == nullptr ) {
		ERRORLOG( "invalid preferences" );
		return false;
	}

	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();

	Preferences::get_instance()->replaceInstance( pPreferences );

	pAudioEngine->getMetronomeInstrument()->set_volume(
		pPreferences->m_fMetronomeVolume );

	InstrumentComponent::setMaxLayers( pPreferences->getMaxLayers() );

	// If the GUI is active, we have to update it to reflect the
	// changes in the preferences.
	if ( pHydrogen->getGUIState() == H2Core::Hydrogen::GUIState::ready ) {
		H2Core::EventQueue::get_instance()->push_event(
			H2Core::EVENT_UPDATE_PREFERENCES, 1 );
	}

	return true;
}

bool CoreActionController::savePreferences() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::headless ) {
		// Update the status bar and let the GUI save the preferences
		// (after writing its current settings to disk).
		EventQueue::get_instance()->push_event( EVENT_UPDATE_PREFERENCES, 0 );
		return true;
	}
	
	return Preferences::get_instance()->save();
}

bool CoreActionController::quit() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	EventQueue::get_instance()->push_event( EVENT_QUIT, 0 );

	return true;
}

bool CoreActionController::toggleTimeline() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	if ( pHydrogen->isTimelineEnabled() ) {
		activateTimeline( false );
	} else {
		activateTimeline( true );
	}

	return true;
}

bool CoreActionController::activateTimeline( bool bActivate ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pHydrogen->setIsTimelineActivated( bActivate );
	
	if ( pHydrogen->getJackTimebaseState() ==
		 JackAudioDriver::Timebase::Listener ) {
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
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pTimeline = pHydrogen->getTimeline();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );

	pTimeline->deleteTempoMarker( nPosition );
	pTimeline->addTempoMarker( nPosition, fBpm );
	pHydrogen->getAudioEngine()->handleTimelineChange();

	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->push_event( EVENT_TIMELINE_UPDATE, 0 );

	return true;
}

bool CoreActionController::deleteTempoMarker( int nPosition ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );
	
	pHydrogen->getTimeline()->deleteTempoMarker( nPosition );
	pHydrogen->getAudioEngine()->handleTimelineChange();

	pAudioEngine->unlock();
	
	pHydrogen->setIsModified( true );
	EventQueue::get_instance()->push_event( EVENT_TIMELINE_UPDATE, 0 );

	return true;
}

bool CoreActionController::addTag( int nPosition, const QString& sText ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pTimeline = pHydrogen->getTimeline();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pTimeline->deleteTag( nPosition );
	pTimeline->addTag( nPosition, sText );

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->push_event( EVENT_TIMELINE_UPDATE, 0 );

	return true;
}

bool CoreActionController::deleteTag( int nPosition ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pHydrogen->getTimeline()->deleteTag( nPosition );
	
	pHydrogen->setIsModified( true );
	EventQueue::get_instance()->push_event( EVENT_TIMELINE_UPDATE, 0 );

	return true;
}

bool CoreActionController::toggleJackTransport() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	if ( Preferences::get_instance()->m_nJackTransportMode ==
		 Preferences::USE_JACK_TRANSPORT ) {
		activateJackTransport( false );
	} else {
		activateJackTransport( true );
	}

	return true;
}

bool CoreActionController::activateJackTransport( bool bActivate ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

#ifdef H2CORE_HAVE_JACK
	if ( !pHydrogen->hasJackAudioDriver() ) {
		ERRORLOG( "Unable to (de)activate Jack transport. Please select the Jack driver first." );
		return false;
	}
	
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	if ( bActivate ) {
		Preferences::get_instance()->m_nJackTransportMode = Preferences::USE_JACK_TRANSPORT;
	} else {
		Preferences::get_instance()->m_nJackTransportMode = Preferences::NO_JACK_TRANSPORT;
	}
	pHydrogen->getAudioEngine()->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_JACK_TRANSPORT_ACTIVATION, static_cast<int>( bActivate ) );
	
	return true;
#else
	ERRORLOG( "Unable to (de)activate Jack transport. Your Hydrogen version was not compiled with jack support." );
	return false;
#endif
}

bool CoreActionController::toggleJackTimebaseMaster() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	if ( Preferences::get_instance()->m_bJackMasterMode ==
		 Preferences::USE_JACK_TIME_MASTER ) {
		activateJackTimebaseMaster( false );
	} else {
		activateJackTimebaseMaster( true );
	}

	return true;
}

bool CoreActionController::activateJackTimebaseMaster( bool bActivate ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

#ifdef H2CORE_HAVE_JACK
	if ( !pHydrogen->hasJackAudioDriver() ) {
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

bool CoreActionController::toggleSongMode() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	if ( pHydrogen->getMode() == Song::Mode::Song ) {
		activateSongMode( false );
	} else {
		activateSongMode( true );
	}

	return true;
}

bool CoreActionController::activateSongMode( bool bActivate ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( !( bActivate && pHydrogen->getMode() != Song::Mode::Song ) &&
		 ! ( ! bActivate && pHydrogen->getMode() != Song::Mode::Pattern ) ) {
		// No changes.
		return true;
	}		
	
	pHydrogen->sequencerStop();

	pAudioEngine->lock( RIGHT_HERE );

	if ( bActivate && pHydrogen->getMode() != Song::Mode::Song ) {
		pHydrogen->setMode( Song::Mode::Song );
	}
	else if ( ! bActivate && pHydrogen->getMode() != Song::Mode::Pattern ) {
		pHydrogen->setMode( Song::Mode::Pattern );
	}
	
	pAudioEngine->handleSongModeChanged();

	pAudioEngine->unlock();
	
	return true;
}

bool CoreActionController::toggleLoopMode() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( pSong->getLoopMode() != Song::LoopMode::Enabled ) {
		return activateLoopMode( true );
	}
	else {
		return activateLoopMode( false );
	}
}

bool CoreActionController::activateLoopMode( bool bActivate ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	

	bool bChange = false;

	if ( bActivate &&
		 pSong->getLoopMode() != Song::LoopMode::Enabled ) {
		pSong->setLoopMode( Song::LoopMode::Enabled );
		bChange = true;
		
	} else if ( ! bActivate &&
				pSong->getLoopMode() == Song::LoopMode::Enabled ) {
		// If the transport was already looped at least once, disabling
		// loop mode will result in immediate stop. Instead, we want to
		// stop transport at the end of the song.
		if ( pSong->lengthInTicks() <
			 pAudioEngine->getTransportPosition()->getTick() ) {
			pSong->setLoopMode( Song::LoopMode::Finishing );
		} else {
			pSong->setLoopMode( Song::LoopMode::Disabled );
		}
		bChange = true;
	}

	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->handleLoopModeChanged();
	pAudioEngine->unlock();
	
	if ( bChange ) {
		EventQueue::get_instance()->push_event( EVENT_LOOP_MODE_ACTIVATION,
												static_cast<int>( bActivate ) );
	}
	
	return true;
}

bool CoreActionController::setDrumkit( const QString& sDrumkit ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pDrumkit = pHydrogen->getSoundLibraryDatabase()
		->getDrumkit( sDrumkit );
	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Drumkit [%1] could not be loaded." )
				  .arg( sDrumkit ) );
		return false;
	}

	return setDrumkit( pDrumkit );
}

bool CoreActionController::setDrumkit( std::shared_ptr<Drumkit> pDrumkit ) {
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Provided Drumkit is not valid" );
		return false;
	}

	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	INFOLOG( QString( "Setting drumkit [%1] located at [%2]" )
			.arg( pDrumkit->getName() ).arg( pDrumkit->getPath() ) );

	// Use a cloned version of the kit from e.g. the SoundLibrary in
	// order to not overwrite the original when altering the properties
	// of the current kit.
	auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );

	// It would be more clean to lock the audio engine _before_ loading
	// the samples. We might pass a tempo marker while loading and users
	// of Rubberband end up with a wrong sample length. But this is an
	// edge-case and the regular user will benefit from a load prior to
	// the locking resulting in lesser XRUNs.
	pNewDrumkit->loadSamples(
		pAudioEngine->getTransportPosition()->getBpm());

	pAudioEngine->lock( RIGHT_HERE );

	pSong->setDrumkit( pNewDrumkit );

	for ( auto& pPattern : *pSong->getPatternList() ) {
		for ( auto& pNote : *pPattern->get_notes() ) {
			pNote.second->mapTo( pNewDrumkit );
		}
	}

	pHydrogen->renameJackPorts( pSong );

	pAudioEngine->unlock();

	if ( pHydrogen->getSelectedInstrumentNumber() >=
		 pNewDrumkit->getInstruments()->size() ) {
		pHydrogen->setSelectedInstrumentNumber(
			std::max( 0, pNewDrumkit->getInstruments()->size() - 1 ), false );
	}

	initExternalControlInterfaces();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->push_event( EVENT_DRUMKIT_LOADED, 0 );

	return true;
}

bool CoreActionController::upgradeDrumkit(const QString &sDrumkitPath,
                                          const QString &sNewPath) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
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
				return false;
			}
		} else {
			QString sBackupPath = Filesystem::drumkit_backup_path( sDrumkitPath );
			if ( ! Filesystem::file_copy( sDrumkitPath, sBackupPath, true, true ) ) {
				ERRORLOG( QString( "Unable to backup source .h2drumkit file from [%1] to [%2]. We abort instead of overwriting things." )
						  .arg( sDrumkitPath ).arg( sBackupPath ) );
				return false;
			}
		}

		sPath = sDrumkitDir;
	}

	if ( ! pDrumkit->save( sPath, true ) ) {
		ERRORLOG( QString( "Error while saving upgraded kit to [%1]" )
				  .arg( sPath ) );
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
		
		if ( ! pDrumkit->exportTo( sExportPath, false ) ) {
			ERRORLOG( QString( "Unable to export upgrade drumkit to [%1]" )
					  .arg( sExportPath ) );
			return false;
		}

		INFOLOG( QString( "Upgraded drumkit exported as [%1]" )
				 .arg( sExportPath + "/" + pDrumkit->getName() +
					   Filesystem::drumkit_ext ) );
	}

	// Upgrade was successful. Cleanup
	if ( ! sTemporaryFolder.isEmpty() ) {
		Filesystem::rm( sTemporaryFolder, true, true );
	}

	INFOLOG( QString( "Drumkit [%1] successfully upgraded!" )
			 .arg( sDrumkitPath ) );

	return true;
}

bool CoreActionController::validateDrumkit( const QString& sDrumkitPath,
											bool bCheckLegacyVersions ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	INFOLOG( QString( "Validating kit [%1]" ).arg( sDrumkitPath ) );

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
		return false;
	}

	auto validateXSD = [&]( const QString& sXSDPath, const QString& sContext ) {
		
		XMLDoc doc;
		if ( !doc.read( Filesystem::drumkit_file( sDrumkitDir ),
						sXSDPath, true ) ) {
			ERRORLOG( QString( "Drumkit file [%1] does not comply with [%2] XSD definition" )
					  .arg( Filesystem::drumkit_file( sDrumkitDir ) )
					  .arg( sContext ) );
			return false;
		}
	
		XMLNode root = doc.firstChildElement( "drumkit_info" );
		if ( root.isNull() ) {
			ERRORLOG( QString( "Drumkit file [%1] seems bricked: 'drumkit_info' node not found" )
					  .arg( Filesystem::drumkit_file( sDrumkitDir ) ) );
			return false;
		}

		INFOLOG( QString( "Drumkit file [%1] validates [%2] XSD definition" )
				 .arg( Filesystem::drumkit_file( sDrumkitDir ) )
				 .arg( sContext ) );
		
		return true;
	};

	bool bValid = validateXSD( Filesystem::drumkit_xsd_path(), "current" );
	if ( ! bValid && ! bCheckLegacyVersions ) {
		return false;
	}

	if ( ! bValid && bCheckLegacyVersions ) {
		const auto legacyXSDFiles = Filesystem::drumkit_xsd_legacy_paths();

		for ( const auto& sPath : legacyXSDFiles ) {
			QString sContext( sPath );
			sContext.remove( Filesystem::xsd_dir() );
			sContext.remove( Filesystem::drumkit_xsd() );

			if ( validateXSD( sPath, sContext ) ) {
				bValid = true;
				break;
			}
		}

		if ( ! bValid ) {
			return false;
		}
	}

	// Trailing whitespaces will cause the Windows version to fail
	// extracting it.

	// Trailing whitespace in drumkit folder or archive name
	if ( sDrumkitDir.endsWith( " " ) ) {
		ERRORLOG( QString( "Drumkit folder [%1] must not end with a trailing whitespace" )
				  .arg( sDrumkitDir ) );
		return false;
	}

	// Trailing whitespace in drumkit name element
	XMLDoc doc;
	if ( !doc.read( Filesystem::drumkit_file( sDrumkitDir ), nullptr, true ) ) {
		ERRORLOG( QString( "Drumkit file in [%1] could not be opened." )
				  .arg( Filesystem::drumkit_file( sDrumkitDir ) ) );
		return false;
	}

	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( QString( "Drumkit file [%1] seems bricked: 'drumkit_info' node not found" )
				  .arg( Filesystem::drumkit_file( sDrumkitDir ) ) );
		return false;
	}

	const QString sDrumkitName = root.read_string( "name", "", false, false, false );
	if ( sDrumkitName.isEmpty() ) {
		ERRORLOG( QString( "Drumkit must have a non-empty 'name' element" ) );
		return false;
	}

	if ( sDrumkitName.endsWith( " " ) ){
		ERRORLOG( QString( "Drumkit name [%1] must not end with a trailing whitespace" )
				  .arg( sDrumkitName ) );
		return false;
	}

	// Everything is valid. No need to keep temporary artifacts.
	if ( ! sTemporaryFolder.isEmpty() ) {
		Filesystem::rm( sTemporaryFolder, true, true );
	}

	INFOLOG( QString( "Drumkit [%1] is valid!" )
			 .arg( sDrumkitPath ) );
	
	return true;
}

std::shared_ptr<Drumkit> CoreActionController::retrieveDrumkit( const QString& sDrumkitPath, bool* bIsCompressed, QString *sDrumkitDir, QString* sTemporaryFolder ) {
	auto pHydrogen = Hydrogen::get_instance();
	assert( pHydrogen );
	if ( pHydrogen == nullptr ) {
		ERRORLOG( "Core not ready yet!" );
		return nullptr;
	}

	std::shared_ptr<Drumkit> pDrumkit = nullptr;

	// We do not attempt to retrieve the drumkit from SoundLibrary
	// since this function is intended to be used for validating or
	// upgrading drumkits via CLI or OSC command. It should always
	// refer to the latest copy found on disk.

	*bIsCompressed = false;
	*sTemporaryFolder = "";
	*sDrumkitDir = "";

	QFileInfo sourceFileInfo( sDrumkitPath );

	if ( Filesystem::dir_readable( sDrumkitPath, true ) ) {

		// Providing the folder containing the drumkit
		pDrumkit = Drumkit::load( sDrumkitPath, false, true );
		*sDrumkitDir = sDrumkitPath;
		
	}
	else if ( sourceFileInfo.fileName() == Filesystem::drumkit_xml() ) {
		if ( ! Filesystem::file_readable( sDrumkitPath, true ) ) {
			ERRORLOG( QString( "Drumkit file [%1] not readable" )
					  .arg( sDrumkitPath ) );
			return nullptr;
		}

		// Providing the path of a drumkit.xml file within a drumkit
		// folder.
		QString sDrumkitDirPath = QFileInfo( sDrumkitPath ).absoluteDir().absolutePath();
		pDrumkit = Drumkit::load( sDrumkitDirPath, false, true );
		*sDrumkitDir = sourceFileInfo.dir().absolutePath();
			
	}
	else if ( ( "." + sourceFileInfo.suffix() ) == Filesystem::drumkit_ext ) {
		if ( ! Filesystem::file_readable( sDrumkitPath, true ) ) {
			ERRORLOG( QString( "Drumkit archive [%1] not readable" )
					  .arg( sDrumkitPath ) );
			return nullptr;
		}

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
		if ( ! Drumkit::install( sDrumkitPath, tmpDir.path(), nullptr, true ) ) {
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
		
		pDrumkit = Drumkit::load( *sDrumkitDir, false, true );
		
	} else {
		ERRORLOG( QString( "Provided source path [%1] does not point to a Hydrogen drumkit" )
				  .arg( sDrumkitPath ) );
		return nullptr;
	}

	return pDrumkit;
}

bool CoreActionController::extractDrumkit( const QString& sDrumkitPath, const QString& sTargetDir ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	QString sTarget;
	bool bInstall = false;
	if ( sTargetDir.isEmpty() ) {
		bInstall = true;
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

	if ( ! Drumkit::install( sDrumkitPath, sTarget, nullptr, true ) ) {
		ERRORLOG( QString( "Unabled to extract provided drumkit [%1] into [%2]" )
				  .arg( sDrumkitPath ).arg( sTarget ) );
		return false;
	}

	if ( bInstall ) {
		pHydrogen->getSoundLibraryDatabase()->updateDrumkits();
	}

	return true;
}

bool CoreActionController::addInstrument( std::shared_ptr<Instrument> pInstrument ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready yet" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pDrumkit = pSong->getDrumkit();

	pAudioEngine->lock( RIGHT_HERE );

	pDrumkit->addInstrument( pInstrument );
	pHydrogen->renameJackPorts( pSong );
	pSong->getPatternList()->mapTo( pDrumkit );

	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->push_event( EVENT_DRUMKIT_LOADED, 0 );

	return true;
}

bool CoreActionController::removeInstrument( std::shared_ptr<Instrument> pInstrument ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready yet" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pDrumkit = pSong->getDrumkit();

	const int nInstrumentNumber = pDrumkit->getInstruments()->index( pInstrument );
	if ( nInstrumentNumber == -1 ) {
		ERRORLOG( "Provided instrument is not part of current drumkit!" );
		return false;
	}

	if ( pDrumkit->getInstruments()->size() == 1 ) {
		ERRORLOG( "This is the last instrument. It can not be removed." );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );

	// Remove all notes in pattern corresponding to the provided instrument.
	for ( const auto& pPattern : *pSong->getPatternList() ) {
		pPattern->purge_instrument( pInstrument, false );
	}

	pDrumkit->removeInstrument( pInstrument );

	// At this point the instrument has been removed from both the current
	// drumkit and every pattern in the song. But it still lives on as a shared
	// pointer in all Notes within the queues of the AudioEngine and Sampler.
	// Thus, it will be added to the death row, which guarantuees that its
	// samples will be unloaded once all notes referencing it are gone. Note
	// that this does not mean the instrument will be destructed. GUI can still
	// hold a shared pointer as part of an undo/redo action (that's why it is so
	// important to unload the samples).
	pHydrogen->addInstrumentToDeathRow( pInstrument );

	const int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();
	if ( nSelectedInstrument == nInstrumentNumber ||
		 nSelectedInstrument >= pDrumkit->getInstruments()->size() ) {
		pHydrogen->setSelectedInstrumentNumber(
			std::clamp( nSelectedInstrument, 0,
						static_cast<int>(pDrumkit->getInstruments()->size() - 1 ) ) );
	}

	pHydrogen->renameJackPorts( pSong );

	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->push_event( EVENT_DRUMKIT_LOADED, 0 );

	return true;
}

bool CoreActionController::replaceInstrument( std::shared_ptr<Instrument> pNewInstrument,
											  std::shared_ptr<Instrument> pOldInstrument ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready yet" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pDrumkit = pSong->getDrumkit();
	const int nOldInstrumentNumber = pDrumkit->getInstruments()->index(
		pOldInstrument );
	if ( nOldInstrumentNumber == -1 ) {
		ERRORLOG( "Old instrument is not part of current drumkit!" );
		return false;
	}

	const auto fBpm = pAudioEngine->getTransportPosition()->getBpm();

	pAudioEngine->lock( RIGHT_HERE );

	if ( pNewInstrument != nullptr && pNewInstrument->hasSamples() ) {
		pNewInstrument->load_samples( fBpm );
	}

	pDrumkit->removeInstrument( pOldInstrument );

	// At this point the instrument has been removed from both the current
	// drumkit and every pattern in the song. But it still lives on as a shared
	// pointer in all Notes within the queues of the AudioEngine and Sampler.
	// Thus, it will be added to the death row, which guarantuees that its
	// samples will be unloaded once all notes referencing it are gone. Note
	// that this does not mean the instrument will be destructed. GUI can still
	// hold a shared pointer as part of an undo/redo action (that's why it is so
	// important to unload the samples).
	QString sNewName = QString( "deathrow_%1" ).arg( pOldInstrument->get_name() );
	pOldInstrument->set_name( sNewName );
	pHydrogen->addInstrumentToDeathRow( pOldInstrument );

	pDrumkit->addInstrument( pNewInstrument );
	pSong->getPatternList()->mapTo( pDrumkit );

	// Unloading the samples of the old instrument will be done in the death
	// row.

	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->push_event( H2Core::EVENT_DRUMKIT_LOADED, 0 );

	return true;
}

bool CoreActionController::locateToColumn( int nPatternGroup ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	if ( nPatternGroup < -1 ) {
		ERRORLOG( QString( "Provided column [%1] too low. Assigning 0  instead." )
				  .arg( nPatternGroup ) );
		nPatternGroup = 0;
	}
	
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	long nTotalTick = pHydrogen->getTickForColumn( nPatternGroup );
	if ( nTotalTick < 0 ) {
		if ( pHydrogen->getMode() == Song::Mode::Song ) {
			ERRORLOG( QString( "Provided column [%1] violates the allowed range [0;%2). No relocation done." )
					  .arg( nPatternGroup )
					  .arg( pHydrogen->getSong()->getPatternGroupVector()->size() ) );
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
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );
    
	pAudioEngine->locate( nTick, bWithJackBroadcast );
	
	pAudioEngine->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_RELOCATION, 0 );
	return true;
}

bool CoreActionController::newPattern( const QString& sPatternName ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pPatternList = pHydrogen->getSong()->getPatternList();
	Pattern* pPattern = new Pattern( sPatternName );
	
	return setPattern( pPattern, pPatternList->size() );
}
bool CoreActionController::openPattern( const QString& sPath, int nPatternPosition ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	auto pPatternList = pSong->getPatternList();
	Pattern* pNewPattern = Pattern::load_file( sPath );

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
	ASSERT_HYDROGEN

	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pPattern->mapTo( pSong->getDrumkit() );

	auto pPatternList = pSong->getPatternList();

	// Check whether the name of the new pattern is unique.
	if ( !pPatternList->check_name( pPattern->get_name() ) ){
		pPattern->set_name( pPatternList->find_unused_pattern_name( pPattern->get_name() ) );
	}

	pPatternList->insert( nPatternPosition, pPattern );
	if ( pHydrogen->isPatternEditorLocked() ) {
		pHydrogen->updateSelectedPattern( true );
	} else  {
		pHydrogen->setSelectedPatternNumber( nPatternPosition );
	}
	pHydrogen->setIsModified( true );
	
	// Update the SongEditor.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::headless ) {
		EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, 0 );
	}
	return true;
}

bool CoreActionController::removePattern( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();


	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	INFOLOG( QString( "Deleting pattern [%1]" ).arg( nPatternNumber ) );
	
	auto pPatternList = pSong->getPatternList();
	auto pPatternGroupVector = pSong->getPatternGroupVector();
	auto pPlayingPatterns = pAudioEngine->getPlayingPatterns();
	auto pNextPatterns = pAudioEngine->getNextPatterns();
	
	int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	auto pPattern = pPatternList->get( nPatternNumber );

	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Pattern [%1] not found" ).arg( nPatternNumber ) );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );

	// Ensure there is always at least one pattern present in the
	// list.
	if ( pPatternList->size() == 0 ) {
		Pattern* pEmptyPattern = new Pattern( "Pattern 1" );
		pPatternList->add( pEmptyPattern );
	}

	// Delete all instances of the pattern in the pattern group vector
	// (columns of the SongEditor)
	for ( const auto& ppatternList : *pPatternGroupVector ) {
		for ( int ii = 0; ii < ppatternList->size(); ++ii ) {
			if ( ppatternList->get( ii ) == pPattern ) {
				ppatternList->del( ii );
				// there is at most one instance of a pattern per
				// column.
				continue;
			}
		}
	}

	PatternList* pColumn;
	// Ensure there are no empty columns in the pattern group vector.
	for ( int ii = pPatternGroupVector->size() - 1; ii >= 0; --ii ) {
		pColumn = pPatternGroupVector->at( ii );
		if ( pColumn->size() == 0 ) {
			pPatternGroupVector->erase( pPatternGroupVector->begin() + ii );
			delete pColumn;
		}
		else {
			break;
		}
	}
	
	if ( pHydrogen->isPatternEditorLocked() ) {
		pHydrogen->updateSelectedPattern( false );
	} else if ( nPatternNumber == nSelectedPatternNumber ) {
		pHydrogen->setSelectedPatternNumber( std::max( 0, nPatternNumber - 1 ),
											 false );
	}

	// Remove the pattern from the list of of patterns that are played
	// next in pattern mode.
	// IMPORTANT: it has to be removed from the next patterns list
	// _before_ updating the playing patterns.
	for ( int ii = 0; ii < pNextPatterns->size(); ++ii ) {
		if ( pNextPatterns->get( ii ) == pPattern ) {
			pAudioEngine->toggleNextPattern( nPatternNumber );
		}
	}
	
	// Ensure the pattern is not among the list of currently played
	// patterns cached in the audio engine if transport is in pattern
	// mode.
	pAudioEngine->removePlayingPattern( pPattern );

	// Delete the pattern from the list of available patterns.
	pPatternList->del( pPattern );

	pHydrogen->updateSongSize();

	pAudioEngine->unlock();

	// Update virtual pattern presentation.
	for ( const auto& ppattern : *pPatternList ) {

		Pattern::virtual_patterns_cst_it_t it =
			ppattern->get_virtual_patterns()->find( pPattern );
		if ( it != ppattern->get_virtual_patterns()->end() ) {
			ppattern->virtual_patterns_del( *it );
		}
	}

	pHydrogen->updateVirtualPatterns();
	pHydrogen->setIsModified( true );
	
	delete pPattern;
	
	return true;
}

bool CoreActionController::clearInstrumentInPattern( int nInstrument,
													 int nPatternNumber ) {

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	int nPattern;
	if ( nPatternNumber != -1 ) {
		nPattern = nPatternNumber;
	} else {
		nPattern = pHydrogen->getSelectedPatternNumber();
	}

	auto pPattern = pSong->getPatternList()->get( nPattern );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" ).arg( nPattern ) );
		return false;
	}

	auto pInstrument = pSong->getDrumkit()->getInstruments()->get( nInstrument );
	if ( pInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" ).arg( nInstrument ) );
		return false;
	}

	pPattern->purge_instrument( pInstrument, true );

	EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, 0 );

	return true;
}

bool CoreActionController::toggleGridCell( int nColumn, int nRow ){
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

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
	
	pHydrogen->updateSongSize();
	pHydrogen->updateSelectedPattern( false );
	
	pHydrogen->getAudioEngine()->unlock();

	pHydrogen->setIsModified( true );
	
	// Update the SongEditor.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::headless ) {
		EventQueue::get_instance()->push_event( EVENT_GRID_CELL_TOGGLED, 0 );
	}

	return true;
}

bool CoreActionController::handleNote( int nNote, float fVelocity, bool bNoteOff ) {
	const auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	int nInstrument = nNote - MidiMessage::instrumentOffset;
	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	std::shared_ptr<Instrument> pInstrument = nullptr;
	QString sMode;

	if ( pPref->m_bPlaySelectedInstrument ){
		nInstrument = pHydrogen->getSelectedInstrumentNumber();
		pInstrument = pInstrumentList->get( pHydrogen->getSelectedInstrumentNumber());
		if ( pInstrument == nullptr ) {
			WARNINGLOG( "No instrument selected!" );
			return false;
		}
		sMode = "Play Selected Instrument";
	}
	else if ( pPref->m_bMidiFixedMapping ){
		pInstrument = pInstrumentList->findMidiNote( nNote );
		if ( pInstrument == nullptr ) {
			WARNINGLOG( QString( "Unable to map note [%1] to instrument" )
						.arg( nNote ) );
			return false;
		}
		nInstrument = pInstrumentList->index( pInstrument );
		sMode = "Map to Output MIDI note";
	}
	else {
		nInstrument = nNote - MidiMessage::instrumentOffset;
		if( nInstrument < 0 || nInstrument >= pInstrumentList->size()) {
			WARNINGLOG( QString( "Instrument number [%1] - derived from note [%2] - out of bound note [%3,%4]" )
						.arg( nInstrument ).arg( nNote )
						.arg( 0 ).arg( pInstrumentList->size() ) );
			return false;
		}

		pInstrument = pInstrumentList->get( nInstrument );
		if ( pInstrument == nullptr ) {
			WARNINGLOG( QString( "Unable to retrieve instrument [%1]" )
						.arg( nInstrument ) );
			return false;
		}
		sMode = "Map to instrument list position";
	}


	// Only look to change instrument if the current note is actually of hihat
	// and hihat openness is outside the instrument selected
	const int nHihatOpenness = pHydrogen->getHihatOpenness();
	if ( pInstrument != nullptr &&
		 pInstrument->get_hihat_grp() >= 0 &&
		 ( nHihatOpenness < pInstrument->get_lower_cc() ||
		   nHihatOpenness > pInstrument->get_higher_cc() ) ) {

		for ( int i = 0; i <= pInstrumentList->size(); i++ ) {
			auto ppInstr = pInstrumentList->get( i );
			if ( ppInstr != nullptr &&
				pInstrument->get_hihat_grp() == ppInstr->get_hihat_grp() &&
				nHihatOpenness >= ppInstr->get_lower_cc() &&
				nHihatOpenness <= ppInstr->get_higher_cc() ) {

				nInstrument = i;
				sMode = "Hihat Pressure Group";
				break;
			}
		}
	}

	INFOLOG( QString( "[%1] mapped note [%2] to instrument [%3]" )
			 .arg( sMode ).arg( nNote ).arg( nInstrument ) );

	return pHydrogen->addRealtimeNote( nInstrument, fVelocity, false, nNote );
}

void CoreActionController::insertRecentFile( const QString& sFilename ){
	auto pPref = Preferences::get_instance();

	// The most recent file will always be added on top and possible
	// duplicates are removed later on.
	bool bAlreadyContained = false;

	QStringList recentFiles = pPref->getRecentFiles();

	// We have to normalize directory separators. Else opening a
	// song via double click from file browser and from within
    // Hydrogen will give to distinct entries on Windows.
    const QString sFilenameCleaned = QDir::cleanPath( sFilename );

    recentFiles.push_front( sFilenameCleaned );
	recentFiles.removeDuplicates();

	pPref->setRecentFiles( recentFiles );
}

bool CoreActionController::setBpm( float fBpm ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set yet" );
		return false;
	}

	fBpm = std::clamp( fBpm, static_cast<float>(MIN_BPM),
						  static_cast<float>(MAX_BPM) );

	pAudioEngine->lock( RIGHT_HERE );
	// Use tempo in the next process cycle of the audio engine.
	pAudioEngine->setNextBpm( fBpm );
	pAudioEngine->unlock();

	// Store it's value in the .h2song file.
	pHydrogen->getSong()->setBpm( fBpm );

	pHydrogen->setIsModified( true );
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );

	return true;
}

std::shared_ptr<Playlist> CoreActionController::loadPlaylist( const QString& sPath,
															  const QString& sRecoverPath ) {
	auto pHydrogen = Hydrogen::get_instance();
	assert( pHydrogen );
	if ( pHydrogen == nullptr ) {
		ERRORLOG( "Core not ready yet!" );
		return nullptr;
	}

	// Check whether the provided path is valid.
	if ( sPath != Filesystem::empty_path( Filesystem::Type::Playlist ) &&
		 ! Filesystem::isPathValid(
			 Filesystem::Type::Playlist, sPath, true ) ) {
		// Filesystem::isPathValid takes care of the error log message.
		return nullptr;
	}

	std::shared_ptr<Playlist> pPlaylist;
	if ( ! sRecoverPath.isEmpty() && Filesystem::isPathValid(
			 Filesystem::Type::Playlist, sRecoverPath, true ) ) {
		// Use an autosave file to load the playlist
		pPlaylist = Playlist::load( sRecoverPath );
		if ( pPlaylist != nullptr ) {
			pPlaylist->setFilename( sPath );
		} else {
			ERRORLOG( QString( "Unable to recover changes from [%1]. Loading [%2] instead." )
					  .arg( sRecoverPath ).arg( sPath ) );
		}
	}

	if ( pPlaylist == nullptr ) {
		pPlaylist = Playlist::load( sPath );
	}

	if ( pPlaylist == nullptr ) {
		ERRORLOG( QString( "Unable to open playlist [%1]." )
				  .arg( sPath ) );
		return nullptr;
	}

	return pPlaylist;
}

bool CoreActionController::setPlaylist( std::shared_ptr<Playlist> pPlaylist ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid playlist" );
		return false;
	}
	pHydrogen->setPlaylist( pPlaylist );

	if ( pPlaylist->getFilename() ==
		 Filesystem::empty_path( Filesystem::Type::Playlist ) ) {
		// To indicate that the user closed the previous playlsit in favor
		// of a new one, we store an empty string. This way the changes from
		// the empty playlist can be recovered.
		Preferences::get_instance()->setLastPlaylistFilename( "" );
	}
	else {
		Preferences::get_instance()->setLastPlaylistFilename(
			pPlaylist->getFilename() );
	}

	EventQueue::get_instance()->push_event( EVENT_PLAYLIST_CHANGED, 0 );

	// In case the playlist is read-only, autosave won't work.
	if ( ! Filesystem::file_writable( pPlaylist->getFilename() ) ) {
		WARNINGLOG( QString( "You don't have permissions to write to the playlist found in path [%1]. It will be opened as read-only (no autosave)." )
					.arg( pPlaylist->getFilename() ));
		EventQueue::get_instance()->push_event( EVENT_PLAYLIST_CHANGED, 2 );
	}

	return true;
}

bool CoreActionController::savePlaylist() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pPlaylist = pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}
	if ( ! pPlaylist->save() ) {
		return false;
	}

	pPlaylist->setIsModified( false );
	EventQueue::get_instance()->push_event( EVENT_PLAYLIST_CHANGED, 1 );
	return true;
}

bool CoreActionController::savePlaylistAs( const QString& sPath ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pPlaylist = pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}
	if ( ! pHydrogen->getPlaylist()->saveAs( sPath ) ) {
		ERRORLOG( QString( "Unable to save playlist to [%1]" ).arg( sPath ) );
		return false;
	}

	pPlaylist->setIsModified( false );

	Preferences::get_instance()->setLastPlaylistFilename( sPath );

	EventQueue::get_instance()->push_event( EVENT_PLAYLIST_CHANGED, 0 );

	return true;
}

bool CoreActionController::addToPlaylist( std::shared_ptr<PlaylistEntry> pEntry,
										  int nIndex ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	if ( pEntry == nullptr ) {
		return false;
	}
	auto pPlaylist = pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}

	if ( ! pPlaylist->add( pEntry, nIndex ) ) {
		return false;
	}

	pPlaylist->setIsModified( true );
	EventQueue::get_instance()->push_event( EVENT_PLAYLIST_CHANGED, 0 );
	return true;

}
bool CoreActionController::removeFromPlaylist( std::shared_ptr<PlaylistEntry> pEntry,
											   int nIndex ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	if ( pEntry == nullptr ) {
		return false;
	}
	auto pPlaylist = pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}

	if ( ! pPlaylist->remove( pEntry, nIndex ) ) {
		return false;
	}

	pPlaylist->setIsModified( true );
	EventQueue::get_instance()->push_event( EVENT_PLAYLIST_CHANGED, 0 );
	return true;

}
bool CoreActionController::activatePlaylistSong( int nSongNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pPlaylist = pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}

	if ( ! pPlaylist->activateSong( nSongNumber ) ) {
		ERRORLOG( QString( "Unable to set playlist song [%1]" )
				  .arg( nSongNumber ) );
		return false;
	}
	EventQueue::get_instance()->push_event( H2Core::EVENT_PLAYLIST_LOADSONG,
											nSongNumber );

	return true;
}
}
