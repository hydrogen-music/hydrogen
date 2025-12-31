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
#include "MidiEventMap.h"

#include <QMutexLocker>

#include <core/Basics/Event.h>
#include <core/EventQueue.h>
#include <core/Midi/MidiEvent.h>
#include <core/Helpers/Xml.h>

namespace H2Core {

/**
* @class MidiEventMap
*
* @brief The MidiEventMap maps MidiActions to MidiEvents
*
*
* The MidiEventMap stores the mapping between midi events
* and midi actions. Each event relates to at most 1
* midi action. Several events can relate to the same action.
* Midi events are note, mmc or cc messages.
*
*
* @author Sebastian Moors
*
*/
MidiEventMap::MidiEventMap()
{
}

MidiEventMap::~MidiEventMap()
{
	QMutexLocker mx(&__mutex);
}

std::shared_ptr<MidiEventMap> MidiEventMap::loadFrom( const H2Core::XMLNode& node,
											bool bSilent ) {
	auto pMidiEventMap = std::make_shared<MidiEventMap>();

	auto eventNode = node.firstChildElement( "midiEvent" );
	while ( ! eventNode.isNull() ) {
		const QString sNodeName = eventNode.firstChildElement().nodeName();
		if ( sNodeName == "mmcEvent" ) {
			std::shared_ptr<MidiAction> pAction = std::make_shared<MidiAction>(
				MidiAction::parseType( eventNode.firstChildElement( "action" ).text() ) );
			pAction->setParameter1(
				eventNode.firstChildElement( "parameter" ).text() );
			pAction->setParameter2(
				eventNode.firstChildElement( "parameter2" ).text() );
			pAction->setParameter3(
				eventNode.firstChildElement( "parameter3" ).text() );

			pMidiEventMap->registerEvent(
				MidiEvent::QStringToType(
					eventNode.firstChildElement( "mmcEvent" ).text()
				),
				MidiEvent::nNullParameter, pAction
			);
		}
		else if ( sNodeName == "noteEvent" ) {
			std::shared_ptr<MidiAction> pAction = std::make_shared<MidiAction>(
				MidiAction::parseType( eventNode.firstChildElement( "action" ).text() ) );
			pAction->setParameter1(
				eventNode.firstChildElement( "parameter" ).text() );
			pAction->setParameter2(
				eventNode.firstChildElement( "parameter2" ).text() );
			pAction->setParameter3(
				eventNode.firstChildElement( "parameter3" ).text() );

			pMidiEventMap->registerEvent(
				MidiEvent::Type::Note,
				eventNode.firstChildElement( "eventParameter" ).text().toInt(),
				pAction
			);
		}
		else if ( sNodeName == "ccEvent" ){
			std::shared_ptr<MidiAction> pAction = std::make_shared<MidiAction>(
				MidiAction::parseType( eventNode.firstChildElement( "action" ).text() ) );
			pAction->setParameter1(
				eventNode.firstChildElement( "parameter" ).text() );
			pAction->setParameter2(
				eventNode.firstChildElement( "parameter2" ).text() );
			pAction->setParameter3(
				eventNode.firstChildElement( "parameter3" ).text() );
			pMidiEventMap->registerEvent(
				MidiEvent::Type::CC,
				eventNode.firstChildElement( "eventParameter" ).text().toInt(),
				pAction
			);
		}
		else if ( sNodeName == "pcEvent" ){
			std::shared_ptr<MidiAction> pAction = std::make_shared<MidiAction>(
				MidiAction::parseType( eventNode.firstChildElement( "action" ).text() ) );
			pAction->setParameter1(
				eventNode.firstChildElement( "parameter" ).text() );
			pAction->setParameter2(
				eventNode.firstChildElement( "parameter2" ).text() );
			pAction->setParameter3(
				eventNode.firstChildElement( "parameter3" ).text() );
			pMidiEventMap->registerEvent(
				MidiEvent::Type::PC, MidiEvent::nNullParameter, pAction
			);
		}
		else {
			WARNINGLOG( QString( "Unknown MIDI map node [%1]" )
						.arg( sNodeName ) );
		}

		eventNode = eventNode.nextSiblingElement( "midiEvent" );
	}

	return pMidiEventMap;
}

void MidiEventMap::saveTo( H2Core::XMLNode& node, bool bSilent ) const {
	auto midiEventMapNode = node.createNode( "midiEventMap" );

	for( const auto& ppEvent : m_events ){
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 !ppEvent->getMidiAction()->isNull() ) {

			auto midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			switch ( ppEvent->getType() ) {
				case MidiEvent::Type::CC: {
					midiEventNode.write_string(
						"ccEvent", H2Core::MidiEvent::TypeToQString(
									   H2Core::MidiEvent::Type::CC
								   )
					);
					midiEventNode.write_int(
						"eventParameter", ppEvent->getParameter()
					);
					break;
				}
				case MidiEvent::Type::Note: {
					midiEventNode.write_string(
						"noteEvent", H2Core::MidiEvent::TypeToQString(
										 H2Core::MidiEvent::Type::Note
									 )
					);
					midiEventNode.write_int(
						"eventParameter", ppEvent->getParameter()
					);
					break;
				}
				case MidiEvent::Type::PC: {
					midiEventNode.write_string(
						"pcEvent", H2Core::MidiEvent::TypeToQString(
									   H2Core::MidiEvent::Type::PC
								   )
					);
					break;
				}
				case MidiEvent::Type::MmcDeferredPlay:
				case MidiEvent::Type::MmcFastForward:
				case MidiEvent::Type::MmcPlay:
				case MidiEvent::Type::MmcPause:
				case MidiEvent::Type::MmcRecordExit:
				case MidiEvent::Type::MmcRecordReady:
				case MidiEvent::Type::MmcRecordStrobe:
				case MidiEvent::Type::MmcRewind:
				case MidiEvent::Type::MmcStop: {
					midiEventNode.write_string(
						"mmcEvent",
						H2Core::MidiEvent::TypeToQString( ppEvent->getType() )
					);
					break;
				}
				case MidiEvent::Type::Null:
				default:
					ERRORLOG( QString( "Unknown type [%1]" )
								  .arg( ppEvent->toQString() ) );
					return;
			}
			midiEventNode.write_string(
				"action",
				MidiAction::typeToQString( ppEvent->getMidiAction()->getType() )
			);
			midiEventNode.write_string(
				"parameter", ppEvent->getMidiAction()->getParameter1()
			);
			midiEventNode.write_string(
				"parameter2", ppEvent->getMidiAction()->getParameter2()
			);
			midiEventNode.write_string(
				"parameter3", ppEvent->getMidiAction()->getParameter3()
			);
		}
	}
}

