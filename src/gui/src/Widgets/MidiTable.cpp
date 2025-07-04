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
#include "MidiTable.h"

#include "LCDCombo.h"
#include "LCDSpinBox.h"
#include "MidiSenseWidget.h"
#include "../Skin.h"

#include <core/Basics/InstrumentComponent.h>
#include <core/Hydrogen.h>
#include <core/Globals.h>
#include <core/Midi/MidiCommon.h>
#include <core/Midi/MidiMap.h>
#include <core/Preferences/Preferences.h>

#include <QHeaderView>

MidiTable::MidiTable( QWidget *pParent )
	: QTableWidget( pParent )
	, m_nRowHeight( 29 )
	, m_nColumn0Width( 25 )
	, m_nMinComboWidth( 100 )
	, m_nMaxComboWidth( 1460 )
	, m_nDefaultComboWidth( 146 )
	, m_nSpinBoxWidth( 60 )
 {
	m_nRowCount = 0;
	setupMidiTable();

	m_pUpdateTimer = new QTimer( this );
	m_nCurrentMidiAutosenseRow = 0;
}


MidiTable::~MidiTable()
{
	for( int myRow = 0; myRow <=  m_nRowCount ; myRow++ ) {
		delete cellWidget( myRow, 0 );
		delete cellWidget( myRow, 1 );
		delete cellWidget( myRow, 2 );
		delete cellWidget( myRow, 3 );
		delete cellWidget( myRow, 4 );
		delete cellWidget( myRow, 5 );
		delete cellWidget( myRow, 6 );
	}
}

void MidiTable::midiSensePressed( int nRow ){

	m_nCurrentMidiAutosenseRow = nRow;
	MidiSenseWidget midiSenseWidget( this );
	midiSenseWidget.exec();
	if ( midiSenseWidget.getLastMidiEvent() == H2Core::MidiMessage::Event::Null ) {
		// Rejected
		return;
	}

	LCDCombo* pEventCombo = dynamic_cast<LCDCombo*>( cellWidget( nRow, 1 ) );
	LCDSpinBox* pEventSpinner = dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 2 ) );
	if ( pEventCombo == nullptr ) {
		ERRORLOG( QString( "No event combobox in row [%1]" ).arg( nRow ) );
		return;
	}
	if ( pEventSpinner == nullptr ) {
		ERRORLOG( QString( "No event spinner in row [%1]" ).arg( nRow ) );
		return;
	}

	pEventCombo->setCurrentIndex( pEventCombo->findText(
									 H2Core::MidiMessage::EventToQString(
										 midiSenseWidget.getLastMidiEvent() ) ) );
	pEventSpinner->setValue( midiSenseWidget.getLastMidiEventParameter() );

	m_pUpdateTimer->start( 100 );

	emit changed();
}

// Reimplementing this one is quite expensive. But the visibility of
// the spinBoxes is reset after the end of updateTable(). In addition,
// the function is only called frequently when interacting the the
// table via mouse. This won't happen too often.
void MidiTable::paintEvent( QPaintEvent* ev ) {
	QTableWidget::paintEvent( ev );
	updateTable();
}

void MidiTable::updateTable() {
	if( m_nRowCount > 0 ) {
		// Ensure that the last row is empty
		LCDCombo* pEventCombo =  dynamic_cast<LCDCombo*>( cellWidget( m_nRowCount - 1, 1 ) );
		LCDCombo* pActionCombo = dynamic_cast<LCDCombo*>( cellWidget( m_nRowCount - 1, 3 ) );

		if ( pEventCombo == nullptr || pActionCombo == nullptr ) {
			return;
		}

		if( ! pActionCombo->currentText().isEmpty() && ! pEventCombo->currentText().isEmpty() ) {
			std::shared_ptr<Action> pAction = std::make_shared<Action>();
			insertNewRow( pAction, "", 0 );
		}

		// Ensure that all other empty rows are removed and that the
		// parameter spinboxes are only shown when required for that
		// particular parameter.
		for ( int ii = 0; ii < m_nRowCount; ii++ ) {
			updateRow( ii );
		}
	}
}

void MidiTable::sendChanged() {
	emit changed();
}

