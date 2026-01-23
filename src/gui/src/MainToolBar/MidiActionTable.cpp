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

#include "MidiActionTable.h"

#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../UndoActions.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/MidiSenseWidget.h"

#include <core/Basics/InstrumentComponent.h>
#include <core/Globals.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiEvent.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include <QHeaderView>

using namespace H2Core;

MidiActionTable::MidiActionTable( QWidget* pParent ) : QTableWidget( pParent )
{
	// Add an "empty" action used to reset the combo box.
	m_availableActions << "";

	const auto pActionHandler =
		Hydrogen::get_instance()->getMidiActionManager();
	for ( const auto& ttype : pActionHandler->getMidiActions() ) {
		m_availableActions << MidiAction::typeToQString( ttype );
	}

	QStringList items;
	items << "" << tr( "Incoming Event" ) << tr( "E. Para." ) << tr( "Action" )
		  << tr( "Para. 1" ) << tr( "Para. 2" ) << tr( "Para. 3" ) << "";

	setRowCount( 0 );
	setColumnCount( 8 );

	verticalHeader()->hide();

	setHorizontalHeaderLabels( items );

	setColumnWidth( 0, MidiActionTable::nColumnButtonWidth );
	setColumnWidth( 1, MidiActionTable::nDefaultComboWidth );
	setColumnWidth( 2, MidiActionTable::nSpinBoxWidth );
	setColumnWidth( 3, MidiActionTable::nDefaultComboWidth );
	setColumnWidth( 4, MidiActionTable::nSpinBoxWidth );
	setColumnWidth( 5, MidiActionTable::nSpinBoxWidth );
	setColumnWidth( 6, MidiActionTable::nSpinBoxWidth );
	setColumnWidth( 7, MidiActionTable::nColumnButtonWidth );

	// When resizing the table all of the new space should go into the
	// combo boxes. They can hold long strings which per default do
	// not fit the column width. The spin boxes, however, only show
	// numbers up to 127 and do not need more width.
	horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Fixed );
	horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Stretch );
	horizontalHeader()->setSectionResizeMode( 2, QHeaderView::Fixed );
	horizontalHeader()->setSectionResizeMode( 3, QHeaderView::Stretch );
	horizontalHeader()->setSectionResizeMode( 4, QHeaderView::Fixed );
	horizontalHeader()->setSectionResizeMode( 5, QHeaderView::Fixed );
	horizontalHeader()->setSectionResizeMode( 6, QHeaderView::Fixed );
	horizontalHeader()->setSectionResizeMode( 7, QHeaderView::Fixed );

	updateTable();

	m_nCurrentMidiAutosenseRow = 0;

	HydrogenApp::get_instance()->addEventListener( this );
    installEventFilter( HydrogenApp::get_instance()->getMainForm() );
}

MidiActionTable::~MidiActionTable()
{
	for ( int nnRow = 0; nnRow < rowCount(); nnRow++ ) {
		for ( int nnColumn = 0; nnColumn < columnCount(); ++nnColumn ) {
			delete cellWidget( nnRow, nnColumn );
		}
	}

	HydrogenApp::get_instance()->removeEventListener( this );
}

void MidiActionTable::updateTable()
{
	const auto pMidiEventMap = Preferences::get_instance()->getMidiEventMap();

	m_cachedEventMap.clear();

	int nIndex = 0;
	for ( const auto& ppEvent : pMidiEventMap->getMidiEvents() ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr &&
			 !ppEvent->getMidiAction()->isNull() ) {
			if ( nIndex >= rowCount() ) {
				appendNewRow();
			}
			updateRow( ppEvent, nIndex );

			m_cachedEventMap[nIndex] = ppEvent;

			++nIndex;
		}
	}

	// All other rows are superfluous ones and have to be removed.
	for ( int ii = nIndex; ii < rowCount(); ++ii ) {
		removeRow( ii );
	}
}

