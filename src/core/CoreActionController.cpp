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

#include "CoreActionController.h"

#include <QDir>
#include "Midi/Midi.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/GridPoint.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/IO/AlsaMidiDriver.h>
#include <core/IO/JackAudioDriver.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Midi/MidiMessage.h>
#include <core/OscServer.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

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
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->getVolume() != fVolumeValue ) {
		pInstr->setVolume( fVolumeValue );

		pHydrogen->setIsModified( true );

		return sendStripVolumeFeedback( nStrip );
	}

	return true;
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

	if ( pInstrument->getPitchOffset() != fValue ) {
		pInstrument->setPitchOffset( fValue );
		EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
	}
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nInstrument );

	return true;
}

bool CoreActionController::setInstrumentMidiOutNote(
	int nInstrument,
	Midi::Note note,
	long* pEventId
)
{
	if ( pEventId != nullptr ) {
		*pEventId = Event::nInvalidId;
	}

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
	if ( pInstrument == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" )
				  .arg( nInstrument ) );
		return false;
	}

	if ( pInstrument->getMidiOutNote() != note ) {
		pInstrument->setMidiOutNote( note );
		const auto nId = EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		if ( pEventId != nullptr ) {
			*pEventId = nId;
		}
	}

	return true;
}

bool CoreActionController::setInstrumentMidiOutChannel(
	int nInstrument,
	Midi::Channel channel,
	long* pEventId
)
{
	if ( pEventId != nullptr ) {
		*pEventId = Event::nInvalidId;
	}

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
	if ( pInstrument == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" )
				  .arg( nInstrument ) );
		return false;
	}

	if ( pInstrument->getMidiOutChannel() != channel ) {
		pInstrument->setMidiOutChannel( channel );
		const auto nId = EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		if ( pEventId != nullptr ) {
			*pEventId = nId;
		}
	}

	return true;
}

bool CoreActionController::setMetronomeIsActive( bool isActive )
{
	auto pPref = Preferences::get_instance();
	if ( pPref->m_bUseMetronome != isActive ) {
		pPref->m_bUseMetronome = isActive;

		EventQueue::get_instance()->pushEvent( Event::Type::Metronome, 2 );

		return sendMetronomeIsActiveFeedback();
	}

	return true;
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

	if ( pSong->getIsMuted() != bIsMuted ) {
		pSong->setIsMuted( bIsMuted );
	
		pHydrogen->setIsModified( true );

		EventQueue::get_instance()->pushEvent( Event::Type::MixerSettingsChanged, 0 );

		return sendMasterIsMutedFeedback();
	}

	return true;
}

bool CoreActionController::setHumanizeTime( float fValue ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( pSong->getHumanizeTimeValue() != fValue ) {
		pSong->setHumanizeTimeValue( fValue );

		EventQueue::get_instance()->pushEvent( Event::Type::MixerSettingsChanged, 0 );

		pHydrogen->setIsModified( true );
	}

	return true;
}

bool CoreActionController::setHumanizeVelocity( float fValue ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( pSong->getHumanizeVelocityValue() != fValue ) {
		pSong->setHumanizeVelocityValue( fValue );

		EventQueue::get_instance()->pushEvent( Event::Type::MixerSettingsChanged, 0 );

		pHydrogen->setIsModified( true );
	}

	return true;
}

bool CoreActionController::setSwing( float fValue ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( pSong->getSwingFactor() != fValue ) {
		pSong->setSwingFactor( fValue );

		EventQueue::get_instance()->pushEvent( Event::Type::MixerSettingsChanged, 0 );

		pHydrogen->setIsModified( true );
	}

	return true;
}

bool CoreActionController::toggleStripIsMuted( int nStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}
	
	return setStripIsMuted( nStrip, !pInstr->isMuted(), false );
}

bool CoreActionController::setStripIsMuted( int nStrip, bool bIsMuted,
											bool bSelectStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->isMuted() != bIsMuted ) {
		pInstr->setMuted( bIsMuted );

		EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip );
		EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentMuteSoloChanged, nStrip );
	
		pHydrogen->setIsModified( true );

		return sendStripIsMutedFeedback( nStrip );
	}

	return true;
}

bool CoreActionController::toggleStripIsSoloed( int nStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
		auto pInstr = getStrip( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}
	
	return setStripIsSoloed( nStrip, !pInstr->isSoloed(), false );
}

bool CoreActionController::setStripIsSoloed( int nStrip, bool isSoloed,
											 bool bSelectStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->isSoloed() != isSoloed ) {
	
		pInstr->setSoloed( isSoloed );

		EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip );
		EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentMuteSoloChanged, nStrip );
	
		pHydrogen->setIsModified( true );

		return sendStripIsSoloedFeedback( nStrip );
	}

	return true;
}

bool CoreActionController::setStripPan( int nStrip, float fValue, bool bSelectStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->getPanWithRangeFrom0To1() != fValue ) {
		pInstr->setPanWithRangeFrom0To1( fValue );
		
		EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip );
		
		pHydrogen->setIsModified( true );

		return sendStripPanFeedback( nStrip );
	}

	return true;
}


bool CoreActionController::setStripPanSym( int nStrip, float fValue, bool bSelectStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->getPan() != fValue ) {
		pInstr->setPan( fValue );
		
		EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip );
		
		pHydrogen->setIsModified( true );

		return sendStripPanFeedback( nStrip );
	}

	return true;
}

bool CoreActionController::setStripEffectLevel( int nStrip, int nEffect,
												float fValue, bool bSelectStrip )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr == nullptr || nEffect < 0 || nEffect >= MAX_FX ) {
		return false;
	}

	if ( bSelectStrip ) {
		pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->getFxLevel( nEffect ) != fValue ) {
		pInstr->setFxLevel( fValue, nEffect );

		EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip );

		pHydrogen->setIsModified( true );
	}

	return true;
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
		
		auto pFeedbackAction = std::make_shared<MidiAction>(
			MidiAction::Type::MasterVolumeAbsolute );
		
		pFeedbackAction->setValue( QString("%1")
								   .arg( fMasterVolume ) );
		OscServer::get_instance()->handleMidiAction( pFeedbackAction );
	}
#endif
	
	const auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	
	auto ccParamValues = pMidiEventMap->findCCParametersByType(
		MidiAction::Type::MasterVolumeAbsolute );

	return handleOutgoingControlChanges(
		ccParamValues,
		Midi::parameterFromIntClamp(
			( fMasterVolume / 1.5 ) * static_cast<int>( Midi::ParameterMaximum )
		)
	);
}

