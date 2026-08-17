/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitMap.h>
#include <core/Basics/GridPoint.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Note.h>
#include <core/Sampler/Sampler.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/IO/AlsaMidiDriver.h>
#include <core/IO/JackDriver.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Midi/MidiMessage.h>
#include <core/OscServer.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/Timeline.h>

#ifdef H2CORE_HAVE_OSC
#include <core/NsmClient.h>
#endif

namespace H2Core
{

CoreActionController::CoreActionController( Hydrogen* pHydrogen )
	: m_pHydrogen( pHydrogen ) {}


bool CoreActionController::setMasterVolume( float fMasterVolumeValue )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( pSong->getVolume() != fMasterVolumeValue ) {
		pSong->setVolume( fMasterVolumeValue );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::MixerSettingsChanged, 0
		);

		m_pHydrogen->setSongModified( true );
	}

	return sendMasterVolumeFeedback();
}

bool CoreActionController::setStripVolume(
	int nStrip,
	float fVolumeValue,
	bool bSelectStrip
)
{
	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		m_pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->getVolume() != fVolumeValue ) {
		pInstr->setVolume( fVolumeValue );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip
		);

		m_pHydrogen->setDrumkitModified( true );

		return sendStripVolumeFeedback( nStrip );
	}

	return true;
}

std::shared_ptr<Instrument> CoreActionController::resolveInstrument(
	int nInstrument ) const
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "no song or drumkit set" );
		return nullptr;
	}
	auto pInstrument = pSong->getDrumkit()->getInstruments()->get( nInstrument );
	if ( pInstrument == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument [%1]" )
				  .arg( nInstrument ) );
	}
	return pInstrument;
}

bool CoreActionController::setInstrumentPitch( int nInstrument, float fValue )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->getPitchOffset() != fValue ) {
		pInstrument->setPitchOffset( fValue );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument
		);
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );

	return true;
}

bool CoreActionController::setInstrumentGain( int nInstrument, float fValue )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->getGain() != fValue ) {
		pInstrument->setGain( fValue );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentRandomPitch( int nInstrument,
													 float fValue )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->getRandomPitchFactor() != fValue ) {
		pInstrument->setRandomPitchFactor( fValue );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentFilterCutoff( int nInstrument,
													  float fValue )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->getFilterCutoff() != fValue ) {
		pInstrument->setFilterCutoff( fValue );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentFilterResonance( int nInstrument,
														 float fValue )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->getFilterResonance() != fValue ) {
		pInstrument->setFilterResonance( fValue );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentAttack( int nInstrument, float fValue )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	const unsigned int nValue = static_cast<unsigned int>( fValue );
	if ( pInstrument->getAdsr()->getAttack() != nValue ) {
		pInstrument->getAdsr()->setAttack( nValue );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentDecay( int nInstrument, float fValue )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	const unsigned int nValue = static_cast<unsigned int>( fValue );
	if ( pInstrument->getAdsr()->getDecay() != nValue ) {
		pInstrument->getAdsr()->setDecay( nValue );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentSustain( int nInstrument, float fValue )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->getAdsr()->getSustain() != fValue ) {
		pInstrument->getAdsr()->setSustain( fValue );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentRelease( int nInstrument, float fValue )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	const unsigned int nValue = static_cast<unsigned int>( fValue );
	if ( pInstrument->getAdsr()->getRelease() != nValue ) {
		pInstrument->getAdsr()->setRelease( nValue );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentFilterActive( int nInstrument,
													  bool bActive )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->isFilterActive() != bActive ) {
		pInstrument->setFilterActive( bActive );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentMuteGroup( int nInstrument,
												   int nMuteGroup )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->getMuteGroup() != nMuteGroup ) {
		pInstrument->setMuteGroup( nMuteGroup );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentStopNotes( int nInstrument,
												   bool bStopNotes )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->isStopNotes() != bStopNotes ) {
		pInstrument->setStopNotes( bStopNotes );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentApplyVelocity( int nInstrument,
													   bool bApplyVelocity )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->getApplyVelocity() != bApplyVelocity ) {
		pInstrument->setApplyVelocity( bApplyVelocity );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentHihatGroup( int nInstrument,
												    int nHihatGroup )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	if ( pInstrument->getHihatGrp() != nHihatGroup ) {
		pInstrument->setHihatGrp( nHihatGroup );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentLowerCc( int nInstrument, int nCc )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	const auto param = Midi::parameterFromIntClamp( nCc );
	if ( pInstrument->getLowerCc() != param ) {
		pInstrument->setLowerCc( param );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentHigherCc( int nInstrument, int nCc )
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}
	const auto param = Midi::parameterFromIntClamp( nCc );
	if ( pInstrument->getHigherCc() != param ) {
		pInstrument->setHigherCc( param );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

std::shared_ptr<InstrumentComponent> CoreActionController::resolveComponent(
	int nInstrument, int nComponent ) const
{
	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return nullptr;
	}
	auto pComponent = pInstrument->getComponent( nComponent );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1] of instrument [%2]" )
				  .arg( nComponent ).arg( nInstrument ) );
	}
	return pComponent;
}

std::shared_ptr<InstrumentLayer> CoreActionController::resolveLayer(
	int nInstrument, int nComponent, int nLayer ) const
{
	auto pComponent = resolveComponent( nInstrument, nComponent );
	if ( pComponent == nullptr ) {
		return nullptr;
	}
	auto pLayer = pComponent->getLayer( nLayer );
	if ( pLayer == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve layer [%1] of component [%2] of instrument [%3]" )
				  .arg( nLayer ).arg( nComponent ).arg( nInstrument ) );
	}
	return pLayer;
}

bool CoreActionController::setComponentIsMuted( int nInstrument, int nComponent,
												bool bIsMuted )
{
	auto pComponent = resolveComponent( nInstrument, nComponent );
	if ( pComponent == nullptr ) {
		return false;
	}
	if ( pComponent->getIsMuted() != bIsMuted ) {
		pComponent->setIsMuted( bIsMuted );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setComponentIsSoloed( int nInstrument, int nComponent,
												 bool bIsSoloed )
{
	auto pComponent = resolveComponent( nInstrument, nComponent );
	if ( pComponent == nullptr ) {
		return false;
	}
	if ( pComponent->getIsSoloed() != bIsSoloed ) {
		pComponent->setIsSoloed( bIsSoloed );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setComponentGain( int nInstrument, int nComponent,
											 float fGain )
{
	auto pComponent = resolveComponent( nInstrument, nComponent );
	if ( pComponent == nullptr ) {
		return false;
	}
	if ( pComponent->getGain() != fGain ) {
		pComponent->setGain( fGain );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setComponentSelection( int nInstrument,
												  int nComponent, int nSelection )
{
	auto pComponent = resolveComponent( nInstrument, nComponent );
	if ( pComponent == nullptr ) {
		return false;
	}
	const auto selection =
		static_cast<InstrumentComponent::Selection>( nSelection );
	if ( pComponent->getSelection() != selection ) {
		pComponent->setSelection( selection );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setLayerIsMuted( int nInstrument, int nComponent,
											int nLayer, bool bIsMuted )
{
	auto pLayer = resolveLayer( nInstrument, nComponent, nLayer );
	if ( pLayer == nullptr ) {
		return false;
	}
	if ( pLayer->getIsMuted() != bIsMuted ) {
		pLayer->setIsMuted( bIsMuted );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setLayerIsSoloed( int nInstrument, int nComponent,
											 int nLayer, bool bIsSoloed )
{
	auto pLayer = resolveLayer( nInstrument, nComponent, nLayer );
	if ( pLayer == nullptr ) {
		return false;
	}
	if ( pLayer->getIsSoloed() != bIsSoloed ) {
		pLayer->setIsSoloed( bIsSoloed );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setLayerGain( int nInstrument, int nComponent,
										 int nLayer, float fGain )
{
	auto pLayer = resolveLayer( nInstrument, nComponent, nLayer );
	if ( pLayer == nullptr ) {
		return false;
	}
	if ( pLayer->getGain() != fGain ) {
		pLayer->setGain( fGain );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setLayerPitchOffset( int nInstrument, int nComponent,
												int nLayer, float fPitchOffset )
{
	auto pLayer = resolveLayer( nInstrument, nComponent, nLayer );
	if ( pLayer == nullptr ) {
		return false;
	}
	if ( pLayer->getPitchOffset() != fPitchOffset ) {
		pLayer->setPitchOffset( fPitchOffset );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setLayerStartVelocity( int nInstrument, int nComponent,
												  int nLayer, float fVelocity )
{
	auto pLayer = resolveLayer( nInstrument, nComponent, nLayer );
	if ( pLayer == nullptr ) {
		return false;
	}
	if ( pLayer->getStartVelocity() != fVelocity ) {
		pLayer->setStartVelocity( fVelocity );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setLayerEndVelocity( int nInstrument, int nComponent,
												int nLayer, float fVelocity )
{
	auto pLayer = resolveLayer( nInstrument, nComponent, nLayer );
	if ( pLayer == nullptr ) {
		return false;
	}
	if ( pLayer->getEndVelocity() != fVelocity ) {
		pLayer->setEndVelocity( fVelocity );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument );
		m_pHydrogen->setDrumkitModified( true );
	}
	m_pHydrogen->setSelectedInstrumentNumber( nInstrument );
	return true;
}

bool CoreActionController::setInstrumentMidiOutNote(
	int nInstrument,
	Midi::Note note,
	long nEventId
)
{
	auto pSong = m_pHydrogen->getSong();
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
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument, nEventId
		);
		m_pHydrogen->setDrumkitModified( true );
	}

	return true;
}

bool CoreActionController::setInstrumentMidiOutChannel(
	int nInstrument,
	Midi::Channel channel,
	long nEventId
)
{
	auto pSong = m_pHydrogen->getSong();
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
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nInstrument, nEventId
		);
		m_pHydrogen->setDrumkitModified( true );
	}

	return true;
}

bool CoreActionController::setMetronomeIsActive( bool isActive )
{
	auto pPref = m_pHydrogen->getPreferences();
	if ( pPref->m_bUseMetronome != isActive ) {
		pPref->m_bUseMetronome = isActive;

		m_pHydrogen->getEventQueue()->pushEvent( Event::Type::Metronome, 2 );

		return sendMetronomeIsActiveFeedback();
	}

	return true;
}

bool CoreActionController::setMasterIsMuted( bool bIsMuted )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( pSong->getIsMuted() != bIsMuted ) {
		pSong->setIsMuted( bIsMuted );

		m_pHydrogen->setSongModified( true );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::MixerSettingsChanged, 0
		);

		return sendMasterIsMutedFeedback();
	}

	return true;
}

bool CoreActionController::setHumanizeTime( float fValue )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( pSong->getHumanizeTimeValue() != fValue ) {
		pSong->setHumanizeTimeValue( fValue );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::MixerSettingsChanged, 0
		);

		m_pHydrogen->setSongModified( true );
	}

	return true;
}

bool CoreActionController::setHumanizeVelocity( float fValue )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( pSong->getHumanizeVelocityValue() != fValue ) {
		pSong->setHumanizeVelocityValue( fValue );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::MixerSettingsChanged, 0
		);

		m_pHydrogen->setSongModified( true );
	}

	return true;
}

bool CoreActionController::setSwing( float fValue )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( pSong->getSwingFactor() != fValue ) {
		pSong->setSwingFactor( fValue );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::MixerSettingsChanged, 0
		);

		m_pHydrogen->setSongModified( true );
	}

	return true;
}

bool CoreActionController::setPanLaw( int nPanLawType, float fPanLawKNorm )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pSong->setPanLawType( nPanLawType );
	pSong->setPanLawKNorm( fPanLawKNorm );

	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::MixerSettingsChanged, 0
	);
	m_pHydrogen->setSongModified( true );

	return true;
}

bool CoreActionController::previewInstrument( int nInstrument, bool bStop )
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return false;
	}

	auto pInstrument = resolveInstrument( nInstrument );
	if ( pInstrument == nullptr ) {
		return false;
	}

	auto pNote = std::make_shared<Note>(
		pInstrument, 0, bStop ? 0.0f : 1.0f );
	if ( bStop ) {
		pNote->setNoteOff( true );
	}
	m_pHydrogen->getAudioEngine()->getSampler()->noteOn( pNote );

	return true;
}

bool CoreActionController::noteOn( std::shared_ptr<Note> pNote )
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return false;
	}

	if ( pNote == nullptr ) {
		ERRORLOG( "Invalid note" );
		return false;
	}

	m_pHydrogen->getAudioEngine()->getSampler()->noteOn( pNote );
	return true;
}

