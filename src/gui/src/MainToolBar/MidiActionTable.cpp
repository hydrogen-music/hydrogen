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
#include "../Widgets/Button.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/MidiSenseWidget.h"

#include <core/Basics/InstrumentComponent.h>
#include <core/Globals.h>
#include <core/Hydrogen.h>
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

	setColumnCount( 8 );

	setSelectionMode( QAbstractItemView::NoSelection );
	verticalHeader()->hide();

	setHorizontalHeaderLabels( items );

	setColumnWidth( 0, MidiActionTable::nColumnButtonWidth );
	setColumnWidth( 1, MidiActionTable::nDefaultComboWidth );
	setColumnWidth( 2, MidiActionTable::nSpinBoxWidth );
	setColumnWidth( 3, MidiActionTable::nDefaultComboWidth );
	setColumnWidth(
		4, MidiActionTable::nSpinBoxWidth + MidiActionTable::nColumnButtonWidth
	);
	setColumnWidth(
		5, MidiActionTable::nSpinBoxWidth + MidiActionTable::nColumnButtonWidth
	);
	setColumnWidth(
		6, MidiActionTable::nSpinBoxWidth + MidiActionTable::nColumnButtonWidth
	);
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

	resetTable();

	installEventFilter( HydrogenApp::get_instance()->getMainForm() );
}

MidiActionTable::~MidiActionTable()
{
	for ( int nnRow = 0; nnRow < rowCount(); nnRow++ ) {
		for ( int nnColumn = 0; nnColumn < columnCount(); ++nnColumn ) {
			delete cellWidget( nnRow, nnColumn );
		}
	}
}

void MidiActionTable::resetTable()
{
	for ( int nnRow = rowCount() - 1; nnRow >= 0; --nnRow ) {
		QTableWidget::removeRow( nnRow );
	}
	setRowCount( 0 );

	// Initialize all data with the ones found in data structure loaded from
	// disk. Afterwards, #m_tableData will serve as our single source of truth.
	const auto midiEvents =
		Preferences::get_instance()->getMidiEventMap()->getMidiEvents();

	m_tableRows.clear();
	for ( const auto& ppEvent : midiEvents ) {
		if ( ppEvent != nullptr && ppEvent->getMidiAction() != nullptr ) {
			RowContent row = {
				ppEvent->getType(), ppEvent->getParameter(),
				ppEvent->getMidiAction()
			};
			m_tableRows.push_back( std::move( row ) );
		}
	}

	int nIndex = 0;
	for ( const auto& rrow : m_tableRows ) {
		appendEmptyRow();
		updateRowContent(
			nIndex, rrow.eventType, rrow.eventParameter, rrow.pMidiAction
		);
		++nIndex;
	}
}

void MidiActionTable::insertRow(
	int nRow,
	H2Core::MidiEvent::Type eventType,
	H2Core::Midi::Parameter eventParameter,
	std::shared_ptr<MidiAction> pMidiAction
)
{
	if ( eventType != MidiEvent::Type::Null && pMidiAction != nullptr &&
		 pMidiAction->getType() != MidiAction::Type::Null ) {
		long nEventId;
		H2Core::Preferences::get_instance()->getMidiEventMap()->registerEvent(
			eventType, eventParameter, pMidiAction, Event::Trigger::Default,
			&nEventId
		);
		if ( nEventId != Event::nInvalidId ) {
			blacklistEventId( nEventId );
		}
	}

	RowContent newRow = { eventType, eventParameter, pMidiAction };
	if ( nRow >= 0 && nRow < m_tableRows.size() ) {
		m_tableRows.insert( m_tableRows.begin() + nRow, newRow );
	}
	else {
		// Append
		m_tableRows.push_back( newRow );
	}

	appendEmptyRow();

	// Update the content of the entire table since we inserted at an arbitrary
	// row.
	int nIndex = 0;
	for ( const auto& rrow : m_tableRows ) {
		updateRowContent(
			nIndex, rrow.eventType, rrow.eventParameter, rrow.pMidiAction
		);
		++nIndex;
	}
}

