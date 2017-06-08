/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#include <QObject>

#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/playlist.h>

#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/pattern_list.h>

#include <hydrogen/Preferences.h>
#include <hydrogen/midi_action.h>

#include <sstream>

using namespace H2Core;

/**
* @class Action
*
* @brief This class represents a midi action.
*
* This class represents actions which can be executed
* after a midi event occured. An example is the "MUTE"
* action, which mutes the outputs of hydrogen.
*
* An action can be linked to an event. If this event occurs,
* the action gets triggered. The handling of events takes place
* in midi_input.cpp .
*
* Each action has two independ parameters. The two parameters are optional and
* can be used to carry additional information, which mean
* only something to this very Action. They can have totally different meanings for other Actions.
* Example: parameter1 is the Mixer strip and parameter 2 a multiplier for the volume change on this strip
*
* @author Sebastian Moors
*
*/

const char* Action::__class_name = "MidiAction";

Action::Action( QString typeString ) : Object( __class_name ) {
	type = typeString;
	QString parameter1 = "0";
	QString parameter2 = "0";
	QString parameter3 = "0";
	QString value = "0";
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
MidiActionManager* MidiActionManager::__instance = NULL;
const char* MidiActionManager::__class_name = "ActionManager";

MidiActionManager::MidiActionManager() : Object( __class_name ) {
	__instance = this;

	m_nLastBpmChangeCCParameter = -1;
	/*
		the actionMap holds all Action identifiers which hydrogen is able to interpret.
		it holds pointer to member function
	*/
	actionMap.insert(make_pair("PLAY", &MidiActionManager::play));
	actionMap.insert(make_pair("PLAY/STOP_TOGGLE", &MidiActionManager::play_stop_pause_toggle));
	actionMap.insert(make_pair("PLAY/PAUSE_TOGGLE", &MidiActionManager::play_stop_pause_toggle));
	actionMap.insert(make_pair("STOP", &MidiActionManager::stop));
	actionMap.insert(make_pair("PAUSE", &MidiActionManager::pause));
	actionMap.insert(make_pair("RECORD_READY", &MidiActionManager::record_ready));
	actionMap.insert(make_pair("RECORD/STROBE_TOGGLE", &MidiActionManager::record_strobe_toggle));
	actionMap.insert(make_pair("RECORD_STROBE", &MidiActionManager::record_strobe));
	actionMap.insert(make_pair("RECORD_EXIT", &MidiActionManager::record_exit));
	actionMap.insert(make_pair("MUTE", &MidiActionManager::mute));
	actionMap.insert(make_pair("UNMUTE", &MidiActionManager::unmute));
	actionMap.insert(make_pair("MUTE_TOGGLE", &MidiActionManager::mute_toggle));
	actionMap.insert(make_pair(">>_NEXT_BAR", &MidiActionManager::next_bar));
	actionMap.insert(make_pair("<<_PREVIOUS_BAR", &MidiActionManager::previous_bar));
	actionMap.insert(make_pair("BPM_INCR", &MidiActionManager::bpm_increase));
	actionMap.insert(make_pair("BPM_DECR", &MidiActionManager::bpm_decrease));
	actionMap.insert(make_pair("BPM_CC_RELATIVE", &MidiActionManager::bpm_cc_relative));
	actionMap.insert(make_pair("BPM_FINE_CC_RELATIVE", &MidiActionManager::bpm_fine_cc_relative));
	actionMap.insert(make_pair("MASTER_VOLUME_RELATIVE", &MidiActionManager::master_volume_relative));
	actionMap.insert(make_pair("MASTER_VOLUME_ABSOLUTE", &MidiActionManager::master_volume_absolute));
	actionMap.insert(make_pair("STRIP_VOLUME_RELATIVE", &MidiActionManager::strip_volume_relative));
	actionMap.insert(make_pair("STRIP_VOLUME_ABSOLUTE", &MidiActionManager::strip_volume_absolute));
	actionMap.insert(make_pair("EFFECT_LEVEL_ABSOLUTE", &MidiActionManager::effect_level_absolute));
	actionMap.insert(make_pair("EFFECT_LEVEL_RELATIVE", &MidiActionManager::effect_level_relative));
	actionMap.insert(make_pair("GAIN_LEVEL_ABSOLUTE", &MidiActionManager::gain_level_absolute));
	actionMap.insert(make_pair("PITCH_LEVEL_ABSOLUTE", &MidiActionManager::pitch_level_absolute));
	actionMap.insert(make_pair("SELECT_NEXT_PATTERN", &MidiActionManager::select_next_pattern));
	actionMap.insert(make_pair("SELECT_NEXT_PATTERN_CC_ABSOLUTE", &MidiActionManager::select_next_pattern_cc_absolute));
	actionMap.insert(make_pair("SELECT_NEXT_PATTERN_PROMPTLY", &MidiActionManager::select_next_pattern_promptly));
	actionMap.insert(make_pair("SELECT_NEXT_PATTERN_RELATIVE", &MidiActionManager::select_next_pattern_relative));
	actionMap.insert(make_pair("SELECT_AND_PLAY_PATTERN", &MidiActionManager::select_and_play_pattern));
	actionMap.insert(make_pair("PAN_RELATIVE", &MidiActionManager::pan_relative));
	actionMap.insert(make_pair("PAN_ABSOLUTE", &MidiActionManager::pan_absolute));
	actionMap.insert(make_pair("FILTER_CUTOFF_LEVEL_ABSOLUTE", &MidiActionManager::filter_cutoff_level_absolute));
	actionMap.insert(make_pair("BEATCOUNTER", &MidiActionManager::beatcounter));
	actionMap.insert(make_pair("TAP_TEMPO", &MidiActionManager::tap_tempo));
	actionMap.insert(make_pair("PLAYLIST_SONG", &MidiActionManager::playlist_song));
	actionMap.insert(make_pair("PLAYLIST_NEXT_SONG", &MidiActionManager::playlist_next_song));
	actionMap.insert(make_pair("PLAYLIST_PREV_SONG", &MidiActionManager::playlist_previous_song));
	actionMap.insert(make_pair("TOGGLE_METRONOME", &MidiActionManager::toggle_metronome));
	actionMap.insert(make_pair("SELECT_INSTRUMENT", &MidiActionManager::select_instrument));
	actionMap.insert(make_pair("UNDO_ACTION", &MidiActionManager::undo_action));
	actionMap.insert(make_pair("REDO_ACTION", &MidiActionManager::redo_action));

	/*
		the actionList holds all Action identfiers which hydrogen is able to interpret.
	*/
	actionList <<"";
	for(map<string, action_f >::const_iterator actionIterator = actionMap.begin();
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
	__instance = NULL;
}

void MidiActionManager::create_instance() {
	if ( __instance == 0 ) {
		__instance = new MidiActionManager;
	}
}

bool MidiActionManager::play(Action * , Hydrogen* pEngine ) {
	int nState = pEngine->getState();
	if ( nState == STATE_READY ) {
		pEngine->sequencer_play();
	}
	return true;
}

bool MidiActionManager::pause(Action * , Hydrogen* pEngine ) {
	pEngine->sequencer_stop();
	return true;
}

bool MidiActionManager::stop(Action * , Hydrogen* pEngine ) {
	pEngine->sequencer_stop();
	pEngine->setPatternPos( 0 );
	pEngine->setTimelineBpm();
	return true;
}

bool MidiActionManager::play_stop_pause_toggle(Action * pAction, Hydrogen* pEngine ) {
	QString sActionString = pAction->getType();
	int nState = pEngine->getState();
	switch ( nState )
	{
	case STATE_READY:
		pEngine->sequencer_play();
		break;

	case STATE_PLAYING:
		if( sActionString == "PLAY/STOP_TOGGLE" ) {
			pEngine->setPatternPos( 0 );
		}
		pEngine->sequencer_stop();
		pEngine->setTimelineBpm();
		break;

	default:
		ERRORLOG( "[Hydrogen::ActionManager(PLAY): Unhandled case" );
		break;
	}

	return true;
}

bool MidiActionManager::mute(Action * , Hydrogen* pEngine ) {
	//mutes the master, not a single strip
	pEngine->getCoreActionController()->setMasterIsMuted( true );
	return true;
}

bool MidiActionManager::unmute(Action * , Hydrogen* pEngine ) {
	pEngine->getCoreActionController()->setMasterIsMuted( false );
	return true;
}

bool MidiActionManager::mute_toggle(Action * , Hydrogen* pEngine ) {
	pEngine->getCoreActionController()->setMasterIsMuted( !Hydrogen::get_instance()->getSong()->__is_muted );
	return true;
}

bool MidiActionManager::beatcounter(Action * , Hydrogen* pEngine ) {
	pEngine->handleBeatCounter();
	return true;
}

bool MidiActionManager::tap_tempo(Action * , Hydrogen* pEngine ) {
	pEngine->onTapTempoAccelEvent();
	return true;
}

bool MidiActionManager::select_next_pattern(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	int row = pAction->getParameter1().toInt(&ok,10);
	if( row > pEngine->getSong()->get_pattern_list()->size() -1 ) {
		return false;
	}
	if(Preferences::get_instance()->patternModePlaysSelected()) {
		pEngine->setSelectedPatternNumber( row );
	}
	else {
		pEngine->sequencer_setNextPattern( row );
	}
	return true;
}

bool MidiActionManager::select_next_pattern_relative(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	if(!Preferences::get_instance()->patternModePlaysSelected()) {
		return true;
	}
	int row = pEngine->getSelectedPatternNumber() + pAction->getParameter1().toInt(&ok,10);
	if( row > pEngine->getSong()->get_pattern_list()->size() -1 ) {
		return false;
	}

	pEngine->setSelectedPatternNumber( row );
	return true;
}

bool MidiActionManager::select_next_pattern_cc_absolute(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	int row = pAction->getValue().toInt(&ok,10);
	if( row > pEngine->getSong()->get_pattern_list()->size() -1 ) {
		return false;
	}
	if(Preferences::get_instance()->patternModePlaysSelected()) {
		pEngine->setSelectedPatternNumber( row );
	}
	else {
		return true;// only usefully in normal pattern mode
	}
	return true;
}

bool MidiActionManager::select_next_pattern_promptly(Action * pAction, Hydrogen* pEngine ) {
	// obsolete, use SELECT_NEXT_PATTERN_CC_ABSOLUT instead
	bool ok;
	int row = pAction->getValue().toInt(&ok,10);
	pEngine->setSelectedPatternNumberWithoutGuiEvent( row );
	return true;
}

bool MidiActionManager::select_and_play_pattern(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	int row = pAction->getParameter1().toInt(&ok,10);
	pEngine->setSelectedPatternNumber( row );
	pEngine->sequencer_setNextPattern( row );

	int nState = pEngine->getState();
	if ( nState == STATE_READY ) {
		pEngine->sequencer_play();
	}

	return true;
}

bool MidiActionManager::select_instrument(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	int  instrument_number = pAction->getValue().toInt(&ok,10) ;
	if ( pEngine->getSong()->get_instrument_list()->size() < instrument_number ) {
		instrument_number = pEngine->getSong()->get_instrument_list()->size() -1;
	}
	pEngine->setSelectedInstrumentNumber( instrument_number );
	return true;
}

bool MidiActionManager::effect_level_absolute(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int fx_param = pAction->getValue().toInt(&ok,10);
	int fx_id = pAction->getParameter2().toInt(&ok,10);

	pEngine->setSelectedInstrumentNumber( nLine );

	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();
	Instrument *instr = instrList->get( nLine );
	if ( instr == NULL)  {
		return false;
	}

	if( fx_param != 0 ) {
		instr->set_fx_level(  ( (float) (fx_param / 127.0 ) ), fx_id );
	} else {
		instr->set_fx_level( 0 , fx_id );
	}

	pEngine->setSelectedInstrumentNumber(nLine);

	return true;
}

bool MidiActionManager::effect_level_relative(Action * , Hydrogen*  ) {
	//empty ?
	return true;
}

bool MidiActionManager::master_volume_absolute(Action * pAction, Hydrogen* pEngine ) {
	//sets the volume of a master output to a given level (percentage)

	bool ok;
	int vol_param = pAction->getValue().toInt(&ok,10);

	Song *song = pEngine->getSong();

	if( vol_param != 0 ){
		song->set_volume( 1.5* ( (float) (vol_param / 127.0 ) ));
	} else {
		song->set_volume( 0 );
	}

	return true;
}

bool MidiActionManager::master_volume_relative(Action * pAction, Hydrogen* pEngine ) {
	//increments/decrements the volume of the whole song

	bool ok;
	int vol_param = pAction->getValue().toInt(&ok,10);

	Song *song = pEngine->getSong();

	if( vol_param != 0 ) {
		if ( vol_param == 1 && song->get_volume() < 1.5 ) {
			song->set_volume( song->get_volume() + 0.05 );
		} else {
			if( song->get_volume() >= 0.0 ) {
				song->set_volume( song->get_volume() - 0.05 );
			}
		}
	} else {
		song->set_volume( 0 );
	}

	return true;
}

bool MidiActionManager::strip_volume_absolute(Action * pAction, Hydrogen* pEngine ) {
	//sets the volume of a mixer strip to a given level (percentage)

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int vol_param = pAction->getValue().toInt(&ok,10);

	pEngine->setSelectedInstrumentNumber( nLine );

	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get( nLine );

	if ( instr == NULL) {
		return false;
	}

	if( vol_param != 0 ) {
		instr->set_volume( 1.5* ( (float) (vol_param / 127.0 ) ));
	} else {
		instr->set_volume( 0 );
	}

	pEngine->setSelectedInstrumentNumber(nLine);

	return true;
}

bool MidiActionManager::strip_volume_relative(Action * pAction, Hydrogen* pEngine ) {
	//increments/decrements the volume of one mixer strip

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int vol_param = pAction->getValue().toInt(&ok,10);

	pEngine->setSelectedInstrumentNumber( nLine );

	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get( nLine );

	if ( instr == NULL) {
		return false;
	}

	if( vol_param != 0 ) {
		if ( vol_param == 1 && instr->get_volume() < 1.5 ) {
			instr->set_volume( instr->get_volume() + 0.1 );
		} else {
			if( instr->get_volume() >= 0.0 ){
				instr->set_volume( instr->get_volume() - 0.1 );
			}
		}
	} else {
		instr->set_volume( 0 );
	}

	pEngine->setSelectedInstrumentNumber(nLine);

	return true;
}

bool MidiActionManager::pan_absolute(Action * pAction, Hydrogen* pEngine ) {
	// sets the absolute panning of a given mixer channel

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);


	float pan_L;
	float pan_R;

	pEngine->setSelectedInstrumentNumber( nLine );
	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get( nLine );

	if( instr == NULL ) {
		return false;
	}

	pan_L = instr->get_pan_l();
	pan_R = instr->get_pan_r();

	// pan
	float fPanValue = 0.0;
	if (pan_R == 1.0) {
		fPanValue = 1.0 - (pan_L / 2.0);
	}
	else {
		fPanValue = pan_R / 2.0;
	}


	fPanValue = 1 * ( ((float) pan_param) / 127.0 );


	if (fPanValue >= 0.5) {
		pan_L = (1.0 - fPanValue) * 2;
		pan_R = 1.0;
	}
	else {
		pan_L = 1.0;
		pan_R = fPanValue * 2;
	}


	instr->set_pan_l( pan_L );
	instr->set_pan_r( pan_R );

	pEngine->setSelectedInstrumentNumber(nLine);

	return true;
}

bool MidiActionManager::pan_relative(Action * pAction, Hydrogen* pEngine ) {
	// changes the panning of a given mixer channel
	// this is useful if the panning is set by a rotary control knob

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	float pan_L;
	float pan_R;

	pEngine->setSelectedInstrumentNumber( nLine );
	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get( nLine );

	if( instr == NULL ) {
		return false;
	}

	pan_L = instr->get_pan_l();
	pan_R = instr->get_pan_r();

	// pan
	float fPanValue = 0.0;
	if (pan_R == 1.0) {
		fPanValue = 1.0 - (pan_L / 2.0);
	}
	else {
		fPanValue = pan_R / 2.0;
	}

	if( pan_param == 1 && fPanValue < 1 ) {
		fPanValue += 0.05;
	}

	if( pan_param != 1 && fPanValue > 0 ) {
		fPanValue -= 0.05;
	}

	if (fPanValue >= 0.5) {
		pan_L = (1.0 - fPanValue) * 2;
		pan_R = 1.0;
	}
	else {
		pan_L = 1.0;
		pan_R = fPanValue * 2;
	}


	instr->set_pan_l( pan_L );
	instr->set_pan_r( pan_R );

	pEngine->setSelectedInstrumentNumber(nLine);

	return true;
}

bool MidiActionManager::gain_level_absolute(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int gain_param = pAction->getValue().toInt(&ok,10);
	int component_id = pAction->getParameter2().toInt(&ok,10);
	int layer_id = pAction->getParameter3().toInt(&ok,10);

	pEngine->setSelectedInstrumentNumber( nLine );

	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get( nLine );
	if( instr == 0 ) {
		return false;
	}

	InstrumentComponent* component =  instr->get_component( component_id );
	if( component == 0) {
		return false;
	}

	InstrumentLayer* layer = component->get_layer( layer_id );
	if( layer == 0 ) {
		return false;
	}

	if( gain_param != 0 ) {
		layer->set_gain( 5.0* ( (float) (gain_param / 127.0 ) ) );
	} else {
		layer->set_gain( 0 );
	}

	pEngine->setSelectedInstrumentNumber( nLine );

	pEngine->refreshInstrumentParameters( nLine );

	return true;
}

bool MidiActionManager::pitch_level_absolute(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pitch_param = pAction->getValue().toInt(&ok,10);
	int component_id = pAction->getParameter2().toInt(&ok,10);
	int layer_id = pAction->getParameter3().toInt(&ok,10);

	pEngine->setSelectedInstrumentNumber( nLine );

	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get( nLine );
	if( instr == 0 ) {
		return false;
	}

	InstrumentComponent* component =  instr->get_component( component_id );
	if( component == 0) {
		return false;
	}

	InstrumentLayer* layer = component->get_layer( layer_id );
	if( layer == 0 ) {
		return false;
	}

	if( pitch_param != 0 ){
		layer->set_pitch( 49* ( (float) (pitch_param / 127.0 ) ) -24.5 );
	} else {
		layer->set_pitch( -24.5 );
	}

	pEngine->setSelectedInstrumentNumber( nLine );

	pEngine->refreshInstrumentParameters( nLine );

	return true;
}

bool MidiActionManager::filter_cutoff_level_absolute(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int filter_cutoff_param = pAction->getValue().toInt(&ok,10);

	pEngine->setSelectedInstrumentNumber( nLine );

	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get( nLine );
	if( instr == 0 ) {
		return false;
	}

	instr->set_filter_active( true );
	if( filter_cutoff_param != 0 ) {
		instr->set_filter_cutoff( ( (float) (filter_cutoff_param / 127.0 ) ) );
	} else {
		instr->set_filter_cutoff( 0 );
	}

	pEngine->setSelectedInstrumentNumber( nLine );

	pEngine->refreshInstrumentParameters( nLine );

	return true;
}

bool MidiActionManager::bpm_cc_relative(Action * pAction, Hydrogen* pEngine ) {
	/*
	 * increments/decrements the BPM
	 * this is useful if the bpm is set by a rotary control knob
	*/

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	//this Action should be triggered only by CC commands

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getValue().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	Song* pSong = pEngine->getSong();

	if ( m_nLastBpmChangeCCParameter >= cc_param && pSong->__bpm  < 300) {
		pEngine->setBPM( pSong->__bpm - 1*mult );
	}

	if ( m_nLastBpmChangeCCParameter < cc_param && pSong->__bpm  > 40 ) {
		pEngine->setBPM( pSong->__bpm + 1*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;

	AudioEngine::get_instance()->unlock();

	return true;
}

bool MidiActionManager::bpm_fine_cc_relative(Action * pAction, Hydrogen* pEngine ) {
	/*
	 * increments/decrements the BPM
	 * this is useful if the bpm is set by a rotary control knob
	*/

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	//this Action should be triggered only by CC commands
	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getValue().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	Song* pSong = pEngine->getSong();

	if ( m_nLastBpmChangeCCParameter >= cc_param && pSong->__bpm  < 300) {
		pEngine->setBPM( pSong->__bpm - 0.01*mult );
	}

	if ( m_nLastBpmChangeCCParameter < cc_param && pSong->__bpm  > 40 ) {
		pEngine->setBPM( pSong->__bpm + 0.01*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;

	AudioEngine::get_instance()->unlock();

	return true;
}

bool MidiActionManager::bpm_increase(Action * pAction, Hydrogen* pEngine ) {
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);

	Song* pSong = pEngine->getSong();
	if (pSong->__bpm  < 300) {
		pEngine->setBPM( pSong->__bpm + 1*mult );
	}
	AudioEngine::get_instance()->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );

	return true;
}

bool MidiActionManager::bpm_decrease(Action * pAction, Hydrogen* pEngine ) {
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);

	Song* pSong = pEngine->getSong();
	if (pSong->__bpm  > 40 ) {
		pEngine->setBPM( pSong->__bpm - 1*mult );
	}
	AudioEngine::get_instance()->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );

	return true;
}

bool MidiActionManager::next_bar(Action * , Hydrogen* pEngine ) {
	pEngine->setPatternPos(pEngine->getPatternPos() +1 );
	pEngine->setTimelineBpm();
	return true;
}

bool MidiActionManager::previous_bar(Action * , Hydrogen* pEngine ) {
	pEngine->setPatternPos(pEngine->getPatternPos() -1 );
	pEngine->setTimelineBpm();
	return true;
}

bool setSong( int songnumber, Hydrogen * pEngine ) {
	int asn = Playlist::get_instance()->getActiveSongNumber();
	if(asn != songnumber && songnumber >= 0 && songnumber <= pEngine->m_PlayList.size()-1) {
		Playlist::get_instance()->setNextSongByNumber( songnumber );
	}
	return true;
}

bool MidiActionManager::playlist_song(Action * pAction, Hydrogen* pEngine ) {
	bool ok;
	int songnumber = pAction->getValue().toInt(&ok,10);
	return setSong( songnumber, pEngine );
}

bool MidiActionManager::playlist_next_song(Action * pAction, Hydrogen* pEngine ) {
	int songnumber = Playlist::get_instance()->getActiveSongNumber();
	return setSong( ++songnumber, pEngine );
}

bool MidiActionManager::playlist_previous_song(Action * pAction, Hydrogen* pEngine ) {
	int songnumber = Playlist::get_instance()->getActiveSongNumber();
	return setSong( --songnumber, pEngine );
}

bool MidiActionManager::record_ready(Action * pAction, Hydrogen* pEngine ) {
	if ( pEngine->getState() != STATE_PLAYING ) {
		if (!Preferences::get_instance()->getRecordEvents()) {
			Preferences::get_instance()->setRecordEvents(true);
		}
		else {
			Preferences::get_instance()->setRecordEvents(false);
		}
	}
	return true;
}

bool MidiActionManager::record_strobe_toggle(Action * , Hydrogen*  ) {
	if (!Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(true);
	}
	else {
		Preferences::get_instance()->setRecordEvents(false);
	}
	return true;
}

bool MidiActionManager::record_strobe(Action * , Hydrogen*  ) {
	if (!Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(true);
	}
	return true;
}

bool MidiActionManager::record_exit(Action * , Hydrogen*  ) {
	if (Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(false);
	}
	return true;
}

bool MidiActionManager::toggle_metronome(Action * , Hydrogen*  ) {
	Preferences::get_instance()->m_bUseMetronome = !Preferences::get_instance()->m_bUseMetronome;
	return true;
}

bool MidiActionManager::undo_action(Action * , Hydrogen*  ) {
	EventQueue::get_instance()->push_event( EVENT_UNDO_REDO, 0);// 0 = undo
	return true;
}

bool MidiActionManager::redo_action(Action * , Hydrogen*  ) {
	EventQueue::get_instance()->push_event( EVENT_UNDO_REDO, 1);// 1 = redo
	return true;
}

/**
 * The handleAction method is the heart of the MidiActionManager class.
 * It executes the operations that are needed to carry the desired action.
 */
bool MidiActionManager::handleAction( Action * pAction ) {

	Hydrogen *pEngine = Hydrogen::get_instance();
	/*
		return false if action is null
		(for example if no Action exists for an event)
	*/
	if( pAction == NULL ) {
		return false;
	}

	QString sActionString = pAction->getType();

	map<string, action_f >::const_iterator foundAction = actionMap.find(sActionString.toStdString());
	if( foundAction != actionMap.end() ) {
		action_f action = foundAction->second;
		return (this->*action)(pAction, pEngine);
	}

	return false;
}