bool CoreActionController::sendStripVolumeFeedback( int nStrip ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {

		float fStripVolume = pInstr->getVolume();
		
#ifdef H2CORE_HAVE_OSC
		if ( Preferences::get_instance()->getOscFeedbackEnabled() ) {
		
			auto pFeedbackAction = std::make_shared<MidiAction>(
				MidiAction::Type::StripVolumeAbsolute );
		
			pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
			pFeedbackAction->setValue( QString("%1").arg( fStripVolume ) );
			OscServer::get_instance()->handleMidiAction( pFeedbackAction );
		}
#endif

		const auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	
		auto ccParamValues = pMidiEventMap->findCCParametersByTypeAndParam1(
			MidiAction::Type::StripVolumeAbsolute, QString("%1").arg( nStrip ) );

		return handleOutgoingControlChanges(
			ccParamValues, Midi::parameterFromIntClamp(
							   ( fStripVolume / 1.5 ) *
							   static_cast<int>( Midi::ParameterMaximum )
						   )
		);
	}

	return false;
}

bool CoreActionController::sendMetronomeIsActiveFeedback() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	const auto pPref = Preferences::get_instance();
	
#ifdef H2CORE_HAVE_OSC
	if ( pPref->getOscFeedbackEnabled() ) {
		auto pFeedbackAction = std::make_shared<MidiAction>(
			MidiAction::Type::ToggleMetronome );
		
		pFeedbackAction->setParameter1(
			QString("%1") .arg( static_cast<int>(pPref->m_bUseMetronome) ) );
		OscServer::get_instance()->handleMidiAction( pFeedbackAction );
	}
#endif
	
	const auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	
	auto ccParamValues = pMidiEventMap->findCCParametersByType(
		MidiAction::Type::ToggleMetronome );

	return handleOutgoingControlChanges(
		ccParamValues, Midi::parameterFromIntClamp(
						   static_cast<int>( pPref->m_bUseMetronome ) *
						   static_cast<int>( Midi::ParameterMaximum )
					   )
	);
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
		auto pFeedbackAction = std::make_shared<MidiAction>(
			MidiAction::Type::MuteToggle );
		
		pFeedbackAction->setParameter1(
			QString("%1") .arg( static_cast<int>(pSong->getIsMuted()) ) );
		OscServer::get_instance()->handleMidiAction( pFeedbackAction );
	}
#endif

	const auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();

	auto ccParamValues = pMidiEventMap->findCCParametersByType(
		MidiAction::Type::MuteToggle );

	return handleOutgoingControlChanges(
		ccParamValues, Midi::parameterFromIntClamp(
						   static_cast<int>( pSong->getIsMuted() ) *
						   static_cast<int>( Midi::ParameterMaximum )
					   )
	);
}

bool CoreActionController::sendStripIsMutedFeedback( int nStrip ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pInstr = getStrip( nStrip );
	if ( pInstr != nullptr ) {
	
#ifdef H2CORE_HAVE_OSC
		if ( Preferences::get_instance()->getOscFeedbackEnabled() ) {
			auto pFeedbackAction = std::make_shared<MidiAction>(
				MidiAction::Type::StripMuteToggle );
		
			pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
			pFeedbackAction->setValue(
				QString("%1") .arg( static_cast<int>(pInstr->isMuted()) ) );
			OscServer::get_instance()->handleMidiAction( pFeedbackAction );
		}
#endif

		const auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
	
		auto ccParamValues = pMidiEventMap->findCCParametersByTypeAndParam1(
			MidiAction::Type::StripMuteToggle, QString("%1").arg( nStrip ) );

		return handleOutgoingControlChanges(
			ccParamValues, Midi::parameterFromIntClamp(
							   static_cast<int>( pInstr->isMuted() ) *
							   static_cast<int>( Midi::ParameterMaximum )
						   )
		);
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
			auto pFeedbackAction = std::make_shared<MidiAction>(
				MidiAction::Type::StripSoloToggle );
		
			pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
			pFeedbackAction->setValue(
				QString("%1") .arg( static_cast<int>(pInstr->isSoloed()) ) );
			OscServer::get_instance()->handleMidiAction( pFeedbackAction );
		}
#endif

		const auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
		auto ccParamValues = pMidiEventMap->findCCParametersByTypeAndParam1(
			MidiAction::Type::StripSoloToggle, QString("%1").arg( nStrip ) );

		return handleOutgoingControlChanges(
			ccParamValues, Midi::parameterFromIntClamp(
							   static_cast<int>( pInstr->isSoloed() ) *
							   static_cast<int>( Midi::ParameterMaximum )
						   )
		);
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
			auto pFeedbackAction = std::make_shared<MidiAction>(
				MidiAction::Type::PanAbsolute );
		
			pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
			pFeedbackAction->setValue(
				QString("%1") .arg( pInstr->getPanWithRangeFrom0To1() ) );
			OscServer::get_instance()->handleMidiAction( pFeedbackAction );
		}
#endif
	
		const auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();
		auto ccParamValues = pMidiEventMap->findCCParametersByTypeAndParam1(
			MidiAction::Type::PanAbsolute, QString("%1").arg( nStrip ) );

		return handleOutgoingControlChanges(
			ccParamValues, Midi::parameterFromIntClamp(
							   pInstr->getPanWithRangeFrom0To1() *
							   static_cast<int>( Midi::ParameterMaximum )
						   )
		);
	}

	return false;
}