void MidiActionTable::removeRow( int nRow )
{
	if ( nRow < 0 || nRow >= m_tableRows.size() ) {
		ERRORLOG( QString( "Row [%1] out of bounds [0,%2)" )
					  .arg( nRow )
					  .arg( m_tableRows.size() ) );
		return;
	}

	const auto row = m_tableRows[nRow];
	if ( row.eventType != MidiEvent::Type::Null && row.pMidiAction != nullptr &&
		 row.pMidiAction->getType() != MidiAction::Type::Null ) {
		long nEventId;
		H2Core::Preferences::get_instance()
			->getMidiEventMap()
			->removeRegisteredEvent(
				row.eventType, row.eventParameter, row.pMidiAction, &nEventId
			);
		if ( nEventId != Event::nInvalidId ) {
			blacklistEventId( nEventId );
		}
	}

    m_tableRows.erase( m_tableRows.begin() + nRow );

	QTableWidget::removeRow( nRow );

	// Update the content of the entire table since we remote an arbitrary row.
	int nIndex = 0;
	for ( const auto& rrow : m_tableRows ) {
		updateRowContent(
			nIndex, rrow.eventType, rrow.eventParameter, rrow.pMidiAction
		);
		++nIndex;
	}
}

void MidiActionTable::replaceRow(
	int nRow,
	H2Core::MidiEvent::Type eventType,
	H2Core::Midi::Parameter eventParameter,
	std::shared_ptr<MidiAction> pMidiAction
)
{
	if ( nRow < 0 || nRow >= m_tableRows.size() ) {
		ERRORLOG( QString( "Row [%1] out of bounds [0,%2)" )
					  .arg( nRow )
					  .arg( m_tableRows.size() ) );
		return;
	}

	// Remove old event
	const auto row = m_tableRows[nRow];
	if ( row.eventType != MidiEvent::Type::Null && row.pMidiAction != nullptr &&
		 row.pMidiAction->getType() != MidiAction::Type::Null ) {
		long nEventId;
		H2Core::Preferences::get_instance()
			->getMidiEventMap()
			->removeRegisteredEvent(
				row.eventType, row.eventParameter, row.pMidiAction, &nEventId
			);
		if ( nEventId != Event::nInvalidId ) {
			blacklistEventId( nEventId );
		}
	}

	// Add new one
	if ( eventType != MidiEvent::Type::Null && pMidiAction != nullptr &&
		 pMidiAction->getType() != MidiAction::Type::Null ) {
		long nEventId;
		H2Core::Preferences::get_instance()->getMidiEventMap()->registerEvent(
			eventType, eventParameter, pMidiAction, Event::Trigger::Default,
			&nEventId
		);
		if ( nEventId != Event::nInvalidId ) {
			blacklistEventId( nEventId );
		}
	}

	m_tableRows[nRow].eventType = eventType;
	m_tableRows[nRow].eventParameter = eventParameter;
	m_tableRows[nRow].pMidiAction = pMidiAction;

	updateRowContent( nRow, eventType, eventParameter, pMidiAction );
}

void MidiActionTable::removeRegisteredEvents(
	std::shared_ptr<MidiAction> pMidiAction
)
{
	if ( pMidiAction == nullptr ) {
		return;
	}

	std::vector<std::pair<int, RowContent>> rowsToRemove;
	for ( int nnRow = m_tableRows.size() - 1; nnRow >= 0; --nnRow ) {
		if ( m_tableRows[nnRow].eventType != MidiEvent::Type::Null &&
			 m_tableRows[nnRow].pMidiAction != nullptr &&
			 m_tableRows[nnRow].pMidiAction->isEquivalentTo( pMidiAction ) ) {
			rowsToRemove.push_back( std::make_pair( nnRow, m_tableRows[nnRow] )
			);
		}
	}

	if ( rowsToRemove.size() == 0 ) {
		return;
	}

	/*: Label for entry within the undo/redo history. The name of the action
	 *  will be appended after an additional white space. */
	HydrogenApp::get_instance()->beginUndoMacro(
		QString( "%1 [%2]" )
			.arg( tr( "Remove all binding for MIDI action" ) )
			.arg( MidiAction::typeToQString( pMidiAction->getType() ) )
	);

	for ( const auto [nnRow, rrowContent] : rowsToRemove ) {
		HydrogenApp::get_instance()->pushUndoCommand(
			new SE_addOrRemoveMidiEventsAction(
				nnRow, rrowContent.eventType, rrowContent.eventParameter,
				rrowContent.pMidiAction, false
			)
		);
	}

	HydrogenApp::get_instance()->endUndoMacro();
}

