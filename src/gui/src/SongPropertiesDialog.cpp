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

#include "SongPropertiesDialog.h"

#include "CommonStrings.h"
#include "HydrogenApp.h"

#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
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
	// Allow to focus the widget using mouse wheel and tab
	versionSpinBox->setFocusPolicy( Qt::WheelFocus );
	licenseComboBox->setFocusPolicy( Qt::WheelFocus );
	okBtn->setFocusPolicy( Qt::WheelFocus );
	cancelBtn->setFocusPolicy( Qt::WheelFocus );

	// Allow to save the dialog by pressing Return.
	okBtn->setFocus();

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

	tabWidget->setTabText( 0, pCommonStrings->getTabGeneralDialog() );
	tabWidget->setTabText( 1, pCommonStrings->getTabLicensesDialog() );
	tabWidget->setCurrentIndex( 0 );

	nameLabel->setText( pCommonStrings->getNameDialog() );
	authorLabel->setText( pCommonStrings->getAuthorDialog() );
	licenseLabel->setText( pCommonStrings->getLicenseDialog() );
	licenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	licenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip() );
	notesLabel->setText( pCommonStrings->getNotesDialog() );

	okBtn->setFixedFontSize( 12 );
	okBtn->setSize( QSize( 70, 23 ) );
	okBtn->setBorderRadius( 3 );
	okBtn->setType( Button::Type::Push );
	okBtn->setIsActive( true );
	okBtn->setText( pCommonStrings->getButtonOk() );
	cancelBtn->setFixedFontSize( 12 );
	cancelBtn->setSize( QSize( 70, 23 ) );
	cancelBtn->setBorderRadius( 3 );
	cancelBtn->setType( Button::Type::Push );
	cancelBtn->setText( pCommonStrings->getButtonCancel() );

	updatePatternLicenseTable();
}

SongPropertiesDialog::~SongPropertiesDialog() {
}

void SongPropertiesDialog::updatePatternLicenseTable() {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pColorTheme = H2Core::Preferences::get_instance()->getColorTheme();
	const auto pSong = H2Core::Hydrogen::get_instance()->getSong();

	licensesTable->setColumnCount( 4 );
	licensesTable->setHorizontalHeaderLabels(
		QStringList() <<
		pCommonStrings->getNameDialog() <<
		pCommonStrings->getVersionDialog() <<
		pCommonStrings->getAuthorDialog() <<
		pCommonStrings->getLicenseDialog() );
	licensesTable->verticalHeader()->hide();
	licensesTable->horizontalHeader()->setStretchLastSection( true );
	licensesTable->setColumnWidth( 0, 210 );
	licensesTable->setColumnWidth( 1, 60 );
	licensesTable->setColumnWidth( 2, 140 );

	if ( pSong == nullptr ){
		return;
	}

	const auto pPatternList = pSong->getPatternList();
	licensesTable->setRowCount( pPatternList->size() );

	int nFirstMismatchRow = -1;
	int rrow = 0;
	for ( const auto& ppPattern : *pPatternList ) {
		if ( ppPattern != nullptr ) {

			LCDDisplay* pNameItem = new LCDDisplay( nullptr );
			pNameItem->setText( ppPattern->getName());
			pNameItem->setIsActive( false );
			pNameItem->setToolTip( ppPattern->getName() );
			LCDDisplay* pVersionItem = new LCDDisplay( nullptr );
			pVersionItem->setText( QString::number( ppPattern->getVersion() ) );
			pVersionItem->setIsActive( false );
			pVersionItem->setToolTip( QString::number( ppPattern->getVersion() ) );
			LCDDisplay* pAuthorItem = new LCDDisplay( nullptr );
			pAuthorItem->setText( ppPattern->getAuthor() );
			pAuthorItem->setIsActive( false );
			pAuthorItem->setToolTip( ppPattern->getAuthor() );
			LCDDisplay* pLicenseItem = new LCDDisplay( nullptr );
			pLicenseItem->setText( ppPattern->getLicense().getLicenseString() );
			pLicenseItem->setIsActive( false );
			pLicenseItem->setToolTip( ppPattern->getLicense().getLicenseString() );

			// In case the pattern features a dedicated license and this one
			// does not match the one set for the whole song, we highlight the
			// corresponding row.
			if ( ! ppPattern->getLicense().isEmpty() &&
				 ppPattern->getLicense() != pSong->getLicense() ) {
				QString sHighlight = QString( "color: %1; background-color: %2" )
					.arg( pColorTheme->m_buttonRedTextColor.name() )
					.arg( pColorTheme->m_buttonRedColor.name() );
				pNameItem->setStyleSheet( sHighlight );
				pVersionItem->setStyleSheet( sHighlight );
				pAuthorItem->setStyleSheet( sHighlight );
				pLicenseItem->setStyleSheet( sHighlight );

				if ( nFirstMismatchRow == -1 ) {
					nFirstMismatchRow = rrow;
				}
			}

			licensesTable->setCellWidget( rrow, 0, pNameItem );
			licensesTable->setCellWidget( rrow, 1, pVersionItem );
			licensesTable->setCellWidget( rrow, 2, pAuthorItem );
			licensesTable->setCellWidget( rrow, 3, pLicenseItem );

			++rrow;
		}
	}

	// In case of a mismatch scroll into view
	if ( nFirstMismatchRow != -1 ) {
		licensesTable->showRow( nFirstMismatchRow );
	}
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
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

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