bool CoreActionController::setPlaybackTrackMuted( bool bMuted )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	auto pInstrument = pSong->getPlaybackTrackInstrument();
	if ( pInstrument == nullptr ) {
		return false;
	}

	pInstrument->setMuted( bMuted );
	if ( pInstrument->getComponent( 0 ) != nullptr &&
		 pInstrument->getComponent( 0 )->getLayer( 0 ) != nullptr ) {
		pInstrument->getComponent( 0 )->getLayer( 0 )->setIsMuted( bMuted );
	}

	m_pHydrogen->setSongModified( true );
	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::PlaybackTrackParameterChanged, 0 );

	return true;
}

bool CoreActionController::setPlaybackTrackVolume( float fVolume )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	auto pInstrument = pSong->getPlaybackTrackInstrument();
	if ( pInstrument == nullptr ) {
		return false;
	}

	if ( pInstrument->getVolume() != fVolume ) {
		pInstrument->setVolume( fVolume );
		m_pHydrogen->setSongModified( true );
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::PlaybackTrackParameterChanged, 0 );
	}

	return true;
}

bool CoreActionController::toggleStripIsMuted( int nStrip )
{
	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	return setStripIsMuted( nStrip, !pInstr->isMuted(), false );
}

bool CoreActionController::setStripIsMuted(
	int nStrip,
	bool bIsMuted,
	bool bSelectStrip
)
{
	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		m_pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->isMuted() != bIsMuted ) {
		pInstr->setMuted( bIsMuted );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip
		);
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentMuteSoloChanged, nStrip
		);

		m_pHydrogen->setDrumkitModified( true );

		return sendStripIsMutedFeedback( nStrip );
	}

	return true;
}

bool CoreActionController::toggleStripIsSoloed( int nStrip )
{
	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	return setStripIsSoloed( nStrip, !pInstr->isSoloed(), false );
}

bool CoreActionController::setStripIsSoloed(
	int nStrip,
	bool isSoloed,
	bool bSelectStrip
)
{
	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		m_pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->isSoloed() != isSoloed ) {
		pInstr->setSoloed( isSoloed );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip
		);
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentMuteSoloChanged, nStrip
		);

		m_pHydrogen->setDrumkitModified( true );

		return sendStripIsSoloedFeedback( nStrip );
	}

	return true;
}

bool CoreActionController::setStripPan(
	int nStrip,
	float fValue,
	bool bSelectStrip
)
{
	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		m_pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->getPanWithRangeFrom0To1() != fValue ) {
		pInstr->setPanWithRangeFrom0To1( fValue );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip
		);

		m_pHydrogen->setDrumkitModified( true );

		return sendStripPanFeedback( nStrip );
	}

	return true;
}

bool CoreActionController::setStripPanSym(
	int nStrip,
	float fValue,
	bool bSelectStrip
)
{
	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr == nullptr ) {
		return false;
	}

	if ( bSelectStrip ) {
		m_pHydrogen->setSelectedInstrumentNumber( nStrip );
	}

	if ( pInstr->getPan() != fValue ) {
		pInstr->setPan( fValue );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::InstrumentParametersChanged, nStrip
		);

		m_pHydrogen->setDrumkitModified( true );

		return sendStripPanFeedback( nStrip );
	}

	return true;
}

bool CoreActionController::sendMasterVolumeFeedback()
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return true;
	}

	float fMasterVolume = pSong->getVolume();

#ifdef H2CORE_HAVE_OSC
	if ( m_pHydrogen->getPreferences()->getOscFeedbackEnabled() ) {
		m_pHydrogen->getOscServer()->sendFeedbackMessage(
			MidiAction::Type::MasterVolumeAbsolute, fMasterVolume, -1
		);
	}
#endif

	const auto pMidiEventMap = m_pHydrogen->getPreferences()->getMidiEventMap();

	auto ccParamValues =
		pMidiEventMap->findCCParameters( MidiAction::Type::MasterVolumeAbsolute
		);

	return handleOutgoingControlChanges(
		ccParamValues,
		Midi::parameterFromIntClamp(
			( fMasterVolume / 1.5 ) * static_cast<int>( Midi::ParameterMaximum )
		)
	);
}

bool CoreActionController::sendStripVolumeFeedback( int nStrip )
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return true;
	}

	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr != nullptr ) {
		float fStripVolume = pInstr->getVolume();

#ifdef H2CORE_HAVE_OSC
		if ( m_pHydrogen->getPreferences()->getOscFeedbackEnabled() ) {
			m_pHydrogen->getOscServer()->sendFeedbackMessage(
				MidiAction::Type::StripVolumeAbsolute, fStripVolume, nStrip + 1
			);
		}
#endif

		const auto pMidiEventMap =
			m_pHydrogen->getPreferences()->getMidiEventMap();

		auto ccParamValues = pMidiEventMap->findCCParameters(
			MidiAction::Type::StripVolumeAbsolute, nStrip, m_pHydrogen );

		return handleOutgoingControlChanges(
			ccParamValues, Midi::parameterFromIntClamp(
							   ( fStripVolume / 1.5 ) *
							   static_cast<int>( Midi::ParameterMaximum )
						   )
		);
	}

	return false;
}

bool CoreActionController::sendMetronomeIsActiveFeedback()
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return true;
	}

	const auto pPref = m_pHydrogen->getPreferences();

#ifdef H2CORE_HAVE_OSC
	if ( pPref->getOscFeedbackEnabled() ) {
		m_pHydrogen->getOscServer()->sendFeedbackMessage(
			MidiAction::Type::ToggleMetronome,
			static_cast<float>( pPref->m_bUseMetronome ), -1
		);
	}
#endif

	const auto pMidiEventMap = m_pHydrogen->getPreferences()->getMidiEventMap();

	auto ccParamValues =
		pMidiEventMap->findCCParameters( MidiAction::Type::ToggleMetronome );

	return handleOutgoingControlChanges(
		ccParamValues, Midi::parameterFromIntClamp(
						   static_cast<int>( pPref->m_bUseMetronome ) *
						   static_cast<int>( Midi::ParameterMaximum )
					   )
	);
}

bool CoreActionController::sendMasterIsMutedFeedback()
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return true;
	}

#ifdef H2CORE_HAVE_OSC
	if ( m_pHydrogen->getPreferences()->getOscFeedbackEnabled() ) {
		m_pHydrogen->getOscServer()->sendFeedbackMessage(
			MidiAction::Type::MuteToggle,
			static_cast<int>( pSong->getIsMuted() ), -1
		);
	}
#endif

	const auto pMidiEventMap = m_pHydrogen->getPreferences()->getMidiEventMap();

	auto ccParamValues =
		pMidiEventMap->findCCParameters( MidiAction::Type::MuteToggle );

	return handleOutgoingControlChanges(
		ccParamValues, Midi::parameterFromIntClamp(
						   static_cast<int>( pSong->getIsMuted() ) *
						   static_cast<int>( Midi::ParameterMaximum )
					   )
	);
}

bool CoreActionController::sendStripIsMutedFeedback( int nStrip )
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return true;
	}

	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr != nullptr ) {
#ifdef H2CORE_HAVE_OSC
		if ( m_pHydrogen->getPreferences()->getOscFeedbackEnabled() ) {
			m_pHydrogen->getOscServer()->sendFeedbackMessage(
				MidiAction::Type::StripMuteToggle,
				static_cast<float>( pInstr->isMuted() ), nStrip + 1
			);
		}
#endif

		const auto pMidiEventMap =
			m_pHydrogen->getPreferences()->getMidiEventMap();

		auto ccParamValues = pMidiEventMap->findCCParameters(
			MidiAction::Type::StripMuteToggle, nStrip, m_pHydrogen );

		return handleOutgoingControlChanges(
			ccParamValues, Midi::parameterFromIntClamp(
							   static_cast<int>( pInstr->isMuted() ) *
							   static_cast<int>( Midi::ParameterMaximum )
						   )
		);
	}

	return false;
}

bool CoreActionController::sendStripIsSoloedFeedback( int nStrip )
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return true;
	}

	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr != nullptr ) {
#ifdef H2CORE_HAVE_OSC
		if ( m_pHydrogen->getPreferences()->getOscFeedbackEnabled() ) {
			m_pHydrogen->getOscServer()->sendFeedbackMessage(
				MidiAction::Type::StripSoloToggle,
				static_cast<float>( pInstr->isSoloed() ), nStrip + 1
			);
		}
#endif

		const auto pMidiEventMap =
			m_pHydrogen->getPreferences()->getMidiEventMap();
		auto ccParamValues = pMidiEventMap->findCCParameters(
			MidiAction::Type::StripSoloToggle, nStrip, m_pHydrogen );

		return handleOutgoingControlChanges(
			ccParamValues, Midi::parameterFromIntClamp(
							   static_cast<int>( pInstr->isSoloed() ) *
							   static_cast<int>( Midi::ParameterMaximum )
						   )
		);
	}

	return false;
}

bool CoreActionController::sendStripPanFeedback( int nStrip )
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return true;
	}

	auto pInstr = resolveInstrument( nStrip );
	if ( pInstr != nullptr ) {
#ifdef H2CORE_HAVE_OSC
		if ( m_pHydrogen->getPreferences()->getOscFeedbackEnabled() ) {
			m_pHydrogen->getOscServer()->sendFeedbackMessage(
				MidiAction::Type::PanAbsolute,
				pInstr->getPanWithRangeFrom0To1(), nStrip + 1
			);
		}
#endif

		const auto pMidiEventMap =
			m_pHydrogen->getPreferences()->getMidiEventMap();
		auto ccParamValues = pMidiEventMap->findCCParameters(
			MidiAction::Type::PanAbsolute, nStrip, m_pHydrogen );

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
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return true;
	}

	const auto pPref = m_pHydrogen->getPreferences();
	if ( pPref->getMidiFeedbackChannel() == Midi::ChannelOff ) {
		return true;
	}
	else if ( pPref->getMidiFeedbackChannel() == Midi::ChannelInvalid ) {
		return false;
	}
	auto pMidiDriver = m_pHydrogen->getMidiDriver();

	if ( m_pHydrogen->getSong() == nullptr || pMidiDriver == nullptr ) {
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

bool CoreActionController::initExternalControlInterfaces()
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return true;
	}

	/*
	 * Push the current state of Hydrogen to the attached control interfaces
	 * (e.g. OSC clients)
	 */

	// MASTER_VOLUME_ABSOLUTE
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	sendMasterVolumeFeedback();

	// PER-INSTRUMENT/STRIP STATES
	auto pInstrList = pSong->getDrumkit()->getInstruments();
	for ( int ii = 0; ii < pInstrList->size(); ii++ ) {
		auto pInstr = pInstrList->get( ii );
		if ( pInstr != nullptr ) {
			// STRIP_VOLUME_ABSOLUTE
			sendStripVolumeFeedback( ii );

			// PAN_ABSOLUTE
			sendStripPanFeedback( ii );

			// STRIP_MUTE_TOGGLE
			sendStripIsMutedFeedback( ii );

			// SOLO
			sendStripIsSoloedFeedback( ii );
		}
	}

	// TOGGLE_METRONOME
	sendMetronomeIsActiveFeedback();

	// MUTE_TOGGLE
	sendMasterIsMutedFeedback();

	return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

std::shared_ptr<Song> CoreActionController::loadSong(
	const QString& sPath,
	const QString& sRecoverPath
)
{
	// Check whether the provided path is valid.
	if ( sPath != Filesystem::emptyPath( Filesystem::Artifact::Song ) &&
		 !Filesystem::isPathValid( Filesystem::Artifact::Song, sPath, true ) ) {
		// Filesystem::isPathValid takes care of the error log message.
		return nullptr;
	}

	std::shared_ptr<Song> pSong;
	if ( !sRecoverPath.isEmpty() &&
		 Filesystem::isPathValid(
			 Filesystem::Artifact::Song, sRecoverPath, true
		 ) ) {
		// Use an autosave file to load the playlist
		pSong = Song::load( sRecoverPath, false, m_pHydrogen );
		if ( pSong != nullptr ) {
			pSong->setPath( sPath );
		}
		else {
			ERRORLOG(
				QString(
					"Unable to recover changes from [%1]. Loading [%2] instead."
				)
					.arg( sRecoverPath )
					.arg( sPath )
			);
		}
	}

	if ( pSong == nullptr ) {
		pSong = Song::load( sPath, false, m_pHydrogen );
	}

	if ( pSong == nullptr ) {
		ERRORLOG( QString( "Unable to open song [%1]." ).arg( sPath ) );
		return nullptr;
	}

	return pSong;
}

