/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "OnlineImportSourcesDialog.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../Widgets/StatusLED.h"

#include <core/OnlineImporter.h>
#include <core/Preferences/Preferences.h>

#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

using namespace H2Core;

OnlineImportSourcesDialog::OnlineImportSourcesDialog( QWidget* pParent )
	: QDialog( pParent ),
	  m_pTable( nullptr ),
	  m_pCheckButton( nullptr ),
	  m_pOkButton( nullptr ),
	  m_pCancelButton( nullptr )
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	setWindowTitle( pCommonStrings->getManageSourcesTitle() );
	setMinimumWidth( OnlineImportSourcesDialog::nMinimumWidth );

	const auto repos = Preferences::get_instance()->getOnlineRepos();
	for ( const auto& sUrl : repos ) {
		m_sources << sUrl;
	}

	buildLayout();
	populateTable();
	updateStyleSheet();
}

OnlineImportSourcesDialog::~OnlineImportSourcesDialog()
{
}

void OnlineImportSourcesDialog::buildLayout()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pOverallLayout = new QVBoxLayout( this );
	pOverallLayout->setSpacing( 0 );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	setLayout( pOverallLayout );

	auto pScrollArea = new QScrollArea( this );
	pScrollArea->setWidgetResizable( true );
	pOverallLayout->addWidget( pScrollArea );

	auto pScrollAreaContent = new QWidget( pScrollArea );
	pScrollAreaContent->setMinimumSize(
		OnlineImportSourcesDialog::nMinimumWidth - 5, 50
	);
	pScrollArea->setWidget( pScrollAreaContent );
	auto* pMainLayout = new QVBoxLayout( pScrollAreaContent );
	pMainLayout->setContentsMargins( 10, 10, 10, 10 );

	// --- Table (no scroll area — dialog resizes to fit) ---
	m_pTable = new QTableWidget( pScrollAreaContent );
	pMainLayout->addWidget( m_pTable, 1 );

	m_pTable->setColumnCount( 3 );
	m_pTable->setRowCount( 1 );
	m_pTable->setSelectionMode( QAbstractItemView::NoSelection );
	m_pTable->horizontalHeader()->hide();
	m_pTable->verticalHeader()->hide();
	m_pTable->setColumnWidth( 0, OnlineImportSourcesDialog::nButtonWidth );
	m_pTable->setColumnWidth( 2, OnlineImportSourcesDialog::nButtonWidth );
	m_pTable->horizontalHeader()->setSectionResizeMode(
		1, QHeaderView::Stretch
	);
	m_pTable->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Fixed );
	m_pTable->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::Fixed );

	// --- Button row ---
	auto* pButtonLayout = new QHBoxLayout();

	m_pCheckButton =
		new QPushButton( pCommonStrings->getCheckSourcesButton(), pScrollAreaContent );
	pButtonLayout->addWidget( m_pCheckButton );

	pButtonLayout->addStretch();

	m_pCancelButton =
		new QPushButton( pCommonStrings->getButtonCancel(), pScrollAreaContent );
	pButtonLayout->addWidget( m_pCancelButton );

	m_pOkButton = new QPushButton( pCommonStrings->getButtonOk(), pScrollAreaContent );
	pButtonLayout->addWidget( m_pOkButton );

	connect( m_pOkButton, &QPushButton::clicked, this, &QDialog::accept );
	connect( m_pCancelButton, &QPushButton::clicked, this, &QDialog::reject );
	connect(
		m_pCheckButton, &QPushButton::clicked, this,
		&OnlineImportSourcesDialog::checkAllSourceStatus
	);

	pMainLayout->addLayout( pButtonLayout );
}

void OnlineImportSourcesDialog::populateTable()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	m_pTable->clearContents();
	m_pTable->setRowCount( 0 );

	for ( const auto& sUrl : m_sources ) {
		addRow( sUrl );
	}

	// Bottom row: "+" add button in the right column (same column as bin
	// buttons)
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	}
	else {
		sIconPath.append( "/icons/black/" );
	}

	const int nAddRow = m_pTable->rowCount();
	auto* pAddButton = new QToolButton( m_pTable );
	pAddButton->setFocusPolicy( Qt::NoFocus );
	pAddButton->setFixedSize(
		OnlineImportSourcesDialog::nButtonWidth,
		OnlineImportSourcesDialog::nButtonWidth
	);
	pAddButton->setIcon( QIcon( sIconPath + "new.svg" ) );
	pAddButton->setIconSize( QSize(
		OnlineImportSourcesDialog::nIconSize, OnlineImportSourcesDialog::nIconSize
	) );
	pAddButton->setToolTip( pCommonStrings->getMenuActionAdd() );
	connect( pAddButton, &QPushButton::clicked, this, [=]() {
		m_sources << "";
		populateTable();
	} );
	m_pTable->insertRow( nAddRow );
	m_pTable->setCellWidget( nAddRow, 2, pAddButton );

	// Resize dialog to fit content
	adjustSize();
}

