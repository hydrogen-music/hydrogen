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
#include <QObject>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/EventQueue.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>

#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/Basics/PatternList.h>

#include <core/Preferences/Preferences.h>
#include <core/MidiAction.h>

#include <core/Basics/Drumkit.h>

// #include <QFileInfo>

#include <sstream>

using namespace H2Core;

/**
* @class MidiAction
*
* @brief This class represents a midi action.
*
* This class represents actions which can be executed
* after a midi event occurred. An example is the "MUTE"
* action, which mutes the outputs of hydrogen.
*
* An action can be linked to an event. If this event occurs,
* the action gets triggered. The handling of events takes place
* in midi_input.cpp .
*
* Each action has two independent parameters. The two parameters are optional and
* can be used to carry additional information, which mean
* only something to this very Action. They can have totally different meanings for other Actions.
* Example: parameter1 is the Mixer strip and parameter 2 a multiplier for the volume change on this strip
*
* @author Sebastian Moors
*
*/

Action::Action( QString sType ) {
	m_sType = sType;
	m_sParameter1 = "0";
	m_sParameter2 = "0";
	m_sParameter3 = "0";
	m_sValue = "0";
}

Action::Action( std::shared_ptr<Action> pOther ) {
	m_sType = pOther->m_sType;
	m_sParameter1 = pOther->m_sParameter1;
	m_sParameter2 = pOther->m_sParameter2;
	m_sParameter3 = pOther->m_sParameter3;
	m_sValue = pOther->m_sValue;
}

bool Action::isNull() const {
	return m_sType == Action::getNullActionType();
}

bool Action::isEquivalentTo( std::shared_ptr<Action> pOther ) {
	if ( pOther == nullptr ) {
		return false;
	}
	
	return ( m_sType == pOther->m_sType &&
			 m_sParameter1 == pOther->m_sParameter1 &&
			 m_sParameter2 == pOther->m_sParameter2 &&
			 m_sParameter3 == pOther->m_sParameter3 );
}

