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

#include "MidiActionManager.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

using namespace H2Core;

MidiActionManager* MidiActionManager::__instance = nullptr;

MidiActionManager::MidiActionManager() {
	__instance = this;

	m_nLastBpmChangeCCParameter = -1;
	/*
		the m_actionMap holds all Action identifiers which hydrogen is able to interpret.
		it holds pointer to member function
	*/
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Play,
					   std::make_pair( &MidiActionManager::play, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlayStopToggle,
					   std::make_pair( &MidiActionManager::play_stop_toggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlayPauseToggle,
					   std::make_pair( &MidiActionManager::play_pause_toggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Stop,
					   std::make_pair( &MidiActionManager::stop, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Pause,
					   std::make_pair( &MidiActionManager::pause, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RecordReady,
					   std::make_pair( &MidiActionManager::record_ready, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RecordStrobeToggle,
					   std::make_pair( &MidiActionManager::record_strobe_toggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RecordStrobe,
					   std::make_pair( &MidiActionManager::record_strobe, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RecordExit,
					   std::make_pair( &MidiActionManager::record_exit, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Mute,
					   std::make_pair( &MidiActionManager::mute, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Unmute,
					   std::make_pair( &MidiActionManager::unmute, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::MuteToggle,
					   std::make_pair( &MidiActionManager::mute_toggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::StripMuteToggle,
					   std::make_pair( &MidiActionManager::strip_mute_toggle, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::StripSoloToggle,
					   std::make_pair( &MidiActionManager::strip_solo_toggle, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::NextBar,
					   std::make_pair( &MidiActionManager::next_bar, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PreviousBar,
					   std::make_pair( &MidiActionManager::previous_bar, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BpmIncr,
					   std::make_pair( &MidiActionManager::bpm_increase, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BpmDecr,
					   std::make_pair( &MidiActionManager::bpm_decrease, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BpmCcRelative,
					   std::make_pair( &MidiActionManager::bpm_cc_relative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BpmFineCcRelative,
					   std::make_pair( &MidiActionManager::bpm_fine_cc_relative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::MasterVolumeRelative,
					   std::make_pair( &MidiActionManager::master_volume_relative, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::MasterVolumeAbsolute,
					   std::make_pair( &MidiActionManager::master_volume_absolute, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::StripVolumeRelative,
					   std::make_pair( &MidiActionManager::strip_volume_relative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::StripVolumeAbsolute,
					   std::make_pair( &MidiActionManager::strip_volume_absolute, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::EffectLevelAbsolute,
					   std::make_pair( &MidiActionManager::effect_level_absolute, 2 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::EffectLevelRelative,
					   std::make_pair( &MidiActionManager::effect_level_relative, 2 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::GainLevelAbsolute,
					   std::make_pair( &MidiActionManager::gain_level_absolute, 3 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PitchLevelAbsolute,
					   std::make_pair( &MidiActionManager::pitch_level_absolute, 3 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectNextPattern,
					   std::make_pair( &MidiActionManager::select_next_pattern, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectOnlyNextPattern,
					   std::make_pair( &MidiActionManager::select_only_next_pattern, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectNextPatternCcAbsolute,
					   std::make_pair( &MidiActionManager::select_next_pattern_cc_absolute, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectOnlyNextPatternCcAbsolute,
					   std::make_pair( &MidiActionManager::select_only_next_pattern_cc_absolute, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectNextPatternRelative,
					   std::make_pair( &MidiActionManager::select_next_pattern_relative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectAndPlayPattern,
					   std::make_pair( &MidiActionManager::select_and_play_pattern, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PanRelative,
					   std::make_pair( &MidiActionManager::pan_relative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PanAbsolute,
					   std::make_pair( &MidiActionManager::pan_absolute, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PanAbsoluteSym,
					   std::make_pair( &MidiActionManager::pan_absolute_sym, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::InstrumentPitch,
									  std::make_pair( &MidiActionManager::instrument_pitch, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::FilterCutoffLevelAbsolute,
					   std::make_pair( &MidiActionManager::filter_cutoff_level_absolute, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BeatCounter,
					   std::make_pair( &MidiActionManager::beatcounter, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::TapTempo,
					   std::make_pair( &MidiActionManager::tap_tempo, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlaylistSong,
					   std::make_pair( &MidiActionManager::playlist_song, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlaylistNextSong,
					   std::make_pair( &MidiActionManager::playlist_next_song, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlaylistPrevSong,
					   std::make_pair( &MidiActionManager::playlist_previous_song, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::ToggleMetronome,
					   std::make_pair( &MidiActionManager::toggle_metronome, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectInstrument,
					   std::make_pair( &MidiActionManager::select_instrument, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::UndoAction,
					   std::make_pair( &MidiActionManager::undo_action, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RedoAction,
					   std::make_pair( &MidiActionManager::redo_action, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::ClearSelectedInstrument,
					   std::make_pair( &MidiActionManager::clear_selected_instrument, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::ClearPattern,
					   std::make_pair( &MidiActionManager::clear_pattern, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::LoadNextDrumkit,
					   std::make_pair( &MidiActionManager::loadNextDrumkit, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::LoadPrevDrumkit,
					   std::make_pair( &MidiActionManager::loadPrevDrumkit, 0 ) ));

	for ( const auto& ppAction : m_midiActionMap ) {
		auto ret = m_midiActions.insert( ppAction.first );
		if ( ! ret.second ) {
			ERRORLOG( QString( "Unable to insert [%1]" )
					  .arg( MidiAction::typeToQString( ppAction.first ) ) );
		}
	}
}


MidiActionManager::~MidiActionManager() {
	//INFOLOG( "ActionManager delete" );
	__instance = nullptr;
}

void MidiActionManager::create_instance() {
	if ( __instance == nullptr ) {
		__instance = new MidiActionManager;
	}
}

bool MidiActionManager::play( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Ready ) {
		pHydrogen->sequencerPlay();
	}
	return true;
}

bool MidiActionManager::pause( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->sequencerStop();
	return true;
}

bool MidiActionManager::stop( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->sequencerStop();
	return CoreActionController::locateToColumn( 0 );
}

bool MidiActionManager::play_pause_toggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		return false;
	}

	switch ( pHydrogen->getAudioEngine()->getState() ) {
	case AudioEngine::State::Ready:
		pHydrogen->sequencerPlay();
		break;

	case AudioEngine::State::Playing:
		pHydrogen->sequencerStop();
		break;

	default:
		ERRORLOG( "AudioEngine not ready yet." );
		return false;
	}

	return true;
}

bool MidiActionManager::play_stop_toggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		return false;
	}
	
	switch ( pHydrogen->getAudioEngine()->getState() ) {
	case AudioEngine::State::Ready:
		pHydrogen->sequencerPlay();
		break;

	case AudioEngine::State::Playing:
		CoreActionController::locateToColumn( 0 );
		pHydrogen->sequencerStop();
		break;

	default:
		ERRORLOG( "AudioEngine not ready yet." );
		return false;
	}

	return true;
}

//mutes the master, not a single strip
bool MidiActionManager::mute( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return CoreActionController::setMasterIsMuted( true );
}

bool MidiActionManager::unmute( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return CoreActionController::setMasterIsMuted( false );
}

bool MidiActionManager::mute_toggle( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return CoreActionController::setMasterIsMuted( !pHydrogen->getSong()->getIsMuted() );
}

bool MidiActionManager::strip_mute_toggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	return CoreActionController::setStripIsMuted(
		nLine, !pInstr->isMuted(), false );
}

bool MidiActionManager::strip_solo_toggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	return CoreActionController::setStripIsSoloed(
		nLine, !pInstr->isSoloed(), false );
}

bool MidiActionManager::beatcounter( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return pHydrogen->handleBeatCounter();
}

bool MidiActionManager::tap_tempo( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->onTapTempoAccelEvent();
	return true;
}

bool MidiActionManager::select_next_pattern( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	return nextPatternSelection( pAction->getParameter1().toInt(&ok,10) );
}


bool MidiActionManager::select_next_pattern_relative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	bool ok;
	return nextPatternSelection( pHydrogen->getSelectedPatternNumber() +
								 pAction->getParameter1().toInt(&ok,10) );
}

bool MidiActionManager::select_next_pattern_cc_absolute( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	return nextPatternSelection( pAction->getValue().toInt(&ok,10) );
}

bool MidiActionManager::nextPatternSelection( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
    
	if ( nPatternNumber > pSong->getPatternList()->size() - 1 ||
		nPatternNumber < 0 ) {
		ERRORLOG( QString( "Provided value [%1] out of bound [0,%2]" ).arg( nPatternNumber )
				  .arg( pSong->getPatternList()->size() - 1 ) );
		return false;
	}
	
	if ( pHydrogen->getPatternMode() == Song::PatternMode::Selected ) {
		pHydrogen->setSelectedPatternNumber(
			nPatternNumber, true, Event::Trigger::Default );
	}
	else if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
		pHydrogen->toggleNextPattern( nPatternNumber );
	}
	
	return true;
}

bool MidiActionManager::select_only_next_pattern( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	return onlyNextPatternSelection( pAction->getParameter1().toInt(&ok,10) );
}

bool MidiActionManager::select_only_next_pattern_cc_absolute( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	return onlyNextPatternSelection( pAction->getValue().toInt(&ok,10) );
}

bool MidiActionManager::onlyNextPatternSelection( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( nPatternNumber > pSong->getPatternList()->size() -1 ||
		nPatternNumber < 0 ) {
		if ( pHydrogen->getPatternMode() == Song::PatternMode::Selected ) {
			ERRORLOG( QString( "Provided pattern number [%1] out of bound [0,%2]." )
					  .arg( nPatternNumber )
					  .arg( pSong->getPatternList()->size() - 1 ) );
			return false;
		}
		else {
			INFOLOG( QString( "Provided pattern number [%1] out of bound [0,%2]. All patterns will be deselected." )
					 .arg( nPatternNumber )
					 .arg( pSong->getPatternList()->size() - 1 ) );
		}
	}
	
	if ( pHydrogen->getPatternMode() == Song::PatternMode::Selected ) {
		return nextPatternSelection( nPatternNumber );
	}
	
	return pHydrogen->flushAndAddNextPattern( nPatternNumber );
}

bool MidiActionManager::select_and_play_pattern( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( ! select_next_pattern( pAction ) ) {
		return false;
	}

	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Ready ) {
		pHydrogen->sequencerPlay();
	}

	return true;
}

bool MidiActionManager::select_instrument( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int  nInstrumentNumber = pAction->getValue().toInt(&ok,10) ;

	if ( pSong->getDrumkit()->getInstruments()->size() < nInstrumentNumber ) {
		nInstrumentNumber = pSong->getDrumkit()->getInstruments()->size() -1;
	} else if ( nInstrumentNumber < 0 ) {
		nInstrumentNumber = 0;
	}
	
	pHydrogen->setSelectedInstrumentNumber( nInstrumentNumber );
	return true;
}

bool MidiActionManager::effect_level_absolute( std::shared_ptr<MidiAction> pAction) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int fx_param = pAction->getValue().toInt(&ok,10);
	int fx_id = pAction->getParameter2().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	float fValue = 0;
	if ( fx_param != 0 ) {
		fValue = static_cast<float>(fx_param) / 127.0;
	}

	return CoreActionController::setStripEffectLevel( nLine, fx_id, fValue, true );
}

bool MidiActionManager::effect_level_relative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int fx_param = pAction->getValue().toInt(&ok,10);
	int fx_id = pAction->getParameter2().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	float fValue = 0;
	if ( fx_param != 0 ) {
		if ( fx_param == 1 && pInstr->getFxLevel( fx_id ) <= 0.95 ) {
			fValue = pInstr->getFxLevel( fx_id ) + 0.05;
		}
		else if ( pInstr->getFxLevel( fx_id ) >= 0.05 ) {
			fValue = pInstr->getFxLevel( fx_id ) - 0.05;
		}
	}

	return CoreActionController::setStripEffectLevel( nLine, fx_id, fValue, true );
}

//sets the volume of a master output to a given level (percentage)
bool MidiActionManager::master_volume_absolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nVolume = pAction->getValue().toInt(&ok,10);

	if ( nVolume != 0 ) {
		pSong->setVolume( 1.5* ( (float) (nVolume / 127.0 ) ));
	} else {
		pSong->setVolume( 0 );
	}

	return true;
}

//increments/decrements the volume of the whole song
bool MidiActionManager::master_volume_relative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nVolume = pAction->getValue().toInt(&ok,10);

	if ( nVolume != 0 ) {
		if ( nVolume == 1 && pSong->getVolume() < 1.5 ) {
			pSong->setVolume( pSong->getVolume() + 0.05 );
		} else if ( pSong->getVolume() >= 0.0 ) {
			pSong->setVolume( pSong->getVolume() - 0.05 );
		}
	} else {
		pSong->setVolume( 0 );
	}

	return true;
}

//sets the volume of a mixer strip to a given level (percentage)
bool MidiActionManager::strip_volume_absolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int nVolume = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	if ( nVolume != 0 ) {
		pInstr->setVolume( 1.5* ( (float) (nVolume / 127.0 ) ));
	} else {
		pInstr->setVolume( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber(nLine);
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

//increments/decrements the volume of one mixer strip
bool MidiActionManager::strip_volume_relative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();

	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int nVolume = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();

	auto pInstr = pInstrList->get( nLine );
	
	if ( pInstr == nullptr) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	if( nVolume != 0 ) {
		if ( nVolume == 1 && pInstr->getVolume() < 1.5 ) {
			pInstr->setVolume( pInstr->getVolume() + 0.1 );
		}
		else if( pInstr->getVolume() >= 0.0 ){
			pInstr->setVolume( pInstr->getVolume() - 0.1 );
		}
	}
	else {
		pInstr->setVolume( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

// sets the absolute panning of a given mixer channel
bool MidiActionManager::pan_absolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();

	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	pInstr->setPanWithRangeFrom0To1( (float) pan_param / 127.f );
	
	pHydrogen->setSelectedInstrumentNumber(nLine);

	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

// sets the absolute panning of a given mixer channel
bool MidiActionManager::pan_absolute_sym( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	pInstr->setPan( (float) pan_param / 127.f );
	
	pHydrogen->setSelectedInstrumentNumber(nLine);
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}


// changes the panning of a given mixer channel
// this is useful if the panning is set by a rotary control knob
bool MidiActionManager::pan_relative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	float fPan = pInstr->getPan();

	if ( pan_param == 1 && fPan < PAN_MAX ) {
		pInstr->setPan( fPan + 0.1 );
	}
	else if ( pan_param != 1 && fPan > PAN_MIN ) {
		pInstr->setPan( fPan - 0.1 );
	}

	pHydrogen->setSelectedInstrumentNumber(nLine);
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

bool MidiActionManager::gain_level_absolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int gain_param = pAction->getValue().toInt(&ok,10);
	int component_id = pAction->getParameter2().toInt(&ok,10);
	int layer_id = pAction->getParameter3().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	auto pComponent =  pInstr->getComponent( component_id );
	if( pComponent == nullptr) {
		ERRORLOG( QString( "Unable to retrieve component (Par. 2) [%1]" ).arg( component_id ) );
		return false;
	}
	
	auto pLayer = pComponent->getLayer( layer_id );
	if( pLayer == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve layer (Par. 3) [%1]" ).arg( layer_id ) );
		return false;
	}
	
	if ( gain_param != 0 ) {
		pLayer->setGain( 5.0* ( (float) (gain_param / 127.0 ) ) );
	} else {
		pLayer->setGain( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );
	
	return true;
}

bool MidiActionManager::pitch_level_absolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pitch_param = pAction->getValue().toInt(&ok,10);
	int component_id = pAction->getParameter2().toInt(&ok,10);
	int layer_id = pAction->getParameter3().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();

	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	auto pComponent =  pInstr->getComponent( component_id );
	if( pComponent == nullptr) {
		ERRORLOG( QString( "Unable to retrieve component (Par. 2) [%1]" ).arg( component_id ) );
		return false;
	}
	
	auto pLayer = pComponent->getLayer( layer_id );
	if( pLayer == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve layer (Par. 3) [%1]" ).arg( layer_id ) );
		return false;
	}
	
	if ( pitch_param != 0 ) {
		pLayer->setPitch(
			( Instrument::fPitchMax - Instrument::fPitchMin ) *
			( (float) (pitch_param / 127.0 ) ) + Instrument::fPitchMin );
	} else {
		pLayer->setPitch( Instrument::fPitchMin );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

bool MidiActionManager::instrument_pitch( std::shared_ptr<MidiAction> pAction ) {

	bool ok;
	float fPitch;
	const int nInstrument = pAction->getParameter1().toInt(&ok,10);
	const int nPitchMidi = pAction->getValue().toInt(&ok,10);
	if ( nPitchMidi != 0 ) {
		fPitch = ( Instrument::fPitchMax - Instrument::fPitchMin ) *
			( (float) (nPitchMidi / 127.0 ) ) + Instrument::fPitchMin;
	} else {
		fPitch = Instrument::fPitchMin;
	}

	return CoreActionController::setInstrumentPitch( nInstrument, fPitch );
}

bool MidiActionManager::filter_cutoff_level_absolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int filter_cutoff_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();

	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	pInstr->setFilterActive( true );
	if( filter_cutoff_param != 0 ) {
		pInstr->setFilterCutoff( ( (float) (filter_cutoff_param / 127.0 ) ) );
	} else {
		pInstr->setFilterCutoff( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );
	
	return true;
}


/*
 * increments/decrements the BPM
 * this is useful if the bpm is set by a rotary control knob
 */
bool MidiActionManager::bpm_cc_relative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	//this MidiAction should be triggered only by CC commands

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getValue().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	if ( m_nLastBpmChangeCCParameter >= cc_param &&
		 fBpm - mult > MIN_BPM ) {
		CoreActionController::setBpm( fBpm - 1*mult );
	}

	if ( m_nLastBpmChangeCCParameter < cc_param
		 && fBpm + mult < MAX_BPM ) {
		CoreActionController::setBpm( fBpm + 1*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;
	
	EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );

	return true;
}

/*
 * increments/decrements the BPM
 * this is useful if the bpm is set by a rotary control knob
 */
bool MidiActionManager::bpm_fine_cc_relative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	//this MidiAction should be triggered only by CC commands
	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getValue().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	if ( m_nLastBpmChangeCCParameter >= cc_param &&
		 fBpm - mult > MIN_BPM ) {
		CoreActionController::setBpm( fBpm - 0.01*mult );
	}
	if ( m_nLastBpmChangeCCParameter < cc_param
		 && fBpm + mult < MAX_BPM ) {
		CoreActionController::setBpm( fBpm + 0.01*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;
	
	EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );

	return true;
}

bool MidiActionManager::bpm_increase( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	const float fMult = pAction->getParameter1().toFloat();

	CoreActionController::setBpm( fBpm + 1 * fMult );
	
	EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );

	return true;
}

bool MidiActionManager::bpm_decrease( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	const float fMult = pAction->getParameter1().toFloat();

	CoreActionController::setBpm( fBpm - 1 * fMult );
	
	EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );

	return true;
}

bool MidiActionManager::next_bar( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pSong->getMode() == Song::Mode::Pattern ) {
		// Restart transport at the beginning of the pattern
		CoreActionController::locateToColumn( 0 );
		if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
			pAudioEngine->lock( RIGHT_HERE );
			pAudioEngine->updatePlayingPatterns( Event::Trigger::Default );
			pAudioEngine->unlock();
		}
	}
	else {
		const int nTotalColumns = pSong->getPatternGroupVector()->size();
		int nNewColumn = 1 +
			std::max( 0, pAudioEngine->getTransportPosition()->getColumn() );
		if ( nNewColumn >= nTotalColumns ) {
			if ( pSong->getLoopMode() == Song::LoopMode::Enabled ) {
				// Transport exceeds length of the song and is wrapped
				// to the beginning again.
				CoreActionController::locateToColumn( 0 );
			}
			else {
				// With loop mode disabled this command won't have any
				// effect in the last column.
			}
		}
		else {
			CoreActionController::locateToColumn( nNewColumn );
		}
	}
	
	return true;
}


bool MidiActionManager::previous_bar( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	if ( pSong->getMode() == Song::Mode::Pattern ) {
		// Restart transport at the beginning of the pattern. Does not
		// trigger activation of pending stacked patterns.
		CoreActionController::locateToColumn( 0 );
	}
	else {
		int nNewColumn = pHydrogen->getAudioEngine()->
			getTransportPosition()->getColumn() - 1;

		if ( nNewColumn < 0 ) {
			if ( pSong->getLoopMode() == Song::LoopMode::Enabled ) {
				// In case the song is looped, assume periodic
				// boundary conditions and move to the last column.
				CoreActionController::locateToColumn(
					pSong->getPatternGroupVector()->size() - 1 );
			}
			else {
				CoreActionController::locateToColumn( 0 );
			}
		}
		else {
			CoreActionController::locateToColumn( nNewColumn );
		}
	}
	return true;
}

bool MidiActionManager::setSongFromPlaylist( int nSongNumber ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pPlaylist = pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}

	if ( nSongNumber >= 0 && nSongNumber <= pPlaylist->size() - 1 ) {
		if ( pPlaylist->getActiveSongNumber() != nSongNumber ) {
			auto pSong = CoreActionController::loadSong(
				pPlaylist->getSongFilenameByNumber( nSongNumber ) );

			if ( pSong == nullptr ||
				 ! CoreActionController::setSong( pSong ) ) {
				ERRORLOG( QString( "Unable to set song [%1] of playlist" )
						  .arg( nSongNumber ) );
				return false;
			}
			CoreActionController::activatePlaylistSong( nSongNumber );
		}
	} else {
		// Preventive measure to avoid bad things.
		if ( pHydrogen->getPlaylist()->size() == 0 ) {
			___ERRORLOG( QString( "No songs added to the current playlist yet" ) );
		}
		else {
			___ERRORLOG( QString( "Provided song number [%1] out of bound [0,%2]" )
						 .arg( nSongNumber )
						 .arg( pHydrogen->getPlaylist()->size() - 1 ) );
		}
		return false;
	}
	return true;
}

bool MidiActionManager::playlist_song( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	int songnumber = pAction->getParameter1().toInt(&ok,10);
	return setSongFromPlaylist( songnumber );
}

bool MidiActionManager::playlist_next_song( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	int songnumber = pHydrogen->getPlaylist()->getActiveSongNumber();
	return setSongFromPlaylist( ++songnumber );
}

bool MidiActionManager::playlist_previous_song( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	int songnumber = pHydrogen->getPlaylist()->getActiveSongNumber();
	return setSongFromPlaylist( --songnumber );
}

bool MidiActionManager::record_ready( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( pHydrogen->getAudioEngine()->getState() != AudioEngine::State::Playing ) {
		return CoreActionController::toggleRecordMode();
	}
	return true;
}

bool MidiActionManager::record_strobe_toggle( std::shared_ptr<MidiAction>  ) {
	return CoreActionController::toggleRecordMode();
}

bool MidiActionManager::record_strobe( std::shared_ptr<MidiAction>  ) {
	return CoreActionController::activateRecordMode( true );
}

bool MidiActionManager::record_exit( std::shared_ptr<MidiAction> ) {
	return CoreActionController::activateRecordMode( false );
}

bool MidiActionManager::toggle_metronome( std::shared_ptr<MidiAction> ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	// Use the wrapper in CAC over a plain setting of the parameter in
	// order to send MIDI feedback
	CoreActionController::setMetronomeIsActive(
		! Preferences::get_instance()->m_bUseMetronome );
	
	return true;
}

bool MidiActionManager::undo_action( std::shared_ptr<MidiAction> ) {
	EventQueue::get_instance()->pushEvent( Event::Type::UndoRedo, 0);// 0 = undo
	return true;
}

bool MidiActionManager::redo_action( std::shared_ptr<MidiAction> ) {
	EventQueue::get_instance()->pushEvent( Event::Type::UndoRedo, 1);// 1 = redo
	return true;
}

bool MidiActionManager::loadNextDrumkit( std::shared_ptr<MidiAction> ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	return CoreActionController::setDrumkit(
		pHydrogen->getSoundLibraryDatabase()->getNextDrumkit() );
}

bool MidiActionManager::loadPrevDrumkit( std::shared_ptr<MidiAction> ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	return CoreActionController::setDrumkit(
		pHydrogen->getSoundLibraryDatabase()->getPreviousDrumkit() );
}

int MidiActionManager::getParameterNumber( const MidiAction::Type& type ) const {
	auto foundActionPair = m_midiActionMap.find( type );
	if ( foundActionPair != m_midiActionMap.end() ) {
		return foundActionPair->second.second;
	}
	else {
		ERRORLOG( QString( "MIDI MidiAction type [%1] couldn't be found" )
				  .arg( MidiAction::typeToQString( type ) ) );
	}
		
	return -1;
}

bool MidiActionManager::clear_selected_instrument( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	const int nInstr = pHydrogen->getSelectedInstrumentNumber();
	if ( nInstr == -1 ) {
		WARNINGLOG( "No instrument selected" );
		return false;
	}

	return CoreActionController::clearInstrumentInPattern( nInstr );
}

bool MidiActionManager::clear_pattern( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	int nPattern = pHydrogen->getSelectedPatternNumber();

	auto pPattern = pSong->getPatternList()->get( nPattern );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" ).arg( nPattern ) );
		return false;
	}

	pPattern->clear( true );

	EventQueue::get_instance()->pushEvent( Event::Type::PatternModified, 0 );

	return true;
}

bool MidiActionManager::handleMidiActions( const std::vector<std::shared_ptr<MidiAction>>& midiActions ) {

	bool bResult = false;
	
	for ( const auto& ppMidiAction : midiActions ) {
		if ( ppMidiAction != nullptr ) {
			if ( handleMidiAction( ppMidiAction ) ) {
				bResult = true;
			}
		}
	}

	return bResult;
}

bool MidiActionManager::handleMidiAction( const std::shared_ptr<MidiAction> pAction ) {

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	/*
		return false if Midiaction is null
		(for example if no MidiAction exists for an event)
	*/
	if ( pAction == nullptr ) {
		return false;
	}

	auto foundActionPair = m_midiActionMap.find( pAction->getType() );
	if( foundActionPair != m_midiActionMap.end() ) {
		action_f Midiaction = foundActionPair->second.first;
		return (this->*Midiaction)(pAction);
	}
	else {
		ERRORLOG( QString( "MIDI MidiAction type [%1] couldn't be found" )
				  .arg( MidiAction::typeToQString( pAction->getType() ) ) );
	}

	return false;
}