bool CoreActionController::setSong( std::shared_ptr<Song> pSong )
{
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return false;
	}

	if ( m_pHydrogen->getAudioEngine()->getState() ==
		 AudioEngine::State::Playing ) {
		// Stops recording, all queued MIDI notes, and the playback of
		// the audio driver.
		m_pHydrogen->sequencerStop();
	}

	// Update the Song.
	m_pHydrogen->setSong( pSong );

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->getSampler()->clearLastUsedLayers();
	pAudioEngine->unlock();

	if ( m_pHydrogen->isUnderSessionManagement() ) {
		m_pHydrogen->restartAudioDriver();
	}
	else {
		// Add the new loaded song in the "last used song" vector.
		// This behavior is prohibited under session management. Only
		// songs open during normal runs will be listed. In addition,
		// empty songs - created and set when hitting "New Song" in
		// the main menu - aren't listed either.

		if ( pSong->getPath() ==
			 Filesystem::emptyPath( Filesystem::Artifact::Song ) ) {
			// To indicate that the user closed the previous song in favor of a
			// new one, we store an empty string. This way the changes from the
			// empty song can be recovered.
			m_pHydrogen->getPreferences()->setLastSongPath( "" );
		}
		else {
			insertRecentFile( pSong->getPath() );
			m_pHydrogen->getPreferences()->setLastSongPath( pSong->getPath() );
		}
	}

	// Be sure to not make GUI render its content twice by triggering this
	// during startup.
	if ( m_pHydrogen->isFullyOperational() ) {
		m_pHydrogen->getEventQueue()->pushEvent( Event::Type::UpdateSong, 0 );
	}

	// In case the song is read-only, autosave won't work.
	if ( !Filesystem::fileWritable( pSong->getPath(), true ) ) {
		WARNINGLOG(
			QString( "You don't have permissions to write to the song found in "
					 "path [%1]. It will be opened as read-only (no autosave)."
			)
				.arg( pSong->getPath() )
		);
		m_pHydrogen->getEventQueue()->pushEvent( Event::Type::UpdateSong, 2 );
	}

	// As we just set a fresh song, we can mark it not modified
	m_pHydrogen->setSongModified( false );

	return true;
}

bool CoreActionController::saveSong( bool bKeepMissingSamples )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	// Extract the path to the associate .h2song file.
	const QString sSongPath = pSong->getPath();
	if ( sSongPath.isEmpty() ) {
		ERRORLOG( "Unable to save song. Empty path!" );
		return false;
	}

	const bool bHadMissingSamples = pSong->hasMissingSamples();

	// Actual saving
	if ( ! pSong->save( sSongPath, bKeepMissingSamples, true ) ) {
		ERRORLOG(
			QString( "Current song [%1] could not be saved!" ).arg( sSongPath )
		);
		return false;
	}

	// Update the status bar.
	if ( m_pHydrogen->getProcessMode() != H2Core::ProcessMode::Headless ) {
		if ( !bKeepMissingSamples && bHadMissingSamples ) {
			// Some instrument layers might have been discarded. Reload the
			// entire drumkit.
			m_pHydrogen->getEventQueue()->pushEvent( Event::Type::UpdateSong, 0 );
		}
		else {
			m_pHydrogen->getEventQueue()->pushEvent( Event::Type::UpdateSong, 1 );
		}
	}

	return true;
}

bool CoreActionController::saveSongAs(
	const QString& sNewPath,
	bool bKeepMissingSamples
)
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	// Check whether the provided path is valid.
	if ( !Filesystem::isPathValid(
			 Filesystem::Artifact::Song, sNewPath
		 ) ) {
		// Filesystem::isPathValid takes care of the error log message.
		return false;
	}

	pSong->setPath( sNewPath );

	// Actual saving
	if ( !saveSong( bKeepMissingSamples ) ) {
		// In case saving failed, we try to hint possible reasons. E.g. that the
		// user has not sufficient permissions to write the selected folder.
		if ( !Filesystem::fileWritable( sNewPath ) ) {
			ERRORLOG(
				QString( "Song can not be written to read-only location [%1]" )
					.arg( sNewPath )
			);
		}
		return false;
	}

	// Update the recentFiles list by replacing the former file name
	// with the new one.
	insertRecentFile( sNewPath );
	if ( !m_pHydrogen->isUnderSessionManagement() ) {
		m_pHydrogen->getPreferences()->setLastSongPath( pSong->getPath() );
	}

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::UpdateSong, 1 );

	return true;
}

std::shared_ptr<Preferences> CoreActionController::loadPreferences(
	const QString& sPath
)
{
	return Preferences::load( sPath, false, m_pHydrogen );
}

bool CoreActionController::setPreferences(
	std::shared_ptr<Preferences> pPreferences
)
{
	if ( pPreferences == nullptr ) {
		ERRORLOG( "invalid preferences" );
		return false;
	}

	auto pAudioEngine = m_pHydrogen->getAudioEngine();

	m_pHydrogen->setPreferences( pPreferences );

	pAudioEngine->getMetronomeInstrument()->setVolume(
		pPreferences->m_fMetronomeVolume
	);

	m_pHydrogen->restartAudioDriver();
	m_pHydrogen->restartMidiDriver();
	m_pHydrogen->recreateOscServer();

	// If the GUI is active, we have to update it to reflect the
	// changes in the preferences.
	if ( m_pHydrogen->isFullyOperational() ) {
		m_pHydrogen->getEventQueue()->pushEvent(
			H2Core::Event::Type::UpdatePreferences, 1
		);
	}

	return true;
}

bool CoreActionController::savePreferences()
{

	if ( m_pHydrogen->getProcessMode() != H2Core::ProcessMode::Headless ) {
		// Update the status bar and let the GUI save the preferences
		// (after writing its current settings to disk).
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::UpdatePreferences, 0
		);
		return true;
	}

	return m_pHydrogen->getPreferences()->save();
}

bool CoreActionController::quit()
{
	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::Quit, 0 );

	return true;
}

bool CoreActionController::panic()
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );
	m_pHydrogen->sequencerStop();
	pAudioEngine->getSampler()->stopPlayingNotes();
	pAudioEngine->unlock();

	if ( pAudioEngine->getMidiDriver() != nullptr ) {
		pAudioEngine->getMidiDriver()->sendAllNotesOff();
	}

	return true;
}

bool CoreActionController::toggleTimeline()
{
	if ( m_pHydrogen->isTimelineEnabled() ) {
		activateTimeline( false );
	}
	else {
		activateTimeline( true );
	}

	return true;
}

bool CoreActionController::activateTimeline( bool bActivate )
{

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	m_pHydrogen->setIsTimelineActivated( bActivate );

	const auto tempoSource = m_pHydrogen->getTempoSource();
	if ( tempoSource == Hydrogen::Tempo::Jack ) {
		WARNINGLOG(
			QString( "Timeline usage was [%1] in the Preferences. But these "
					 "changes won't have an effect as long as there is still "
					 "an external JACK Timebase controller." )
				.arg( bActivate ? "enabled" : "disabled" )
		);
	}
	else if ( tempoSource == Hydrogen::Tempo::Midi ) {
		WARNINGLOG( QString( "Timeline usage was [%1] in the Preferences. But "
							 "these changes won't have an effect as long as "
							 "MIDI clock handling is enabled." )
						.arg( bActivate ? "enabled" : "disabled" ) );
	}
	else if ( m_pHydrogen->getMode() == Song::Mode::Pattern ) {
		WARNINGLOG( QString( "Timeline usage was [%1] in the Preferences. But "
							 "these changes won't have an effect as long as "
							 "Pattern Mode is still activated." )
						.arg( bActivate ? "enabled" : "disabled" ) );
	}

	return true;
}

bool CoreActionController::addTempoMarker( int nPosition, float fBpm )
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	auto pTimeline = m_pHydrogen->getSong()->getTimeline();

	if ( pTimeline->hasColumnTempoMarker( nPosition ) ) {
		const auto pPreviousMarker =
			pTimeline->getTempoMarkerAtColumn( nPosition, m_pHydrogen );
		if ( fBpm == pPreviousMarker->fBpm ) {
			// Markers is already present. Nothing to do.
			return true;
		}
	}
	pAudioEngine->lock( RIGHT_HERE );

	pTimeline->addTempoMarker( nPosition, fBpm );
	m_pHydrogen->getAudioEngine()->handleTimelineChange();

	pAudioEngine->unlock();

	m_pHydrogen->setSongModified( true );

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::UpdateTimeline, 0 );

	return true;
}

bool CoreActionController::deleteTempoMarker( int nPosition )
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( !m_pHydrogen->getSong()->getTimeline()->hasColumnTempoMarker( nPosition
		 ) ) {
		// Nothing to do
		return true;
	}

	pAudioEngine->lock( RIGHT_HERE );

	m_pHydrogen->getSong()->getTimeline()->deleteTempoMarker( nPosition );
	m_pHydrogen->getAudioEngine()->handleTimelineChange();

	pAudioEngine->unlock();

	m_pHydrogen->setSongModified( true );
	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::UpdateTimeline, 0 );

	return true;
}

bool CoreActionController::addTag( int nPosition, const QString& sText )
{

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	auto pTimeline = m_pHydrogen->getSong()->getTimeline();

	pTimeline->deleteTag( nPosition );
	pTimeline->addTag( nPosition, sText );

	m_pHydrogen->setSongModified( true );

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::UpdateTimeline, 0 );

	return true;
}

bool CoreActionController::deleteTag( int nPosition )
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	m_pHydrogen->getSong()->getTimeline()->deleteTag( nPosition );

	m_pHydrogen->setSongModified( true );
	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::UpdateTimeline, 0 );

	return true;
}

bool CoreActionController::toggleJackTransport()
{
	if ( m_pHydrogen->getPreferences()->m_nJackTransportMode ==
		 Preferences::USE_JACK_TRANSPORT ) {
		activateJackTransport( false );
	}
	else {
		activateJackTransport( true );
	}

	return true;
}

bool CoreActionController::activateJackTransport( bool bActivate )
{

#ifdef H2CORE_HAVE_JACK
	if ( !m_pHydrogen->hasJackDriver() ) {
		ERRORLOG(
			"Unable to (de)activate Jack transport. Please select the Jack "
			"driver first."
		);
		return false;
	}

	m_pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	if ( bActivate ) {
		m_pHydrogen->getPreferences()->m_nJackTransportMode =
			Preferences::USE_JACK_TRANSPORT;
	}
	else {
		m_pHydrogen->getPreferences()->m_nJackTransportMode =
			Preferences::NO_JACK_TRANSPORT;
	}
	m_pHydrogen->getAudioEngine()->unlock();

	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::JackTransportActivation, static_cast<int>( bActivate )
	);

	return true;
#else
	ERRORLOG(
		"Unable to (de)activate Jack transport. Your Hydrogen version was not "
		"compiled with jack support."
	);
	return false;
#endif
}

bool CoreActionController::toggleJackTimebaseControl()
{
	if ( m_pHydrogen->getPreferences()->m_bJackTimebaseMode ==
		 Preferences::USE_JACK_TIMEBASE_CONTROL ) {
		activateJackTimebaseControl( false );
	}
	else {
		activateJackTimebaseControl( true );
	}

	return true;
}

bool CoreActionController::activateJackTimebaseControl( bool bActivate )
{

#ifdef H2CORE_HAVE_JACK
	if ( bActivate ) {
		m_pHydrogen->getPreferences()->m_bJackTimebaseMode =
			Preferences::USE_JACK_TIMEBASE_CONTROL;
	}
	else {
		m_pHydrogen->getPreferences()->m_bJackTimebaseMode =
			Preferences::NO_JACK_TIMEBASE_CONTROL;
	}

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	if ( pAudioEngine->getAudioDriver() == nullptr ||
		 m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return false;
	}
	auto pJackDriver =
		std::dynamic_pointer_cast<JackDriver>( pAudioEngine->getAudioDriver() );
	if ( pJackDriver == nullptr ) {
		ERRORLOG(
			"Unable to (de)activate JACK Timebase support. Please select the "
			"JACK driver first."
		);
		return false;
	}

	if ( m_pHydrogen->getPreferences()->m_nJackTransportMode ==
		 Preferences::USE_JACK_TRANSPORT ) {
		pAudioEngine->lock( RIGHT_HERE );
		if ( bActivate ) {
			pJackDriver->initTimebaseControl();
		}
		else {
			pJackDriver->releaseTimebaseControl();
		}
		pAudioEngine->unlock();
	}

	return true;
#else
	ERRORLOG(
		"Unable to (de)activate JACK Timebase support. Your Hydrogen version "
		"was not compiled with JACK support."
	);
	return false;
#endif
}

