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
#include "config.h"

#include <hydrogen/sample.h>
#include <hydrogen/Song.h>
#include <hydrogen/instrument.h>
using namespace H2Core;

#include "TargetWaveDisplay.h"
#include "../Skin.h"


TargetWaveDisplay::TargetWaveDisplay(QWidget* pParent)
 : QWidget( pParent )
 , Object( "TargetWaveDisplay" )
 , m_sSampleName( "" )
{
	setAttribute(Qt::WA_NoBackground);

	//INFOLOG( "INIT" );
	int w = 451;
	int h = 91;
	resize( w, h );

	bool ok = m_background.load( Skin::getImagePath() + "/waveDisplay/targetsamplewavedisplay.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	m_pPeakData = new int[ w ];

}




TargetWaveDisplay::~TargetWaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakData;
}



void TargetWaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );
	painter.drawPixmap( ev->rect(), m_background, ev->rect() );

	painter.setPen( QColor( 102, 150, 205 ) );
	int VCenter = height() / 2;
	for ( int x = 0; x < width(); x++ ) {
		painter.drawLine( x, -m_pPeakData[x] +VCenter, x, -m_pPeakData[x +1] +VCenter  );
	}

	QFont font;
	font.setWeight( 63 );
	painter.setFont( font );
	painter.setPen( QColor( 255 , 255, 255, 200 ) );
	painter.drawText( 0, 0, width(), 20, Qt::AlignCenter, m_sSampleName );
}




void TargetWaveDisplay::updateDisplay( float *pSampleData, unsigned nSampleLenght )
{



	if ( pSampleData ) {
		m_psampleData = pSampleData;
		 m_pSampleLenght = nSampleLenght;
	
		float nScaleFactor = nSampleLenght / (width());


		float fGain = height() / 2.0 * 1.0;

		int nSamplePos =0;
		int nVal;
		for ( int i = 0; i < width(); ++i ){
			for ( int j = 0; j < nScaleFactor; ++j ) {
				if ( j < nSampleLenght ) {
					int newVal = (int)( pSampleData[ nSamplePos ] * fGain );
					nVal = newVal;
				}
				++nSamplePos;
			}
			m_pPeakData[ i ] = nVal;
		}

	}

	update();

}

void TargetWaveDisplay::reloadDisplay()
{

	float nScaleFactor = m_pSampleLenght / (width());


	float fGain = height() / 2.0 * 1.0;

	int nSamplePos =0;
	int nVal;
	for ( int i = 0; i < width(); ++i ){
		for ( int j = 0; j < nScaleFactor; ++j ) {
			if ( j < m_pSampleLenght ) {
				int newVal = (int)( m_psampleData[ nSamplePos ] * fGain );
				nVal = newVal;
			}
			++nSamplePos;
		}
		m_pPeakData[ i ] = nVal;
	}


	update();

}