void MidiTable::insertNewRow(std::shared_ptr<Action> pAction,
							 const QString& eventString, int eventParameter)
{
	MidiActionManager *pActionHandler = MidiActionManager::get_instance();

	insertRow( m_nRowCount );
	
	int oldRowCount = m_nRowCount;

	++m_nRowCount;

	QPushButton *midiSenseButton = new QPushButton(this);
	midiSenseButton->setObjectName( "MidiSenseButton" );
	midiSenseButton->setIcon(QIcon(Skin::getSvgImagePath() + "/icons/record.svg"));
	midiSenseButton->setIconSize( QSize( 13, 13 ) );
	midiSenseButton->setToolTip( tr("press button to record midi event") );

	connect( midiSenseButton, &QPushButton::clicked, [=](){
		for ( int ii = 0; ii < rowCount(); ii++ ) {
			if ( cellWidget( ii, 0 ) == midiSenseButton ) {
				midiSensePressed( ii );
				return;
			}
		}
		ERRORLOG( QString( "Unable to midiSenseButton of initial row [%1] in MidiTable!" )
				  .arg( oldRowCount ) );
	});
	setCellWidget( oldRowCount, 0, midiSenseButton );

	LCDCombo *eventBox = new LCDCombo(this);
	eventBox->setMinimumSize( QSize( m_nMinComboWidth, m_nRowHeight ) );
	eventBox->setMaximumSize( QSize( m_nMaxComboWidth, m_nRowHeight ) );
	eventBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	eventBox->insertItems( oldRowCount , H2Core::MidiMessage::getEventList() );
	eventBox->setCurrentIndex( eventBox->findText(eventString) );
	connect( eventBox , SIGNAL( currentIndexChanged( int ) ) , this , SLOT( updateTable() ) );
	connect( eventBox , SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( sendChanged() ) );
	setCellWidget( oldRowCount, 1, eventBox );
	
	
	LCDSpinBox *eventParameterSpinner = new LCDSpinBox(
		this, QSize( m_nSpinBoxWidth, m_nRowHeight ) );
	eventParameterSpinner->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	setCellWidget( oldRowCount , 2, eventParameterSpinner );
	eventParameterSpinner->setMaximum( 999 );
	eventParameterSpinner->setValue( eventParameter );
	connect( eventParameterSpinner, SIGNAL( valueChanged( double ) ),
			 this, SLOT( sendChanged() ) );


	LCDCombo *actionBox = new LCDCombo(this);
	actionBox->setMinimumSize( QSize( m_nMinComboWidth, m_nRowHeight ) );
	actionBox->setMaximumSize( QSize( m_nMaxComboWidth, m_nRowHeight ) );
	actionBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	actionBox->insertItems( oldRowCount, pActionHandler->getActionList());
	actionBox->setCurrentIndex ( actionBox->findText( pAction->getType() ) );
	connect( actionBox , SIGNAL( currentIndexChanged( int ) ) , this , SLOT( updateTable() ) );
	connect( actionBox , SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( sendChanged() ) );
	setCellWidget( oldRowCount , 3, actionBox );

	bool ok;
	LCDSpinBox *actionParameterSpinner1 = new LCDSpinBox(
		this, QSize( m_nSpinBoxWidth, m_nRowHeight ) );
	actionParameterSpinner1->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	setCellWidget( oldRowCount , 4, actionParameterSpinner1 );
	actionParameterSpinner1->setMaximum( 999 );
	actionParameterSpinner1->setValue( pAction->getParameter1().toInt(&ok,10) );
	actionParameterSpinner1->hide();
	connect( actionParameterSpinner1, SIGNAL( valueChanged( double ) ),
			 this, SLOT( sendChanged() ) );

	LCDSpinBox *actionParameterSpinner2 = new LCDSpinBox(
		this, QSize( m_nSpinBoxWidth, m_nRowHeight ) );
	actionParameterSpinner2->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	setCellWidget( oldRowCount , 5, actionParameterSpinner2 );
	actionParameterSpinner2->setMaximum( std::max(MAX_FX, MAX_COMPONENTS) );
	actionParameterSpinner2->setValue( pAction->getParameter2().toInt(&ok,10) );
	actionParameterSpinner2->hide();
	connect( actionParameterSpinner2, SIGNAL( valueChanged( double ) ),
			 this, SLOT( sendChanged() ) );

	LCDSpinBox *actionParameterSpinner3 = new LCDSpinBox(
		this, QSize( m_nSpinBoxWidth, m_nRowHeight ) );
	actionParameterSpinner3->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	setCellWidget( oldRowCount , 6, actionParameterSpinner3 );
	actionParameterSpinner3->setMaximum( H2Core::InstrumentComponent::getMaxLayers() );
	actionParameterSpinner3->setValue( pAction->getParameter3().toInt(&ok,10) );
	actionParameterSpinner3->hide();
	connect( actionParameterSpinner3, SIGNAL( valueChanged( double ) ),
			 this, SLOT( sendChanged() ) );
}

