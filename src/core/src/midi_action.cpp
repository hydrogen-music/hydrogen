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
* @class MidiAction
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

const char* MidiAction::__class_name = "MidiAction";

MidiAction::MidiAction( QString typeString ) : Object( __class_name ) {
	type = typeString;
	QString parameter1 = "0";
	QString parameter2 = "0" ;
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
	targeted_element empty = {0,0};
	actionMap.insert(make_pair("PLAY", make_pair(&MidiActionManager::play, empty)));
	actionMap.insert(make_pair("PLAY/STOP_TOGGLE", make_pair(&MidiActionManager::play_stop_pause_toggle, empty)));
	actionMap.insert(make_pair("PLAY/PAUSE_TOGGLE", make_pair(&MidiActionManager::play_stop_pause_toggle, empty)));
	actionMap.insert(make_pair("STOP", make_pair(&MidiActionManager::stop, empty)));
	actionMap.insert(make_pair("PAUSE", make_pair(&MidiActionManager::pause, empty)));
	actionMap.insert(make_pair("RECORD_READY", make_pair(&MidiActionManager::record_ready, empty)));
	actionMap.insert(make_pair("RECORD/STROBE_TOGGLE", make_pair(&MidiActionManager::record_strobe_toggle, empty)));
	actionMap.insert(make_pair("RECORD_STROBE", make_pair(&MidiActionManager::record_strobe, empty)));
	actionMap.insert(make_pair("RECORD_EXIT", make_pair(&MidiActionManager::record_exit, empty)));
	actionMap.insert(make_pair("MUTE", make_pair(&MidiActionManager::mute, empty)));
	actionMap.insert(make_pair("UNMUTE", make_pair(&MidiActionManager::unmute, empty)));
	actionMap.insert(make_pair("MUTE_TOGGLE", make_pair(&MidiActionManager::mute_toggle, empty)));
	actionMap.insert(make_pair(">>_NEXT_BAR", make_pair(&MidiActionManager::next_bar, empty)));
	actionMap.insert(make_pair("<<_PREVIOUS_BAR", make_pair(&MidiActionManager::previous_bar, empty)));
	actionMap.insert(make_pair("BPM_INCR", make_pair(&MidiActionManager::bpm_increase, empty)));
	actionMap.insert(make_pair("BPM_DECR", make_pair(&MidiActionManager::bpm_decrease, empty)));
	actionMap.insert(make_pair("BPM_CC_RELATIVE", make_pair(&MidiActionManager::bpm_cc_relative, empty)));
	actionMap.insert(make_pair("BPM_FINE_CC_RELATIVE", make_pair(&MidiActionManager::bpm_fine_cc_relative, empty)));
	actionMap.insert(make_pair("MASTER_VOLUME_RELATIVE", make_pair(&MidiActionManager::master_volume_relative, empty)));
	actionMap.insert(make_pair("MASTER_VOLUME_ABSOLUTE", make_pair(&MidiActionManager::master_volume_absolute, empty)));
	actionMap.insert(make_pair("STRIP_VOLUME_RELATIVE", make_pair(&MidiActionManager::strip_volume_relative, empty)));
	actionMap.insert(make_pair("STRIP_VOLUME_ABSOLUTE", make_pair(&MidiActionManager::strip_volume_absolute, empty)));
	for(int i = 0; i < MAX_FX; ++i) {
		targeted_element effect = {i,0};
		ostringstream toChar;
		toChar << (i+1);
		string keyAbsolute("EFFECT");
		string keyRelative("EFFECT");
		keyAbsolute += toChar.str();
		keyRelative += toChar.str();
		keyAbsolute += "_LEVEL_ABSOLUTE";
		keyRelative += "_LEVEL_RELATIVE";
		actionMap.insert(make_pair(keyAbsolute, make_pair(&MidiActionManager::effect_level_absolute, effect)));
		actionMap.insert(make_pair(keyRelative, make_pair(&MidiActionManager::effect_level_relative, effect)));
	}
	for(int i = 0; i < MAX_COMPONENTS; ++i) {
		ostringstream componentToChar;
		componentToChar << (i+1);
		for(int j = 0; j < MAX_LAYERS; ++j) {
			targeted_element sample = {i,j};
			ostringstream toChar;
			toChar << (j+1);
			string keyGain("GAIN_C");
			string keyPitch("PITCH_C");
			keyGain += componentToChar.str();
			keyPitch += componentToChar.str();
			keyGain += "_L";
			keyPitch += "_L";
			keyGain += toChar.str();
			keyPitch += toChar.str();
			keyGain += "_LEVEL_ABSOLUTE";
			keyPitch += "_LEVEL_ABSOLUTE";
			actionMap.insert(make_pair(keyGain, make_pair(&MidiActionManager::gain_level_absolute, sample)));
			actionMap.insert(make_pair(keyPitch, make_pair(&MidiActionManager::pitch_level_absolute, sample)));
		}
	}
	actionMap.insert(make_pair("SELECT_NEXT_PATTERN", make_pair(&MidiActionManager::select_next_pattern, empty)));
	actionMap.insert(make_pair("SELECT_NEXT_PATTERN_CC_ABSOLUTE", make_pair(&MidiActionManager::select_next_pattern_cc_absolute, empty)));
	actionMap.insert(make_pair("SELECT_NEXT_PATTERN_PROMPTLY", make_pair(&MidiActionManager::select_next_pattern_promptly, empty)));
	actionMap.insert(make_pair("SELECT_NEXT_PATTERN_RELATIVE", make_pair(&MidiActionManager::select_next_pattern_relative, empty)));
	actionMap.insert(make_pair("SELECT_AND_PLAY_PATTERN", make_pair(&MidiActionManager::select_and_play_pattern, empty)));
	actionMap.insert(make_pair("PAN_RELATIVE", make_pair(&MidiActionManager::pan_relative, empty)));
	actionMap.insert(make_pair("PAN_ABSOLUTE", make_pair(&MidiActionManager::pan_absolute, empty)));
	actionMap.insert(make_pair("FILTER_CUTOFF_LEVEL_ABSOLUTE", make_pair(&MidiActionManager::filter_cutoff_level_absolute, empty)));
	actionMap.insert(make_pair("BEATCOUNTER", make_pair(&MidiActionManager::beatcounter, empty)));
	actionMap.insert(make_pair("TAP_TEMPO", make_pair(&MidiActionManager::tap_tempo, empty)));
	actionMap.insert(make_pair("PLAYLIST_SONG", make_pair(&MidiActionManager::playlist_song, empty)));
	actionMap.insert(make_pair("PLAYLIST_NEXT_SONG", make_pair(&MidiActionManager::playlist_next_song, empty)));
	actionMap.insert(make_pair("PLAYLIST_PREV_SONG", make_pair(&MidiActionManager::playlist_previous_song, empty)));
	actionMap.insert(make_pair("TOGGLE_METRONOME", make_pair(&MidiActionManager::toggle_metronome, empty)));
	actionMap.insert(make_pair("SELECT_INSTRUMENT", make_pair(&MidiActionManager::select_instrument, empty)));
	actionMap.insert(make_pair("UNDO_ACTION", make_pair(&MidiActionManager::undo_action, empty)));
	actionMap.insert(make_pair("REDO_ACTION", make_pair(&MidiActionManager::redo_action, empty)));

	/*
		the actionList holds all Action identfiers which hydrogen is able to interpret.
	*/
	actionList <<"";
	for(map<string, pair<action_f, targeted_element> >::const_iterator actionIterator = actionMap.begin();
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

bool MidiActionManager::play(MidiAction * , Hydrogen* pEngine, targeted_element ) {
	int nState = pEngine->getState();
	if ( nState == STATE_READY ) {
		pEngine->sequencer_play();
	}
	return true;
}

bool MidiActionManager::pause(MidiAction * , Hydrogen* pEngine, targeted_element ) {
	pEngine->sequencer_stop();
	return true;
}

bool MidiActionManager::stop(MidiAction * , Hydrogen* pEngine, targeted_element ) {
	pEngine->sequencer_stop();
	pEngine->setPatternPos( 0 );
	pEngine->setTimelineBpm();
	return true;
}

bool MidiActionManager::play_stop_pause_toggle(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
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

bool MidiActionManager::mute(MidiAction * , Hydrogen* pEngine, targeted_element ) {
	//mutes the master, not a single strip
	pEngine->getSong()->__is_muted = true;
	return true;
}

bool MidiActionManager::unmute(MidiAction * , Hydrogen* pEngine, targeted_element ) {
	pEngine->getSong()->__is_muted = false;
	return true;
}

bool MidiActionManager::mute_toggle(MidiAction * , Hydrogen* pEngine, targeted_element ) {
	pEngine->getSong()->__is_muted = !Hydrogen::get_instance()->getSong()->__is_muted;
	return true;
}

bool MidiActionManager::beatcounter(MidiAction * , Hydrogen* pEngine, targeted_element ) {
	pEngine->handleBeatCounter();
	return true;
}

bool MidiActionManager::tap_tempo(MidiAction * , Hydrogen* pEngine, targeted_element ) {
	pEngine->onTapTempoAccelEvent();
	return true;
}

bool MidiActionManager::select_next_pattern(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	bool ok;
	int row = pAction->getParameter1().toInt(&ok,10);
	if( row> pEngine->getSong()->get_pattern_list()->size() -1 ) {
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

bool MidiActionManager::select_next_pattern_relative(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	bool ok;
	if(!Preferences::get_instance()->patternModePlaysSelected()) {
		return true;
	}
	int row = pEngine->getSelectedPatternNumber() + pAction->getParameter1().toInt(&ok,10);
	if( row> pEngine->getSong()->get_pattern_list()->size() -1 ) {
		return false;
	}

	pEngine->setSelectedPatternNumber( row );
	return true;
}

bool MidiActionManager::select_next_pattern_cc_absolute(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	bool ok;
	int row = pAction->getParameter2().toInt(&ok,10);
	if( row> pEngine->getSong()->get_pattern_list()->size() -1 ) {
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

bool MidiActionManager::select_next_pattern_promptly(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	// obsolete, use SELECT_NEXT_PATTERN_CC_ABSOLUT instead
	bool ok;
	int row = pAction->getParameter2().toInt(&ok,10);
	pEngine->setSelectedPatternNumberWithoutGuiEvent( row );
	return true;
}

bool MidiActionManager::select_and_play_pattern(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
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

bool MidiActionManager::select_instrument(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	bool ok;
	int  instrument_number = pAction->getParameter2().toInt(&ok,10) ;
	if ( pEngine->getSong()->get_instrument_list()->size() < instrument_number ) {
		instrument_number = pEngine->getSong()->get_instrument_list()->size() -1;
	}
	pEngine->setSelectedInstrumentNumber( instrument_number );
	return true;
}

bool MidiActionManager::effect_level_absolute(MidiAction * pAction, Hydrogen* pEngine, targeted_element nEffect) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int fx_param = pAction->getParameter2().toInt(&ok,10);
	pEngine->setSelectedInstrumentNumber( nLine );

	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();
	Instrument *instr = instrList->get( nLine );
	if ( instr == NULL)  {
		return false;
	}

	if( fx_param != 0 ) {
		instr->set_fx_level(  ( (float) (fx_param / 127.0 ) ), nEffect._id );
	} else {
		instr->set_fx_level( 0 , nEffect._id );
	}

	pEngine->setSelectedInstrumentNumber(nLine);

	return true;
}

bool MidiActionManager::effect_level_relative(MidiAction * , Hydrogen* , targeted_element ) {
	//empty ?
	return true;
}

bool MidiActionManager::master_volume_absolute(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	//sets the volume of a master output to a given level (percentage)

	bool ok;
	int vol_param = pAction->getParameter2().toInt(&ok,10);

	Song *song = pEngine->getSong();

	if( vol_param != 0 ){
		song->set_volume( 1.5* ( (float) (vol_param / 127.0 ) ));
	} else {
		song->set_volume( 0 );
	}

	return true;
}

bool MidiActionManager::master_volume_relative(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	//increments/decrements the volume of the whole song

	bool ok;
	int vol_param = pAction->getParameter2().toInt(&ok,10);

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

bool MidiActionManager::strip_volume_absolute(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	//sets the volume of a mixer strip to a given level (percentage)

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int vol_param = pAction->getParameter2().toInt(&ok,10);

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

bool MidiActionManager::strip_volume_relative(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	//increments/decrements the volume of one mixer strip

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int vol_param = pAction->getParameter2().toInt(&ok,10);

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

bool MidiActionManager::pan_absolute(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	// sets the absolute panning of a given mixer channel

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getParameter2().toInt(&ok,10);


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

bool MidiActionManager::pan_relative(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	// changes the panning of a given mixer channel
	// this is useful if the panning is set by a rotary control knob

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getParameter2().toInt(&ok,10);

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

bool MidiActionManager::gain_level_absolute(MidiAction * pAction, Hydrogen* pEngine, targeted_element nSample) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int gain_param = pAction->getParameter2().toInt(&ok,10);

	pEngine->setSelectedInstrumentNumber( nLine );
	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get( nLine );
	if( instr == 0 ) {
		return false;
	}

	InstrumentLayer* layer = instr->get_component( nSample._id )->get_layer( nSample._subId );
	if( layer == 0 ) {
		return false;
	}

	if( gain_param != 0 ) {
		layer->set_gain( 5.0* ( (float) (gain_param / 127.0 ) ) );
	} else {
		layer->set_gain( 0 );
	}

	pEngine->setSelectedInstrumentNumber( nLine );

	return true;
}

bool MidiActionManager::pitch_level_absolute(MidiAction * pAction, Hydrogen* pEngine, targeted_element nSample) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pitch_param = pAction->getParameter2().toInt(&ok,10);

	pEngine->setSelectedInstrumentNumber( nLine );
	Song *song = pEngine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get( nLine );
	if( instr == 0 ) {
		return false;
	}

	InstrumentLayer* layer = instr->get_component( nSample._id )->get_layer( nSample._subId );
	if( layer == 0 ) {
		return false;
	}

	if( pitch_param != 0 ){
		layer->set_pitch( 49* ( (float) (pitch_param / 127.0 ) ) -24.5 );
	} else {
		layer->set_pitch( -24.5 );
	}

	pEngine->setSelectedInstrumentNumber( nLine );

	return true;
}

bool MidiActionManager::filter_cutoff_level_absolute(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int filter_cutoff_param = pAction->getParameter2().toInt(&ok,10);

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

	pEngine->setSelectedInstrumentNumber(nLine);

	return true;
}

bool MidiActionManager::bpm_cc_relative(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	/*
	 * increments/decrements the BPM
	 * this is useful if the bpm is set by a rotary control knob
	*/

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	//this Action should be triggered only by CC commands

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getParameter2().toInt(&ok,10);

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

bool MidiActionManager::bpm_fine_cc_relative(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	/*
	 * increments/decrements the BPM
	 * this is useful if the bpm is set by a rotary control knob
	*/

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	//this Action should be triggered only by CC commands
	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getParameter2().toInt(&ok,10);

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

bool MidiActionManager::bpm_increase(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);

	Song* pSong = pEngine->getSong();
	if (pSong->__bpm  < 300) {
		pEngine->setBPM( pSong->__bpm + 1*mult );
	}
	AudioEngine::get_instance()->unlock();

	return true;
}

bool MidiActionManager::bpm_decrease(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);

	Song* pSong = pEngine->getSong();
	if (pSong->__bpm  > 40 ) {
		pEngine->setBPM( pSong->__bpm - 1*mult );
	}
	AudioEngine::get_instance()->unlock();

	return true;
}

bool MidiActionManager::next_bar(MidiAction * , Hydrogen* pEngine, targeted_element ) {
	pEngine->setPatternPos(pEngine->getPatternPos() +1 );
	pEngine->setTimelineBpm();
	return true;
}


bool MidiActionManager::previous_bar(MidiAction * , Hydrogen* pEngine, targeted_element ) {
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

bool MidiActionManager::playlist_song(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	bool ok;
	int songnumber = pAction->getParameter1().toInt(&ok,10);
	return setSong( songnumber, pEngine );
}

bool MidiActionManager::playlist_next_song(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	int songnumber = Playlist::get_instance()->getActiveSongNumber();
	return setSong( ++songnumber, pEngine );
}

bool MidiActionManager::playlist_previous_song(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
	int songnumber = Playlist::get_instance()->getActiveSongNumber();
	return setSong( --songnumber, pEngine );
}

bool MidiActionManager::record_ready(MidiAction * pAction, Hydrogen* pEngine, targeted_element ) {
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

bool MidiActionManager::record_strobe_toggle(MidiAction * , Hydrogen* , targeted_element ) {
	if (!Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(true);
	}
	else {
		Preferences::get_instance()->setRecordEvents(false);
	}
	return true;
}

bool MidiActionManager::record_strobe(MidiAction * , Hydrogen* , targeted_element ) {
	if (!Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(true);
	}
	return true;
}

bool MidiActionManager::record_exit(MidiAction * , Hydrogen* , targeted_element ) {
	if (Preferences::get_instance()->getRecordEvents()) {
		Preferences::get_instance()->setRecordEvents(false);
	}
	return true;
}

bool MidiActionManager::toggle_metronome(MidiAction * , Hydrogen* , targeted_element ) {
	Preferences::get_instance()->m_bUseMetronome = !Preferences::get_instance()->m_bUseMetronome;
	return true;
}

bool MidiActionManager::undo_action(MidiAction * , Hydrogen* , targeted_element ) {
	EventQueue::get_instance()->push_event( EVENT_UNDO_REDO, 0);// 0 = undo
	return true;
}

bool MidiActionManager::redo_action(MidiAction * , Hydrogen* , targeted_element ) {
	EventQueue::get_instance()->push_event( EVENT_UNDO_REDO, 1);// 1 = redo
	return true;
}

/**
 * The handleAction method is the heart of the MidiActionManager class.
 * It executes the operations that are needed to carry the desired action.
 */
bool MidiActionManager::handleAction( MidiAction * pAction ) {

	Hydrogen *pEngine = Hydrogen::get_instance();
	/*
		return false if action is null
		(for example if no Action exists for an event)
	*/
	if( pAction == NULL ) {
		return false;
	}

	QString sActionString = pAction->getType();

	map<string, pair<action_f, targeted_element> >::const_iterator foundAction = actionMap.find(sActionString.toStdString());
	if( foundAction != actionMap.end() ) {
		action_f action = foundAction->second.first;
		targeted_element nElement = foundAction->second.second;
		return (this->*action)(pAction, pEngine, nElement);
	}

	return false;
}