/**
 * Clears the complete midi map and releases the memory
 * of the contained actions
 */
void MidiEventMap::reset()
{
	QMutexLocker mx(&__mutex);

	m_events.clear();
}

void MidiEventMap::registerEvent(
	const MidiEvent::Type& type,
	int nParameter,
	std::shared_ptr<MidiAction> pAction
)
{
	QMutexLocker mx( &__mutex );

	if ( pAction == nullptr || pAction->isNull() ||
         type == H2Core::MidiEvent::Type::Null ) {
		ERRORLOG( "Invalid input" );
		return;
	}

	if ( type == MidiEvent::Type::Note && ( nParameter < MIDI_OUT_NOTE_MIN ||
											nParameter > MIDI_OUT_NOTE_MAX ) ) {
		ERRORLOG( QString( "Unable to register Note MIDI [%1]: Provided note "
						   "[%2] out of bound [%3,%4]" )
					  .arg( pAction->toQString() )
					  .arg( nParameter )
					  .arg( MIDI_OUT_NOTE_MIN )
					  .arg( MIDI_OUT_NOTE_MAX ) );
		return;
	}
	else if ( type == MidiEvent::Type::CC &&
			  ( nParameter < 0 || nParameter > 127 ) ) {
		ERRORLOG( QString( "Unable to register CC MIDI [%1]: Provided "
						   "parameter [%2] out of bound [0,127]" )
					  .arg( pAction->toQString() )
					  .arg( nParameter ) );
		return;
	}

	auto pEvent = std::make_shared<MidiEvent>();
	pEvent->setType( type );
	pEvent->setParameter( nParameter );
	pEvent->setMidiAction( pAction );

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == type &&
			 ppEvent->getParameter() == nParameter &&
			 ppEvent->getMidiAction()->isEquivalentTo( pAction ) ) {
			WARNINGLOG(
				QString( "Event [%1] for MidiAction [%2: Param1: [%3], "
						 "Param2: [%4], Param3: [%5]] was already registered" )
					.arg( MidiEvent::TypeToQString( type ) )
					.arg( MidiAction::typeToQString( pAction->getType() ) )
					.arg( pAction->getParameter1() )
					.arg( pAction->getParameter2() )
					.arg( pAction->getParameter3() )
			);
			return;
		}
	}

	m_events.push_back( pEvent );
}