void MidiActionTable::midiSensePressed( int nRow )
{
	if ( nRow < 0 || nRow >= m_tableRows.size() ) {
		ERRORLOG( QString( "Row [%1] out of bounds [0,%2)" )
					  .arg( nRow )
					  .arg( m_tableRows.size() ) );
		return;
	}

	MidiSenseWidget midiSenseWidget( this );
	midiSenseWidget.exec();
	if ( midiSenseWidget.getLastMidiEvent() == MidiEvent::Type::Null ) {
		// Rejected
		return;
	}

	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_replaceMidiEventsAction(
			nRow, midiSenseWidget.getLastMidiEvent(),
			m_tableRows[nRow].eventType,
			midiSenseWidget.getLastMidiEventParameter(),
			m_tableRows[nRow].eventParameter, m_tableRows[nRow].pMidiAction,
			m_tableRows[nRow].pMidiAction
		)
	);
}

void MidiActionTable::appendEmptyRow()
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
		midiSensePressed( findRowOf( pMidiSenseButton ) );
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
			const int nRow = findRowOf( pEventTypeComboBox );
			if ( nRow != -1 ) {
				writeBackRow( nRow );
			}
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
		[=]() { writeBackRow( findRowOf( pEventParameterSpinBox ) ); }
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
			const int nRow = findRowOf( pActionTypeComboBox );
			if ( nRow != -1 ) {
				writeBackRow( nRow );
			}
		}
	);
	setCellWidget( nNewRow, 3, pActionTypeComboBox );

	auto pActionParameterSpinBox1 = new SpinBoxWithIcon( this );
	pActionParameterSpinBox1->hide();
	setCellWidget( nNewRow, 4, pActionParameterSpinBox1 );
	connect(
		pActionParameterSpinBox1->m_pSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[=]() { writeBackRow( findRowOf( pActionParameterSpinBox1 ) ); }
	);

	auto pActionParameterSpinBox2 = new SpinBoxWithIcon( this );
	pActionParameterSpinBox2->hide();
	setCellWidget( nNewRow, 5, pActionParameterSpinBox2 );
	connect(
		pActionParameterSpinBox2->m_pSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[=]() { writeBackRow( findRowOf( pActionParameterSpinBox2 ) ); }
	);

	auto pActionParameterSpinBox3 = new SpinBoxWithIcon( this );
	pActionParameterSpinBox3->hide();
	setCellWidget( nNewRow, 6, pActionParameterSpinBox3 );
	connect(
		pActionParameterSpinBox3->m_pSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[=]() { writeBackRow( findRowOf( pActionParameterSpinBox3 ) ); }
	);

	auto pDeleteRowButton = new QToolButton( this );
	pDeleteRowButton->setObjectName( "MidiActionDeleteRowButton" );
	pDeleteRowButton->setIcon( QIcon( sIconPath + "bin.svg" ) );
	pDeleteRowButton->setIconSize( QSize( 18, 18 ) );
	pDeleteRowButton->setToolTip( tr( "press to delete row" ) );
	connect( pDeleteRowButton, &QPushButton::clicked, [=]() {
		const int nRow = findRowOf( pDeleteRowButton );
		if ( nRow < 0 || nRow >= m_tableRows.size() ) {
			ERRORLOG(
				QString(
					"Delete button associated with incorrect row [%1] [0,%2)"
				)
					.arg( nRow )
					.arg( m_tableRows.size() )
			);
			return;
		}
		HydrogenApp::get_instance()->pushUndoCommand(
			new SE_addOrRemoveMidiEventsAction(
				nRow, m_tableRows[nRow].eventType,
				m_tableRows[nRow].eventParameter, m_tableRows[nRow].pMidiAction,
				false
			)
		);
	} );
	setCellWidget( nNewRow, 7, pDeleteRowButton );
}

