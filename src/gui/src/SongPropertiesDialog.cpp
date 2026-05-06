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
#include "Widgets/Button.h"
#include "Widgets/FileDialog.h"
#include "Widgets/LCDCombo.h"
#include "Widgets/LCDDisplay.h"
#include "Widgets/LCDSpinBox.h"
#include "Widgets/LCDTextEdit.h"
#include "Widgets/TagEdit.h"
#include "UndoActions.h"

#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/License.h>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QSpacerItem>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace H2Core;

SongPropertiesDialog::SongPropertiesDialog(
	QWidget* parent,
	std::shared_ptr<Song> pSong,
	Action action
)
	: QDialog( parent ), m_pSong( pSong ), m_action( action )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();

	setMinimumSize( 757, 876 );

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	if ( pHydrogen->isUnderSessionManagement() && ( action & Action::ModifyViaUndo ) && ( action & Action::SaveAs ) ) {
		setWindowTitle( tr( "Export song from Session" ) );
	}
	else if ( action & Action::Duplicate ) {
		setWindowTitle( pCommonStrings->getMenuActionDuplicate() );
	}
	else if ( action & Action::SaveAs ) {
		setWindowTitle( pCommonStrings->getActionSaveSong() );
	}
	else {
		setWindowTitle( tr( "Song properties" ) );
	}

	// Overall layout
	auto pOverallLayout = new QVBoxLayout( this );
	pOverallLayout->setSpacing( 0 );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	setLayout( pOverallLayout );

	auto pScrollArea = new QScrollArea( this );
	pScrollArea->setWidgetResizable( true );
	pOverallLayout->addWidget( pScrollArea );

	auto pScrollAreaContent = new QWidget( pScrollArea );
	pScrollAreaContent->setMinimumSize( 752, 869 );
	pScrollArea->setWidget( pScrollAreaContent );

	auto pOuterLayout = new QVBoxLayout( pScrollAreaContent );
	pOuterLayout->setSpacing( 0 );
	pOuterLayout->setContentsMargins( 0, 0, 0, 0 );
	pScrollAreaContent->setLayout( pOuterLayout );

	// Tab widget
	m_pTabWidget = new QTabWidget( pScrollAreaContent );
	m_pTabWidget->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Expanding
	);
	pOuterLayout->addWidget( m_pTabWidget );

	// ---- General tab ----
	auto pTabGeneral = new QWidget( m_pTabWidget );
	pTabGeneral->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Expanding
	);
	m_pTabWidget->addTab( pTabGeneral, QString() );

	auto pGeneralLayout = new QGridLayout( pTabGeneral );
	pTabGeneral->setLayout( pGeneralLayout );

	// Row 0: Path
	auto pPathLabel = new QLabel( pTabGeneral );
	pGeneralLayout->addWidget( pPathLabel, 0, 0 );

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

	pGeneralLayout->addWidget( pPathContainer, 0, 1 );

	auto pNameLabel = new QLabel( pTabGeneral );
	pNameLabel->setMinimumHeight( 20 );
	pGeneralLayout->addWidget( pNameLabel, 1, 0 );

	m_pSongNameTxt = new LCDDisplay( pTabGeneral );
	pGeneralLayout->addWidget( m_pSongNameTxt, 1, 1 );

	auto pVersionLabel = new QLabel( pTabGeneral );
	pGeneralLayout->addWidget( pVersionLabel, 2, 0 );

	m_pVersionSpinBox = new LCDSpinBox( pTabGeneral );
	pGeneralLayout->addWidget( m_pVersionSpinBox, 2, 1 );

	auto pAuthorLabel = new QLabel( pTabGeneral );
	pAuthorLabel->setMinimumHeight( 20 );
	pGeneralLayout->addWidget( pAuthorLabel, 3, 0 );

	m_pAuthorTxt = new LCDDisplay( pTabGeneral );
	pGeneralLayout->addWidget( m_pAuthorTxt, 3, 1 );

	auto pLicenseLabel = new QLabel( pTabGeneral );
	pLicenseLabel->setMinimumHeight( 20 );
	pGeneralLayout->addWidget( pLicenseLabel, 4, 0 );

	m_pLicenseComboBox = new LCDCombo( pTabGeneral );
	m_pLicenseComboBox->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed
	);
	pGeneralLayout->addWidget( m_pLicenseComboBox, 4, 1 );

	m_pLicenseStringTxt = new LCDDisplay( pTabGeneral );
	pGeneralLayout->addWidget( m_pLicenseStringTxt, 5, 1 );

	auto pNotesLabel = new QLabel( pTabGeneral );
	pGeneralLayout->addWidget( pNotesLabel, 6, 0 );

	m_pNotesTxt = new LCDTextEdit( pTabGeneral );
	pGeneralLayout->addWidget( m_pNotesTxt, 6, 1 );

	auto pTagsLabel = new QLabel( pTabGeneral );
	pGeneralLayout->addWidget( pTagsLabel, 7, 0 );

	m_pTagEdit = new TagEdit( pTabGeneral );
	pGeneralLayout->addWidget( m_pTagEdit, 7, 1 );

	// ---- Licenses tab ----
	auto pTabLicenses = new QWidget( m_pTabWidget );
	m_pTabWidget->addTab( pTabLicenses, QString() );

	auto pLicensesLayout = new QHBoxLayout( pTabLicenses );
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
	m_pVersionSpinBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
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
	pVersionLabel->setText( pCommonStrings->getVersionDialog() );

	setupLicenseComboBox( m_pLicenseComboBox );

	if ( pSong != nullptr ) {
		if ( pSong->getPath() != Filesystem::emptyPath( Filesystem::Artifact::Song ) ) {
			// In order to allow to recover empty songs not associated with a
			// file (path) yet from an autosave file, we assign those "empty"
			// paths. But we do not show them in here since those paths are not
			// actually backed by a file but only might have an associated
			// autosave file.
			m_pPathEdit->setText( pSong->getPath() );
		}
		m_pVersionSpinBox->setValue( pSong->getVersion() );
		m_pSongNameTxt->setText( pSong->getName() );

		m_pAuthorTxt->setText( pSong->getAuthor() );
		m_pNotesTxt->append( pSong->getNotes() );

		m_pLicenseComboBox->setCurrentIndex(
			static_cast<int>( pSong->getLicense().getType() ) );
		m_pLicenseStringTxt->setText( pSong->getLicense().getLicenseString() );
		if ( pSong->getLicense().getType() == License::Unspecified ) {
			m_pLicenseStringTxt->hide();
		}
		m_pTagEdit->setTags( pSong->getTags() );
	}

	m_pPathEdit->setIsActive( action & Action::SaveAs );
	m_pPathBrowseButton->setVisible( action & Action::SaveAs );

	connect( m_pLicenseComboBox, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( licenseComboBoxChanged( int ) ) );

	m_pTabWidget->setTabText( 0, pCommonStrings->getTabGeneralDialog() );
	m_pTabWidget->setTabText( 1, pCommonStrings->getTabLicensesDialog() );
	m_pTabWidget->setCurrentIndex( 0 );

	pNameLabel->setText( pCommonStrings->getNameDialog() );
	pAuthorLabel->setText( pCommonStrings->getAuthorDialog() );
	pLicenseLabel->setText( pCommonStrings->getLicenseDialog() );
	m_pLicenseComboBox->setToolTip( pCommonStrings->getLicenseComboToolTip() );
	m_pLicenseStringTxt->setToolTip( pCommonStrings->getLicenseStringToolTip() );
	pNotesLabel->setText( pCommonStrings->getNotesDialog() );

	pTagsLabel->setText( pCommonStrings->getTagsLabel() );

	auto styleButton = [&]( Button* pButton ) {
		pButton->setFixedFontSize( 12 );
		pButton->setSize( QSize( 70, 23 ) );
		pButton->setBorderRadius( 3 );
		pButton->setType( Button::Type::Push );
		pButton->setIsActive( true );
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
		fd.setNameFilter( Filesystem::sSongFilter );
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

		if ( sFilePath.endsWith( Filesystem::sSongSuffix ) == false ) {
			sFilePath += Filesystem::sSongSuffix;
		}

		m_pPathEdit->setText( sFilePath );
	} );

	updatePatternLicenseTable();
}

