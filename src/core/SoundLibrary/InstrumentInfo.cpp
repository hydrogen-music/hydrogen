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

#include <core/SoundLibrary/InstrumentInfo.h>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/SoundLibrary/DrumkitInfo.h>

namespace H2Core {

InstrumentInfo::InstrumentInfo(
	DrumkitInfo* pInfo,
	const QString& sName,
	Instrument::Id id,
	Instrument::Type sType
)
	: m_id( id ), m_sType( sType )
{
	m_sName = sName;
	m_type = SoundLibraryInfo::Type::Instrument;

	if ( pInfo != nullptr ) {
		m_sURL = pInfo->getUrl();
		m_sInfo = pInfo->getInfo();
		m_sAuthor = pInfo->getAuthor();
		m_license = pInfo->getLicense();
		m_sPath = pInfo->getPath();
		m_sLabel = pInfo->getLabel();
		m_context = pInfo->getContext();
	}
}

InstrumentInfo::~InstrumentInfo()
{
}

QString InstrumentInfo::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;

	auto sOutput = SoundLibraryInfo::toQString( sPrefix, bShort );
	sOutput.replace( "SoundLibraryInfo", "InstrumentInfo" );

	if ( !bShort ) {
		sOutput.append( QString( "%1%2m_id: %3\n" )
							.arg( static_cast<int>( m_id ) )
							.append( QString( "%1%2m_sType: %3" ).arg( m_sType )
							) );
	}
	else {
		sOutput.append( QString( ", m_id: %1" )
							.arg( static_cast<int>( m_id ) )
							.append( QString( ", m_sType: %1" ).arg( m_sType ) )
		);
	}
	return sOutput;
}
};	// namespace H2Core
