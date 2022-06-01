/*
 * Hydrogen
 * Copyright (C) 2008-2021 The hydrogen development team <hydrogen-devel@lists.sourceforge.net>
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
#include <QObject>
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

		H2Core::License license;
		license.setType( H2Core::License::CC_0 );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_0),
							   license.toQString()
							   .append( " (Public Domain)" ) );
	
		license.setType( H2Core::License::CC_BY );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY),
							   license.toQString() );
	
		license.setType( H2Core::License::CC_BY_NC );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_NC),
							   license.toQString() );
	
		license.setType( H2Core::License::CC_BY_SA );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_SA),
							   license.toQString() );
	
		license.setType( H2Core::License::CC_BY_NC_SA );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_NC_SA),
							   license.toQString() );
	
		license.setType( H2Core::License::CC_BY_ND );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_ND),
							   license.toQString() );
	
		license.setType( H2Core::License::CC_BY_NC_ND );
		pComboBox->insertItem( static_cast<int>(H2Core::License::CC_BY_NC_ND),
							   license.toQString() );
	
		license.setType( H2Core::License::GPL );
		pComboBox->insertItem( static_cast<int>(H2Core::License::GPL),
							   license.toQString() );
	
		license.setType( H2Core::License::AllRightsReserved );
		pComboBox->insertItem( static_cast<int>(H2Core::License::AllRightsReserved),
							   license.toQString() );
	
		pComboBox->insertItem( static_cast<int>(H2Core::License::Other),
							   tr( "Other" ) );
	
		license.setType( H2Core::License::Unspecified );
		pComboBox->insertItem( static_cast<int>(H2Core::License::Unspecified),
							   license.toQString() );

		// Default value.
		pComboBox->setCurrentIndex( static_cast<int>(H2Core::License::Unspecified) );
	}
};

#endif