void MidiActionTable::updateRowContent(
	int nRow,
	H2Core::MidiEvent::Type eventType,
	H2Core::Midi::Parameter eventParameter,
	std::shared_ptr<MidiAction> pMidiAction
)
{
	auto pEventTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 1 ) );
	if ( pEventTypeComboBox != nullptr ) {
		pEventTypeComboBox->setCurrentIndex( pEventTypeComboBox->findText(
			MidiEvent::TypeToQString( eventType )
		) );
	}

	auto pEventParameterSpinBox =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 2 ) );
	if ( pEventParameterSpinBox != nullptr ) {
		pEventParameterSpinBox->setValue(
			static_cast<int>( eventParameter ), Event::Trigger::Suppress
		);
		pEventParameterSpinBox->setVisible(
			eventType == MidiEvent::Type::CC ||
			eventType == MidiEvent::Type::Note
		);
	}

	const auto actionType = pMidiAction != nullptr ? pMidiAction->getType()
												   : MidiAction::Type::Null;

	auto pActionTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 3 ) );
	if ( pActionTypeComboBox != nullptr ) {
		pActionTypeComboBox->setCurrentIndex( std::max(
			0,
			pActionTypeComboBox->findText( MidiAction::typeToQString( actionType
			) )
		) );
	}

	auto pActionParameterSpinBox1 =
		dynamic_cast<SpinBoxWithIcon*>( cellWidget( nRow, 4 ) );
	auto pActionParameterSpinBox2 =
		dynamic_cast<SpinBoxWithIcon*>( cellWidget( nRow, 5 ) );
	auto pActionParameterSpinBox3 =
		dynamic_cast<SpinBoxWithIcon*>( cellWidget( nRow, 6 ) );

	if ( pActionParameterSpinBox1 != nullptr &&
		 pActionParameterSpinBox2 != nullptr &&
		 pActionParameterSpinBox3 != nullptr ) {
		const int nActionParameters = Hydrogen::get_instance()
										  ->getMidiActionManager()
										  ->getParameterNumber( actionType );
		pActionParameterSpinBox1->setVisible( nActionParameters >= 1 );
		pActionParameterSpinBox2->setVisible( nActionParameters >= 2 );
		pActionParameterSpinBox3->setVisible( nActionParameters >= 3 );

		if ( actionType == MidiAction::Type::Null ) {
			return;
		}
		const auto required = MidiAction::requiresFromType( actionType );

        // For instruments we provide a special value indicating that the action
        // will be applied to the currently selected instrument.
		if ( required & MidiAction::RequiresInstrument ) {
			pActionParameterSpinBox1->m_pSpinBox->setFlag(
				LCDSpinBox::Flag::MinusOneAsCurrentlySelected
			);
			pActionParameterSpinBox1->m_pSpinBox->setMinimum(
				static_cast<int>( MidiAction::nCurrentSelectionParameter )
			);
		}
		else {
			pActionParameterSpinBox1->m_pSpinBox->setFlag(
				LCDSpinBox::Flag::None
			);
			pActionParameterSpinBox1->m_pSpinBox->setMinimum( 0 );
		}

		if ( required & MidiAction::RequiresInstrument ) {
			pActionParameterSpinBox1->m_pSpinBox->setValue(
				pMidiAction->getInstrument(), Event::Trigger::Suppress
			);
		}
		else if ( required & MidiAction::RequiresFactor ) {
			pActionParameterSpinBox1->m_pSpinBox->setValue(
				pMidiAction->getFactor(), Event::Trigger::Suppress
			);
		}
		else if ( required & MidiAction::RequiresPattern ) {
			pActionParameterSpinBox1->m_pSpinBox->setValue(
				pMidiAction->getPattern(), Event::Trigger::Suppress
			);
		}
		else if ( required & MidiAction::RequiresSong ) {
			pActionParameterSpinBox1->m_pSpinBox->setValue(
				pMidiAction->getSong(), Event::Trigger::Suppress
			);
		}

		if ( required & MidiAction::RequiresComponent ) {
			pActionParameterSpinBox2->m_pSpinBox->setValue(
				pMidiAction->getComponent(), Event::Trigger::Suppress
			);
		}
		else if ( required & MidiAction::RequiresFx ) {
			pActionParameterSpinBox2->m_pSpinBox->setValue(
				pMidiAction->getFx(), Event::Trigger::Suppress
			);
		}

		if ( required & MidiAction::RequiresLayer ) {
			pActionParameterSpinBox3->m_pSpinBox->setValue(
				pMidiAction->getLayer(), Event::Trigger::Suppress
			);
		}

		QString sIconPath( Skin::getSvgImagePath() );
		if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
			 InterfaceTheme::IconColor::White ) {
			sIconPath.append( "/icons/white/" );
		}
		else {
			sIconPath.append( "/icons/black/" );
		}

		if ( required & MidiAction::RequiresInstrument ) {
			pActionParameterSpinBox1->m_pButton->setText( "" );
			pActionParameterSpinBox1->m_pButton->setIcon(
				QIcon( sIconPath + "drum.svg" )
			);
		}
		else if ( required & MidiAction::RequiresFactor ) {
			pActionParameterSpinBox1->m_pButton->setIcon( QIcon() );
			pActionParameterSpinBox1->m_pButton->setText(
				QString::fromUtf8( "\u00d7" )
			);
		}
		else if ( required & MidiAction::RequiresPattern ) {
			pActionParameterSpinBox1->m_pButton->setText( "" );
			pActionParameterSpinBox1->m_pButton->setIcon(
				QIcon( sIconPath + "pattern-editor.svg" )
			);
		}
		else if ( required & MidiAction::RequiresSong ) {
			pActionParameterSpinBox1->m_pButton->setText( "" );
			pActionParameterSpinBox1->m_pButton->setIcon(
				QIcon( sIconPath + "song-editor.svg" )
			);
		}

		if ( required & MidiAction::RequiresComponent ) {
			pActionParameterSpinBox2->m_pButton->setText( "" );
			pActionParameterSpinBox2->m_pButton->setIcon(
				QIcon( sIconPath + "component-editor.svg" )
			);
		}
		else if ( required & MidiAction::RequiresFx ) {
			pActionParameterSpinBox2->m_pButton->setIcon( QIcon() );
			pActionParameterSpinBox2->m_pButton->setText( "Fx" );
		}

		if ( required & MidiAction::RequiresLayer ) {
			pActionParameterSpinBox3->m_pButton->setIcon(
				QIcon( sIconPath + "sample-editor.svg" )
			);
		}
	}
}