bool CoreActionController::handleOutgoingControlChanges(
	const std::vector<Midi::Parameter>& params,
	Midi::Parameter value
)
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	const auto pPref = Preferences::get_instance();
	if ( pPref->getMidiFeedbackChannel() == Midi::ChannelOff ) {
		return true;
	}
	else if ( pPref->getMidiFeedbackChannel() == Midi::ChannelInvalid ) {
		return false;
	}
	auto pMidiDriver = pHydrogen->getMidiDriver();

	if ( pHydrogen->getSong() == nullptr || pMidiDriver == nullptr ) {
		return false;
	}

	MidiMessage::ControlChange controlChange;
	for ( const auto& pparam : params ) {
		if ( pPref->m_bEnableMidiFeedback &&
			 pparam != Midi::ParameterInvalid ) {
			controlChange.parameter = pparam;
			controlChange.value = value;
			// For now the MIDI feedback channel is always 0.
			controlChange.channel = pPref->getMidiFeedbackChannel();
			pMidiDriver->enqueueOutputMessage( MidiMessage::from( controlChange
			) );
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
			pSong->setFileName( sPath );
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

	auto pAudioEngine = pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->getSampler()->clearLastUsedLayers();
	pAudioEngine->unlock();
		
	if ( pHydrogen->isUnderSessionManagement() ) {
		pHydrogen->restartAudioDriver();
	}
	else {
		// Add the new loaded song in the "last used song" vector.
		// This behavior is prohibited under session management. Only
		// songs open during normal runs will be listed. In addition,
		// empty songs - created and set when hitting "New Song" in
		// the main menu - aren't listed either.

		if ( pSong->getFileName() ==
			 Filesystem::empty_path( Filesystem::Type::Song ) ) {
			// To indicate that the user closed the previous song in favor of a
			// new one, we store an empty string. This way the changes from the
			// empty song can be recovered.
			Preferences::get_instance()->setLastSongFileName( "" );
		}
		else {
			insertRecentFile( pSong->getFileName() );
			Preferences::get_instance()->setLastSongFileName( pSong->getFileName() );
		}
	}

	// Be sure to not make GUI render its content twice by triggering this
	// during startup.
	if ( pHydrogen->getGUIState() == Hydrogen::GUIState::ready ) {
		EventQueue::get_instance()->pushEvent( Event::Type::UpdateSong, 0 );
	}

	// In case the song is read-only, autosave won't work.
	if ( ! Filesystem::file_writable( pSong->getFileName() ) ) {
		WARNINGLOG( QString( "You don't have permissions to write to the song found in path [%1]. It will be opened as read-only (no autosave)." )
					.arg( pSong->getFileName() ));
		EventQueue::get_instance()->pushEvent( Event::Type::UpdateSong, 2 );
	}

	// As we just set a fresh song, we can mark it not modified
	pHydrogen->setIsModified( false );
	
	return true;
}

bool CoreActionController::saveSong( bool bKeepMissingSamples ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	// Extract the path to the associate .h2song file.
	QString sSongPath = pSong->getFileName();
	
	if ( sSongPath.isEmpty() ) {
		ERRORLOG( "Unable to save song. Empty filename!" );
		return false;
	}

	const bool bHadMissingSamples = pSong->hasMissingSamples();

	// Actual saving
	bool bSaved = pSong->save( sSongPath, bKeepMissingSamples, true );
	if ( ! bSaved ) {
		ERRORLOG( QString( "Current song [%1] could not be saved!" )
				  .arg( sSongPath ) );
		return false;
	}
	
	// Update the status bar.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::headless ) {
		if ( ! bKeepMissingSamples && bHadMissingSamples ) {
			// Some instrument layers might have been discarded. Reload the
			// entire drumkit.
			EventQueue::get_instance()->pushEvent( Event::Type::UpdateSong, 0 );
		}
		else {
			EventQueue::get_instance()->pushEvent( Event::Type::UpdateSong, 1 );
		}
	}
	
	return true;
}

bool CoreActionController::saveSongAs( const QString& sNewFileName,
									  bool bKeepMissingSamples ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	// Check whether the provided path is valid.
	if ( !Filesystem::isPathValid(
			 Filesystem::Type::Song, sNewFileName ) ) {
		// Filesystem::isPathValid takes care of the error log message.
		return false;
	}
	if ( ! Filesystem::file_writable( sNewFileName ) ) {
		ERRORLOG( QString( "Song can not be written to read-only location [%1]" )
				  .arg( sNewFileName ) );
		return false;
	}

	QString sPreviousFileName( pSong->getFileName() );
	pSong->setFileName( sNewFileName );
	
	// Actual saving
	if ( ! saveSong( bKeepMissingSamples ) ) {
		return false;
	}

	// Update the recentFiles list by replacing the former file name
	// with the new one.
	insertRecentFile( sNewFileName );
	if ( ! pHydrogen->isUnderSessionManagement() ) {
		Preferences::get_instance()->setLastSongFileName( pSong->getFileName() );
	}

	EventQueue::get_instance()->pushEvent( Event::Type::UpdateSong, 1 );
	
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

	pAudioEngine->getMetronomeInstrument()->setVolume(
		pPreferences->m_fMetronomeVolume );

	pHydrogen->restartAudioDriver();
	pHydrogen->restartMidiDriver();
	pHydrogen->recreateOscServer();

	// If the GUI is active, we have to update it to reflect the
	// changes in the preferences.
	if ( pHydrogen->getGUIState() == H2Core::Hydrogen::GUIState::ready ) {
		H2Core::EventQueue::get_instance()->pushEvent(
			H2Core::Event::Type::UpdatePreferences, 1 );
	}

	return true;
}

bool CoreActionController::savePreferences() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::headless ) {
		// Update the status bar and let the GUI save the preferences
		// (after writing its current settings to disk).
		EventQueue::get_instance()->pushEvent( Event::Type::UpdatePreferences, 0 );
		return true;
	}
	
	return Preferences::get_instance()->save();
}

bool CoreActionController::quit() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	EventQueue::get_instance()->pushEvent( Event::Type::Quit, 0 );

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

	const auto tempoSource = pHydrogen->getTempoSource();
	if ( tempoSource == Hydrogen::Tempo::Jack ) {
		WARNINGLOG( QString( "Timeline usage was [%1] in the Preferences. But these changes won't have an effect as long as there is still an external JACK Timebase controller." )
					.arg( bActivate ? "enabled" : "disabled" ) );
	}
	else if ( tempoSource == Hydrogen::Tempo::Midi ) {
		WARNINGLOG( QString( "Timeline usage was [%1] in the Preferences. But these changes won't have an effect as long as MIDI clock handling is enabled." )
					.arg( bActivate ? "enabled" : "disabled" ) );
	}
	else if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
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

	if ( pTimeline->hasColumnTempoMarker( nPosition ) ) {
		const auto pPreviousMarker = pTimeline->getTempoMarkerAtColumn( nPosition );
		if ( fBpm == pPreviousMarker->fBpm ) {
			// Markers is already present. Nothing to do.
			return true;
		}
	}
	pAudioEngine->lock( RIGHT_HERE );

	pTimeline->addTempoMarker( nPosition, fBpm );
	pHydrogen->getAudioEngine()->handleTimelineChange();

	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->pushEvent( Event::Type::UpdateTimeline, 0 );

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

	if ( ! pHydrogen->getTimeline()->hasColumnTempoMarker( nPosition ) ) {
		// Nothing to do
		return true;
	}

	pAudioEngine->lock( RIGHT_HERE );
	
	pHydrogen->getTimeline()->deleteTempoMarker( nPosition );
	pHydrogen->getAudioEngine()->handleTimelineChange();

	pAudioEngine->unlock();
	
	pHydrogen->setIsModified( true );
	EventQueue::get_instance()->pushEvent( Event::Type::UpdateTimeline, 0 );

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

	EventQueue::get_instance()->pushEvent( Event::Type::UpdateTimeline, 0 );

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
	EventQueue::get_instance()->pushEvent( Event::Type::UpdateTimeline, 0 );

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
	
	EventQueue::get_instance()->pushEvent( Event::Type::JackTransportActivation, static_cast<int>( bActivate ) );
	
	return true;
#else
	ERRORLOG( "Unable to (de)activate Jack transport. Your Hydrogen version was not compiled with jack support." );
	return false;
#endif
}

bool CoreActionController::toggleJackTimebaseControl() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	if ( Preferences::get_instance()->m_bJackTimebaseMode ==
		 Preferences::USE_JACK_TIMEBASE_CONTROL ) {
		activateJackTimebaseControl( false );
	} else {
		activateJackTimebaseControl( true );
	}

	return true;
}

