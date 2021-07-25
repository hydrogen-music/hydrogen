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
#include <QObject>

#include <core/AudioEngine.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>

#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/Basics/PatternList.h>

#include <core/Preferences.h>
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

const char* Action::__class_name = "MidiAction";

Action::Action( QString typeString ) : Object( __class_name ) {
	m_sType = typeString;
	m_sParameter1 = "0";
	m_sParameter2 = "0";
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
const char* MidiActionManager::__class_name = "ActionManager";

MidiActionManager::MidiActionManager() : Object( __class_name ) {
	__instance = this;

	m_nLastBpmChangeCCParameter = -1;
	/*
		the actionMap holds all Action identifiers which hydrogen is able to interpret.
		it holds pointer to member function
	*/
	targeted_element empty = {0,0};
	actionMap.insert(std::make_pair("PLAY", std::make_pair(&MidiActionManager::play, empty)));
	actionMap.insert(std::make_pair("PLAY/STOP_TOGGLE", std::make_pair(&MidiActionManager::play_stop_pause_toggle, empty)));
	actionMap.insert(std::make_pair("PLAY/PAUSE_TOGGLE", std::make_pair(&MidiActionManager::play_stop_pause_toggle, empty)));
	actionMap.insert(std::make_pair("STOP", std::make_pair(&MidiActionManager::stop, empty)));
	actionMap.insert(std::make_pair("PAUSE", std::make_pair(&MidiActionManager::pause, empty)));
	actionMap.insert(std::make_pair("RECORD_READY", std::make_pair(&MidiActionManager::record_ready, empty)));
	actionMap.insert(std::make_pair("RECORD/STROBE_TOGGLE", std::make_pair(&MidiActionManager::record_strobe_toggle, empty)));
	actionMap.insert(std::make_pair("RECORD_STROBE", std::make_pair(&MidiActionManager::record_strobe, empty)));
	actionMap.insert(std::make_pair("RECORD_EXIT", std::make_pair(&MidiActionManager::record_exit, empty)));
	actionMap.insert(std::make_pair("MUTE", std::make_pair(&MidiActionManager::mute, empty)));
	actionMap.insert(std::make_pair("UNMUTE", std::make_pair(&MidiActionManager::unmute, empty)));
	actionMap.insert(std::make_pair("MUTE_TOGGLE", std::make_pair(&MidiActionManager::mute_toggle, empty)));
	actionMap.insert(std::make_pair("STRIP_MUTE_TOGGLE", std::make_pair(&MidiActionManager::strip_mute_toggle, empty)));
	actionMap.insert(std::make_pair("STRIP_SOLO_TOGGLE", std::make_pair(&MidiActionManager::strip_solo_toggle, empty)));	
	actionMap.insert(std::make_pair(">>_NEXT_BAR", std::make_pair(&MidiActionManager::next_bar, empty)));
	actionMap.insert(std::make_pair("<<_PREVIOUS_BAR", std::make_pair(&MidiActionManager::previous_bar, empty)));
	actionMap.insert(std::make_pair("BPM_INCR", std::make_pair(&MidiActionManager::bpm_increase, empty)));
	actionMap.insert(std::make_pair("BPM_DECR", std::make_pair(&MidiActionManager::bpm_decrease, empty)));
	actionMap.insert(std::make_pair("BPM_CC_RELATIVE", std::make_pair(&MidiActionManager::bpm_cc_relative, empty)));
	actionMap.insert(std::make_pair("BPM_FINE_CC_RELATIVE", std::make_pair(&MidiActionManager::bpm_fine_cc_relative, empty)));
	actionMap.insert(std::make_pair("MASTER_VOLUME_RELATIVE", std::make_pair(&MidiActionManager::master_volume_relative, empty)));
	actionMap.insert(std::make_pair("MASTER_VOLUME_ABSOLUTE", std::make_pair(&MidiActionManager::master_volume_absolute, empty)));
	actionMap.insert(std::make_pair("STRIP_VOLUME_RELATIVE", std::make_pair(&MidiActionManager::strip_volume_relative, empty)));
	actionMap.insert(std::make_pair("STRIP_VOLUME_ABSOLUTE", std::make_pair(&MidiActionManager::strip_volume_absolute, empty)));

	actionMap.insert(std::make_pair("HUMANIZE_VELOCITY_ABSOLUTE", std::make_pair(&MidiActionManager::humanize_velocity_absolute, empty)));
	actionMap.insert(std::make_pair("HUMANIZE_TIME_ABSOLUTE", std::make_pair(&MidiActionManager::humanize_time_absolute, empty)));
	actionMap.insert(std::make_pair("SWING_ABSOLUTE", std::make_pair(&MidiActionManager::swing_absolute, empty)));
	actionMap.insert(std::make_pair("FILL_VALUE_ABSOLUTE", std::make_pair(&MidiActionManager::fill_value_absolute, empty)));
	actionMap.insert(std::make_pair("FILL_RANDOMIZE_ABSOLUTE", std::make_pair(&MidiActionManager::fill_randomize_absolute, empty)));

	for(int i = 0; i < MAX_FX; ++i) {
		targeted_element effect = {i,0};
		std::ostringstream toChar;
		toChar << (i+1);
		std::string keyAbsolute("EFFECT");
		std::string keyRelative("EFFECT");
		keyAbsolute += toChar.str();
		keyRelative += toChar.str();
		keyAbsolute += "_LEVEL_ABSOLUTE";
		keyRelative += "_LEVEL_RELATIVE";
		actionMap.insert(std::make_pair(keyAbsolute, std::make_pair(&MidiActionManager::effect_level_absolute, effect)));
		actionMap.insert(std::make_pair(keyRelative, std::make_pair(&MidiActionManager::effect_level_relative, effect)));
	}
	for(int i = 0; i < MAX_COMPONENTS; ++i) {
		std::ostringstream componentToChar;
		componentToChar << (i+1);
		for(int j = 0; j < InstrumentComponent::getMaxLayers(); ++j ) {
			targeted_element sample = {i,j};
			std::ostringstream toChar;
			toChar << (j+1);
			std::string keyGain("GAIN_C");
			std::string keyPitch("PITCH_C");
			keyGain += componentToChar.str();
			keyPitch += componentToChar.str();
			keyGain += "_L";
			keyPitch += "_L";
			keyGain += toChar.str();
			keyPitch += toChar.str();
			keyGain += "_LEVEL_ABSOLUTE";
			keyPitch += "_LEVEL_ABSOLUTE";
			actionMap.insert(std::make_pair(keyGain, std::make_pair(&MidiActionManager::gain_level_absolute, sample)));
			actionMap.insert(std::make_pair(keyPitch, std::make_pair(&MidiActionManager::pitch_level_absolute, sample)));
		}
	}
	actionMap.insert(std::make_pair("SELECT_NEXT_PATTERN", std::make_pair(&MidiActionManager::select_next_pattern, empty)));
	actionMap.insert(std::make_pair("SELECT_ONLY_NEXT_PATTERN", std::make_pair(&MidiActionManager::select_only_next_pattern, empty)));
	actionMap.insert(std::make_pair("SELECT_NEXT_PATTERN_CC_ABSOLUTE", std::make_pair(&MidiActionManager::select_next_pattern_cc_absolute, empty)));
	actionMap.insert(std::make_pair("SELECT_NEXT_PATTERN_RELATIVE", std::make_pair(&MidiActionManager::select_next_pattern_relative, empty)));
	actionMap.insert(std::make_pair("SELECT_AND_PLAY_PATTERN", std::make_pair(&MidiActionManager::select_and_play_pattern, empty)));
	actionMap.insert(std::make_pair("PAN_RELATIVE", std::make_pair(&MidiActionManager::pan_relative, empty)));
	actionMap.insert(std::make_pair("PAN_ABSOLUTE", std::make_pair(&MidiActionManager::pan_absolute, empty)));
	actionMap.insert(std::make_pair("FILTER_CUTOFF_LEVEL_ABSOLUTE", std::make_pair(&MidiActionManager::filter_cutoff_level_absolute, empty)));
	actionMap.insert(std::make_pair("BEATCOUNTER", std::make_pair(&MidiActionManager::beatcounter, empty)));
	actionMap.insert(std::make_pair("TAP_TEMPO", std::make_pair(&MidiActionManager::tap_tempo, empty)));
	actionMap.insert(std::make_pair("PLAYLIST_SONG", std::make_pair(&MidiActionManager::playlist_song, empty)));
	actionMap.insert(std::make_pair("PLAYLIST_NEXT_SONG", std::make_pair(&MidiActionManager::playlist_next_song, empty)));
	actionMap.insert(std::make_pair("PLAYLIST_PREV_SONG", std::make_pair(&MidiActionManager::playlist_previous_song, empty)));
	actionMap.insert(std::make_pair("TOGGLE_METRONOME", std::make_pair(&MidiActionManager::toggle_metronome, empty)));
	actionMap.insert(std::make_pair("SELECT_INSTRUMENT", std::make_pair(&MidiActionManager::select_instrument, empty)));
	actionMap.insert(std::make_pair("UNDO_ACTION", std::make_pair(&MidiActionManager::undo_action, empty)));
	actionMap.insert(std::make_pair("REDO_ACTION", std::make_pair(&MidiActionManager::redo_action, empty)));
	/*
	  the actionList holds all Action identfiers which hydrogen is able to interpret.
	*/
	actionList <<"";
	for(std::map<std::string, std::pair<action_f, targeted_element> >::const_iterator actionIterator = actionMap.begin();
	    actionIterator != actionMap.end();
	    ++actionIterator) {
		actionList << actionIterator->first.c_str();
	}

	eventList << ""
			  << "MMC_PLAY"
			  << "MMC_DEFERRED_PLAY"
			  << "MMC_STOP"
			  << "MMC_FAST_FORWARD"
			  << "MMC_REWIND"
			  << "MMC_RECORD_STROBE"
			  << "MMC_RECORD_EXIT"
			  << "MMC_RECORD_READY"
			  << "MMC_PAUSE"
			  << "NOTE"
			  << "CC"
			  << "PROGRAM_CHANGE";
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

bool MidiActionManager::play(Action * , Hydrogen* pHydrogen, targeted_element ) {
	int nState = pHydrogen->getState();
	if ( nState == STATE_READY ) {
		pHydrogen->sequencer_play();
	}
	return true;
}

bool MidiActionManager::pause(Action * , Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->sequencer_stop();
	return true;
}

bool MidiActionManager::stop(Action * , Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->sequencer_stop();
	pHydrogen->setPatternPos( 0 );
	pHydrogen->setTimelineBpm();
	return true;
}

bool MidiActionManager::play_stop_pause_toggle(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	QString sActionString = pAction->getType();
	int nState = pHydrogen->getState();
	switch ( nState )
	{
	case STATE_READY:
		pHydrogen->sequencer_play();
		break;

	case STATE_PLAYING:
		if( sActionString == "PLAY/STOP_TOGGLE" ) {
			pHydrogen->setPatternPos( 0 );
		}
		pHydrogen->sequencer_stop();
		pHydrogen->setTimelineBpm();
		break;

	default:
		ERRORLOG( "[Hydrogen::ActionManager(PLAY): Unhandled case" );
		break;
	}

	return true;
}

//mutes the master, not a single strip
bool MidiActionManager::mute(Action * , Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->getCoreActionController()->setMasterIsMuted( true );
	return true;
}

bool MidiActionManager::unmute(Action * , Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->getCoreActionController()->setMasterIsMuted( false );
	return true;
}

bool MidiActionManager::mute_toggle(Action * , Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->getCoreActionController()->setMasterIsMuted( !pHydrogen->getSong()->getIsMuted() );
	return true;
}

bool MidiActionManager::strip_mute_toggle(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	
	bool ok;
	bool bSucccess = true;
	
	int nLine = pAction->getParameter1().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if ( pInstrList->is_valid_index( nLine ) ) {
		auto pInstr = pInstrList->get( nLine );
		
		if ( pInstr ) {
			pHydrogen->getCoreActionController()->setStripIsMuted( nLine, !pInstr->is_muted() );
		} else {
			bSucccess = false;
		}
	} else {
		bSucccess = false;
	}

	return bSucccess;
}

bool MidiActionManager::strip_solo_toggle(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	
	bool ok;
	bool bSucccess = true;
	
	int nLine = pAction->getParameter1().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	if ( pInstrList->is_valid_index( nLine ) ) {
		auto pInstr = pInstrList->get( nLine );
		
		if ( pInstr ) {
			pHydrogen->getCoreActionController()->setStripIsSoloed( nLine, !pInstr->is_soloed() );
		} else {
			bSucccess = false;
		}
	} else {
		bSucccess = false;
	}
	
	return bSucccess;
}

bool MidiActionManager::beatcounter(Action * , Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->handleBeatCounter();
	return true;
}

bool MidiActionManager::tap_tempo(Action * , Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->onTapTempoAccelEvent();
	return true;
}

bool MidiActionManager::select_next_pattern(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	bool ok;
	int row = pAction->getParameter1().toInt(&ok,10);
	if( row > pHydrogen->getSong()->getPatternList()->size() - 1 ||
		row < 0 ) {
		return false;
	}
	if(Preferences::get_instance()->patternModePlaysSelected()) {
		pHydrogen->setSelectedPatternNumber( row );
	}
	else {
		pHydrogen->sequencer_setNextPattern( row );
	}
	return true;
}

bool MidiActionManager::select_only_next_pattern(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	bool ok;
	int row = pAction->getParameter1().toInt(&ok,10);
	if( row > pHydrogen->getSong()->getPatternList()->size() -1 ||
		row < 0 ) {
		return false;
	}
	if(Preferences::get_instance()->patternModePlaysSelected())
	{
		return true;
	}
	
	pHydrogen->sequencer_setOnlyNextPattern( row );
	return true; 
}

bool MidiActionManager::select_next_pattern_relative(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	bool ok;
	if(!Preferences::get_instance()->patternModePlaysSelected()) {
		return true;
	}
	int row = pHydrogen->getSelectedPatternNumber() + pAction->getParameter1().toInt(&ok,10);
	if( row > pHydrogen->getSong()->getPatternList()->size() - 1 ||
		row < 0 ) {
		return false;
	}
	
	pHydrogen->setSelectedPatternNumber( row );
	return true;
}

bool MidiActionManager::select_next_pattern_cc_absolute(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	bool ok;
	int row = pAction->getParameter2().toInt(&ok,10);
	
	if( row > pHydrogen->getSong()->getPatternList()->size() - 1 ||
		row < 0 ) {
		return false;
	}
	
	if(Preferences::get_instance()->patternModePlaysSelected()) {
		pHydrogen->setSelectedPatternNumber( row );
	}
	else {
		return true;// only usefully in normal pattern mode
	}
	
	return true;
}

bool MidiActionManager::select_and_play_pattern(Action * pAction, Hydrogen* pHydrogen, targeted_element t ) {
	if ( ! select_next_pattern( pAction, pHydrogen, t ) ) {
		return false;
	}

	int nState = pHydrogen->getState();
	if ( nState == STATE_READY ) {
		pHydrogen->sequencer_play();
	}

	return true;
}

bool MidiActionManager::select_instrument(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	bool ok;
	int  nInstrumentNumber = pAction->getParameter2().toInt(&ok,10) ;
	

	if ( pHydrogen->getSong()->getInstrumentList()->size() < nInstrumentNumber ) {
		nInstrumentNumber = pHydrogen->getSong()->getInstrumentList()->size() -1;
	} else if ( nInstrumentNumber < 0 ) {
		nInstrumentNumber = 0;
	}
	
	pHydrogen->setSelectedInstrumentNumber( nInstrumentNumber );
	return true;
}

bool MidiActionManager::effect_level_absolute(Action * pAction, Hydrogen* pHydrogen, targeted_element nEffect) {
	bool ok;
	bool bSuccess = true;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int fx_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if ( pInstrList->is_valid_index( nLine) )
	{
		auto pInstr = pInstrList->get( nLine );
		
		if ( pInstr ) {
			if( fx_param != 0 ) {
				pInstr->set_fx_level(  ( (float) (fx_param / 127.0 ) ), nEffect._id );
			} else {
				pInstr->set_fx_level( 0 , nEffect._id );
			}
			
			pHydrogen->setSelectedInstrumentNumber( nLine );			
		} else {
			bSuccess = false;
		}
	
	}

	return bSuccess;
}

bool MidiActionManager::effect_level_relative(Action * , Hydrogen* , targeted_element ) {
	//empty ?
	return true;
}

//sets the volume of a master output to a given level (percentage)
bool MidiActionManager::master_volume_absolute(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {

	bool ok;
	int vol_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> song = pHydrogen->getSong();

	if( vol_param != 0 ){
		song->setVolume( 1.5* ( (float) (vol_param / 127.0 ) ));
	} else {
		song->setVolume( 0 );
	}

	return true;
}

//increments/decrements the volume of the whole song
bool MidiActionManager::master_volume_relative(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {

	bool ok;
	int vol_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> song = pHydrogen->getSong();

	if( vol_param != 0 ) {
		if ( vol_param == 1 && song->getVolume() < 1.5 ) {
			song->setVolume( song->getVolume() + 0.05 );
		} else {
			if( song->getVolume() >= 0.0 ) {
				song->setVolume( song->getVolume() - 0.05 );
			}
		}
	} else {
		song->setVolume( 0 );
	}

	return true;
}

//sets the volume of a mixer strip to a given level (percentage)
bool MidiActionManager::strip_volume_absolute(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int vol_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if ( pInstrList->is_valid_index( nLine) )
	{
		auto pInstr = pInstrList->get( nLine );
	
		if ( pInstr == nullptr) {
			return false;
		}
	
		if( vol_param != 0 ) {
			pInstr->set_volume( 1.5* ( (float) (vol_param / 127.0 ) ));
		} else {
			pInstr->set_volume( 0 );
		}
	
		pHydrogen->setSelectedInstrumentNumber(nLine);
	}

	return true;
}

//increments/decrements the volume of one mixer strip
bool MidiActionManager::strip_volume_relative(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int vol_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	if ( pInstrList->is_valid_index( nLine) )
	{
		auto pInstr = pInstrList->get( nLine );
	
		if ( pInstr == nullptr) {
			return false;
		}
	
		if( vol_param != 0 ) {
			if ( vol_param == 1 && pInstr->get_volume() < 1.5 ) {
				pInstr->set_volume( pInstr->get_volume() + 0.1 );
			} else {
				if( pInstr->get_volume() >= 0.0 ){
					pInstr->set_volume( pInstr->get_volume() - 0.1 );
				}
			}
		} else {
			pInstr->set_volume( 0 );
		}
	
		pHydrogen->setSelectedInstrumentNumber(nLine);
	}

	return true;
}

// sets the absolute panning of a given mixer channel
bool MidiActionManager::pan_absolute(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if( pInstrList->is_valid_index( nLine ) ) {
		pHydrogen->setSelectedInstrumentNumber( nLine );
	
		auto pInstr = pInstrList->get( nLine );
	
		if( pInstr == nullptr ) {
			return false;
		}

		pInstr->setPanWithRangeFrom0To1( (float) pan_param / 127.f );
	
		pHydrogen->setSelectedInstrumentNumber(nLine);
	}

	return true;
}

// changes the panning of a given mixer channel
// this is useful if the panning is set by a rotary control knob
bool MidiActionManager::pan_relative(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if( pInstrList->is_valid_index( nLine ) ) {	
		pHydrogen->setSelectedInstrumentNumber( nLine );

		auto pInstr = pInstrList->get( nLine );

		if( pInstr == nullptr ) {
			return false;
		}
	
		float fPan = pInstr->getPan();

		if( pan_param == 1 && fPan < 1.f ) {
			pInstr->setPan( fPan + 0.1 );
		} else if( pan_param != 1 && fPan > -1.f ) {
			pInstr->setPan( fPan - 0.1 );
		}

		pHydrogen->setSelectedInstrumentNumber(nLine);
	}

	return true;
}

bool MidiActionManager::gain_level_absolute(Action * pAction, Hydrogen* pHydrogen, targeted_element nSample) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int gain_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	
	if( pInstrList->is_valid_index( nLine ) )
	{
		auto pInstr = pInstrList->get( nLine );
		if( pInstr == nullptr ) {
			return false;
		}
	
		auto pComponent =  pInstr->get_component( nSample._id );
		if( pComponent == nullptr) {
			return false;
		}
	
		auto pLayer = pComponent->get_layer( nSample._subId );
		if( pLayer == nullptr ) {
			return false;
		}
	
		if( gain_param != 0 ) {
			pLayer->set_gain( 5.0* ( (float) (gain_param / 127.0 ) ) );
		} else {
			pLayer->set_gain( 0 );
		}
	
		pHydrogen->setSelectedInstrumentNumber( nLine );
	
		pHydrogen->refreshInstrumentParameters( nLine );
	}
	
	return true;
}

bool MidiActionManager::pitch_level_absolute(Action * pAction, Hydrogen* pHydrogen, targeted_element nSample) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pitch_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	if( pInstrList->is_valid_index( nLine ) )
	{
		auto pInstr = pInstrList->get( nLine );
		if( pInstr == nullptr ) {
			return false;
		}
	
		auto pComponent =  pInstr->get_component( nSample._id );
		if( pComponent == nullptr) {
			return false;
		}
	
		auto pLayer = pComponent->get_layer( nSample._subId );
		if( pLayer == nullptr ) {
			return false;
		}
	
		if( pitch_param != 0 ){
			pLayer->set_pitch( 49* ( (float) (pitch_param / 127.0 ) ) -24.5 );
		} else {
			pLayer->set_pitch( -24.5 );
		}
	
		pHydrogen->setSelectedInstrumentNumber( nLine );
	
		pHydrogen->refreshInstrumentParameters( nLine );
	}

	return true;
}

bool MidiActionManager::filter_cutoff_level_absolute(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int filter_cutoff_param = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	if( pInstrList->is_valid_index( nLine ) )
	{
		auto pInstr = pInstrList->get( nLine );
		if( pInstr == nullptr ) {
			return false;
		}
	
		pInstr->set_filter_active( true );
		if( filter_cutoff_param != 0 ) {
			pInstr->set_filter_cutoff( ( (float) (filter_cutoff_param / 127.0 ) ) );
		} else {
			pInstr->set_filter_cutoff( 0 );
		}
	
		pHydrogen->setSelectedInstrumentNumber( nLine );
	
		pHydrogen->refreshInstrumentParameters( nLine );
	}
	
	return true;
}


/*
 * increments/decrements the BPM
 * this is useful if the bpm is set by a rotary control knob
 */
bool MidiActionManager::bpm_cc_relative(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	//this Action should be triggered only by CC commands

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getParameter2().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	if ( m_nLastBpmChangeCCParameter >= cc_param && pSong->getBpm()  < 300) {
		pHydrogen->setBPM( pSong->getBpm() - 1*mult );
	}

	if ( m_nLastBpmChangeCCParameter < cc_param && pSong->getBpm()  > 40 ) {
		pHydrogen->setBPM( pSong->getBpm() + 1*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;

	pHydrogen->getAudioEngine()->unlock();

	return true;
}

/*
 * increments/decrements the BPM
 * this is useful if the bpm is set by a rotary control knob
 */
bool MidiActionManager::bpm_fine_cc_relative(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	//this Action should be triggered only by CC commands
	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getParameter2().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	if ( m_nLastBpmChangeCCParameter >= cc_param && pSong->getBpm()  < 300) {
		pHydrogen->setBPM( pSong->getBpm() - 0.01*mult );
	}

	if ( m_nLastBpmChangeCCParameter < cc_param && pSong->getBpm()  > 40 ) {
		pHydrogen->setBPM( pSong->getBpm() + 0.01*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;

	pHydrogen->getAudioEngine()->unlock();

	return true;
}

bool MidiActionManager::humanize_velocity_absolute(Action * pAction, Hydrogen* pHydrogen , targeted_element ) {
	bool ok;
	int value = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	pSong->setHumanizeVelocityValue( value / 127.0 );

	return true;
}

bool MidiActionManager::humanize_time_absolute(Action *pAction , Hydrogen* pHydrogen , targeted_element ) {
	bool ok;
	int value = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	pSong->setHumanizeTimeValue( value / 127.0 );

	return true;
}

bool MidiActionManager::swing_absolute(Action *pAction , Hydrogen* pHydrogen , targeted_element ) {
	bool ok;
	int value = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	pSong->setSwingFactor( value / 127.0 );

	return true;
}

bool MidiActionManager::fill_value_absolute(Action *pAction , Hydrogen* pHydrogen , targeted_element ) {
	bool ok;
	int value = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	pSong->setFillValue( value / 127.0 );

	return true;
}

bool MidiActionManager::fill_randomize_absolute(Action *pAction , Hydrogen* pHydrogen , targeted_element ) {
	bool ok;
	int value = pAction->getParameter2().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	pSong->setFillRandomize( value / 127.0 );

	return true;
}

bool MidiActionManager::bpm_increase(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	pHydrogen->setBPM( pSong->getBpm() + 1*mult );

	pHydrogen->getAudioEngine()->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );

	return true;
}

bool MidiActionManager::bpm_decrease(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	pHydrogen->setBPM( pSong->getBpm() - 1*mult );
	
	pHydrogen->getAudioEngine()->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );

	return true;
}

bool MidiActionManager::next_bar(Action * , Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->setPatternPos(pHydrogen->getPatternPos() +1 );
	pHydrogen->setTimelineBpm();
	return true;
}


bool MidiActionManager::previous_bar(Action * , Hydrogen* pHydrogen, targeted_element ) {
	pHydrogen->setPatternPos(pHydrogen->getPatternPos() -1 );
	pHydrogen->setTimelineBpm();
	return true;
}

bool setSong( int songnumber, Hydrogen * pHydrogen ) {
	int asn = Playlist::get_instance()->getActiveSongNumber();
	if(asn != songnumber && songnumber >= 0 && songnumber <= Playlist::get_instance()->size() - 1 ) {
		Playlist::get_instance()->setNextSongByNumber( songnumber );
	}
	return true;
}

bool MidiActionManager::playlist_song(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	bool ok;
	int songnumber = pAction->getParameter1().toInt(&ok,10);
	return setSong( songnumber, pHydrogen );
}

bool MidiActionManager::playlist_next_song(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	int songnumber = Playlist::get_instance()->getActiveSongNumber();
	return setSong( ++songnumber, pHydrogen );
}

bool MidiActionManager::playlist_previous_song(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	int songnumber = Playlist::get_instance()->getActiveSongNumber();
	return setSong( --songnumber, pHydrogen );
}

bool MidiActionManager::record_ready(Action * pAction, Hydrogen* pHydrogen, targeted_element ) {
	if ( pHydrogen->getState() != STATE_PLAYING ) {
		if (!Preferences::get_instance()->getRecordEvents()) {
			Preferences::get_instance()->setRecordEvents(true);
		}
		else {
			Preferences::get_instance()->setRecordEvents(false);
		}
	}
	return true;
}

bool MidiActionManager::record_strobe_toggle(Action * , Hydrogen* , targeted_element ) {
	if (!Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(true);
	}
	else {
		Preferences::get_instance()->setRecordEvents(false);
	}
	return true;
}

bool MidiActionManager::record_strobe(Action * , Hydrogen* , targeted_element ) {
	if (!Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(true);
	}
	return true;
}

bool MidiActionManager::record_exit(Action * , Hydrogen* , targeted_element ) {
	if (Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(false);
	}
	return true;
}

bool MidiActionManager::toggle_metronome(Action * , Hydrogen* , targeted_element ) {
	Preferences::get_instance()->m_bUseMetronome = !Preferences::get_instance()->m_bUseMetronome;
	return true;
}

bool MidiActionManager::undo_action(Action * , Hydrogen* , targeted_element ) {
	EventQueue::get_instance()->push_event( EVENT_UNDO_REDO, 0);// 0 = undo
	return true;
}

bool MidiActionManager::redo_action(Action * , Hydrogen* , targeted_element ) {
	EventQueue::get_instance()->push_event( EVENT_UNDO_REDO, 1);// 1 = redo
	return true;
}

bool MidiActionManager::handleAction( Action * pAction ) {

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	/*
		return false if action is null
		(for example if no Action exists for an event)
	*/
	if( pAction == nullptr ) {
		return false;
	}

	QString sActionString = pAction->getType();

	std::map<std::string, std::pair<action_f, targeted_element> >::const_iterator foundAction = actionMap.find(sActionString.toStdString());
	if( foundAction != actionMap.end() ) {
		action_f action = foundAction->second.first;
		targeted_element nElement = foundAction->second.second;
		return (this->*action)(pAction, pHydrogen, nElement);
	}

	return false;
}