SongPropertiesDialog::~SongPropertiesDialog() {
}

void SongPropertiesDialog::updatePatternLicenseTable() {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pColorTheme = H2Core::Preferences::get_instance()->getColorTheme();

	m_pLicensesTable->setColumnCount( 4 );
	m_pLicensesTable->setHorizontalHeaderLabels(
		QStringList() <<
		pCommonStrings->getNameDialog() <<
		pCommonStrings->getVersionDialog() <<
		pCommonStrings->getAuthorDialog() <<
		pCommonStrings->getLicenseDialog() );
	m_pLicensesTable->verticalHeader()->hide();
	m_pLicensesTable->horizontalHeader()->setStretchLastSection( true );
	m_pLicensesTable->setColumnWidth( 0, 210 );
	m_pLicensesTable->setColumnWidth( 1, 60 );
	m_pLicensesTable->setColumnWidth( 2, 140 );

	if ( m_pSong == nullptr ){
		return;
	}

	const auto pPatternList = m_pSong->getPatternList();
	m_pLicensesTable->setRowCount( pPatternList->size() );

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
				 ppPattern->getLicense() != m_pSong->getLicense() ) {
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

			m_pLicensesTable->setCellWidget( rrow, 0, pNameItem );
			m_pLicensesTable->setCellWidget( rrow, 1, pVersionItem );
			m_pLicensesTable->setCellWidget( rrow, 2, pAuthorItem );
			m_pLicensesTable->setCellWidget( rrow, 3, pLicenseItem );

			++rrow;
		}
	}

	// In case of a mismatch scroll into view
	if ( nFirstMismatchRow != -1 ) {
		m_pLicensesTable->showRow( nFirstMismatchRow );
	}
}

