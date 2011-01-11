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
#include <gui/src/HydrogenApp.h>

#include <hydrogen/instrument.h>
#include <hydrogen/Song.h>


#include <hydrogen/Preferences.h>
#include <hydrogen/action.h>
#include <map>

ActionManager* ActionManager::__instance = NULL;

using namespace H2Core;

/* Class Action */
Action::Action( QString typeString ) : Object( "Action" ) {

	/*
	    An "Action" is something which can be interpreted and executed by hydrogen.
	    Example: If hydrogen executes the Action with type "MUTE", it will mute the outputs.

	    The two parameters are optional and can be used to carry additional informations, which mean
	    only something to this very Action. They can have totally different meanings for other Actions.
	    Example: parameter1 is the Mixer strip and parameter 2 a multiplier for the volume change on this strip
	*/

	type = typeString;
	QString parameter1 = "0";
	QString parameter2 = "0" ;
}


/* Class ActionManager */

ActionManager::ActionManager() : Object( "ActionManager" )
{
	__instance = this;
	
	/*
	    the actionList holds all Action identfiers which hydrogen is able to interpret.
	*/
	actionList <<""
	<< "PLAY" 
	<< "PLAY/STOP_TOGGLE"
	<< "PLAY/PAUSE_TOGGLE"
	<< "STOP"
	<< "PAUSE"
	<< "MUTE"
	<< "UNMUTE"
	<< "MUTE_TOGGLE"
	<< ">>_NEXT_BAR"
	<< "<<_PREVIOUS_BAR"
	<< "BPM_INCR"
	<< "BPM_DECR"
	<< "BPM_CC_RELATIVE"
	<< "MASTER_VOLUME_RELATIVE"
	<< "MASTER_VOLUME_ABSOLUTE"
	<< "STRIP_VOLUME_RELATIVE"
	<< "STRIP_VOLUME_ABSOLUTE"
	<< "EFFECT1_LEVEL_RELATIVE"
	<< "EFFECT2_LEVEL_RELATIVE"
	<< "EFFECT3_LEVEL_RELATIVE"
	<< "EFFECT4_LEVEL_RELATIVE"
	<< "EFFECT1_LEVEL_ABSOLUTE"
	<< "EFFECT2_LEVEL_ABSOLUTE"
	<< "EFFECT3_LEVEL_ABSOLUTE"
	<< "EFFECT4_LEVEL_ABSOLUTE"
	<< "SELECT_NEXT_PATTERN"
        << "SELECT_AND_PLAY_NEXT_PATTERN"
	<< "PAN_RELATIVE"
	<< "PAN_ABSOLUTE"
	<< "BEATCOUNTER"
	<< "TAP_TEMPO"
	<< "SELECT_INSTRUMENT";

	eventList << ""
	<< "MMC_PLAY"
	<< "MMC_DEFERRED_PLAY"
	<< "MMC_STOP"
	<< "MMC_FAST_FORWARD"
	<< "MMC_REWIND"
	<< "MMC_RECORD_STROBE"
	<< "MMC_RECORD_EXIT"
	<< "MMC_PAUSE"
	<< "NOTE"
	<< "CC";
}


ActionManager::~ActionManager(){
	//INFOLOG( "ActionManager delete" );
	__instance = NULL;
}

void ActionManager::create_instance()
{
	if ( __instance == 0 ) {
		__instance = new ActionManager;
	}
}



bool setAbsoluteFXLevel( int nLine, int fx_channel , int fx_param)
{
	//helper function to set fx levels
			
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->get_instrument_list();
	Instrument *instr = instrList->get( nLine );
	if ( instr == NULL) return false;

	if( fx_param != 0 ){
			instr->set_fx_level(  ( (float) (fx_param / 127.0 ) ), fx_channel );
	} else {
			instr->set_fx_level( 0 , fx_channel );
	}
		
	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
	
	return true;

}

