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

#ifndef LICENSE_H
#define LICENSE_H

#include <core/Object.h>

#include <QDateTime>
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
 *
 * The license string contained in the XML file is parsed and Hydrogen
 * tries to map it to one of its supported #LicenseType.
 */
/** \ingroup docCore*/
class License : public H2Core::Object<License>
{
	H2_OBJECT(License)
public:

	License( const QString& sLicenseString = "", const QString& sCopyrightHolder = "" );

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

	static QString LicenseTypeToQString( const LicenseType& license );

	static QString getGPLLicenseNotice( const QString& sAuthor );

	void parse( const QString& sLicenseString );
	const QString& getLicenseString() const;
	const QString& getCopyrightHolder() const;
	void setCopyrightHolder( const QString& sCopyrightHolder );

		/** Whether a dedicated license string or copyright holder was set. */
		bool isEmpty() const;

	bool isCopyleft() const;
	bool hasAttribution() const;

	const LicenseType& getType() const;
	void setType( const LicenseType& license );

	bool operator==( const License& other ) const {
		if ( m_license == other.m_license &&
			 m_sCopyrightHolder == other.m_sCopyrightHolder ) {
			if ( m_license == License::Other &&
				 m_sLicenseString != other.m_sLicenseString ) {
				return false;
			}
			else {
				return true;
			}
		}

		return false;
	}

	bool operator!=( const License& other ) const {
		if ( m_license == other.m_license &&
			 m_sCopyrightHolder == other.m_sCopyrightHolder ) {
			if ( m_license == License::Other &&
				 m_sLicenseString != other.m_sLicenseString ) {
				return true;
			}
			else {
				return false;
			}
		}

		return true;
	}
	
	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	LicenseType m_license;
	QString m_sLicenseString;
	/** This variable will not be written to disk. It just serves as a
	 * temporary vessel for the e.g. drumkit's or song's
	 * author. Storing it would lead to additional effort for keeping
	 * it in sync with the former.
	 */
	QString m_sCopyrightHolder;
};

inline const License::LicenseType& License::getType() const {
	return m_license;
}
inline const QString& License::getLicenseString() const {
	return m_sLicenseString;
}
inline const QString& License::getCopyrightHolder() const {
	return m_sCopyrightHolder;
}
inline void License::setCopyrightHolder( const QString& sCopyrightHolder ) {
	m_sCopyrightHolder = sCopyrightHolder;
}
inline QString License::LicenseTypeToQString( const License::LicenseType& license ) {

	QString sType;
	
	switch( license ) {
	case License::CC_0:
		return "CC0";
		
	case License::CC_BY:
		return "CC BY";
		
	case License::CC_BY_NC:
		return "CC BY-NC";
		
	case License::CC_BY_SA:
		return "CC BY-SA";
		
	case License::CC_BY_NC_SA:
		return "CC BY-NC-SA";
		
	case License::CC_BY_ND:
		return "CC BY-ND";
		
	case License::CC_BY_NC_ND:
		return "CC BY-NC-ND";
		
	case License::GPL:
		return "GPL";
		
	case License::AllRightsReserved:
		return "All rights reserved";
		
	case License::Other:
		return "Other";
		
	default:
		return "undefined license";
	}
}
inline QString License::getGPLLicenseNotice( const QString& sAuthor ) {
	return QString("Copyright (C) %1  %2\n\
\n\
    This program is free software: you can redistribute it and/or modify\n\
    it under the terms of the GNU General Public License as published by\n\
    the Free Software Foundation, either version 3 of the License, or\n\
    (at your option) any later version.\n\
\n\
    This program is distributed in the hope that it will be useful,\n\
    but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
    GNU General Public License for more details.\n\
\n\
    You should have received a copy of the GNU General Public License\n\
    along with this program.  If not, see <https://www.gnu.org/licenses/>." )
	.arg( QDateTime::currentDateTime().toString( "yyyy" ) )
	.arg( sAuthor );
}
};

#endif // LICENSE_H

/* vim: set softtabstop=4 noexpandtab: */
