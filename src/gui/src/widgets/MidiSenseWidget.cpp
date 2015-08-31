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

#include "hydrogen/midi_map.h"
#include "MidiSenseWidget.h"
#include <hydrogen/hydrogen.h>

const char* MidiSenseWidget::__class_name = "MidiSenseWidget";

MidiSenseWidget::MidiSenseWidget(QWidget* pParent, bool directWr, MidiAction* midiAction): QDialog( pParent ) , Object(__class_name)
{
	m_DirectWrite = directWr;
	m_pAction = midiAction;

	setWindowTitle( "Waiting.." );
	setFixedSize( 280, 100 );

	bool midiOperable = false;
	
	m_pURLLabel = new QLabel( this );
	m_pURLLabel->setAlignment( Qt::AlignCenter );

	if(m_pAction != NULL){
		m_pURLLabel->setText( "Waiting for midi input..." );
		midiOperable = true;
	} else {

		/*
		 *   Check if this widget got called from the midiTable in the preferences
		 *   window(directWrite=false) or by clicking on a midiLearn-capable gui item(directWrite=true)
		 */

		if(m_DirectWrite){
			m_pURLLabel->setText( trUtf8("This element is not midi operable.") );
			midiOperable = false;
		} else {
			m_pURLLabel->setText( trUtf8("Waiting for midi input...") );
			midiOperable = true;
		}
	}
	
	QVBoxLayout* pVBox = new QVBoxLayout( this );
	pVBox->addWidget( m_pURLLabel );
	setLayout( pVBox );
	
	H2Core::Hydrogen *pEngine = H2Core::Hydrogen::get_instance();
	pEngine->lastMidiEvent = "";
	pEngine->lastMidiEventParameter = 0;

	m_LastMidiEventParameter = 0;
	
	m_pUpdateTimer = new QTimer( this );

	if(midiOperable)
	{
		/*
		 * If the widget is not midi operable, we can omit
		 * starting the timer which listens to midi input..
		 */

		connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateMidi() ) );
		m_pUpdateTimer->start( 100 );
	}
};

MidiSenseWidget::~MidiSenseWidget(){
	INFOLOG("DESTROY");
	m_pUpdateTimer->stop();
}

void MidiSenseWidget::updateMidi(){
	H2Core::Hydrogen *pEngine = H2Core::Hydrogen::get_instance();
	if(	!pEngine->lastMidiEvent.isEmpty() ){
		m_sLastMidiEvent = pEngine->lastMidiEvent;
		m_LastMidiEventParameter = pEngine->lastMidiEventParameter;


		if( m_DirectWrite ){
			//write the action / parameter combination to the midiMap
			MidiMap *pMidiMap = MidiMap::get_instance();

			assert(m_pAction);

			MidiAction* pAction = new MidiAction( m_pAction->getType() );

			pAction->setParameter1( m_pAction->getParameter1() );

			if( m_sLastMidiEvent.left(2) == "CC" ){
				pMidiMap->registerCCEvent( m_LastMidiEventParameter , pAction );
			} else if( m_sLastMidiEvent.left(3) == "MMC" ){
				pMidiMap->registerMMCEvent( m_sLastMidiEvent , pAction );
			} else if( m_sLastMidiEvent.left(4) == "NOTE" ){
				pMidiMap->registerNoteEvent( m_LastMidiEventParameter , pAction );
			} else if (m_sLastMidiEvent.left(14) == "PROGRAM_CHANGE" ){
				pMidiMap->registerPCEvent( pAction );
			} else {
				/* In all other cases, the midiMap cares for deleting the pointer */

				delete pAction;
			}
		}

		close();
	}

}