void MidiActionTable::updateRowVisibility( int nRow )
{
	if ( nRow < 0 || nRow >= m_tableRows.size() ) {
		ERRORLOG( QString( "Row [%1] out of bounds [0,%2)" )
					  .arg( nRow )
					  .arg( m_tableRows.size() ) );
		return;
	}

	auto pEventParameterSpinBox =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 2 ) );
	if ( pEventParameterSpinBox != nullptr ) {
		pEventParameterSpinBox->setVisible(
			m_tableRows[nRow].eventType == MidiEvent::Type::CC ||
			m_tableRows[nRow].eventType == MidiEvent::Type::Note
		);
	}

	const auto actionType = m_tableRows[nRow].pMidiAction != nullptr
								? m_tableRows[nRow].pMidiAction->getType()
								: MidiAction::Type::Null;

	auto pActionParameterSpinBox1 =
		dynamic_cast<SpinBoxWithIcon*>( cellWidget( nRow, 4 ) );
	auto pActionParameterSpinBox2 =
		dynamic_cast<SpinBoxWithIcon*>( cellWidget( nRow, 5 ) );
	auto pActionParameterSpinBox3 =
		dynamic_cast<SpinBoxWithIcon*>( cellWidget( nRow, 6 ) );

	const int nActionParameters =
		Hydrogen::get_instance()->getMidiActionManager()->getParameterNumber(
			actionType
		);
	if ( pActionParameterSpinBox1 != nullptr &&
		 pActionParameterSpinBox2 != nullptr &&
		 pActionParameterSpinBox3 != nullptr ) {
		pActionParameterSpinBox1->setVisible( nActionParameters >= 1 );
		pActionParameterSpinBox2->setVisible( nActionParameters >= 2 );
		pActionParameterSpinBox3->setVisible( nActionParameters >= 3 );
	}
}

