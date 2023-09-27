/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include "../Widgets/Button.h"
#include "../Widgets/LCDDisplay.h"

#include <core/Basics/DrumkitMap.h>
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
{
	setObjectName( "SoundLibraryPropertiesDialog" );
	
	setupUi( this );

	auto pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	setWindowTitle( tr( "SoundLibrary Properties" ) );
	adjustSize();
	setMinimumSize( width(), height() );

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
	
	mappingTable->setColumnCount( 2 );
	mappingTable->setHorizontalHeaderLabels( QStringList() <<
											 tr( "Instrument" ) <<
											 tr( "Type" ) );
	mappingTable->setColumnWidth( 0, 220 );
	mappingTable->verticalHeader()->hide();
	mappingTable->horizontalHeader()->setStretchLastSection( true );

	licensesTable->setColumnCount( 4 );
	licensesTable->setHorizontalHeaderLabels( QStringList() <<
											 tr( "Instrument" ) <<
											 tr( "Component" ) <<
											 tr( "Sample" ) <<
											 tr( "License" ) );
	licensesTable->verticalHeader()->hide();
	licensesTable->horizontalHeader()->setStretchLastSection( true );

	licensesTable->setColumnWidth( 0, 160 );
	licensesTable->setColumnWidth( 1, 80 );
	licensesTable->setColumnWidth( 2, 210 );

	updateLicensesTable();
	updateMappingTable();
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

void SoundLibraryPropertiesDialog::updateLicensesTable() {
	DEBUGLOG("");
	auto pPref = H2Core::Preferences::get_instance();
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();

	if ( m_pDrumkit == nullptr ){
		return;
	}

	auto contentVector = m_pDrumkit->summarizeContent();

	if ( contentVector.size() > 0 ) {
		licensesTable->show();
		licensesTable->setRowCount( contentVector.size() );

		int nFirstMismatchRow = -1;

		for ( int ii = 0; ii < contentVector.size(); ++ ii ) {
			const auto ccontent = contentVector[ ii ];

			LCDDisplay* pInstrumentItem = new LCDDisplay( nullptr );
			pInstrumentItem->setText( ccontent->m_sInstrumentName );
			pInstrumentItem->setIsActive( false );
			pInstrumentItem->setToolTip( ccontent->m_sInstrumentName );
			LCDDisplay* pComponentItem = new LCDDisplay( nullptr );
			pComponentItem->setText( ccontent->m_sComponentName );
			pComponentItem->setIsActive( false );
			pComponentItem->setToolTip( ccontent->m_sComponentName );
			LCDDisplay* pSampleItem = new LCDDisplay( nullptr );
			pSampleItem->setText( ccontent->m_sSampleName );
			pSampleItem->setIsActive( false );
			pSampleItem->setToolTip( ccontent->m_sSampleName );
			LCDDisplay* pLicenseItem = new LCDDisplay( nullptr );
			pLicenseItem->setText( ccontent->m_license.getLicenseString() );
			pLicenseItem->setIsActive( false );
			pLicenseItem->setToolTip( ccontent->m_license.getLicenseString() );

			// In case of a license mismatch we highlight the row
			if ( ccontent->m_license != m_pDrumkit->get_license() ) {
				QString sHighlight = QString( "color: %1; background-color: %2" )
					.arg( pPref->getColorTheme()->m_buttonRedTextColor.name() )
					.arg( pPref->getColorTheme()->m_buttonRedColor.name() );
				pInstrumentItem->setStyleSheet( sHighlight );
				pComponentItem->setStyleSheet( sHighlight );
				pSampleItem->setStyleSheet( sHighlight );
				pLicenseItem->setStyleSheet( sHighlight );

				if ( nFirstMismatchRow == -1 ) {
					nFirstMismatchRow = ii;
				}
			}

			licensesTable->setCellWidget( ii, 0, pInstrumentItem );
			licensesTable->setCellWidget( ii, 1, pComponentItem );
			licensesTable->setCellWidget( ii, 2, pSampleItem );
			licensesTable->setCellWidget( ii, 3, pLicenseItem );
		}

		// In case of a mismatch scroll into view
		if ( nFirstMismatchRow != -1 ) {
			licensesTable->showRow( nFirstMismatchRow );
		}
	}
	else {
		licensesTable->hide();
	}
}

void SoundLibraryPropertiesDialog::updateMappingTable() {
	const auto pPref = Preferences::get_instance();
	const auto pDatabase =
		Hydrogen::get_instance()->getSoundLibraryDatabase();

	if ( m_pDrumkit == nullptr || m_pDrumkit->getDrumkitMap() == nullptr ||
		 m_pDrumkit->get_instruments() == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	const auto pMap = m_pDrumkit->getDrumkitMap();
	const auto pInstrumentList = m_pDrumkit->get_instruments();

	mappingTable->clearContents();
	mappingTable->setRowCount( std::max( pMap->size(),
										 pInstrumentList->size() ) );

	QMenu* pTypesMenu = new QMenu( this );
	for ( const auto& ssType : pDatabase->getAllTypes() ) {
		pTypesMenu->addAction( ssType );
	}

	// Coloring of highlighted rows
	const QString sHighlight = QString( "color: %1; background-color: %2" )
		.arg( pPref->getColorTheme()->m_buttonRedTextColor.name() )
		.arg( pPref->getColorTheme()->m_buttonRedColor.name() );

	auto insertRow = [=]( const QString& sTextName, const QString& sTextType, int nCell ) {
		LCDDisplay* pInstrumentName = new LCDDisplay( nullptr );
		pInstrumentName->setText( sTextName );
		pInstrumentName->setIsActive( false );
		pInstrumentName->setSizePolicy( QSizePolicy::Expanding,
										QSizePolicy::Expanding );
		pInstrumentName->setToolTip( sTextName );

		LCDCombo* pInstrumentType = new LCDCombo( nullptr);
		for ( const auto& ssType : pDatabase->getAllTypes() ) {
			pInstrumentType->addItem( ssType );
		}
		pInstrumentType->setEditable( true );
		pInstrumentType->setCurrentText( sTextType );

		mappingTable->setCellWidget( nCell, 0, pInstrumentName );
		mappingTable->setCellWidget( nCell, 1, pInstrumentType );
	};

	int nnCell = 0;
	for ( const auto& ppInstrument : *pInstrumentList ) {

		const std::vector<DrumkitMap::Type> types =
			pMap->getTypes( ppInstrument->get_id() );

		if ( types.size() > 0 ) {
			// Mapping for instrument found
			for ( const auto& ssType : types ) {
				insertRow( ppInstrument->get_name(), ssType, nnCell );
				++nnCell;
			}
		}
		else {
			// No mapping found for instrument
			insertRow( ppInstrument->get_name(), "", nnCell );
			++nnCell;
		}
	}

	// In case there was some garbage data or duplicates in the mapping file, we
	// ensure to only show the elements we just added.
	mappingTable->setRowCount( nnCell );
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
	
	updateLicensesTable();
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
	QString drumkitDir = m_pDrumkit->get_path();

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
	m_pDrumkit->set_image( filename );
	updateImage( fileName );
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
		
	m_pDrumkit->set_image( imageText->text() );
	if ( m_pDrumkit->get_image_license() != newImageLicense ) {
		m_pDrumkit->set_image_license( newImageLicense );
	}
	
	QApplication::setOverrideCursor(Qt::WaitCursor);
			
	if ( ! m_pDrumkit->save() ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::information( this, "Hydrogen", tr ( "Saving of this drumkit failed."));
		ERRORLOG( "Saving of this drumkit failed." );
		return;
	}

	pHydrogen->getSoundLibraryDatabase()->updateDrumkits();
			
	QApplication::restoreOverrideCursor();

	accept();

}

}
