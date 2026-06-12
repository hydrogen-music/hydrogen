/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/SoundLibrary/DrumkitInfo.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/InstrumentInfo.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core {

DrumkitInfo::DrumkitInfo()
{
}

DrumkitInfo::~DrumkitInfo()
{
}

std::shared_ptr<DrumkitInfo> DrumkitInfo::from(
	std::shared_ptr<Drumkit> pDrumkit
)
{
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "invalid drumkit" );
		return nullptr;
	}

	auto pInfo = std::make_shared<DrumkitInfo>();
	pInfo->assignFrom( pDrumkit );
	return pInfo;
}

bool DrumkitInfo::load( const QString& sPath, Hydrogen* pHydrogen )
{
	// T1.5: make pHydrogen required and drop this fallback (ADR 0015).
	if ( pHydrogen == nullptr ) {
		pHydrogen = Hydrogen::get_instance();
	}
	const auto pDrumkit =
		pHydrogen->getSoundLibraryDatabase()->getDrumkit(
			sPath, false
		);
	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve drumkit [%1]" ).arg( sPath ) );
		return false;
	}

	assignFrom( pDrumkit );

    return true;
}

void DrumkitInfo::assignFrom( std::shared_ptr<Drumkit> pDrumkit )
{
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "invalid drumkit" );
		return;
	}

	m_type = SoundLibraryInfo::Type::Drumkit;
	m_sPath = pDrumkit->getPath();
	m_sAuthor = pDrumkit->getAuthor();
	m_context = pDrumkit->getContext();
	m_sName = pDrumkit->getName();
	m_sInfo = pDrumkit->getInfo();
	m_license = pDrumkit->getLicense();
	m_tags = pDrumkit->getTags();
	m_nVersion = pDrumkit->getVersion();

	m_instrumentInfos.clear();
	for ( const auto& ppInstrument : *pDrumkit->getInstruments() ) {
		if ( ppInstrument == nullptr ) {
			continue;
		}
		m_instrumentInfos.push_back( std::make_shared<InstrumentInfo>(
			this, ppInstrument->getName(), ppInstrument->getId(),
			ppInstrument->getType()
		) );
	}
}

QString DrumkitInfo::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;

	auto sOutput = SoundLibraryInfo::toQString( sPrefix, bShort );
	sOutput.replace( "SoundLibraryInfo", "DrumkitInfo" );

	QStringList infoStrings;
	for ( const auto& ppInfo : m_instrumentInfos ) {
		infoStrings << QString( "[id: %1, sType: %2]" )
						   .arg( static_cast<int>( ppInfo->getId() ) )
						   .arg( ppInfo->getType() );
	}

	if ( !bShort ) {
		sOutput.append(
			QString( "%1%2m_instrumentInfos:\n" ).arg( sPrefix ).arg( s )
		);
		for ( const auto& ssInfoString : infoStrings ) {
			sOutput.append( QString( "%1%2%2%3\n" )
								.arg( sPrefix )
								.arg( s )
								.arg( ssInfoString ) );
		}
	}
	else {
		sOutput.append( QString( ", m_instrumentInfos: [%1]" )
							.arg( infoStrings.join( ", " ) ) );
	}

	return sOutput;
}
};	// namespace H2Core