void MidiActionTable::writeBackRow( int nRow )
{
	if ( nRow < 0 || nRow >= m_tableRows.size() ) {
		ERRORLOG( QString( "Row [%1] out of bounds [0,%2)" )
					  .arg( nRow )
					  .arg( m_tableRows.size() ) );
		return;
	}

	// Used to define a macro combining all undo events of a single widget
	// within this row.
	QString sChange;

	auto pEventTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 1 ) );
	auto pEventParameterSpinBox =
		dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 2 ) );
	auto pActionTypeComboBox = dynamic_cast<LCDCombo*>( cellWidget( nRow, 3 ) );
	auto pActionParameterSpinBox1 =
		dynamic_cast<SpinBoxWithIcon*>( cellWidget( nRow, 4 ) );
	auto pActionParameterSpinBox2 =
		dynamic_cast<SpinBoxWithIcon*>( cellWidget( nRow, 5 ) );
	auto pActionParameterSpinBox3 =
		dynamic_cast<SpinBoxWithIcon*>( cellWidget( nRow, 6 ) );
	if ( pEventTypeComboBox == nullptr || pEventParameterSpinBox == nullptr ||
		 pActionTypeComboBox == nullptr ||
		 pActionParameterSpinBox1 == nullptr ||
		 pActionParameterSpinBox2 == nullptr ||
		 pActionParameterSpinBox3 == nullptr ) {
		ERRORLOG( "Unable to retrieve widgets" );
		return;
	}

	MidiEvent::Type eventType = MidiEvent::Type::Null;
	Midi::Parameter eventParameter = Midi::ParameterInvalid;
	if ( !pEventTypeComboBox->currentText().isEmpty() ) {
		eventType =
			MidiEvent::QStringToType( pEventTypeComboBox->currentText() );
		eventParameter =
			Midi::parameterFromIntClamp( pEventParameterSpinBox->value() );

		if ( eventType != m_tableRows[nRow].eventType ) {
			sChange = "EventType";
		}
		else if ( eventParameter != m_tableRows[nRow].eventParameter ) {
			sChange = "EventParameter";
		}
	}

	std::shared_ptr<MidiAction> pMidiAction;
	if ( !pActionTypeComboBox->currentText().isEmpty() ) {
		const auto actionType =
			MidiAction::parseType( pActionTypeComboBox->currentText() );
		pMidiAction = std::make_shared<MidiAction>( actionType );

		const auto previsousActionType =
			m_tableRows[nRow].pMidiAction != nullptr
				? m_tableRows[nRow].pMidiAction->getType()
				: MidiAction::Type::Null;
		if ( actionType != previsousActionType ) {
			sChange = "ActionType";
		}

		const auto required = MidiAction::requiresFromType( actionType );

		if ( required & MidiAction::RequiresInstrument ) {
			pMidiAction->setInstrument(
				pActionParameterSpinBox1->m_pSpinBox->value()
			);
		}
		else if ( required & MidiAction::RequiresFactor ) {
			pMidiAction->setFactor( pActionParameterSpinBox1->m_pSpinBox->value(
			) );
		}
		else if ( required & MidiAction::RequiresPattern ) {
			pMidiAction->setPattern(
				pActionParameterSpinBox1->m_pSpinBox->value()
			);
		}
		else if ( required & MidiAction::RequiresSong ) {
			pMidiAction->setSong( pActionParameterSpinBox1->m_pSpinBox->value()
			);
		}

		if ( required & MidiAction::RequiresComponent ) {
			pMidiAction->setComponent(
				pActionParameterSpinBox2->m_pSpinBox->value()
			);
		}
		else if ( required & MidiAction::RequiresFx ) {
			pMidiAction->setFx( pActionParameterSpinBox2->m_pSpinBox->value() );
		}

		if ( required & MidiAction::RequiresLayer ) {
			pMidiAction->setLayer( pActionParameterSpinBox3->m_pSpinBox->value()
			);
		}

		if ( sChange.isEmpty() && m_tableRows[nRow].pMidiAction != nullptr ) {
			QString sParameter1, sParameter2, sParameter3;
			m_tableRows[nRow].pMidiAction->toQStrings(
				&sParameter1, &sParameter2, &sParameter3
			);
			if ( sParameter1 !=
				 QString::number( pActionParameterSpinBox1->m_pSpinBox->value()
				 ) ) {
				sChange = "ActionParameter1";
			}
			else if ( sParameter2 !=
					  QString::number(
						  pActionParameterSpinBox2->m_pSpinBox->value()
					  ) ) {
				sChange = "ActionParameter2";
			}
			else if ( sParameter3 !=
					  QString::number(
						  pActionParameterSpinBox3->m_pSpinBox->value()
					  ) ) {
				sChange = "ActionParameter3";
			}
		}
	}

	const QString sContext = QString( "MidiActionTable::writeBackRow::%1::%2" )
								 .arg( nRow )
								 .arg( sChange );

	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_replaceMidiEventsAction(
			nRow, eventType, m_tableRows[nRow].eventType, eventParameter,
			m_tableRows[nRow].eventParameter, pMidiAction,
			m_tableRows[nRow].pMidiAction
		),
		sContext
	);
}

