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
	licenseComboBox->setCurrentIndex( static_cast<int>( pSong->getLicense().getType() ) );
	if ( pSong->getLicense().getType() == License::Other ) {
		licenseTxt->setText( pSong->getLicense().toQString() );
	}
	else {
		licenseTxt->hide();
	}
}



SongPropertiesDialog::~SongPropertiesDialog()
{
}

void SongPropertiesDialog::licenseComboBoxChanged( int ) {

	if ( licenseComboBox->currentIndex() == static_cast<int>(License::Other) ) {
		licenseTxt->show();
	}
	else {
		licenseTxt->hide();
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
	if ( static_cast<int>(pSong->getLicense().getType()) !=
		 licenseComboBox->currentIndex() ) {

		auto license = pSong->getLicense();
		
		if ( licenseComboBox->currentIndex() ==
			 static_cast<int>(License::Other) ) {
			license.parse( licenseTxt->text() );
		}
		else {
			license.setType( static_cast<License::LicenseType>(licenseComboBox->currentIndex()) );
		}
		pSong->setLicense( license );
		bIsModified = true;
	}

	if ( bIsModified ) {
		pHydrogen->setIsModified( true );
	}

	accept();
}
