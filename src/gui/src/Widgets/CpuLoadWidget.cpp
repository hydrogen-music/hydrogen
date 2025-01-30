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

#include "CpuLoadWidget.h"

#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>

#include "../HydrogenApp.h"

CpuLoadWidget::CpuLoadWidget( QWidget *pParent )
 : QWidget( pParent )
 , m_fValue( 0 )
 , m_nXRunValue( 0 )
 , m_size( QSize( 96, 10 ) )
{
	setAttribute(Qt::WA_OpaquePaintEvent);

	adjustSize();
	setFixedSize( m_size.width(), m_size.height() );

	m_recentValues.resize( 5 );
	for ( auto& ii : m_recentValues ) {
		ii = 0;
	}

	QTimer* pTimer = new QTimer(this);
	connect( pTimer, SIGNAL( timeout() ), this, SLOT( updateCpuLoadWidget() ) );
	pTimer->start( 100 );	// update player control at 10 fps

	HydrogenApp::get_instance()->addEventListener( this );

	resize( m_size.width(), m_size.height() );
}



CpuLoadWidget::~CpuLoadWidget(){
}

void CpuLoadWidget::paintEvent( QPaintEvent*)
{
	if ( !isVisible() ) {
		return;
	}

	QPainter painter(this);

	float fSum = 0;
	for ( auto ii : m_recentValues ) {
		fSum += ii;
	}
	float fPeak = static_cast<float>( m_size.width() ) * fSum / static_cast<float>( m_recentValues.size() );
	float fBorderWidth = 2;

	QColor colorGradientGreen( Qt::green );
	QColor colorGradientLightGreen( 175, 255, 0 );
	QColor colorGradientYellow( Qt::yellow );
	QColor colorGradientOrange( 255, 125, 0 );
	QColor colorGradientRed( Qt::red );
	QColor colorBorder( QColor( 0, 0, 0 ) );
		
	QLinearGradient gradient = QLinearGradient( 0, 0, m_size.width(), m_size.height() );	
	gradient.setColorAt( 0.0, colorGradientGreen );
	gradient.setColorAt( 0.5, colorGradientLightGreen );
	gradient.setColorAt( 0.7, colorGradientYellow );
	gradient.setColorAt( 0.8, colorGradientOrange );
	gradient.setColorAt( 0.92, colorGradientRed );

	painter.fillRect( QRect( 0, 0, m_size.width(), m_size.height() ),
					  H2Core::Preferences::get_instance()->getColorTheme()->m_midLightColor );
	painter.fillRect( QRectF( fBorderWidth / 2, fBorderWidth / 2, fPeak, m_size.height() - fBorderWidth ), QBrush( gradient ) );
		
	QPen pen;
	if ( m_nXRunValue > 0 ) {
		pen.setColor( colorGradientRed );
	} else {
		pen.setColor( colorBorder );
	}
	pen.setWidth( fBorderWidth );
	painter.setPen( pen );

	// Border
	painter.drawRoundedRect( QRect( fBorderWidth / 2, fBorderWidth / 2, m_size.width() - fBorderWidth,
									m_size.height() - fBorderWidth ), 1, 1 );

	// Grid lines
	float fDistance = 5;
		
	pen.setWidth( 1 );
	painter.setPen( pen );
	float fXX = fDistance;
	while ( fXX < m_size.width() - fBorderWidth ) {
		painter.drawLine( fXX, fBorderWidth, fXX, m_size.height() - fBorderWidth );
		fXX += fDistance;
	}
}

void CpuLoadWidget::updateCpuLoadWidget()
{
	// Process time
	H2Core::AudioEngine *pAudioEngine = H2Core::Hydrogen::get_instance()->getAudioEngine();
	float fPercentage = 0;
	if ( pAudioEngine->getMaxProcessTime() != 0.0 ) {
		fPercentage = ( pAudioEngine->getProcessTime() / pAudioEngine->getMaxProcessTime() );
	}

	if ( fPercentage > 1.0 ) {
		fPercentage = 1.0;
	} else if ( fPercentage < 0.0 ) {
		fPercentage = 0.0;
	}

	for ( int ii = ( m_recentValues.size() - 1 ) ; ii > 0; ii-- ) {
		m_recentValues[ ii ] = m_recentValues[ ii - 1 ];
	}
	m_recentValues[ 0 ] = fPercentage;

	if ( m_nXRunValue > 0 ){
		m_nXRunValue--;
	}

	update();
}



void CpuLoadWidget::XRunEvent()
{
	m_nXRunValue = 31;

	update();
}