bool ActionManager::handleAction( Action * pAction ){

	Hydrogen *pEngine = Hydrogen::get_instance();

	/* 
		return false if action is null 
		(for example if no Action exists for an event)
	*/
	if( pAction == NULL )	return false;

	QString sActionString = pAction->getType();

	
	if( sActionString == "PLAY" )
	{
		int nState = pEngine->getState();
		if ( nState == STATE_READY ){
			pEngine->sequencer_play();
		}
		return true;
	}

	if( sActionString == "PLAY/STOP_TOGGLE" || sActionString == "PLAY/PAUSE_TOGGLE" )
	{
		int nState = pEngine->getState();
		switch ( nState ) 
		{
			case STATE_READY:
				pEngine->sequencer_play();
				break;

			case STATE_PLAYING:
				if( sActionString == "PLAY/STOP_TOGGLE" ) pEngine->setPatternPos( 0 );
				pEngine->sequencer_stop();
				pEngine->setTimelineBpm();
				break;

			default:
				ERRORLOG( "[Hydrogen::ActionManager(PLAY): Unhandled case" );
		}

		return true;
	}
	

	if( sActionString == "PAUSE" )
	{	
		pEngine->sequencer_stop();
		return true;
	}

	if( sActionString == "STOP" )
	{	
		pEngine->sequencer_stop();
		pEngine->setPatternPos( 0 );
		pEngine->setTimelineBpm();
		return true;
	}

	if( sActionString == "MUTE" ){
		//mutes the master, not a single strip
		pEngine->getSong()->__is_muted = true;
		return true;
	}

	if( sActionString == "UNMUTE" ){
		pEngine->getSong()->__is_muted = false;
		return true;
	}

	if( sActionString == "MUTE_TOGGLE" ){
		pEngine->getSong()->__is_muted = !Hydrogen::get_instance()->getSong()->__is_muted;
		return true;
	}

	if( sActionString == "BEATCOUNTER" ){
		pEngine->handleBeatCounter();
		return true;
	}

	if( sActionString == "TAP_TEMPO" ){
		pEngine->onTapTempoAccelEvent();
		return true;
	}

	  if( sActionString == "SELECT_NEXT_PATTERN"){
		bool ok;
		int row = pAction->getParameter1().toInt(&ok,10);
		pEngine->setSelectedPatternNumber( row );
		pEngine->sequencer_setNextPattern( row, false, true );
                return true;
        }

          if( sActionString == "SELECT_AND_PLAY_NEXT_PATTERN"){
                bool ok;
                int row = pAction->getParameter1().toInt(&ok,10);
                pEngine->setSelectedPatternNumber( row );
                pEngine->sequencer_setNextPattern( row, false, true );

                int nState = pEngine->getState();
                if ( nState == STATE_READY ){
                        pEngine->sequencer_play();
                }

                return true;
        }


	if( sActionString == "SELECT_INSTRUMENT" ){
		bool ok;
		int  instrument_number = pAction->getParameter2().toInt(&ok,10) ;
		if ( pEngine->getSong()->get_instrument_list()->get_size() < instrument_number ) 
			instrument_number = pEngine->getSong()->get_instrument_list()->get_size() -1;	
		pEngine->setSelectedInstrumentNumber( instrument_number );
		return true;
	}



	
	if( sActionString == "EFFECT1_LEVEL_ABSOLUTE" ){
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int fx_param = pAction->getParameter2().toInt(&ok,10);
		setAbsoluteFXLevel( nLine, 0 , fx_param );
	}

	if( sActionString == "EFFECT2_LEVEL_ABSOLUTE" ){
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int fx_param = pAction->getParameter2().toInt(&ok,10);
		setAbsoluteFXLevel( nLine, 1 , fx_param );
	}

	if( sActionString == "EFFECT3_LEVEL_ABSOLUTE" ){
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int fx_param = pAction->getParameter2().toInt(&ok,10);
		setAbsoluteFXLevel( nLine, 2 , fx_param );
	}

	if( sActionString == "EFFECT4_LEVEL_ABSOLUTE" ){
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int fx_param = pAction->getParameter2().toInt(&ok,10);
		setAbsoluteFXLevel( nLine, 3 , fx_param );
	}
	

	
	

	if( sActionString == "MASTER_VOLUME_RELATIVE" ){
		//increments/decrements the volume of the whole song	
		
		bool ok;
		int vol_param = pAction->getParameter2().toInt(&ok,10);
			
		Hydrogen *engine = Hydrogen::get_instance();
		Song *song = engine->getSong();



		if( vol_param != 0 ){
				if ( vol_param == 1 && song->get_volume() < 1.5 ){
					song->set_volume( song->get_volume() + 0.05 );  
				}  else  {
					if( song->get_volume() >= 0.0 ){
						song->set_volume( song->get_volume() - 0.05 );
					}
				}
		} else {
			song->set_volume( 0 );
		}

	}

	

	if( sActionString == "MASTER_VOLUME_ABSOLUTE" ){
		//sets the volume of a master output to a given level (percentage)

		bool ok;
		int vol_param = pAction->getParameter2().toInt(&ok,10);
			

		Hydrogen *engine = Hydrogen::get_instance();
		Song *song = engine->getSong();


		if( vol_param != 0 ){
				song->set_volume( 1.5* ( (float) (vol_param / 127.0 ) ));
		} else {
				song->set_volume( 0 );
		}

	}

	

	if( sActionString == "STRIP_VOLUME_RELATIVE" ){
		//increments/decrements the volume of one mixer strip	
		
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int vol_param = pAction->getParameter2().toInt(&ok,10);
			
		Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

		Hydrogen *engine = Hydrogen::get_instance();
		Song *song = engine->getSong();
		InstrumentList *instrList = song->get_instrument_list();

		Instrument *instr = instrList->get( nLine );

		if ( instr == NULL) return 0;

		if( vol_param != 0 ){
				if ( vol_param == 1 && instr->get_volume() < 1.5 ){
					instr->set_volume( instr->get_volume() + 0.1 );  
				}  else  {
					if( instr->get_volume() >= 0.0 ){
						instr->set_volume( instr->get_volume() - 0.1 );
					}
				}
		} else {
			instr->set_volume( 0 );
		}

		Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
	}

	if( sActionString == "STRIP_VOLUME_ABSOLUTE" ){
		//sets the volume of a mixer strip to a given level (percentage)

		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int vol_param = pAction->getParameter2().toInt(&ok,10);
			
		Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

		Hydrogen *engine = Hydrogen::get_instance();
		Song *song = engine->getSong();
		InstrumentList *instrList = song->get_instrument_list();

		Instrument *instr = instrList->get( nLine );

		if ( instr == NULL) return 0;

		if( vol_param != 0 ){
				instr->set_volume( 1.5* ( (float) (vol_param / 127.0 ) ));
		} else {
				instr->set_volume( 0 );
		}

		Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
	}

	
	if( sActionString == "PAN_ABSOLUTE" ){
		
		 // sets the absolute panning of a given mixer channel
		
		
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int pan_param = pAction->getParameter2().toInt(&ok,10);

		
		float pan_L;
		float pan_R;

		Hydrogen *engine = Hydrogen::get_instance();
		engine->setSelectedInstrumentNumber( nLine );
		Song *song = engine->getSong();
		InstrumentList *instrList = song->get_instrument_list();

		Instrument *instr = instrList->get( nLine );
		
		if( instr == NULL )
			return false;
		
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

		Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);

		return true;
	}

	if( sActionString == "PAN_RELATIVE" ){
		
		 // changes the panning of a given mixer channel
		// this is useful if the panning is set by a rotary control knob
		
		
		bool ok;
		int nLine = pAction->getParameter1().toInt(&ok,10);
		int pan_param = pAction->getParameter2().toInt(&ok,10);

		
		float pan_L;
		float pan_R;

		Hydrogen *engine = Hydrogen::get_instance();
		engine->setSelectedInstrumentNumber( nLine );
		Song *song = engine->getSong();
		InstrumentList *instrList = song->get_instrument_list();

		Instrument *instr = instrList->get( nLine );
		
		if( instr == NULL )
			return false;
		
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

		if( pan_param == 1 && fPanValue < 1 ){
			fPanValue += 0.05;
		}

		if( pan_param != 1 && fPanValue > 0 ){
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

		Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);

		return true;
	}
	

	if( sActionString == "BPM_CC_RELATIVE" ){
		/*
		 * increments/decrements the BPM
		 * this is useful if the bpm is set by a rotary control knob
		*/

		AudioEngine::get_instance()->lock( RIGHT_HERE );

		int mult = 1;	

		//second parameter of cc command
		//this value should be 1 to decrement and something other then 1 to increment the bpm
		int cc_param = 1;

		//this Action should be triggered only by CC commands

		bool ok;
		mult = pAction->getParameter1().toInt(&ok,10);
		cc_param = pAction->getParameter2().toInt(&ok,10);
			


		Song* pSong = pEngine->getSong();


		
		if ( cc_param == 1 && pSong->__bpm  < 300) {
			pEngine->setBPM( pSong->__bpm + 1*mult );
		}


		if ( cc_param != 1 && pSong->__bpm  > 40 ) {
			pEngine->setBPM( pSong->__bpm - 1*mult );
		}


		AudioEngine::get_instance()->unlock();

		return true;
	}


	if( sActionString == "BPM_INCR" ){
		AudioEngine::get_instance()->lock( RIGHT_HERE );

		int mult = 1;	

		bool ok;
		mult = pAction->getParameter1().toInt(&ok,10);


		Song* pSong = pEngine->getSong();
		if (pSong->__bpm  < 300) {
			pEngine->setBPM( pSong->__bpm + 1*mult );
		}
		AudioEngine::get_instance()->unlock();

		return true;
	}



	if( sActionString == "BPM_DECR" ){
		AudioEngine::get_instance()->lock( RIGHT_HERE );

		int mult = 1;	

		bool ok;
		mult = pAction->getParameter1().toInt(&ok,10);

		Song* pSong = pEngine->getSong();
		if (pSong->__bpm  > 40 ) {
			pEngine->setBPM( pSong->__bpm - 1*mult );
		}
		AudioEngine::get_instance()->unlock();
		
		return true;
	}

	if( sActionString == ">>_NEXT_BAR"){
		pEngine->setPatternPos(pEngine->getPatternPos() +1 );
		pEngine->setTimelineBpm();
		return true;
	}

	if( sActionString == "<<_PREVIOUS_BAR"){
		pEngine->setPatternPos(pEngine->getPatternPos() -1 );
		pEngine->setTimelineBpm();
		return true;
	}
	
	return false;
}
