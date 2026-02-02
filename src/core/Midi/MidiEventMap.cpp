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
#include "Midi/Midi.h"

#include <core/Basics/Event.h>
#include <core/EventQueue.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>

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

std::shared_ptr<MidiEventMap>
MidiEventMap::loadFrom( const H2Core::XMLNode& node, bool bSilent )
{
	auto pMidiEventMap = std::make_shared<MidiEventMap>();

	auto eventNode = node.firstChildElement( "midiEvent" );
	while ( !eventNode.isNull() ) {
		MidiAction::Type actionType = MidiAction::Type::Null;
		MidiEvent::Type eventType = MidiEvent::Type::Null;
		QString sParameter1, sParameter2, sParameter3;
		Midi::Parameter parameter = Midi::ParameterInvalid;
		const QString sNodeName = eventNode.firstChildElement().nodeName();
		if ( sNodeName == "mmcEvent" ) {
			actionType = MidiAction::parseType(
				eventNode.firstChildElement( "action" ).text()
			);
			sParameter1 = eventNode.firstChildElement( "parameter" ).text();
			sParameter2 = eventNode.firstChildElement( "parameter2" ).text();
			sParameter3 = eventNode.firstChildElement( "parameter3" ).text();

			eventType = MidiEvent::QStringToType(
				eventNode.firstChildElement( "mmcEvent" ).text()
			);
		}
		else if ( sNodeName == "noteEvent" ) {
			actionType = MidiAction::parseType(
				eventNode.firstChildElement( "action" ).text()
			);
			sParameter1 = eventNode.firstChildElement( "parameter" ).text();
			sParameter2 = eventNode.firstChildElement( "parameter2" ).text();
			sParameter3 = eventNode.firstChildElement( "parameter3" ).text();

			eventType = MidiEvent::Type::Note;
			parameter = Midi::parameterFromIntClamp(
				eventNode.firstChildElement( "eventParameter" ).text().toInt()
			);
		}
		else if ( sNodeName == "ccEvent" ) {
			actionType = MidiAction::parseType(
				eventNode.firstChildElement( "action" ).text()
			);
			sParameter1 = eventNode.firstChildElement( "parameter" ).text();
			sParameter2 = eventNode.firstChildElement( "parameter2" ).text();
			sParameter3 = eventNode.firstChildElement( "parameter3" ).text();
			eventType = MidiEvent::Type::CC;
			parameter = Midi::parameterFromIntClamp(
				eventNode.firstChildElement( "eventParameter" ).text().toInt()
			);
		}
		else if ( sNodeName == "pcEvent" ) {
			actionType = MidiAction::parseType(
				eventNode.firstChildElement( "action" ).text()
			);
			sParameter1 = eventNode.firstChildElement( "parameter" ).text();
			sParameter2 = eventNode.firstChildElement( "parameter2" ).text();
			sParameter3 = eventNode.firstChildElement( "parameter3" ).text();
			eventType = MidiEvent::Type::PC;
		}
		else {
			WARNINGLOG( QString( "Unknown MIDI map node [%1]" ).arg( sNodeName )
			);
		}

		if ( actionType != MidiAction::Type::Null &&
			 eventType != MidiEvent::Type::Null ) {
			auto pAction = MidiAction::fromQStrings(
				actionType, sParameter1, sParameter2, sParameter3
			);
			pMidiEventMap->registerEvent(
				eventType, parameter, pAction, nullptr
			);
		}
		else {
			WARNINGLOG( QString( "Invalid MIDI action. action [%1], parameter1 "
								 "[%2], parameter2 "
								 "[%3], parameter3 [%4], eventParameter [%5], "
								 "eventType [%6]]" )
							.arg( MidiAction::typeToQString( actionType ) )
							.arg( sParameter1 )
							.arg( sParameter2 )
							.arg( sParameter3 )
							.arg( static_cast<int>( parameter ) )
							.arg( MidiEvent::TypeToQString( eventType ) ) );
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

			QString sParameter1, sParameter2, sParameter3;
			ppEvent->getMidiAction()->toQStrings(
				&sParameter1, &sParameter2, &sParameter3
			);

			auto midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			switch ( ppEvent->getType() ) {
				case MidiEvent::Type::CC: {
					midiEventNode.write_string(
						"ccEvent", H2Core::MidiEvent::TypeToQString(
									   H2Core::MidiEvent::Type::CC
								   )
					);
					midiEventNode.write_int(
						"eventParameter",
						static_cast<int>( ppEvent->getParameter() )
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
						"eventParameter",
						static_cast<int>( ppEvent->getParameter() )
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
				"parameter", sParameter1
			);
			midiEventNode.write_string(
				"parameter2", sParameter2
			);
			midiEventNode.write_string(
				"parameter3", sParameter3
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
	Midi::Parameter parameter,
	std::shared_ptr<MidiAction> pAction,
	long* pEventId
)
{
	if ( pEventId != nullptr ) {
		*pEventId = Event::nInvalidId;
	}

	if ( pAction == nullptr || pAction->isNull() ||
		 type == H2Core::MidiEvent::Type::Null ) {
		ERRORLOG( "Invalid input" );
		return;
	}

	if ( ( type == MidiEvent::Type::Note || type == MidiEvent::Type::CC ) &&
		 parameter == Midi::ParameterInvalid ) {
		ERRORLOG( "Invalid parameter" );
		return;
	}

	{
		QMutexLocker mx( &__mutex );

		auto pEvent = std::make_shared<MidiEvent>();
		pEvent->setType( type );
		pEvent->setParameter( parameter );
		pEvent->setMidiAction( pAction );

		for ( const auto& ppEvent : m_events ) {
			if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
				 ppEvent->getType() == type &&
				 ppEvent->getParameter() == parameter &&
				 ppEvent->getMidiAction()->isEquivalentTo( pAction ) ) {
				WARNINGLOG(
					QString(
						"Event [%1] for MidiAction [%2] was already registered"
					)
						.arg( MidiEvent::TypeToQString( type ) )
						.arg( pAction->toQString() )
				);
				return;
			}
		}

		m_events.push_back( pEvent );
	}

	const auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen == nullptr ) {
        // The MidiEventMap is loaded during startup. We skip sending event,
        // since the EventQueue is not ready yet.
        return;
	}

	const auto nId = EventQueue::get_instance()->pushEvent(
		Event::Type::MidiEventMapChanged, 0
	);
	if ( pEventId != nullptr ) {
		*pEventId = nId;
	}
}

std::vector<std::shared_ptr<MidiAction>> MidiEventMap::getMMCActions(
	const QString& sEventString
)
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

std::vector<std::shared_ptr<MidiAction>> MidiEventMap::getNoteActions(
	Midi::Note note
)
{
	QMutexLocker mx( &__mutex );

	std::vector<std::shared_ptr<MidiAction>> actions;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::Type::Note &&
			 ppEvent->getParameter() == static_cast<Midi::Parameter>( note ) ) {
			actions.push_back( ppEvent->getMidiAction() );
		}
	}

	return std::move( actions );
}

std::vector<std::shared_ptr<MidiAction>> MidiEventMap::getCCActions(
	Midi::Parameter parameter
)
{
	QMutexLocker mx( &__mutex );

	std::vector<std::shared_ptr<MidiAction>> actions;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::Type::CC &&
			 ppEvent->getParameter() == parameter ) {
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

std::vector<Midi::Parameter> MidiEventMap::findCCParameters(
	MidiAction::Type type
)
{
	QMutexLocker mx( &__mutex );
	std::vector<Midi::Parameter> values;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::Type::CC &&
			 ppEvent->getMidiAction()->getType() == type ) {
			values.push_back( ppEvent->getParameter() );
		}
	}

	return std::move( values );
}

