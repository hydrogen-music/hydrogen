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

#include "MidiAction.h"

using namespace H2Core;

MidiAction::MidiAction( const QString& sType ) {
	m_sType = sType;
	m_sParameter1 = "0";
	m_sParameter2 = "0";
	m_sParameter3 = "0";
	m_sValue = "0";
}

MidiAction::MidiAction( const std::shared_ptr<MidiAction> pOther ) {
       m_sType = pOther->m_sType;
       m_sParameter1 = pOther->m_sParameter1;
       m_sParameter2 = pOther->m_sParameter2;
       m_sParameter3 = pOther->m_sParameter3;
       m_sValue = pOther->m_sValue;
}

bool MidiAction::isNull() const {
	return m_sType == MidiAction::getNullMidiActionType();
}

bool MidiAction::isEquivalentTo( const std::shared_ptr<MidiAction> pOther ) const {
	if ( pOther == nullptr ) {
		return false;
	}
	
	return ( m_sType == pOther->m_sType &&
			 m_sParameter1 == pOther->m_sParameter1 &&
			 m_sParameter2 == pOther->m_sParameter2 &&
			 m_sParameter3 == pOther->m_sParameter3 );
}

QString MidiAction::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[MidiAction]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_sType: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sType ) )
			.append( QString( "%1%2m_sValue: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sValue ) )
			.append( QString( "%1%2m_sParameter1: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sParameter1 ) )
			.append( QString( "%1%2m_sParameter2: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sParameter2 ) )
			.append( QString( "%1%2m_sParameter3: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sParameter3 ) );
	} else {
		sOutput = QString( "[MidiAction]" )
			.append( QString( " m_sType: %1" ).arg( m_sType ) )
			.append( QString( ", m_sValue: %1" ).arg( m_sValue ) )
			.append( QString( ", m_sParameter1: %1" ).arg( m_sParameter1 ) )
			.append( QString( ", m_sParameter2: %1" ).arg( m_sParameter2 ) )
			.append( QString( ", m_sParameter3: %1" ).arg( m_sParameter3 ) );
	}
	
	return sOutput;
}
