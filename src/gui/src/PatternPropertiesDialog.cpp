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

#include "PatternPropertiesDialog.h"

#include "HydrogenApp.h"
#include "UndoActions.h"

#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

using namespace H2Core;

PatternPropertiesDialog::PatternPropertiesDialog( QWidget* parent,
												  std::shared_ptr<Pattern> pattern,
												  int nselectedPattern,
												  bool savepattern)
 : QDialog(parent)
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setupUi( this );
	setWindowTitle( tr( "Pattern properties" ) );

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	this->pattern = pattern;

	// Remove size constraints
	versionSpinBox->setFixedSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
	versionSpinBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	// Arbitrary high number.
	versionSpinBox->setMaximum( 300 );
	// Allow to focus the widget using mouse wheel and tab
	versionSpinBox->setFocusPolicy( Qt::WheelFocus );
	licenseComboBox->setFocusPolicy( Qt::WheelFocus );
	categoryComboBox->setFocusPolicy( Qt::WheelFocus );
	okBtn->setFocusPolicy( Qt::WheelFocus );
	cancelBtn->setFocusPolicy( Qt::WheelFocus );

	// Allow to save the dialog by pressing Return.
	okBtn->setFocus();

	nameLabel->setText( pCommonStrings->getNameDialog() );
	versionLabel->setText( pCommonStrings->getVersionDialog() );
	licenseLabel->setText( pCommonStrings->getLicenseDialog() );
	authorLabel->setText( pCommonStrings->getAuthorDialog() );
	notesLabel->setText( pCommonStrings->getNotesDialog() );

	patternNameTxt->selectAll();

	setupLicenseComboBox( licenseComboBox );

	QString sCategory;
	if ( pattern != nullptr ) {
		versionSpinBox->setValue( pattern->getVersion() );
		authorTxt->setText( pattern->getAuthor() );
		licenseComboBox->setCurrentIndex(
			static_cast<int>( pattern->getLicense().getType() ) );
		licenseStringTxt->setText( pattern->getLicense().getLicenseString() );
		if ( pattern->getLicense().getType() == License::Unspecified ) {
			licenseStringTxt->hide();
		}
		patternDescTxt->setText( pattern->getInfo() );
		patternNameTxt->setText( pattern->getName() );
		defaultNameCheck( pattern->getName(), savepattern );

		sCategory = pattern->getCategory();
	}

	connect( licenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( licenseComboBoxChanged( int ) ) );

	licenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	licenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip() );

	__nselectedPattern = nselectedPattern;
	__savepattern = savepattern;

	if ( sCategory.isEmpty() ){
		sCategory = SoundLibraryDatabase::m_sPatternBaseCategory;
	}
	categoryComboBox->addItem( sCategory );

	const auto pPref = H2Core::Preferences::get_instance();
	for ( const auto& ssCategory : pPref->m_patternCategories ) {
		if ( categoryComboBox->currentText() != ssCategory ){
			categoryComboBox->addItem( ssCategory );
		}
	}

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
}

PatternPropertiesDialog::~PatternPropertiesDialog() {
}

void PatternPropertiesDialog::licenseComboBoxChanged( int ) {

	licenseStringTxt->setText( License::LicenseTypeToQString(
		static_cast<License::LicenseType>( licenseComboBox->currentIndex() ) ) );

	if ( licenseComboBox->currentIndex() == static_cast<int>( License::Unspecified ) ) {
		licenseStringTxt->hide();
	}
	else {
		licenseStringTxt->show();
	}
}

void PatternPropertiesDialog::on_cancelBtn_clicked()
{
	reject();
}


void PatternPropertiesDialog::on_okBtn_clicked()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const int nVersion = versionSpinBox->value();
	const QString sAuthor = authorTxt->text();
	QString sPattName = patternNameTxt->text();
	const License license( licenseStringTxt->text() );
	const QString sPattCategory = categoryComboBox->currentText();
	const QString sPattInfo = patternDescTxt->toPlainText();

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


	// Ensure the pattern name is unique
	auto pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	sPattName = pPatternList->findUnusedPatternName(sPattName, pattern);

	auto pPref = H2Core::Preferences::get_instance();

	if ( pPref->m_patternCategories.contains( sPattCategory ) ) {
		pPref->m_patternCategories.push_back( sPattCategory );
	}

	if( __savepattern ){
		if ( pattern->getVersion() != nVersion ) {
			pattern->setVersion( nVersion );
		}
		pattern->setName( sPattName );
		pattern->setAuthor( sAuthor );
		pattern->setInfo( sPattInfo );
		pattern->setLicense( license );
		pattern->setCategory( sPattCategory );
	}
	else if ( pattern->getVersion() != nVersion ||
			  pattern->getName() != sPattName  ||
			  pattern->getAuthor() != sAuthor   ||
			  pattern->getInfo() != sPattInfo  ||
			  pattern->getLicense() != license  ||
			  pattern->getCategory() != sPattCategory ) {
		SE_modifyPatternPropertiesAction *action =
			new SE_modifyPatternPropertiesAction(
				pattern->getVersion(),
				pattern->getName(),
				pattern->getAuthor(),
				pattern->getInfo(),
				pattern->getLicense(),
				pattern->getCategory(),
				nVersion,
				sPattName,
				sAuthor,
				sPattInfo,
				license,
				sPattCategory,
				__nselectedPattern );
		HydrogenApp::get_instance()->pushUndoCommand( action );
	}
	accept();
}

void PatternPropertiesDialog::defaultNameCheck( const QString& pattName, bool savepattern)
{
	auto pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	if ( savepattern && !pPatternList->checkName(pattName, pattern) ) {
		patternNameTxt->setText(
			pPatternList->findUnusedPatternName(pattName, pattern));
	}
	else {
		patternNameTxt->setText(pattName);
	}

}