bool CoreActionController::activateJackTimebaseControl( bool bActivate ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

#ifdef H2CORE_HAVE_JACK
	if ( !pHydrogen->hasJackAudioDriver() ) {
		ERRORLOG( "Unable to (de)activate JACK Timebase support. Please select the JACK driver first." );
		return false;
	}
	
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	if ( bActivate ) {
		Preferences::get_instance()->m_bJackTimebaseMode =
			Preferences::USE_JACK_TIMEBASE_CONTROL;
		pHydrogen->initJackTimebaseControl();
	} else {
		Preferences::get_instance()->m_bJackTimebaseMode =
			Preferences::NO_JACK_TIMEBASE_CONTROL;
		pHydrogen->releaseJackTimebaseControl();
	}
	pHydrogen->getAudioEngine()->unlock();
	
	return true;
#else
	ERRORLOG( "Unable to (de)activate JACK Timebase support. Your Hydrogen version was not compiled with JACK support." );
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
		pHydrogen->setMode( Song::Mode::Song, Event::Trigger::Default );
	}
	else if ( ! bActivate && pHydrogen->getMode() != Song::Mode::Pattern ) {
		pHydrogen->setMode( Song::Mode::Pattern, Event::Trigger::Default );
	}

	if ( pHydrogen->getSelectedPatternNumber() == -1 ) {
		pHydrogen->setSelectedPatternNumber( 0, false, Event::Trigger::Suppress );
	}
	
	pAudioEngine->handleSongModeChanged( Event::Trigger::Suppress );

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
		EventQueue::get_instance()->pushEvent( Event::Type::LoopModeActivation,
												static_cast<int>( bActivate ) );
	}
	
	return true;
}

bool CoreActionController::activateRecordMode( bool bActivate ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	if ( pHydrogen->getRecordEnabled() != bActivate ) {
		pHydrogen->setRecordEnabled( bActivate );

		EventQueue::get_instance()->pushEvent(
			Event::Type::RecordModeChanged, static_cast<int>( bActivate ) );
	}

	return true;
}

bool CoreActionController::toggleRecordMode() {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	return activateRecordMode( ! pHydrogen->getRecordEnabled() );
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

	return setDrumkit( std::make_shared<Drumkit>(pDrumkit) );
}

