/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "../Skin.h"
#include "MidiSenseWidget.h"
#include "MidiTable.h"

#include <hydrogen/midi_map.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/globals.h>
#include <hydrogen/midi_action.h>
#include <hydrogen/hydrogen.h>

#include <QHeaderView>

const char* MidiTable::__class_name = "MidiTable";

MidiTable::MidiTable( QWidget *pParent )
 : QTableWidget( pParent )
 , Object( __class_name )
{
	__row_count = 0;
	setupMidiTable();

	m_pUpdateTimer = new QTimer( this );
	currentMidiAutosenseRow = 0;
}


MidiTable::~MidiTable()
{
	for( int myRow = 0; myRow <=  __row_count ; myRow++ ) {
		delete cellWidget( myRow, 0 );
		delete cellWidget( myRow, 1 );
		delete cellWidget( myRow, 2 );
		delete cellWidget( myRow, 3 );
		delete cellWidget( myRow, 4 );
		delete cellWidget( myRow, 5 );
		delete cellWidget( myRow, 6 );
	}
}

void MidiTable::midiSensePressed( int row ){

	currentMidiAutosenseRow = row;
	MidiSenseWidget midiSenseWidget( this );
	midiSenseWidget.exec();

	QComboBox * eventCombo =  dynamic_cast <QComboBox *> ( cellWidget( row, 1 ) );
	QSpinBox * eventSpinner = dynamic_cast <QSpinBox *> ( cellWidget( row, 2 ) );


	eventCombo->setCurrentIndex( eventCombo->findText( midiSenseWidget.m_sLastMidiEvent ) );
	eventSpinner->setValue( midiSenseWidget.m_LastMidiEventParameter );

	m_pUpdateTimer->start( 100 );	
}


void MidiTable::updateTable()
{
	if( __row_count > 0 ) {
		QComboBox * eventCombo =  dynamic_cast <QComboBox *> ( cellWidget( __row_count - 1, 1 ) );
		QComboBox * actionCombo = dynamic_cast <QComboBox *> ( cellWidget( __row_count - 1, 3 ) );

		if( eventCombo == NULL || actionCombo == NULL) return;

		if( actionCombo->currentText() != "" && eventCombo->currentText() != "" ) {
			insertNewRow("", "", 0, 0, 0, 0);
		}
	}
}


void MidiTable::insertNewRow(QString actionString , QString eventString, int eventParameter , int actionParameter1 , int actionParameter2 , int actionParameter3)
{
	MidiActionManager *pActionHandler = MidiActionManager::get_instance();

	insertRow( __row_count );
	
	int oldRowCount = __row_count;

	++__row_count;

	

	QPushButton *midiSenseButton = new QPushButton(this);
	midiSenseButton->setIcon(QIcon(Skin::getImagePath() + "/preferencesDialog/rec.png"));
	midiSenseButton->setToolTip( trUtf8("press button to record midi event") );

	QSignalMapper *signalMapper = new QSignalMapper(this);

	connect(midiSenseButton, SIGNAL( clicked()), signalMapper, SLOT( map() ));
	signalMapper->setMapping( midiSenseButton, oldRowCount );
	connect( signalMapper, SIGNAL(mapped( int ) ), this, SLOT( midiSensePressed(int) ) );
	setCellWidget( oldRowCount, 0, midiSenseButton );



	QComboBox *eventBox = new QComboBox();
	connect( eventBox , SIGNAL( currentIndexChanged( int ) ) , this , SLOT( updateTable() ) );
	eventBox->insertItems( oldRowCount , pActionHandler->getEventList() );
	eventBox->setCurrentIndex( eventBox->findText(eventString) );
	setCellWidget( oldRowCount, 1, eventBox );
	
	
	QSpinBox *eventParameterSpinner = new QSpinBox();
	setCellWidget( oldRowCount , 2, eventParameterSpinner );
	eventParameterSpinner->setMaximum( 999 );
	eventParameterSpinner->setValue( eventParameter );


	QComboBox *actionBox = new QComboBox();
	connect( actionBox , SIGNAL( currentIndexChanged( int ) ) , this , SLOT( updateTable() ) );
	actionBox->insertItems( oldRowCount, pActionHandler->getActionList());
	actionBox->setCurrentIndex ( actionBox->findText( actionString ) );
	setCellWidget( oldRowCount , 3, actionBox );
	

	QSpinBox *actionParameterSpinner1 = new QSpinBox();
	setCellWidget( oldRowCount , 4, actionParameterSpinner1 );
	actionParameterSpinner1->setValue( actionParameter1);
	actionParameterSpinner1->setMaximum( 999 );

	QSpinBox *actionParameterSpinner2 = new QSpinBox();
	setCellWidget( oldRowCount , 5, actionParameterSpinner2 );
	actionParameterSpinner2->setValue( actionParameter2);
	actionParameterSpinner2->setMaximum( std::max(MAX_FX, MAX_COMPONENTS) );

	QSpinBox *actionParameterSpinner3 = new QSpinBox();
	setCellWidget( oldRowCount , 6, actionParameterSpinner3 );
	actionParameterSpinner3->setValue( actionParameter3);
	actionParameterSpinner3->setMaximum( MAX_LAYERS );
}

