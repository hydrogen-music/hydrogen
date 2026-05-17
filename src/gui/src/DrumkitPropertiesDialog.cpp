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

#include "DrumkitPropertiesDialog.h"

#include "CommonStrings.h"
#include "HydrogenApp.h"
#include "MainForm.h"
#include "PatternEditor/PatternEditor.h"
#include "Rack/SoundLibrary/TypesTable.h"
#include "UndoActions.h"
#include "Widgets/Button.h"
#include "Widgets/FileDialog.h"
#include "Widgets/LCDCombo.h"
#include "Widgets/LCDDisplay.h"
#include "Widgets/LCDSpinBox.h"
#include "Widgets/LCDTextEdit.h"
#include "Widgets/TagEdit.h"

#include <core/Basics/DrumkitMap.h>
#include <core/Basics/InstrumentList.h>
#include <core/Hydrogen.h>
#include <core/NsmClient.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QSpacerItem>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace H2Core {

DrumkitPropertiesDialog::DrumkitPropertiesDialog(
	QWidget* pParent,
	std::shared_ptr<Drumkit> pDrumkit,
	Action action,
	const QString& sTargetPath,
	Instrument::Id id
)
	: QDialog( pParent ), m_pDrumkit( pDrumkit ), m_action( action )
{
	const auto pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setObjectName( "DrumkitPropertiesDialog" );

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags(
		windowFlags() | Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint
	);

	setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
	setSizeGripEnabled( false );

	resize( 759, 926 );

	if ( m_action & Action::NsmSession ) {
		setWindowTitle(
			tr( "Save a copy of the current drumkit to NSM session folder" )
		);
	}
	else if ( m_action & Action::SaveAs ) {
		setWindowTitle( pCommonStrings->getActionSaveDrumkit() );
	}
	else if ( action & Action::Duplicate ) {
		setWindowTitle( pCommonStrings->getMenuActionDuplicate() );
	}
	else if ( m_action & Action::ModifyViaUndo ) {
		setWindowTitle( pCommonStrings->getActionEditCurrentDrumkitProperties()
		);
	}
	else {
		setWindowTitle( pCommonStrings->getActionEditDrumkitProperties() );
	}

	// Overall layout
	auto pOverallLayout = new QVBoxLayout();
	pOverallLayout->setSpacing( 0 );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	setLayout( pOverallLayout );

	auto pScrollArea = new QScrollArea( this );
	pScrollArea->setWidgetResizable( true );
	pOverallLayout->addWidget( pScrollArea );

	auto pScrollAreaContent = new QWidget( pScrollArea );
	pScrollAreaContent->setMinimumSize( 752, 919 );
	pScrollArea->setWidget( pScrollAreaContent );

	auto pVerticalLayout = new QVBoxLayout();
	pScrollAreaContent->setLayout( pVerticalLayout );

	// Tab widget
	m_pTabWidget = new QTabWidget( pScrollAreaContent );
	m_pTabWidget->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Expanding
	);
	pVerticalLayout->addWidget( m_pTabWidget );

	// ---- General tab ----
	auto pTabGeneral = new QWidget( m_pTabWidget );
	pTabGeneral->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Expanding
	);
	m_pTabWidget->addTab( pTabGeneral, QString() );

	auto pGridLayout = new QGridLayout();
	pGridLayout->setColumnStretch( 0, 0 );
	pGridLayout->setColumnStretch( 1, 1 );
	pTabGeneral->setLayout( pGridLayout );

	auto pPathLabel = new QLabel( pTabGeneral );
	pGridLayout->addWidget( pPathLabel, 0, 0 );

	auto pPathContainer = new QWidget( pTabGeneral );
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

	auto pNameLabel = new QLabel( pTabGeneral );
	pNameLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	pGridLayout->addWidget( pNameLabel, 1, 0 );

	m_pNameTxt = new LCDDisplay( pTabGeneral );
	m_pNameTxt->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	pGridLayout->addWidget( m_pNameTxt, 1, 1 );

	auto pVersionLabel = new QLabel( pTabGeneral );
	pGridLayout->addWidget( pVersionLabel, 2, 0 );

	m_pVersionSpinBox = new LCDSpinBox( pTabGeneral );
	pGridLayout->addWidget( m_pVersionSpinBox, 2, 1 );

	auto pAuthorLabel = new QLabel( pTabGeneral );
	pAuthorLabel->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Preferred
	);
	pGridLayout->addWidget( pAuthorLabel, 3, 0 );

	m_pAuthorTxt = new LCDDisplay( pTabGeneral );
	m_pAuthorTxt->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	pGridLayout->addWidget( m_pAuthorTxt, 3, 1 );

	auto pLicenseLbl = new QLabel( pTabGeneral );
	pLicenseLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
	pLicenseLbl->setAlignment(
		Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter
	);
	pLicenseLbl->setWordWrap( true );
	pGridLayout->addWidget( pLicenseLbl, 4, 0 );

	m_pLicenseComboBox = new LCDCombo( pTabGeneral );
	m_pLicenseComboBox->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed
	);
	pGridLayout->addWidget( m_pLicenseComboBox, 4, 1 );

	m_pLicenseStringLbl = new QLabel( pTabGeneral );
	pGridLayout->addWidget( m_pLicenseStringLbl, 5, 0 );

	m_pLicenseStringTxt = new LCDDisplay( pTabGeneral );
	m_pLicenseStringTxt->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed
	);
	pGridLayout->addWidget( m_pLicenseStringTxt, 5, 1 );

	auto pNotesLabel = new QLabel( pTabGeneral );
	pNotesLabel->setAlignment(
		Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter
	);
	pGridLayout->addWidget( pNotesLabel, 6, 0 );

	m_pInfoTxt = new LCDTextEdit( pTabGeneral );
	m_pInfoTxt->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::MinimumExpanding
	);
	m_pInfoTxt->setMinimumHeight( 46 );
	m_pInfoTxt->setAcceptRichText( false );
	pGridLayout->addWidget( m_pInfoTxt, 6, 1 );

	auto pTagsLabel = new QLabel( pTabGeneral );
	pGridLayout->addWidget( pTagsLabel, 7, 0 );

	m_pTagEdit = new TagEdit( pTabGeneral );
	pGridLayout->addWidget( m_pTagEdit, 7, 1 );

	auto pImageLabel = new QLabel( tr( "Image" ), pTabGeneral );
	pImageLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
	pGridLayout->addWidget( pImageLabel, 8, 0 );

	auto pImageContainer = new QWidget( pTabGeneral );
	auto pImageContainerLayout = new QHBoxLayout();
	pImageContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pImageContainer->setLayout( pImageContainerLayout );

	m_pImageText = new LCDDisplay( pImageContainer );
	m_pImageText->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	m_pImageText->setMinimumSize( 370, 21 );
	pImageContainerLayout->addWidget( m_pImageText );

	m_pImageBrowsePushButton = new Button( pImageContainer );
	m_pImageBrowsePushButton->setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Fixed
	);
	pImageContainerLayout->addWidget( m_pImageBrowsePushButton );

	pGridLayout->addWidget( pImageContainer, 8, 1 );

	auto pImageLicenseLbl = new QLabel( tr( "Image License" ), pTabGeneral );
	pImageLicenseLbl->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Minimum
	);
	pImageLicenseLbl->setWordWrap( true );
	pGridLayout->addWidget( pImageLicenseLbl, 9, 0 );

	m_pImageLicenseComboBox = new LCDCombo( pTabGeneral );
	m_pImageLicenseComboBox->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed
	);
	pGridLayout->addWidget( m_pImageLicenseComboBox, 9, 1 );

	m_pImageLicenseStringLbl = new QLabel( pTabGeneral );
	pGridLayout->addWidget( m_pImageLicenseStringLbl, 10, 0 );

	m_pImageLicenseStringTxt = new LCDDisplay( pTabGeneral );
	m_pImageLicenseStringTxt->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed
	);
	pGridLayout->addWidget( m_pImageLicenseStringTxt, 10, 1 );

	m_pDrumkitImageLabel = new QLabel( pTabGeneral );
	m_pDrumkitImageLabel->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Expanding
	);
	m_pDrumkitImageLabel->setMinimumHeight( 75 );
	m_pDrumkitImageLabel->setMaximumHeight( 320 );
	{
		QPalette pal = m_pDrumkitImageLabel->palette();
		QColor transparentColor( 34, 31, 30, 0 );
		pal.setColor(
			QPalette::Active, QPalette::WindowText, transparentColor
		);
		pal.setColor(
			QPalette::Inactive, QPalette::WindowText, transparentColor
		);
		QColor disabledColor( 144, 141, 139, 255 );
		pal.setColor( QPalette::Disabled, QPalette::WindowText, disabledColor );
		m_pDrumkitImageLabel->setPalette( pal );
	}
	m_pDrumkitImageLabel->setAutoFillBackground( true );
	m_pDrumkitImageLabel->setFrameShape( QFrame::StyledPanel );
	m_pDrumkitImageLabel->setScaledContents( false );
	m_pDrumkitImageLabel->setAlignment( Qt::AlignCenter );
	pGridLayout->addWidget( m_pDrumkitImageLabel, 11, 1 );

	// ---- Types tab ----
	auto pTabTypes = new QWidget( m_pTabWidget );
	pTabTypes->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	m_pTabWidget->addTab( pTabTypes, tr( "Types" ) );

	auto pTabTypesLayout = new QVBoxLayout();
	pTabTypesLayout->setSpacing( 0 );
	pTabTypesLayout->setContentsMargins( 0, 0, 0, 0 );
	pTabTypes->setLayout( pTabTypesLayout );

	m_pTypesTable = new TypesTable( pTabTypes );
	m_pTypesTable->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Expanding
	);
	pTabTypesLayout->addWidget( m_pTypesTable );

	// ---- Licenses tab ----
	auto pTabLicenses = new QWidget( m_pTabWidget );
	m_pTabWidget->addTab( pTabLicenses, QString() );

	auto pLicensesLayout = new QHBoxLayout();
	pLicensesLayout->setSpacing( 0 );
	pLicensesLayout->setContentsMargins( 0, 0, 0, 0 );
	pTabLicenses->setLayout( pLicensesLayout );

	m_pLicensesTable = new QTableWidget( pTabLicenses );
	pLicensesLayout->addWidget( m_pLicensesTable );

	// ---- Bottom button bar ----
	auto pButtonLayout = new QHBoxLayout();
	pButtonLayout->addSpacerItem(
		new QSpacerItem( 37, 28, QSizePolicy::Expanding, QSizePolicy::Minimum )
	);

	m_pCancelBtn = new Button( pScrollAreaContent );
	pButtonLayout->addWidget( m_pCancelBtn );

	m_pSaveBtn = new Button( pScrollAreaContent );
	pButtonLayout->addWidget( m_pSaveBtn );

	pButtonLayout->addSpacerItem(
		new QSpacerItem( 37, 28, QSizePolicy::Expanding, QSizePolicy::Minimum )
	);

	pVerticalLayout->addLayout( pButtonLayout );

	// ---- Widget configuration ----

	setupLicenseComboBox( m_pLicenseComboBox );
	setupLicenseComboBox( m_pImageLicenseComboBox );

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
	m_pImageLicenseComboBox->setFocusPolicy( Qt::WheelFocus );
	m_pCancelBtn->setFocusPolicy( Qt::WheelFocus );
	m_pSaveBtn->setFocusPolicy( Qt::WheelFocus );
	m_pImageBrowsePushButton->setFocusPolicy( Qt::WheelFocus );
	m_pImageBrowsePushButton->setText( pCommonStrings->getButtonBrowse() );

	// Allow to save the dialog by pressing Return.
	m_pSaveBtn->setFocus();

	pNameLabel->setText( pCommonStrings->getNameDialog() );
	pVersionLabel->setText( pCommonStrings->getVersionDialog() );
	pNotesLabel->setText( pCommonStrings->getNotesDialog() );

	pPathLabel->setText( pCommonStrings->getPathDialog() );
	pTagsLabel->setText( pCommonStrings->getTagsLabel() );

	if ( ( m_action & Action::NsmSession ) &&
		 !Hydrogen::get_instance()->isUnderSessionManagement() ) {
		ERRORLOG(
			"NSM session export request while there is no active NSM session. "
			"Saving to Sound Library instead."
		);
		m_action = static_cast<Action>( m_action & ~Action::NsmSession );
	}

	bool bWritable = false;
	// display the current drumkit infos into the qlineedit
	if ( pDrumkit != nullptr ) {
		m_pVersionSpinBox->setValue( pDrumkit->getVersion() );
		auto drumkitContext = pDrumkit->getContext();
		if ( drumkitContext == Filesystem::Context::User ||
			 drumkitContext == Filesystem::Context::SessionReadWrite ||
			 drumkitContext == Filesystem::Context::Custom ||
			 drumkitContext == Filesystem::Context::Song ) {
			bWritable = true;
		}

		m_pNameTxt->setText( pDrumkit->getName() );
		pAuthorLabel->setText( pCommonStrings->getAuthorDialog() );
		m_pAuthorTxt->setText( QString( pDrumkit->getAuthor() ) );
		m_pInfoTxt->append( QString( pDrumkit->getInfo() ) );

		if ( pDrumkit->getContext() == Filesystem::Context::Song &&
			 ! ( m_action & Action::SaveAs ) ) {
			// In case the drumkit is not a standalone one but part of a .h2song
			// file, we show the path to that file instead of Drumkit::m_sPath,
			// which is still set to the drumkit file loaded into the song.
			m_pPathEdit->setText( Hydrogen::get_instance()->getSong()->getPath()
			);
		}
		else {
			m_pPathEdit->setText(
				Filesystem::drumkitDirFromPath( sTargetPath )
			);
		}
		m_pTagEdit->setTags( pDrumkit->getTags() );

		License license = pDrumkit->getLicense();
		m_pLicenseComboBox->setCurrentIndex( static_cast<int>( license.getType()
		) );
		m_pLicenseStringTxt->setText( license.getLicenseString() );

		// Will contain a file name in case of an image file located in the
		// drumkit folder or an absolute path in case of one located outside of
		// it (in our cache folder in case of a song kit).
		m_pImageText->setText( pDrumkit->getImage() );
		m_pImageText->setAlignment( Qt::AlignLeft );

		License imageLicense = pDrumkit->getImageLicense();
		m_pImageLicenseComboBox->setCurrentIndex(
			static_cast<int>( imageLicense.getType() )
		);
		m_pImageLicenseStringTxt->setText( imageLicense.getLicenseString() );
	}

	if ( action & Action::SaveAs ) {
		m_pPathEdit->setIsActive( true );
		m_pPathBrowseButton->setVisible( true );
		m_pSaveBtn->setEnabled( !m_pPathEdit->text().isEmpty() );
		connect(
			m_pPathEdit, &QLineEdit::textChanged,
			[&]( const QString& sNewText ) {
				m_pSaveBtn->setEnabled( ! sNewText.isEmpty() );
			}
		);
	}
	else {
		m_pPathEdit->setIsActive( false );
		m_pPathBrowseButton->setVisible( false );
	}

	if ( m_pLicenseComboBox->currentIndex() ==
		 static_cast<int>( License::Unspecified ) ) {
		m_pLicenseStringLbl->hide();
		m_pLicenseStringTxt->hide();
	}
	if ( m_pImageLicenseComboBox->currentIndex() ==
		 static_cast<int>( License::Unspecified ) ) {
		m_pImageLicenseStringLbl->hide();
		m_pImageLicenseStringTxt->hide();
	}

	m_pLicenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	m_pLicenseStringLbl->setText( pCommonStrings->getLicenseStringLbl() );
	m_pLicenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip()
	);
	m_pImageLicenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip(
	) );
	m_pImageLicenseStringLbl->setText( pCommonStrings->getLicenseStringLbl() );
	m_pImageLicenseStringTxt->setToolTip(
		pCommonStrings->getLicenseStringToolTip()
	);

	connect(
		m_pLicenseComboBox, SIGNAL( currentIndexChanged( int ) ), this,
		SLOT( licenseComboBoxChanged( int ) )
	);
	connect(
		m_pImageLicenseComboBox, SIGNAL( currentIndexChanged( int ) ), this,
		SLOT( imageLicenseComboBoxChanged( int ) )
	);

	// In case the drumkit name is not locked/the dialog is used as
	// "Save As" nothing needs to be disabled.
	if ( !bWritable && ( m_action & Action::ModifyViaUndo ) ) {
		QString sToolTip = pCommonStrings->getArtifactIsReadOnly();

		// The drumkit is read-only. Thus we won't support altering
		// any of its properties.
		m_pAuthorTxt->setIsActive( false );
		m_pAuthorTxt->setToolTip( sToolTip );
		m_pVersionSpinBox->setIsActive( false );
		m_pVersionSpinBox->setToolTip( sToolTip );
		m_pInfoTxt->setEnabled( false );
		m_pInfoTxt->setReadOnly( true );
		m_pInfoTxt->setToolTip( sToolTip );
		m_pLicenseComboBox->setIsActive( false );
		m_pLicenseComboBox->setToolTip( sToolTip );
		m_pLicenseStringTxt->setIsActive( false );
		m_pLicenseStringTxt->setToolTip( sToolTip );
		m_pImageText->setIsActive( false );
		m_pImageText->setToolTip( sToolTip );
		m_pImageLicenseComboBox->setIsActive( false );
		m_pImageLicenseComboBox->setToolTip( sToolTip );
		m_pImageLicenseStringTxt->setIsActive( false );
		m_pImageLicenseStringTxt->setToolTip( sToolTip );
		m_pSaveBtn->setIsActive( false );
		m_pSaveBtn->setToolTip( sToolTip );
		m_pImageBrowsePushButton->setIsActive( false );
		m_pImageBrowsePushButton->setToolTip( sToolTip );

		// Rather dirty fix to align the design of the QTextEdit to
		// the coloring of our custom QLineEdits.
		m_pInfoTxt->setStyleSheet(
			QString( "\
QTextEdit { \
    color: %1; \
    background-color: %2; \
}" )
				.arg( pPref->getColorTheme()->m_windowTextColor.name() )
				.arg( pPref->getColorTheme()->m_windowColor.name() )
		);
	}

	m_pTabWidget->setTabText( 0, pCommonStrings->getTabGeneralDialog() );
	m_pTabWidget->setTabText( 2, pCommonStrings->getTabLicensesDialog() );
	m_pTabWidget->setCurrentIndex( 0 );

	pLicenseLbl->setText( pCommonStrings->getLicenseDialog() );

	auto styleButton = [&]( Button* pButton ) {
		pButton->setFixedFontSize( 12 );
		pButton->setSize( QSize( 70, 23 ) );
		pButton->setBorderRadius( 3 );
		pButton->setType( Button::Type::Push );
	};

	styleButton( m_pSaveBtn );
	m_pSaveBtn->setFixedWidth( 110 );
	if ( m_pDrumkit != nullptr && !( m_action & Action::SaveAs ) &&
		 ( m_action & Action::ModifyViaUndo ) &&
		 m_pDrumkit->getContext() == Filesystem::Context::Song ) {
		m_pSaveBtn->setText( pCommonStrings->getActionSaveSong() );
	}
	else {
		m_pSaveBtn->setText( pCommonStrings->getActionSaveDrumkit() );
	}
	styleButton( m_pCancelBtn );
	m_pCancelBtn->setText( pCommonStrings->getButtonCancel() );
	styleButton( m_pPathBrowseButton );
	m_pPathBrowseButton->setText( pCommonStrings->getButtonBrowse() );

	styleButton( m_pImageBrowsePushButton );

	m_pTypesTable->setColumnCount( 3 );
	m_pTypesTable->setHorizontalHeaderLabels(
		QStringList() << pCommonStrings->getInstrumentId()
					  << pCommonStrings->getInstrumentButton()
					  << pCommonStrings->getInstrumentType()
	);
	m_pTypesTable->setColumnWidth( 0, 55 );
	m_pTypesTable->setColumnWidth( 1, 220 );
	m_pTypesTable->verticalHeader()->hide();
	m_pTypesTable->horizontalHeader()->setStretchLastSection( true );

	m_pLicensesTable->setColumnCount( 4 );
	m_pLicensesTable->setHorizontalHeaderLabels(
		QStringList() << pCommonStrings->getInstrumentButton()
					  << pCommonStrings->getComponent()
					  << pCommonStrings->getSample()
					  << pCommonStrings->getLicense()
	);

	m_pLicensesTable->verticalHeader()->hide();
	m_pLicensesTable->horizontalHeader()->setStretchLastSection( true );

	m_pLicensesTable->setColumnWidth( 0, 160 );
	m_pLicensesTable->setColumnWidth( 1, 80 );
	m_pLicensesTable->setColumnWidth( 2, 210 );

	updateLicensesTable();
	updateTypesTable( bWritable );

	if ( id != Instrument::EmptyId &&
		 m_idToTypeMap.find( id ) != m_idToTypeMap.end() ) {
		// Widget opened by double clicking a type of an instrument. Select the
		// corresponding type.
		auto pTypeWidget = m_idToTypeMap[id];
		if ( pTypeWidget != nullptr ) {
			m_pTabWidget->setCurrentIndex( 1 );
			pTypeWidget->setFocus( Qt::PopupFocusReason );
		}
	}

	// Explicit button connections (previously auto-connected by setupUi)
	connect( m_pSaveBtn, SIGNAL( clicked() ), this,
			 SLOT( on_saveBtn_clicked() ) );
	connect( m_pImageBrowsePushButton, SIGNAL( clicked() ), this,
			 SLOT( on_imageBrowsePushButton_clicked() ) );
	connect( m_pCancelBtn, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( m_pPathBrowseButton, &QPushButton::clicked, [&]() {
		const QString sPath = m_pPathEdit->text();
		QFileInfo info( sPath );
		QString sDir = info.absoluteDir().absolutePath();
		if ( sDir.isEmpty() || !Filesystem::dirWritable( sDir, false ) ) {
			sDir = Filesystem::userDataPath();
		}

		FileDialog fd( this );

		fd.setFileMode( QFileDialog::AnyFile );
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

		m_pPathEdit->setText( sFilePath );
	} );
}