void MidiTable::setupMidiTable()
{
	const auto pMidiMap = H2Core::Preferences::get_instance()->getMidiMap();

	QStringList items;
	items << "" << tr("Incoming Event")  << tr("E. Para.")
		  << tr("Action") <<  tr("Para. 1") << tr("Para. 2") << tr("Para. 3");

	setRowCount( 0 );
	setColumnCount( 7 );

	verticalHeader()->hide();

	setHorizontalHeaderLabels( items );

	setColumnWidth( 0, m_nColumn0Width );
	setColumnWidth( 1, m_nDefaultComboWidth );
	setColumnWidth( 2, m_nSpinBoxWidth );
	setColumnWidth( 3, m_nDefaultComboWidth );
	setColumnWidth( 4, m_nSpinBoxWidth );
	setColumnWidth( 5, m_nSpinBoxWidth );
	setColumnWidth( 6, m_nSpinBoxWidth );

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

	for ( const auto& [ssMmcType, ppAction] : pMidiMap->getMMCActionMap() ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			insertNewRow( ppAction, ssMmcType, 0 );
		}
	}

	for ( const auto& [nnPitch, ppAction] : pMidiMap->getNoteActionMap() ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			insertNewRow( ppAction,
						  H2Core::MidiMessage::EventToQString(
							  H2Core::MidiMessage::Event::Note ),
						  nnPitch );
		}
	}

	for ( const auto& [nnParam, ppAction] : pMidiMap->getCCActionMap() ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			insertNewRow( ppAction,
						  H2Core::MidiMessage::EventToQString(
							  H2Core::MidiMessage::Event::CC ),
						  nnParam );
		}
	}

	for ( const auto& ppAction : pMidiMap->getPCActions() ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			insertNewRow( ppAction,
						  H2Core::MidiMessage::EventToQString(
							  H2Core::MidiMessage::Event::PC ), 0 );
		}
	}

	std::shared_ptr<Action> pAction = std::make_shared<Action>();
	insertNewRow( pAction, "", 0 );
}


void MidiTable::saveMidiTable()
{
	auto pMidiMap = H2Core::Preferences::get_instance()->getMidiMap();
	pMidiMap->reset();
	
	for ( int row = 0; row < m_nRowCount; row++ ) {

		LCDCombo * eventCombo =  dynamic_cast <LCDCombo *> ( cellWidget( row, 1 ) );
		LCDSpinBox * eventSpinner = dynamic_cast <LCDSpinBox *> ( cellWidget( row, 2 ) );
		LCDCombo * actionCombo = dynamic_cast <LCDCombo *> ( cellWidget( row, 3 ) );
		LCDSpinBox * actionSpinner1 = dynamic_cast <LCDSpinBox *> ( cellWidget( row, 4 ) );
		LCDSpinBox * actionSpinner2 = dynamic_cast <LCDSpinBox *> ( cellWidget( row, 5 ) );
		LCDSpinBox * actionSpinner3 = dynamic_cast <LCDSpinBox *> ( cellWidget( row, 6 ) );

		if( !eventCombo->currentText().isEmpty() && !actionCombo->currentText().isEmpty() ){
			const QString sEventString = eventCombo->currentText();
			const auto event = H2Core::MidiMessage::QStringToEvent( sEventString );

			const QString actionString = actionCombo->currentText();
		
			std::shared_ptr<Action> pAction = std::make_shared<Action>( actionString );

			if( actionSpinner1->cleanText() != ""){
				pAction->setParameter1( actionSpinner1->cleanText() );
			}
			if( actionSpinner2->cleanText() != ""){
				pAction->setParameter2( actionSpinner2->cleanText() );
			}
			if( actionSpinner3->cleanText() != ""){
				pAction->setParameter3( actionSpinner3->cleanText() );
			}

			switch ( event ) {
			case H2Core::MidiMessage::Event::CC:
				pMidiMap->registerCCEvent( eventSpinner->cleanText().toInt() , pAction );
				break;
				
			case H2Core::MidiMessage::Event::Note:
				pMidiMap->registerNoteEvent( eventSpinner->cleanText().toInt() , pAction );
				break;
				
			case H2Core::MidiMessage::Event::PC:
				pMidiMap->registerPCEvent( pAction );
				break;
				
			case H2Core::MidiMessage::Event::Null:
				// Event not recognized
				continue;
				
			default:
				// All remaining events should be different trades of
				// MMC events. If not, registerMMCEvent will handle it.
				pMidiMap->registerMMCEvent( sEventString , pAction );
			}
		}
	}
}