int MidiActionTable::findRowOf( QWidget* pWidget ) const
{
	if ( pWidget == nullptr ) {
		return -1;
	}

	for ( int nnRow = 0; nnRow < rowCount(); ++nnRow ) {
		for ( int nnColumn = 0; nnColumn < columnCount(); ++nnColumn ) {
			if ( pWidget == cellWidget( nnRow, nnColumn ) ) {
				return nnRow;
			}
		}
	}
	return -1;
}

// Reimplementing this one is quite expensive. But the visibility of
// the spinBoxes is reset after the end of updateTable(). In addition,
// the function is only called frequently when interacting the the
// table via mouse. This won't happen too often.
void MidiActionTable::paintEvent( QPaintEvent* ev )
{
	QTableWidget::paintEvent( ev );

	for ( int nnRow = 0; nnRow < rowCount(); ++nnRow ) {
		updateRowVisibility( nnRow );
	}
}

SpinBoxWithIcon::SpinBoxWithIcon( QWidget* pParent ) : QWidget( pParent )
{
	auto pLayout = new QHBoxLayout();
	pLayout->setSpacing( 0 );
	pLayout->setContentsMargins( 0, 0, 0, 0 );
	setLayout( pLayout );

	m_pButton = new Button(
		this,
		QSize(
			MidiActionTable::nColumnButtonWidth - 2,
			MidiActionTable::nColumnButtonWidth
		),
		Button::Type::Icon
	);
	m_pButton->setIconSize( QSize( 18, 18 ) );
	pLayout->addWidget( m_pButton );

	m_pSpinBox = new LCDSpinBox(
		this,
		QSize( MidiActionTable::nSpinBoxWidth, MidiActionTable::nRowHeight )
	);
	m_pSpinBox->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	m_pSpinBox->setMaximum( 999 );
	pLayout->addWidget( m_pSpinBox );
}

SpinBoxWithIcon::~SpinBoxWithIcon()
{
}
