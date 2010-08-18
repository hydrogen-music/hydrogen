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

#include "hydrogen/midiMap.h"
#include "MidiSenseWidget.h"
#include <hydrogen/hydrogen.h>


MidiSenseWidget::MidiSenseWidget(QWidget* pParent, bool directWr  , Action* midiAction ): QDialog( pParent ) , Object("MidiSenseWidget")
{
	directWrite = directWr;
	action = midiAction;


	setWindowTitle( "Waiting.." );
	setFixedSize( 280, 100 );
	
	m_pURLLabel = new QLabel( this );
	m_pURLLabel->setAlignment( Qt::AlignCenter );

	if(action != NULL){
	    m_pURLLabel->setText( "Waiting for midi input..." );
	} else{

            /*
             *   Check if this widget got called from the midiTable in the preferences
             *   window(directWrite=false) or by clicking on a midiLearn-capable gui item(directWrite=true)
             */

            if(directWrite){
                m_pURLLabel->setText( "This element is not midi operable." );
            } else {
                m_pURLLabel->setText( "Waiting for midi input..." );
            }
	}
	
	QVBoxLayout* pVBox = new QVBoxLayout( this );
	pVBox->addWidget( m_pURLLabel );
	setLayout( pVBox );
	
	H2Core::Hydrogen *pEngine = H2Core::Hydrogen::get_instance();
	pEngine->lastMidiEvent = "";
	pEngine->lastMidiEventParameter = 0;
	
	m_pUpdateTimer = new QTimer( this );
	connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateMidi() ) );

	m_pUpdateTimer->start( 100 );


};

MidiSenseWidget::~MidiSenseWidget(){
	INFOLOG("DESTROY");
	m_pUpdateTimer->stop();
}

void MidiSenseWidget::updateMidi(){
	H2Core::Hydrogen *pEngine = H2Core::Hydrogen::get_instance();
	if(	!pEngine->lastMidiEvent.isEmpty() ){
		lastMidiEvent = pEngine->lastMidiEvent;
		lastMidiEventParameter = pEngine->lastMidiEventParameter;


		if( directWrite ){
		    //write the action / parameter combination to the midiMap
		    MidiMap *mM = MidiMap::get_instance();
		    assert(action);
		    Action* pAction = new Action( action->getType() );

		    //if( action->getParameter1() != 0){
		    pAction->setParameter1( action->getParameter1() );
		    //}

		    if( lastMidiEvent.left(2) == "CC" ){
			    mM->registerCCEvent( lastMidiEventParameter , pAction );
		    }

		    if( lastMidiEvent.left(3) == "MMC" ){
			    mM->registerMMCEvent( lastMidiEvent , pAction );
		    }

		    if( lastMidiEvent.left(4) == "NOTE" ){
			    mM->registerNoteEvent( lastMidiEvent.toInt() , pAction );
		    }
		}

		close();
	}

}

