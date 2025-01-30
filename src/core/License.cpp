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

#include <QRegularExpression>
#include "core/License.h"

namespace H2Core {

	License::License( const QString& sLicenseString, const QString& sCopyrightHolder ) :
	m_sLicenseString( sLicenseString ),
	m_sCopyrightHolder( sCopyrightHolder )
{
	parse( sLicenseString );
}

License::License( const License* pOther ) :
	m_license( pOther->m_license ),
	m_sLicenseString( pOther->m_sLicenseString ),
	m_sCopyrightHolder( pOther->m_sCopyrightHolder ) {
}

License::~License() {
}

void License::setType( LicenseType license ) {
	m_license = license;
	m_sLicenseString = LicenseTypeToQString( license );
}

void License::parse( const QString& sLicenseString ) {

	m_sLicenseString = sLicenseString;

	const QString sUp = sLicenseString.toUpper();

	if ( sUp.isEmpty() ||
		 sUp == "UNDEFINED LICENSE" ||
		 sUp == "UNKNOWN LICENSE" ) {
		m_sLicenseString = "undefined license";
		m_license = License::Unspecified;
	}
	else if ( ( sUp.contains( "CC" ) ||
				( sUp.contains( "CREATIVE" ) &&
				  sUp.contains( "COMMONS" ) ) ) &&
			  ( sUp.contains( "BY" ) ||
				sUp.contains( "ATTRIBUTION" ) ) ) {
		if ( sUp.contains( "SA" ) ||
			 ( sUp.contains( "SHARE" ) &&
			   sUp.contains( "ALIKE" ) ) ) {
			if ( sUp.contains( "NC" ) ||
				 ( sUp.contains( "NON" ) &&
				   sUp.contains( "COMMERCIAL" ) ) ) {
				m_license = License::CC_BY_NC_SA;
			}
			else {
				m_license = License::CC_BY_SA;
			}
		}
		else if ( sUp.contains( "ND" ) ||
				  ( sUp.contains( "NO" ) &&
					sUp.contains( "DERIVATIVES" ) ) ) {
			if ( sUp.contains( "NC" ) ||
				 ( sUp.contains( "NON" ) &&
				   sUp.contains( "COMMERCIAL" ) ) ) {
				m_license = License::CC_BY_NC_ND;
			}
			else {
				m_license = License::CC_BY_ND;
			}
		}
		else {
			if ( sUp.contains( "NC" ) ||
				 ( sUp.contains( "NON" ) &&
				   sUp.contains( "COMMERCIAL" ) ) ) {
				m_license = License::CC_BY_NC;
			}
			else {
				m_license = License::CC_BY;
			}
		}
	}
	else if ( ( ( sUp.contains( "CC" ) ||
				  ( sUp.contains( "CREATIVE" ) &&
					sUp.contains( "COMMONS" ) ) ) &&
				( sUp.contains( "0" ) ||
				  sUp.contains( "ZERO" ) ) ) ||
			  ( sUp.contains( "PUBLIC" ) &&
				sUp.contains( "DOMAIN" ) ) &&
			  ( sUp.contains( "NO" ) &&
				sUp.contains( "KNOWN" ) &&
				sUp.contains( "COPYRIGHT" ) ) ) {
		m_license = License::CC_0;
	}
	else if ( sUp.contains( "GPL" ) ||
			  ( sUp.contains( "GENERAL" ) &&
				sUp.contains( "PUBLIC" ) &&
				sUp.contains( "LICENSE" ) ) ) {
		m_license = License::GPL;
	}
	else if ( sUp.contains( "ALL" ) &&
			  sUp.contains( "RIGHTS" ) &&
			  sUp.contains( "RESERVED" ) ) {
		m_license = License::AllRightsReserved;
	}
	else {
		m_license = License::Other;
	}
}

bool License::isCopyleft() const {
	if ( m_license == License::GPL ||
		 m_license == License::CC_BY_SA ||
		 m_license == License::CC_BY_NC_SA ) {
		return true;
	}

	return false;
}

bool License::hasAttribution() const {
	if ( m_license == License::CC_BY ||
		 m_license == License::CC_BY_NC ||
		 m_license == License::CC_BY_SA ||
		 m_license == License::CC_BY_NC_SA ||
		 m_license == License::CC_BY_ND ||
		 m_license == License::CC_BY_NC_ND ) {
		return true;
	} 

	return false;
}
	
QString License::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[License]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_license: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( LicenseTypeToQString( m_license ) ) )
			.append( QString( "%1%2m_sLicenseString: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sLicenseString ) )
			.append( QString( "%1%2m_sCopyrightHolder: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sCopyrightHolder ) );
	}
	else {
		sOutput = QString( "[License]" )
			.append( QString( " m_license: %1" )
					 .arg( LicenseTypeToQString( m_license ) ) )
			.append( QString( ", m_sLicenseString: %1" )
					 .arg( m_sLicenseString ) )
			.append( QString( ", m_sCopyrightHolder: %1" )
					 .arg( m_sCopyrightHolder ) )
			.append( "\n" );
	}
	
	return sOutput;
}
};

/* vim: set softtabstop=4 noexpandtab: */
