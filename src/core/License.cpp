/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "core/License.h"

namespace H2Core {

License::License( const QString& sRawLicense ) : m_sRawLicense( sRawLicense )
{
	parse( sRawLicense );
}

License::License( const License* pOther ) :
	m_license( pOther->m_license ),
	m_sRawLicense( pOther->m_sRawLicense ) {
}

License::~License() {
}

void License::setType( LicenseType license ) {
	m_license = license;
	m_sRawLicense = toQString();
}

void License::parse( const QString& sRawLicense ) {

	m_sRawLicense = sRawLicense;

	QString sUp = sRawLicense.toUpper();

	if ( sRawLicense.isEmpty() ) {
		m_license = License::Unspecified;
	}
	else if ( sUp.contains( "CC" ) &&
		 sUp.contains( "BY" ) ) {
		if ( sUp.contains( "SA" ) ) {
			if ( sUp.contains( "NC" ) ) {
				m_license = License::CC_BY_NC_SA;
			}
			else {
				m_license = License::CC_BY_SA;
			}
		}
		else if ( sUp.contains( "ND" ) ) {
			if ( sUp.contains( "NC" ) ) {
				m_license = License::CC_BY_NC_ND;
			}
			else {
				m_license = License::CC_BY_ND;
			}
		}
		else {
			if ( sUp.contains( "NC" ) ) {
				m_license = License::CC_BY_NC;
			}
			else {
				m_license = License::CC_BY;
			}
		}
	}
	else if ( ( sUp.contains( "CC" ) &&
				sUp.contains( "0" ) ) ||
			  ( sUp.contains( "PUBLIC" ) &&
				sUp.contains( "DOMAIN" ) ) &&
			  ( sUp.contains( "NO" ) &&
				sUp.contains( "KNOWN" ) &&
				sUp.contains( "COPYRIGHT" ) ) ) {
		m_license = License::CC_0;
	}
	else if ( sUp.contains( "GPL" ) ) {
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

QString License::toQString( const QString& sPrefix, bool ) const {

	QString sType;
	
	switch( m_license ) {
	case License::CC_0:
		sType = "CC0";
		break;
	case License::CC_BY:
		sType = "CC BY";
		break;
	case License::CC_BY_NC:
		sType = "CC BY-NC";
		break;
	case License::CC_BY_SA:
		sType = "CC BY-SA";
		break;
	case License::CC_BY_NC_SA:
		sType = "CC BY-NC-SA";
		break;
	case License::CC_BY_ND:
		sType = "CC BY-ND";
		break;
	case License::CC_BY_NC_ND:
		sType = "CC BY-NC-ND";
		break;
	case License::GPL:
		sType = "GPL";
		break;
	case License::AllRightsReserved:
		sType = "All rights reserved";
		break;
	case License::Other:
		sType = m_sRawLicense;
		break;
	default:
		sType = "undefined license";
	}

	return QString( "%1%2" ).arg( sPrefix ).arg( sType );
}

bool License::isCopyleft() const {
	if ( m_license == License::GPL ||
		 m_license == License::CC_BY_SA ||
		 m_license == License::CC_BY_NC_SA ) {
		return true;
	}

	return false;
}

bool License::isCCBY() const {
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

};

/* vim: set softtabstop=4 noexpandtab: */