void MidiActionTable::appendNewRow()
{
	auto pMidiActionManager = Hydrogen::get_instance()->getMidiActionManager();

	const int nNewRow = rowCount();
	setRowCount( rowCount() + 1 );

	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	}
	else {
		sIconPath.append( "/icons/black/" );
	}

	auto pMidiSenseButton = new QToolButton( this );
	pMidiSenseButton->setObjectName( "pMidiSenseButton" );
	pMidiSenseButton->setIcon( QIcon( sIconPath + "record.svg" ) );
	pMidiSenseButton->setIconSize( QSize( 13, 13 ) );
	pMidiSenseButton->setToolTip( tr( "press button to record midi event" ) );
	connect( pMidiSenseButton, &QPushButton::clicked, [=]() {
		midiSensePressed( nNewRow );
	} );
	setCellWidget( nNewRow, 0, pMidiSenseButton );

	auto pEventTypeComboBox = new LCDCombo( this );
	pEventTypeComboBox->setMinimumSize(
		QSize( MidiActionTable::nMinComboWidth, MidiActionTable::nRowHeight )
	);
	pEventTypeComboBox->setMaximumSize(
		QSize( MidiActionTable::nMaxComboWidth, MidiActionTable::nRowHeight )
	);
	pEventTypeComboBox->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed
	);
	pEventTypeComboBox->insertItems( 0, MidiEvent::getAllTypes() );

	connect(
		pEventTypeComboBox, QOverload<int>::of( &QComboBox::activated ),
		[=]() {
			updateRow( nNewRow );
			saveRow( nNewRow );
		}
	);
	setCellWidget( nNewRow, 1, pEventTypeComboBox );

	auto pEventParameterSpinBox = new LCDSpinBox(
		this,
		QSize( MidiActionTable::nSpinBoxWidth, MidiActionTable::nRowHeight )
	);
	pEventParameterSpinBox->setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Fixed
	);
	pEventParameterSpinBox->hide();
	pEventParameterSpinBox->setMaximum( static_cast<int>( Midi::ParameterMaximum
	) );
	setCellWidget( nNewRow, 2, pEventParameterSpinBox );
	connect(
		pEventParameterSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[=]() { saveRow( nNewRow ); }
	);

	auto pActionTypeComboBox = new LCDCombo( this );
	pActionTypeComboBox->setMinimumSize(
		QSize( MidiActionTable::nMinComboWidth, MidiActionTable::nRowHeight )
	);
	pActionTypeComboBox->setMaximumSize(
		QSize( MidiActionTable::nMaxComboWidth, MidiActionTable::nRowHeight )
	);
	pActionTypeComboBox->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed
	);
	pActionTypeComboBox->insertItems( 0, m_availableActions );
	connect(
		pActionTypeComboBox, QOverload<int>::of( &QComboBox::activated ),
		[=]() {
			// Find row and update it.
			updateRow( nNewRow );
			saveRow( nNewRow );
		}
	);
	setCellWidget( nNewRow, 3, pActionTypeComboBox );

	auto pActionParameterSpinBox1 = new LCDSpinBox(
		this,
		QSize( MidiActionTable::nSpinBoxWidth, MidiActionTable::nRowHeight )
	);
	pActionParameterSpinBox1->setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Fixed
	);
	pActionParameterSpinBox1->setMaximum( 999 );
	pActionParameterSpinBox1->hide();
	setCellWidget( nNewRow, 4, pActionParameterSpinBox1 );
	connect(
		pActionParameterSpinBox1,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[=]() { saveRow( nNewRow ); }
	);

	auto pActionParameterSpinBox2 = new LCDSpinBox(
		this,
		QSize( MidiActionTable::nSpinBoxWidth, MidiActionTable::nRowHeight )
	);
	pActionParameterSpinBox2->setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Fixed
	);
	// Since this spinner handles both the FX and component number, this hard
	// coded value could in principle - as the user is allowed to add arbitrary
	// components to an instrument - be set too low. But I doubt this will ever
	// happen in a real-world example.
	pActionParameterSpinBox2->setMaximum( 999 );
	pActionParameterSpinBox2->hide();
	setCellWidget( nNewRow, 5, pActionParameterSpinBox2 );
	connect(
		pActionParameterSpinBox2,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[=]() { saveRow( nNewRow ); }
	);

	auto pActionParameterSpinBox3 = new LCDSpinBox(
		this,
		QSize( MidiActionTable::nSpinBoxWidth, MidiActionTable::nRowHeight )
	);
	pActionParameterSpinBox3->setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Fixed
	);
	pActionParameterSpinBox3->setMaximum( 999 );
	pActionParameterSpinBox3->hide();
	setCellWidget( nNewRow, 6, pActionParameterSpinBox3 );
	connect(
		pActionParameterSpinBox3,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[=]() { saveRow( nNewRow ); }
	);

	auto pDeleteRowButton = new QToolButton( this );
	pDeleteRowButton->setObjectName( "MidiActionDeleteRowButton" );
	pDeleteRowButton->setIcon( QIcon( sIconPath + "bin.svg" ) );
	pDeleteRowButton->setIconSize( QSize( 18, 18 ) );
	pDeleteRowButton->setToolTip( tr( "press to delete row" ) );
	connect( pDeleteRowButton, &QPushButton::clicked, [=]() {
		const auto pOldMidiEvent = m_cachedEventMap[nNewRow];
		if ( pOldMidiEvent != nullptr ) {
			long nEventIdRemove = Event::nInvalidId;
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_editMidiEventsAction(
					nullptr, pOldMidiEvent, nullptr, &nEventIdRemove
				)
			);

			if ( nEventIdRemove != Event::nInvalidId ) {
				blacklistEventId( nEventIdRemove );
			}
			updateTable();
		}
	} );
	setCellWidget( nNewRow, 7, pDeleteRowButton );
}