std::vector<std::shared_ptr<MidiAction>> MidiEventMap::getMMCActions( const QString& sEventString )
{
	QMutexLocker mx(&__mutex);

	std::vector<std::shared_ptr<MidiAction>> actions;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::QStringToType( sEventString ) ) {
            actions.push_back( ppEvent->getMidiAction() );
		}
	}

	return std::move( actions );
}

std::vector<std::shared_ptr<MidiAction>> MidiEventMap::getNoteActions( int nNote )
{
	QMutexLocker mx(&__mutex);

	std::vector<std::shared_ptr<MidiAction>> actions;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::Type::Note &&
			 ppEvent->getParameter() == nNote ) {
            actions.push_back( ppEvent->getMidiAction() );
		}
	}

	return std::move( actions );
}

std::vector<std::shared_ptr<MidiAction>> MidiEventMap::getCCActions( int nParameter ) {
	QMutexLocker mx(&__mutex);

	std::vector<std::shared_ptr<MidiAction>> actions;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::Type::CC &&
			 ppEvent->getParameter() == nParameter ) {
            actions.push_back( ppEvent->getMidiAction() );
		}
	}

	return std::move( actions );
}

std::vector<std::shared_ptr<MidiAction>> MidiEventMap::getPCActions(
)
{
	QMutexLocker mx(&__mutex);
	std::vector<std::shared_ptr<MidiAction>> actions;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::Type::PC ) {
            actions.push_back( ppEvent->getMidiAction() );
		}
	}

	return std::move( actions );
}


std::vector<int> MidiEventMap::findCCValuesByTypeAndParam1( MidiAction::Type type,
													   const QString& sParam1 ) {
	QMutexLocker mx(&__mutex);
	std::vector<int> values;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::Type::CC &&
			 ppEvent->getMidiAction()->getParameter1() == sParam1 ) {
			values.push_back( ppEvent->getParameter() );
		}
	}
	
	return std::move( values );
}

std::vector<int> MidiEventMap::findCCValuesByType( MidiAction::Type type ) {
	QMutexLocker mx(&__mutex);
	std::vector<int> values;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::Type::CC &&
			 ppEvent->getMidiAction()->getType() == type ) {
			values.push_back( ppEvent->getParameter() );
		}
	}

	return values;
}

std::vector<std::pair<H2Core::MidiEvent::Type,int>> MidiEventMap::getRegisteredMidiEvents( std::shared_ptr<MidiAction> pAction ) const {
	std::vector<std::pair<H2Core::MidiEvent::Type, int>> midiEvents;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getMidiAction()->isEquivalentTo( pAction ) ) {
			midiEvents.push_back(
				std::make_pair( ppEvent->getType(), ppEvent->getParameter() )
			);
		}
	}

	return midiEvents;
}

void MidiEventMap::removeRegisteredMidiEvents(
	std::shared_ptr<MidiAction> pAction
)
{
	QMutexLocker mx( &__mutex );

	for ( auto it = m_events.begin(); it != m_events.end(); ) {
		if ( *it != nullptr && ( *it )->getMidiAction() != nullptr &&
			 ( *it )->getMidiAction()->isEquivalentTo( pAction ) ) {
			m_events.erase( it );
		}
		else {
			++it;
		}
	}

	EventQueue::get_instance()->pushEvent(
		Event::Type::MidiEventMapChanged, 0
	);
}

QString MidiEventMap::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput =
			QString( "%1[MidiEventMap]\n" )
				.arg( sPrefix )
				.append( QString( "%1%2m_events:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppEvent : m_events ) {
			sOutput.append( QString( "%1%2%2%3\n" )
								.arg( sPrefix )
								.arg( s )
								.arg(
									ppEvent != nullptr ? ppEvent->toQString(
															 sPrefix + s, bShort
														 )
													   : "nullptr"
								) );
		}
	}
	else {
		sOutput = QString( "[MidiEventMap] m_events: [" );
		for ( const auto& ppEvent : m_events ) {
			sOutput.append( QString( "%1, " ).arg(
				ppEvent != nullptr ? ppEvent->toQString( "", bShort )
								   : "nullptr"
			) );
		}
		sOutput.append( "]" );
	}

	return sOutput;
}
};
