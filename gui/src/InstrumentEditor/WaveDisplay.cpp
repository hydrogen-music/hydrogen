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

#include "WaveDisplay.h"
#include "../Skin.h"


WaveDisplay::WaveDisplay(QWidget* pParent)
 : QWidget( pParent )
 , Object( "WaveDisplay" )
 , m_sSampleName( "" )
{
	setAttribute(Qt::WA_NoBackground);

	//INFOLOG( "INIT" );
	int w = 277;
	int h = 58;
	resize( w, h );

	bool ok = m_background.load( Skin::getImagePath() + "/waveDisplay/background.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	m_pPeakData = new int[ w ];

}




WaveDisplay::~WaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakData;
}



void WaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );
	painter.drawPixmap( ev->rect(), m_background, ev->rect() );

	painter.setPen( QColor( 102, 150, 205 ) );
	int VCenter = height() / 2;
	for ( int x = 0; x < width(); x++ ) {
		painter.drawLine( x, VCenter, x, m_pPeakData[x] + VCenter );
		painter.drawLine( x, VCenter, x, -m_pPeakData[x] + VCenter );
	}

	QFont font;
	font.setWeight( 63 );
	painter.setFont( font );
	painter.setPen( QColor( 255 , 255, 255, 200 ) );
	painter.drawText( 0, 0, width(), 20, Qt::AlignCenter, m_sSampleName );
}



void WaveDisplay::updateDisplay( H2Core::InstrumentLayer *pLayer )
{
	if ( pLayer && pLayer->get_sample() ) {
		// Extract the filename from the complete path
		QString sName = pLayer->get_sample()->get_filename();
		int nPos = sName.lastIndexOf( "/" );
		m_sSampleName = sName.mid( nPos + 1, sName.length() );

//		INFOLOG( "[updateDisplay] sample: " + m_sSampleName  );

		int nSampleLength = pLayer->get_sample()->get_n_frames();
		float nScaleFactor = nSampleLength / width();

		float fGain = height() / 2.0 * pLayer->get_gain();

		float *pSampleData = pLayer->get_sample()->get_data_l();

		int nSamplePos =0;
		int nVal;
		for ( int i = 0; i < width(); ++i ){
			nVal = 0;
			for ( int j = 0; j < nScaleFactor; ++j ) {
				if ( j < nSampleLength ) {
					int newVal = (int)( pSampleData[ nSamplePos ] * fGain );
					if ( newVal > nVal ) {
						nVal = newVal;
					}
				}
				++nSamplePos;
			}
			m_pPeakData[ i ] = nVal;
		}
	}
	else {
		m_sSampleName = "-";
		for ( int i =0; i < width(); ++i ){
			m_pPeakData[ i ] = 0;
		}
	}

	update();
}