void MidiActionTable::midiMapChangedEvent()
{
	updateTable();
}

void MidiActionTable::midiSensePressed( int nRow )
{
	m_nCurrentMidiAutosenseRow = nRow;
	MidiSenseWidget midiSenseWidget( this );
	midiSenseWidget.exec();
	if ( midiSenseWidget.getLastMidiEvent() == MidiEvent::Type::Null ) {
		// Rejected
		return;
	}

	auto pEventComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 1 ) );
	auto pEventSpinBox = dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 2 ) );
	if ( pEventComboBox == nullptr ) {
		ERRORLOG( QString( "No event combobox in row [%1]" ).arg( nRow ) );
		return;
	}
	if ( pEventSpinBox == nullptr ) {
		ERRORLOG( QString( "No event spinner in row [%1]" ).arg( nRow ) );
		return;
	}

	pEventComboBox->setCurrentIndex( pEventComboBox->findText(
		MidiEvent::TypeToQString( midiSenseWidget.getLastMidiEvent() )
	) );
	pEventSpinBox->setValue(
		static_cast<int>( midiSenseWidget.getLastMidiEventParameter() ),
		Event::Trigger::Suppress
	);

	updateRow( nRow );
	saveRow( nRow );
}

void MidiActionTable::updateRow( int nRow )
{
	auto pEventTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 1 ) );
	auto pEventParameterSpinBox =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 2 ) );
	auto pActionTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 3 ) );
	auto pActionParameterSpinBox1 =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 4 ) );
	auto pActionParameterSpinBox2 =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 5 ) );
	auto pActionParameterSpinBox3 =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 6 ) );

	MidiEvent::Type eventType = MidiEvent::Type::Null;
	if ( !pEventTypeComboBox->currentText().isEmpty() ) {
		eventType =
			MidiEvent::QStringToType( pEventTypeComboBox->currentText() );
	}
	pEventParameterSpinBox->setVisible(
		eventType == MidiEvent::Type::CC || eventType == MidiEvent::Type::Note
	);

	MidiAction::Type actionType = MidiAction::Type::Null;
	if ( !pActionTypeComboBox->currentText().isEmpty() ) {
		actionType =
			MidiAction::parseType( pActionTypeComboBox->currentText() );
	}
	const int nActionParameters =
		Hydrogen::get_instance()->getMidiActionManager()->getParameterNumber(
			actionType
		);
	pActionParameterSpinBox1->setVisible( nActionParameters >= 1 );
	pActionParameterSpinBox2->setVisible( nActionParameters >= 2 );
	pActionParameterSpinBox3->setVisible( nActionParameters >= 3 );
}

