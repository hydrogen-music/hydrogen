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

#include "DetailWaveDisplay.h"

#include <core/Basics/Sample.h>

#include "../Skin.h"

using namespace H2Core;

DetailWaveDisplay::DetailWaveDisplay( QWidget* pParent )
	: QWidget( pParent ),
	  m_nNormalImageDetailFrames( 180 ),
	  m_nDetailSamplePosition( 0 ),
	  m_fZoomFactor( 1 )
{
	resize( DetailWaveDisplay::nWidth, DetailWaveDisplay::nHeight );

	bool ok = m_background.load(
		Skin::getImagePath() + "/waveDisplay/detailsamplewavedisplay.png"
	);
	if ( ok == false ) {
		ERRORLOG( "Error loading pixmap" );
	}
}

DetailWaveDisplay::~DetailWaveDisplay()
{
}

void DetailWaveDisplay::setDetailSamplePosition(
	int nPosition,
	float fZoomFactor,
	const QString& sType
)
{
	m_nDetailSamplePosition = nPosition;
	m_fZoomFactor = fZoomFactor;
	m_sType = sType;
	update();
}

void DetailWaveDisplay::paintEvent( QPaintEvent* ev )
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );
	painter.drawPixmap( ev->rect(), m_background, ev->rect() );

	painter.setPen( QColor( 230, 230, 230 ) );
	const int nUpperCenter = height() / 4;
	const int nLowerCenter = height() / 4 + height() / 2;

	int nStartPosition =
		m_nDetailSamplePosition - m_nNormalImageDetailFrames / 2;

	for ( int x = 0; x < width(); x++ ) {
		if ( nStartPosition > 0 ) {
			painter.drawLine(
				x,
				( -m_peakDataL[nStartPosition - 1] * m_fZoomFactor ) +
					nUpperCenter,
				x,
				( -m_peakDataL[nStartPosition] * m_fZoomFactor ) + nUpperCenter
			);
			painter.drawLine(
				x,
				( -m_peakDataR[nStartPosition - 1] * m_fZoomFactor ) +
					nLowerCenter,
				x,
				( -m_peakDataR[nStartPosition] * m_fZoomFactor ) + nLowerCenter
			);
		}
		else {
			painter.drawLine( x, 0 + nUpperCenter, x, 0 + nUpperCenter );
			painter.drawLine( x, 0 + nLowerCenter, x, 0 + nLowerCenter );
		}
		nStartPosition++;
	}

	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 0, nUpperCenter, width(), nUpperCenter );
	painter.drawLine( 0, nLowerCenter, width(), nLowerCenter );
	QColor color;
	if ( m_sType == "Start" ) {
		color = QColor( 32, 173, 0 );
	}
	else if ( m_sType == "Loop" ) {
		color = QColor( 93, 170, 254 );
	}
	else if ( m_sType == "End" ) {
		color = QColor( 217, 68, 0 );
	}
	else {
		color = QColor( 255, 255, 255 );
	}

	painter.setPen( QPen( color, 1, Qt::SolidLine ) );
	painter.drawLine( 90, 0, 90, 265 );
}

void DetailWaveDisplay::updateDisplay( std::shared_ptr<Sample> pNewSample )
{
	if ( pNewSample == nullptr ) {
		return;
	}

	const int nSampleLength = pNewSample->getFrames();
	const int nNewLength = nSampleLength + m_nNormalImageDetailFrames / 2;

	m_peakDataL.clear();
	m_peakDataL.resize( nNewLength );
	m_peakDataR.clear();
	m_peakDataR.resize( nNewLength );

	for ( int ii = nSampleLength; ii < nNewLength; ii++ ) {
		m_peakDataL[ii] = 0;
		m_peakDataR[ii] = 0;
	}

	const float fGain = height() / 4.0 * 1.0;

	auto pSampleDataL = pNewSample->getData_L();
	auto pSampleDataR = pNewSample->getData_R();

	for ( int ii = 0; ii < nSampleLength; ii++ ) {
		m_peakDataL[ii] = static_cast<int>( pSampleDataL[ii] * fGain );
		m_peakDataR[ii] = static_cast<int>( pSampleDataR[ii] * fGain );
	}
}