void SongPropertiesDialog::licenseComboBoxChanged( int ) {

	m_pLicenseStringTxt->setText( License::LicenseTypeToQString(
		static_cast<License::LicenseType>( m_pLicenseComboBox->currentIndex() ) ) );

	if ( m_pLicenseComboBox->currentIndex() == static_cast<int>( License::Unspecified ) ) {
		m_pLicenseStringTxt->hide();
	}
	else {
		m_pLicenseStringTxt->show();
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
	const int nVersion = m_pVersionSpinBox->value();
	const QString sAuthor = m_pAuthorTxt->text();
	const QString sSongName = m_pSongNameTxt->text();
	QString sNewLicenseString( m_pLicenseStringTxt->text() );
	if ( m_pLicenseComboBox->currentIndex() ==
		 static_cast<int>(License::Unspecified) ) {
		sNewLicenseString = "";
	}
	const License license( sNewLicenseString );
	const QStringList tags = m_pTagEdit->getTags();
	const QString sNotes = m_pNotesTxt->toPlainText();

	// Sanity checks.
	//
	// Check whether the license strings from the line edits comply to
	// the license types selected in the combo boxes.
	License licenseCheck( m_pLicenseStringTxt->text() );
	if ( static_cast<int>(licenseCheck.getType()) != m_pLicenseComboBox->currentIndex() ) {
		if ( QMessageBox::warning(
				 this, "Hydrogen", pCommonStrings->getLicenseMismatchingUserInput(),
				 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel )
			 == QMessageBox::Cancel ) {
			WARNINGLOG( QString( "Abort, since drumkit License String [%1] does not comply to selected License Type [%2]" )
						.arg( m_pLicenseStringTxt->text() )
						.arg( License::LicenseTypeToQString(
						    static_cast<License::LicenseType>(m_pLicenseComboBox->currentIndex()) ) ) );
			return;
		}
	}

	bool bIsModified = false;

	if ( ( m_action & Action::SaveAs ) &&
		 ( m_pSong->getPath() != m_pPathEdit->text() &&
		   !m_pPathEdit->text().isEmpty() ) ) {
		if ( !Filesystem::isPathValid(
				 Filesystem::Artifact::Song, m_pPathEdit->text(), false
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
		 ( m_pSong->getPath() != m_pPathEdit->text() ||
		   m_pSong->getVersion() != nVersion ||
		   m_pSong->getName() != sSongName ||
		   m_pSong->getAuthor() != sAuthor ||
		   m_pSong->getNotes() != sNotes ||
		   m_pSong->getLicense() != license ||
		   m_pSong->getTags() != tags ) ) {
		auto pAction = new SE_modifySongPropertiesAction(
			m_pSong->getPath(), m_pSong->getVersion(), m_pSong->getName(),
			m_pSong->getAuthor(), m_pSong->getNotes(), m_pSong->getLicense(),
			m_pSong->getTags(), m_pPathEdit->text(), nVersion, sSongName,
			sAuthor, sNotes, license, tags
		);
		HydrogenApp::get_instance()->pushUndoCommand( pAction );

		accept();

		return;
	}

	if ( ( m_action & Action::SaveAs ) &&
		 m_pSong->getPath() != m_pPathEdit->text() ) {
		m_pSong->setPath( m_pPathEdit->text() );
		bIsModified = true;
	}
	if ( m_pSong->getVersion() != nVersion ) {
		m_pSong->setVersion( nVersion );
		bIsModified = true;
	}

	if ( sSongName != m_pSong->getName() ) {
		m_pSong->setName( sSongName );
		bIsModified = true;
	}
	if ( m_pSong->getAuthor() != sAuthor ) {
		m_pSong->setAuthor( sAuthor );
		bIsModified = true;
	}
	if ( m_pSong->getNotes() != sNotes ) {
		m_pSong->setNotes( sNotes );
		bIsModified = true;
	}

	if ( tags != m_pSong->getTags() ) {
		m_pSong->setTags( tags );
		bIsModified = true;
	}

	if ( m_pSong->getLicense() != license ) {
		m_pSong->setLicense( license );
		bIsModified = true;
	}

	pHydrogen->setIsModified( bIsModified && !( m_action & Action::SaveAs ) );

	// We do not need to send an Event::SongModified in here. This is only
	// required for the currently active song, which must always be altered
	// using Action::ModifyViaUndo.

	accept();
}
