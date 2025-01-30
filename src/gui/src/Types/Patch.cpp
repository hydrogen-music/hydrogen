/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "Patch.h"

#include <core/Basics/Instrument.h>

QString Patch::Mapping::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Mapping]\n" ).arg( sPrefix )
			.append( QString( "%1%2sOldPatternType: %3" ).arg( sPrefix ).arg( s )
					 .arg( sOldPatternType ) )
			.append( QString( "%1%2nNewInstrumentId: %3" ).arg( sPrefix ).arg( s )
					 .arg( nNewInstrumentId ) )
			.append( QString( "%1%2affectedNotes: [" ).arg( sPrefix ).arg( s ) );
		for ( const auto ppNote : affectedNotes ) {
			if ( ppNote != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ppNote->toQString(
					sPrefix + s + s, bShort ) ) );
			}
		}
		sOutput.append( QString( "%1%2]" ).arg( sPrefix ).arg( s ) );
	}
	else {
		sOutput = QString( "[Mapping]" )
			.append( QString( " sOldPatternType: %1" ).arg( sOldPatternType ) )
			.append( QString( ", nNewInstrumentId: %1" ).arg( nNewInstrumentId ) )
			.append( ", affectedNotes: [" );
		for ( const auto ppNote : affectedNotes ) {
			if ( ppNote != nullptr ) {
				sOutput.append( QString( "[type: %1, pos: %2, instrument: %3] " )
								.arg( ppNote->getType() )
								.arg( ppNote->getPosition() )
								.arg( ppNote->getInstrument() != nullptr ?
									  ppNote->getInstrument()->get_name() :
									  "nullptr" ) );
			}
		}
		sOutput.append( "]" );
	}
	return sOutput;
}

Patch::Patch() {}

Patch::~Patch() {
}

void Patch::addMapping( const QString& sOldPatternType, int nNewInstrumentId,
						std::vector< std::shared_ptr<H2Core::Note> > afftectedNotes ) {
	Mapping mapping;
	mapping.sOldPatternType = sOldPatternType;
	mapping.nNewInstrumentId = nNewInstrumentId;
	mapping.affectedNotes = afftectedNotes;

	m_mappings.push_back( mapping );
}

QString Patch::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Patch]\n" ).arg( sPrefix );
		for ( const auto& mmapping : m_mappings ) {
			sOutput.append( QString( "%1" ).arg(
								mmapping.toQString( sPrefix + s + s, bShort ) ) );
		}
	}
	else {
		sOutput = QString( "[Patch] " );
		for ( const auto& mmapping : m_mappings ) {
			sOutput.append( QString( "[%1], " ).arg(
								mmapping.toQString( "", bShort ) ) );
		}
	}
	return sOutput;
}