bool CoreActionController::toggleSongMode()
{
	if ( m_pHydrogen->getMode() == Song::Mode::Song ) {
		activateSongMode( false );
	}
	else {
		activateSongMode( true );
	}

	return true;
}

bool CoreActionController::activateSongMode( bool bActivate )
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	auto pSong = m_pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	if ( !( bActivate && m_pHydrogen->getMode() != Song::Mode::Song ) &&
		 !( !bActivate && m_pHydrogen->getMode() != Song::Mode::Pattern ) ) {
		// No changes.
		return true;
	}

	m_pHydrogen->sequencerStop();

	pAudioEngine->lock( RIGHT_HERE );

	if ( bActivate && m_pHydrogen->getMode() != Song::Mode::Song ) {
		m_pHydrogen->setMode( Song::Mode::Song, Event::Trigger::Default );
	}
	else if ( !bActivate && m_pHydrogen->getMode() != Song::Mode::Pattern ) {
		m_pHydrogen->setMode( Song::Mode::Pattern, Event::Trigger::Default );
	}

	if ( m_pHydrogen->getSelectedPatternNumber() == -1 ) {
		m_pHydrogen->setSelectedPatternNumber(
			0, false, Event::Trigger::Suppress
		);
	}

	pAudioEngine->handleSongModeChanged( Event::Trigger::Suppress );

	pAudioEngine->unlock();

	return true;
}

bool CoreActionController::toggleLoopMode()
{
	auto pSong = m_pHydrogen->getSong();
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

bool CoreActionController::activateLoopMode( bool bActivate )
{
	auto pSong = m_pHydrogen->getSong();
	auto pAudioEngine = m_pHydrogen->getAudioEngine();

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	bool bChange = false;

	if ( bActivate && pSong->getLoopMode() != Song::LoopMode::Enabled ) {
		pSong->setLoopMode( Song::LoopMode::Enabled );
		bChange = true;
	}
	else if ( !bActivate && pSong->getLoopMode() == Song::LoopMode::Enabled ) {
		// If the transport was already looped at least once, disabling
		// loop mode will result in immediate stop. Instead, we want to
		// stop transport at the end of the song.
		if ( pSong->lengthInTicks() < pAudioEngine->getPlayhead()->getTick() ) {
			pSong->setLoopMode( Song::LoopMode::Finishing );
		}
		else {
			pSong->setLoopMode( Song::LoopMode::Disabled );
		}
		bChange = true;
	}

	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->handleLoopModeChanged();
	pAudioEngine->unlock();

	if ( bChange ) {
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::LoopModeActivation, static_cast<int>( bActivate )
		);
	}

	return true;
}

bool CoreActionController::activateRecordMode( bool bActivate )
{

	if ( m_pHydrogen->getRecordEnabled() != bActivate ) {
		m_pHydrogen->setRecordEnabled( bActivate );

		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::RecordModeChanged, static_cast<int>( bActivate )
		);
	}

	return true;
}

bool CoreActionController::toggleRecordMode()
{

	return activateRecordMode( !m_pHydrogen->getRecordEnabled() );
}

bool CoreActionController::setDrumkit( std::shared_ptr<Drumkit> pNewDrumkit )
{
	if ( pNewDrumkit == nullptr ) {
		ERRORLOG( "Provided Drumkit is not valid" );
		return false;
	}

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	auto pPreviousDrumkit = pSong->getDrumkit();

	QString sNewDrumkitLog;
	if ( !pNewDrumkit->getPath().isEmpty() ) {
		sNewDrumkitLog = QString( "[%1] located at [%2]" )
							 .arg( pNewDrumkit->getName() )
							 .arg( pNewDrumkit->getPath() );
	}
	else {
		sNewDrumkitLog = QString( "new drumkit" );
	}

	if ( pPreviousDrumkit == nullptr ) {
		INFOLOG( QString( "Setting drumkit %1" ).arg( sNewDrumkitLog ) );
	}
	else {
		INFOLOG( QString( "Switching drumkits [%1] -> %2" )
					 .arg( pPreviousDrumkit->getName() )
					 .arg( sNewDrumkitLog ) );
	}

	// Ensure instruments of the new kit aren't already in the death row.
	for ( const auto& ppInstrument : *pNewDrumkit->getInstruments() ) {
		m_pHydrogen->removeInstrumentFromDeathRow( ppInstrument );
	}

	// It would be more clean to lock the audio engine _before_ loading
	// the samples. We might pass a tempo marker while loading and users
	// of Rubberband end up with a wrong sample length. But this is an
	// edge-case and the regular user will benefit from a load prior to
	// the locking resulting in lesser XRUNs.
	pNewDrumkit->loadSamples( pAudioEngine->getPlayhead()->getBpm(),
							  m_pHydrogen->getPreferences().get() );

	pAudioEngine->lock( RIGHT_HERE );

	// Add all instruments of the previous drumkit to the death row. This way
	// all notes in audio engine and sampler queue can be rendered till they are
	// done. Unloading their samples will be done at a latter point.
	if ( pPreviousDrumkit != nullptr ) {
		for ( const auto& ppInstrument : *pPreviousDrumkit->getInstruments() ) {
			m_pHydrogen->addInstrumentToDeathRow( ppInstrument );
		}
	}

	// Instead of letting all notes associated with this instrument ring till
	// the end, we discard those for which playback did not started yet and make
	// the remaining ones enter ADSR release phase.
	if ( m_pHydrogen->getProcessMode() != H2Core::ProcessMode::Editor ) {
		pAudioEngine->clearNoteQueues();
		pAudioEngine->getSampler()->releasePlayingNotes();
		pAudioEngine->getSampler()->clearLastUsedLayers();
	}

	pSong->setDrumkit( pNewDrumkit );
	pSong->getPatternList()->mapToDrumkit( pNewDrumkit, pPreviousDrumkit );

	m_pHydrogen->renamePerTrackJackAudioPorts( pSong, pPreviousDrumkit );

	if ( m_pHydrogen->getSelectedInstrumentNumber() >=
		 pNewDrumkit->getInstruments()->size() ) {
		m_pHydrogen->setSelectedInstrumentNumber(
			std::max( 0, pNewDrumkit->getInstruments()->size() - 1 ),
			Event::Trigger::Suppress
		);
	}

	pAudioEngine->unlock();

	initExternalControlInterfaces();

	m_pHydrogen->setSongModified( true );

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::DrumkitLoaded, 0 );

	return true;
}

bool CoreActionController::upgradeDrumkit(
	const QString& sDrumkitDirOrXml,
	const QString& sNewDir
)
{
	if ( sNewDir.isEmpty() ) {
		INFOLOG(
			QString( "Upgrading kit at [%1] inplace." ).arg( sDrumkitDirOrXml )
		);
	}
	else {
		INFOLOG( QString( "Upgrading kit at [%1] into [%2]." )
					 .arg( sDrumkitDirOrXml )
					 .arg( sNewDir ) );
	}

	QFileInfo sourceFileInfo( sDrumkitDirOrXml );
	if ( !sNewDir.isEmpty() ) {
		// Check whether there is already a file or directory
		// present. The latter has to be writable. If none is present,
		// create a folder.
		if ( !Filesystem::pathUsable( sNewDir, true, false ) ) {
			return false;
		}
	}
	else {
		// We have to assure that the source folder is not just
		// readable since an inplace upgrade was requested
		if ( !Filesystem::dirWritable(
				 sourceFileInfo.dir().absolutePath(), true
			 ) ) {
			ERRORLOG( QString( "Unable to upgrade drumkit [%1] in place: "
							   "Folder is in read-only mode" )
						  .arg( sDrumkitDirOrXml ) );
			return false;
		}
	}

	QString sTemporaryFolder, sDrumkitDir;
	// Whether the drumkit was provided as compressed .h2drumkit file.
	bool bIsCompressed, bLegacyFormatEncountered;
	auto pDrumkit = retrieveDrumkit(
		sDrumkitDirOrXml, &bIsCompressed, &sDrumkitDir, &sTemporaryFolder,
		&bLegacyFormatEncountered
	);

	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to load drumkit from source path [%1]" )
					  .arg( sDrumkitDirOrXml ) );
		return false;
	}

	// If the drumkit is not updated inplace, we also need to copy
	// all samples and metadata, like images.
	QString sPath;
	if ( !sNewDir.isEmpty() ) {
		// When dealing with a compressed drumkit, we can just leave
		// it in the temporary folder and copy the compressed content
		// to the destination right away.
		if ( !bIsCompressed ) {
			// Copy content
			QDir drumkitDir( sDrumkitDir );
			for ( const auto& ssFile : drumkitDir.entryList( QDir::Files ) ) {
				// We handle the drumkit file later
				if ( ssFile.contains( ".xml" ) ) {
					continue;
				}
				Filesystem::fileCopy(
					drumkitDir.absolutePath() + "/" + ssFile,
					sNewDir + "/" + ssFile, true, true
				);
			}
			sPath = sNewDir;
		}
		else {
			sPath = sDrumkitDir;
		}
	}
	else {
		// Upgrade inplace.
		if ( !bIsCompressed ) {
			const auto sDrumkitPath =
				Filesystem::sanitizeDrumkitPath( sDrumkitDir );
			if ( sDrumkitPath.isEmpty() ) {
				ERRORLOG( QString( "Upgrade failed. Invalid drumkit dir [%1]" )
							  .arg( sDrumkitDir ) );
				return false;
			}

			// Make a backup of the original file in order to make the
			// upgrade reversible.
			QString sBackupPath = Filesystem::drumkitBackupPath( sDrumkitPath );
			if ( !Filesystem::fileCopy(
					 sDrumkitPath, sBackupPath, true, true
				 ) ) {
				ERRORLOG(
					QString(
						"Unable to backup source drumkit XML file from [%1] to "
						"[%2]. We abort instead of overwriting things."
					)
						.arg( sDrumkitPath )
						.arg( sBackupPath )
				);
				return false;
			}
		}
		else {
			QString sBackupPath =
				Filesystem::drumkitBackupPath( sDrumkitDirOrXml );
			if ( !Filesystem::fileCopy(
					 sDrumkitDirOrXml, sBackupPath, true, true
				 ) ) {
				ERRORLOG(
					QString(
						"Unable to backup source .h2drumkit file from [%1] to "
						"[%2]. We abort instead of overwriting things."
					)
						.arg( sDrumkitDirOrXml )
						.arg( sBackupPath )
				);
				return false;
			}
		}

		sPath = sDrumkitDir;
	}

	// For backward compatibility absolute paths to extracted drumkits are still
	// pointing to the overall drumkit folder when handed over via CLI. But
	// internally, a path to a drumkit is now expected to be the absolute path
	// to its drumkit.xml file.
	sPath = Filesystem::sanitizeDrumkitPath( sPath );
	if ( sPath.isEmpty() ) {
		ERRORLOG( "Upgrade failed. Invalid drumkit path" );
		return false;
	}

	if ( !pDrumkit->save( sPath, true ) ) {
		ERRORLOG(
			QString( "Error while saving upgraded kit to [%1]" ).arg( sPath )
		);
		return false;
	}

	// Compress the updated drumkit again in order to provide the same
	// format handed over as input.
	if ( bIsCompressed ) {
		QString sExportPath;
		if ( !sNewDir.isEmpty() ) {
			sExportPath = sNewDir;
		}
		else {
			sExportPath = sourceFileInfo.dir().absolutePath();
		}

		if ( !pDrumkit->exportTo( sExportPath, nullptr, false ) ) {
			ERRORLOG( QString( "Unable to export upgrade drumkit to [%1]" )
						  .arg( sExportPath ) );
			return false;
		}

		INFOLOG( QString( "Upgraded drumkit exported as [%1]" )
					 .arg(
						 sExportPath + "/" + pDrumkit->getName() +
						 Filesystem::sDrumkitSuffix
					 ) );
	}

	// Upgrade was successful. Cleanup
	if ( !sTemporaryFolder.isEmpty() ) {
		Filesystem::rm( sTemporaryFolder, true, true );
	}

	INFOLOG(
		QString( "Drumkit [%1] successfully upgraded!" ).arg( sDrumkitDirOrXml )
	);

	return true;
}

