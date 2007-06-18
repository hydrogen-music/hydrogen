/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: MidiActivityWidget.cpp,v 1.15 2005/05/30 21:23:48 comix Exp $
 *
 */

#include "../Skin.h"
#include "MidiActivityWidget.h"
#include "gui/HydrogenApp.h"
#include "lib/Hydrogen.h"

#include <qtimer.h>
#include <qpainter.h>

MidiActivityWidget::MidiActivityWidget( QWidget * parent )
 : QWidget( parent , "MidiActivityWidget", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "MidiActivityWidget" )
 , m_nValue( 0 )
{
	static const uint WIDTH = 58;
	static const uint HEIGHT = 9;

	resize( WIDTH, HEIGHT );
	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	// Create temp image
	m_temp.resize( width(), height() );

	// Background image
	string background_path = Skin::getImagePath() + string( "/playerControlPanel/midiActivity_back.png" );
	bool ok = m_back.load( background_path.c_str() );
	if( ok == false ) {
		errorLog("Error loading pixmap");
	}

	// Leds image
	string leds_path = Skin::getImagePath() + string( "/playerControlPanel/midiActivity_on.png" );
	ok = m_leds.load( leds_path.c_str() );
	if( ok == false ) {
		errorLog( "Error loading pixmap" );
	}

	QTimer *timer = new QTimer(this);
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateMidiActivityWidget() ) );
	timer->start(200);	// update at 5 fps

	HydrogenApp::getInstance()->addEventListener(this);
}




/**
 * Destructor
 */
MidiActivityWidget::~MidiActivityWidget()
{
}


void MidiActivityWidget::mousePressEvent(QMouseEvent *ev)
{
}



void MidiActivityWidget::setValue(uint newValue)
{
	if (newValue > 100) {
		newValue = 100;
	}
	else if (newValue < 0) {
		newValue = 0;
	}

	if (m_nValue != newValue) {
		m_nValue = newValue;
		update();
	}
}




uint MidiActivityWidget::getValue()
{
	return m_nValue;
}



void MidiActivityWidget::paintEvent( QPaintEvent*)
{
	if ( !isVisible() ) {
		return;
	}

	if (m_nValue > 0 ) {
		bitBlt( this, 0, 0, &m_leds, 0, 0, width(), height(), CopyROP, true);
	}
	else {
		bitBlt( this, 0, 0, &m_back, 0, 0, width(), height(), CopyROP, true);
	}
}



void MidiActivityWidget::updateMidiActivityWidget()
{
	int newValue = m_nValue - 40;
	if (newValue < 0 ) {
		newValue = 0;
	}
	setValue( newValue );
}



void MidiActivityWidget::midiActivityEvent()
{
	setValue( 100 );
	updateMidiActivityWidget();
}