QString Action::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Action]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_sType: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sType ) )
			.append( QString( "%1%2m_sValue: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sValue ) )
			.append( QString( "%1%2m_sParameter1: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sParameter1 ) )
			.append( QString( "%1%2m_sParameter2: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sParameter2 ) )
			.append( QString( "%1%2m_sParameter3: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sParameter3 ) );
	} else {
		sOutput = QString( "[Action]" )
			.append( QString( " m_sType: %1" ).arg( m_sType ) )
			.append( QString( ", m_sValue: %1" ).arg( m_sValue ) )
			.append( QString( ", m_sParameter1: %1" ).arg( m_sParameter1 ) )
			.append( QString( ", m_sParameter2: %1" ).arg( m_sParameter2 ) )
			.append( QString( ", m_sParameter3: %1" ).arg( m_sParameter3 ) );
	}
	
	return sOutput;
}

/**
* @class MidiActionManager
*
* @brief The MidiActionManager cares for the execution of MidiActions
*
*
* The MidiActionManager handles the execution of midi actions. The class
* includes the names and implementations of all possible actions.
*
*
* @author Sebastian Moors
*
*/
MidiActionManager* MidiActionManager::__instance = nullptr;

MidiActionManager::MidiActionManager() {
	__instance = this;

	m_nLastBpmChangeCCParameter = -1;
	/*
		the m_actionMap holds all Action identifiers which hydrogen is able to interpret.
		it holds pointer to member function
	*/
	m_actionMap.insert(std::make_pair("PLAY", std::make_pair( &MidiActionManager::play, 0 ) ));
	m_actionMap.insert(std::make_pair("PLAY/STOP_TOGGLE", std::make_pair( &MidiActionManager::play_stop_pause_toggle, 0 ) ));
	m_actionMap.insert(std::make_pair("PLAY/PAUSE_TOGGLE", std::make_pair( &MidiActionManager::play_stop_pause_toggle, 0 ) ));
	m_actionMap.insert(std::make_pair("STOP", std::make_pair( &MidiActionManager::stop, 0 ) ));
	m_actionMap.insert(std::make_pair("PAUSE", std::make_pair( &MidiActionManager::pause, 0 ) ));
	m_actionMap.insert(std::make_pair("RECORD_READY", std::make_pair( &MidiActionManager::record_ready, 0 ) ));
	m_actionMap.insert(std::make_pair("RECORD/STROBE_TOGGLE", std::make_pair( &MidiActionManager::record_strobe_toggle, 0 ) ));
	m_actionMap.insert(std::make_pair("RECORD_STROBE", std::make_pair( &MidiActionManager::record_strobe, 0 ) ));
	m_actionMap.insert(std::make_pair("RECORD_EXIT", std::make_pair( &MidiActionManager::record_exit, 0 ) ));
	m_actionMap.insert(std::make_pair("MUTE", std::make_pair( &MidiActionManager::mute, 0 ) ));
	m_actionMap.insert(std::make_pair("UNMUTE", std::make_pair( &MidiActionManager::unmute, 0 ) ));
	m_actionMap.insert(std::make_pair("MUTE_TOGGLE", std::make_pair( &MidiActionManager::mute_toggle, 0 ) ));
	m_actionMap.insert(std::make_pair("STRIP_MUTE_TOGGLE", std::make_pair( &MidiActionManager::strip_mute_toggle, 1 ) ));
	m_actionMap.insert(std::make_pair("STRIP_SOLO_TOGGLE", std::make_pair( &MidiActionManager::strip_solo_toggle, 1 ) ));	
	m_actionMap.insert(std::make_pair(">>_NEXT_BAR", std::make_pair( &MidiActionManager::next_bar, 0 ) ));
	m_actionMap.insert(std::make_pair("<<_PREVIOUS_BAR", std::make_pair( &MidiActionManager::previous_bar, 0 ) ));
	m_actionMap.insert(std::make_pair("BPM_INCR", std::make_pair( &MidiActionManager::bpm_increase, 1 ) ));
	m_actionMap.insert(std::make_pair("BPM_DECR", std::make_pair( &MidiActionManager::bpm_decrease, 1 ) ));
	m_actionMap.insert(std::make_pair("BPM_CC_RELATIVE", std::make_pair( &MidiActionManager::bpm_cc_relative, 1 ) ));
	m_actionMap.insert(std::make_pair("BPM_FINE_CC_RELATIVE", std::make_pair( &MidiActionManager::bpm_fine_cc_relative, 1 ) ));
	m_actionMap.insert(std::make_pair("MASTER_VOLUME_RELATIVE", std::make_pair( &MidiActionManager::master_volume_relative, 0 ) ));
	m_actionMap.insert(std::make_pair("MASTER_VOLUME_ABSOLUTE", std::make_pair( &MidiActionManager::master_volume_absolute, 0 ) ));
	m_actionMap.insert(std::make_pair("STRIP_VOLUME_RELATIVE", std::make_pair( &MidiActionManager::strip_volume_relative, 1 ) ));
	m_actionMap.insert(std::make_pair("STRIP_VOLUME_ABSOLUTE", std::make_pair( &MidiActionManager::strip_volume_absolute, 1 ) ));
	m_actionMap.insert(std::make_pair("EFFECT_LEVEL_ABSOLUTE", std::make_pair( &MidiActionManager::effect_level_absolute, 2 ) ));
	m_actionMap.insert(std::make_pair("EFFECT_LEVEL_RELATIVE", std::make_pair( &MidiActionManager::effect_level_relative, 2 ) ));
	m_actionMap.insert(std::make_pair("GAIN_LEVEL_ABSOLUTE", std::make_pair( &MidiActionManager::gain_level_absolute, 3 ) ));
	m_actionMap.insert(std::make_pair("PITCH_LEVEL_ABSOLUTE", std::make_pair( &MidiActionManager::pitch_level_absolute, 3 ) ));
	m_actionMap.insert(std::make_pair("SELECT_NEXT_PATTERN", std::make_pair( &MidiActionManager::select_next_pattern, 1 ) ));
	m_actionMap.insert(std::make_pair("SELECT_ONLY_NEXT_PATTERN", std::make_pair( &MidiActionManager::select_only_next_pattern, 1 ) ));
	m_actionMap.insert(std::make_pair("SELECT_NEXT_PATTERN_CC_ABSOLUTE", std::make_pair( &MidiActionManager::select_next_pattern_cc_absolute, 0 ) ));
	m_actionMap.insert(std::make_pair("SELECT_ONLY_NEXT_PATTERN_CC_ABSOLUTE", std::make_pair( &MidiActionManager::select_only_next_pattern_cc_absolute, 0 ) ));
	m_actionMap.insert(std::make_pair("SELECT_NEXT_PATTERN_RELATIVE", std::make_pair( &MidiActionManager::select_next_pattern_relative, 1 ) ));
	m_actionMap.insert(std::make_pair("SELECT_AND_PLAY_PATTERN", std::make_pair( &MidiActionManager::select_and_play_pattern, 1 ) ));
	m_actionMap.insert(std::make_pair("PAN_RELATIVE", std::make_pair( &MidiActionManager::pan_relative, 1 ) ));
	m_actionMap.insert(std::make_pair("PAN_ABSOLUTE", std::make_pair( &MidiActionManager::pan_absolute, 1 ) ));
	m_actionMap.insert(std::make_pair("PAN_ABSOLUTE_SYM", std::make_pair( &MidiActionManager::pan_absolute_sym, 1 ) ));
	m_actionMap.insert(std::make_pair("INSTRUMENT_PITCH",
									  std::make_pair( &MidiActionManager::instrument_pitch, 1 ) ));
	m_actionMap.insert(std::make_pair("FILTER_CUTOFF_LEVEL_ABSOLUTE", std::make_pair( &MidiActionManager::filter_cutoff_level_absolute, 1 ) ));
	m_actionMap.insert(std::make_pair("BEATCOUNTER", std::make_pair( &MidiActionManager::beatcounter, 0 ) ));
	m_actionMap.insert(std::make_pair("TAP_TEMPO", std::make_pair( &MidiActionManager::tap_tempo, 0 ) ));
	m_actionMap.insert(std::make_pair("PLAYLIST_SONG", std::make_pair( &MidiActionManager::playlist_song, 1 ) ));
	m_actionMap.insert(std::make_pair("PLAYLIST_NEXT_SONG", std::make_pair( &MidiActionManager::playlist_next_song, 0 ) ));
	m_actionMap.insert(std::make_pair("PLAYLIST_PREV_SONG", std::make_pair( &MidiActionManager::playlist_previous_song, 0 ) ));
	m_actionMap.insert(std::make_pair("TOGGLE_METRONOME", std::make_pair( &MidiActionManager::toggle_metronome, 0 ) ));
	m_actionMap.insert(std::make_pair("SELECT_INSTRUMENT", std::make_pair( &MidiActionManager::select_instrument, 0 ) ));
	m_actionMap.insert(std::make_pair("UNDO_ACTION", std::make_pair( &MidiActionManager::undo_action, 0 ) ));
	m_actionMap.insert(std::make_pair("REDO_ACTION", std::make_pair( &MidiActionManager::redo_action, 0 ) ));
	m_actionMap.insert(std::make_pair("CLEAR_SELECTED_INSTRUMENT", std::make_pair(
										  &MidiActionManager::clear_selected_instrument, 0 ) ));
	m_actionMap.insert(std::make_pair("CLEAR_PATTERN", std::make_pair(
										  &MidiActionManager::clear_pattern, 0 ) ));
	/*
	  the m_actionList holds all Action identifiers which hydrogen is able to interpret.
	*/
	m_actionList <<"";
	for ( const auto& ppAction : m_actionMap ) {
		m_actionList << ppAction.first;
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

bool MidiActionManager::play( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Ready ) {
		pHydrogen->sequencer_play();
	}
	return true;
}

bool MidiActionManager::pause( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->sequencer_stop();
	return true;
}

bool MidiActionManager::stop( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->sequencer_stop();
	return pHydrogen->getCoreActionController()->locateToColumn( 0 );
}

bool MidiActionManager::play_stop_pause_toggle( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	QString sActionString = pAction->getType();
	switch ( pHydrogen->getAudioEngine()->getState() )
	{
	case AudioEngine::State::Ready:
		pHydrogen->sequencer_play();
		break;

	case AudioEngine::State::Playing:
		if( sActionString == "PLAY/STOP_TOGGLE" ) {
			pHydrogen->getCoreActionController()->locateToColumn( 0 );
		}
		pHydrogen->sequencer_stop();
		break;

	default:
		ERRORLOG( "[Hydrogen::ActionManager(PLAY): Unhandled case" );
		break;
	}

	return true;
}

//mutes the master, not a single strip
bool MidiActionManager::mute( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return pHydrogen->getCoreActionController()->setMasterIsMuted( true );
}

bool MidiActionManager::unmute( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return pHydrogen->getCoreActionController()->setMasterIsMuted( false );
}

bool MidiActionManager::mute_toggle( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return pHydrogen->getCoreActionController()->setMasterIsMuted( !pHydrogen->getSong()->getIsMuted() );
}

bool MidiActionManager::strip_mute_toggle( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);

	auto pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	return pHydrogen->getCoreActionController()->setStripIsMuted( nLine, !pInstr->is_muted() );
}

bool MidiActionManager::strip_solo_toggle( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);

	auto pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	return pHydrogen->getCoreActionController()->setStripIsSoloed( nLine, !pInstr->is_soloed() );
}

bool MidiActionManager::beatcounter( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return pHydrogen->handleBeatCounter();
}

bool MidiActionManager::tap_tempo( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->onTapTempoAccelEvent();
	return true;
}

bool MidiActionManager::select_next_pattern( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	bool ok;
	return nextPatternSelection( pAction->getParameter1().toInt(&ok,10) );
}


bool MidiActionManager::select_next_pattern_relative( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	bool ok;
	return nextPatternSelection( pHydrogen->getSelectedPatternNumber() +
								 pAction->getParameter1().toInt(&ok,10) );
}

bool MidiActionManager::select_next_pattern_cc_absolute( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
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
		pHydrogen->setSelectedPatternNumber( nPatternNumber );
	}
	else if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
		pHydrogen->toggleNextPattern( nPatternNumber );
	}
	
	return true;
}

bool MidiActionManager::select_only_next_pattern( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	bool ok;
	return onlyNextPatternSelection( pAction->getParameter1().toInt(&ok,10) );
}

bool MidiActionManager::select_only_next_pattern_cc_absolute( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
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

bool MidiActionManager::select_and_play_pattern( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( ! select_next_pattern( pAction, pHydrogen ) ) {
		return false;
	}

	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Ready ) {
		pHydrogen->sequencer_play();
	}

	return true;
}

bool MidiActionManager::select_instrument( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int  nInstrumentNumber = pAction->getValue().toInt(&ok,10) ;

	if ( pSong->getInstrumentList()->size() < nInstrumentNumber ) {
		nInstrumentNumber = pSong->getInstrumentList()->size() -1;
	} else if ( nInstrumentNumber < 0 ) {
		nInstrumentNumber = 0;
	}
	
	pHydrogen->setSelectedInstrumentNumber( nInstrumentNumber );
	return true;
}

bool MidiActionManager::effect_level_absolute( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen) {
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

	auto pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
		
	if( fx_param != 0 ) {
		pInstr->set_fx_level( (float) (fx_param / 127.0 ), fx_id );
	} else {
		pInstr->set_fx_level( 0 , fx_id );
	}
			
	pHydrogen->setSelectedInstrumentNumber( nLine );

	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );

	return true;
}

bool MidiActionManager::effect_level_relative( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
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

	auto pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	if ( fx_param != 0 ) {
		if ( fx_param == 1 && pInstr->get_fx_level( fx_id ) <= 0.95 ) {
			pInstr->set_fx_level( pInstr->get_fx_level( fx_id ) + 0.05, fx_id );
		}
		else if ( pInstr->get_fx_level( fx_id ) >= 0.05 ) {
			pInstr->set_fx_level( pInstr->get_fx_level( fx_id ) - 0.05, fx_id );
		}
	}
			
	pHydrogen->setSelectedInstrumentNumber( nLine );

	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );

	return true;
}

//sets the volume of a master output to a given level (percentage)
bool MidiActionManager::master_volume_absolute( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
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
bool MidiActionManager::master_volume_relative( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
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
bool MidiActionManager::strip_volume_absolute( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int nVolume = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	if ( nVolume != 0 ) {
		pInstr->set_volume( 1.5* ( (float) (nVolume / 127.0 ) ));
	} else {
		pInstr->set_volume( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber(nLine);
	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );

	return true;
}

//increments/decrements the volume of one mixer strip
bool MidiActionManager::strip_volume_relative( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	auto pSong = pHydrogen->getSong();

	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int nVolume = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nLine );
	
	if ( pInstr == nullptr) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	if( nVolume != 0 ) {
		if ( nVolume == 1 && pInstr->get_volume() < 1.5 ) {
			pInstr->set_volume( pInstr->get_volume() + 0.1 );
		}
		else if( pInstr->get_volume() >= 0.0 ){
			pInstr->set_volume( pInstr->get_volume() - 0.1 );
		}
	}
	else {
		pInstr->set_volume( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );

	return true;
}

// sets the absolute panning of a given mixer channel
bool MidiActionManager::pan_absolute( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	pInstr->setPanWithRangeFrom0To1( (float) pan_param / 127.f );
	
	pHydrogen->setSelectedInstrumentNumber(nLine);

	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );

	return true;
}

// sets the absolute panning of a given mixer channel
bool MidiActionManager::pan_absolute_sym( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	pInstr->setPan( (float) pan_param / 127.f );
	
	pHydrogen->setSelectedInstrumentNumber(nLine);
	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );

	return true;
}


// changes the panning of a given mixer channel
// this is useful if the panning is set by a rotary control knob
bool MidiActionManager::pan_relative( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	float fPan = pInstr->getPan();

	if ( pan_param == 1 && fPan < 1.f ) {
		pInstr->setPan( fPan + 0.1 );
	}
	else if ( pan_param != 1 && fPan > -1.f ) {
		pInstr->setPan( fPan - 0.1 );
	}

	pHydrogen->setSelectedInstrumentNumber(nLine);
	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );

	return true;
}

bool MidiActionManager::gain_level_absolute( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
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

	auto pInstrList = pSong->getInstrumentList();
	
	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	auto pComponent =  pInstr->get_component( component_id );
	if( pComponent == nullptr) {
		ERRORLOG( QString( "Unable to retrieve component (Par. 2) [%1]" ).arg( component_id ) );
		return false;
	}
	
	auto pLayer = pComponent->get_layer( layer_id );
	if( pLayer == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve layer (Par. 3) [%1]" ).arg( layer_id ) );
		return false;
	}
	
	if ( gain_param != 0 ) {
		pLayer->set_gain( 5.0* ( (float) (gain_param / 127.0 ) ) );
	} else {
		pLayer->set_gain( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );
	
	return true;
}

bool MidiActionManager::pitch_level_absolute( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
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

	auto pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	auto pComponent =  pInstr->get_component( component_id );
	if( pComponent == nullptr) {
		ERRORLOG( QString( "Unable to retrieve component (Par. 2) [%1]" ).arg( component_id ) );
		return false;
	}
	
	auto pLayer = pComponent->get_layer( layer_id );
	if( pLayer == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve layer (Par. 3) [%1]" ).arg( layer_id ) );
		return false;
	}
	
	if ( pitch_param != 0 ) {
		pLayer->set_pitch(
			( Instrument::fPitchMax - Instrument::fPitchMin ) *
			( (float) (pitch_param / 127.0 ) ) + Instrument::fPitchMin );
	} else {
		pLayer->set_pitch( Instrument::fPitchMin );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );

	return true;
}

bool MidiActionManager::instrument_pitch( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {

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

	return pHydrogen->getCoreActionController()->
		setInstrumentPitch( nInstrument, fPitch );
}

bool MidiActionManager::filter_cutoff_level_absolute( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int filter_cutoff_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getInstrumentList();

	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	pInstr->set_filter_active( true );
	if( filter_cutoff_param != 0 ) {
		pInstr->set_filter_cutoff( ( (float) (filter_cutoff_param / 127.0 ) ) );
	} else {
		pInstr->set_filter_cutoff( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->push_event( EVENT_INSTRUMENT_PARAMETERS_CHANGED, nLine );
	
	return true;
}


/*
 * increments/decrements the BPM
 * this is useful if the bpm is set by a rotary control knob
 */
bool MidiActionManager::bpm_cc_relative( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	//this Action should be triggered only by CC commands

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getValue().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	if ( m_nLastBpmChangeCCParameter >= cc_param &&
		 fBpm - mult > MIN_BPM ) {
		// Use tempo in the next process cycle of the audio engine.
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setNextBpm( fBpm - 1*mult );
		pAudioEngine->unlock();
		// Store it's value in the .h2song file.
		pHydrogen->getSong()->setBpm( fBpm - 1*mult );
	}

	if ( m_nLastBpmChangeCCParameter < cc_param
		 && fBpm + mult < MAX_BPM ) {
		// Use tempo in the next process cycle of the audio engine.
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setNextBpm( fBpm + 1*mult );
		pAudioEngine->unlock();
		// Store it's value in the .h2song file.
		pHydrogen->getSong()->setBpm( fBpm + 1*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );

	return true;
}

/*
 * increments/decrements the BPM
 * this is useful if the bpm is set by a rotary control knob
 */
bool MidiActionManager::bpm_fine_cc_relative( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	//this Action should be triggered only by CC commands
	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getValue().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	if ( m_nLastBpmChangeCCParameter >= cc_param &&
		 fBpm - mult > MIN_BPM ) {
		// Use tempo in the next process cycle of the audio engine.
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setNextBpm( fBpm - 0.01*mult );
		pAudioEngine->unlock();
		// Store it's value in the .h2song file.
		pHydrogen->getSong()->setBpm( fBpm - 0.01*mult );
	}
	if ( m_nLastBpmChangeCCParameter < cc_param
		 && fBpm + mult < MAX_BPM ) {
		// Use tempo in the next process cycle of the audio engine.
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setNextBpm( fBpm + 0.01*mult );
		pAudioEngine->unlock();
		// Store it's value in the .h2song file.
		pHydrogen->getSong()->setBpm( fBpm + 0.01*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );

	return true;
}

bool MidiActionManager::bpm_increase( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);

	// Use tempo in the next process cycle of the audio engine.
	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->setNextBpm( fBpm + 1*mult );
	pAudioEngine->unlock();
	// Store it's value in the .h2song file.
	pHydrogen->getSong()->setBpm( fBpm + 1*mult );
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );

	return true;
}

bool MidiActionManager::bpm_decrease( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);

	// Use tempo in the next process cycle of the audio engine.
	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->setNextBpm( fBpm - 1*mult );
	pAudioEngine->unlock();
	// Store it's value in the .h2song file.
	pHydrogen->getSong()->setBpm( fBpm - 1*mult );
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );

	return true;
}

bool MidiActionManager::next_bar( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	int nNewColumn = std::max( 0, pHydrogen->getAudioEngine()->
							   getTransportPosition()->getColumn() ) + 1;
	
	pHydrogen->getCoreActionController()->locateToColumn( nNewColumn );
	return true;
}


bool MidiActionManager::previous_bar( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->getCoreActionController()->locateToColumn(
		pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() -1 );
	return true;
}

bool MidiActionManager::setSong( int nSongNumber, Hydrogen* pHydrogen ) {
	int nActiveSongNumber = Playlist::get_instance()->getActiveSongNumber();
	if( nSongNumber >= 0 && nSongNumber <= Playlist::get_instance()->size() - 1 ) {
		if ( nActiveSongNumber != nSongNumber ) {
			Playlist::get_instance()->setNextSongByNumber( nSongNumber );
		}
	} else {
		// Preventive measure to avoid bad things.
		if ( pHydrogen->getSong() == nullptr ) {
			___ERRORLOG( "No song set yet" );
		}
		else if ( Playlist::get_instance()->size() == 0 ) {
			___ERRORLOG( QString( "No songs added to the current playlist yet" ) );
		}
		else {
			___ERRORLOG( QString( "Provided song number [%1] out of bound [0,%2]" )
						 .arg( nSongNumber )
						 .arg( Playlist::get_instance()->size() - 1 ) );
		}
		return false;
	}
	return true;
}

bool MidiActionManager::playlist_song( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	bool ok;
	int songnumber = pAction->getParameter1().toInt(&ok,10);
	return setSong( songnumber, pHydrogen );
}

bool MidiActionManager::playlist_next_song( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	int songnumber = Playlist::get_instance()->getActiveSongNumber();
	return setSong( ++songnumber, pHydrogen );
}

bool MidiActionManager::playlist_previous_song( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	int songnumber = Playlist::get_instance()->getActiveSongNumber();
	return setSong( --songnumber, pHydrogen );
}

bool MidiActionManager::record_ready( std::shared_ptr<Action> pAction, Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( pHydrogen->getAudioEngine()->getState() != AudioEngine::State::Playing ) {
		if (!Preferences::get_instance()->getRecordEvents()) {
			Preferences::get_instance()->setRecordEvents(true);
		}
		else {
			Preferences::get_instance()->setRecordEvents(false);
		}
	}
	return true;
}

bool MidiActionManager::record_strobe_toggle( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if (!Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(true);
	}
	else {
		Preferences::get_instance()->setRecordEvents(false);
	}
	return true;
}

bool MidiActionManager::record_strobe( std::shared_ptr<Action> , Hydrogen* pHydrogen ) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if (!Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(true);
	}
	return true;
}

bool MidiActionManager::record_exit( std::shared_ptr<Action> , Hydrogen* pHydrogen) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if (Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(false);
	}
	return true;
}

bool MidiActionManager::toggle_metronome( std::shared_ptr<Action> , Hydrogen* pHydrogen) {
	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	// Use the wrapper in CAC over a plain setting of the parameter in
	// order to send MIDI feedback
	pHydrogen->getCoreActionController()->setMetronomeIsActive( 
		! Preferences::get_instance()->m_bUseMetronome );
	
	return true;
}

bool MidiActionManager::undo_action( std::shared_ptr<Action> , Hydrogen* ) {
	EventQueue::get_instance()->push_event( EVENT_UNDO_REDO, 0);// 0 = undo
	return true;
}

bool MidiActionManager::redo_action( std::shared_ptr<Action> , Hydrogen* ) {
	EventQueue::get_instance()->push_event( EVENT_UNDO_REDO, 1);// 1 = redo
	return true;
}

int MidiActionManager::getParameterNumber( const QString& sActionType ) const {
	auto foundActionPair = m_actionMap.find( sActionType );
	if ( foundActionPair != m_actionMap.end() ) {
		return foundActionPair->second.second;
	} else {
		ERRORLOG( QString( "MIDI Action type [%1] couldn't be found" ).arg( sActionType ) );
	}
		
	return -1;
}

bool MidiActionManager::clear_selected_instrument( std::shared_ptr<Action> pAction,
												   Hydrogen* pHydrogen ) {
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

	return pHydrogen->getCoreActionController()->clearInstrumentInPattern( nInstr );
}

bool MidiActionManager::clear_pattern( std::shared_ptr<Action> pAction,
										   Hydrogen* pHydrogen ) {
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

	if ( pHydrogen->getGUIState() != Hydrogen::GUIState::unavailable ) {
		EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, 0 );
	}

	return true;
}

bool MidiActionManager::handleActions( std::vector<std::shared_ptr<Action>> actions ) {

	bool bResult = false;
	
	for ( const auto& action : actions ) {
		if ( action != nullptr ) {
			if ( handleAction( action ) ) {
				bResult = true;
			}
		}
	}

	return bResult;
}

bool MidiActionManager::handleAction(  std::shared_ptr<Action> pAction ) {

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	/*
		return false if action is null
		(for example if no Action exists for an event)
	*/
	if( pAction == nullptr ) {
		return false;
	}

	QString sActionString = pAction->getType();

	auto foundActionPair = m_actionMap.find( sActionString );
	if( foundActionPair != m_actionMap.end() ) {
		action_f action = foundActionPair->second.first;
		return (this->*action)(pAction, pHydrogen);
	} else {
		ERRORLOG( QString( "MIDI Action type [%1] couldn't be found" ).arg( sActionString ) );
	}

	return false;
}
