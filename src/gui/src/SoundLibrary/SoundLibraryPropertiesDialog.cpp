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

#include <QtGui>
#include <QtWidgets>

#include "../HydrogenApp.h"
#include "../CommonStrings.h"

#include "SoundLibraryPropertiesDialog.h"
#include "../InstrumentRack.h"
#include "SoundLibraryPanel.h"
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

namespace H2Core
{

SoundLibraryPropertiesDialog::SoundLibraryPropertiesDialog( QWidget* pParent, Drumkit *pDrumkitInfo, Drumkit *pPreDrumkit )
 : QDialog( pParent )
 , m_pDrumkitInfo( pDrumkitInfo )
 , m_pPreDrumkitInfo( pPreDrumkit )
{
	setupUi( this );
	
	setWindowTitle( tr( "SoundLibrary Properties" ) );
	adjustSize();
	setMinimumSize( width(), height() );

	setupLicenseComboBox( licenseComboBox );
	connect( licenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( licenseComboBoxChanged( int ) ) );
	setupLicenseComboBox( imageLicenseComboBox );
	connect( imageLicenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( imageLicenseComboBoxChanged( int ) ) );

	//display the current drumkit infos into the qlineedit
	if ( pDrumkitInfo != nullptr ){
		nameTxt->setText( QString( pDrumkitInfo->get_name() ) );
		authorTxt->setText( QString( pDrumkitInfo->get_author() ) );
		infoTxt->append( QString( pDrumkitInfo->get_info() ) );

		License license = pDrumkitInfo->get_license();
		licenseComboBox->setCurrentIndex( static_cast<int>( license.getType() ) );
		if ( license.getType() == License::Other ) {
			licenseTxt->setText( license.toQString() );
		}
		else {
			licenseTxt->hide();
		}
	
		imageText->setText( QString( pDrumkitInfo->get_image() ) );

		License imageLicense = pDrumkitInfo->get_image_license();
		imageLicenseComboBox->setCurrentIndex( static_cast<int>( imageLicense.getType() ) );
		if ( imageLicense.getType() == License::Other ) {
			imageLicenseText->setText( imageLicense.toQString() );
			// Licence with attribution is often too long...
			imageLicenseText->setToolTip( imageLicense.toQString() );
		
		}
		else {
			imageLicenseText->hide();
		}

		contentTable->setColumnCount( 4 );
		contentTable->setHorizontalHeaderLabels( QStringList() <<
														 tr( "Instrument" ) <<
														 tr( "Component" ) <<
														 tr( "Sample" ) <<
														 tr( "License" ) );
		contentTable->verticalHeader()->hide();
		contentTable->horizontalHeader()->setStretchLastSection( true );

		contentTable->setColumnWidth( 0, 170 );
		contentTable->setColumnWidth( 1, 80 );
		contentTable->setColumnWidth( 2, 210 );
		
		updateLicenseTable();
	}
}


SoundLibraryPropertiesDialog::~SoundLibraryPropertiesDialog()
{
	INFOLOG( "DESTROY" );

}

	
void SoundLibraryPropertiesDialog::licenseComboBoxChanged( int ) {

	if ( licenseComboBox->currentIndex() == static_cast<int>(License::Other) ) {
		licenseTxt->show();
	}
	else {
		licenseTxt->hide();
	}

	updateLicenseTable();
}


/// On showing the dialog (after layout sizes have been applied), load the drumkit image if any.
void SoundLibraryPropertiesDialog::showEvent( QShowEvent *e )
{
	if ( m_pDrumkitInfo != nullptr &&
		 ! m_pDrumkitInfo->get_image().isEmpty() ) {
		QString sImage = m_pDrumkitInfo->get_path() + "/" + m_pDrumkitInfo->get_image();
		updateImage( sImage );
	}
	else {
		drumkitImageLabel->hide();
	}
}

void SoundLibraryPropertiesDialog::updateLicenseTable() {
	auto pPref = H2Core::Preferences::get_instance();
	
	std::vector<QStringList> content = m_pDrumkitInfo->summarizeContent();

	if ( content.size() > 0 ) {
		contentTable->show();
		contentLabel->show();
		contentTable->setRowCount( content.size() );

		int nFirstMismatchRow = -1;

		for ( int ii = 0; ii < content.size(); ++ii ) {
			QLineEdit* pInstrumentItem = new QLineEdit( content[ ii ][ 0 ] );
			pInstrumentItem->setEnabled( false );
			pInstrumentItem->setToolTip( content[ ii ][ 0 ] );
			QLineEdit* pComponentItem = new QLineEdit( content[ ii ][ 1 ] );
			pComponentItem->setEnabled( false );
			pComponentItem->setToolTip( content[ ii ][ 1 ] );
			QLineEdit* pSampleItem = new QLineEdit( content[ ii ][ 2 ] );
			pSampleItem->setEnabled( false );
			pSampleItem->setToolTip( content[ ii ][ 2 ] );
			QLineEdit* pLicenseItem = new QLineEdit( content[ ii ][ 3 ] );
			pLicenseItem->setEnabled( false );
			pLicenseItem->setToolTip( content[ ii ][ 3 ] );

			// In case of a license mismatch we highlight the row
			if ( content[ ii ][ 3 ] != m_pDrumkitInfo->get_license().toQString() ) {
				QString sRed = QString( "color: %1;" )
					.arg( pPref->getColorTheme()->m_buttonRedColor.name() );
				pInstrumentItem->setStyleSheet( sRed );
				pComponentItem->setStyleSheet( sRed );
				pSampleItem->setStyleSheet( sRed );
				pLicenseItem->setStyleSheet( sRed );

				if ( nFirstMismatchRow == -1 ) {
					nFirstMismatchRow = ii;
				}
			}

			contentTable->setCellWidget( ii, 0, pInstrumentItem );
			contentTable->setCellWidget( ii, 1, pComponentItem );
			contentTable->setCellWidget( ii, 2, pSampleItem );
			contentTable->setCellWidget( ii, 3, pLicenseItem );
		}

		// In case of a mismatch scroll into view
		if ( nFirstMismatchRow != -1 ) {
			contentTable->showRow( nFirstMismatchRow );
		}
	}
	else {
		contentTable->hide();
		contentLabel->hide();
	}
}
	
void SoundLibraryPropertiesDialog::imageLicenseComboBoxChanged( int ) {

	if ( imageLicenseComboBox->currentIndex() ==
		 static_cast<int>(License::Other) ) {
		imageLicenseText->show();
	}
	else {
		imageLicenseText->hide();
	}
}

void SoundLibraryPropertiesDialog::updateImage( QString& filename )
{
	QPixmap *pPixmap = new QPixmap ( filename );

	// Check whether the loading worked.
	if ( pPixmap->isNull() ) {
		ERRORLOG( QString( "Unable to load pixmap from [%1]" ).arg( filename ) );
		drumkitImageLabel->hide();
		return;
	}
	
	// scale the image down to fit if required
	int x = (int) drumkitImageLabel->size().width();
	int y = drumkitImageLabel->size().height();
	float labelAspect = (float) x / y;
	float imageAspect = (float) pPixmap->width() / pPixmap->height();

	if ( ( x < pPixmap->width() ) || ( y < pPixmap->height() ) )
	{
		if ( labelAspect >= imageAspect )
		{
			// image is taller or the same as label frame
			*pPixmap = pPixmap->scaledToHeight( y );
		}
		else
		{
			// image is wider than label frame
			*pPixmap = pPixmap->scaledToWidth( x );
		}
	}
	drumkitImageLabel->setPixmap(*pPixmap);
	drumkitImageLabel->show();

}

void SoundLibraryPropertiesDialog::on_imageBrowsePushButton_clicked()
{
	// Try to get the drumkit directory and open file browser
	QString drumkitDir = m_pDrumkitInfo->get_path();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), drumkitDir, tr("Image Files (*.png *.jpg *.jpeg)"));

	// If cancel was clicked just abort
	if ( fileName == nullptr )
	{
		return;
	}

	// If this file is in different directory copy it here
	
	QFile file( fileName );
	QFileInfo fileInfo(file.fileName());

	if ( fileInfo.dir().path() != drumkitDir )
	{
		INFOLOG("Copying " + fileName + " to " + drumkitDir.toLocal8Bit() );
		if ( !QFile::copy( fileName, drumkitDir + "/" + fileInfo.fileName() ))
		{
			WARNINGLOG( "Could not copy " + fileInfo.fileName() + " to " + drumkitDir );
		}

	}
	QString filename(fileInfo.fileName());
	imageText->setText( filename );
	m_pDrumkitInfo->set_image( filename );
	updateImage( fileName );
}

