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
#include "Widgets/Button.h"
#include "Widgets/FileDialog.h"
#include "Widgets/LCDCombo.h"
#include "Widgets/LCDDisplay.h"
#include "Widgets/LCDSpinBox.h"
#include "Widgets/LCDTextEdit.h"
#include "Widgets/TagEdit.h"

#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSpacerItem>
#include <QVBoxLayout>

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
	auto pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	if ( action & Action::Duplicate ) {
		setWindowTitle( pCommonStrings->getActionDuplicatePattern() );
	}
	else if ( action & Action::SaveAs ) {
		setWindowTitle( pCommonStrings->getActionSavePatternAs() );
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

	resize( 657, 542 );

	// Overall layout
	auto pOverallLayout = new QVBoxLayout( this );
	pOverallLayout->setSpacing( 0 );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	setLayout( pOverallLayout );

	auto pScrollArea = new QScrollArea( this );
	pScrollArea->setWidgetResizable( true );
	pOverallLayout->addWidget( pScrollArea );

	auto pScrollAreaContent = new QWidget( pScrollArea );
	pScrollAreaContent->setMinimumSize( 652, 534 );
	pScrollArea->setWidget( pScrollAreaContent );

	auto pOuterLayout = new QVBoxLayout();
	pOuterLayout->setSpacing( 0 );
	pOuterLayout->setContentsMargins( 0, 0, 0, 0 );
	pScrollAreaContent->setLayout( pOuterLayout );

	auto pFormContainer = new QWidget( pScrollAreaContent );
	pFormContainer->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Expanding
	);
	pOuterLayout->addWidget( pFormContainer );

	auto pGridLayout = new QGridLayout( pFormContainer );
	pFormContainer->setLayout( pGridLayout );

	auto pPathLabel = new QLabel( pFormContainer );
	pGridLayout->addWidget( pPathLabel, 0, 0 );

	auto pPathContainer = new QWidget( pFormContainer );
	auto pPathContainerLayout = new QHBoxLayout( pPathContainer );
	pPathContainerLayout->setSpacing( 0 );
	pPathContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pPathContainer->setLayout( pPathContainerLayout );

	m_pPathEdit = new LCDDisplay( pPathContainer );
	pPathContainerLayout->addWidget( m_pPathEdit );

	m_pPathBrowseButton = new Button( pPathContainer );
	m_pPathBrowseButton->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Preferred
	);
	m_pPathBrowseButton->setMaximumWidth( 120 );
	pPathContainerLayout->addWidget( m_pPathBrowseButton );

	pGridLayout->addWidget( pPathContainer, 0, 1 );

	auto pNameLabel = new QLabel( pFormContainer );
	pGridLayout->addWidget( pNameLabel, 1, 0 );

	m_pPatternNameTxt = new LCDDisplay( pFormContainer );
	pGridLayout->addWidget( m_pPatternNameTxt, 1, 1 );

	auto pVersionLabel = new QLabel( pFormContainer );
	pGridLayout->addWidget( pVersionLabel, 2, 0 );

	m_pVersionSpinBox = new LCDSpinBox( pFormContainer );
	pGridLayout->addWidget( m_pVersionSpinBox, 2, 1 );

	auto pAuthorLabel = new QLabel( pFormContainer );
	pGridLayout->addWidget( pAuthorLabel, 3, 0 );

	m_pAuthorTxt = new LCDDisplay( pFormContainer );
	pGridLayout->addWidget( m_pAuthorTxt, 3, 1 );

	auto pLicenseLabel = new QLabel( pFormContainer );
	pLicenseLabel->setMinimumHeight( 20 );
	pGridLayout->addWidget( pLicenseLabel, 4, 0 );

	m_pLicenseComboBox = new LCDCombo( pFormContainer );
	m_pLicenseComboBox->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed
	);
	pGridLayout->addWidget( m_pLicenseComboBox, 4, 1 );

	m_pLicenseStringTxt = new LCDDisplay( pFormContainer );
	pGridLayout->addWidget( m_pLicenseStringTxt, 5, 1 );

	auto pNotesLabel = new QLabel( pFormContainer );
	pGridLayout->addWidget( pNotesLabel, 6, 0 );

	m_pPatternDescTxt = new LCDTextEdit( pFormContainer );
	pGridLayout->addWidget( m_pPatternDescTxt, 6, 1 );

	auto pTagsLabel = new QLabel( pFormContainer );
	pGridLayout->addWidget( pTagsLabel, 7, 0 );

	m_pTagEdit = new TagEdit( pFormContainer );
	pGridLayout->addWidget( m_pTagEdit, 7, 1 );

	// Bottom button bar
	auto pButtonLayout = new QHBoxLayout();
	pButtonLayout->addSpacerItem(
		new QSpacerItem( 37, 28, QSizePolicy::Expanding, QSizePolicy::Minimum )
	);

	m_pCancelBtn = new Button( pScrollAreaContent );
	pButtonLayout->addWidget( m_pCancelBtn );

	m_pOkBtn = new Button( pScrollAreaContent );
	m_pOkBtn->setDefault( true );
	pButtonLayout->addWidget( m_pOkBtn );

	pButtonLayout->addSpacerItem(
		new QSpacerItem( 37, 28, QSizePolicy::Expanding, QSizePolicy::Minimum )
	);
	pOuterLayout->addLayout( pButtonLayout );

	// ---- Widget configuration ----

	// Remove size constraints
	m_pVersionSpinBox->setFixedSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
	m_pVersionSpinBox->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed
	);
	// Arbitrary high number.
	m_pVersionSpinBox->setMaximum( 300 );
	// Allow to focus the widget using mouse wheel and tab
	m_pVersionSpinBox->setFocusPolicy( Qt::WheelFocus );
	m_pLicenseComboBox->setFocusPolicy( Qt::WheelFocus );
	m_pOkBtn->setFocusPolicy( Qt::WheelFocus );
	m_pCancelBtn->setFocusPolicy( Qt::WheelFocus );

	// Allow to save the dialog by pressing Return.
	m_pOkBtn->setFocus();

	pPathLabel->setText( pCommonStrings->getPathDialog() );
	pNameLabel->setText( pCommonStrings->getNameDialog() );
	pVersionLabel->setText( pCommonStrings->getVersionDialog() );
	pLicenseLabel->setText( pCommonStrings->getLicenseDialog() );
	pAuthorLabel->setText( pCommonStrings->getAuthorDialog() );
	pNotesLabel->setText( pCommonStrings->getNotesDialog() );

	m_pPatternNameTxt->selectAll();

	setupLicenseComboBox( m_pLicenseComboBox );

	QStringList tags;
	if ( pPattern != nullptr ) {
		m_pPathEdit->setText( pPattern->getPath() );
		m_pVersionSpinBox->setValue( pPattern->getVersion() );
		m_pAuthorTxt->setText( pPattern->getAuthor() );
		m_pLicenseComboBox->setCurrentIndex(
			static_cast<int>( pPattern->getLicense().getType() )
		);
		m_pLicenseStringTxt->setText( pPattern->getLicense().getLicenseString()
		);
		if ( pPattern->getLicense().getType() == License::Unspecified ) {
			m_pLicenseStringTxt->hide();
		}
		m_pPatternDescTxt->setText( pPattern->getInfo() );
		m_pPatternNameTxt->setText( pPattern->getName() );
		// Only if we duplicate the pattern in the pattern list of the current
		// song (Action::ModifyViaUndo) we care about uniqueness. This approach
		// would scale badly within the sound library.
		if ( ( action & Action::ModifyViaUndo ) &&
			 ( action & Action::Duplicate ) ) {
			auto pPatternList =
				Hydrogen::get_instance()->getSong()->getPatternList();
			m_pPatternNameTxt->setText(
				pPatternList->findUnusedPatternName( pPattern->getName() )
			);
		}

		tags = pPattern->getTags();
	}

	if ( action & Action::SaveAs ) {
		m_pPathEdit->setIsActive( true );
		m_pPathBrowseButton->setVisible( true );
		m_pOkBtn->setEnabled(
			m_pPathEdit->text().endsWith( Filesystem::sPatternSuffix )
		);
		connect(
			m_pPathEdit, &QLineEdit::textChanged,
			[&]( const QString& sNewText ) {
				m_pOkBtn->setEnabled( sNewText.endsWith( Filesystem::sPatternSuffix
				) );
			}
		);
	}
	else {
		m_pPathEdit->setIsActive( false );
		m_pPathBrowseButton->setVisible( false );
	}

	connect(
		m_pLicenseComboBox, SIGNAL( currentIndexChanged( int ) ), this,
		SLOT( licenseComboBoxChanged( int ) )
	);

	m_pLicenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	m_pLicenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip()
	);

	pTagsLabel->setText( pCommonStrings->getTagsLabel() );
	m_pTagEdit->setTags( pPattern->getTags() );

	if ( !m_pPathEdit->text().isEmpty() &&
		 !Filesystem::fileWritable( m_pPathEdit->text(), true ) &&
		 !( action & Action::SaveAs ) ) {
		// Read-only artifact
		QString sToolTip = pCommonStrings->getArtifactIsReadOnly();

		m_pPathEdit->setIsActive( false );
		m_pPatternNameTxt->setIsActive( false );
		m_pVersionSpinBox->setIsActive( false );
		m_pAuthorTxt->setIsActive( false );
		m_pLicenseComboBox->setIsActive( false );
		m_pLicenseStringTxt->setIsActive( false );
		m_pPatternDescTxt->setIsActive( false );
		m_pTagEdit->setEnabled( false );
		m_pOkBtn->setIsActive( false );
		m_pPathBrowseButton->setIsActive( false );

		m_pPathEdit->setToolTip( sToolTip );
		m_pPatternNameTxt->setToolTip( sToolTip );
		m_pVersionSpinBox->setToolTip( sToolTip );
		m_pAuthorTxt->setToolTip( sToolTip );
		m_pLicenseComboBox->setToolTip( sToolTip );
		m_pLicenseStringTxt->setToolTip( sToolTip );
		m_pPatternDescTxt->setToolTip( sToolTip );
		m_pTagEdit->setToolTip( sToolTip );
		m_pOkBtn->setToolTip( sToolTip );
		m_pPathBrowseButton->setToolTip( sToolTip );

		// Rather dirty fix to align the design of the QTextEdit to
		// the coloring of our custom QLineEdits.
		m_pPatternDescTxt->setStyleSheet(
			QString( "\
QTextEdit { \
    color: %1; \
    background-color: %2; \
}" )
				.arg( pPref->getColorTheme()->m_windowTextColor.name() )
				.arg( pPref->getColorTheme()->m_windowColor.name() )
		);
	}

	auto styleButton = [&]( Button* pButton ) {
		pButton->setFixedFontSize( 12 );
		pButton->setSize( QSize( 70, 23 ) );
		pButton->setBorderRadius( 3 );
		pButton->setType( Button::Type::Push );
	};

	styleButton( m_pOkBtn );
	m_pOkBtn->setText( pCommonStrings->getButtonOk() );
	styleButton( m_pCancelBtn );
	m_pCancelBtn->setText( pCommonStrings->getButtonCancel() );
	styleButton( m_pPathBrowseButton );
	m_pPathBrowseButton->setText( pCommonStrings->getButtonBrowse() );

	// Explicit button connections (previously auto-connected by setupUi)
	connect( m_pOkBtn, SIGNAL( clicked() ), this, SLOT( on_okBtn_clicked() ) );
	connect(
		m_pCancelBtn, SIGNAL( clicked() ), this, SLOT( on_cancelBtn_clicked() )
	);
	connect( m_pPathBrowseButton, &QPushButton::clicked, [&]() {
		const QString sPath = m_pPathEdit->text();
		QFileInfo info( sPath );
		QString sDir = info.absoluteDir().absolutePath();
		if ( sDir.isEmpty() || !Filesystem::dirWritable( sDir, false ) ) {
			sDir = Filesystem::userDataPath();
		}

		FileDialog fd( this );

		fd.setFileMode( QFileDialog::AnyFile );
		fd.setNameFilter( Filesystem::sPatternFilter );
		fd.setDirectory( sDir );
		fd.setWindowTitle( windowTitle() );
		fd.setAcceptMode( QFileDialog::AcceptSave );
		fd.selectFile( info.fileName() );

		if ( fd.exec() != QDialog::Accepted ) {
			return;
		}

		QString sFilePath = fd.selectedFiles().first();
		if ( sFilePath.isEmpty() ) {
			return;
		}

		if ( sFilePath.endsWith( Filesystem::sPatternSuffix ) == false ) {
			sFilePath += Filesystem::sPatternSuffix;
		}

		m_pPathEdit->setText( sFilePath );
	} );
}