DrumkitPropertiesDialog::~DrumkitPropertiesDialog()
{
}

/// On showing the dialog (after layout sizes have been applied), load the
/// drumkit image if any.
void DrumkitPropertiesDialog::showEvent( QShowEvent* e )
{
	if ( m_pDrumkit != nullptr &&
		 !m_pDrumkit->getAbsoluteImagePath().isEmpty() ) {
		updateImage( m_pDrumkit->getAbsoluteImagePath() );
	}
	else {
		m_pDrumkitImageLabel->hide();
	}
}

void DrumkitPropertiesDialog::updateLicensesTable()
{
	const auto pColorTheme =
		H2Core::Preferences::get_instance()->getColorTheme();
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();

	if ( m_pDrumkit == nullptr ) {
		return;
	}

	auto contentVector = m_pDrumkit->summarizeContent();

	if ( contentVector.size() > 0 ) {
		m_pLicensesTable->show();
		m_pLicensesTable->setRowCount( contentVector.size() );

		int nFirstMismatchRow = -1;

		for ( int ii = 0; ii < contentVector.size(); ++ii ) {
			const auto ccontent = contentVector[ii];

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
			if ( ccontent->m_license != m_pDrumkit->getLicense() ) {
				QString sHighlight =
					QString( "color: %1; background-color: %2" )
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

			m_pLicensesTable->setCellWidget( ii, 0, pInstrumentItem );
			m_pLicensesTable->setCellWidget( ii, 1, pComponentItem );
			m_pLicensesTable->setCellWidget( ii, 2, pSampleItem );
			m_pLicensesTable->setCellWidget( ii, 3, pLicenseItem );
		}

		// In case of a mismatch scroll into view
		if ( nFirstMismatchRow != -1 ) {
			m_pLicensesTable->showRow( nFirstMismatchRow );
		}
	}
	else {
		m_pLicensesTable->hide();
	}
}

void DrumkitPropertiesDialog::updateTypesTable( bool bWritable )
{
	const auto pPref = Preferences::get_instance();
	const auto pDatabase = Hydrogen::get_instance()->getSoundLibraryDatabase();
	m_idToTypeMap.clear();

	if ( m_pDrumkit == nullptr || m_pDrumkit->getInstruments() == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	const auto pInstrumentList = m_pDrumkit->getInstruments();

	m_pTypesTable->clearContents();
	m_pTypesTable->setRowCount( pInstrumentList->size() );

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

	auto insertRow = [=]( Instrument::Id id, const QString& sTextName,
						  const QString& sTextType, int nCell ) {
		LCDDisplay* pInstrumentId = new LCDDisplay( nullptr );
		pInstrumentId->setText( QString::number( static_cast<int>( id ) ) );
		pInstrumentId->setIsActive( false );
		pInstrumentId->setSizePolicy(
			QSizePolicy::Fixed, QSizePolicy::Expanding
		);

		LCDDisplay* pInstrumentName = new LCDDisplay( nullptr );
		pInstrumentName->setText( sTextName );
		pInstrumentName->setIsActive( false );
		pInstrumentName->setSizePolicy(
			QSizePolicy::Expanding, QSizePolicy::Expanding
		);
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

		if ( nIndex == -1 && !sTextType.isEmpty() ) {
			ERRORLOG(
				QString( "Provided type [%1] could not be found in database" )
					.arg( sTextType )
			);
		}
		else if ( nIndex != -1 ) {
			pInstrumentType->setCurrentIndex( nIndex );
		}
		else {
			pInstrumentType->setCurrentText( sTextType );
		}

		if ( bWritable ) {
			pInstrumentType->setIsActive( true );
			pInstrumentType->setEditable( true );
			pInstrumentType->setFocusPolicy( Qt::StrongFocus );
		}
		else {
			pInstrumentType->setIsActive( false );
		}

		m_pTypesTable->setCellWidget( nCell, 0, pInstrumentId );
		m_pTypesTable->setCellWidget( nCell, 1, pInstrumentName );
		m_pTypesTable->setCellWidget( nCell, 2, pInstrumentType );

		m_idToTypeMap[id] = pInstrumentType;
	};

	int nnCell = 0;
	for ( const auto& ppInstrument : *pInstrumentList ) {
		insertRow(
			ppInstrument->getId(), ppInstrument->getName(),
			ppInstrument->getType(), nnCell
		);
		nnCell++;
	}

	highlightDuplicates();
}

void DrumkitPropertiesDialog::licenseComboBoxChanged( int )
{
	m_pLicenseStringTxt->setText( License::LicenseTypeToQString(
		static_cast<License::LicenseType>( m_pLicenseComboBox->currentIndex() )
	) );

	if ( m_pLicenseComboBox->currentIndex() ==
		 static_cast<int>( License::Unspecified ) ) {
		m_pLicenseStringLbl->hide();
		m_pLicenseStringTxt->hide();
	}
	else {
		m_pLicenseStringLbl->show();
		m_pLicenseStringTxt->show();
	}

	updateLicensesTable();
}

void DrumkitPropertiesDialog::imageLicenseComboBoxChanged( int )
{
	m_pImageLicenseStringTxt->setText( License::LicenseTypeToQString(
		static_cast<License::LicenseType>( m_pImageLicenseComboBox->currentIndex()
		)
	) );

	if ( m_pImageLicenseComboBox->currentIndex() ==
		 static_cast<int>( License::Unspecified ) ) {
		m_pImageLicenseStringLbl->hide();
		m_pImageLicenseStringTxt->hide();
	}
	else {
		m_pImageLicenseStringLbl->show();
		m_pImageLicenseStringTxt->show();
	}
}

void DrumkitPropertiesDialog::updateImage( const QString& sFilePath )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pColorTheme = Preferences::get_instance()->getColorTheme();

	//  Styling used in case we assign text not images.
	m_pDrumkitImageLabel->setStyleSheet(
		QString( "QLabel { color: %1; background-color: %2;}" )
			.arg( pColorTheme->m_windowTextColor.name() )
			.arg( pColorTheme->m_windowColor.name() )
	);
	m_pDrumkitImageLabel->show();

	if ( !Filesystem::fileExists( sFilePath, false ) ) {
		m_pDrumkitImageLabel->setText( "File could not be found." );
		return;
	}

	QPixmap* pPixmap = new QPixmap( sFilePath );

	// Check whether the loading worked.
	if ( pPixmap->isNull() ) {
		ERRORLOG( QString( "Unable to load pixmap from [%1]" ).arg( sFilePath )
		);
		m_pDrumkitImageLabel->setText( tr( "Unable to load pixmap" ) );
		return;
	}

	// scale the image down to fit if required
	int x = (int) m_pDrumkitImageLabel->size().width();
	int y = m_pDrumkitImageLabel->size().height();
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
	m_pDrumkitImageLabel->setPixmap( *pPixmap );
	m_pDrumkitImageLabel->show();
}

void DrumkitPropertiesDialog::on_imageBrowsePushButton_clicked()
{
	if ( m_pDrumkit == nullptr ) {
		return;
	}

	// Try to get the drumkit directory and open file browser
	QString sDrumkitDir = m_pDrumkit->getPath();

	QString sFilePath = QFileDialog::getOpenFileName(
		this, tr( "Open Image" ), sDrumkitDir,
		tr( "Image Files (*.png *.jpg *.jpeg)" )
	);

	// If cancel was clicked just abort
	if ( sFilePath == nullptr || sFilePath.isEmpty() ) {
		return;
	}

	m_pImageText->setText( sFilePath );
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
	License licenseCheck( m_pLicenseStringTxt->text() );
	if ( static_cast<int>( licenseCheck.getType() ) !=
		 m_pLicenseComboBox->currentIndex() ) {
		if ( QMessageBox::warning(
				 this, "Hydrogen",
				 pCommonStrings->getLicenseMismatchingUserInput(),
				 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
			 ) == QMessageBox::Cancel ) {
			WARNINGLOG( QString( "Abort. License String [%1] "
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
	License imageLicenseCheck( m_pImageLicenseStringTxt->text() );
	if ( static_cast<int>( imageLicenseCheck.getType() ) !=
		 m_pImageLicenseComboBox->currentIndex() ) {
		if ( QMessageBox::warning(
				 this, "Hydrogen",
				 tr( "Specified image License String does not comply with the "
					 "license selected in the combo box." ),
				 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
			 ) == QMessageBox::Cancel ) {
			WARNINGLOG(
				QString( "Abort. Image License String [%1] does "
						 "not comply to selected License Type [%2]" )
					.arg( m_pImageLicenseStringTxt->text() )
					.arg( License::LicenseTypeToQString(
						static_cast<License::LicenseType>(
							m_pImageLicenseComboBox->currentIndex()
						)
					) )
			);
			return;
		}
	}

	if ( m_pNameTxt->text().isEmpty() ) {
		QMessageBox::warning(
			this, "Hydrogen", pCommonStrings->getErrorEmptyName() );
		return;
	}

	if ( !HydrogenApp::checkDrumkitLicense( m_pDrumkit ) ) {
		ERRORLOG( "User cancelled dialog due to licensing issues." );
		return;
	}

	// Types have to be unique.
	std::set<QString> types;
	for ( int ii = 0; ii < m_pTypesTable->rowCount(); ++ii ) {
		auto ppItemType =
			dynamic_cast<LCDCombo*>( m_pTypesTable->cellWidget( ii, 2 ) );
		if ( ppItemType != nullptr && ! ppItemType->currentText().isEmpty() ) {
			const auto [_, bSuccess] =
				types.insert( ppItemType->currentText() );
			if ( !bSuccess ) {
				highlightDuplicates();
				QMessageBox::warning(
					this, "Hydrogen", pCommonStrings->getErrorUniqueTypes()
				);
				return;
			}
		}
	}

	QString sNewLicenseString( m_pLicenseStringTxt->text() );
	if ( m_pLicenseComboBox->currentIndex() ==
		 static_cast<int>( License::Unspecified ) ) {
		sNewLicenseString = "";
	}
	License newLicense( sNewLicenseString );
	newLicense.setCopyrightHolder( m_pDrumkit->getAuthor() );

	QString sNewImageLicenseString( m_pImageLicenseStringTxt->text() );
	if ( m_pImageLicenseComboBox->currentIndex() ==
		 static_cast<int>( License::Unspecified ) ) {
		sNewImageLicenseString = "";
	}
	License newImageLicense( sNewImageLicenseString );
	newImageLicense.setCopyrightHolder( m_pDrumkit->getAuthor() );

	const QString sOldPath = m_pDrumkit->getPath();
	if ( m_pDrumkit->getName() != m_pNameTxt->text() ) {
		m_pDrumkit->setName( m_pNameTxt->text() );
	}
	if ( m_pDrumkit->getVersion() != m_pVersionSpinBox->value() ) {
		m_pDrumkit->setVersion( m_pVersionSpinBox->value() );
	}
	m_pDrumkit->setAuthor( m_pAuthorTxt->text() );
	m_pDrumkit->setInfo( m_pInfoTxt->toHtml() );

	// Only update the license in case it changed (in order to not
	// overwrite an attribution).
	if ( m_pDrumkit->getLicense() != newLicense ) {
		m_pDrumkit->setLicense( newLicense );
	}

	// Will contain image which should be removed. To keep the previous image,
	// this string should be empty.
	QString sOldImagePath;
	// If set, indicates that the image has changed and the new one requires
	// copying.
	QString sNewImagePath;
	if ( m_pImageText->text() != m_pDrumkit->getImage() ) {
		// Only ask for deleting the previous file if it exists.
		if ( !m_pDrumkit->getImage().isEmpty() &&
			 Filesystem::fileExists(
				 m_pDrumkit->getAbsoluteImagePath(), true
			 ) ) {
			int nRes = QMessageBox::information(
				this, "Hydrogen",
				tr( "Delete previous drumkit image" )
					.append( QString( " [%1]" ).arg(
						m_pDrumkit->getAbsoluteImagePath()
					) ),
				QMessageBox::Yes | QMessageBox::No
			);
			if ( nRes == QMessageBox::Yes ) {
				sOldImagePath = m_pDrumkit->getAbsoluteImagePath();
			}
		}

		m_pDrumkit->setImage( m_pImageText->text() );
		sNewImagePath = m_pImageText->text();
	}

	if ( m_pDrumkit->getImageLicense() != newImageLicense ) {
		m_pDrumkit->setImageLicense( newImageLicense );
	}

	m_pDrumkit->setTags( m_pTagEdit->getTags() );

	for ( int ii = 0; ii < m_pTypesTable->rowCount(); ++ii ) {
		auto ppItemId =
			dynamic_cast<LCDDisplay*>( m_pTypesTable->cellWidget( ii, 0 ) );
		auto ppItemName =
			dynamic_cast<LCDDisplay*>( m_pTypesTable->cellWidget( ii, 1 ) );
		auto ppItemType =
			dynamic_cast<LCDCombo*>( m_pTypesTable->cellWidget( ii, 2 ) );

		if ( ppItemId != nullptr && ppItemType != nullptr ) {
			const auto ppInstrument = m_pDrumkit->getInstruments()->find(
				static_cast<Instrument::Id>( ppItemId->text().toInt() )
			);

			if ( ppInstrument != nullptr ) {
				ppInstrument->setType( ppItemType->currentText() );
			}
			else {
				if ( ppItemName != nullptr ) {
					ERRORLOG( QString( "Unable to find instrument [%1] (name: "
									   "[%2], type: [%3])" )
								  .arg( ppItemId->text() )
								  .arg( ppItemName->text() )
								  .arg( ppItemType->currentText() ) );
				}
				else {
					ERRORLOG(
						QString( "Unable to find instrument [%1] (type: [%2])" )
							.arg( ppItemId->text() )
							.arg( ppItemType->currentText() )
					);
				}
			}
		}
		else {
			WARNINGLOG( QString( "Invalid row [%1]" ).arg( ii ) );
		}
	}

	bool bOldImageDeleted = false;
	if ( m_pDrumkit->getContext() == Filesystem::Context::Song &&
		 ( m_action & Action::ModifyViaUndo )) {
		// Copy the selected image into our cache folder as the kit is a
		// floating one associated to a song.
		if ( !sNewImagePath.isEmpty() ) {
			QFileInfo fileInfo( sNewImagePath );

			const QString sTargetPath = Filesystem::addUniquePrefix(
				QDir( Filesystem::cacheDir() )
					.absoluteFilePath( fileInfo.fileName() )
			);

			// Logging is done in file_copy.
			if ( Filesystem::fileCopy(
					 sNewImagePath, sTargetPath, true, false
				 ) ) {
				m_pDrumkit->setImage( sTargetPath );
			}
		}

		if ( !sOldImagePath.isEmpty() ) {
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
			Instrument::Type type;
			int nPatternNumber;
		};
		std::vector<noteToBeMapped> notesToBeMapped;
		struct instrumentToBeMapped {
			std::shared_ptr<Instrument> pInstrument;
			Instrument::Type sOldType;
		};
		// This should always be true. Let's keep it safe.
		if ( pOldKit != nullptr && pOldKit->getInstruments()->size() ==
									   m_pDrumkit->getInstruments()->size() ) {
			for ( int nnIdx = 0; nnIdx < pOldKit->getInstruments()->size();
				  ++nnIdx ) {
				auto pOldInstrument = pOldKit->getInstruments()->get( nnIdx );
				auto pNewInstrument =
					m_pDrumkit->getInstruments()->get( nnIdx );
				if ( pOldInstrument->getType().isEmpty() &&
					 !pNewInstrument->getType().isEmpty() &&
					 pOldInstrument->getId() == pNewInstrument->getId() ) {
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
								notesToBeMapped.push_back(
									{ ppNote, pNewInstrument->getType(),
									  pSong->getPatternList()->index( ppPattern
									  ) }
								);
							}
						}
					}
				}
			}
		}

		if ( notesToBeMapped.size() > 0 ) {
			pHydrogenApp->beginUndoMacro(
				pCommonStrings->getActionEditDrumkitProperties()
			);
		}

		for ( const auto& [ppNote, ssType, nnPatternNumber] :
			  notesToBeMapped ) {
			pHydrogenApp->pushUndoCommand( new SE_editNotePropertiesAction(
				PatternEditor::Property::Type, nnPatternNumber,
				ppNote->getPosition(), ppNote->getInstrumentId(),
				ppNote->getInstrumentId(), ssType, "", ppNote->getVelocity(),
				ppNote->getVelocity(), ppNote->getPan(), ppNote->getPan(),
				ppNote->getLeadLag(), ppNote->getLeadLag(),
				ppNote->getProbability(), ppNote->getProbability(),
				ppNote->getLength(), ppNote->getLength(), ppNote->getKey(),
				ppNote->getKey(), ppNote->getOctave(), ppNote->getOctave()
			) );
		}

		// When editing the properties of the current kit, the new version will
		// be loaded in a way that can be undone.
		//
		// This affects mostly metadata and can be done more efficiently.
		// But due to the license propagation into the instruments, we switch
		// the entire kit.
		pHydrogenApp->pushUndoCommand( new SE_switchDrumkitAction(
			m_pDrumkit, pSong->getDrumkit(),
			SE_switchDrumkitAction::Type::EditProperties
		) );

		if ( notesToBeMapped.size() > 0 ) {
			pHydrogenApp->endUndoMacro( "" );
		}

		// Since we hit save on the song's drumkit, we should also save the song
		// for the sake of consistency.
		pHydrogenApp->getMainForm()->action_file_save( false );

		if ( ! ( m_action & Action::SaveAs ) ) {
			// We are not saving the kit itself and are done.
			pHydrogenApp->showStatusBarMessage(
				pCommonStrings->getActionEditCurrentDrumkitProperties()
			);
			accept();
			return;
		}
	}

	if ( ( m_action & Action::SaveAs ) &&
		 m_pPathEdit->text() != m_pDrumkit->getPath() ) {
		m_pDrumkit->setPath( Filesystem::drumkitPathFromDir( m_pPathEdit->text()
		) );
	}

	// Store the drumkit in the NSM session folder
#ifdef H2CORE_HAVE_OSC
	if ( ( m_action & Action::NsmSession ) &&
		 m_pDrumkit->getContext() == Filesystem::Context::Song ) {
		m_pDrumkit->setPath(
			QDir(
				NsmClient::get_instance()->getSessionFolderPath() +
				QDir::separator() + m_pDrumkit->getName()
			)
				.absoluteFilePath( Filesystem::drumkitXml() )
		);
	}
#endif

	// Check whether there is already a kit present we would overwrite.
	if ( ( m_action & Action::SaveAs ) &&
		 Filesystem::fileExists( m_pDrumkit->getPath(), true ) ) {
		int nRes = QMessageBox::information(
			this, "Hydrogen",
			QString( "%1\n%2\n\n%3" )
				/*: asked when saving a drumkit to a certain location */
				.arg( tr( "Overwrite existing drumkit stored in" ) )
				.arg( m_pDrumkit->getPath() )
				.arg( pCommonStrings->getActionIrreversible() ),
			QMessageBox::Yes | QMessageBox::No
		);
		if ( nRes != QMessageBox::Yes ) {
			INFOLOG( "Aborted by user to not overwrite drumkit" );
			return;
		}
	}

	QApplication::setOverrideCursor( Qt::WaitCursor );

	// Write new properties/drumkit to disk.
	if ( !m_pDrumkit->save() ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::information(
			this, "Hydrogen", pCommonStrings->getErrorDrumkitSaved()
		);
		ERRORLOG( pCommonStrings->getErrorDrumkitSaved() );
		return;
	}

	if ( m_action & Action::SaveAs ) {
		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2] -> [%3]" )
				.arg(
					m_pDrumkit->getContext() == Filesystem::Context::Song
						? pCommonStrings->getActionSaveCurrentDrumkit()
						: pCommonStrings->getActionSaveDrumkit()
				)
				.arg( m_pDrumkit->getName() )
				.arg( m_pDrumkit->getPath() )
		);
	}
	else {
		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2]" )
				.arg( pCommonStrings->getActionEditDrumkitProperties() )
				.arg( m_pDrumkit->getName() )
		);
	}

	// Copy the selected image into the drumkit folder (in case a file outside
	// of it was selected.)
	if ( !sNewImagePath.isEmpty() ) {
		QFileInfo fileInfo( sNewImagePath );

		if ( fileInfo.dir().absolutePath() != m_pDrumkit->getPath() ) {
			const QString sTargetPath =
				QDir( Filesystem::drumkitDirFromPath( m_pDrumkit->getPath() ) )
					.absoluteFilePath( fileInfo.fileName() );

			// Logging is done in file_copy.
			Filesystem::fileCopy( sNewImagePath, sTargetPath, true, false );
		}
	}

	if ( !sOldImagePath.isEmpty() && !bOldImageDeleted ) {
		Filesystem::rm( sOldImagePath, false, false );
	}

	pHydrogen->getSoundLibraryDatabase()->updateDrumkits(
		Event::Trigger::Default
	);

	QApplication::restoreOverrideCursor();

	accept();
}

void DrumkitPropertiesDialog::highlightDuplicates()
{
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();
	QStringList duplicates;

	const QString sHighlight =
		QString( "color: %1; background-color: %2" )
			.arg( pColorTheme->m_buttonRedTextColor.name() )
			.arg( pColorTheme->m_buttonRedColor.name() );

	// Compile a list of all duplicated types.
	std::set<QString> types;
	for ( int ii = 0; ii < m_pTypesTable->rowCount(); ++ii ) {
		auto ppType =
			dynamic_cast<LCDCombo*>( m_pTypesTable->cellWidget( ii, 2 ) );
		if ( ppType != nullptr ) {
			const auto [_, bSuccess] = types.insert( ppType->currentText() );
			if ( !bSuccess ) {
				duplicates << ppType->currentText();
			}
		}
	}

	// Highlight the corresponding combo boxes
	for ( int ii = 0; ii < m_pTypesTable->rowCount(); ++ii ) {
		auto ppType =
			dynamic_cast<LCDCombo*>( m_pTypesTable->cellWidget( ii, 2 ) );
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

}  // namespace H2Core
