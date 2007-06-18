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
 * $Id: WaveDisplay.cpp,v 1.12 2005/05/09 18:11:47 comix Exp $
 *
 */
#include <qpainter.h>

#include "gui/Skin.h"
#include "lib/Sample.h"
#include "lib/Song.h"
#include "WaveDisplay.h"

WaveDisplay::WaveDisplay(QWidget* pParent)
 : QWidget( pParent , "WaveDisplay", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "WaveDisplay" )
 , m_bChanged( true )
 , m_sSampleName( "" )
{
	infoLog( "INIT" );
	int w = 235;
	int h = 58;
	resize( w, h );

	string background_path = Skin::getImagePath() + string( "/waveDisplay/background.png" );
	bool ok = m_background.load( background_path.c_str() );
	if( ok == false ){
		errorLog( string("Error loading pixmap ").append( background_path ) );
	}

	m_temp.resize( width(), height() );

	m_pPeakData = new int[ w ];

}

WaveDisplay::~WaveDisplay()
{
	infoLog( "DESTROY" );

	delete[] m_pPeakData;
}


void WaveDisplay::paintEvent(QPaintEvent *ev)
{
	if ( !isVisible() ) {
		return;
	}
	if ( m_bChanged ) {
		m_bChanged = false;
		bitBlt( &m_temp, 0, 0, &m_background, 0, 0, width(), height(), CopyROP);

		QPainter p( &m_temp );
		p.setPen( QColor( 102, 150, 205 ) );
		p.drawText( 0, 0, width(), 20, Qt::AlignCenter, QString( m_sSampleName.c_str() ) );

		int VCenter = height() / 2;
		for ( int x = 0; x < width(); x++ ) {
			p.drawLine( x, VCenter, x, m_pPeakData[x] + VCenter );
			p.drawLine( x, VCenter, x, -m_pPeakData[x] + VCenter );
		}
	}
	bitBlt(this, 0, 0, &m_temp, 0, 0, width(), height(), CopyROP);
}


void WaveDisplay::updateDisplay( InstrumentLayer *pLayer )
{
	if ( pLayer && pLayer->m_pSample ) {
		// Extract the filename from the complete path
		string sName = pLayer->m_pSample->m_sFilename;
		int nPos = sName.rfind("/");
		m_sSampleName = sName.substr( nPos + 1, sName.length() );

//		infoLog( "[updateDisplay] sample: " + m_sSampleName  );

		int nSampleLenght = pLayer->m_pSample->m_nFrames;
		float nScaleFactor = nSampleLenght / width();

		float fGain = height() / 2.0 * pLayer->m_fGain;

		float *pSampleData = pLayer->m_pSample->m_pData_L;

		int nSamplePos =0;
		int nVal;
		for ( int i =0; i < width(); ++i ){
			nVal = 0;
			for ( int j = 0; j < nScaleFactor; ++j ) {
				if ( j < nSampleLenght ) {
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

	m_bChanged = true;
	update();
}

