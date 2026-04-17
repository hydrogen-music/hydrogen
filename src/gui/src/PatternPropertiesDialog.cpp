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

PatternPropertiesDialog::PatternPropertiesDialog(
	QWidget* pParent,
	std::shared_ptr<Pattern> pPattern,
	int nSelectedPattern,
	Action action
)
	: QDialog( pParent ),
	  m_pPattern( pPattern ),
	  m_nSelectedPattern( nSelectedPattern ),
	  m_action( action )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setupUi( this );
	if ( action & Action::Duplicate ) {
		setWindowTitle( pCommonStrings->getActionDuplicatePattern() );
	}
	else {
		setWindowTitle( tr( "Pattern properties" ) );
	}

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags(
		windowFlags() | Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint
	);

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

	m_pPathLabel->setText( pCommonStrings->getPathDialog() );
	nameLabel->setText( pCommonStrings->getNameDialog() );
	versionLabel->setText( pCommonStrings->getVersionDialog() );
	licenseLabel->setText( pCommonStrings->getLicenseDialog() );
	authorLabel->setText( pCommonStrings->getAuthorDialog() );
	notesLabel->setText( pCommonStrings->getNotesDialog() );

	patternNameTxt->selectAll();

	setupLicenseComboBox( licenseComboBox );

	QStringList tags;
	if ( pPattern != nullptr ) {
		m_pPathEdit->setText( pPattern->getPath() );
		versionSpinBox->setValue( pPattern->getVersion() );
		authorTxt->setText( pPattern->getAuthor() );
		licenseComboBox->setCurrentIndex(
			static_cast<int>( pPattern->getLicense().getType() )
		);
		licenseStringTxt->setText( pPattern->getLicense().getLicenseString() );
		if ( pPattern->getLicense().getType() == License::Unspecified ) {
			licenseStringTxt->hide();
		}
		patternDescTxt->setText( pPattern->getInfo() );
		patternNameTxt->setText( pPattern->getName() );
		defaultNameCheck( pPattern->getName(), action & Action::ModifyViaUndo );

		tags = pPattern->getTags();
	}

	m_pPathEdit->setIsActive( action & Action::Duplicate );

	connect(
		licenseComboBox, SIGNAL( currentIndexChanged( int ) ), this,
		SLOT( licenseComboBoxChanged( int ) )
	);

	licenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	licenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip() );

	m_pTagsLabel->setText( pCommonStrings->getTagsLabel() );
	m_pTagEdit->setTags( pPattern->getTags() );

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

PatternPropertiesDialog::~PatternPropertiesDialog()
{
}

void PatternPropertiesDialog::licenseComboBoxChanged( int )
{
	licenseStringTxt->setText( License::LicenseTypeToQString(
		static_cast<License::LicenseType>( licenseComboBox->currentIndex() )
	) );

	if ( licenseComboBox->currentIndex() ==
		 static_cast<int>( License::Unspecified ) ) {
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
	const QStringList tags = m_pTagEdit->getTags();
	const QString sPattInfo = patternDescTxt->toPlainText();

	// Sanity checks.
	//
	// Check whether the license strings from the line edits comply to
	// the license types selected in the combo boxes.
	License licenseCheck( licenseStringTxt->text() );
	if ( static_cast<int>( licenseCheck.getType() ) !=
		 licenseComboBox->currentIndex() ) {
		if ( QMessageBox::warning(
				 this, "Hydrogen",
				 pCommonStrings->getLicenseMismatchingUserInput(),
				 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
			 ) == QMessageBox::Cancel ) {
			WARNINGLOG( QString( "Abort, since drumkit License String [%1] "
								 "does not comply to selected License Type [%2]"
			)
							.arg( licenseStringTxt->text() )
							.arg( License::LicenseTypeToQString(
								static_cast<License::LicenseType>(
									licenseComboBox->currentIndex()
								)
							) ) );
			return;
		}
	}

	// Ensure the pattern name is unique
	auto pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	sPattName = pPatternList->findUnusedPatternName( sPattName, m_pPattern );

	if ( ! ( m_action & Action::ModifyViaUndo ) ) {
		if ( m_action & Action::Duplicate &&
			 m_pPattern->getPath() != m_pPathEdit->text() ) {
			if ( !Filesystem::isPathValid(
					 Filesystem::Artifact::Pattern, m_pPathEdit->text(), false
				 ) ) {
				QMessageBox::critical(
					this, "Hydrogen",
					QString( "[%1]\n\n%2 [%3]" )
						.arg( m_pPathEdit->text() )
						.arg( pCommonStrings->getErrorInvalidPath() )
						.arg( Filesystem::sPatternSuffix )
				);
				return;
			}
			m_pPattern->setPath( m_pPathEdit->text() );
		}
		if ( m_pPattern->getVersion() != nVersion ) {
			m_pPattern->setVersion( nVersion );
		}
		m_pPattern->setName( sPattName );
		m_pPattern->setAuthor( sAuthor );
		m_pPattern->setInfo( sPattInfo );
		m_pPattern->setLicense( license );
		m_pPattern->setTags( tags );
	}
	else if ( m_pPattern->getVersion() != nVersion || m_pPattern->getName() != sPattName || m_pPattern->getAuthor() != sAuthor || m_pPattern->getInfo() != sPattInfo || m_pPattern->getLicense() != license || m_pPattern->getTags() != tags ) {
		SE_modifyPatternPropertiesAction* action =
			new SE_modifyPatternPropertiesAction(
				m_pPattern->getVersion(), m_pPattern->getName(), m_pPattern->getAuthor(),
				m_pPattern->getInfo(), m_pPattern->getLicense(), m_pPattern->getTags(),
				nVersion, sPattName, sAuthor, sPattInfo, license, tags,
				m_nSelectedPattern
			);
		HydrogenApp::get_instance()->pushUndoCommand( action );
	}
	accept();
}

void PatternPropertiesDialog::defaultNameCheck(
	const QString& pattName,
	bool bSavePattern
)
{
	auto pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	if ( bSavePattern && !pPatternList->checkName( pattName, m_pPattern ) ) {
		patternNameTxt->setText(
			pPatternList->findUnusedPatternName( pattName, m_pPattern )
		);
	}
	else {
		patternNameTxt->setText( pattName );
	}
}
