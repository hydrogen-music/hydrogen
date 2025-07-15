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
 * along with this program. If not, see 
https://www.gnu.org/licenses
 *
 */


#include "MidiControlDialog.h"

#include "MainToolBar.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

#include <core/Hydrogen.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>


using namespace H2Core;

MidiControlDialog::MidiControlDialog( QWidget* pParent )
	: QDialog( pParent )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setFixedSize( MidiControlDialog::nWidth, MidiControlDialog::nHeight );
	setFocusPolicy( Qt::NoFocus );
	setObjectName( "MidiControlDialog" );
	// Not translated because it would make explanation within tickets more
	// difficult.
	setWindowTitle( "MidiControlDialog" );

	auto pMainLayout = new QVBoxLayout( this );
	setLayout( pMainLayout );

	m_pTabWidget = new QTabWidget( this );
	pMainLayout->addWidget( m_pTabWidget );

	auto pInputWidget = new QWidget( m_pTabWidget );
	m_pTabWidget->addTab( pInputWidget, tr( "Incoming" ) );
	auto pInputLayout = new QVBoxLayout( pInputWidget );
	pInputLayout->setContentsMargins( 0, 0, 0, 0 );
	pInputLayout->setSpacing( 1 );
	pInputWidget->setLayout( pInputLayout );

	m_pMidiInputTable = new QTableWidget( this );
	m_pMidiInputTable->setColumnCount( 7 );
	m_pMidiInputTable->setHorizontalHeaderLabels(
		QStringList() << tr( "Timestamp" ) << tr( "Type" ) << tr( "Data1" ) <<
		tr( "Data2" ) << tr( "Channel" ) << tr( "Action" ) << tr( "Instrument" ) );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Fixed );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		1, QHeaderView::Fixed );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		2, QHeaderView::Fixed );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		3, QHeaderView::Fixed );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		4, QHeaderView::Fixed );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		5, QHeaderView::Stretch );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		6, QHeaderView::Stretch );
	pInputLayout->addWidget( m_pMidiInputTable );

	const auto binButtonSize = QSize(
		MidiControlDialog::nBinButtonHeight * Skin::fButtonWidthHeightRatio,
		MidiControlDialog::nBinButtonHeight );

	auto addBinButton = [&]( QWidget* pParent ) {
		auto pContainerWidget = new QWidget( pParent );
		pParent->layout()->addWidget( pContainerWidget );
		auto pContainerLayout = new QHBoxLayout( pContainerWidget );
		pContainerLayout->setAlignment( Qt::AlignRight );
		pContainerLayout->setContentsMargins( 1, 1, 1, 1 );
		pContainerWidget->setLayout( pContainerLayout );

		auto pBinButton = new QToolButton( pContainerWidget );
		pBinButton->setCheckable( false );
		pBinButton->setFixedSize( binButtonSize );
		pBinButton->setIconSize(
			binButtonSize - QSize( MidiControlDialog::nBinButtonMargin,
								   MidiControlDialog::nBinButtonMargin ) );
		pContainerLayout->addWidget( pBinButton );

		return pBinButton;
	};
	m_pInputBinButton = addBinButton( pInputWidget );
	connect( m_pInputBinButton, &QToolButton::clicked, [&]() {
		Hydrogen::get_instance()->getMidiDriver()->clearHandledInput();
		updateInputTable();
	});

	auto pOutputWidget = new QWidget( m_pTabWidget );
	m_pTabWidget->addTab( pOutputWidget, tr( "Outgoing" ) );
	auto pOutputLayout = new QVBoxLayout( pOutputWidget );
	pOutputLayout->setContentsMargins( 0, 0, 0, 0 );
	pOutputLayout->setSpacing( 1 );
	pOutputWidget->setLayout( pOutputLayout );

	m_pMidiOutputTable = new QTableWidget( pOutputWidget );
	m_pMidiOutputTable->setColumnCount( 5 );
	m_pMidiOutputTable->setHorizontalHeaderLabels(
		QStringList() << tr( "Timestamp" ) << tr( "Type" ) << tr( "Data1" ) <<
		tr( "Data2" ) << tr( "Channel" ) );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Stretch );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		1, QHeaderView::Stretch );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		2, QHeaderView::Stretch );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		3, QHeaderView::Stretch );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		4, QHeaderView::Stretch );

	pOutputLayout->addWidget( m_pMidiOutputTable );
	m_pOutputBinButton = addBinButton( pOutputWidget );
	connect( m_pOutputBinButton, &QToolButton::clicked, [&]() {
		Hydrogen::get_instance()->getMidiDriver()->clearHandledOutput();
		updateOutputTable();
	});

	updateFont();
	updateIcons();
	updateInputTable();
	updateOutputTable();

	HydrogenApp::get_instance()->addEventListener( this );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &MidiControlDialog::onPreferencesChanged );
}