void MidiTable::updateRow( int nRow ) {
	LCDCombo* pEventCombo = dynamic_cast<LCDCombo*>( cellWidget( nRow, 1 ) );
	LCDCombo* pActionCombo = dynamic_cast<LCDCombo*>( cellWidget( nRow, 3 ) );

	if ( pEventCombo == nullptr || pActionCombo == nullptr ) {
		return;
	}

	if( pActionCombo->currentText().isEmpty() &&
		pEventCombo->currentText().isEmpty() && nRow != m_nRowCount - 1 ) {

		removeRow( nRow );
		m_nRowCount--;
		return;
	}

	// Adjust the event parameter spin box to fit the need of the
	// particular event.
	LCDSpinBox* pEventParameterSpinner = dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 2 ) );
	const QString sEventString = pEventCombo->currentText();
	const auto event = H2Core::MidiMessage::QStringToEvent( sEventString );

	switch ( event ) {
	case H2Core::MidiMessage::Event::CC:
		pEventParameterSpinner->show();
		pEventParameterSpinner->setMinimum( 0 );
		pEventParameterSpinner->setMaximum( 127 );
		break;
		
	case H2Core::MidiMessage::Event::Note:
		pEventParameterSpinner->show();
		pEventParameterSpinner->setMinimum( MIDI_OUT_NOTE_MIN );
		pEventParameterSpinner->setMaximum( MIDI_OUT_NOTE_MAX );
		break;
		
	case H2Core::MidiMessage::Event::PC:
	case H2Core::MidiMessage::Event::Null:
	default:
		// Includes all MMC events
		pEventParameterSpinner->hide();
	}

	QString sActionType = pActionCombo->currentText();
	LCDSpinBox* pActionSpinner1 = dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 4 ) );
	LCDSpinBox* pActionSpinner2 = dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 5 ) );
	LCDSpinBox* pActionSpinner3 = dynamic_cast<LCDSpinBox*>( cellWidget( nRow, 6 ) );
	if ( sActionType == Action::getNullActionType() || sActionType.isEmpty() ) {
		pActionSpinner1->hide();
		pActionSpinner2->hide();
		pActionSpinner3->hide();

	} else {
		int nParameterNumber = MidiActionManager::get_instance()->getParameterNumber( sActionType );
		if ( nParameterNumber != -1 ) {
			if ( nParameterNumber < 3 ) {
				pActionSpinner3->hide();
			} else {
				pActionSpinner3->show();
			}
			if ( nParameterNumber < 2 ) {
				pActionSpinner2->hide();
			} else {
				pActionSpinner2->show();
			}
			if ( nParameterNumber < 1 ) {
				pActionSpinner1->hide();
			} else {
				pActionSpinner1->show();
			}
		} else {
			ERRORLOG( QString( "Unable to find MIDI action [%1]" ).arg( sActionType ) );
		}

		// Relative changes should allow for both increasing and
		// decreasing the pattern number.
		if ( sActionType == "SELECT_NEXT_PATTERN_RELATIVE" ) {
			pActionSpinner1->setMinimum( -1 * pActionSpinner1->maximum() );
		}
	}
}
