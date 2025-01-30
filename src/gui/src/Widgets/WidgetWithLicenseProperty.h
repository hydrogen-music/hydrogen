/*
 * Hydrogen
 * Copyright(c) 2008-2025 The hydrogen development team <hydrogen-devel@lists.sourceforge.net>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef WIDGET_WITH_LICENSE_PROPERTY_H
#define WIDGET_WITH_LICENSE_PROPERTY_H

#include <QString>
#include <QComboBox>
#include <core/License.h>

/** Widget is affected by the "Font size" settings in the
 *	PreferencesDialog.
 *
 * To improve accessibility, three different font sizes,
 * H2Core::FontTheme::FontSize::Small,
 * H2Core::FontTheme::FontSize::Medium, and
 * H2Core::FontTheme::FontSize::Large, are available.
 */
/** \ingroup docGUI docWidgets*/
class WidgetWithLicenseProperty {
	Q_DECLARE_TR_FUNCTIONS( WidgetWithLicenseProperty )
protected:
	void setupLicenseComboBox( QComboBox* pComboBox ) {

		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_0),
							   H2Core::License::LicenseTypeToQString( H2Core::License::CC_0 )
							   .append( " (Public Domain)" ) );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY),
							   H2Core::License::LicenseTypeToQString( H2Core::License::CC_BY ) );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_NC),
							   H2Core::License::LicenseTypeToQString( H2Core::License::CC_BY_NC ) );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_SA),
							   H2Core::License::LicenseTypeToQString( H2Core::License::CC_BY_SA ) );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_NC_SA),
							   H2Core::License::LicenseTypeToQString( H2Core::License::CC_BY_NC_SA ) );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_ND),
							   H2Core::License::LicenseTypeToQString( H2Core::License::CC_BY_ND ) );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_NC_ND),
							   H2Core::License::LicenseTypeToQString( H2Core::License::CC_BY_NC_ND ) );
		pComboBox->insertItem( static_cast<int>(H2Core::License::GPL),
							   H2Core::License::LicenseTypeToQString( H2Core::License::GPL ) );
		pComboBox->insertItem( static_cast<int>(H2Core::License::AllRightsReserved),
							   H2Core::License::LicenseTypeToQString( H2Core::License::AllRightsReserved ) );

		/*: Label used for all license not directly supported in
		  Hydrogen's license combo box.*/ 
		pComboBox->insertItem( static_cast<int>(H2Core::License::Other),
							   tr( "Other" ) );
		/*: Label used if no license was specified.*/
		pComboBox->insertItem( static_cast<int>(H2Core::License::Unspecified),
							   tr( "Unspecified" ) );

		// Default value.
		pComboBox->setCurrentIndex( static_cast<int>(H2Core::License::Unspecified) );
	}
};

#endif
