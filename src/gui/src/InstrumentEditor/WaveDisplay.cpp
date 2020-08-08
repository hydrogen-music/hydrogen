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

#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_layer.h>
using namespace H2Core;

#include "WaveDisplay.h"
#include "../Skin.h"

const char* WaveDisplay::__class_name = "WaveDisplay";

WaveDisplay::WaveDisplay(QWidget* pParent)
 : QWidget( pParent )
 , Object( __class_name )
 , m_nCurrentWidth( 0 )
 , m_sSampleName( "-" )
 , m_pLayer( nullptr )
 , m_SampleNameAlignment( Qt::AlignCenter )
{
	setAttribute(Qt::WA_NoBackground);

	//INFOLOG( "INIT" );

	bool ok = m_Background.load( Skin::getImagePath() + "/waveDisplay/bgsamplewavedisplay.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	m_pPeakData = new int[ width() ];
	memset( m_pPeakData, 0, width() * sizeof( m_pPeakData[0] ) );
}




WaveDisplay::~WaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakData;
}

void WaveDisplay::paintEvent( QPaintEvent *ev )
{
	UNUSED(ev);
	
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );

	QBrush brush = QBrush(Qt::red, m_Background);
	brush.setStyle(Qt::TexturePattern);
	painter.setBrush(brush);
	painter.drawRect(0, 0, width(), height());
	
	if( m_pLayer ){
		painter.setPen( QColor( 102, 150, 205 ) );
		int VCenter = height() / 2;
		for ( int x = 0; x < width(); x++ ) {
			painter.drawLine( x, VCenter, x, m_pPeakData[x] + VCenter );
			painter.drawLine( x, VCenter, x, -m_pPeakData[x] + VCenter );
		}
		
	}
	
	QFont font;
	font.setWeight( 63 );
	painter.setFont( font );
	painter.setPen( QColor( 255 , 255, 255, 200 ) );
	
	if( m_SampleNameAlignment == Qt::AlignCenter ){
		painter.drawText( 0, 0, width(), 20, m_SampleNameAlignment, m_sSampleName );
	} 
	else if( m_SampleNameAlignment == Qt::AlignLeft )
	{
		// Use a small offnset iso. starting directly at the left border
		painter.drawText( 20, 0, width(), 20, m_SampleNameAlignment, m_sSampleName );
	}
	
}

void WaveDisplay::resizeEvent( QResizeEvent * event )
{
	updateDisplay(m_pLayer);
}



void WaveDisplay::updateDisplay( H2Core::InstrumentLayer *pLayer )
{
	int currentWidth = width();
	
	if(!pLayer || currentWidth <= 0){
		m_pLayer = nullptr;
		m_sSampleName = "-";

		update();
		return;
	}
	
	if(currentWidth != m_nCurrentWidth){
		delete[] m_pPeakData;
		m_pPeakData = new int[ currentWidth ];
		
		m_nCurrentWidth = currentWidth;
	}
	
	if ( pLayer && pLayer->get_sample() ) {
		m_pLayer = pLayer;
		m_sSampleName = pLayer->get_sample()->get_filename();

		//INFOLOG( "[updateDisplay] sample: " + m_sSampleName  );

		int nSampleLength = pLayer->get_sample()->get_frames();
		int nScaleFactor = nSampleLength / m_nCurrentWidth;

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
		for ( int i =0; i < m_nCurrentWidth; ++i ){
			m_pPeakData[ i ] = 0;
		}
		
	}

	update();
}

void WaveDisplay::mouseDoubleClickEvent(QMouseEvent *ev)
{
	if (ev->button() == Qt::LeftButton) {
	    emit doubleClicked(this);
	}	
}