bool CoreActionController::setDrumkit( std::shared_ptr<Drumkit> pNewDrumkit ) {
	if ( pNewDrumkit == nullptr ) {
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
	auto pPreviousDrumkit = pSong->getDrumkit();
	if ( pPreviousDrumkit == pNewDrumkit ) {
		return true;
	}

	if ( pPreviousDrumkit == nullptr ) {
		INFOLOG( QString( "Setting drumkit [%1] located at [%2]" )
				 .arg( pNewDrumkit->getName() ).arg( pNewDrumkit->getPath() ) );
	} else {
		INFOLOG( QString( "Switching drumkits [%1] -> [%2] located at [%3]" )
				 .arg( pPreviousDrumkit->getName() )
				 .arg( pNewDrumkit->getName() ).arg( pNewDrumkit->getPath() ) );
	}

	// Ensure instruments of the new kit aren't already in the death row.
	for ( const auto& ppInstrument : *pNewDrumkit->getInstruments() ) {
		pHydrogen->removeInstrumentFromDeathRow( ppInstrument );
	}

	// It would be more clean to lock the audio engine _before_ loading
	// the samples. We might pass a tempo marker while loading and users
	// of Rubberband end up with a wrong sample length. But this is an
	// edge-case and the regular user will benefit from a load prior to
	// the locking resulting in lesser XRUNs.
	pNewDrumkit->loadSamples(
		pAudioEngine->getTransportPosition()->getBpm());

	pAudioEngine->lock( RIGHT_HERE );

	// Add all instruments of the previous drumkit to the death row. This way
	// all notes in audio engine and sampler queue can be rendered till they are
	// done. Unloading their samples will be done at a latter point.
	if ( pPreviousDrumkit != nullptr ) {
		for ( const auto& ppInstrument : *pPreviousDrumkit->getInstruments() ) {
			pHydrogen->addInstrumentToDeathRow( ppInstrument );
		}
	}

	// Instead of letting all notes associated with this instrument ring till
	// the end, we discard those for which playback did not started yet and make
	// the remaining ones enter ADSR release phase.
	pAudioEngine->clearNoteQueues();
	pAudioEngine->getSampler()->releasePlayingNotes();
	pAudioEngine->getSampler()->clearLastUsedLayers();

	pSong->setDrumkit( pNewDrumkit );
	pSong->getPatternList()->mapToDrumkit( pNewDrumkit, pPreviousDrumkit );

	pHydrogen->renameJackPorts( pSong, pPreviousDrumkit );

	if ( pHydrogen->getSelectedInstrumentNumber() >=
		 pNewDrumkit->getInstruments()->size() ) {
		pHydrogen->setSelectedInstrumentNumber(
			std::max( 0, pNewDrumkit->getInstruments()->size() - 1 ),
			Event::Trigger::Suppress );
	}

	pAudioEngine->unlock();

	initExternalControlInterfaces();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->pushEvent( Event::Type::DrumkitLoaded, 0 );

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
	bool bIsCompressed, bLegacyFormatEncountered;
	auto pDrumkit = retrieveDrumkit(
		sDrumkitPath, &bIsCompressed, &sDrumkitDir, &sTemporaryFolder,
		&bLegacyFormatEncountered );

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
		
		if ( ! pDrumkit->exportTo( sExportPath, nullptr, false ) ) {
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
	bool bIsCompressed, bLegacyFormatEncountered;
	const auto pDrumkit = retrieveDrumkit(
		sDrumkitPath, &bIsCompressed, &sDrumkitDir, &sTemporaryFolder,
		&bLegacyFormatEncountered );

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

	XMLDoc doc;
	if ( !doc.read( Filesystem::drumkit_file( sDrumkitDir ), true ) ) {
		ERRORLOG( QString( "Drumkit XML file [%1] can not be parsed." )
				  .arg( Filesystem::drumkit_file( sDrumkitDir ) ) );
		return false;
	}
	
	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( QString( "Drumkit file [%1] seems bricked: 'drumkit_info' node not found" )
				  .arg( Filesystem::drumkit_file( sDrumkitDir ) ) );
		return false;
	}

	if ( bLegacyFormatEncountered && ! bCheckLegacyVersions ) {
		ERRORLOG( QString( "Drumkit [%1] uses a legacy format" )
				  .arg( sDrumkitPath ) );
		return false;
	}

	// Trailing whitespaces will cause the Windows version to fail
	// extracting it.
	if ( sDrumkitDir.endsWith( " " ) ) {
		ERRORLOG( QString( "Drumkit folder [%1] must not end with a trailing whitespace" )
				  .arg( sDrumkitDir ) );
		return false;
	}

	// Trailing whitespace in drumkit name element
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

std::shared_ptr<Drumkit> CoreActionController::retrieveDrumkit(
	const QString& sDrumkitPath,
	bool* bIsCompressed,
	QString *sDrumkitDir,
	QString* sTemporaryFolder,
	bool* pLegacyFormatEncountered )
{
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
	if ( bIsCompressed == nullptr || sTemporaryFolder == nullptr ||
		 sDrumkitDir == nullptr || pLegacyFormatEncountered == nullptr ) {
		ERRORLOG( "Invalid input" );
		return nullptr;
	}

	*bIsCompressed = false;
	*sTemporaryFolder = "";
	*sDrumkitDir = "";
	*pLegacyFormatEncountered = false;

	QFileInfo sourceFileInfo( sDrumkitPath );

	if ( Filesystem::dir_readable( sDrumkitPath, true ) ) {

		// Providing the folder containing the drumkit
		pDrumkit = Drumkit::load(
			sDrumkitPath, false, pLegacyFormatEncountered, true );
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
		pDrumkit = Drumkit::load(
			sDrumkitDirPath, false, pLegacyFormatEncountered, true );
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
		QString sTemplateName( Filesystem::tmp_dir() + "/XXXXXX" );
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
		if ( ! Drumkit::install( sDrumkitPath, tmpDir.path(), sDrumkitDir,
								 nullptr, true ) ) {
			ERRORLOG( QString( "Unabled to extract provided drumkit [%1] into [%2]" )
					  .arg( sDrumkitPath ).arg( tmpDir.path() ) );
			return nullptr;
		}

		INFOLOG( QString( "Extracting drumkit [%1] into [%2]" )
				 .arg( sDrumkitPath ).arg( tmpDir.path() ) );

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

		pDrumkit = Drumkit::load(
			*sDrumkitDir, false, pLegacyFormatEncountered, true );
		
	} else {
		ERRORLOG( QString( "Provided source path [%1] does not point to a Hydrogen drumkit" )
				  .arg( sDrumkitPath ) );
		return nullptr;
	}

	return pDrumkit;
}

bool CoreActionController::extractDrumkit( const QString& sDrumkitPath,
										   const QString& sTargetDir,
										   QString* pInstalledPath,
										   bool* pEncodingIssuesDetected ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	// Ensure variables are always set/initialized.
	if ( pInstalledPath != nullptr ) {
		*pInstalledPath = "";
	}
	if ( pEncodingIssuesDetected != nullptr ) {
		*pEncodingIssuesDetected = false;
	}

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

	if ( ! Drumkit::install( sDrumkitPath, sTarget, pInstalledPath,
							 pEncodingIssuesDetected, true ) ) {
		ERRORLOG( QString( "Unabled to extract provided drumkit [%1] into [%2]" )
				  .arg( sDrumkitPath ).arg( sTarget ) );
		return false;
	}

	if ( bInstall ) {
		pHydrogen->getSoundLibraryDatabase()->updateDrumkits();
	}

	return true;
}

bool CoreActionController::addInstrument( std::shared_ptr<Instrument> pInstrument,
										  int nIndex ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready yet" );
		return false;
	}
	if ( pInstrument == nullptr ) {
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pDrumkit = pSong->getDrumkit();

	pAudioEngine->lock( RIGHT_HERE );

	// Ensure instrument isn't already in the death row.
	pHydrogen->removeInstrumentFromDeathRow( pInstrument );
	pInstrument->loadSamples( pAudioEngine->getTransportPosition()->getBpm() );

	pDrumkit->addInstrument( pInstrument, nIndex );
	pHydrogen->renameJackPorts( pSong, nullptr );
	pSong->getPatternList()->mapToDrumkit( pDrumkit, pDrumkit );

	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->pushEvent( Event::Type::DrumkitLoaded, 0 );

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

	pDrumkit->removeInstrument( pInstrument );

	// At this point the instrument has been removed from both the current
	// drumkit and every pattern in the song. But it still lives on as a shared
	// pointer in all Notes within the queues of the AudioEngine and Sampler.
	// Thus, it will be added to the death row, which guarantuees that its
	// samples will be unloaded once all notes referencing it are gone. Note
	// that this does not mean the instrument will be destructed. GUI can still
	// hold a shared pointer as part of an undo/redo Midiaction (that's why it is so
	// important to unload the samples).
	pHydrogen->addInstrumentToDeathRow( pInstrument );

	// Instead of letting all notes associated with this instrument ring till
	// the end, we discard those for which playback did not started yet and make
	// the remaining ones enter ADSR release phase.
	pAudioEngine->clearNoteQueues( pInstrument );
	pAudioEngine->getSampler()->releasePlayingNotes( pInstrument );

	const int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();
	if ( nSelectedInstrument == nInstrumentNumber ||
		 nSelectedInstrument >= pDrumkit->getInstruments()->size() ) {
		pHydrogen->setSelectedInstrumentNumber(
			std::clamp( nSelectedInstrument, 0,
						static_cast<int>(pDrumkit->getInstruments()->size() - 1 ) ),
			Event::Trigger::Suppress );
	}

	pHydrogen->renameJackPorts( pSong, nullptr );
	pSong->getPatternList()->mapToDrumkit( pDrumkit, pDrumkit );

	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->pushEvent( Event::Type::DrumkitLoaded, 0 );

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

	if ( pNewInstrument != nullptr ) {
		// Ensure instrument isn't already in the death row.
		pHydrogen->removeInstrumentFromDeathRow( pNewInstrument );
		pNewInstrument->loadSamples( fBpm );
	}

	pDrumkit->removeInstrument( pOldInstrument );

	// At this point the instrument has been removed from both the current
	// drumkit and every pattern in the song. But it still lives on as a shared
	// pointer in all Notes within the queues of the AudioEngine and Sampler.
	// Thus, it will be added to the death row, which guarantuees that its
	// samples will be unloaded once all notes referencing it are gone. Note
	// that this does not mean the instrument will be destructed. GUI can still
	// hold a shared pointer as part of an undo/redo Midiaction (that's why it is so
	// important to unload the samples).
	pHydrogen->addInstrumentToDeathRow( pOldInstrument );

	// Instead of letting all notes associated with this instrument ring till
	// the end, we discard those for which playback did not started yet and make
	// the remaining ones enter ADSR release phase.
	pAudioEngine->clearNoteQueues( pOldInstrument );
	pAudioEngine->getSampler()->releasePlayingNotes( pOldInstrument );

	pDrumkit->addInstrument( pNewInstrument,
							 nOldInstrumentNumber );
	pHydrogen->renameJackPorts( pSong, nullptr );
	pSong->getPatternList()->mapToDrumkit( pDrumkit, pDrumkit );

	// Unloading the samples of the old instrument will be done in the death
	// row.

	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->pushEvent( Event::Type::DrumkitLoaded, 0 );

	return true;
}

bool CoreActionController::moveInstrument( int nSourceIndex, int nTargetIndex ) {
	if ( nSourceIndex == nTargetIndex ) {
		return true;
	}

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return false;
	}

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	if ( nSourceIndex >= pInstrumentList->size() ||
		 nSourceIndex < 0 ) {
		ERRORLOG( QString( "Source index [%1] out of bound [0,%2)" )
				  .arg( nSourceIndex ).arg( pInstrumentList->size() ) );
		pHydrogen->getAudioEngine()->unlock();
		return false;
	}

	if ( nTargetIndex >= pInstrumentList->size() ||
		 nTargetIndex < 0 ) {
		ERRORLOG( QString( "Target index [%1] out of bound [0,%2)" )
				  .arg( nTargetIndex ).arg( pInstrumentList->size() ) );
		pHydrogen->getAudioEngine()->unlock();
		return false;
	}

	pInstrumentList->move( nSourceIndex, nTargetIndex );
	pHydrogen->renameJackPorts( pSong, nullptr );

	pHydrogen->getAudioEngine()->unlock();

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->pushEvent( Event::Type::DrumkitLoaded, 0 );

	return true;
}

bool CoreActionController::renameComponent(
	int nComponentId,
	const QString& sNewName
)
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	const auto pInstrument = pHydrogen->getSelectedInstrument();
	if ( pInstrument == nullptr ) {
		return false;
	}

	auto pComponent = pInstrument->getComponent( nComponentId );
	if ( pComponent == nullptr ) {
		ERRORLOG(
			QString( "Unable to retrieve component [%1]" ).arg( nComponentId )
		);
		return true;
	}

	pComponent->setName( sNewName );

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->pushEvent(
		Event::Type::SelectedInstrumentChanged, 0
	);

	return true;
}

