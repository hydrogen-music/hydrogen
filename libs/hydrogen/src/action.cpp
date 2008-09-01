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

#include <hydrogen/Preferences.h>
#include <hydrogen/action.h>
#include <map>

actionManager* actionManager::instance = NULL;

using namespace H2Core;

/* Class action */

QString action::getType(){
	return type;
}

QStringList action::getParameterList(){
	return parameterList;
}

void action::addParameter( QString param ){
	parameterList.append( param );
}

action::action( QString s ) : Object( "action" ) {
	type = s;
	QStringList parameterList;
}


/* Class actionManager */


actionManager::actionManager() : Object( "actionManager" ) {
	INFOLOG( "actionManager Init" );
	
	actionList <<""
	<< "PLAY" 
	<< "PLAY_TOGGLE"
	<< "STOP"
	<< "PAUSE"
	<< ">>_NEXT_BAR"
	<< "<<_PREVIOUS_BAR"
	<< "BPM_INCR"
	<< "BPM_DECR"
	<< "BEATCOUNTER"
	<< "TAP_TEMPO";

	eventList << ""
	<< "MMC_PLAY"
	<< "MMC_DEFERRED_PLAY"
	<< "MMC_STOP"
	<< "MMC_FAST_FORWARD"
	<< "MMC_REWIND"
	<< "MMC_RECORD_STROBE"
	<< "MMC_RECORD_EXIT"
	<< "MMC_PAUSE"
	<< "NOTE";
}


actionManager::~actionManager(){
	INFOLOG( "actionManager delete" );
	instance = NULL;
}


/// Return an instance of actionManager
actionManager* actionManager::getInstance()
{
	if ( instance == NULL ) {
		instance = new actionManager();
	}
	
	return instance;
}

QStringList actionManager::getActionList(){
	return actionList;
}

QStringList actionManager::getEventList(){
	return eventList;
}

bool actionManager::handleAction( action * pAction ){

	Hydrogen *pEngine = Hydrogen::get_instance();

	/* 
		return false if action is null 
		(for example if no action exists for an event)
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

	if( sActionString == "PLAY_TOGGLE" )
	{
		int nState = pEngine->getState();
		switch ( nState ) 
		{
			case STATE_READY:
				pEngine->sequencer_play();
				break;

			case STATE_PLAYING:
				pEngine->sequencer_stop();
				break;

			default:
				ERRORLOG( "[Hydrogen::actionManager(PLAY): Unhandled case" );
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
		return true;
	}

	if( sActionString == "MUTE" ){
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


	if( sActionString == "BPM_INCR" ){
		AudioEngine::get_instance()->lock( "Action::BPM_INCR" );

		int mult = 1;	

		if( pAction->getParameterList().size() > 0){
			bool ok;
			mult = pAction->getParameterList().at(0).toInt(&ok,10);
		}


		Song* pSong = pEngine->getSong();
		if (pSong->__bpm  < 300) {
			pEngine->setBPM( pSong->__bpm + 1*mult );
		}
		AudioEngine::get_instance()->unlock();

		return true;
	}

	if( sActionString == "BPM_DECR" ){
		AudioEngine::get_instance()->lock( "Action::BPM_DECR" );

		int mult = 1;	

		if( pAction->getParameterList().size() > 0){
			bool ok;
			mult = pAction->getParameterList().at(0).toInt(&ok,10);
		}

		Song* pSong = pEngine->getSong();
		if (pSong->__bpm  > 40 ) {
			pEngine->setBPM( pSong->__bpm - 1*mult );
		}
		AudioEngine::get_instance()->unlock();
		
		return true;
	}

	if( sActionString == ">>_NEXT_BAR"){
		pEngine->setPatternPos(pEngine->getPatternPos() +1 );
		return true;
	}

	if( sActionString == "<<_PREVIOUS_BAR"){
		pEngine->setPatternPos(pEngine->getPatternPos() -1 );
		return true;
	}
	
	return false;
}
