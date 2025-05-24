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

#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Basics/Instrument.h>
using namespace H2Core;

#include "SampleWaveDisplay.h"
#include "../Skin.h"

SampleWaveDisplay::SampleWaveDisplay(QWidget* pParent)
 : QWidget( pParent )
 , m_sSampleName( "" )
{
//	setAttribute(Qt::WA_OpaquePaintEvent);

	//
	int w = 445;
	int h = 85;
	resize( w, h );

	bool ok = m_Background.load( Skin::getImagePath() + "/waveDisplay/bgsamplewavedisplay.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	m_pPeakData = new int[ w ];

}




SampleWaveDisplay::~SampleWaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakData;
}



void SampleWaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );
	painter.drawPixmap( ev->rect(), m_Background, ev->rect() );

	painter.setPen( QColor( 102, 150, 205 ) );
	int VCenter = height() / 2;
	for ( int x = 0; x < width(); x++ ) {
		painter.drawLine( x, VCenter, x, m_pPeakData[x] + VCenter );
		painter.drawLine( x, VCenter, x, -m_pPeakData[x] + VCenter );
	}

	QFont font;
	font.setWeight( QFont::Bold );
	painter.setFont( font );
	painter.setPen( QColor( 255 , 255, 255, 200 ) );
	painter.drawText( 0, 0, width(), 20, Qt::AlignCenter, m_sSampleName );
}



void SampleWaveDisplay::updateDisplay( QString filename )
{

	auto pNewSample = Sample::load( filename );

	if ( pNewSample != nullptr ) {
		// Extract the filename from the complete path
		QString sName = filename;
		int nPos = sName.lastIndexOf( "/" );

		if ( sName.endsWith("emptySample.wav")){
			m_sSampleName = "";
		}else
		{
			m_sSampleName = sName.mid( nPos + 1, sName.length() );
		}

//		INFOLOG( "[updateDisplay] sample: " + m_sSampleName  );

		int nSampleLength = pNewSample->get_frames();
		float nScaleFactor = nSampleLength / width();

		float fGain = height() / 2.0 * 1.0;

		auto pSampleData = pNewSample->get_data_l();

		int nSamplePos =0;
		int nVal;
		for ( int i = 0; i < width(); ++i ){
			nVal = 0;
			for ( int j = 0; j < nScaleFactor; ++j ) {
				if ( j < nSampleLength ) {
					int newVal = static_cast<int>( pSampleData[ nSamplePos ] * fGain );
					if ( newVal > nVal ) {
						nVal = newVal;
					}
				}
				++nSamplePos;
			}
			m_pPeakData[ i ] = nVal;
		}
	}

	update();

}