bool CoreActionController::validateDrumkit(
	const QString& sDrumkitDirOrXml,
	bool bCheckLegacyVersions
)
{

	INFOLOG( QString( "Validating kit [%1]" ).arg( sDrumkitDirOrXml ) );

	QString sTemporaryFolder, sDrumkitDir;
	// Whether the drumkit was provided as compressed .h2drumkit file.
	bool bIsCompressed, bLegacyFormatEncountered;
	const auto pDrumkit = retrieveDrumkit(
		sDrumkitDirOrXml, &bIsCompressed, &sDrumkitDir, &sTemporaryFolder,
		&bLegacyFormatEncountered
	);

	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to load drumkit from source path [%1]" )
					  .arg( sDrumkitDirOrXml ) );
		return false;
	}

	const QString sDrumkitPath = Filesystem::drumkitPathFromDir( sDrumkitDir );
	if ( !Filesystem::fileReadable( sDrumkitPath ) ) {
		ERRORLOG( QString( "Something went wrong in the drumkit retrieval of "
						   "[%1]. Unable to load from [%2]" )
					  .arg( sDrumkitDirOrXml )
					  .arg( sDrumkitDir ) );
		return false;
	}

	XMLDoc doc;
	if ( !doc.read( sDrumkitPath, true ) ) {
		ERRORLOG( QString( "Drumkit XML file [%1] can not be parsed." )
					  .arg( sDrumkitPath ) );
		return false;
	}

	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG(
			QString(
				"Drumkit file [%1] seems bricked: 'drumkit_info' node not found"
			)
				.arg( sDrumkitPath )
		);
		return false;
	}

	if ( bLegacyFormatEncountered && !bCheckLegacyVersions ) {
		ERRORLOG( QString( "Drumkit [%1] uses a legacy format" )
					  .arg( sDrumkitDirOrXml ) );
		return false;
	}

	// Trailing whitespaces will cause the Windows version to fail
	// extracting it.
	if ( sDrumkitDir.endsWith( " " ) ) {
		ERRORLOG(
			QString(
				"Drumkit folder [%1] must not end with a trailing whitespace"
			)
				.arg( sDrumkitDir )
		);
		return false;
	}

	// Trailing whitespace in drumkit name element
	const QString sDrumkitName =
		root.read_string( "name", "", false, false, false );
	if ( sDrumkitName.isEmpty() ) {
		ERRORLOG( QString( "Drumkit must have a non-empty 'name' element" ) );
		return false;
	}

	if ( sDrumkitName.endsWith( " " ) ) {
		ERRORLOG(
			QString( "Drumkit name [%1] must not end with a trailing whitespace"
			)
				.arg( sDrumkitName )
		);
		return false;
	}

	// Everything is valid. No need to keep temporary artifacts.
	if ( !sTemporaryFolder.isEmpty() ) {
		Filesystem::rm( sTemporaryFolder, true, true );
	}

	INFOLOG( QString( "Drumkit [%1] is valid!" ).arg( sDrumkitDirOrXml ) );

	return true;
}

std::shared_ptr<Drumkit> CoreActionController::retrieveDrumkit(
	const QString& sDrumkitDirOrXml,
	bool* bIsCompressed,
	QString* sDrumkitDir,
	QString* sTemporaryFolder,
	bool* pLegacyFormatEncountered
)
{
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

	QFileInfo sourceFileInfo( sDrumkitDirOrXml );

	if ( ( "." + sourceFileInfo.suffix() ) == Filesystem::sDrumkitSuffix ) {
		if ( !Filesystem::fileReadable( sDrumkitDirOrXml, true ) ) {
			ERRORLOG( QString( "Drumkit archive [%1] not readable" )
						  .arg( sDrumkitDirOrXml ) );
			return nullptr;
		}

		*bIsCompressed = true;

		// Temporary folder used to extract a compressed drumkit (
		// .h2drumkit ).
		QString sTemplateName( Filesystem::tmpDir() + "/XXXXXX" );
		QTemporaryDir tmpDir( sTemplateName );
		tmpDir.setAutoRemove( false );
		if ( !tmpDir.isValid() ) {
			ERRORLOG(
				QString(
					"Unable to create temporary folder using template name [%1]"
				)
					.arg( sTemplateName )
			);
			return nullptr;
		}

		*sTemporaryFolder = tmpDir.path();

		// Providing the path to a compressed .h2drumkit file. It will
		// be extracted to a temporary folder and loaded from there.
		if ( !Drumkit::install(
				 sDrumkitDirOrXml, tmpDir.path(), sDrumkitDir, nullptr, true
			 ) ) {
			ERRORLOG(
				QString( "Unabled to extract provided drumkit [%1] into [%2]" )
					.arg( sDrumkitDirOrXml )
					.arg( tmpDir.path() )
			);
			return nullptr;
		}

		INFOLOG( QString( "Extracting drumkit [%1] into [%2]" )
					 .arg( sDrumkitDirOrXml )
					 .arg( tmpDir.path() ) );

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
			ERRORLOG(
				QString( "Unsupported content of [%1]. Expected a single "
						 "folder within the archive containing all samples, "
						 "metadata, as well as the drumkit.xml file. Instead:\n"
				)
					.arg( sDrumkitDirOrXml )
			);
			for ( const auto& sFile : extractedContent ) {
				ERRORLOG( sFile );
			}
			return nullptr;
		}

		pDrumkit = Drumkit::load(
			Filesystem::sanitizeDrumkitPath( *sDrumkitDir ), false,
			pLegacyFormatEncountered, true, m_pHydrogen
		);
	}
	else {
		const auto sDrumkitPath =
			Filesystem::sanitizeDrumkitPath( sDrumkitDirOrXml );
		if ( sDrumkitPath.isEmpty() ) {
			ERRORLOG( QString( "Provided source path [%1] does not point to a "
							   "Hydrogen drumkit" )
						  .arg( sDrumkitDirOrXml ) );
			return nullptr;
		}
		pDrumkit = Drumkit::load(
			sDrumkitPath, false, pLegacyFormatEncountered, true, m_pHydrogen
		);
		*sDrumkitDir = Filesystem::drumkitDirFromPath( sDrumkitPath );
	}

	return pDrumkit;
}

bool CoreActionController::extractDrumkit(
	const QString& sDrumkitBundledPath,
	const QString& sTargetDir,
	QString* pInstalledDir,
	bool* pEncodingIssuesDetected
)
{

	// Ensure variables are always set/initialized.
	if ( pInstalledDir != nullptr ) {
		*pInstalledDir = "";
	}
	if ( pEncodingIssuesDetected != nullptr ) {
		*pEncodingIssuesDetected = false;
	}

	QString sTarget;
	bool bInstall = false;
	if ( sTargetDir.isEmpty() ) {
		bInstall = true;
		INFOLOG( QString( "Installing drumkit [%1]" ).arg( sDrumkitBundledPath )
		);
		sTarget = Filesystem::userDrumkitsDir();
	}
	else {
		INFOLOG( QString( "Extracting drumkit [%1] to [%2]" )
					 .arg( sDrumkitBundledPath )
					 .arg( sTargetDir ) );
		sTarget = sTargetDir;
	}

	if ( !Filesystem::pathUsable( sTarget, true, false ) ) {
		ERRORLOG( QString( "Target dir [%1] is neither a writable folder nor "
						   "can it be created." )
					  .arg( sTarget ) );
		return false;
	}

	QFileInfo sKitInfo( sDrumkitBundledPath );
	if ( !Filesystem::fileReadable( sDrumkitBundledPath, true ) ||
		 "." + sKitInfo.suffix() != Filesystem::sDrumkitSuffix ) {
		ERRORLOG( QString( "Invalid drumkit path [%1]. Please provide an "
						   "absolute path to a .h2drumkit file." )
					  .arg( sDrumkitBundledPath ) );
		return false;
	}

	if ( !Drumkit::install(
			 sDrumkitBundledPath, sTarget, pInstalledDir,
			 pEncodingIssuesDetected, true
		 ) ) {
		ERRORLOG( QString( "Unabled to extract provided drumkit [%1] into [%2]"
		)
					  .arg( sDrumkitBundledPath )
					  .arg( sTarget ) );
		return false;
	}

	if ( bInstall ) {
		m_pHydrogen->getSoundLibraryDatabase()->updateDrumkits(
			Event::Trigger::Default
		);
	}

	return true;
}

bool CoreActionController::addInstrument(
	std::shared_ptr<Instrument> pInstrument,
	int nIndex,
	long nEventId
)
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready yet" );
		return false;
	}
	if ( pInstrument == nullptr ) {
		return false;
	}

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	auto pDrumkit = pSong->getDrumkit();

	pAudioEngine->lock( RIGHT_HERE );

	// Ensure instrument isn't already in the death row.
	m_pHydrogen->removeInstrumentFromDeathRow( pInstrument );
	pInstrument->loadSamples( pAudioEngine->getPlayhead()->getBpm(),
							  m_pHydrogen->getPreferences().get() );

	pDrumkit->addInstrument( pInstrument, nIndex );
	m_pHydrogen->renamePerTrackJackAudioPorts( pSong, nullptr );
	pSong->getPatternList()->mapToDrumkit( pDrumkit, pDrumkit );

	pAudioEngine->unlock();

	m_pHydrogen->setDrumkitModified( true );

	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::DrumkitLoaded, 0, nEventId
	);

	return true;
}

bool CoreActionController::removeInstrument(
	std::shared_ptr<Instrument> pInstrument,
	long nEventId
)
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready yet" );
		return false;
	}

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	auto pDrumkit = pSong->getDrumkit();

	const int nInstrumentNumber =
		pDrumkit->getInstruments()->index( pInstrument );
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
	// hold a shared pointer as part of an undo/redo Midiaction (that's why it
	// is so important to unload the samples).
	m_pHydrogen->addInstrumentToDeathRow( pInstrument );

	// Instead of letting all notes associated with this instrument ring till
	// the end, we discard those for which playback did not started yet and make
	// the remaining ones enter ADSR release phase.
	if ( m_pHydrogen->getProcessMode() != H2Core::ProcessMode::Editor ) {
		pAudioEngine->clearNoteQueues( pInstrument );
		pAudioEngine->getSampler()->releasePlayingNotes(
			pInstrument->getUuid() );
	}

	const int nSelectedInstrument = m_pHydrogen->getSelectedInstrumentNumber();
	if ( nSelectedInstrument == nInstrumentNumber ||
		 nSelectedInstrument >= pDrumkit->getInstruments()->size() ) {
		m_pHydrogen->setSelectedInstrumentNumber(
			std::clamp(
				nSelectedInstrument, 0,
				static_cast<int>( pDrumkit->getInstruments()->size() - 1 )
			),
			Event::Trigger::Suppress
		);
	}

	m_pHydrogen->renamePerTrackJackAudioPorts( pSong, nullptr );
	pSong->getPatternList()->mapToDrumkit( pDrumkit, pDrumkit );

	pAudioEngine->unlock();

	m_pHydrogen->setDrumkitModified( true );

	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::DrumkitLoaded, 0, nEventId
	);

	return true;
}

bool CoreActionController::replaceInstrument(
	std::shared_ptr<Instrument> pNewInstrument,
	std::shared_ptr<Instrument> pOldInstrument
)
{

	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready yet" );
		return false;
	}

	if ( ( pNewInstrument != nullptr &&
		   pNewInstrument->getId() == Instrument::PlaybackTrackId ) ||
		 ( pOldInstrument != nullptr &&
		   pOldInstrument->getId() == Instrument::PlaybackTrackId ) ) {
		return replacePlaybackTrackInstrument(
			pNewInstrument, pOldInstrument
		);
	}
	else {
		return replaceDrumkitInstrument(
			pNewInstrument, pOldInstrument
		);
	}
}

