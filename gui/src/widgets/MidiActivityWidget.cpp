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
#include "../HydrogenApp.h"
#include "MidiActivityWidget.h"
#include <hydrogen/hydrogen.h>

#include <QTimer>
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>

MidiActivityWidget::MidiActivityWidget( QWidget * parent )
 : QWidget( parent )
 , Object( "MidiActivityWidget" )
 , m_nValue( 0 )
{
	setAttribute(Qt::WA_NoBackground);

	static const uint WIDTH = 58;
	static const uint HEIGHT = 9;

	resize( WIDTH, HEIGHT );
	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	// Background image
	bool ok = m_back.load( Skin::getImagePath() + "/playerControlPanel/midiActivity_back.png" );
	if( ok == false ) {
		ERRORLOG("Error loading pixmap");
	}

	// Leds image
	ok = m_leds.load( Skin::getImagePath() + "/playerControlPanel/midiActivity_on.png" );
	if( ok == false ) {
		ERRORLOG( "Error loading pixmap" );
	}

	QTimer *timer = new QTimer(this);
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateMidiActivityWidget() ) );
	timer->start(200);	// update at 5 fps

	HydrogenApp::get_instance()->addEventListener(this);
}




/**
 * Destructor
 */
MidiActivityWidget::~MidiActivityWidget()
{
}


void MidiActivityWidget::mousePressEvent(QMouseEvent *ev)
{
	UNUSED( ev );
}



void MidiActivityWidget::setValue( int newValue )
{
	if (newValue > 100) {
		newValue = 100;
	}
	else if (newValue < 0) {
		newValue = 0;
	}

	if ( m_nValue != (uint)newValue ) {
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

	QPainter painter(this);


	if (m_nValue > 0 ) {
//		bitBlt( this, 0, 0, &m_leds, 0, 0, width(), height(), CopyROP, true);
		painter.drawPixmap( rect(), m_leds, rect() );
	}
	else {
//		bitBlt( this, 0, 0, &m_back, 0, 0, width(), height(), CopyROP, true);
		painter.drawPixmap( rect(), m_back, rect() );
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



