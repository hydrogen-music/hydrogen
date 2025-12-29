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

#include "MidiEvent.h"

#include <core/Midi/MidiAction.h>

using namespace H2Core;

QString MidiEvent::TypeToQString( const Type& type ) {
	QString sType;

	switch ( type ) {
	case Type::Note:
		sType = "NOTE";
		break;
	case Type::CC:
		sType = "CC";
		break;
	case Type::PC:
		sType = "PROGRAM_CHANGE";
		break;
	case Type::MmcStop:
		sType = "MMC_STOP";
		break;
	case Type::MmcPlay:
		sType = "MMC_PLAY";
		break;
	case Type::MmcPause:
		sType = "MMC_PAUSE";
		break;
	case Type::MmcDeferredPlay:
		sType = "MMC_DEFERRED_PLAY";
		break;
	case Type::MmcRewind:
		sType = "MMC_REWIND";
		break;
	case Type::MmcFastForward:
		sType = "MMC_FAST_FORWARD";
		break;
	case Type::MmcRecordStrobe:
		sType = "MMC_RECORD_STROBE";
		break;
	case Type::MmcRecordExit:
		sType = "MMC_RECORD_EXIT";
		break;
	case Type::MmcRecordReady:
		sType = "MMC_RECORD_READY";
		break;
	case Type::Null:
	default:
		sType = "";
	}

	return std::move( sType );
}

MidiEvent::Type MidiEvent::QStringToType( const QString& sType ) {
	if ( sType == "NOTE" ) {
		return Type::Note;
	}
	else if ( sType == "CC" ) {
		return Type::CC;
	}
	else if ( sType == "PROGRAM_CHANGE" ) {
		return Type::PC;
	}
	else if ( sType == "MMC_STOP" ) {
		return Type::MmcStop;
	}
	else if ( sType == "MMC_PLAY" ) {
		return Type::MmcPlay;
	}
	else if ( sType == "MMC_PAUSE" ) {
		return Type::MmcPause;
	}
	else if ( sType == "MMC_DEFERRED_PLAY" ) {
		return Type::MmcDeferredPlay;
	}
	else if ( sType == "MMC_FAST_FORWARD" ) {
		return Type::MmcFastForward;
	}
	else if ( sType == "MMC_REWIND" ) {
		return Type::MmcRewind;
	}
	else if ( sType == "MMC_RECORD_STROBE" ) {
		return Type::MmcRecordStrobe;
	}
	else if ( sType == "MMC_RECORD_EXIT" ) {
		return Type::MmcRecordExit;
	}
	else if ( sType == "MMC_RECORD_READY" ) {
		return Type::MmcRecordReady;
	}
	else {
		return Type::Null;
	}
}

QStringList MidiEvent::getAllTypes() {
	QStringList typeList;
	typeList << TypeToQString( Type::Null )
			  << TypeToQString( Type::MmcPlay )
			  << TypeToQString( Type::MmcDeferredPlay )
			  << TypeToQString( Type::MmcStop )
			  << TypeToQString( Type::MmcFastForward )
			  << TypeToQString( Type::MmcRewind )
			  << TypeToQString( Type::MmcRecordStrobe )
			  << TypeToQString( Type::MmcRecordExit )
			  << TypeToQString( Type::MmcRecordReady )
			  << TypeToQString( Type::MmcPause )
			  << TypeToQString( Type::Note )
			  << TypeToQString( Type::CC )
			  << TypeToQString( Type::PC );

	return std::move( typeList );
}

MidiEvent::MidiEvent()
	: m_type( MidiEvent::Type::Null ),
	  m_nParameter( -1 ),
	  m_pMidiAction( nullptr )
{
}

MidiEvent::MidiEvent( const std::shared_ptr<MidiEvent> pOther )
	: m_type( pOther->m_type ), m_nParameter( pOther->m_nParameter )
{
	if ( pOther->m_pMidiAction != nullptr ) {
		m_pMidiAction = std::make_shared<MidiAction>( pOther->m_pMidiAction );
	}
}

QString MidiEvent::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;

	if ( !bShort ) {
		sOutput = QString( "%1[MidiEvent]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2m_type: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( MidiEvent::TypeToQString( m_type ) ) )
					  .append( QString( "%1%2m_nParameter: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( m_nParameter ) )
					  .append( QString( "%1%2m_pMidiAction: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg(
									   m_pMidiAction != nullptr
										   ? m_pMidiAction->toQString(
												 sPrefix + s, bShort
											 )
										   : "nullptr"
								   ) );
	}
	else {
		sOutput =
			QString( "[MidiEvent]" )
				.append( QString( " m_type: %1" )
							 .arg( MidiEvent::TypeToQString( m_type ) ) )
				.append( QString( ", m_nParameter: %1" ).arg( m_nParameter ) )
				.append( QString( ", m_pMidiAction: %1" )
							 .arg(
								 m_pMidiAction != nullptr
									 ? m_pMidiAction->toQString( "", bShort )
									 : "nullptr"
							 ) );
	}

	return sOutput;
}