void MidiTable::setupMidiTable()
{
	MidiMap *pMidiMap = MidiMap::get_instance();

	QStringList items;
	items << "" << trUtf8("Event")  <<  trUtf8("Param.")
	            << trUtf8("Action") <<  trUtf8("Param.")  <<  trUtf8("Param.")  <<  trUtf8("Param.");

	setRowCount( 0 );
	setColumnCount( 7 );

	verticalHeader()->hide();

	setHorizontalHeaderLabels( items );
	horizontalHeader()->setStretchLastSection(true);

	setColumnWidth( 0 , 25 );
	setColumnWidth( 1 , 155 );
	setColumnWidth( 2, 73 );
	setColumnWidth( 3, 175 );
	setColumnWidth( 4 , 73 );
	setColumnWidth( 5 , 73 );
	setColumnWidth( 6 , 73 );

	bool ok;
	std::map< QString , MidiAction* > mmcMap = pMidiMap->getMMCMap();
	std::map< QString , MidiAction* >::iterator dIter( mmcMap.begin() );
	
	for( dIter = mmcMap.begin(); dIter != mmcMap.end(); dIter++ ) {
		MidiAction * pAction = dIter->second;
		int actionParameterInteger1 = pAction->getParameter1().toInt(&ok,10);
		int actionParameterInteger2 = pAction->getParameter2().toInt(&ok,10);
		int actionParameterInteger3 = pAction->getParameter3().toInt(&ok,10);
		
		insertNewRow(pAction->getType() , dIter->first , 0 , actionParameterInteger1 , actionParameterInteger2 , actionParameterInteger3 );
	}

	for( int note = 0; note < 128; note++ ) {
		MidiAction * pAction = pMidiMap->getNoteAction( note );
		int actionParameterInteger1 = pAction->getParameter1().toInt(&ok,10);
		int actionParameterInteger2 = pAction->getParameter2().toInt(&ok,10);
		int actionParameterInteger3 = pAction->getParameter3().toInt(&ok,10);

		if ( pAction->getType() == "NOTHING" ){
			continue;
		}

		insertNewRow(pAction->getType() , "NOTE" , note , actionParameterInteger1 , actionParameterInteger2 , actionParameterInteger3 );
	}

	for( int parameter = 0; parameter < 128; parameter++ ){
		MidiAction * pAction = pMidiMap->getCCAction( parameter );
		int actionParameterInteger1 = pAction->getParameter1().toInt(&ok,10);
		int actionParameterInteger2 = pAction->getParameter2().toInt(&ok,10);
		int actionParameterInteger3 = pAction->getParameter3().toInt(&ok,10);

		if ( pAction->getType() == "NOTHING" ){
			continue;
		}

		insertNewRow(pAction->getType() , "CC" , parameter , actionParameterInteger1 , actionParameterInteger2 , actionParameterInteger3 );
	}

	{
		MidiAction * pAction = pMidiMap->getPCAction();
		if ( pAction->getType() != "NOTHING" ) {
			int actionParameterInteger1 = pAction->getParameter1().toInt(&ok,10);
			int actionParameterInteger2 = pAction->getParameter2().toInt(&ok,10);
			int actionParameterInteger3 = pAction->getParameter3().toInt(&ok,10);

			insertNewRow( pAction->getType() , "PROGRAM_CHANGE" , 0 , actionParameterInteger1 , actionParameterInteger2 , actionParameterInteger3 );
		}
	}
	
	insertNewRow( "", "", 0, 0, 0, 0 );
}


void MidiTable::saveMidiTable()
{
	MidiMap *mM = MidiMap::get_instance();
	
	for ( int row = 0; row < __row_count; row++ ) {

		QComboBox * eventCombo =  dynamic_cast <QComboBox *> ( cellWidget( row, 1 ) );
		QSpinBox * eventSpinner = dynamic_cast <QSpinBox *> ( cellWidget( row, 2 ) );
		QComboBox * actionCombo = dynamic_cast <QComboBox *> ( cellWidget( row, 3 ) );
		QSpinBox * actionSpinner1 = dynamic_cast <QSpinBox *> ( cellWidget( row, 4 ) );
		QSpinBox * actionSpinner2 = dynamic_cast <QSpinBox *> ( cellWidget( row, 5 ) );
		QSpinBox * actionSpinner3 = dynamic_cast <QSpinBox *> ( cellWidget( row, 6 ) );

		QString eventString;
		QString actionString;

		if( !eventCombo->currentText().isEmpty() && !actionCombo->currentText().isEmpty() ){
			eventString = eventCombo->currentText();

			actionString = actionCombo->currentText();
		
			MidiAction* pAction = new MidiAction( actionString );

			if( actionSpinner1->cleanText() != ""){
				pAction->setParameter1( actionSpinner1->cleanText() );
			}
			if( actionSpinner2->cleanText() != ""){
				pAction->setParameter2( actionSpinner2->cleanText() );
			}
			if( actionSpinner3->cleanText() != ""){
				pAction->setParameter3( actionSpinner3->cleanText() );
			}

			if( eventString.left(2) == "CC" ){
				mM->registerCCEvent( eventSpinner->cleanText().toInt() , pAction );
			} else if( eventString.left(3) == "MMC" ){
				mM->registerMMCEvent( eventString , pAction );
			} else if( eventString.left(4) == "NOTE" ){
				mM->registerNoteEvent( eventSpinner->cleanText().toInt() , pAction );
			} else if( eventString.left(14) == "PROGRAM_CHANGE" ){
				mM->registerPCEvent( pAction );
			}
		}
	}
}