PatternPropertiesDialog::~PatternPropertiesDialog()
{
}

void PatternPropertiesDialog::licenseComboBoxChanged( int )
{
	m_pLicenseStringTxt->setText( License::LicenseTypeToQString(
		static_cast<License::LicenseType>( m_pLicenseComboBox->currentIndex() )
	) );

	if ( m_pLicenseComboBox->currentIndex() ==
		 static_cast<int>( License::Unspecified ) ) {
		m_pLicenseStringTxt->hide();
	}
	else {
		m_pLicenseStringTxt->show();
	}
}

void PatternPropertiesDialog::on_cancelBtn_clicked()
{
	reject();
}

void PatternPropertiesDialog::on_okBtn_clicked()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const int nVersion = m_pVersionSpinBox->value();
	const QString sAuthor = m_pAuthorTxt->text();
	QString sPattName = m_pPatternNameTxt->text();
	QString sNewLicenseString( m_pLicenseStringTxt->text() );
	if ( m_pLicenseComboBox->currentIndex() ==
		 static_cast<int>(License::Unspecified) ) {
		sNewLicenseString = "";
	}
	const License license( sNewLicenseString );
	const QStringList tags = m_pTagEdit->getTags();
	const QString sPattInfo = m_pPatternDescTxt->toPlainText();

	// Sanity checks.
	//
	// Check whether the license strings from the line edits comply to
	// the license types selected in the combo boxes.
	License licenseCheck( m_pLicenseStringTxt->text() );
	if ( static_cast<int>( licenseCheck.getType() ) !=
		 m_pLicenseComboBox->currentIndex() ) {
		if ( QMessageBox::warning(
				 this, "Hydrogen",
				 pCommonStrings->getLicenseMismatchingUserInput(),
				 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
			 ) == QMessageBox::Cancel ) {
			WARNINGLOG( QString( "Abort, since drumkit License String [%1] "
								 "does not comply to selected License Type [%2]"
			)
							.arg( m_pLicenseStringTxt->text() )
							.arg( License::LicenseTypeToQString(
								static_cast<License::LicenseType>(
									m_pLicenseComboBox->currentIndex()
								)
							) ) );
			return;
		}
	}

	if ( ( m_action & Action::SaveAs ) &&
		 ( m_pPattern->getPath() != m_pPathEdit->text() &&
		   !m_pPathEdit->text().isEmpty() ) ) {
		if ( !Filesystem::isPathValid(
				 Filesystem::Artifact::Pattern, m_pPathEdit->text(), false
			 ) ) {
			QMessageBox::critical(
				this, "Hydrogen",
				QString( "[%1]\n\n%2" )
					.arg( m_pPathEdit->text() )
					.arg( pCommonStrings->getErrorInvalidPath() )
			);
			return;
		}
	}

	if ( ( m_action & Action::ModifyViaUndo ) &&
		 ( ! ( m_action & Action::Duplicate ) ) &&
		 ( m_pPattern->getPath() != m_pPathEdit->text() ||
		   m_pPattern->getVersion() != nVersion ||
		   m_pPattern->getName() != sPattName ||
		   m_pPattern->getAuthor() != sAuthor ||
		   m_pPattern->getInfo() != sPattInfo ||
		   m_pPattern->getLicense() != license ||
		   m_pPattern->getTags() != tags ) ) {

		// Within the current song we need to take extra care for the current
		// song to have an unique name.
		auto pPatternList =
			Hydrogen::get_instance()->getSong()->getPatternList();
		sPattName = pPatternList->findUnusedPatternName(
			sPattName, m_nSelectedPattern
		);

		SE_modifyPatternPropertiesAction* action =
			new SE_modifyPatternPropertiesAction(
				m_pPattern->getPath(), m_pPattern->getVersion(),
				m_pPattern->getName(), m_pPattern->getAuthor(),
				m_pPattern->getInfo(), m_pPattern->getLicense(),
				m_pPattern->getTags(), m_pPathEdit->text(), nVersion, sPattName,
				sAuthor, sPattInfo, license, tags, m_nSelectedPattern
			);
		HydrogenApp::get_instance()->pushUndoCommand( action );
	}

	if ( ( m_action & Action::SaveAs ) &&
		 m_pPattern->getPath() != m_pPathEdit->text() ) {
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

	accept();
}
