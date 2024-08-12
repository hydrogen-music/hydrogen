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

PatternPropertiesDialog::PatternPropertiesDialog(QWidget* parent, Pattern *pattern, int nselectedPattern, bool savepattern)
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

	versionLabel->setText( pCommonStrings->getVersionDialog() );
	licenseLabel->setText( pCommonStrings->getLicenseDialog() );
	authorLabel->setText( pCommonStrings->getAuthorDialog() );

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
		patternDescTxt->setText( pattern->get_info() );
		patternNameTxt->setText( pattern->get_name() );
		defaultNameCheck( pattern->get_name(), savepattern );

		sCategory = pattern->get_category();
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
	cancelBtn->setFixedFontSize( 12 );
	cancelBtn->setSize( QSize( 70, 23 ) );
	cancelBtn->setBorderRadius( 3 );
	cancelBtn->setType( Button::Type::Push );
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
	const int nVersion = versionSpinBox->value();
	QString sPattName = patternNameTxt->text();
	const QString sPattCategory = categoryComboBox->currentText();
	const QString sPattInfo = patternDescTxt->toPlainText();

	// Ensure the pattern name is unique
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	sPattName = pPatternList->find_unused_pattern_name(sPattName, pattern);

	auto pPref = H2Core::Preferences::get_instance();

	if ( pPref->m_patternCategories.contains( sPattCategory ) ) {
		pPref->m_patternCategories.push_back( sPattCategory );
	}

	if( __savepattern ){
		if ( pattern->getVersion() != nVersion ) {
			pattern->setVersion( nVersion );
		}
		pattern->set_name( sPattName );
		pattern->set_info( sPattInfo );
		pattern->set_category( sPattCategory );
	}
	else if ( pattern->getVersion() != nVersion ||
			  pattern->get_name() != sPattName ||
			  pattern->get_info() != sPattInfo ||
			  pattern->get_category() != sPattCategory ) {
		SE_modifyPatternPropertiesAction *action = new SE_modifyPatternPropertiesAction(
			pattern->getVersion(), pattern->get_name(), pattern->get_info(),
			pattern->get_category(), nVersion, sPattName, sPattInfo,
			sPattCategory, __nselectedPattern );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}
	accept();
}

void PatternPropertiesDialog::defaultNameCheck( const QString& pattName, bool savepattern)
{
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	if ( savepattern && !pPatternList->check_name(pattName, pattern) ) {
		patternNameTxt->setText(
			pPatternList->find_unused_pattern_name(pattName, pattern));
	}
	else {
		patternNameTxt->setText(pattName);
	}

}
