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

#include <hydrogen/event_queue.h>
#include <hydrogen/hydrogen.h>
#include <gui/src/HydrogenApp.h>

#include <hydrogen/Preferences.h>
#include <hydrogen/action.h>
#include <map>

actionManager* actionManager::instance = NULL;
midiMap * midiMap::instance = NULL;

using namespace H2Core;


QString action::getType(){
	return type;
}

action::action(QString s) : Object("action") {
	type = s;
}


midiMap::midiMap() : Object( "midiMap" )
{
	//constructor
}

midiMap::~midiMap()
{
	std::map< QString , action *>::iterator dIter(mmcMap.begin());
	for( dIter = mmcMap.begin(); dIter != mmcMap.end(); dIter++ ){
		delete dIter->second;
	}
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

void midiMap::registerMMCEvent( QString eventString , action * pAction){
	mmcMap[eventString] = pAction;
}


action * midiMap::getMMCAction( QString eventString ){
	
	std::map< QString , action *>::iterator dIter;
	dIter = mmcMap.find(eventString);
	if ( dIter == mmcMap.end() ){
		return NULL;
	}	

	return mmcMap[eventString];
}


actionManager::actionManager() : Object( "actionManager" ) {
	INFOLOG( "actionManager Init" );
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


bool actionManager::handleAction( action * pAction ){

	Hydrogen *pEngine = Hydrogen::get_instance();

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
		switch (nState) 
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

	if( sActionString == "RECORD_TOGGLE"){
	}

	return true;
}