MidiControlDialog::~MidiControlDialog() {
}


void MidiControlDialog::midiDriverChangedEvent() {
}

void MidiControlDialog::midiInputEvent() {
	updateInputTable();
}

void MidiControlDialog::midiOutputEvent() {
	updateOutputTable();
}

void MidiControlDialog::onPreferencesChanged(
	const H2Core::Preferences::Changes& changes )
{
	if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		updateIcons();
	}
	if ( changes & H2Core::Preferences::Changes::Font ) {
		updateFont();
	}
}

QString MidiControlDialog::timestampToQString( QTime timestamp ) {
	return timestamp.toString( "HH:mm:ss.zzz" );
}

void MidiControlDialog::hideEvent( QHideEvent* pEvent ) {
	UNUSED( pEvent );

	// Update corresponding button
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
}

void MidiControlDialog::showEvent( QShowEvent* pEvent ) {
	UNUSED( pEvent );

	// Update corresponding button
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
}

void MidiControlDialog::updateFont() {
	const auto pPref = H2Core::Preferences::get_instance();

	QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
				getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	QFont childFont( pPref->getTheme().m_font.m_sLevel2FontFamily,
					 getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	setFont( font );

	// In order to affect the fonts of all child widgets in the table as well,
	// we have to use its stylesheet instead of the setFont() method.
	const auto sTableStyle = QString( "\
font-family: %1; \
font-size: %2; \
" )
		.arg( childFont.family() )
		.arg( childFont.pixelSize() );

	m_pMidiInputTable->setStyleSheet( sTableStyle );
	m_pMidiOutputTable->setStyleSheet( sTableStyle );
}

void MidiControlDialog::updateIcons() {
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getTheme().m_interface.m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	m_pInputBinButton->setIcon( QIcon( sIconPath + "bin.svg" ) );
	m_pOutputBinButton->setIcon( QIcon( sIconPath + "bin.svg" ) );

}

void MidiControlDialog::updateInputTable() {
	const auto handledInputs = Hydrogen::get_instance()->getAudioEngine()->
		getMidiDriver()->getHandledInputs();

	// First, we check whether the table holds mostly the same entries and we
	// just have to insert a new one at the bottom.
	bool bInvalid = false;
	for ( int ii = 0; ii < handledInputs.size(); ++ii ) {
		if ( ii < m_pMidiInputTable->rowCount() ) {
			auto ppLabel = dynamic_cast<QLabel*>(
				m_pMidiInputTable->cellWidget( ii, 0 ) );
			if ( ppLabel == nullptr || ppLabel->text() !=
				 timestampToQString( handledInputs[ ii ].timestamp ) ) {
				bInvalid = true;
				break;
			}
		}
	}

	auto newLabel = [&]( const QString& sText ) {
		auto pLabel = new QLabel( this );
		pLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
		pLabel->setAlignment( Qt::AlignCenter );
		pLabel->setText( sText );

		return pLabel;
	};

	auto addRow = [&]( const MidiBaseDriver::HandledInput& handledInput,
					   int nRow ) {
		m_pMidiInputTable->setCellWidget(
			nRow, 0, newLabel( timestampToQString( handledInput.timestamp ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 1, newLabel( MidiMessage::TypeToQString( handledInput.type ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 2, newLabel( QString::number( handledInput.nData1 ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 3, newLabel( QString::number( handledInput.nData2 ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 4, newLabel( QString::number( handledInput.nChannel ) ) );

		QStringList types;
		for ( const auto& ttype : handledInput.actionTypes ) {
			types << MidiAction::typeToQString( ttype );
		}
		m_pMidiInputTable->setCellWidget(
			nRow, 5, newLabel( types.join( ", " ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 6, newLabel( handledInput.mappedInstruments.join( ", " ) ) );
	};

	m_pMidiInputTable->setRowCount( handledInputs.size() );

	// Table is in prestine shape. We just add the missing rows.
	if ( ! bInvalid ) {
		for ( int ii = m_pMidiInputTable->rowCount() - 1;
			  ii < handledInputs.size(); ++ii ) {
			addRow( handledInputs[ ii ], ii );
		}
	}
	else {
		// Clear and renew the whole table.
		m_pMidiInputTable->clearContents();
		for ( int ii = 0; ii < handledInputs.size(); ++ii ) {
			addRow( handledInputs[ ii ], ii );
		}
	}
}

void MidiControlDialog::updateOutputTable() {
	const auto handledOutputs = Hydrogen::get_instance()->getAudioEngine()->
		getMidiDriver()->getHandledOutputs();

	// First, we check whether the table holds mostly the same entries and we
	// just have to insert a new one at the bottom.
	bool bInvalid = false;
	for ( int ii = 0; ii < handledOutputs.size(); ++ii ) {
		if ( ii < m_pMidiOutputTable->rowCount() ) {
			auto ppLabel = dynamic_cast<QLabel*>(
				m_pMidiOutputTable->cellWidget( ii, 0 ) );
			if ( ppLabel == nullptr || ppLabel->text() !=
				 timestampToQString( handledOutputs[ ii ].timestamp ) ) {
				bInvalid = true;
				break;
			}
		}
	}

	auto newLabel = [&]( const QString& sText ) {
		auto pLabel = new QLabel( this );
		pLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
		pLabel->setAlignment( Qt::AlignCenter );
		pLabel->setText( sText );

		return pLabel;
	};

	auto addRow = [&]( const MidiBaseDriver::HandledOutput& handledOutput,
					   int nRow ) {
		m_pMidiOutputTable->setCellWidget(
			nRow, 0, newLabel( timestampToQString( handledOutput.timestamp ) ) );
		m_pMidiOutputTable->setCellWidget(
			nRow, 1, newLabel( MidiMessage::TypeToQString( handledOutput.type ) ) );
		m_pMidiOutputTable->setCellWidget(
			nRow, 2, newLabel( QString::number( handledOutput.nData1 ) ) );
		m_pMidiOutputTable->setCellWidget(
			nRow, 3, newLabel( QString::number( handledOutput.nData2 ) ) );
		m_pMidiOutputTable->setCellWidget(
			nRow, 4, newLabel( QString::number( handledOutput.nChannel ) ) );
	};

	m_pMidiOutputTable->setRowCount( handledOutputs.size() );

	// Table is in prestine shape. We just add the missing rows.
	if ( ! bInvalid ) {
		for ( int ii = m_pMidiOutputTable->rowCount() - 1;
			  ii < handledOutputs.size(); ++ii ) {
			addRow( handledOutputs[ ii ], ii );
		}
	}
	else {
		// Clear and renew the whole table.
		m_pMidiOutputTable->clearContents();
		for ( int ii = 0; ii < handledOutputs.size(); ++ii ) {
			addRow( handledOutputs[ ii ], ii );
		}
	}
}
