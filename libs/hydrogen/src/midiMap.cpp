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

#include <hydrogen/action.h>
#include <hydrogen/midiMap.h>
#include <map>

midiMap * midiMap::instance = NULL;

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