void OnlineImportSourcesDialog::addRow( const QString& sUrl )
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const int nRow = m_pTable->rowCount();
	m_pTable->insertRow( nRow );

	// LED in left column. We will wrap it into another widget in order to
	// center it.
	auto pContainer = new QWidget( m_pTable );
	pContainer->setFixedSize( QSize(
		OnlineImportSourcesDialog::nButtonWidth,
		OnlineImportSourcesDialog::nButtonWidth
	) );
	auto pContainerLayout = new QHBoxLayout();
	pContainerLayout->setAlignment( Qt::AlignCenter );
	pContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pContainer->setLayout( pContainerLayout );
	auto* pLed = new StatusLED(
		pContainer, QSize(
						OnlineImportSourcesDialog::nLedSize,
						OnlineImportSourcesDialog::nLedSize
					)
	);
	pContainerLayout->addWidget( pLed );
	m_pTable->setCellWidget( nRow, 0, pContainer );

	// URL edit in middle column
	auto* pEdit = new QLineEdit( m_pTable );
	pEdit->setText( sUrl );
	pEdit->setPlaceholderText( "https://example.url/index.json" );
	connect( pEdit, &QLineEdit::editingFinished, this, [=]() {
		const QString sText = pEdit->text().trimmed();
		if ( !sText.isEmpty() ) {
			QUrl url( sText );
			if ( !url.isValid() ) {
				QMessageBox::warning(
					this, pCommonStrings->getInvalidUrl(),
					pCommonStrings->getInvalidUrlMessage()
				);
			}
		}

		if ( m_sources.size() > nRow && m_sources[ nRow ] != sText ) {
			m_sources[ nRow ] = sText;
			// Resize to fit new content
			adjustSize();
		}
	} );
	m_pTable->setCellWidget( nRow, 1, pEdit );

	// Remove button in right column
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	}
	else {
		sIconPath.append( "/icons/black/" );
	}

	auto* pRemoveButton = new QToolButton( m_pTable );
	pRemoveButton->setFocusPolicy( Qt::NoFocus );
	pRemoveButton->setFixedSize(
		OnlineImportSourcesDialog::nButtonWidth,
		OnlineImportSourcesDialog::nButtonWidth
	);
	pRemoveButton->setIcon( QIcon( sIconPath + "bin.svg" ) );
	pRemoveButton->setIconSize( QSize(
		OnlineImportSourcesDialog::nIconSize, OnlineImportSourcesDialog::nIconSize
	) );
	pRemoveButton->setToolTip( pCommonStrings->getMenuActionDelete() );
	connect( pRemoveButton, &QPushButton::clicked, this, [=]() {
		m_sources.remove( nRow, 1 );
		populateTable();
	} );
	m_pTable->setCellWidget( nRow, 2, pRemoveButton );

	// Resize dialog to fit new row
	adjustSize();
}

void OnlineImportSourcesDialog::checkAllSourceStatus()
{
	m_pCheckButton->setEnabled( false );

	const int nTotal = m_pTable->rowCount() - 1;

	auto getLed = [&]( int nRow ) -> StatusLED* {
		auto pContainer = m_pTable->cellWidget( nRow, 0 );
		if ( pContainer == nullptr || pContainer->layout()->count() == 0 ) {
			return nullptr;
		}

		return dynamic_cast<StatusLED*>(
			pContainer->layout()->itemAt( 0 )->widget()
		);
	};

	// Phase 1: Set all LEDs to Unchecked (grey) — progress starts at 0%
	for ( int i = 0; i < nTotal; ++i ) {
		auto* pLed = getLed( i );
		if ( pLed != nullptr ) {
			pLed->setState( StatusLED::State::Unchecked );
		}
	}
	QCoreApplication::processEvents();

	// Phase 2: Check each source, LEDs update to Online/Offline
	for ( int i = 0; i < nTotal; ++i ) {
		auto* pEdit = qobject_cast<QLineEdit*>( m_pTable->cellWidget( i, 1 ) );
		auto* pLed = getLed( i );
		if ( pEdit == nullptr || pLed == nullptr ) {
			continue;
		}

		const QString sUrl = pEdit->text().trimmed();
		if ( sUrl.isEmpty() ) {
			continue;
		}

		QString sError;
		OnlineImporter importer;
		const auto index =
			importer.fetchAndParseIndex( QUrl( sUrl ), 5000, &sError );
		if ( sError.isEmpty() && !index.sVersion.isEmpty() ) {
			pLed->setState( StatusLED::State::Online );
		}
		else {
			pLed->setState( StatusLED::State::Offline );
		}
		QCoreApplication::processEvents();
	}

	m_pCheckButton->setEnabled( true );
}

void OnlineImportSourcesDialog::updateStyleSheet()
{
	const QColor backgroundColor =
		Preferences::get_instance()->getColorTheme()->m_midLightColor;

	m_pTable->setStyleSheet(
		QString( "                \
QTableWidget {                              \
    background-color: %1;                   \
    border: none;                           \
    selection-background-color: %1;         \
}                                           \
" )
			.arg( backgroundColor.name() )
			.append( Skin::getToolButtonStyle( backgroundColor ) )
	);
}
