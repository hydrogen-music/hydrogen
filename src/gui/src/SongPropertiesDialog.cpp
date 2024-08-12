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

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	setWindowTitle( tr( "Song properties" ) );

	// Remove size constraints
	versionSpinBox->setFixedSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
	versionSpinBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	// Arbitrary high number.
	versionSpinBox->setMaximum( 300 );
	versionLabel->setText( pCommonStrings->getVersionDialog() );

	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();

	setupLicenseComboBox( licenseComboBox );

	if ( pSong != nullptr ) {
		versionSpinBox->setValue( pSong->getVersion() );
		songNameTxt->setText( pSong->getName() );

		authorTxt->setText( pSong->getAuthor() );
		notesTxt->append( pSong->getNotes() );

		licenseComboBox->setCurrentIndex(
			static_cast<int>( pSong->getLicense().getType() ) );
		licenseStringTxt->setText( pSong->getLicense().getLicenseString() );
		if ( pSong->getLicense().getType() == License::Unspecified ) {
			licenseStringTxt->hide();
		}
	}

	connect( licenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( licenseComboBoxChanged( int ) ) );

	authorLabel->setText( pCommonStrings->getAuthorDialog() );
	licenseLabel->setText( pCommonStrings->getLicenseDialog() );
	licenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	licenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip() );

	okBtn->setFixedFontSize( 12 );
	okBtn->setSize( QSize( 70, 23 ) );
	okBtn->setBorderRadius( 3 );
	okBtn->setType( Button::Type::Push );
	okBtn->setIsActive( true );
	cancelBtn->setFixedFontSize( 12 );
	cancelBtn->setSize( QSize( 70, 23 ) );
	cancelBtn->setBorderRadius( 3 );
	cancelBtn->setType( Button::Type::Push );
}

SongPropertiesDialog::~SongPropertiesDialog() {
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
	if ( versionSpinBox->value() != pSong->getVersion() ) {
		pSong->setVersion( versionSpinBox->value() );
		bIsModified = true;
	}
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
