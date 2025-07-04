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

#include "MidiSenseWidget.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"

#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiMap.h>
#include <core/Preferences/Preferences.h>

MidiSenseWidget::MidiSenseWidget(QWidget* pParent, bool bDirectWrite, std::shared_ptr<Action> pAction)
	: QDialog( pParent )
	, m_lastMidiEvent( H2Core::MidiMessage::Event::Null )
	, m_nLastMidiEventParameter( 0 )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	m_bDirectWrite = bDirectWrite;
	m_pAction = pAction;

	setWindowTitle( pCommonStrings->getMidiSenseWindowTitle() );
	setFixedSize( 280, 100 );

	bool midiOperable = false;
	
	m_pURLLabel = new QLabel( this );
	m_pURLLabel->setAlignment( Qt::AlignCenter );

	if(m_pAction != nullptr){
		m_pURLLabel->setText( pCommonStrings->getMidiSenseInput() );
		midiOperable = true;
	} else {

		/*
		 *   Check if this widget got called from the midiTable in the preferences
		 *   window(directWrite=false) or by clicking on a midiLearn-capable gui item(directWrite=true)
		 */

		if(m_bDirectWrite){
			m_pURLLabel->setText( pCommonStrings->getMidiSenseUnavailable() );
			midiOperable = false;
		} else {
			m_pURLLabel->setText( pCommonStrings->getMidiSenseInput() );
			midiOperable = true;
		}
	}
	
	QVBoxLayout* pVBox = new QVBoxLayout( this );
	pVBox->addWidget( m_pURLLabel );
	setLayout( pVBox );
	
	H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::get_instance();
	pHydrogen->setLastMidiEvent( H2Core::MidiMessage::Event::Null );
	pHydrogen->setLastMidiEventParameter( 0 );
	
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
	m_pUpdateTimer->stop();
}

void MidiSenseWidget::updateMidi(){
	H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getLastMidiEvent() != H2Core::MidiMessage::Event::Null ){

		m_lastMidiEvent = pHydrogen->getLastMidiEvent();
		m_nLastMidiEventParameter = pHydrogen->getLastMidiEventParameter();

		if ( m_bDirectWrite ) {
			// write the action / parameter combination to the midiMap
			auto pMidiMap = H2Core::Preferences::get_instance()->getMidiMap();

			assert(m_pAction);

			auto pAction = std::make_shared<Action>( m_pAction );
			pAction->setValue( "0" );

			switch( m_lastMidiEvent ) {
			case H2Core::MidiMessage::Event::CC: 
				pMidiMap->registerCCEvent( m_nLastMidiEventParameter, pAction );
				break;

			case H2Core::MidiMessage::Event::Note:
				pMidiMap->registerNoteEvent( m_nLastMidiEventParameter, pAction );
				break;

			case H2Core::MidiMessage::Event::PC:
				pMidiMap->registerPCEvent( pAction );
				break;

			case H2Core::MidiMessage::Event::Null:
				return;

			default:
				// MMC event
				pMidiMap->registerMMCEvent(
					H2Core::MidiMessage::EventToQString( m_lastMidiEvent ),
					pAction );
			}

			H2Core::EventQueue::get_instance()->pushEvent( H2Core::Event::Type::MidiMapChanged, 0 );
		}

		close();
	}
}

