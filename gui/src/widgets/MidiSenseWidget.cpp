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

#include "MidiSenseWidget.h"
#include <hydrogen/hydrogen.h>


MidiSenseWidget::MidiSenseWidget(QWidget* pParent) : QDialog( pParent ) , Object("MidiSenseWidget")
{
	setWindowTitle( "Waiting.." );
	setFixedSize( 200, 100 );	
	
	m_pURLLabel = new QLabel( this );
	m_pURLLabel->setAlignment( Qt::AlignCenter );
	m_pURLLabel->setText( "Waiting for midi input..." );
	
	
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
	if(	pEngine->lastMidiEvent != ""){
		lastMidiEvent = pEngine->lastMidiEvent;
		lastMidiEventParameter = pEngine->lastMidiEventParameter;
		close();
	}

}

