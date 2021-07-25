/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "../Skin.h"
#include "../HydrogenApp.h"
#include "MidiActivityWidget.h"
#include <core/Hydrogen.h>

#include <QTimer>
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>

const char* MidiActivityWidget::__class_name = "MidiActivityWidget";

MidiActivityWidget::MidiActivityWidget( QWidget * parent )
 : QWidget( parent )
 , Object( __class_name )
 , m_bValue( false )
{
	setAttribute(Qt::WA_OpaquePaintEvent);

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

	HydrogenApp::get_instance()->addEventListener(this);
	m_qTimer = new QTimer(this);
	connect( m_qTimer, SIGNAL( timeout() ), this, SLOT( restoreMidiActivityWidget() ) );

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


void MidiActivityWidget::paintEvent( QPaintEvent*)
{
	if ( !isVisible() ) {
		return;
	}

	QPainter painter(this);


	if (m_bValue ) {
//		bitBlt( this, 0, 0, &m_leds, 0, 0, width(), height(), CopyROP, true);
		painter.drawPixmap( rect(), m_leds, rect() );
	}
	else {
//		bitBlt( this, 0, 0, &m_back, 0, 0, width(), height(), CopyROP, true);
		painter.drawPixmap( rect(), m_back, rect() );
	}

}



void MidiActivityWidget::restoreMidiActivityWidget()
{
	m_bValue = false;
	update();
	m_qTimer->stop();
}



void MidiActivityWidget::midiActivityEvent()
{
	m_qTimer->stop();
	m_bValue = true;
	update();
	m_qTimer->start( 100 );
}