void SoundLibraryPropertiesDialog::on_saveBtn_clicked()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	bool reload = false;

	if ( saveChanges_checkBox->isChecked() ){
		//test if the drumkit is loaded
		if ( m_pDrumkitInfo->get_name() != m_pPreDrumkitInfo->get_name() ||
			 m_pDrumkitInfo->isUserDrumkit() != m_pPreDrumkitInfo->isUserDrumkit() ){
			QMessageBox::information( this, "Hydrogen", tr ( "This is not possible, you can only save changes inside instruments to the current loaded sound library"));
			saveChanges_checkBox->setChecked( false );
			return;
		}
		reload = true;
	}

	//load the selected drumkit to save it correct.... later the old drumkit will be reloaded
	if ( m_pDrumkitInfo != nullptr && ( !saveChanges_checkBox->isChecked() ) ){
		if ( m_pPreDrumkitInfo->get_name() != m_pDrumkitInfo->get_name() ||
			 m_pPreDrumkitInfo->isUserDrumkit() != m_pDrumkitInfo->isUserDrumkit() ){
			Hydrogen::get_instance()->loadDrumkit( m_pDrumkitInfo );
		}
	}

	// Check the drumkit name. if the name is a new one, one qmessagebox with question "are you sure" will displayed.
	if ( m_pDrumkitInfo != nullptr && nameTxt->text() != m_pDrumkitInfo->get_name()  ){
		int res = QMessageBox::information( this, "Hydrogen",
											tr( "Warning! Changing the drumkit name will result in creating a new drumkit with this name.\nAre you sure?"),
											pCommonStrings->getButtonOk(),
											pCommonStrings->getButtonCancel(),
											nullptr, 1 );
		if ( res == 1 ) {
			return;
		}
		else
		{
			reload = true;
		}
	}
	
	//check the name and set the drumkitinfo to current drumkit
	if ( m_pDrumkitInfo != nullptr && !nameTxt->text().isEmpty() ){
		m_pDrumkitInfo->set_name( nameTxt->text() );
		m_pDrumkitInfo->set_author( authorTxt->text() );
		m_pDrumkitInfo->set_info( infoTxt->toHtml() );

		License license = m_pDrumkitInfo->get_license();
		if ( licenseComboBox->currentIndex() ==
			 static_cast<int>(License::Other) ) {
			license.parse( licenseTxt->text() );
		}
		else {
			license.setType( static_cast<License::LicenseType>(licenseComboBox->currentIndex()) );
		}
		m_pDrumkitInfo->set_license( license );
		
		m_pDrumkitInfo->set_image( imageText->text() );
		
		License imageLicense = m_pDrumkitInfo->get_license();
		if ( imageLicenseComboBox->currentIndex() ==
			 static_cast<int>(License::Other) ) {
			imageLicense.parse( imageLicenseText->text() );
		}
		else {
			imageLicense.setType( static_cast<License::LicenseType>(imageLicenseComboBox->currentIndex()) );
		}
		m_pDrumkitInfo->set_image_license( imageLicense );

		if( !H2Core::Drumkit::save( nameTxt->text(),
									authorTxt->text(),
									infoTxt->toHtml(),
									m_pDrumkitInfo->get_license(),
									m_pDrumkitInfo->get_path() + "/" + m_pDrumkitInfo->get_image(),
									m_pDrumkitInfo->get_image_license(),
									H2Core::Hydrogen::get_instance()->getSong()->getInstrumentList(),
									H2Core::Hydrogen::get_instance()->getSong()->getComponents(),
									true ) ) {
			QMessageBox::information( this, "Hydrogen", tr ( "Saving of this drumkit failed."));
		}
	}

	//check pre loaded drumkit name  and reload the old drumkit
	if ( m_pPreDrumkitInfo != nullptr && m_pDrumkitInfo != nullptr){
		if ( m_pPreDrumkitInfo->get_name() != Hydrogen::get_instance()->getCurrentDrumkitName() ||
			 m_pPreDrumkitInfo->isUserDrumkit() != m_pDrumkitInfo->isUserDrumkit() ) {
			Hydrogen::get_instance()->loadDrumkit( m_pPreDrumkitInfo );
		}
	}

	//reload if necessary
	if ( reload == true ){
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
	}

	accept();

}

}
