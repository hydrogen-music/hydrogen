/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include "MidiMap.h"

#include <core/Midi/MidiAction.h>

#include <map>
#include <QMutexLocker>

namespace H2Core {

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
MidiMap::MidiMap()
{
	QMutexLocker mx(&__mutex);

	// Constructor
	m_pcActionVector.resize( 1 );
	m_pcActionVector[ 0 ] = std::make_shared<Action>(
		Action::getNullActionType() );
}

MidiMap::~MidiMap()
{
	QMutexLocker mx(&__mutex);
}

std::shared_ptr<MidiMap> MidiMap::loadFrom( const H2Core::XMLNode& node,
											bool bSilent ) {
	auto pMidiMap = std::make_shared<MidiMap>();

	auto eventNode = node.firstChildElement( "midiEvent" );
	while ( ! eventNode.isNull() ) {
		const QString sNodeName = eventNode.firstChildElement().nodeName();
		if ( sNodeName == "mmcEvent" ) {
			std::shared_ptr<Action> pAction = std::make_shared<Action>(
				eventNode.firstChildElement( "action" ).text() );
			pAction->setParameter1(
				eventNode.firstChildElement( "parameter" ).text() );
			pAction->setParameter2(
				eventNode.firstChildElement( "parameter2" ).text() );
			pAction->setParameter3(
				eventNode.firstChildElement( "parameter3" ).text() );

			pMidiMap->registerMMCEvent(
				eventNode.firstChildElement( "mmcEvent" ).text(),
				pAction );
		}
		else if ( sNodeName == "noteEvent" ) {
			std::shared_ptr<Action> pAction = std::make_shared<Action>(
				eventNode.firstChildElement( "action" ).text() );
			pAction->setParameter1(
				eventNode.firstChildElement( "parameter" ).text() );
			pAction->setParameter2(
				eventNode.firstChildElement( "parameter2" ).text() );
			pAction->setParameter3(
				eventNode.firstChildElement( "parameter3" ).text() );

			pMidiMap->registerNoteEvent(
				eventNode.firstChildElement( "eventParameter").text().toInt(),
				pAction );
		}
		else if ( sNodeName == "ccEvent" ){
			std::shared_ptr<Action> pAction = std::make_shared<Action>(
				eventNode.firstChildElement( "action" ).text() );
			pAction->setParameter1(
				eventNode.firstChildElement( "parameter" ).text() );
			pAction->setParameter2(
				eventNode.firstChildElement( "parameter2" ).text() );
			pAction->setParameter3(
				eventNode.firstChildElement( "parameter3" ).text() );
			pMidiMap->registerCCEvent(
				eventNode.firstChildElement( "eventParameter" ).text().toInt(),
				pAction );
		}
		else if ( sNodeName == "pcEvent" ){
			std::shared_ptr<Action> pAction = std::make_shared<Action>(
				eventNode.firstChildElement( "action" ).text() );
			pAction->setParameter1(
				eventNode.firstChildElement( "parameter" ).text() );
			pAction->setParameter2(
				eventNode.firstChildElement( "parameter2" ).text() );
			pAction->setParameter3(
				eventNode.firstChildElement( "parameter3" ).text() );
			pMidiMap->registerPCEvent( pAction );
		}
		else {
			WARNINGLOG( QString( "Unknown MIDI map node [%1]" )
						.arg( sNodeName ) );
		}

		eventNode = eventNode.nextSiblingElement( "midiEvent" );
	}

	return pMidiMap;
}

void MidiMap::saveTo( H2Core::XMLNode& node, bool bSilent ) const {
	auto midiEventMapNode = node.createNode( "midiEventMap" );

	for( const auto& [ssType, ppAction] : m_mmcActionMap ){
		if ( ppAction != nullptr && ! ppAction->isNull() ){
			auto midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			midiEventNode.write_string( "mmcEvent" , ssType );
			midiEventNode.write_string( "action" , ppAction->getType());
			midiEventNode.write_string( "parameter" , ppAction->getParameter1() );
			midiEventNode.write_string( "parameter2" , ppAction->getParameter2() );
			midiEventNode.write_string( "parameter3" , ppAction->getParameter3() );
		}
	}

	for ( const auto& [nnPitch, ppAction] : m_noteActionMap ){
		if ( ppAction != nullptr && ! ppAction->isNull() ){
			auto midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			midiEventNode.write_string(
				"noteEvent", H2Core::MidiMessage::EventToQString(
					H2Core::MidiMessage::Event::Note ) );
			midiEventNode.write_int( "eventParameter" , nnPitch );
			midiEventNode.write_string( "action" , ppAction->getType() );
			midiEventNode.write_string( "parameter" , ppAction->getParameter1() );
			midiEventNode.write_string( "parameter2" , ppAction->getParameter2() );
			midiEventNode.write_string( "parameter3" , ppAction->getParameter3() );
		}
	}

	for ( const auto& [nnParam, ppAction] : m_ccActionMap ){
		if ( ppAction != nullptr && ! ppAction->isNull() ){
			auto midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			midiEventNode.write_string(
				"ccEvent", H2Core::MidiMessage::EventToQString(
					H2Core::MidiMessage::Event::CC ) );
			midiEventNode.write_int( "eventParameter" , nnParam );
			midiEventNode.write_string( "action" , ppAction->getType() );
			midiEventNode.write_string( "parameter" , ppAction->getParameter1() );
			midiEventNode.write_string( "parameter2" , ppAction->getParameter2() );
			midiEventNode.write_string( "parameter3" , ppAction->getParameter3() );
		}
	}

	for ( const auto& ppAction : m_pcActionVector ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ){
			auto midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			midiEventNode.write_string(
				"pcEvent", H2Core::MidiMessage::EventToQString(
					H2Core::MidiMessage::Event::PC ) );
			midiEventNode.write_string( "action" , ppAction->getType() );
			midiEventNode.write_string( "parameter" , ppAction->getParameter1() );
			midiEventNode.write_string( "parameter2" , ppAction->getParameter2() );
			midiEventNode.write_string( "parameter3" , ppAction->getParameter3() );
		}
	}
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

void MidiMap::registerMMCEvent( const QString& sEventString, std::shared_ptr<Action> pAction )
{
	QMutexLocker mx(&__mutex);

	if ( pAction == nullptr || pAction->isNull() ) {
		ERRORLOG( "Invalid action" );
		return;
	}

	const auto event = H2Core::MidiMessage::QStringToEvent( sEventString );
	if ( event == H2Core::MidiMessage::Event::Null ||
		 event == H2Core::MidiMessage::Event::Note ||
		 event == H2Core::MidiMessage::Event::CC ||
		 event == H2Core::MidiMessage::Event::PC ) {
		ERRORLOG( QString( "Provided event string [%1] is no supported MMC event" )
				  .arg( sEventString ) );
		return;
	}

	for ( const auto& [ssType, ppAction] : m_mmcActionMap ) {
		if ( ppAction != nullptr && ssType == sEventString &&
			 ppAction->isEquivalentTo( pAction ) ) {
			WARNINGLOG( QString( "MMC event [%1] for Action [%2: Param1: [%3], Param2: [%4], Param3: [%5]] was already registered" )
						.arg( sEventString ).arg( pAction->getType() )
						.arg( pAction->getParameter1() )
						.arg( pAction->getParameter2() )
						.arg( pAction->getParameter3() ) );
			return;
		}
	}
	
	m_mmcActionMap.insert( { sEventString, pAction } );
}

void MidiMap::registerNoteEvent( int nNote, std::shared_ptr<Action> pAction )
{
	QMutexLocker mx(&__mutex);

	if ( pAction == nullptr || pAction->isNull() ) {
		ERRORLOG( "Invalid action" );
		return;
	}
	
	if ( nNote < MIDI_OUT_NOTE_MIN || nNote > MIDI_OUT_NOTE_MAX ) {
		ERRORLOG( QString( "Unable to register Note MIDI [%1]: Provided note [%2] out of bound [%3,%4]" )
				  .arg( pAction->toQString() ).arg( nNote )
				  .arg( MIDI_OUT_NOTE_MIN ).arg( MIDI_OUT_NOTE_MAX ) );
		return;
	}

	for ( const auto& [nnPitch, ppAction] : m_noteActionMap ) {
		if ( ppAction != nullptr && nnPitch == nNote &&
			 ppAction->isEquivalentTo( pAction ) ) {
			WARNINGLOG( QString( "NOTE event [%1] for Action [%2: Param1: [%3], Param2: [%4], Param3: [%5]] was already registered" )
						.arg( nNote ).arg( pAction->getType() )
						.arg( pAction->getParameter1() )
						.arg( pAction->getParameter2() )
						.arg( pAction->getParameter3() ) );
			return;
		}
	}

	m_noteActionMap.insert( { nNote, pAction } );
}

void MidiMap::registerCCEvent( int nParameter, std::shared_ptr<Action> pAction ){
	QMutexLocker mx(&__mutex);

	if ( pAction == nullptr || pAction->isNull() ) {
		ERRORLOG( "Invalid action" );
		return;
	}

	if ( nParameter < 0 || nParameter > 127 ) {
		ERRORLOG( QString( "Unable to register CC MIDI [%1]: Provided parameter [%2] out of bound [0,127]" )
				  .arg( pAction->toQString() ).arg( nParameter ) );
		return;
	}

	for ( const auto& [nnParam, ppAction] : m_ccActionMap ) {
		if ( ppAction != nullptr && nnParam == nParameter &&
			 ppAction->isEquivalentTo( pAction ) ) {
			WARNINGLOG( QString( "CC event [%1] for Action [%2: Param1: [%3], Param2: [%4], Param3: [%5]] was already registered" )
						.arg( nParameter ).arg( pAction->getType() )
						.arg( pAction->getParameter1() )
						.arg( pAction->getParameter2() )
						.arg( pAction->getParameter3() ) );
			return;
		}
	}

	m_ccActionMap.insert( { nParameter, pAction } );
}

void MidiMap::registerPCEvent( std::shared_ptr<Action> pAction ){
	QMutexLocker mx(&__mutex);

	if ( pAction == nullptr || pAction->isNull() ) {
		ERRORLOG( "Invalid action" );
		return;
	}

	for ( const auto& ppAction : m_pcActionVector ) {
		if ( ppAction != nullptr && ppAction->isEquivalentTo( pAction ) ) {
			WARNINGLOG( QString( "PC event for Action [%2: Param1: [%3], Param2: [%4], Param3: [%5]] was already registered" )
						.arg( pAction->getType() )
						.arg( pAction->getParameter1() )
						.arg( pAction->getParameter2() )
						.arg( pAction->getParameter3() ) );
			return;
		}
	}

	m_pcActionVector.push_back( pAction );
}

std::vector<std::shared_ptr<Action>> MidiMap::getMMCActions( const QString& sEventString )
{
	QMutexLocker mx(&__mutex);

	std::vector<std::shared_ptr<Action>> actions;

	auto range = m_mmcActionMap.equal_range( sEventString );
 
    for ( auto ii = range.first; ii != range.second; ++ii ) {
		if ( ii->second != nullptr ) {
			actions.push_back( ii->second );
		}
	}

	return actions;
}

std::vector<std::shared_ptr<Action>> MidiMap::getNoteActions( int nNote )
{
	QMutexLocker mx(&__mutex);

	std::vector<std::shared_ptr<Action>> actions;

	auto range = m_noteActionMap.equal_range( nNote );
 
    for ( auto ii = range.first; ii != range.second; ++ii ) {
		if ( ii->second != nullptr ) {
			actions.push_back( ii->second );
		}
	}

	return actions;
}

std::vector<std::shared_ptr<Action>> MidiMap::getCCActions( int nParameter ) {
	QMutexLocker mx(&__mutex);

	std::vector<std::shared_ptr<Action>> actions;

	auto range = m_ccActionMap.equal_range( nParameter );
 
    for ( auto ii = range.first; ii != range.second; ++ii ) {
		if ( ii->second != nullptr ) {
			actions.push_back( ii->second );
		}
	}

	return actions;
}

std::vector<int> MidiMap::findCCValuesByActionParam1( const QString& sActionType,
													  const QString& sParam1 ) {
	QMutexLocker mx(&__mutex);
	std::vector<int> values;

	for ( const auto& [nnParam, ppAction] : m_ccActionMap ) {
		if ( ppAction != nullptr && ppAction->getType() == sActionType &&
			 ppAction->getParameter1() == sParam1 ){
			values.push_back( nnParam );
		}
	}
	
	return values;
}

std::vector<int> MidiMap::findCCValuesByActionType( const QString& sActionType ) {
	QMutexLocker mx(&__mutex);
	std::vector<int> values;

	for ( const auto& [nnParam, ppAction] : m_ccActionMap ) {
		if ( ppAction != nullptr && ppAction->getType() == sActionType ){
			values.push_back( nnParam );
		}
	}
	
	return values;
}

std::vector<std::pair<H2Core::MidiMessage::Event,int>> MidiMap::getRegisteredMidiEvents( std::shared_ptr<Action> pAction ) const {
	std::vector<std::pair<H2Core::MidiMessage::Event,int>> midiEvents;

	if ( pAction != nullptr && ! pAction->isNull() ) {
		for ( const auto& [nnParam, ppAction] : m_noteActionMap ) {
			if ( ppAction != nullptr &&
				 ppAction->isEquivalentTo( pAction ) ) {
				midiEvents.push_back( std::make_pair(
										  H2Core::MidiMessage::Event::Note, nnParam ) );
			}
		}
		for ( const auto& [nnParam, ppAction] : m_ccActionMap ) {
			if ( ppAction != nullptr &&
				 ppAction->isEquivalentTo( pAction ) ) {
				midiEvents.push_back( std::make_pair(
										  H2Core::MidiMessage::Event::CC, nnParam ) );
			}
		}
		for ( const auto& [ssType, ppAction] : m_mmcActionMap ) {
			if ( ppAction != nullptr &&
				 ppAction->isEquivalentTo( pAction ) ) {
				const auto event = H2Core::MidiMessage::QStringToEvent( ssType );
				if ( event == H2Core::MidiMessage::Event::Null ||
					 event == H2Core::MidiMessage::Event::Note ||
					 event == H2Core::MidiMessage::Event::CC ||
					 event == H2Core::MidiMessage::Event::PC ) {
					ERRORLOG( QString( "Unexpected event type [%1] found in mmcActionMap" )
							  .arg( ssType ) );
					continue;
				}
				midiEvents.push_back( std::make_pair( event, 0 ) );
			}
		}
		for ( const auto& ppAction : m_pcActionVector ) {
			if ( ppAction != nullptr &&
				 ppAction->isEquivalentTo( pAction ) ) {
				midiEvents.push_back( std::make_pair(
										  H2Core::MidiMessage::Event::PC, 0 ) );
			}
		}
	}

	return midiEvents;
}

QString MidiMap::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[MidiMap]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_noteActionMap:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& [nParam, ppAction] : m_noteActionMap ) {
			if ( ppAction != nullptr && ! ppAction->isNull() ) {
				sOutput.append( QString( "%1%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
								.arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "%1%2m_ccActionMap:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& [nParam, ppAction] : m_ccActionMap ) {
			if ( ppAction != nullptr && ! ppAction->isNull() ) {
				sOutput.append( QString( "%1%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
								.arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "%1%2m_mmcActionMap:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& [nParam, ppAction] : m_mmcActionMap ) {
			if ( ppAction != nullptr && ! ppAction->isNull() ) {
				sOutput.append( QString( "%1%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
								.arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "%1%2m_pcActionVector:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppAction : m_pcActionVector ) {
			if ( ppAction != nullptr && ! ppAction->isNull() ) {
				sOutput.append( QString( "%1%2%2%3\n" ).arg( sPrefix ).arg( s )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
	}
	else {

		sOutput = QString( "[MidiMap] m_noteActionMap: [" );
		for ( const auto& [nParam, ppAction] : m_noteActionMap ) {
			if ( ppAction != nullptr && ! ppAction->isNull() ) {
				sOutput.append( QString( "%1: %2, " ).arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "], m_ccActionMap: [" ) );
		for ( const auto& [nParam, ppAction] : m_ccActionMap ) {
			if ( ppAction != nullptr && ! ppAction->isNull() ) {
				sOutput.append( QString( "%1: %2, " ).arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "], m_mmcActionMap: [" ) );
		for ( const auto& [nParam, ppAction] : m_mmcActionMap ) {
			if ( ppAction != nullptr && ! ppAction->isNull() ) {
				sOutput.append( QString( "%1: %2, " ).arg( nParam )
								.arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( ", m_pcActionVector: [" ) );
		for ( const auto& ppAction : m_pcActionVector ) {
			if ( ppAction != nullptr && ! ppAction->isNull() ) {
				sOutput.append( QString( "%1, " ).arg( ppAction->toQString( "", true ) ) );
			}
		}
		sOutput.append( "]" );
	}
		
	return sOutput;
}
};
