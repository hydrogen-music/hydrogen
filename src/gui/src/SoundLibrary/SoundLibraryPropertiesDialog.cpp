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

#include <QtGui>
#include <QtWidgets>

#include "../HydrogenApp.h"
#include "../CommonStrings.h"

#include "SoundLibraryPropertiesDialog.h"
#include "../InstrumentRack.h"
#include "SoundLibraryPanel.h"
#include <core/Basics/InstrumentList.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core
{

SoundLibraryPropertiesDialog::SoundLibraryPropertiesDialog( QWidget* pParent, std::shared_ptr<Drumkit> pDrumkit, bool bDrumkitNameLocked )
 : QDialog( pParent )
 , m_pDrumkit( pDrumkit )
 , m_bDrumkitNameLocked( bDrumkitNameLocked )
 , m_sNewImagePath( "" )
{
	setObjectName( "SoundLibraryPropertiesDialog" );
	
	setupUi( this );

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	auto pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	setWindowTitle( tr( "SoundLibrary Properties" ) );

	setupLicenseComboBox( licenseComboBox );
	connect( licenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( licenseComboBoxChanged( int ) ) );
	setupLicenseComboBox( imageLicenseComboBox );
	connect( imageLicenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( imageLicenseComboBoxChanged( int ) ) );

	bool bDrumkitWritable = false;
	//display the current drumkit infos into the qlineedit
	if ( pDrumkit != nullptr ){

		auto drumkitType = Filesystem::determineDrumkitType(
			pDrumkit->get_path() );
		if ( drumkitType == Filesystem::DrumkitType::User ||
			 drumkitType == Filesystem::DrumkitType::SessionReadWrite ) {
			bDrumkitWritable = true;
		}

		nameTxt->setText( pDrumkit->get_name() );

		if ( bDrumkitNameLocked ) {
			nameTxt->setIsActive( false );
			nameTxt->setToolTip( tr( "Altering the name of a drumkit would result in the creation of a new one. To do so, you need to load the drumkit (if you haven't done so already) using right click > load and select Drumkits > Save As in the main menu" ) );
		}
		
		authorTxt->setText( QString( pDrumkit->get_author() ) );
		infoTxt->append( QString( pDrumkit->get_info() ) );

		License license = pDrumkit->get_license();
		licenseComboBox->setCurrentIndex( static_cast<int>( license.getType() ) );
		licenseStringTxt->setText( license.getLicenseString() );
	
		imageText->setText( QString( pDrumkit->get_image() ) );

		License imageLicense = pDrumkit->get_image_license();
		imageLicenseComboBox->setCurrentIndex( static_cast<int>( imageLicense.getType() ) );
		imageLicenseStringTxt->setText( imageLicense.getLicenseString() );
	}

	if ( licenseComboBox->currentIndex() == static_cast<int>( License::Unspecified ) ) {
		licenseStringLbl->hide();
		licenseStringTxt->hide();
	}
	if ( imageLicenseComboBox->currentIndex() == static_cast<int>( License::Unspecified ) ) {
		imageLicenseStringLbl->hide();
		imageLicenseStringTxt->hide();
	}
	
	licenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	licenseStringLbl->setText( pCommonStrings->getLicenseStringLbl() );
	licenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip() );
	imageLicenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	imageLicenseStringLbl->setText( pCommonStrings->getLicenseStringLbl() );
	imageLicenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip() );

	// In case the drumkit name is not locked/the dialog is used as
	// "Save As" nothing needs to be disabled.
	if ( ! bDrumkitWritable && bDrumkitNameLocked ) {
		QString sToolTip = tr( "The current drumkit is read-only. Please use Drumkits > Save As in the main menu to create a new one first." );
		
		// The drumkit is read-only. Thus we won't support altering
		// any of its properties.
		authorTxt->setIsActive( false );
		authorTxt->setToolTip( sToolTip );
		infoTxt->setEnabled( false );
		infoTxt->setReadOnly( true );
		infoTxt->setToolTip( sToolTip );
		licenseComboBox->setIsActive( false );
		licenseComboBox->setToolTip( sToolTip );
		licenseStringTxt->setIsActive( false );
		licenseStringTxt->setToolTip( sToolTip );
		imageText->setIsActive( false );
		imageText->setToolTip( sToolTip );
		imageLicenseComboBox->setIsActive( false );
		imageLicenseComboBox->setToolTip( sToolTip );
		imageLicenseStringTxt->setIsActive( false );
		imageLicenseStringTxt->setToolTip( sToolTip );
		saveBtn->setIsActive( false );
		saveBtn->setToolTip( sToolTip );
		imageBrowsePushButton->setIsActive( false );
		imageBrowsePushButton->setToolTip( sToolTip );

		// Rather dirty fix to align the design of the QTextEdit to
		// the coloring of our custom QLineEdits.
		infoTxt->setStyleSheet( QString( "\
QTextEdit { \
    color: %1; \
    background-color: %2; \
}" )
								.arg( pPref->getColorTheme()->m_windowTextColor.name() )
								.arg( pPref->getColorTheme()->m_windowColor.name() ) );
										
	}

	saveBtn->setFixedFontSize( 12 );
	saveBtn->setSize( QSize( 70, 23 ) );
	saveBtn->setBorderRadius( 3 );
	m_cancelBtn->setFixedFontSize( 12 );
	m_cancelBtn->setSize( QSize( 70, 23 ) );
	m_cancelBtn->setBorderRadius( 3 );
	imageBrowsePushButton->setFixedFontSize( 12 );
	imageBrowsePushButton->setBorderRadius( 3 );
	imageBrowsePushButton->setSize( QSize( 70, 23 ) );
	
	contentTable->setColumnCount( 4 );
	contentTable->setHorizontalHeaderLabels( QStringList() <<
											 tr( "Instrument" ) <<
											 tr( "Component" ) <<
											 tr( "Sample" ) <<
											 tr( "License" ) );
	contentTable->verticalHeader()->hide();
	contentTable->horizontalHeader()->setStretchLastSection( true );

	contentTable->setColumnWidth( 0, 160 );
	contentTable->setColumnWidth( 1, 80 );
	contentTable->setColumnWidth( 2, 210 );

	updateLicenseTable();
}