bool CoreActionController::replaceDrumkitInstrument(
	std::shared_ptr<Instrument> pNewInstrument,
	std::shared_ptr<Instrument> pOldInstrument
)
{

	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready yet" );
		return false;
	}

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	auto pDrumkit = pSong->getDrumkit();
	const int nOldInstrumentNumber =
		pDrumkit->getInstruments()->index( pOldInstrument );
	if ( nOldInstrumentNumber == -1 ) {
		ERRORLOG( "Old instrument is not part of current drumkit!" );
		return false;
	}

	const auto fBpm = pAudioEngine->getPlayhead()->getBpm();

	pAudioEngine->lock( RIGHT_HERE );

	if ( pNewInstrument != nullptr ) {
		// Ensure instrument isn't already in the death row.
		m_pHydrogen->removeInstrumentFromDeathRow( pNewInstrument );
		pNewInstrument->loadSamples( fBpm, m_pHydrogen->getPreferences().get() );
	}

	pDrumkit->removeInstrument( pOldInstrument );

	// At this point the instrument has been removed from both the current
	// drumkit and every pattern in the song. But it still lives on as a shared
	// pointer in all Notes within the queues of the AudioEngine and Sampler.
	// Thus, it will be added to the death row, which guarantuees that its
	// samples will be unloaded once all notes referencing it are gone. Note
	// that this does not mean the instrument will be destructed. GUI can still
	// hold a shared pointer as part of an undo/redo Midiaction (that's why it
	// is so important to unload the samples).
	m_pHydrogen->addInstrumentToDeathRow( pOldInstrument );

	// Instead of letting all notes associated with this instrument ring till
	// the end, we discard those for which playback did not started yet and make
	// the remaining ones enter ADSR release phase.
	if ( m_pHydrogen->getProcessMode() != H2Core::ProcessMode::Editor ) {
		pAudioEngine->clearNoteQueues( pOldInstrument );
		pAudioEngine->getSampler()->releasePlayingNotes(
			pOldInstrument->getUuid() );
	}

	pDrumkit->addInstrument( pNewInstrument, nOldInstrumentNumber );
	m_pHydrogen->renamePerTrackJackAudioPorts( pSong, nullptr );
	pSong->getPatternList()->mapToDrumkit( pDrumkit, pDrumkit );

	// Unloading the samples of the old instrument will be done in the death
	// row.

	pAudioEngine->unlock();

	m_pHydrogen->setDrumkitModified( true );

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::DrumkitLoaded, 0 );

	return true;
}

bool CoreActionController::replacePlaybackTrackInstrument(
	std::shared_ptr<Instrument> pNewInstrument,
	std::shared_ptr<Instrument> pOldInstrument
)
{

	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "Song not ready yet" );
		return false;
	}

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	const auto fBpm = pAudioEngine->getPlayhead()->getBpm();

	pAudioEngine->lock( RIGHT_HERE );

	if ( pNewInstrument != nullptr ) {
		// Ensure instrument isn't already in the death row.
		m_pHydrogen->removeInstrumentFromDeathRow( pNewInstrument );
		pNewInstrument->loadSamples( fBpm, m_pHydrogen->getPreferences().get() );
	}

	pSong->setPlaybackTrackInstrument( pNewInstrument );

	// Although Sampler uses a different routine to render the playback track
	// during playback, Sample preview in the SampleEditor is still Note-based.
	m_pHydrogen->addInstrumentToDeathRow( pOldInstrument );

	// Instead of letting all notes associated with this instrument ring till
	// the end, we discard those for which playback did not started yet and make
	// the remaining ones enter ADSR release phase.
	if ( m_pHydrogen->getProcessMode() != H2Core::ProcessMode::Editor ) {
		pAudioEngine->getSampler()->releasePlayingNotes(
			pOldInstrument->getUuid() );
	}

	// Unloading the samples of the old instrument will be done in the death
	// row.

	pAudioEngine->unlock();

	m_pHydrogen->setSongModified( true );

	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::PlaybackTrackChanged, 0
	);

	return true;
}

bool CoreActionController::moveInstrument( int nSourceIndex, int nTargetIndex )
{
	if ( nSourceIndex == nTargetIndex ) {
		return true;
	}

	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return false;
	}

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	m_pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	if ( nSourceIndex >= pInstrumentList->size() || nSourceIndex < 0 ) {
		ERRORLOG( QString( "Source index [%1] out of bound [0,%2)" )
					  .arg( nSourceIndex )
					  .arg( pInstrumentList->size() ) );
		m_pHydrogen->getAudioEngine()->unlock();
		return false;
	}

	if ( nTargetIndex >= pInstrumentList->size() || nTargetIndex < 0 ) {
		ERRORLOG( QString( "Target index [%1] out of bound [0,%2)" )
					  .arg( nTargetIndex )
					  .arg( pInstrumentList->size() ) );
		m_pHydrogen->getAudioEngine()->unlock();
		return false;
	}

	pInstrumentList->move( nSourceIndex, nTargetIndex );
	m_pHydrogen->renamePerTrackJackAudioPorts( pSong, nullptr );

	m_pHydrogen->getAudioEngine()->unlock();

	m_pHydrogen->setDrumkitModified( true );

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::DrumkitLoaded, 0 );

	return true;
}

bool CoreActionController::renameComponent(
	int nInstrumentIdx,
	int nComponentId,
	const QString& sNewName
)
{
	const auto pInstrument = resolveInstrument( nInstrumentIdx );
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

	m_pHydrogen->setDrumkitModified( true );

	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::SelectedInstrumentChanged, 0
	);

	return true;
}

bool CoreActionController::locateToColumn( int nColumn )
{

	if ( nColumn < -1 ) {
		ERRORLOG( QString( "Provided column [%1] too low. Using 0 instead." )
					  .arg( nColumn ) );
		nColumn = 0;
	}

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	long nTotalTick = m_pHydrogen->getTickForColumn( nColumn );
	if ( nTotalTick < 0 ) {
		if ( m_pHydrogen->getMode() == Song::Mode::Song ) {
			ERRORLOG(
				QString( "Provided column [%1] violates the allowed range "
						 "[0;%2). No relocation done." )
					.arg( nColumn )
					.arg( m_pHydrogen->getSong()->getPatternGroupVector()->size()
					)
			);
			return false;
		}
		else {
			// In case of Pattern mode this is not a problem and we
			// will treat this case as the beginning of the song.
			nTotalTick = 0;
		}
	}

	return locateToTick( nTotalTick );
}

bool CoreActionController::locateToTick( long nTick, bool bWithJackBroadcast )
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	const auto pPref = m_pHydrogen->getPreferences();

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );

	pAudioEngine->locate( nTick, bWithJackBroadcast );

	pAudioEngine->unlock();

	if ( pPref->getMidiTransportOutputSend() &&
		 pPref->getMidiFeedbackChannel() != Midi::ChannelOff ) {
		auto pMidiDriver = m_pHydrogen->getMidiDriver();

		if ( pMidiDriver != nullptr ) {
			// A song position provided via MIDI has the lowest resolution of a
			// 1/16 note / 6 MIDI clocks. 24 MIDI clocks make a quarter.
			MidiMessage midiMessage;
			midiMessage.setType( MidiMessage::Type::SongPos );
			midiMessage.setData1( Midi::parameterFromIntClamp(
				pAudioEngine->getPlayhead()->getTick() * 24 / 6 /
				H2Core::nTicksPerQuarter
			) );
			midiMessage.setChannel( pPref->getMidiFeedbackChannel() );
			pMidiDriver->enqueueOutputMessage( midiMessage );
		}
	}

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::Relocation, 0 );
	return true;
}

bool CoreActionController::relocateToFrame( long long nFrame )
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );
	// locateToFrame() queues the Relocation event itself (it is the low-level
	// frame relocate); no JACK broadcast / MIDI feedback, as befits a follow.
	pAudioEngine->locateToFrame( nFrame );
	pAudioEngine->unlock();

	return true;
}

bool CoreActionController::newPattern( const QString& sPatternName )
{
	auto pPatternList = m_pHydrogen->getSong()->getPatternList();
	auto pPattern = std::make_shared<Pattern>();
	pPattern->setName( sPatternName );

	return setPattern( pPattern, pPatternList->size(), false );
}

std::shared_ptr<Pattern> CoreActionController::loadPattern( const QString& sPath
)
{
	auto pNewPattern =
		Pattern::load( sPath, false, m_pHydrogen->getSoundLibraryDatabase() );
	if ( pNewPattern == nullptr ) {
		ERRORLOG( QString( "Unable to load pattern [%1]" ).arg( sPath ) );
		return nullptr;
	}

	return pNewPattern;
}

bool CoreActionController::setPattern(
	std::shared_ptr<Pattern> pNewPattern,
	int nPatternPosition,
	bool bReplace
)
{

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	auto pSong = m_pHydrogen->getSong();
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

	if ( m_pHydrogen->isPatternEditorLocked() ) {
		m_pHydrogen->updateSelectedPattern( false );
	}
	else {
		m_pHydrogen->setSelectedPatternNumber(
			nPatternPosition, false, Event::Trigger::Default
		);
	}
	pAudioEngine->updatePlayingPatterns( Event::Trigger::Default );

	m_pHydrogen->updateSongSize();

	pAudioEngine->unlock();

	if ( bReplace ) {
		m_pHydrogen->updateVirtualPatterns( Event::Trigger::Suppress );
	}

	m_pHydrogen->setSongModified( true );

	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::SelectedPatternChanged, 0
	);

	return true;
}

bool CoreActionController::selectPattern( int nPatternNumber )
{

	const auto pSong = m_pHydrogen->getSong();
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

	if ( !( m_pHydrogen->isPatternEditorLocked() &&
			m_pHydrogen->getAudioEngine()->getState() ==
				AudioEngine::State::Playing ) ) {
		// Event handling will be done in Hydrogen::setSelectedPatternNumber.
		m_pHydrogen->setSelectedPatternNumber(
			nPatternNumber, true, Event::Trigger::Default
		);
	}

	return true;
}

bool CoreActionController::toggleNextPattern( int nPatternNumber )
{
	const auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	const auto pPatternList = pSong->getPatternList();
	if ( nPatternNumber < 0 || nPatternNumber >= pPatternList->size() ) {
		ERRORLOG( QString( "Pattern number [%1] out of bound [0,%2)" )
					  .arg( nPatternNumber )
					  .arg( pPatternList->size() ) );
		return false;
	}

	m_pHydrogen->toggleNextPattern( nPatternNumber );

	return true;
}

bool CoreActionController::movePattern( int nSourcePattern, int nTargetPattern )
{
	if ( nSourcePattern == nTargetPattern ) {
		return true;
	}

	const auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	auto pPatternList = pSong->getPatternList();
	if ( nSourcePattern < 0 || nSourcePattern >= pPatternList->size() ||
		 nTargetPattern < 0 || nTargetPattern >= pPatternList->size() ) {
		ERRORLOG( QString( "Index out of bound — source [%1], target [%2], size [%3]" )
					  .arg( nSourcePattern )
					  .arg( nTargetPattern )
					  .arg( pPatternList->size() ) );
		return false;
	}

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );

	auto pSourcePattern = pPatternList->get( nSourcePattern );
	if ( nSourcePattern < nTargetPattern ) {
		for ( int nPatr = nSourcePattern; nPatr < nTargetPattern; nPatr++ ) {
			pPatternList->replace( nPatr, pPatternList->get( nPatr + 1 ) );
		}
		pPatternList->replace( nTargetPattern, pSourcePattern );
	}
	else {
		for ( int nPatr = nSourcePattern; nPatr > nTargetPattern; nPatr-- ) {
			pPatternList->replace( nPatr, pPatternList->get( nPatr - 1 ) );
		}
		pPatternList->replace( nTargetPattern, pSourcePattern );
	}

	pAudioEngine->unlock();

	if ( m_pHydrogen->isPatternEditorLocked() ) {
		m_pHydrogen->updateSelectedPattern();
	}
	else {
		m_pHydrogen->setSelectedPatternNumber(
			nTargetPattern, true, Event::Trigger::Default
		);
	}

	m_pHydrogen->setSongModified( true );

	// The view reacts to the event (ADR 0027) — no GUI-side editor refresh in the
	// mutation path.
	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::PatternChanged, 0 );

	return true;
}

bool CoreActionController::removePattern( int nPatternNumber )
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	auto pSong = m_pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	INFOLOG( QString( "Deleting pattern [%1]" ).arg( nPatternNumber ) );

	auto pPatternList = pSong->getPatternList();
	auto pPatternGroupVector = pSong->getPatternGroupVector();
	auto pPlayingPatterns = pAudioEngine->getPlayingPatterns();
	auto pNextPatterns = pAudioEngine->getNextPatterns();

	int nSelectedPatternNumber = m_pHydrogen->getSelectedPatternNumber();
	auto pPattern = pPatternList->get( nPatternNumber );

	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Pattern [%1] not found" ).arg( nPatternNumber ) );
		return false;
	}

	pAudioEngine->lock( RIGHT_HERE );

	// Ensure there is always at least one pattern present in the
	// list.
	if ( pPatternList->size() == 0 ) {
		auto pEmptyPattern = std::make_shared<Pattern>();
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

	if ( m_pHydrogen->isPatternEditorLocked() ) {
		m_pHydrogen->updateSelectedPattern( false );
	}
	else if ( nPatternNumber == nSelectedPatternNumber ) {
		m_pHydrogen->setSelectedPatternNumber(
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

	m_pHydrogen->updateSongSize();

	pAudioEngine->unlock();

	// Update virtual pattern presentation.
	for ( const auto& ppattern : *pPatternList ) {
		Pattern::virtual_patterns_cst_it_t it =
			ppattern->getVirtualPatterns()->find( pPattern );
		if ( it != ppattern->getVirtualPatterns()->end() ) {
			ppattern->virtualPatternsDel( *it );
		}
	}

	m_pHydrogen->updateVirtualPatterns();
	m_pHydrogen->setSongModified( true );

	return true;
}

bool CoreActionController::clearInstrumentInPattern(
	int nInstrument,
	int nPatternNumber
)
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	int nPattern;
	if ( nPatternNumber != -1 ) {
		nPattern = nPatternNumber;
	}
	else {
		nPattern = m_pHydrogen->getSelectedPatternNumber();
	}

	auto pPattern = pSong->getPatternList()->get( nPattern );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" ).arg( nPattern ) );
		return false;
	}

	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrument );
	if ( pInstrument == nullptr ) {
		ERRORLOG( QString( "Couldn't find instrument [%1]" ).arg( nInstrument )
		);
		return false;
	}

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );

	pPattern->purgeInstrument( pInstrument );

	pAudioEngine->unlock();

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::PatternChanged, 0 );

	return true;
}

