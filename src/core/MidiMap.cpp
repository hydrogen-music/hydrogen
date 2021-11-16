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

#include <core/MidiAction.h>
#include "MidiMap.h"
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

MidiMap::MidiMap()
{
	__instance = this;
	QMutexLocker mx(&__mutex);

	m_noteVector.resize( 128 );
	m_ccVector.resize( 128 );
	
	//constructor
	for(int note = 0; note < 128; note++ ) {
		m_noteVector[ note ] = std::make_shared<Action>("NOTHING");
		m_ccVector[ note ] = std::make_shared<Action>("NOTHING");
	}
	m_pPcAction = std::make_shared<Action>("NOTHING");
}

MidiMap::~MidiMap()
{
	QMutexLocker mx(&__mutex);

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

	mmcMap.clear();

	m_noteVector.clear();
	m_ccVector.clear();
	m_noteVector.resize( 128 );
	m_ccVector.resize( 128 );
	for( int ii = 0 ; ii < 128 ; ++ii ) {
		m_noteVector[ ii ] = std::make_shared<Action>("NOTHING");
		m_ccVector[ ii ] = std::make_shared<Action>("NOTHING");
	}

	m_pPcAction = std::make_shared<Action>("NOTHING");
}


std::map< QString, std::shared_ptr<Action>> MidiMap::getMMCMap()
{
	return mmcMap;
}


/**
 * Sets up the relation between a mmc event and an action
 */
void MidiMap::registerMMCEvent( QString eventString, std::shared_ptr<Action> pAction )
{
	QMutexLocker mx(&__mutex);
	mmcMap[ eventString ] = pAction;
}


/**
 * Sets up the relation between a note event and an action
 */
void MidiMap::registerNoteEvent( int note, std::shared_ptr<Action> pAction )
{
	QMutexLocker mx(&__mutex);
	if( note >= 0 && note < 128 ) {
		m_noteVector[ note ] = pAction;
	} else {
		ERRORLOG( QString( "Unable to register MIDI action [%1]: Provided note [%2] out of bound [0,128)" )
				  .arg( pAction->getType() ).arg( note ) );
	}
}


/**
 * Sets up the relation between a cc event and an action
 */
void MidiMap::registerCCEvent( int parameter, std::shared_ptr<Action> pAction ){
	QMutexLocker mx(&__mutex);
	if( parameter >= 0 && parameter < 128 ) {
		m_ccVector[ parameter ] = pAction;
	} else {
		ERRORLOG( QString( "Unable to register MIDI action [%1]: Provided parameter [%2] out of bound [0,128)" )
				  .arg( pAction->getType() ).arg( parameter ) );
	}
}

int MidiMap::findCCValueByActionParam1 ( QString actionType, QString param1 ) const
{
	int nParam = -1;

	for(int i=0; i < 128; i++)
	{
		std::shared_ptr<Action> pTmpAction = m_ccVector[i];
		
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
		std::shared_ptr<Action> pTmpAction = m_ccVector[i];
		
		if( pTmpAction->getType() == actionType ){
			nParam = i;
		}
	}
	
	return nParam;
}

/**
 * Sets up the relation between a program change and an action
 */
void MidiMap::registerPCEvent( std::shared_ptr<Action> pAction ){
	QMutexLocker mx(&__mutex);
	m_pPcAction = pAction;
}

/**
 * Returns the mmc action which was linked to the given event.
 */
std::shared_ptr<Action> MidiMap::getMMCAction( QString eventString )
{
	QMutexLocker mx(&__mutex);
	std::map< QString, std::shared_ptr<Action>>::iterator dIter = mmcMap.find( eventString );
	if ( dIter == mmcMap.end() ){
		return nullptr;
	}

	return mmcMap[eventString];
}

/**
 * Returns the note action which was linked to the given event.
 */
std::shared_ptr<Action> MidiMap::getNoteAction( int note )
{
	QMutexLocker mx(&__mutex);
	return m_noteVector[ note ];
}

/**
 * Returns the cc action which was linked to the given event.
 */
std::shared_ptr<Action> MidiMap::getCCAction( int parameter )
{
	QMutexLocker mx(&__mutex);
	return m_ccVector[ parameter ];
}

/**
 * Returns the pc action which was linked to the given event.
 */
std::shared_ptr<Action> MidiMap::getPCAction()
{
	QMutexLocker mx(&__mutex);
	return m_pPcAction;
}