bool CoreActionController::locateToColumn( int nColumn ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	if ( nColumn < -1 ) {
		ERRORLOG( QString( "Provided column [%1] too low. Using 0 instead." )
				  .arg( nColumn ) );
		nColumn = 0;
	}
	
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	long nTotalTick = pHydrogen->getTickForColumn( nColumn );
	if ( nTotalTick < 0 ) {
		if ( pHydrogen->getMode() == Song::Mode::Song ) {
			ERRORLOG( QString( "Provided column [%1] violates the allowed range [0;%2). No relocation done." )
					  .arg( nColumn )
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
    const auto pPref = Preferences::get_instance();

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );
    
	pAudioEngine->locate( nTick, bWithJackBroadcast );
	
	pAudioEngine->unlock();

    if ( pPref->getMidiTransportOutputSend() &&
         pPref->getMidiFeedbackChannel() != Midi::ChannelOff ) {
		auto pMidiDriver = pHydrogen->getMidiDriver();

		if ( pMidiDriver != nullptr ) {
			// A song position provided via MIDI has the lowest resolution of a
			// 1/16 note / 6 MIDI clocks. 24 MIDI clocks make a quarter.
			MidiMessage midiMessage;
			midiMessage.setType( MidiMessage::Type::SongPos );
			midiMessage.setData1( Midi::parameterFromIntClamp(
				pAudioEngine->getTransportPosition()->getTick() * 24 / 6 /
				H2Core::nTicksPerQuarter
			) );
			midiMessage.setChannel( pPref->getMidiFeedbackChannel() );
			pMidiDriver->enqueueOutputMessage( midiMessage );
		}
	}

	EventQueue::get_instance()->pushEvent( Event::Type::Relocation, 0 );
	return true;
}

bool CoreActionController::newPattern( const QString& sPatternName )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pPatternList = pHydrogen->getSong()->getPatternList();
	auto pPattern = std::make_shared<Pattern>( sPatternName );

	return setPattern( pPattern, pPatternList->size(), false );
}

std::shared_ptr<Pattern> CoreActionController::loadPattern( const QString& sPath
)
{
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen == nullptr ) {
		return nullptr;
	}

	auto pNewPattern = Pattern::load( sPath );
	if ( pNewPattern == nullptr ) {
		ERRORLOG( QString( "Unable to load pattern [%1]" ).arg( sPath ) );
		return nullptr;
	}

	return pNewPattern;
}

bool CoreActionController::setPattern(
	std::shared_ptr<Pattern> pNewPattern,
	int nPatternPosition, bool bReplace
)
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pNewPattern->mapToDrumkit( pSong->getDrumkit(), nullptr );
	auto pPatternList = pSong->getPatternList();

	pAudioEngine->lock( RIGHT_HERE );

    std::shared_ptr<Pattern> pOldPattern = nullptr;
	if ( bReplace ) {
		// In case we replace the pattern, we do now use PatternList::replace
		// directly but remove the previous one first before determining the
		// approriate name for the new pattern.
		pOldPattern = pPatternList->del( nPatternPosition );
	}

	// Check whether the name of the new pattern is unique.
	if ( !pPatternList->checkName( pNewPattern->getName() ) ) {
		pNewPattern->setName(
			pPatternList->findUnusedPatternName( pNewPattern->getName() )
		);
	}

	pPatternList->insert( nPatternPosition, pNewPattern );

	if ( bReplace && pOldPattern != nullptr ) {
		// There was already a pattern present. We have to replace all its
		// occurrences in the pattern group vector too.
		pAudioEngine->removePlayingPattern( pOldPattern );

		auto pPatternGroupVector = pSong->getPatternGroupVector();
		for ( auto& ppPatternList : *pPatternGroupVector ) {
			if ( ppPatternList == nullptr ) {
				continue;
			}
			const int nIndex = ppPatternList->index( pOldPattern );
			if ( nIndex != -1 ) {
				ppPatternList->replace( nIndex, pNewPattern );
			}
		}

		// Update virtual pattern presentation.
		for ( const auto& ppattern : *pPatternList ) {
			Pattern::virtual_patterns_cst_it_t it =
				ppattern->getVirtualPatterns()->find( pOldPattern );
			if ( it != ppattern->getVirtualPatterns()->end() ) {
				ppattern->virtualPatternsDel( *it );
			}
		}
	}

	if ( pHydrogen->isPatternEditorLocked() ) {
		pHydrogen->updateSelectedPattern( false );
	}
	else {
		pHydrogen->setSelectedPatternNumber(
			nPatternPosition, false, Event::Trigger::Default
		);
	}
	pAudioEngine->updatePlayingPatterns( Event::Trigger::Default );

	pHydrogen->updateSongSize();

	pAudioEngine->unlock();

	if ( bReplace ) {
		pHydrogen->updateVirtualPatterns( Event::Trigger::Suppress );
	}

	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->pushEvent(
		Event::Type::SelectedPatternChanged, 0
	);

	return true;
}

