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


#include "CpuLoadWidget.h"
#include <hydrogen/hydrogen.h>

#include "../Skin.h"
#include "../HydrogenApp.h"

#include <QTimer>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>


CpuLoadWidget::CpuLoadWidget( QWidget *pParent )
 : QWidget( pParent )
 , Object( "CPULoadQWidget" )
 , m_fValue( 0 )
{
	setAttribute(Qt::WA_NoBackground);

	static const uint WIDTH = 92;
	static const uint HEIGHT = 8;

	resize( WIDTH, HEIGHT );
	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	m_nXRunValue = 0;

	// Background image
	QString background_path = Skin::getImagePath().append( "/playerControlPanel/cpuLoad_back.png" );
	bool ok = m_back.load( background_path );
	if( !ok ) {
		ERRORLOG( "Error loading pixmap " + background_path );
	}

	// Leds image
	QString leds_path = Skin::getImagePath().append( "/playerControlPanel/cpuLoad_leds.png" );
	ok = m_leds.load( leds_path );
	if( !ok ) {
		ERRORLOG( "Error loading pixmap " + leds_path );
	}

	QTimer *timer = new QTimer(this);
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateCpuLoadWidget() ) );
	timer->start(200);	// update player control at 5 fps

	HydrogenApp::get_instance()->addEventListener( this );
}



CpuLoadWidget::~CpuLoadWidget()
{
}



void CpuLoadWidget::mousePressEvent(QMouseEvent *ev)
{
	UNUSED( ev );
}



void CpuLoadWidget::setValue(float newValue)
{
	if ( newValue > 1.0 ) {
		newValue = 1.0;
	}
	else if (newValue < 0.0) {
		newValue = 0.0;
	}

	if (m_fValue != newValue) {
		m_fValue = newValue;
	}
}



float CpuLoadWidget::getValue()
{
	return m_fValue;
}



void CpuLoadWidget::paintEvent( QPaintEvent*)
{
	if (!isVisible()) {
		return;
	}

	QPainter painter(this);

	// background
//	bitBlt( &m_temp, 0, 0, &m_back, 0, 0, width(), height(), CopyROP );
	painter.drawPixmap( rect(), m_back, QRect( 0, 0, width(), height() ) );

	// leds
	int pos = (int)( 3 + m_fValue * ( width() - 3 * 2 ) );
//	bitBlt( &m_temp, 0, 0, &m_leds, 0, 0, pos, height(), CopyROP );
	painter.drawPixmap( QRect( 0, 0, pos, height() ), m_leds, QRect( 0, 0, pos, height() ) );

	if (m_nXRunValue > 0) {
		// xrun led
//		bitBlt( &m_temp, 90, 0, &m_leds, 90, 0, width(), height(), CopyROP );
		painter.drawPixmap( QRect( 90, 0, width(), height() ), m_leds, QRect( 90, 0, width(), height() ) );
	}
}



void CpuLoadWidget::updateCpuLoadWidget()
{
	// Process time
	H2Core::Hydrogen *engine = H2Core::Hydrogen::get_instance();
	int perc = 0;
	if ( engine->getMaxProcessTime() != 0.0 ) {
		perc = (int)( engine->getProcessTime() / ( engine->getMaxProcessTime() / 100.0 ) );
	}
	setValue( perc / 100.0 );

	if (m_nXRunValue > 0) {
		m_nXRunValue -= 5;
	}

	update();
}



void CpuLoadWidget::XRunEvent()
{
	INFOLOG( "[xRunEvent]" );
	m_nXRunValue = 100;
	update();
}



