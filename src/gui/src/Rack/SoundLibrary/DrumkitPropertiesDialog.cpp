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

#include <set>
#include <QtGui>
#include <QtWidgets>

#include "DrumkitPropertiesDialog.h"

#include "../../CommonStrings.h"
#include "../../HydrogenApp.h"
#include "../../MainForm.h"
#include "../../PatternEditor/PatternEditor.h"
#include "../../UndoActions.h"
#include "../../Widgets/Button.h"
#include "../../Widgets/LCDDisplay.h"

#include <core/Basics/DrumkitMap.h>
#include <core/Basics/InstrumentList.h>
#include <core/Hydrogen.h>
#include <core/NsmClient.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core
{

DrumkitPropertiesDialog::DrumkitPropertiesDialog( QWidget* pParent,
												  std::shared_ptr<Drumkit> pDrumkit,
												  bool bEditingNotSaving,
												  bool bSaveToNsmSession,
												  int nInstrumentID )
 : QDialog( pParent )
 , m_pDrumkit( pDrumkit )
 , m_bEditingNotSaving( bEditingNotSaving )
 , m_bSaveToNsmSession( bSaveToNsmSession )
{
	setObjectName( "DrumkitPropertiesDialog" );

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	setupUi( this );

	const auto pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setupLicenseComboBox( licenseComboBox );
	setupLicenseComboBox( imageLicenseComboBox );

	// Remove size constraints
	versionSpinBox->setFixedSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
	versionSpinBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	// Arbitrary high number.
	versionSpinBox->setMaximum( 300 );
	// Allow to focus the widget using mouse wheel and tab
	versionSpinBox->setFocusPolicy( Qt::WheelFocus );
	licenseComboBox->setFocusPolicy( Qt::WheelFocus );
	imageLicenseComboBox->setFocusPolicy( Qt::WheelFocus );
	m_cancelBtn->setFocusPolicy( Qt::WheelFocus );
	saveBtn->setFocusPolicy( Qt::WheelFocus );
	imageBrowsePushButton->setFocusPolicy( Qt::WheelFocus );

	// Allow to save the dialog by pressing Return.
	saveBtn->setFocus();

	nameLabel->setText( pCommonStrings->getNameDialog() );
	versionLabel->setText( pCommonStrings->getVersionDialog() );
	notesLabel->setText( pCommonStrings->getNotesDialog() );

	if ( bSaveToNsmSession &&
		 ! Hydrogen::get_instance()->isUnderSessionManagement() ) {
		ERRORLOG( "NSM session export request while there is no active NSM session. Saving to Sound Library instead." );
		m_bSaveToNsmSession = false;
	}

	bool bDrumkitWritable = false;
	//display the current drumkit infos into the qlineedit
	if ( pDrumkit != nullptr ){

		versionSpinBox->setValue( pDrumkit->getVersion() );
		auto drumkitContext = pDrumkit->getContext();
		if ( drumkitContext == Drumkit::Context::User ||
			 drumkitContext == Drumkit::Context::SessionReadWrite ||
			 drumkitContext == Drumkit::Context::Song ) {
			bDrumkitWritable = true;
		}

		nameTxt->setText( pDrumkit->getName() );

		if ( m_pDrumkit->getContext() == Drumkit::Context::Song ) {
			if ( bEditingNotSaving ) {
				setWindowTitle( pCommonStrings->getActionEditCurrentDrumkitProperties() );
			}
			else if ( m_bSaveToNsmSession ){
				setWindowTitle( tr( "Save a copy of the current drumkit to NSM session folder" ) );
			}
			else {
				setWindowTitle( tr( "Save a copy of the current drumkit to the Sound Library" ) );
			}
		}
		else {
			if ( bEditingNotSaving ) {
				setWindowTitle( pCommonStrings->getActionEditDrumkitProperties() );
				nameTxt->setIsActive( false );
				nameTxt->setToolTip( tr( "Altering the name of a drumkit would result in the creation of a new one. To do so, use 'Duplicate' instead." ) );
			}
			else {
				setWindowTitle( tr( "Create New Drumkit" ) );
			}
		}

		authorLabel->setText( pCommonStrings->getAuthorDialog() );
		authorTxt->setText( QString( pDrumkit->getAuthor() ) );
		infoTxt->append( QString( pDrumkit->getInfo() ) );

		License license = pDrumkit->getLicense();
		licenseComboBox->setCurrentIndex( static_cast<int>( license.getType() ) );
		licenseStringTxt->setText( license.getLicenseString() );

		// Will contain a file name in case of an image file located in the
		// drumkit folder or an absolute path in case of one located outside of
		// it (in our cache folder in case of a song kit).
		imageText->setText( pDrumkit->getImage() );
		imageText->setAlignment( Qt::AlignLeft );

		License imageLicense = pDrumkit->getImageLicense();
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

	connect( licenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( licenseComboBoxChanged( int ) ) );
	connect( imageLicenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( imageLicenseComboBoxChanged( int ) ) );

	// In case the drumkit name is not locked/the dialog is used as
	// "Save As" nothing needs to be disabled.
	if ( ! bDrumkitWritable && bEditingNotSaving ) {
		QString sToolTip = tr( "The current drumkit is read-only. Please use 'Duplicate' to move a copy into user space." );
		
		// The drumkit is read-only. Thus we won't support altering
		// any of its properties.
		authorTxt->setIsActive( false );
		authorTxt->setToolTip( sToolTip );
		versionSpinBox->setIsActive( false );
		versionSpinBox->setToolTip( sToolTip );
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

	tabWidget->setTabText( 0, pCommonStrings->getTabGeneralDialog() );
	tabWidget->setTabText( 2, pCommonStrings->getTabLicensesDialog() );
	tabWidget->setCurrentIndex( 0 );

	saveBtn->setFixedFontSize( 12 );
	saveBtn->setSize( QSize( 110, 23 ) );
	saveBtn->setBorderRadius( 3 );
	saveBtn->setType( Button::Type::Push );
	if ( m_pDrumkit != nullptr && bEditingNotSaving &&
		 m_pDrumkit->getContext() == Drumkit::Context::Song ) {
		saveBtn->setText( pCommonStrings->getActionSaveSong() );
	}
	else {
		saveBtn->setText( pCommonStrings->getActionSaveDrumkit() );
	}

	m_cancelBtn->setFixedFontSize( 12 );
	m_cancelBtn->setSize( QSize( 70, 23 ) );
	m_cancelBtn->setBorderRadius( 3 );
	m_cancelBtn->setType( Button::Type::Push );
	m_cancelBtn->setText( pCommonStrings->getButtonCancel() );
	imageBrowsePushButton->setFixedFontSize( 12 );
	imageBrowsePushButton->setBorderRadius( 3 );
	imageBrowsePushButton->setSize( QSize( 70, 23 ) );
	imageBrowsePushButton->setType( Button::Type::Push );
	
	typesTable->setColumnCount( 3 );
	typesTable->setHorizontalHeaderLabels(
		QStringList() <<
		pCommonStrings->getInstrumentId() <<
		pCommonStrings->getInstrumentButton() <<
		pCommonStrings->getInstrumentType() );
	typesTable->setColumnWidth( 0, 55 );
	typesTable->setColumnWidth( 1, 220 );
	typesTable->verticalHeader()->hide();
	typesTable->horizontalHeader()->setStretchLastSection( true );

	licensesTable->setColumnCount( 4 );
	licensesTable->setHorizontalHeaderLabels(
		QStringList() <<
		pCommonStrings->getInstrumentButton() <<
		pCommonStrings->getComponent() <<
		pCommonStrings->getSample() <<
		pCommonStrings->getLicense() );

	licensesTable->verticalHeader()->hide();
	licensesTable->horizontalHeader()->setStretchLastSection( true );

	licensesTable->setColumnWidth( 0, 160 );
	licensesTable->setColumnWidth( 1, 80 );
	licensesTable->setColumnWidth( 2, 210 );

	updateLicensesTable();
	updateTypesTable( bDrumkitWritable );

	if ( nInstrumentID != EMPTY_INSTR_ID &&
		 m_idToTypeMap.find( nInstrumentID ) != m_idToTypeMap.end() ) {
		// Widget opened by double clicking a type of an instrument. Select the
		// corresponding type.
		auto pTypeWidget = m_idToTypeMap[ nInstrumentID ];
		if ( pTypeWidget != nullptr ) {
			tabWidget->setCurrentIndex( 1 );
			pTypeWidget->setFocus( Qt::PopupFocusReason );
		}
	}
}


DrumkitPropertiesDialog::~DrumkitPropertiesDialog()
{
	INFOLOG( "DESTROY" );
}

/// On showing the dialog (after layout sizes have been applied), load the drumkit image if any.
void DrumkitPropertiesDialog::showEvent( QShowEvent *e )
{
	if ( m_pDrumkit != nullptr &&
		 ! m_pDrumkit->getAbsoluteImagePath().isEmpty() ) {
		updateImage( m_pDrumkit->getAbsoluteImagePath() );
	}
	else {
		drumkitImageLabel->hide();
	}
}

void DrumkitPropertiesDialog::updateLicensesTable() {
	const auto pColorTheme = H2Core::Preferences::get_instance()->getColorTheme();
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
			pInstrumentItem->setText( ccontent->m_sInstrumentName);
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
			if ( ccontent->m_license != m_pDrumkit->getLicense() ) {
				QString sHighlight = QString( "color: %1; background-color: %2" )
					.arg( pColorTheme->m_buttonRedTextColor.name() )
					.arg( pColorTheme->m_buttonRedColor.name() );
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

void DrumkitPropertiesDialog::updateTypesTable( bool bDrumkitWritable ) {
	const auto pPref = Preferences::get_instance();
	const auto pDatabase =
		Hydrogen::get_instance()->getSoundLibraryDatabase();
	m_idToTypeMap.clear();

	if ( m_pDrumkit == nullptr ||
		 m_pDrumkit->getInstruments() == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	const auto pInstrumentList = m_pDrumkit->getInstruments();

	typesTable->clearContents();
	typesTable->setRowCount( pInstrumentList->size() );

	auto types = pDatabase->getAllTypes();
	types.merge( m_pDrumkit->getAllTypes() );

	QStringList allTypeStrings;
	// We need the invalid empty type to set a proper index for all instruments
	// with missing types.
	allTypeStrings << "";
 	for ( const auto& ssType : types ) {
		allTypeStrings << ssType;
	 }

	// Sort them alphabetically in ascending order.
	allTypeStrings.removeDuplicates();
	allTypeStrings.sort();

	QMenu* pTypesMenu = new QMenu( this );
	for ( const auto& ssType : allTypeStrings ) {
		pTypesMenu->addAction( ssType );
	}

	auto insertRow = [=]( int nInstrumentId,
						  const QString& sTextName,
						  const QString& sTextType,
						  int nCell ) {

		LCDDisplay* pInstrumentId = new LCDDisplay( nullptr );
		pInstrumentId->setText( QString::number( nInstrumentId ) );
		pInstrumentId->setIsActive( false );
		pInstrumentId->setSizePolicy( QSizePolicy::Fixed,
										QSizePolicy::Expanding );

		LCDDisplay* pInstrumentName = new LCDDisplay( nullptr );
		pInstrumentName->setText( sTextName );
		pInstrumentName->setIsActive( false );
		pInstrumentName->setSizePolicy( QSizePolicy::Expanding,
										QSizePolicy::Expanding );
		pInstrumentName->setToolTip( sTextName );

		int nIndex = -1;
		int nnType = 0;
		LCDCombo* pInstrumentType = new LCDCombo( nullptr );
		for ( const auto& ssType : allTypeStrings ) {
			pInstrumentType->addItem( ssType );

			if ( ssType == sTextType ) {
				nIndex = nnType;
			}
			nnType++;
		}

		if ( nIndex == -1 && ! sTextType.isEmpty() ) {
			ERRORLOG( QString( "Provided type [%1] could not be found in database" )
					  .arg( sTextType ) );
		}
		else if ( nIndex != -1 ) {
			pInstrumentType->setCurrentIndex( nIndex );
		}
		else {
			pInstrumentType->setCurrentText( sTextType );
		}

		if ( bDrumkitWritable ) {
			pInstrumentType->setIsActive( true );
			pInstrumentType->setEditable( true );
			pInstrumentType->setFocusPolicy( Qt::StrongFocus );
		}
		else {
			pInstrumentType->setIsActive( false );
		}

		typesTable->setCellWidget( nCell, 0, pInstrumentId );
		typesTable->setCellWidget( nCell, 1, pInstrumentName );
		typesTable->setCellWidget( nCell, 2, pInstrumentType );

		m_idToTypeMap[ nInstrumentId ] = pInstrumentType;
	};

	int nnCell = 0;
	for ( const auto& ppInstrument : *pInstrumentList ) {
		insertRow( ppInstrument->getId(), ppInstrument->getName(),
				   ppInstrument->getType(), nnCell );
		nnCell++;
	}

	highlightDuplicates();
}

void DrumkitPropertiesDialog::licenseComboBoxChanged( int ) {

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
	
void DrumkitPropertiesDialog::imageLicenseComboBoxChanged( int ) {

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

void DrumkitPropertiesDialog::updateImage( const QString& sFilePath )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pColorTheme = Preferences::get_instance()->getColorTheme();

	//  Styling used in case we assign text not images.
	drumkitImageLabel->setStyleSheet(
		QString( "QLabel { color: %1; background-color: %2;}" )
		.arg( pColorTheme->m_windowTextColor.name() )
		.arg( pColorTheme->m_windowColor.name() ) );
	drumkitImageLabel->show();

	if ( ! Filesystem::file_exists( sFilePath, false ) ) {
		drumkitImageLabel->setText( "File could not be found." );
		return;
	}

	QPixmap *pPixmap = new QPixmap ( sFilePath );

	// Check whether the loading worked.
	if ( pPixmap->isNull() ) {
		ERRORLOG( QString( "Unable to load pixmap from [%1]" ).arg( sFilePath ) );
		drumkitImageLabel->setText( tr( "Unable to load pixmap" ) );
		return;
	}
	
	// scale the image down to fit if required
	int x = (int) drumkitImageLabel->size().width();
	int y = drumkitImageLabel->size().height();
	float labelAspect = (float) x / y;
	float imageAspect = (float) pPixmap->width() / pPixmap->height();

	if ( ( x < pPixmap->width() ) || ( y < pPixmap->height() ) ) {
		if ( labelAspect >= imageAspect ) {
			// image is taller or the same as label frame
			*pPixmap = pPixmap->scaledToHeight( y );
		}
		else {
			// image is wider than label frame
			*pPixmap = pPixmap->scaledToWidth( x );
		}
	}
	drumkitImageLabel->setPixmap(*pPixmap);
	drumkitImageLabel->show();

}

void DrumkitPropertiesDialog::on_imageBrowsePushButton_clicked()
{
	if ( m_pDrumkit == nullptr ) {
		return;
	}
	
	// Try to get the drumkit directory and open file browser
	QString sDrumkitDir = m_pDrumkit->getPath();

	QString sFilePath =
		QFileDialog::getOpenFileName( this, tr("Open Image"),
									  sDrumkitDir,
									  tr("Image Files (*.png *.jpg *.jpeg)" ) );

	// If cancel was clicked just abort
	if ( sFilePath == nullptr || sFilePath.isEmpty() ) {
		return;
	}

	imageText->setText( sFilePath );
	updateImage( sFilePath );
}

void DrumkitPropertiesDialog::on_saveBtn_clicked()
{
	if ( m_pDrumkit == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pCommonStrings = pHydrogenApp->getCommonStrings();
    
	// Sanity checks.
	//
	// Check whether the license strings from the line edits comply to
	// the license types selected in the combo boxes.
	License licenseCheck( licenseStringTxt->text() );
	if ( static_cast<int>(licenseCheck.getType()) != licenseComboBox->currentIndex() ) {
		if ( QMessageBox::warning(
				 this, "Hydrogen", pCommonStrings->getLicenseMismatchingUserInput(),
				 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel )
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

	// Types have to be unique.
	std::set<QString> types;
	for ( int ii = 0; ii < typesTable->rowCount(); ++ii ) {
		auto ppItemType =
			dynamic_cast<LCDCombo*>(typesTable->cellWidget( ii, 2 ));
		if ( ppItemType != nullptr ) {
			const auto [ _, bSuccess ] =
				types.insert( ppItemType->currentText() );
			if ( ! bSuccess ) {
				highlightDuplicates();
				QMessageBox::warning( this, "Hydrogen",
									  pCommonStrings->getErrorUniqueTypes() );
				return;
			}
		}
	}


	QString sNewLicenseString( licenseStringTxt->text() );
	if ( licenseComboBox->currentIndex() ==
		 static_cast<int>(License::Unspecified) ) {
		sNewLicenseString = "";
	}
	License newLicense( sNewLicenseString );
	newLicense.setCopyrightHolder( m_pDrumkit->getAuthor() );

	QString sNewImageLicenseString( imageLicenseStringTxt->text() );
	if ( imageLicenseComboBox->currentIndex() ==
		 static_cast<int>(License::Unspecified) ) {
		sNewImageLicenseString = "";
	}
	License newImageLicense( sNewImageLicenseString );
	newImageLicense.setCopyrightHolder( m_pDrumkit->getAuthor() );

	const QString sOldPath = m_pDrumkit->getPath();
	if ( m_pDrumkit->getName() != nameTxt->text() ) {
		m_pDrumkit->setName( nameTxt->text() );
		m_pDrumkit->setPath( H2Core::Filesystem::usr_drumkits_dir() +
							  nameTxt->text() );
	}
	if ( m_pDrumkit->getVersion() != versionSpinBox->value() ) {
		m_pDrumkit->setVersion( versionSpinBox->value() );
	}
	m_pDrumkit->setAuthor( authorTxt->text() );
	m_pDrumkit->setInfo( infoTxt->toHtml() );
		
	// Only update the license in case it changed (in order to not
	// overwrite an attribution).
	if ( m_pDrumkit->getLicense() != newLicense ) {
		m_pDrumkit->setLicense( newLicense );
	}

	if ( ! HydrogenApp::checkDrumkitLicense( m_pDrumkit ) ) {
		ERRORLOG( "User cancelled dialog due to licensing issues." );
		return;
	}

	// Will contain image which should be removed. To keep the previous image,
	// this string should be empty.
	QString sOldImagePath;
	// If set, indicates that the image has changed and the new one requires
	// copying.
	QString sNewImagePath;
	if ( imageText->text() != m_pDrumkit->getImage() ) {

		// Only ask for deleting the previous file if it exists.
		if ( ! m_pDrumkit->getImage().isEmpty() &&
			 Filesystem::file_exists( m_pDrumkit->getAbsoluteImagePath(),
									  true ) ) {
			int nRes = QMessageBox::information(
				this, "Hydrogen",
				tr( "Delete previous drumkit image" )
				.append( QString( " [%1]" )
						 .arg( m_pDrumkit->getAbsoluteImagePath() ) ),
				QMessageBox::Yes | QMessageBox::No );
			if ( nRes == QMessageBox::Yes ) {
				sOldImagePath = m_pDrumkit->getAbsoluteImagePath();
			}
		}

		m_pDrumkit->setImage( imageText->text() );
		sNewImagePath = imageText->text();
	}

	if ( m_pDrumkit->getImageLicense() != newImageLicense ) {
		m_pDrumkit->setImageLicense( newImageLicense );
	}
	
	for ( int ii = 0; ii < typesTable->rowCount(); ++ii ) {
		auto ppItemId =
			dynamic_cast<LCDDisplay*>(typesTable->cellWidget( ii, 0 ));
		auto ppItemName =
			dynamic_cast<LCDDisplay*>(typesTable->cellWidget( ii, 1 ));
		auto ppItemType =
			dynamic_cast<LCDCombo*>(typesTable->cellWidget( ii, 2 ));

		if ( ppItemId != nullptr && ppItemType != nullptr ) {
			const auto ppInstrument = m_pDrumkit->getInstruments()->
				find( ppItemId->text().toInt() );

			if ( ppInstrument != nullptr ) {
				ppInstrument->setType( ppItemType->currentText() );
			}
			else {
				if ( ppItemName != nullptr ) {
					ERRORLOG( QString( "Unable to find instrument [%1] (name: [%2], type: [%3])" )
							  .arg( ppItemId->text() )
							  .arg( ppItemName->text() )
							  .arg( ppItemType->currentText() ) );
				} else {
					ERRORLOG( QString( "Unable to find instrument [%1] (type: [%2])" )
							  .arg( ppItemId->text() )
							  .arg( ppItemType->currentText() ) );
				}
			}
		} else {
			WARNINGLOG( QString( "Invalid row [%1]" ).arg( ii  ) );
		}
	}

	bool bOldImageDeleted = false;
	if ( m_pDrumkit->getContext() == Drumkit::Context::Song ) {
		// Copy the selected image into our cache folder as the kit is a
		// floating one associated to a song.
		if ( ! sNewImagePath.isEmpty() ) {
			QFileInfo fileInfo( sNewImagePath );

			const QString sTargetPath = Filesystem::addUniquePrefix(
				QDir( Filesystem::cache_dir() )
				.absoluteFilePath( fileInfo.fileName() ) );

			// Logging is done in file_copy.
			if ( Filesystem::file_copy( sNewImagePath, sTargetPath, true, false ) ) {
				m_pDrumkit->setImage( sTargetPath );
			}
		}

		if ( ! sOldImagePath.isEmpty() ) {
			Filesystem::rm( sOldImagePath, false, false );
			bOldImageDeleted = true;
		}

		// This is the single point we can initially assign a type to a note not
		// bearing one yet. In all other places, notes do either have a type or
		// not. We ensure this action can be undone and is contained in the same
		// macro as the overall drumkit change.
		//
		// Note that an empty type can not be assigned to an instrument.
		const auto pOldKit = pSong->getDrumkit();

		struct noteToBeMapped {
			std::shared_ptr<Note> pNote;
			DrumkitMap::Type type;
			int nPatternNumber;
		};
		std::vector<noteToBeMapped> notesToBeMapped;
		struct instrumentToBeMapped {
			std::shared_ptr<Instrument> pInstrument;
			DrumkitMap::Type sOldType;
		};
		std::vector<instrumentToBeMapped> newInstrumentTypes;
		// This should always be true. Let's keep it safe.
		if ( pOldKit != nullptr && pOldKit->getInstruments()->size() ==
			 m_pDrumkit->getInstruments()->size() ) {
			for ( int nnIdx = 0; nnIdx < pOldKit->getInstruments()->size(); ++nnIdx ) {
				auto pOldInstrument = pOldKit->getInstruments()->get( nnIdx );
				auto pNewInstrument = m_pDrumkit->getInstruments()->get( nnIdx );
				if ( pOldInstrument->getType().isEmpty() &&
					 ! pNewInstrument->getType().isEmpty() &&
					 pOldInstrument->getId() == pNewInstrument->getId() ) {
					// First type assignment.
					newInstrumentTypes.push_back( {
							pNewInstrument, pOldInstrument->getType() } );

					// Apply this type to all affected notes.
					for ( const auto& ppPattern : *pSong->getPatternList() ) {
						if ( ppPattern == nullptr ) {
							continue;
						}

						for ( const auto& ppNote :
								  ppPattern->getAllNotesOfType( "" ) ) {
							if ( ppNote != nullptr &&
								 ppNote->getType().isEmpty() &&
								 ppNote->getInstrumentId() ==
								 pOldInstrument->getId() ) {
								notesToBeMapped.push_back( {
									ppNote,
									pNewInstrument->getType(),
									pSong->getPatternList()->index( ppPattern ) } );
							}
						}
					}
				}
			}
		}

		if ( notesToBeMapped.size() > 0 ) {
			pHydrogenApp->beginUndoMacro(
				pCommonStrings->getActionEditDrumkitProperties() );
		}

		for ( const auto& [ ppNote, ssType, nnPatternNumber ] : notesToBeMapped ) {
			pHydrogenApp->pushUndoCommand(
				new SE_editNotePropertiesAction(
					PatternEditor::Property::Type,
					nnPatternNumber,
					ppNote->getPosition(),
					ppNote->getInstrumentId(),
					ppNote->getInstrumentId(),
					ssType,
					"",
					ppNote->getVelocity(),
					ppNote->getVelocity(),
					ppNote->getPan(),
					ppNote->getPan(),
					ppNote->getLeadLag(),
					ppNote->getLeadLag(),
					ppNote->getProbability(),
					ppNote->getProbability(),
					ppNote->getLength(),
					ppNote->getLength(),
					ppNote->getKey(),
					ppNote->getKey(),
					ppNote->getOctave(),
					ppNote->getOctave() ) );
		}

		for ( const auto& [ ppInstrument, ssOldType ] :
				  newInstrumentTypes ) {
			pHydrogenApp->pushUndoCommand(
				new SE_setInstrumentTypeAction(
					ppInstrument->getId(), ppInstrument->getType(), ssOldType,
					ppInstrument->getName() ) );
		}

		// When editing the properties of the current kit, the new version will
		// be loaded in a way that can be undone.
		//
		// TODO this affects mostly metadata and can be done more efficiently.
		// But due to the license propagation into the instruments, we switch
		// the entire kit for now.
		pHydrogenApp->pushUndoCommand(
			new SE_switchDrumkitAction(
				m_pDrumkit, pSong->getDrumkit(),
				SE_switchDrumkitAction::Type::EditProperties ) );

		if ( notesToBeMapped.size() > 0 ) {
			pHydrogenApp->endUndoMacro( "" );
		}

		// Since we hit save on the song's drumkit, we should also save the song
		// for the sake of consistency.
		pHydrogenApp->getMainForm()->action_file_save( "", false );

		if ( m_bEditingNotSaving ) {
			// We are not saving the kit to the Sound Library and are done for
			// now.
			accept();
			pHydrogenApp->showStatusBarMessage(
				pCommonStrings->getActionEditCurrentDrumkitProperties() );
			return;
		}

		// We are saving the drumkit.
	}

	// Store the drumkit in the NSM session folder
#ifdef H2CORE_HAVE_OSC
	if ( m_bSaveToNsmSession && m_pDrumkit->getContext() == Drumkit::Context::Song ) {
		m_pDrumkit->setPath(
			QDir( NsmClient::get_instance()->getSessionFolderPath() )
			.absoluteFilePath( m_pDrumkit->getName() ) );
#else
	if ( false ) {
#endif
	} // Read-only and song kits we can only duplicate into the user folder.
	else if ( m_pDrumkit->getContext() == Drumkit::Context::SessionReadOnly ||
			  m_pDrumkit->getContext() == Drumkit::Context::System ||
			  m_pDrumkit->getContext() == Drumkit::Context::Song ) {
		m_pDrumkit->setPath(
			Filesystem::drumkit_usr_path( m_pDrumkit->getName() ) );
	}

	// Check whether there is already a kit present we would overwrite.
	if ( Filesystem::dir_exists( m_pDrumkit->getPath(), true ) ) {
		int nRes = QMessageBox::information(
			this, "Hydrogen",
			QString( "%1\n%2\n\n%3" )
			/*: asked when saving a drumkit to a certain location */
			.arg( tr( "Overwrite existing drumkit stored in" ) )
			.arg( m_pDrumkit->getPath() )
			.arg( pCommonStrings->getActionIrreversible() ),
			QMessageBox::Yes | QMessageBox::No );
		if ( nRes != QMessageBox::Yes ) {
			INFOLOG( "Aborted by user to not overwrite drumkit" );
			return;
		}
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	// Write new properties/drumkit to disk.
	if ( ! m_pDrumkit->save() ) {

		QApplication::restoreOverrideCursor();
		QMessageBox::information( this, "Hydrogen", tr ( "Saving of this drumkit failed."));
		ERRORLOG( "Saving of this drumkit failed." );
		return;
	}

	if ( sOldPath != m_pDrumkit->getPath() ) {
		if ( m_pDrumkit->getContext() == Drumkit::Context::Song ) {
			pHydrogenApp->showStatusBarMessage(
				QString( "%1 -> [%2]" )
				.arg( pCommonStrings->getActionSaveCurrentDrumkit() )
				.arg( m_pDrumkit->getPath() ) );
		}
		else {
			pHydrogenApp->showStatusBarMessage(
				QString( "%1 [%2] -> [%3]" )
				.arg( pCommonStrings->getActionSaveDrumkit() )
				.arg( m_pDrumkit->getName() ).arg( m_pDrumkit->getPath() ) );
		}
	}
	else {
		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2]" )
			.arg( pCommonStrings->getActionEditDrumkitProperties() )
			.arg( m_pDrumkit->getName() ) );
	}

	// Copy the selected image into the drumkit folder (in case a file outside
	// of it was selected.)
	if ( ! sNewImagePath.isEmpty() ) {

		QFileInfo fileInfo( sNewImagePath );

		if ( fileInfo.dir().absolutePath() != m_pDrumkit->getPath() ) {
			const QString sTargetPath =
				QDir( m_pDrumkit->getPath() ).absoluteFilePath(fileInfo.fileName() );

			// Logging is done in file_copy.
			Filesystem::file_copy( sNewImagePath, sTargetPath, true, false );
		}
	}

	if ( ! sOldImagePath.isEmpty() && ! bOldImageDeleted ) {
		Filesystem::rm( sOldImagePath, false, false );
	}

	pHydrogen->getSoundLibraryDatabase()->updateDrumkits();

	QApplication::restoreOverrideCursor();

	accept();

}

void DrumkitPropertiesDialog::highlightDuplicates() {
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();
	QStringList duplicates;

	const QString sHighlight = QString( "color: %1; background-color: %2" )
		.arg( pColorTheme->m_buttonRedTextColor.name() )
		.arg( pColorTheme->m_buttonRedColor.name() );

	// Compile a list of all duplicated types.
	std::set<QString> types;
	for ( int ii = 0; ii < typesTable->rowCount(); ++ii ) {
		auto ppType = dynamic_cast<LCDCombo*>(typesTable->cellWidget( ii, 2 ));
		if ( ppType != nullptr ) {
			const auto [ _, bSuccess ] = types.insert( ppType->currentText() );
			if ( ! bSuccess ) {
				duplicates << ppType->currentText();
			}
		}
	}

	// Highlight the corresponding combo boxes
	for ( int ii = 0; ii < typesTable->rowCount(); ++ii ) {
		auto ppType = dynamic_cast<LCDCombo*>(typesTable->cellWidget( ii, 2 ));
		if ( ppType != nullptr ) {
			if ( duplicates.contains( ppType->currentText() ) ) {
				ppType->setStyleSheet( sHighlight );
			}
			else {
				ppType->setStyleSheet( "" );
			}
		}
	}
}

}