bool CoreActionController::selectPattern( int nPatternNumber )
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	const auto pPatternList = pSong->getPatternList();
	if ( nPatternNumber < 0 || nPatternNumber >= pPatternList->size() ) {
		ERRORLOG( QString( "Pattern number [%1] out of bound [0,%2]" )
					  .arg( nPatternNumber )
					  .arg( pPatternList->size() ) );
		return false;
	}

	if ( !( pHydrogen->isPatternEditorLocked() &&
			pHydrogen->getAudioEngine()->getState() ==
				AudioEngine::State::Playing ) ) {
		// Event handling will be done in Hydrogen::setSelectedPatternNumber.
		pHydrogen->setSelectedPatternNumber(
			nPatternNumber, true, Event::Trigger::Default
		);
	}

	return true;
}

bool CoreActionController::removePattern( int nPatternNumber )
{
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
		auto pEmptyPattern = std::make_shared<Pattern>( "Pattern 1" );
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

	std::shared_ptr<PatternList> pColumn;
	// Ensure there are no empty columns in the pattern group vector.
	for ( int ii = pPatternGroupVector->size() - 1; ii >= 0; --ii ) {
		pColumn = pPatternGroupVector->at( ii );
		if ( pColumn->size() == 0 ) {
			pPatternGroupVector->erase( pPatternGroupVector->begin() + ii );
		}
		else {
			break;
		}
	}

	if ( pHydrogen->isPatternEditorLocked() ) {
		pHydrogen->updateSelectedPattern( false );
	}
	else if ( nPatternNumber == nSelectedPatternNumber ) {
		pHydrogen->setSelectedPatternNumber(
			std::max( 0, nPatternNumber - 1 ), false, Event::Trigger::Default
		);
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
			ppattern->getVirtualPatterns()->find( pPattern );
		if ( it != ppattern->getVirtualPatterns()->end() ) {
			ppattern->virtualPatternsDel( *it );
		}
	}

	pHydrogen->updateVirtualPatterns();
	pHydrogen->setIsModified( true );

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

	pPattern->purgeInstrument( pInstrument, true );

	EventQueue::get_instance()->pushEvent( Event::Type::PatternModified, 0 );

	return true;
}

bool CoreActionController::toggleGridCell( const GridPoint& gridPoint ){
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	
	auto pSong = pHydrogen->getSong();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pPatternList = pSong->getPatternList();
	auto pColumns = pSong->getPatternGroupVector();

	if ( gridPoint.getRow() < 0 || gridPoint.getRow() > pPatternList->size() ) {
		ERRORLOG( QString( "Provided row [%1] is out of bound [0,%2]" )
				  .arg( gridPoint.getRow() ).arg( pPatternList->size() ) );
		return false;
	}
	
	auto pNewPattern = pPatternList->get( gridPoint.getRow() );
	if ( pNewPattern == nullptr ) {
		ERRORLOG( QString( "Unable to obtain Pattern in row [%1]." )
				  .arg( gridPoint.getRow() ) );

		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );
	if ( gridPoint.getColumn() >= 0 &&
		 gridPoint.getColumn() < pColumns->size() ) {
		auto pColumn = ( *pColumns )[ gridPoint.getColumn() ];
		auto pPattern = pColumn->del( pNewPattern );
		if ( pPattern == nullptr ) {
			// No pattern in this row. Let's add it.
			pColumn->add( pNewPattern );
		}
		else {
			// There was already a pattern present and we removed it.
			// Ensure that there are no empty columns at the end of
			// the song.
			for ( int ii = pColumns->size() - 1; ii >= 0; ii-- ) {
				auto pColumn = ( *pColumns )[ ii ];
				if ( pColumn->size() == 0 ) {
					pColumns->erase( pColumns->begin() + ii );
				}
				else {
					break;
				}
			}
		}
	}
	else if ( gridPoint.getColumn() >= pColumns->size() ) {
		// We need to add some new columns..
		std::shared_ptr<PatternList> pColumn;

		for ( int ii = 0; gridPoint.getColumn() - pColumns->size() + 1; ii++ ) {
			pColumn = std::make_shared<PatternList>();
			pColumns->push_back( pColumn );
		}
		pColumn->add( pNewPattern );
	}
	else {
		// gridPoint.getColumn() < 0
		ERRORLOG( QString( "Provided column [%1] is out of bound [0,%2]" )
				  .arg( gridPoint.getColumn() ).arg( pColumns->size() ) );
		pAudioEngine->unlock();
		return false;
	}
	
	pHydrogen->updateSongSize();
	pHydrogen->updateSelectedPattern( false );
	
	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );
	
	// Update the SongEditor.
	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::headless ) {
		EventQueue::get_instance()->pushEvent( Event::Type::GridCellToggled, 0 );
	}

	return true;
}

bool CoreActionController::handleNote(
	Midi::Note note,
	Midi::Channel channel,
	float fVelocity,
	bool bNoteOff,
	QStringList* pMappedInstruments
)
{
	const auto pPref = Preferences::get_instance();
	const auto pMidiInstrumentMap = pPref->getMidiInstrumentMap();
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return false;
	}
	const auto pInstrumentList = pSong->getDrumkit()->getInstruments();

	const auto mappedInstruments = pMidiInstrumentMap
		->mapInput( note, channel, pSong->getDrumkit() );
	QString sMode( MidiInstrumentMap::InputToQString(
		pMidiInstrumentMap->getInput() ) );

	// Some finishing touches and note playback.
	bool bSuccess = true;
	QStringList instrumentStrings;
	for ( const auto& ppInstrument : mappedInstruments ) {

		// Only look to change instrument if the current note is actually of hihat
		// and hihat openness is outside the instrument selected
		const auto hihatOpenness = pHydrogen->getHihatOpenness();
		int nCurrentInstrument = pInstrumentList->index( ppInstrument );
		if ( ppInstrument != nullptr && ppInstrument->getHihatGrp() >= 0 &&
			 ( hihatOpenness < ppInstrument->getLowerCc() ||
			   hihatOpenness > ppInstrument->getHigherCc() ) ) {

			for ( int ii = 0; ii <= pInstrumentList->size(); ii++ ) {
				auto ppOtherInstrument = pInstrumentList->get( ii );
				if ( ppOtherInstrument != nullptr &&
					 ppInstrument->getHihatGrp() ==
					   ppOtherInstrument->getHihatGrp() &&
					 hihatOpenness >= ppOtherInstrument->getLowerCc() &&
					 hihatOpenness <= ppOtherInstrument->getHigherCc() ) {

					nCurrentInstrument = ii;
					sMode = "Hihat Pressure Group";
					break;
				}
			}
		}

		if ( pHydrogen->addRealtimeNote(
				 nCurrentInstrument, fVelocity, bNoteOff, note ) ) {
			instrumentStrings << QString( "%1 (%2)" )
				.arg( ppInstrument->getName() ).arg( nCurrentInstrument );
		}
		else {
			bSuccess = false;
		}
	}

	INFOLOG( QString( "[%1] mapped note [%2] to instrument(s) [%3]" )
				 .arg( sMode )
				 .arg( static_cast<int>( note ) )
				 .arg( instrumentStrings.join( ", " ) ) );

	if ( pMappedInstruments != nullptr ) {
		*pMappedInstruments = instrumentStrings;
	}

	return bSuccess;
}