void MidiActionTable::updateRow( std::shared_ptr<MidiEvent> pEvent, int nRow )
{
	if ( pEvent == nullptr || pEvent->getMidiAction() == nullptr ) {
		ERRORLOG( "Invalid event" );
		return;
	}
	const auto pMidiAction = pEvent->getMidiAction();

	auto pEventTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 1 ) );
	pEventTypeComboBox->setCurrentIndex( pEventTypeComboBox->findText(
		MidiEvent::TypeToQString( pEvent->getType() )
	) );

	auto pEventParameterSpinBox =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 2 ) );
	pEventParameterSpinBox->setValue(
		static_cast<int>( pEvent->getParameter() ), Event::Trigger::Suppress
	);
	pEventParameterSpinBox->setVisible(
		pEvent->getType() == MidiEvent::Type::CC ||
		pEvent->getType() == MidiEvent::Type::Note
	);

	auto pActionTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 3 ) );
	pActionTypeComboBox->setCurrentIndex( pActionTypeComboBox->findText(
		MidiAction::typeToQString( pMidiAction->getType() )
	) );

	auto pActionParameterSpinBox1 =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 4 ) );
	auto pActionParameterSpinBox2 =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 5 ) );
	auto pActionParameterSpinBox3 =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 6 ) );
	pActionParameterSpinBox1->setValue(
		pMidiAction->getParameter1().toInt(), Event::Trigger::Suppress
	);
	pActionParameterSpinBox2->setValue(
		pMidiAction->getParameter2().toInt(), Event::Trigger::Suppress
	);
	pActionParameterSpinBox3->setValue(
		pMidiAction->getParameter3().toInt(), Event::Trigger::Suppress
	);

	const int nActionParameters =
		Hydrogen::get_instance()->getMidiActionManager()->getParameterNumber(
			pMidiAction->getType()
		);
	pActionParameterSpinBox1->setVisible( nActionParameters >= 1 );
	pActionParameterSpinBox2->setVisible( nActionParameters >= 2 );
	pActionParameterSpinBox3->setVisible( nActionParameters >= 3 );
}

void MidiActionTable::saveRow( int nRow )
{
	auto pEventTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 1 ) );
	auto pEventParameterSpinBox =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 2 ) );
	auto pActionTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 3 ) );
	auto pActionParameterSpinBox1 =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 4 ) );
	auto pActionParameterSpinBox2 =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 5 ) );
	auto pActionParameterSpinBox3 =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 6 ) );

	MidiEvent::Type eventType = MidiEvent::Type::Null;
	Midi::Parameter eventParameter = Midi::ParameterInvalid;
	std::shared_ptr<MidiAction> pNewMidiAction;
	std::shared_ptr<MidiEvent> pNewMidiEvent;
	if ( ! pEventTypeComboBox->currentText().isEmpty() &&
		 ! pActionTypeComboBox->currentText().isEmpty() ) {
		eventType =
			MidiEvent::QStringToType( pEventTypeComboBox->currentText() );
		eventParameter =
			Midi::parameterFromIntClamp( pEventParameterSpinBox->value() );

		pNewMidiAction = std::make_shared<MidiAction>(
			MidiAction::parseType( pActionTypeComboBox->currentText() )
		);

		if ( pActionParameterSpinBox1->cleanText() != "" ) {
			pNewMidiAction->setParameter1( pActionParameterSpinBox1->cleanText()
			);
		}
		if ( pActionParameterSpinBox2->cleanText() != "" ) {
			pNewMidiAction->setParameter2( pActionParameterSpinBox2->cleanText()
			);
		}
		if ( pActionParameterSpinBox3->cleanText() != "" ) {
			pNewMidiAction->setParameter3( pActionParameterSpinBox3->cleanText()
			);
		}

		if ( eventType != MidiEvent::Type::Null &&
			 pNewMidiAction->getType() != MidiAction::Type::Null ) {
			pNewMidiEvent = std::make_shared<MidiEvent>();
			pNewMidiEvent->setType( eventType );
			pNewMidiEvent->setParameter( eventParameter );
			pNewMidiEvent->setMidiAction( pNewMidiAction );
		}
	}

	const auto pOldMidiEvent = m_cachedEventMap[nRow];

	m_cachedEventMap[nRow] = pNewMidiEvent;

	long nEventIdAdd = Event::nInvalidId;
	long nEventIdRemove = Event::nInvalidId;
	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_editMidiEventsAction(
			pNewMidiEvent, pOldMidiEvent, &nEventIdAdd, &nEventIdRemove
		),
		QString( "MidiActionTable::%1" ).arg( nRow )
	);

	if ( nEventIdAdd != Event::nInvalidId ) {
		blacklistEventId( nEventIdAdd );
	}
	if ( nEventIdRemove != Event::nInvalidId ) {
		blacklistEventId( nEventIdRemove );
	}
}

// Reimplementing this one is quite expensive. But the visibility of
// the spinBoxes is reset after the end of updateTable(). In addition,
// the function is only called frequently when interacting the the
// table via mouse. This won't happen too often.
void MidiActionTable::paintEvent( QPaintEvent* ev )
{
	QTableWidget::paintEvent( ev );
	for ( int nnRow = 0; nnRow < rowCount(); ++nnRow ) {
		updateRow( nnRow );
	}
}
