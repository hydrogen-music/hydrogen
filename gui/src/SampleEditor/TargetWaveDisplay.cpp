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

#include "HydrogenApp.h"
#include "SampleEditor.h"

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
	int w = 841;
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
	painter.setRenderHint( QPainter::HighQualityAntialiasing );
	painter.drawPixmap( ev->rect(), m_background, ev->rect() );

	painter.setPen( QColor( 255 , 255, 255 ) );
	int VCenter = height() / 2;
	for ( int x = 0; x < width(); x++ ) {
		painter.drawLine( x, -m_pPeakData[x] +VCenter, x, -m_pPeakData[x +1] +VCenter  );
	}

	QFont font;
	font.setWeight( 63 );
	painter.setFont( font );
//start frame pointer
	painter.setPen( QColor( 99, 175, 254, 200 ) );
	painter.drawLine( m_pFadeOutFramePosition, 4, m_pFadeOutFramePosition, height() -4 );	
	painter.drawText( m_pFadeOutFramePosition , 1, 10,20, Qt::AlignRight, "F" );

}


void TargetWaveDisplay::updateDisplayPointer()
{
	update();
}

void TargetWaveDisplay::updateDisplay( H2Core::InstrumentLayer *pLayer )
{
	if ( pLayer && pLayer->get_sample() ) {

		int nSampleLenght = pLayer->get_sample()->get_n_frames();
		float nScaleFactor = nSampleLenght / width();

		float fGain = height() / 2.0 * pLayer->get_gain();

		float *pSampleData = pLayer->get_sample()->get_data_l();

		int nSamplePos =0;
		int nVal;
		for ( int i = 0; i < width(); ++i ){
			nVal = 0;
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


void TargetWaveDisplay::mouseMoveEvent(QMouseEvent *ev)
{
	testPosition( ev );
	update();
}



void TargetWaveDisplay::mousePressEvent(QMouseEvent *ev)
{
	testPosition( ev );
	update();
}


void TargetWaveDisplay::testPosition( QMouseEvent *ev )
{
		m_pFadeOutFramePosition = ev->x() ;
}


void TargetWaveDisplay::mouseReleaseEvent(QMouseEvent *ev)
{
	update();
	HydrogenApp::getInstance()->getSampleEditor()->returnAllTargetDisplayValues();
}
