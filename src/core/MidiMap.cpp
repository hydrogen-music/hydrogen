/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

	// Constructor
	m_pcActionVector.resize( 1 );
	m_pcActionVector[ 0 ] = std::make_shared<Action>(
		Action::getNullActionType() );
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

	m_mmcActionMap.clear();
	m_noteActionMap.clear();
	m_ccActionMap.clear();
	
	m_pcActionVector.clear();
	m_pcActionVector.resize( 1 );
	m_pcActionVector[ 0 ] = std::make_shared<Action>(
		Action::getNullActionType() );
}

void MidiMap::registerMMCEvent( QString sEventString, std::shared_ptr<Action> pAction )
{
	QMutexLocker mx(&__mutex);

	for ( const auto& it : m_mmcActionMap ) {
		if ( it.first == sEventString &&
			 it.second == pAction ) {
			INFOLOG( QString( "MMC event [%1] for action [%2] was already registered" )
					 .arg( sEventString ).arg( pAction->getType() ) );
			return;
		}
	}
	
	m_mmcActionMap.insert( { sEventString, pAction } );
}

void MidiMap::registerNoteEvent( int nNote, std::shared_ptr<Action> pAction )
{
	QMutexLocker mx(&__mutex);
	
	if ( nNote < MIDI_OUT_NOTE_MIN || nNote > MIDI_OUT_NOTE_MAX ) {
		ERRORLOG( QString( "Unable to register Note MIDI action [%1]: Provided note [%2] out of bound [%3,%4]" )
				  .arg( pAction->getType() ).arg( nNote )
				  .arg( MIDI_OUT_NOTE_MIN ).arg( MIDI_OUT_NOTE_MAX ) );
		return;
	}

	for ( const auto& it : m_noteActionMap ) {
		if ( it.first == nNote &&
			 it.second == pAction ) {
			INFOLOG( QString( "Note event [%1] for action [%2] was already registered" )
					 .arg( nNote ).arg( pAction->getType() ) );
			return;
		}
	}

	m_noteActionMap.insert( { nNote, pAction } );
}

void MidiMap::registerCCEvent( int nParameter, std::shared_ptr<Action> pAction ){
	QMutexLocker mx(&__mutex);
	
	if ( nParameter < 0 || nParameter > 127 ) {
		ERRORLOG( QString( "Unable to register CC MIDI action [%1]: Provided parameter [%2] out of bound [0,127]" )
				  .arg( pAction->getType() ).arg( nParameter ) );
		return;
	}

	for ( const auto& it : m_ccActionMap ) {
		if ( it.first == nParameter &&
			 it.second == pAction ) {
			INFOLOG( QString( "CC event [%1] for action [%2] was already registered" )
					 .arg( nParameter ).arg( pAction->getType() ) );
			return;
		}
	}

	m_ccActionMap.insert( { nParameter, pAction } );
	
}

void MidiMap::registerPCEvent( std::shared_ptr<Action> pAction ){
	QMutexLocker mx(&__mutex);

	for ( const auto& action : m_pcActionVector ) {
		if ( action == pAction ) {
			INFOLOG( QString( "PC event for action [%1] was already registered" )
					 .arg( pAction->getType() ) );
			return;
		}
	}

	m_pcActionVector.push_back( pAction );
}

std::vector<std::shared_ptr<Action>> MidiMap::getMMCActions( QString sEventString )
{
	QMutexLocker mx(&__mutex);

	std::vector<std::shared_ptr<Action>> actions;

	auto range = m_mmcActionMap.equal_range( sEventString );
 
    for ( auto ii = range.first; ii != range.second; ++ii ) {
		actions.push_back( ii->second );
	}

	return std::move( actions );
}

std::vector<std::shared_ptr<Action>> MidiMap::getNoteActions( int nNote )
{
	QMutexLocker mx(&__mutex);

	std::vector<std::shared_ptr<Action>> actions;

	auto range = m_noteActionMap.equal_range( nNote );
 
    for ( auto ii = range.first; ii != range.second; ++ii ) {
		actions.push_back( ii->second );
	}

	return std::move( actions );
}