bool CoreActionController::setPatternProperties(
	const QString& sNewPatternPath,
	const int nNewVersion,
	const QString& sNewPatternName,
	const QString& sNewAuthor,
	const QString& sNewPatternInfo,
	const H2Core::License& newLicense,
	const QStringList& newTags,
	int nPatternIndex
)
{

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	auto pPatternList = m_pHydrogen->getSong()->getPatternList();
	auto pPattern = pPatternList->get( nPatternIndex );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Unable to find pattern [%1]" ).arg( nPatternIndex )
		);
		return false;
	}

	pPattern->setPath( sNewPatternPath );
	pPattern->setVersion( nNewVersion );
	pPattern->setName( sNewPatternName );
	pPattern->setAuthor( sNewAuthor );
	pPattern->setInfo( sNewPatternInfo );
	// Only update the license in case it changed (in order to not
	// overwrite an attribution).
	if ( pPattern->getLicense() != newLicense ) {
		pPattern->setLicense( newLicense );
	}
	pPattern->setTags( newTags );

	m_pHydrogen->setPatternModified( true, nPatternIndex );

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::PatternChanged, -1 );

	return true;
}

bool CoreActionController::setPatternSize( int nLength, int nDenominator,
										  int nPatternNumber )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	auto pPatternList = pSong->getPatternList();
	auto pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Unable to find pattern [%1]" ).arg( nPatternNumber ) );
		return false;
	}

	// Length and denominator affect playback, so this edit is real-time
	// sensitive: own the AudioEngine lock here (ADR 0027)
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );
	pPattern->setLength( nLength );
	pPattern->setDenominator( nDenominator );
	m_pHydrogen->updateSongSize();
	pAudioEngine->unlock();

	m_pHydrogen->setPatternModified( true, nPatternNumber );
	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::PatternChanged, -1 );

	return true;
}

bool CoreActionController::editNoteProperty(
	NoteProperty property,
	int nPatternNumber,
	int nPosition,
	int nOldInstrumentId,
	int nNewInstrumentId,
	const QString& sOldType,
	const QString& sNewType,
	float fVelocity,
	float fPan,
	float fLeadLag,
	float fProbability,
	int nLength,
	int nNewKey,
	int nOldKey,
	int nNewOctave,
	int nOldOctave )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	auto pPatternList = pSong->getPatternList();
	std::shared_ptr<Pattern> pPattern;
	if ( nPatternNumber != -1 && nPatternNumber < pPatternList->size() ) {
		pPattern = pPatternList->get( nPatternNumber );
	}
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Unable to find pattern [%1]" ).arg( nPatternNumber ) );
		return false;
	}

	const auto oldId = static_cast<Instrument::Id>( nOldInstrumentId );
	const auto newId = static_cast<Instrument::Id>( nNewInstrumentId );
	const auto oldKey = static_cast<Note::Key>( nOldKey );
	const auto newKey = static_cast<Note::Key>( nNewKey );
	const auto oldOctave = static_cast<Note::Octave>( nOldOctave );
	const auto newOctave = static_cast<Note::Octave>( nNewOctave );

	// Note edits touch objects the Sampler/AudioEngine reference live, so this
	// owns the AudioEngine lock.
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );

	auto pNote = pPattern->findNote( nPosition, oldId, sOldType, oldKey, oldOctave );
	if ( pNote == nullptr && property == NoteProperty::Type ) {
		// The type of an unmapped note may have been set to one already present
		// in the drumkit, so its instrument id got remapped and no longer
		// matches the value the undo/redo action was created with.
		bool bOk;
		const auto kitId =
			pSong->getDrumkit()->toDrumkitMap()->getId( sOldType, &bOk );
		if ( bOk ) {
			pNote = pPattern->findNote( nPosition, kitId, sOldType, oldKey,
										oldOctave );
		}
	}
	else if ( pNote == nullptr && property == NoteProperty::InstrumentId ) {
		// When adding an instrument to a row of typed-but-unmapped notes, the
		// redo part is done automatically as part of the mapping to the updated
		// kit. Only the undo part needs covering here.
		pAudioEngine->unlock();
		return false;
	}

	bool bValueChanged = false;
	if ( pNote != nullptr ) {
		switch ( property ) {
		case NoteProperty::Velocity:
			if ( pNote->getVelocity() != fVelocity ) {
				pNote->setVelocity( fVelocity );
				bValueChanged = true;
			}
			break;
		case NoteProperty::Pan:
			if ( pNote->getPan() != fPan ) {
				pNote->setPan( fPan );
				bValueChanged = true;
			}
			break;
		case NoteProperty::LeadLag:
			if ( pNote->getLeadLag() != fLeadLag ) {
				pNote->setLeadLag( fLeadLag );
				bValueChanged = true;
			}
			break;
		case NoteProperty::KeyOctave: {
			// The NotePropertiesRuler addresses key and octave independently:
			// a value of Note::Key::Invalid / Note::Octave::Invalid means "leave
			// this component unchanged". Only touch (and only report a change
			// for) the valid, actually-different component — never write Invalid
			// onto the note.
			const bool bKeyChanges =
				newKey != Note::Key::Invalid && newKey != pNote->getKey();
			const bool bOctaveChanges =
				newOctave != Note::Octave::Invalid &&
				newOctave != pNote->getOctave();
			if ( bKeyChanges ) {
				pNote->setKey( newKey );
			}
			if ( bOctaveChanges ) {
				pNote->setOctave( newOctave );
			}
			bValueChanged = bKeyChanges || bOctaveChanges;
			break;
		}
		case NoteProperty::Probability:
			if ( pNote->getProbability() != fProbability ) {
				pNote->setProbability( fProbability );
				bValueChanged = true;
			}
			break;
		case NoteProperty::Length:
			if ( pNote->getLength() != nLength ) {
				pNote->setLength( nLength );
				bValueChanged = true;
			}
			break;
		case NoteProperty::Type:
			if ( pNote->getType() != sNewType ||
				 pNote->getInstrumentId() != newId ) {
				pNote->setInstrumentId( newId );
				pNote->setType( sNewType );

				auto pInstrument =
					pSong->getDrumkit()->mapInstrument( sNewType, newId );
				pNote->mapToInstrument( pInstrument );
				bValueChanged = true;
			}
			break;
		case NoteProperty::InstrumentId:
			if ( pNote->getInstrumentId() != newId ) {
				pNote->setInstrumentId( newId );
				bValueChanged = true;
			}
			break;
		default:
			ERRORLOG( "No property set. No note property adjusted." );
		}
	}
	else {
		ERRORLOG( "note could not be found" );
	}

	pAudioEngine->unlock();

	if ( bValueChanged ) {
		m_pHydrogen->setPatternModified( true, nPatternNumber );
	}

	return bValueChanged;
}

bool CoreActionController::addOrRemoveNote(
	int nPosition,
	int nInstrumentId,
	const QString& sType,
	int nPatternNumber,
	int nOldLength,
	float fOldVelocity,
	float fOldPan,
	float fOldLeadLag,
	int nOldKey,
	int nOldOctave,
	float fOldProbability,
	bool bIsDelete,
	bool bIsNoteOff,
	bool bIsMappedToDrumkit,
	Uuid* pNewNoteUuid )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	auto pPatternList = pSong->getPatternList();
	if ( nPatternNumber < 0 || nPatternNumber >= pPatternList->size() ) {
		ERRORLOG( QString( "Pattern number [%1] out of bound [0,%2]" )
					  .arg( nPatternNumber ).arg( pPatternList->size() ) );
		return false;
	}

	auto pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Pattern found for pattern number [%1] is not valid" )
					  .arg( nPatternNumber ) );
		return false;
	}

	const auto id = static_cast<Instrument::Id>( nInstrumentId );
	if ( id == Instrument::EmptyId && sType.isEmpty() ) {
		return false;
	}

	const auto oldKey = static_cast<Note::Key>( nOldKey );
	const auto oldOctave = static_cast<Note::Octave>( nOldOctave );

	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );

	if ( bIsDelete ) {
		// Find and delete an existing (matching) note. When several notes share
		// the same position + id/type + key/octave, disambiguate using all
		// remaining properties.
		std::vector<std::shared_ptr<Note>> notesFound;
		const auto pNotes = pPattern->getNotes();
		for ( auto it = pNotes->lower_bound( nPosition );
			  it != pNotes->end() && it->first <= nPosition; ++it ) {
			auto ppNote = it->second;
			if ( ppNote != nullptr && ppNote->getInstrumentId() == id &&
				 ppNote->getType() == sType && ppNote->getKey() == oldKey &&
				 ppNote->getOctave() == oldOctave ) {
				notesFound.push_back( ppNote );
			}
		}

		if ( notesFound.size() == 1 ) {
			pPattern->removeNote( notesFound[ 0 ] );
		}
		else if ( notesFound.size() > 1 ) {
			bool bFound = false;
			for ( const auto& ppNote : notesFound ) {
				if ( ppNote->getLength() == nOldLength &&
					 ppNote->getVelocity() == fOldVelocity &&
					 ppNote->getPan() == fOldPan &&
					 ppNote->getLeadLag() == fOldLeadLag &&
					 ppNote->getProbability() == fOldProbability &&
					 ppNote->getNoteOff() == bIsNoteOff ) {
					bFound = true;
					pPattern->removeNote( ppNote );
				}
			}

			if ( ! bFound ) {
				QStringList noteStrings;
				for ( const auto& ppNote : notesFound ) {
					noteStrings << "\n - " << ppNote->toQString();
				}
				ERRORLOG( QString( "length: %1, velocity: %2, pan: %3, lead&lag: %4, probability: %5, noteOff: %6 not found amongst notes:%7" )
							  .arg( nOldLength ).arg( fOldVelocity ).arg( fOldPan )
							  .arg( fOldLeadLag ).arg( fOldProbability )
							  .arg( bIsNoteOff ).arg( noteStrings.join( "" ) ) );
			}
		}
		else {
			ERRORLOG( "Did not find note to delete" );
		}
	}
	else {
		// Create the new note.
		float fVelocity = fOldVelocity;
		float fPan = fOldPan;
		int nLength = nOldLength;

		if ( bIsNoteOff ) {
			fVelocity = VELOCITY_MIN;
			fPan = PAN_DEFAULT;
			nLength = 1;
		}

		std::shared_ptr<Instrument> pInstrument = nullptr;
		if ( id != Instrument::EmptyId && bIsMappedToDrumkit ) {
			// Can still be nullptr for notes in unmapped rows.
			pInstrument = pSong->getDrumkit()->getInstruments()->find( id );
		}

		auto pNewNote = std::make_shared<Note>(
			pInstrument, nPosition, fVelocity, fPan, nLength );
		pNewNote->setInstrumentId( id );
		pNewNote->setType( sType );
		pNewNote->setNoteOff( bIsNoteOff );
		pNewNote->setLeadLag( fOldLeadLag );
		pNewNote->setProbability( fOldProbability );
		pNewNote->setKey( oldKey );
		pNewNote->setOctave( oldOctave );
		pPattern->insertNote( pNewNote );

		if ( pNewNoteUuid != nullptr ) {
			*pNewNoteUuid = pNewNote->getUuid();
		}
	}

	pAudioEngine->unlock();

	m_pHydrogen->setPatternModified( true, nPatternNumber );

	return true;
}

