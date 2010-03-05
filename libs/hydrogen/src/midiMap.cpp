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
#include <QMutexLocker>

MidiMap * MidiMap::__instance = 0;

MidiMap::MidiMap()
 : Object( "midiMap" )
{
	__instance = this;
	QMutexLocker mx(&__mutex);

	//constructor
	for(int note = 0; note < 128; note++ ) {
		__note_array[ note ] = new Action("NOTHING");
		__cc_array[ note ] = new Action("NOTHING");
	}
}

MidiMap::~MidiMap()
{
	QMutexLocker mx(&__mutex);

	map_t::iterator dIter( mmcMap.begin() );

	for( dIter = mmcMap.begin(); dIter != mmcMap.end(); dIter++ ) {
		delete dIter->second;
	}

	for( int i = 0; i < 128; i++ ) {
		delete __note_array[ i ];
		delete __cc_array[ i ];
	}

	__instance = NULL;
}

void MidiMap::create_instance()
{
	if( __instance == 0 ) {
		__instance = new MidiMap;
	}
}

void MidiMap::reset_instance()
{
	create_instance();
	__instance->reset();
}

void MidiMap::reset()
{
	QMutexLocker mx(&__mutex);

	map_t::iterator iter;
	for( iter = mmcMap.begin() ; iter != mmcMap.end() ; ++iter ) {
		delete iter->second;
	}
	mmcMap.clear();

	int i;
	for( i = 0 ; i < 128 ; ++i ) {
		delete __note_array[ i ];
		delete __cc_array[ i ];
		__note_array[ i ] = new Action("NOTHING");
		__cc_array[ i ] = new Action("NOTHING");
	}

}

std::map< QString, Action* > MidiMap::getMMCMap()
{
	return mmcMap;
}

void MidiMap::registerMMCEvent( QString eventString , Action* pAction )
{
	QMutexLocker mx(&__mutex);

	if( mmcMap[ eventString ] != NULL){
	    delete mmcMap[ eventString ];
	}
	mmcMap[ eventString ] = pAction;
}

void MidiMap::registerNoteEvent( int note, Action* pAction )
{
	QMutexLocker mx(&__mutex);
	if( note >= 0 && note < 128 ) {
		delete __note_array[ note ];
		__note_array[ note ] = pAction;
	}
}

void MidiMap::registerCCEvent( int parameter , Action * pAction ){
	QMutexLocker mx(&__mutex);
	if( parameter >= 0 and parameter < 128 )
	{
		delete __cc_array[ parameter ];
		__cc_array[ parameter ] = pAction;
	}
}

Action* MidiMap::getMMCAction( QString eventString )
{
	QMutexLocker mx(&__mutex);
	std::map< QString, Action *>::iterator dIter = mmcMap.find( eventString );
	if ( dIter == mmcMap.end() ){
		return NULL;
	}

	return mmcMap[eventString];
}

Action* MidiMap::getNoteAction( int note )
{
	QMutexLocker mx(&__mutex);
	return __note_array[ note ];
}

Action * MidiMap::getCCAction( int parameter )
{
	QMutexLocker mx(&__mutex);
	return __cc_array[ parameter ];
}