std::vector<std::shared_ptr<Action>> MidiMap::getCCActions( int nParameter ) {
	QMutexLocker mx(&__mutex);

	std::vector<std::shared_ptr<Action>> actions;

	auto range = m_ccActionMap.equal_range( nParameter );
 
    for ( auto ii = range.first; ii != range.second; ++ii ) {
		actions.push_back( ii->second );
	}

	return std::move( actions );
}

std::vector<int> MidiMap::findCCValuesByActionParam1( QString sActionType, QString sParam1 ) {
	QMutexLocker mx(&__mutex);
	std::vector<int> values;

	for ( const auto& it : m_ccActionMap ) {
		if ( it.second->getType() == sActionType &&
			it.second->getParameter1() == sParam1 ){
			values.push_back( it.first );
		}
	}
	
	return std::move( values );
}

std::vector<int> MidiMap::findCCValuesByActionType( QString sActionType ) {
	QMutexLocker mx(&__mutex);
	std::vector<int> values;

	for ( const auto& it : m_ccActionMap ) {
		if ( it.second->getType() == sActionType ){
			values.push_back( it.first );
		}
	}
	
	return std::move( values );
}

std::vector<std::pair<QString,int>> MidiMap::getRegisteredMidiEvents( std::shared_ptr<Action> pAction ) const {
	std::vector<std::pair<QString,int>> midiEvents;

	if ( pAction != nullptr && ! pAction->isNull() ) {
		for ( const auto& [nParam, ppAction] : m_noteActionMap ) {
			if ( ! ppAction->isNull() && ppAction == pAction ) {
				midiEvents.push_back( std::make_pair( "NOTE", nParam ) );
			}
		}
		for ( const auto& [nParam, ppAction] : m_ccActionMap ) {
			if ( ! ppAction->isNull() && ppAction == pAction ) {
				midiEvents.push_back( std::make_pair( "CC", nParam ) );
			}
		}
		for ( const auto& [sType, ppAction] : m_mmcActionMap ) {
			if ( ! ppAction->isNull() && ppAction == pAction ) {
				midiEvents.push_back( std::make_pair( sType, 0 ) );
			}
		}
		for ( const auto& ppAction : m_pcActionVector ) {
			INFOLOG( QString( "%1 vs %2" ).arg( pAction->toQString() ).arg( ppAction->toQString() ) );
			if ( ! ppAction->isNull() && ppAction == pAction ) {
				WARNINGLOG( "check" );
				midiEvents.push_back(
					std::make_pair( "PROGRAM_CHANGE", 0 ) );
			}
		}
	}

	return std::move( midiEvents );
}

QString MidiMap::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[MidiMap]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_noteActionMap:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& [nParam, ppAction] : m_noteActionMap ) {
			if ( ! ppAction->isNull() ) {
				sOutput.append( QString( "%1%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
								.arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "%1%2m_ccActionMap:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& [nParam, ppAction] : m_ccActionMap ) {
			if ( ! ppAction->isNull() ) {
				sOutput.append( QString( "%1%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
								.arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "%1%2m_mmcActionMap:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& [nParam, ppAction] : m_mmcActionMap ) {
			if ( ! ppAction->isNull() ) {
				sOutput.append( QString( "%1%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
								.arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "%1%2m_pcActionVector:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppAction : m_pcActionVector ) {
			if ( ! ppAction->isNull() ) {
				sOutput.append( QString( "%1%2%2%3\n" ).arg( sPrefix ).arg( s )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
	}
	else {

		sOutput = QString( "[MidiMap] m_noteActionMap: [" );
		for ( const auto& [nParam, ppAction] : m_noteActionMap ) {
			if ( ! ppAction->isNull() ) {
				sOutput.append( QString( "%1: %2, " ).arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "], m_ccActionMap: [" ) );
		for ( const auto& [nParam, ppAction] : m_ccActionMap ) {
			if ( ! ppAction->isNull() ) {
				sOutput.append( QString( "%1: %2, " ).arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "], m_mmcActionMap: [" ) );
		for ( const auto& [nParam, ppAction] : m_mmcActionMap ) {
			if ( ! ppAction->isNull() ) {
				sOutput.append( QString( "%1: %2, " ).arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( ", m_pcActionVector: [" ) );
		for ( const auto& ppAction : m_pcActionVector ) {
			if ( ! ppAction->isNull() ) {
				sOutput.append( QString( "%1, " ).arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( "]" );
	}
		
	return sOutput;
}