SoundLibraryPropertiesDialog::~SoundLibraryPropertiesDialog()
{
	INFOLOG( "DESTROY" );

}


/// On showing the dialog (after layout sizes have been applied), load the drumkit image if any.
void SoundLibraryPropertiesDialog::showEvent( QShowEvent *e )
{
	if ( m_pDrumkit != nullptr &&
		 ! m_pDrumkit->get_image().isEmpty() ) {
		QString sImage = m_pDrumkit->get_path() + "/" + m_pDrumkit->get_image();
		updateImage( sImage );
	}
	else {
		drumkitImageLabel->hide();
	}
}

void SoundLibraryPropertiesDialog::updateLicenseTable() {
	auto pPref = H2Core::Preferences::get_instance();
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();
	
	if ( m_pDrumkit == nullptr ){
		return;
	}

	auto contentVector = m_pDrumkit->summarizeContent();

	if ( contentVector.size() > 0 ) {
		contentTable->show();
		contentLabel->show();
		contentTable->setRowCount( contentVector.size() );

		int nFirstMismatchRow = -1;

		for ( int ii = 0; ii < contentVector.size(); ++ ii ) {
			const auto ccontent = contentVector[ ii ];
			
			QLineEdit* pInstrumentItem = new QLineEdit( ccontent->m_sInstrumentName );
			pInstrumentItem->setEnabled( false );
			pInstrumentItem->setToolTip( ccontent->m_sInstrumentName );
			QLineEdit* pComponentItem = new QLineEdit( ccontent->m_sComponentName );
			pComponentItem->setEnabled( false );
			pComponentItem->setToolTip( ccontent->m_sComponentName );
			QLineEdit* pSampleItem = new QLineEdit( ccontent->m_sSampleName );
			pSampleItem->setEnabled( false );
			pSampleItem->setToolTip( ccontent->m_sSampleName );
			QLineEdit* pLicenseItem =
				new QLineEdit( ccontent->m_license.getLicenseString() );
			pLicenseItem->setEnabled( false );
			pLicenseItem->setToolTip( ccontent->m_license.getLicenseString() );

			// In case of a license mismatch we highlight the row
			if ( ccontent->m_license != m_pDrumkit->get_license() ) {
				QString sRed = QString( "color: %1; background-color: %2" )
					.arg( pPref->getColorTheme()->m_buttonRedColor.name() )
					.arg( pPref->getColorTheme()->m_windowColor.name() );
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
	
void SoundLibraryPropertiesDialog::licenseComboBoxChanged( int ) {

	licenseStringTxt->setText( License::LicenseTypeToQString(
		static_cast<License::LicenseType>( licenseComboBox->currentIndex() ) ) );

	if ( licenseComboBox->currentIndex() == static_cast<int>( License::Unspecified ) ) {
		licenseStringLbl->hide();
		licenseStringTxt->hide();
	}
	else {
		licenseStringLbl->show();
		licenseStringTxt->show();
	}
	
	updateLicenseTable();
}
	
void SoundLibraryPropertiesDialog::imageLicenseComboBoxChanged( int ) {

	imageLicenseStringTxt->setText( License::LicenseTypeToQString(
		static_cast<License::LicenseType>( imageLicenseComboBox->currentIndex() ) ) );

	if ( imageLicenseComboBox->currentIndex() == static_cast<int>( License::Unspecified ) ) {
		imageLicenseStringLbl->hide();
		imageLicenseStringTxt->hide();
	}
	else {
		imageLicenseStringLbl->show();
		imageLicenseStringTxt->show();
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
	if ( m_pDrumkit == nullptr ) {
		return;
	}
	
	// Try to get the drumkit directory and open file browser
	QString sDrumkitDir = m_pDrumkit->get_path();

	QString sFilePath = QFileDialog::getOpenFileName(
		this, tr("Open Image"), sDrumkitDir,
		tr("Image Files (*.png *.jpg *.jpeg)"), nullptr
#if not defined(WIN32) and not defined(__APPLE__) // Linux
		// See FileDialog.h for details
		, QFileDialog::DontUseNativeDialog
#endif
	);

	// If cancel was clicked just abort
	if ( sFilePath == nullptr || sFilePath.isEmpty() ) {
		return;
	}

	m_sNewImagePath = sFilePath;

	QFileInfo fileInfo( sFilePath );
	QString sFileName( fileInfo.fileName() );
	imageText->setText( sFileName );

	updateImage( sFilePath );
}

void SoundLibraryPropertiesDialog::on_saveBtn_clicked()
{
	if ( m_pDrumkit == nullptr ) {
		return;
	}
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
    
	// Sanity checks.
	//
	// Check whether the license strings from the line edits comply to
	// the license types selected in the combo boxes.
	License licenseCheck( licenseStringTxt->text() );
	if ( static_cast<int>(licenseCheck.getType()) != licenseComboBox->currentIndex() ) {
		if ( QMessageBox::warning( this, "Hydrogen",
								   tr( "Specified drumkit License String does not comply with the license selected in the combo box." ),
								   QMessageBox::Ok | QMessageBox::Cancel,
								   QMessageBox::Cancel )
			 == QMessageBox::Cancel ) {
			WARNINGLOG( QString( "Abort, since drumkit License String [%1] does not comply to selected License Type [%2]" )
						.arg( licenseStringTxt->text() )
						.arg( License::LicenseTypeToQString(
						    static_cast<License::LicenseType>(licenseComboBox->currentIndex()) ) ) );
			return;
		}
	}
	License imageLicenseCheck( imageLicenseStringTxt->text() );
	if ( static_cast<int>(imageLicenseCheck.getType()) !=
		 imageLicenseComboBox->currentIndex() ) {
		if ( QMessageBox::warning( this, "Hydrogen",
								   tr( "Specified image License String does not comply with the license selected in the combo box." ),
								   QMessageBox::Ok | QMessageBox::Cancel,
								   QMessageBox::Cancel )
			 == QMessageBox::Cancel ) {
			WARNINGLOG( QString( "Abort, since drumkit image License String [%1] does not comply to selected License Type [%2]" )
						.arg( imageLicenseStringTxt->text() )
						.arg( License::LicenseTypeToQString(
						    static_cast<License::LicenseType>(imageLicenseComboBox->currentIndex()) ) ) );
			return;
		}
	}
	
	//check the name and set the drumkitinfo to current drumkit
	if ( nameTxt->text().isEmpty() ){
		QMessageBox::warning( this, "Hydrogen", tr( "The name of the drumkit must not be left empty" ) );
		return;
	}

	QString sNewLicenseString( licenseStringTxt->text() );
	if ( licenseComboBox->currentIndex() ==
		 static_cast<int>(License::Unspecified) ) {
		sNewLicenseString = "";
	}
	License newLicense( sNewLicenseString );
	newLicense.setCopyrightHolder( m_pDrumkit->get_author() );

	QString sNewImageLicenseString( imageLicenseStringTxt->text() );
	if ( imageLicenseComboBox->currentIndex() ==
		 static_cast<int>(License::Unspecified) ) {
		sNewImageLicenseString = "";
	}
	License newImageLicense( sNewImageLicenseString );
	newImageLicense.setCopyrightHolder( m_pDrumkit->get_author() );

	const QString sOldPath = m_pDrumkit->get_path();
	if ( m_pDrumkit->get_name() != nameTxt->text() ) {
		m_pDrumkit->set_name( nameTxt->text() );
		m_pDrumkit->set_path( H2Core::Filesystem::usr_drumkits_dir() +
							  nameTxt->text() );
	}
	m_pDrumkit->set_author( authorTxt->text() );
	m_pDrumkit->set_info( infoTxt->toHtml() );
		
	// Only update the license in case it changed (in order to not
	// overwrite an attribution).
	if ( m_pDrumkit->get_license() != newLicense ) {
		m_pDrumkit->set_license( newLicense );
	}

	if ( ! HydrogenApp::checkDrumkitLicense( m_pDrumkit ) ) {
		ERRORLOG( "User cancelled dialog due to licensing issues." );
		return;
	}

	// Will contain image which should be removed. To keep the previous image,
	// this string should be empty.
	QString sOldImagePath;
	if ( imageText->text() != m_pDrumkit->get_image() ) {
		int nRes = QMessageBox::information( this, "Hydrogen",
											 tr( "Delete previous drumkit image" )
											 .append( QString( " [%1]" ).arg( m_pDrumkit->get_image() ) ),
											 QMessageBox::Yes | QMessageBox::No );
		if ( nRes == QMessageBox::Yes ) {
			sOldImagePath = QString( "%1/%2" ).arg( sOldPath )
				.arg( m_pDrumkit->get_image() );
		}
		m_pDrumkit->set_image( imageText->text() );
	}

	if ( m_pDrumkit->get_image_license() != newImageLicense ) {
		m_pDrumkit->set_image_license( newImageLicense );
	}
	
	QApplication::setOverrideCursor(Qt::WaitCursor);

	// Write new properties to disk.
	if ( ! m_pDrumkit->save() ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::information( this, "Hydrogen", tr ( "Saving of this drumkit failed."));
		ERRORLOG( "Saving of this drumkit failed." );
		return;
	}

	// Copy the selected image into the drumkit folder (in case a file outside
	// of it was selected.)
	if ( ! m_sNewImagePath.isEmpty() ) {
		QFileInfo fileInfo( m_sNewImagePath );

		if ( fileInfo.dir().absolutePath() != m_pDrumkit->get_path() ) {
			INFOLOG( QString( "Copying [%1] into [%2]" ).arg( m_sNewImagePath )
					 .arg( m_pDrumkit->get_path() ) );
			const QString sTargetPath =
				QString( "%1/%2" ).arg( m_pDrumkit->get_path() )
				.arg( fileInfo.fileName() );
			// Logging is done in file_copy.
			Filesystem::file_copy( m_sNewImagePath, sTargetPath, true, false );
		}
	}

	if ( ! sOldImagePath.isEmpty() ) {
		Filesystem::rm( sOldImagePath, false, false );
	}

	pHydrogen->getSoundLibraryDatabase()->updateDrumkits();
			
	QApplication::restoreOverrideCursor();

	accept();

}

}
