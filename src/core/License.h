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

#ifndef LICENSE_H
#define LICENSE_H

#include <core/Object.h>

#include <QString>

namespace H2Core {

/**
 * Wrapper class to help Hydrogen deal with the license information
 * specified in a drumkit.
 *
 * In order support the user in assigning the right license when
 * creating a new drumkit or song, Hydrogen will keep track of the
 * license information of each individual sample and checks whether
 * any copyleft ones are included or credit must be given due to a CC
 * BY* license.
 */
/** \ingroup docCore docDebugging*/
class License : public H2Core::Object<License>
{
	H2_OBJECT(License)
public:

	License( const QString& sRawLicense = "" );
	License( const License* pOther );
	~License();

	/** A couple of recognized licenses. The ones supplied by Creative
		Commons are the most desired ones.*/
	enum LicenseType {
		CC_0 = 0,
		CC_BY = 1,
		CC_BY_NC = 2,
		CC_BY_SA = 3,
		CC_BY_NC_SA = 4,
		CC_BY_ND = 5,
		CC_BY_NC_ND = 6,
		/** Not a desirable license for audio data but introduced here
			specifically since it is already used by a number of kits.*/
		GPL = 7,
		/** User decides with withhold all rights. */
		AllRightsReserved = 8,
		/** All other licenses not specified above.*/
		Other = 9,
		/** No license set yet.*/
		Unspecified = 10
	};

	void parse( const QString& sRawLicense );
	
	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

	bool isCopyleft() const;
	bool isCCBY() const;

	LicenseType getType() const;
	void setType( LicenseType license );

	bool operator==( const License& other ) const {
		if ( m_license == other.m_license ) {
			if ( m_license == License::Other &&
				 m_sRawLicense != other.m_sRawLicense ) {
				return false;
			}
			else {
				return true;
			}
		}

		return false;
	}

	bool operator!=( const License& other ) const {
		if ( m_license == other.m_license ) {
			if ( m_license == License::Other &&
				 m_sRawLicense != other.m_sRawLicense ) {
				return true;
			}
			else {
				return false;
			}
		}

		return true;
	}

private:
	LicenseType m_license;
	QString m_sRawLicense;
};

inline License::LicenseType License::getType() const {
	return m_license;
}
};

#endif // LICENSE_H

/* vim: set softtabstop=4 noexpandtab: */
