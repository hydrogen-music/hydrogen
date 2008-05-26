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
midiMap * midiMap::instance = NULL;

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


/* Class midiMap */

midiMap::midiMap() : Object( "midiMap" )
{
	//constructor
	for(int note = 0; note < 128; note++ ){
		noteArray[ note ] = new action("NOTHING");
	}
}

midiMap::~midiMap()
{
	std::map< QString , action *>::iterator dIter(mmcMap.begin());

	for( dIter = mmcMap.begin(); dIter != mmcMap.end(); dIter++ )
	{
		delete dIter->second;
	}

	for(int i = 0; i < 128; i++){
		delete noteArray[i];
	}

	instance = NULL;
}

midiMap * midiMap::getInstance(){
	if( instance == NULL ){
		instance = new midiMap();
	}
	return instance;
}

map <QString,action *> midiMap::getMMCMap(){
	return mmcMap;
}

void midiMap::registerMMCEvent( QString eventString , action * pAction ){
	mmcMap[ eventString ] = pAction;
}

void midiMap::registerNoteEvent( int note , action * pAction ){
	
	if( note >= 0 && note < 128 ){
		delete noteArray[ note ];
		noteArray[ note ] = pAction;
	}
}

action * midiMap::getMMCAction( QString eventString ){
	
	std::map< QString , action *>::iterator dIter;
	dIter = mmcMap.find( eventString );
	if ( dIter == mmcMap.end() ){
		return NULL;
	}	

	return mmcMap[eventString];
}

action * midiMap::getNoteAction( int note ){
	return noteArray[ note ];
}




/* Class actionManager */


actionManager::actionManager() : Object( "actionManager" ) {
	INFOLOG( "actionManager Init" );
	
	actionList <<""
	<< "PLAY" 
	<< "PLAY_TOGGLE"
	<< "PAUSE"
	<< "BPM_INCR"
	<< "BPM_DECR"
	<< "STOP";

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
	}

	if( sActionString == "PAUSE" )
	{	
		pEngine->sequencer_stop();
	}

	if( sActionString == "STOP" )
	{	
		pEngine->sequencer_stop();
		pEngine->setPatternPos( 0 );
	}

	if( sActionString == "MUTE" ){
		Hydrogen::get_instance()->getSong()->__is_muted = true;
	}

	if( sActionString == "UNMUTE" ){
		Hydrogen::get_instance()->getSong()->__is_muted = false;
	}

	if( sActionString == "MUTE_TOGGLE" ){
		Hydrogen::get_instance()->getSong()->__is_muted = !Hydrogen::get_instance()->getSong()->__is_muted;
	}

	if( sActionString == "RECORD" ){
		Preferences *pref = ( Preferences::getInstance() );
		pref->setRecordEvents( true );

		//(HydrogenApp::getInstance() )->setStatusBarMessage(QString("Record keyboard/midi events = On") , 2000 );
		
	}

	if( sActionString == "BPM_INCR" ){
		Hydrogen* pEngine = Hydrogen::get_instance();
		AudioEngine::get_instance()->lock( "Action::BPM_INCR" );

		Song* pSong = pEngine->getSong();
		if (pSong->__bpm  < 300) {
			pEngine->setBPM( pSong->__bpm + 0.1 );
		}
		AudioEngine::get_instance()->unlock();
	}

	if( sActionString == "BPM_DECR" ){
		Hydrogen* pEngine = Hydrogen::get_instance();
		AudioEngine::get_instance()->lock( "Action::BPM_DECR" );

		int mult = 0;	

		/*
		if( pAction->getParameterList.size() > 0){
			bool ok;
			mult = pAction->getParameterList.at(0).toInt(&ok,10);
		}*/

		Song* pSong = pEngine->getSong();
		if (pSong->__bpm  > 40 ) {
			pEngine->setBPM( pSong->__bpm - 0.1 );
		}
		AudioEngine::get_instance()->unlock();
	}

	if( sActionString == "RECORD_TOGGLE"){
	}

	return true;
}
