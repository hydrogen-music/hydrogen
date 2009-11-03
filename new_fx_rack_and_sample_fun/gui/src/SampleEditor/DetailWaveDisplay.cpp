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

#include "DetailWaveDisplay.h"
#include "../Skin.h"


DetailWaveDisplay::DetailWaveDisplay(QWidget* pParent )
 : QWidget( pParent )
 , Object( "DetailWaveDisplay" )
 , m_sSampleName( "" )
{
//	setAttribute(Qt::WA_NoBackground);

	//INFOLOG( "INIT" );
	int w = 180;
	int h = 265;
	resize( w, h );

	bool ok = m_background.load( Skin::getImagePath() + "/waveDisplay/detailsamplewavedisplay.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	m_pnormalimagedetailframes = 180;	
	m_pDetailSamplePosition = 0;
	m_pzoomFactor = 1;

}




DetailWaveDisplay::~DetailWaveDisplay()
{
	//INFOLOG( "DESTROY" );
	delete[] m_pPeakDatal;
	delete[] m_pPeakDatar;
}


void DetailWaveDisplay::setDetailSamplePosition( unsigned posi, float zoomfactor, QString type)
{
	m_pDetailSamplePosition = posi ;
	m_pzoomFactor = zoomfactor;
	m_ptype = type;
	update();
}

void DetailWaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::HighQualityAntialiasing );
	painter.drawPixmap( ev->rect(), m_background, ev->rect() );

	painter.setPen( QColor( 230, 230, 230 ) );
	int VCenterl = height() / 4;
	int VCenterr = height() / 4 + height() / 2;

//	int imagedetailframes = m_pnormalimagedetailframes / m_pzoomFactor;
	int startpos = m_pDetailSamplePosition  - m_pnormalimagedetailframes / 2 ;

	for ( int x = 0; x < width() ; x++ ) {
		if ( (startpos) > 0 ){
			painter.drawLine( x, (-m_pPeakDatal[startpos -1] *m_pzoomFactor) +VCenterl, x, (-m_pPeakDatal[startpos ] *m_pzoomFactor)+VCenterl );
			painter.drawLine( x, (-m_pPeakDatar[startpos -1] *m_pzoomFactor) +VCenterr, x, (-m_pPeakDatar[startpos ] *m_pzoomFactor)+VCenterr );
			//ERRORLOG( QString("startpos: %1").arg(startpos) )
		}
		else
		{
			painter.drawLine( x, 0 +VCenterl, x, 0+VCenterl );
			painter.drawLine( x, 0 +VCenterr, x, 0+VCenterr );
		}
		startpos++;
		
	}


	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 0, VCenterl, width(),VCenterl );
	painter.drawLine( 0, VCenterr, width(),VCenterr );
	QColor _color;
	if ( m_ptype == "Start" )
		 _color = QColor( 32, 173, 0 );
	else if ( m_ptype == "Loop" )
		_color = QColor( 93, 170, 254 );
	else if ( m_ptype == "End" )
		_color = QColor( 217, 68, 0 );
	else 
		_color = QColor(  255, 255, 255 );

	painter.setPen( QPen( _color, 1, Qt::SolidLine ) );
	painter.drawLine( 90, 0, 90,265 );
}



void DetailWaveDisplay::updateDisplay( QString filename )
{

	Sample *pNewSample = Sample::load( filename );

	if ( pNewSample ) {

		int mSampleLength = pNewSample->get_n_frames();

		m_pPeakDatal = new int[ mSampleLength + m_pnormalimagedetailframes /2 ];
		m_pPeakDatar = new int[ mSampleLength + m_pnormalimagedetailframes /2 ];

		for ( int i = 0 ; i < mSampleLength + m_pnormalimagedetailframes /2 ; i++){
			m_pPeakDatal[ i ] = 0;
			m_pPeakDatar[ i ] = 0;
		}

		float fGain = height() / 4.0 * 1.0;

		float *pSampleDatal = pNewSample->get_data_l();
		float *pSampleDatar = pNewSample->get_data_r();

		for ( int i = 0; i < mSampleLength; i++ ){
			m_pPeakDatal[ i ] = static_cast<int>( pSampleDatal[ i ] * fGain );
			m_pPeakDatar[ i ] = static_cast<int>( pSampleDatar[ i ] * fGain );
		}


	}
	delete pNewSample;
}