void CoreActionController::insertRecentFile( const QString& sFileName ){
	auto pPref = Preferences::get_instance();

	// The most recent file will always be added on top and possible
	// duplicates are removed later on.
	bool bAlreadyContained = false;

	QStringList recentFiles = pPref->getRecentFiles();

	// We have to normalize directory separators. Else opening a
	// song via double click from file browser and from within
    // Hydrogen will give to distinct entries on Windows.
    const QString sFileNameCleaned = QDir::cleanPath( sFileName );

    recentFiles.push_front( sFileNameCleaned );
	recentFiles.removeDuplicates();

	pPref->setRecentFiles( recentFiles );
}

bool CoreActionController::setBpm( float fBpm ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set yet" );
		return false;
	}

	if ( pHydrogen->getTempoSource() != Hydrogen::Tempo::Song ) {
		return false;
	}

	fBpm = std::clamp( fBpm, static_cast<float>(MIN_BPM),
						  static_cast<float>(MAX_BPM) );

	pAudioEngine->lock( RIGHT_HERE );
	// Use tempo in the next process cycle of the audio engine.
	pAudioEngine->setNextBpm( fBpm );

	// Store it's value in the .h2song file.
	pSong->setBpm( fBpm );
	if ( pSong->getTimeline() != nullptr ) {
		pSong->getTimeline()->setDefaultBpm( fBpm );
	}

	pAudioEngine->unlock();

	pHydrogen->setIsModified( true );

	return true;
}

bool CoreActionController::startCountIn() {
	auto pHydrogen = Hydrogen::get_instance();
	assert( pHydrogen );
	if ( pHydrogen == nullptr ) {
		ERRORLOG( "Core not ready yet!" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->startCountIn();
	pAudioEngine->unlock();

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
			pPlaylist->setFileName( sPath );
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

	if ( pPlaylist->getFileName() ==
		 Filesystem::empty_path( Filesystem::Type::Playlist ) ) {
		// To indicate that the user closed the previous playlsit in favor
		// of a new one, we store an empty string. This way the changes from
		// the empty playlist can be recovered.
		Preferences::get_instance()->setLastPlaylistFileName( "" );
	}
	else {
		Preferences::get_instance()->setLastPlaylistFileName(
			pPlaylist->getFileName() );
	}

	EventQueue::get_instance()->pushEvent( Event::Type::PlaylistChanged, 0 );

	// In case the playlist is read-only, autosave won't work.
	if ( ! Filesystem::file_writable( pPlaylist->getFileName() ) ) {
		WARNINGLOG( QString( "You don't have permissions to write to the playlist found in path [%1]. It will be opened as read-only (no autosave)." )
					.arg( pPlaylist->getFileName() ));
		EventQueue::get_instance()->pushEvent( Event::Type::PlaylistChanged, 2 );
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
	EventQueue::get_instance()->pushEvent( Event::Type::PlaylistChanged, 1 );
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

	Preferences::get_instance()->setLastPlaylistFileName( sPath );

	EventQueue::get_instance()->pushEvent( Event::Type::PlaylistChanged, 0 );

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
	EventQueue::get_instance()->pushEvent( Event::Type::PlaylistChanged, 0 );
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
	EventQueue::get_instance()->pushEvent( Event::Type::PlaylistChanged, 0 );
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
	EventQueue::get_instance()->pushEvent( H2Core::Event::Type::PlaylistLoadSong,
											nSongNumber );

	return true;
}

bool CoreActionController::sendAllNoteOffMessages()
{
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	const auto pPref = Preferences::get_instance();
	if ( pPref->getMidiSendNoteOff() != Preferences::MidiSendNoteOff::Always ) {
		return true;
	}

	const auto pMidiInstrumentMap = pPref->getMidiInstrumentMap();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Unable to send MIDI messages" );
		return false;
	}

	auto pMidiDriver = pHydrogen->getMidiDriver();
	if ( pMidiDriver == nullptr ) {
		return false;
	}

	MidiMessage::NoteOff noteOff;
	noteOff.velocity = Midi::ParameterMinimum;
	for ( const auto& ppInstrument : *pSong->getDrumkit()->getInstruments() ) {
		// Using a negative MIDI channel MIDI output can be deactivated per
		// instrument.
		if ( ppInstrument != nullptr ) {
			const auto noteRef =
				pMidiInstrumentMap->getOutputMapping( nullptr, ppInstrument );
			noteOff.channel = noteRef.channel;
			noteOff.note = noteRef.note;

			if ( noteOff.channel != Midi::ChannelOff ) {
				pMidiDriver->enqueueOutputMessage( MidiMessage::from( noteOff )
				);
			}
		}
	}

	return true;
}

bool CoreActionController::setMidiClockInputHandling( bool bHandle ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	auto pPref = Preferences::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return false;
	}

	if ( pPref->getMidiClockInputHandling() == bHandle ) {
		return false;
	}

	pPref->setMidiClockInputHandling( bHandle );

	EventQueue::get_instance()->pushEvent(
		H2Core::Event::Type::MidiClockActivation, 0 );

	if ( ! bHandle && pHydrogen->getTempoSource() == Hydrogen::Tempo::Song ) {
		// Restore the previous tempo.
		auto pAudioEngine = pHydrogen->getAudioEngine();
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setNextBpm( pSong->getBpm() );
		pAudioEngine->unlock();
	}

	return true;
}

bool CoreActionController::setMidiClockOutputSend( bool bHandle ) {
	auto pHydrogen = Hydrogen::get_instance();
	ASSERT_HYDROGEN

	auto pPref = Preferences::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return false;
	}

	if ( pPref->getMidiClockOutputSend() == bHandle ) {
		return false;
	}

	pPref->setMidiClockOutputSend( bHandle );

	// Jump start sending MIDI clock messages. Else they would only be send on
	// the next tempo change or start of the audio engine.
	auto pMidiDriver = pHydrogen->getAudioEngine()->getMidiDriver();
	if ( pMidiDriver != nullptr ) {
		if ( bHandle ) {
			pMidiDriver->startMidiClockStream(
				pHydrogen->getAudioEngine()->getTransportPosition()->getBpm() );
		}
		else {
			pMidiDriver->stopMidiClockStream();
		}
	}

	return true;
}
}