bool CoreActionController::removeNote( Uuid noteUuid, Uuid patternUuid ) {
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}
	auto pPatternList = pSong->getPatternList();
	const int nPatternNumber = pPatternList->index( patternUuid );
	auto pPattern = pPatternList->get( nPatternNumber );
	if ( pPattern == nullptr ) {
		ERRORLOG( "Unable to find pattern" );
		return false;
	}
	auto pNote = pPattern->findNote( noteUuid );
	if ( pNote == nullptr ) {
		ERRORLOG( "Note not found" );
		return false;
	}

	// Removing a note the Sampler/AudioEngine may iterate live is real-time
	// sensitive, so this owns the AudioEngine lock (ADR 0027).
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );
	pPattern->removeNote( pNote );
	pAudioEngine->unlock();

	m_pHydrogen->setPatternModified( true, nPatternNumber );

	return true;
}

bool CoreActionController::setSongProperties(
	const QString& sNewPath,
	const int nNewVersion,
	const QString& sNewName,
	const QString& sNewAuthor,
	const QString& sNewNotes,
	const H2Core::License& newLicense,
	const QStringList& newTags
)
{

	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	pSong->setPath( sNewPath );
	pSong->setVersion( nNewVersion );
	pSong->setName( sNewName );
	pSong->setAuthor( sNewAuthor );
	pSong->setNotes( sNewNotes );
	// Only update the license in case it changed (in order to not
	// overwrite an attribution).
	if ( pSong->getLicense() != newLicense ) {
		pSong->setLicense( newLicense );
	}
	pSong->setTags( newTags );

	m_pHydrogen->setSongModified( true );

	return true;
}

bool CoreActionController::toggleGridCell( const GridPoint& gridPoint )
{

	if ( m_pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	auto pSong = m_pHydrogen->getSong();
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	auto pPatternList = pSong->getPatternList();
	auto pColumns = pSong->getPatternGroupVector();

	if ( gridPoint.getRow() < 0 || gridPoint.getRow() > pPatternList->size() ) {
		ERRORLOG( QString( "Provided row [%1] is out of bound [0,%2]" )
					  .arg( gridPoint.getRow() )
					  .arg( pPatternList->size() ) );
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
		auto pColumn = ( *pColumns )[gridPoint.getColumn()];
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
				auto pColumn = ( *pColumns )[ii];
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
					  .arg( gridPoint.getColumn() )
					  .arg( pColumns->size() ) );
		pAudioEngine->unlock();
		return false;
	}

	m_pHydrogen->updateSongSize();
	m_pHydrogen->updateSelectedPattern( false );

	pAudioEngine->unlock();

	m_pHydrogen->setSongModified( true );

	// Update the SongEditor.
	if ( m_pHydrogen->getProcessMode() != H2Core::ProcessMode::Headless ) {
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::GridCellToggled, 0
		);
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
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return false;
	}

	const auto pPref = m_pHydrogen->getPreferences();
	const auto pMidiInstrumentMap = pPref->getMidiInstrumentMap();
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return false;
	}
	const auto pInstrumentList = pSong->getDrumkit()->getInstruments();

	const auto mappedInstruments =
		pMidiInstrumentMap->mapInput( note, channel, pSong->getDrumkit(), m_pHydrogen );
	QString sMode(
		MidiInstrumentMap::InputToQString( pMidiInstrumentMap->getInput() )
	);

	// Some finishing touches and note playback.
	bool bSuccess = true;
	QStringList instrumentStrings;
	for ( const auto& ppInstrument : mappedInstruments ) {
		// Only look to change instrument if the current note is actually of
		// hihat and hihat openness is outside the instrument selected
		const auto hihatOpenness = m_pHydrogen->getHihatOpenness();
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

		if ( m_pHydrogen->addRealtimeNote(
				 nCurrentInstrument, fVelocity, bNoteOff, note
			 ) ) {
			instrumentStrings << QString( "%1 (%2)" )
									 .arg( ppInstrument->getName() )
									 .arg( nCurrentInstrument );
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

bool CoreActionController::releasePlayingNotes( Uuid instrumentUuid ) {
	m_pHydrogen->getAudioEngine()->getSampler()->releasePlayingNotes(
		instrumentUuid
	);
	return true;
}

void CoreActionController::insertRecentFile( const QString& sFileName )
{
	auto pPref = m_pHydrogen->getPreferences();

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

bool CoreActionController::setBpm( float fBpm )
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	auto pSong = m_pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song set yet" );
		return false;
	}

	if ( m_pHydrogen->getTempoSource() != Hydrogen::Tempo::Song ) {
		return false;
	}

	fBpm = std::clamp(
		fBpm, static_cast<float>( MIN_BPM ), static_cast<float>( MAX_BPM )
	);

	pAudioEngine->lock( RIGHT_HERE );
	// Use tempo in the next process cycle of the audio engine.
	pAudioEngine->setNextBpm( fBpm );

	// Store it's value in the .h2song file.
	pSong->setBpm( fBpm );
	if ( pSong->getTimeline() != nullptr ) {
		pSong->getTimeline()->setDefaultBpm( fBpm );
	}

	pAudioEngine->unlock();

	m_pHydrogen->setSongModified( true );

	return true;
}

bool CoreActionController::startCountIn()
{
	auto pAudioEngine = m_pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->startCountIn();
	pAudioEngine->unlock();

	return true;
}

std::shared_ptr<Playlist> CoreActionController::loadPlaylist(
	const QString& sPath,
	const QString& sRecoverPath
)
{
	// Check whether the provided path is valid.
	if ( sPath != Filesystem::emptyPath( Filesystem::Artifact::Playlist ) &&
		 !Filesystem::isPathValid(
			 Filesystem::Artifact::Playlist, sPath, true
		 ) ) {
		// Filesystem::isPathValid takes care of the error log message.
		return nullptr;
	}

	std::shared_ptr<Playlist> pPlaylist;
	if ( !sRecoverPath.isEmpty() &&
		 Filesystem::isPathValid(
			 Filesystem::Artifact::Playlist, sRecoverPath, true
		 ) ) {
		// Use an autosave file to load the playlist
		pPlaylist = Playlist::load( sRecoverPath, m_pHydrogen->getPreferences() );
		if ( pPlaylist != nullptr ) {
			pPlaylist->setPath( sPath );
		}
		else {
			ERRORLOG(
				QString(
					"Unable to recover changes from [%1]. Loading [%2] instead."
				)
					.arg( sRecoverPath )
					.arg( sPath )
			);
		}
	}

	if ( pPlaylist == nullptr ) {
		pPlaylist = Playlist::load( sPath, m_pHydrogen->getPreferences() );
	}

	if ( pPlaylist == nullptr ) {
		ERRORLOG( QString( "Unable to open playlist [%1]." ).arg( sPath ) );
		return nullptr;
	}

	return pPlaylist;
}

bool CoreActionController::setPlaylist( std::shared_ptr<Playlist> pPlaylist )
{
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid playlist" );
		return false;
	}
	m_pHydrogen->setPlaylist( pPlaylist );

	if ( pPlaylist->getPath() ==
		 Filesystem::emptyPath( Filesystem::Artifact::Playlist ) ) {
		// To indicate that the user closed the previous playlsit in favor
		// of a new one, we store an empty string. This way the changes from
		// the empty playlist can be recovered.
		m_pHydrogen->getPreferences()->setLastPlaylistPath( "" );
	}
	else {
		m_pHydrogen->getPreferences()->setLastPlaylistPath( pPlaylist->getPath()
		);
	}

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::PlaylistChanged, 0 );

	// In case the playlist is read-only, autosave won't work.
	if ( !Filesystem::fileWritable( pPlaylist->getPath() ) ) {
		WARNINGLOG(
			QString(
				"You don't have permissions to write to the playlist found in "
				"path [%1]. It will be opened as read-only (no autosave)."
			)
				.arg( pPlaylist->getPath() )
		);
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::PlaylistChanged, 2
		);
	}

	return true;
}

bool CoreActionController::savePlaylist()
{
	auto pPlaylist = m_pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}
	if ( !pPlaylist->save( m_pHydrogen->getPreferences() ) ) {
		return false;
	}

	pPlaylist->setIsModified( false );
	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::PlaylistChanged, 1 );
	return true;
}

bool CoreActionController::savePlaylistAs( const QString& sPath )
{
	auto pPlaylist = m_pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}
	if ( !m_pHydrogen->getPlaylist()->saveAs( sPath, m_pHydrogen->getPreferences() ) ) {
		ERRORLOG( QString( "Unable to save playlist to [%1]" ).arg( sPath ) );
		return false;
	}

	pPlaylist->setIsModified( false );

	m_pHydrogen->getPreferences()->setLastPlaylistPath( sPath );

	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::PlaylistChanged, 0 );

	return true;
}

bool CoreActionController::addToPlaylist(
	std::shared_ptr<PlaylistEntry> pEntry,
	int nIndex
)
{
	if ( pEntry == nullptr ) {
		return false;
	}
	auto pPlaylist = m_pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}

	if ( !pPlaylist->add( pEntry, nIndex ) ) {
		return false;
	}

	pPlaylist->setIsModified( true );
	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::PlaylistChanged, 0 );
	return true;
}
bool CoreActionController::removeFromPlaylist(
	std::shared_ptr<PlaylistEntry> pEntry,
	int nIndex
)
{
	if ( pEntry == nullptr ) {
		return false;
	}
	auto pPlaylist = m_pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}

	if ( !pPlaylist->remove( pEntry, nIndex ) ) {
		return false;
	}

	pPlaylist->setIsModified( true );
	m_pHydrogen->getEventQueue()->pushEvent( Event::Type::PlaylistChanged, 0 );
	return true;
}
bool CoreActionController::activatePlaylistSong( int nSongNumber )
{
	auto pPlaylist = m_pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}

	if ( !pPlaylist->activateSong( nSongNumber ) ) {
		ERRORLOG(
			QString( "Unable to set playlist song [%1]" ).arg( nSongNumber )
		);
		return false;
	}
	m_pHydrogen->getEventQueue()->pushEvent(
		H2Core::Event::Type::PlaylistLoadSong, nSongNumber
	);

	return true;
}

bool CoreActionController::setMidiClockInputHandling( bool bHandle )
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return false;
	}
	auto pPref = m_pHydrogen->getPreferences();
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return false;
	}

	if ( pPref->getMidiClockInputHandling() == bHandle ) {
		return false;
	}

	pPref->setMidiClockInputHandling( bHandle );

	m_pHydrogen->getEventQueue()->pushEvent(
		H2Core::Event::Type::MidiClockActivation, 0
	);

	if ( !bHandle && m_pHydrogen->getTempoSource() == Hydrogen::Tempo::Song ) {
		// Restore the previous tempo.
		auto pAudioEngine = m_pHydrogen->getAudioEngine();
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setNextBpm( pSong->getBpm() );
		pAudioEngine->unlock();
	}

	return true;
}

bool CoreActionController::setMidiClockOutputSend( bool bHandle )
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return false;
	}

	auto pPref = m_pHydrogen->getPreferences();
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return false;
	}

	if ( pPref->getMidiClockOutputSend() == bHandle ) {
		return false;
	}

	pPref->setMidiClockOutputSend( bHandle );

	// Jump start sending MIDI clock messages. Else they would only be send on
	// the next tempo change or start of the audio engine.
	auto pMidiDriver = m_pHydrogen->getAudioEngine()->getMidiDriver();
	if ( pMidiDriver != nullptr ) {
		if ( bHandle ) {
			pMidiDriver->startMidiClockStream(
				m_pHydrogen->getAudioEngine()->getPlayhead()->getBpm()
			);
		}
		else {
			pMidiDriver->stopMidiClockStream();
		}
	}

	return true;
}

bool CoreActionController::clearMidiInputLog()
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return false;
	}
	auto pMidiDriver = m_pHydrogen->getMidiDriver();
	if ( pMidiDriver == nullptr ) {
		return false;
	}
	pMidiDriver->clearHandledInput();
	return true;
}

bool CoreActionController::clearMidiOutputLog()
{
	if ( m_pHydrogen->getProcessMode() == H2Core::ProcessMode::Editor ) {
		return false;
	}
	auto pMidiDriver = m_pHydrogen->getMidiDriver();
	if ( pMidiDriver == nullptr ) {
		return false;
	}
	pMidiDriver->clearHandledOutput();
	return true;
}

bool CoreActionController::addAutomationPoint( float fX, float fY )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getAutomationPath() == nullptr ) {
		return false;
	}

	pSong->getAutomationPath()->addPoint( fX, fY );
	m_pHydrogen->setSongModified( true );
	return true;
}

bool CoreActionController::removeAutomationPoint( float fX )
{
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getAutomationPath() == nullptr ) {
		return false;
	}

	pSong->getAutomationPath()->removePoint( fX );
	m_pHydrogen->setSongModified( true );
	return true;
}
}  // namespace H2Core
