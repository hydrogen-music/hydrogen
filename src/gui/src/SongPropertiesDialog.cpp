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

#include "CommonStrings.h"
#include "SongPropertiesDialog.h"

#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/License.h>

#include <QPixmap>

using namespace H2Core;

SongPropertiesDialog::SongPropertiesDialog(QWidget* parent)
 : QDialog(parent)
{
	setupUi( this );
	
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	adjustSize();
	setMaximumSize( width(), height() );
	setMinimumSize( width(), height() );

	setWindowTitle( tr( "Song properties" ) );

	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	songNameTxt->setText( pSong->getName() );

	authorTxt->setText( pSong->getAuthor() );
	notesTxt->append( pSong->getNotes() );
	connect( licenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( licenseComboBoxChanged( int ) ) );

	setupLicenseComboBox( licenseComboBox );
	
	licenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	licenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip() );
	
	licenseComboBox->setCurrentIndex( static_cast<int>( pSong->getLicense().getType() ) );
	licenseStringTxt->setText( pSong->getLicense().getLicenseString() );
	if ( pSong->getLicense().getType() == License::Unspecified ) {
		licenseStringTxt->hide();
	}
}



SongPropertiesDialog::~SongPropertiesDialog()
{
}

void SongPropertiesDialog::licenseComboBoxChanged( int ) {

	licenseStringTxt->setText( License::LicenseTypeToQString(
		static_cast<License::LicenseType>( licenseComboBox->currentIndex() ) ) );

	if ( licenseComboBox->currentIndex() == static_cast<int>( License::Unspecified ) ) {
		licenseStringTxt->hide();
	}
	else {
		licenseStringTxt->show();
	}
}

void SongPropertiesDialog::on_cancelBtn_clicked()
{
	reject();
}

void SongPropertiesDialog::on_okBtn_clicked()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	bool bIsModified = false;
	if ( songNameTxt->text() != pSong->getName() ) {
		pSong->setName( songNameTxt->text() );
		bIsModified = true;
	}
	if ( pSong->getAuthor() != authorTxt->text() ) {
		pSong->setAuthor( authorTxt->text() );
		bIsModified = true;
	}
	if ( pSong->getNotes() != notesTxt->toPlainText() ) {
		pSong->setNotes( notesTxt->toPlainText() );
		bIsModified = true;
	}

	QString sNewLicenseString( licenseStringTxt->text() );
	if ( licenseComboBox->currentIndex() ==
		 static_cast<int>(License::Unspecified) ) {
		sNewLicenseString = "";
	}
	License newLicense( sNewLicenseString );
	if ( pSong->getLicense() != newLicense ) {
		pSong->setLicense( newLicense );
		bIsModified = true;
	}

	if ( bIsModified ) {
		pHydrogen->setIsModified( true );
	}

	accept();
}