std::vector<Midi::Parameter>
MidiEventMap::findCCParameters( MidiAction::Type type, int nInstrument )
{
	QMutexLocker mx( &__mutex );
	std::vector<Midi::Parameter> values;

	for ( const auto& ppEvent : m_events ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 ppEvent->getType() == MidiEvent::Type::CC &&
			 ppEvent->getMidiAction()->getType() == type &&
			 ( ppEvent->getMidiAction()->getRequires() &
			   MidiAction::RequiresInstrument ) &&
			 ( ppEvent->getMidiAction()->getInstrument() == nInstrument ||
			   ( ppEvent->getMidiAction()->getInstrument() ==
					 MidiAction::nCurrentSelectionParameter &&
				 nInstrument ==
					 Hydrogen::get_instance()->getSelectedInstrumentNumber() )
			 ) ) {
			values.push_back( ppEvent->getParameter() );
		}
	}

	return std::move( values );
}

std::vector<std::pair<H2Core::MidiEvent::Type, Midi::Parameter>>
MidiEventMap::getRegisteredMidiEvents( std::shared_ptr<MidiAction> pAction
) const
{
	std::vector<std::pair<H2Core::MidiEvent::Type, Midi::Parameter>> midiEvents;

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

void MidiEventMap::removeRegisteredEvent(
	const MidiEvent::Type& type,
	Midi::Parameter parameter,
	std::shared_ptr<MidiAction> pAction,
    long* pEventId
)
{
	if ( pAction == nullptr ) {
        return;
	}

	if ( pEventId != nullptr ) {
		*pEventId = Event::nInvalidId;
	}

	bool bModified = false;
	{
		QMutexLocker mx( &__mutex );

		for ( auto it = m_events.begin(); it != m_events.end(); ) {
			if ( *it != nullptr && ( *it )->getType() == type &&
				 ( *it )->getParameter() == parameter &&
				 ( *it )->getMidiAction() != nullptr &&
				 ( *it )->getMidiAction()->isEquivalentTo( pAction ) ) {
				m_events.erase( it );
				bModified = true;
			}
			else {
				++it;
			}
		}
	}

	if ( bModified ) {
		const auto nId = EventQueue::get_instance()->pushEvent(
			Event::Type::MidiEventMapChanged, 0
		);
		if ( pEventId != nullptr ) {
			*pEventId = nId;
		}
	}
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
