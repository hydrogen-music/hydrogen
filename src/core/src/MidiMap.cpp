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

#include <hydrogen/midi_action.h>
#include <hydrogen/midi_map.h>
#include <map>
#include <QMutexLocker>

/**
* @class MidiMap
*
* @brief The MidiMap maps MidiActions to MidiEvents
*
*
* The MidiMap stores the mapping between midi events
* and midi actions. Each event relates to at most 1
* midi action. Several events can relate to the same action.
* Midi events are note, mmc or cc messages.
*
*
* @author Sebastian Moors
*
*/

MidiMap * MidiMap::__instance = nullptr;
const char* MidiMap::__class_name = "MidiMap";

MidiMap::MidiMap()
	: Object( __class_name )
{
	__instance = this;
	QMutexLocker mx(&__mutex);

	//constructor
	for(int note = 0; note < 128; note++ ) {
		__note_array[ note ] = new Action("NOTHING");
		__cc_array[ note ] = new Action("NOTHING");
	}
	__pc_action = new Action("NOTHING");
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
	delete __pc_action;

	__instance = nullptr;
}

void MidiMap::create_instance()
{
	if( __instance == nullptr ) {
		__instance = new MidiMap;
	}
}

void MidiMap::reset_instance()
{
	create_instance();
	__instance->reset();
}


/**
 * Clears the complete midi map and releases the memory
 * of the contained actions
 */
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

	delete __pc_action;
	__pc_action = new Action("NOTHING");
}


std::map< QString, Action* > MidiMap::getMMCMap()
{
	return mmcMap;
}


/**
 * Sets up the relation between a mmc event and an action
 */
void MidiMap::registerMMCEvent( QString eventString , Action* pAction )
{
	QMutexLocker mx(&__mutex);

	if( mmcMap[ eventString ] != nullptr){
		delete mmcMap[ eventString ];
	}
	mmcMap[ eventString ] = pAction;
}


/**
 * Sets up the relation between a note event and an action
 */
void MidiMap::registerNoteEvent( int note, Action* pAction )
{
	QMutexLocker mx(&__mutex);
	if( note >= 0 && note < 128 ) {
		delete __note_array[ note ];
		__note_array[ note ] = pAction;
	}
}


/**
 * Sets up the relation between a cc event and an action
 */
void MidiMap::registerCCEvent( int parameter , Action * pAction ){
	QMutexLocker mx(&__mutex);
	if( parameter >= 0 and parameter < 128 )
	{
		delete __cc_array[ parameter ];
		__cc_array[ parameter ] = pAction;
	}
}

int MidiMap::findCCValueByActionParam1 ( QString actionType, QString param1 ) const
{
	int nParam = -1;

	for(int i=0; i < 128; i++)
	{
		Action* pTmpAction = __cc_array[i];
		
		if(    pTmpAction->getType() == actionType
			&& pTmpAction->getParameter1() == param1 ){
			nParam = i;
		}
	}
	
	return nParam;
}

int MidiMap::findCCValueByActionType( QString actionType ) const
{
	int nParam = -1;

	for(int i=0; i < 128; i++)
	{
		Action* pTmpAction = __cc_array[i];
		
		if( pTmpAction->getType() == actionType ){
			nParam = i;
		}
	}
	
	return nParam;
}

/**
 * Sets up the relation between a program change and an action
 */
void MidiMap::registerPCEvent( Action * pAction ){
	QMutexLocker mx(&__mutex);
	delete __pc_action;
	__pc_action = pAction;
}

/**
 * Returns the mmc action which was linked to the given event.
 */
Action* MidiMap::getMMCAction( QString eventString )
{
	QMutexLocker mx(&__mutex);
	std::map< QString, Action *>::iterator dIter = mmcMap.find( eventString );
	if ( dIter == mmcMap.end() ){
		return nullptr;
	}

	return mmcMap[eventString];
}

/**
 * Returns the note action which was linked to the given event.
 */
Action* MidiMap::getNoteAction( int note )
{
	QMutexLocker mx(&__mutex);
	return __note_array[ note ];
}

/**
 * Returns the cc action which was linked to the given event.
 */
Action * MidiMap::getCCAction( int parameter )
{
	QMutexLocker mx(&__mutex);
	return __cc_array[ parameter ];
}

/**
 * Returns the pc action which was linked to the given event.
 */
Action * MidiMap::getPCAction()
{
	QMutexLocker mx(&__mutex);
	return __pc_action;
}

