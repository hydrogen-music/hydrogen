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

#include "midiTable.h"

#include <hydrogen/midiMap.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/globals.h>
#include <hydrogen/action.h>
#include <hydrogen/midiMap.h>


midiTable::midiTable( QWidget *pParent )
 : QTableWidget( pParent ) , Object("midiTable")
{
	rowCount = 0;
	setupMidiTable();
}



midiTable::~midiTable()
{
	
	int myRow = 0;
	
	for( myRow = 0; myRow <=  rowCount ; myRow++)
	{
		delete cellWidget(myRow,0);
		delete cellWidget(myRow,1);
		delete cellWidget(myRow,2);
		delete cellWidget(myRow,3);
	}
}


void midiTable::updateTable(){
	
	
	if( rowCount > 0 ) 
	{
		QComboBox * eventCombo =  dynamic_cast <QComboBox *> ( cellWidget( rowCount-1, 0 ) );
		QComboBox * actionCombo = dynamic_cast <QComboBox *> ( cellWidget( rowCount-1, 2 ) );

		if( eventCombo == NULL or actionCombo == NULL) return;

		if( actionCombo->currentText() != "" and eventCombo->currentText() != "" ){
			insertNewRow("", "", 0, 0);
		}
	}
}

void midiTable::insertNewRow(QString actionString , QString eventString, int eventParameter , int actionParameter)
{
	actionManager *aH = actionManager::getInstance();

	insertRow( rowCount );
	
	int oldRowCount = rowCount;

	rowCount++;

	QComboBox *eventBox = new QComboBox();
	connect( eventBox , SIGNAL( currentIndexChanged( int ) ) , this , SLOT( updateTable() ) );
	eventBox->insertItems( oldRowCount , aH->getEventList() );
	eventBox->setCurrentIndex( eventBox->findText(eventString) );
	setCellWidget( oldRowCount, 0, eventBox );
	
	
	QSpinBox *eventParameterSpinner = new QSpinBox();
	setCellWidget( oldRowCount , 1, eventParameterSpinner );
	eventParameterSpinner->setValue( eventParameter );


	QComboBox *actionBox = new QComboBox();
	connect( actionBox , SIGNAL( currentIndexChanged( int ) ) , this , SLOT( updateTable() ) );
	actionBox->insertItems( oldRowCount, aH->getActionList());
	actionBox->setCurrentIndex ( actionBox->findText( actionString ) );
	setCellWidget( oldRowCount , 2, actionBox );
	

	QSpinBox *actionParameterSpinner = new QSpinBox();
	
	setCellWidget( oldRowCount , 3, actionParameterSpinner );
	actionParameterSpinner->setValue( actionParameter);


}

void midiTable::setupMidiTable()
{
	midiMap *mM = midiMap::getInstance();

	QStringList items;
	items <<  trUtf8("Event")  <<  trUtf8("Param.")  <<  trUtf8("Action")
           <<  trUtf8("Param.") ;

	setRowCount( 0 );
    	setColumnCount( 4 );

	verticalHeader()->hide();

	setHorizontalHeaderLabels( items );
	
	
	setFixedWidth( 500 );
	setColumnWidth( 0 , 175 );
	setColumnWidth( 1, 73 );
	setColumnWidth( 2, 175 );
	setColumnWidth( 3 , 73 );


	bool ok;
	std::map< QString , action *> mmcMap = mM->getMMCMap();
	std::map< QString , action *>::iterator dIter(mmcMap.begin());

	
	for( dIter = mmcMap.begin(); dIter != mmcMap.end(); dIter++ )
	{
		action * pAction = dIter->second;
		QString actionParameter;
		int actionParameterInteger = 0;

		if( pAction->getParameterList().size() != 0 ){
			actionParameter = pAction->getParameterList().at(0);
			actionParameterInteger = actionParameter.toInt(&ok,10);
		}
		
		insertNewRow(pAction->getType() , dIter->first , 0 , actionParameterInteger );
	}

	for( int note = 0; note < 128; note++ )
	{
		action * pAction = mM->getNoteAction( note );
		QString actionParameter;
		int actionParameterInteger = 0;

		if( pAction->getParameterList().size() != 0 ){
			actionParameter = pAction->getParameterList().at(0);
			actionParameterInteger = actionParameter.toInt(&ok,10);
		}

		if ( pAction->getType() == "NOTHING" ) continue;

		insertNewRow(pAction->getType() , "NOTE" , note , actionParameterInteger );
	}
	
	insertNewRow("","",0,0);

}


void midiTable::saveMidiTable(){
	
	delete	midiMap::getInstance();
	midiMap *mM  = midiMap::getInstance();
	
	int row = 0;
	
	for( row = 0; row <  rowCount; row++ ){

		QComboBox * eventCombo =  dynamic_cast <QComboBox *> ( cellWidget( row, 0 ) );
	
		QSpinBox * eventSpinner = dynamic_cast <QSpinBox *> ( cellWidget( row, 1 ) );

		QComboBox * actionCombo = dynamic_cast <QComboBox *> ( cellWidget( row, 2 ) );

		QSpinBox * actionSpinner = dynamic_cast <QSpinBox *> ( cellWidget( row, 3 ) );

		

		QString eventString;
		QString actionString;
		

		if( eventCombo->currentText() != "" && actionCombo->currentText() != "" ){
			eventString = eventCombo->currentText();

			actionString = actionCombo->currentText();
		
			action * pAction = new action( actionString );



			if( actionSpinner->cleanText() != ""){
				pAction->addParameter( actionSpinner->cleanText() );
			}
	
			if( eventString.left(3) == "MMC" ){
				mM->registerMMCEvent( eventString , pAction );
			}
			
			if( eventString.left(4) == "NOTE" ){
				mM->registerNoteEvent( eventSpinner->cleanText().toInt() , pAction );
			}
		}
	}
}
