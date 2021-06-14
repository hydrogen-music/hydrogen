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

#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Basics/Instrument.h>
#include "HydrogenApp.h"
#include "SampleEditor.h"
using namespace H2Core;

#include "MainSampleWaveDisplay.h"
#include "../Skin.h"

const char* MainSampleWaveDisplay::__class_name = "MainSampleWaveDisplay";

MainSampleWaveDisplay::MainSampleWaveDisplay(QWidget* pParent)
 : QWidget( pParent )
 , Object( __class_name )
{
//	setAttribute(Qt::WA_OpaquePaintEvent);

	//INFOLOG( "INIT" );
	int w = 624;
	int h = 265;
	resize( w, h );

	bool ok = m_background.load( Skin::getImagePath() + "/waveDisplay/mainsamplewavedisplay.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	m_pPeakDatal = new int[ w ];
	m_pPeakDatar = new int[ w ];

	m_nStartFramePosition = 25;
	m_nLoopFramePosition = 25;
	m_nEndFramePosition = width() -25;
	m_bMove = false;
	m_nLocator = -1;
	m_bUpdatePosition = false;
	m_nSampleLength = 0;

	m_bStartSliderIsMoved = false;
	m_bLoopSliderIsMoved = false;
	m_bEndSliderIsmoved = false;
	setMouseTracking(true);
}




MainSampleWaveDisplay::~MainSampleWaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakDatal;
	delete[] m_pPeakDatar;
}

void MainSampleWaveDisplay::paintLocatorEvent( int pos, bool updateposi)
{
	m_bUpdatePosition = updateposi;
	if ( !updateposi ){
		m_nLocator = -1;
	}else
	{
		m_nLocator = pos;
	}
	update();
}

void MainSampleWaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );

	bool issmaller = false;

	painter.drawPixmap( ev->rect(), m_background, ev->rect() );
	painter.setPen( QColor( 230, 230, 230 ) );
	int VCenterl = height() / 4;
	int VCenterr = height() / 4 + height() / 2;

	if ( width() >= m_nSampleLength  ) issmaller = true;

	for ( int x = 25; x < width() -25; x++ ) {
		if ( !issmaller || x <= m_nSampleLength){
			painter.drawLine( x, -m_pPeakDatal[x -25] +VCenterl, x, -m_pPeakDatal[x -24] +VCenterl  );
			painter.drawLine( x, -m_pPeakDatar[x -25] +VCenterr, x, -m_pPeakDatar[x -24] +VCenterr  );	
		}else
		{
			painter.drawLine( x, 0 +VCenterl, x, 0 +VCenterl  );
			painter.drawLine( x, 0 +VCenterr, x, 0 +VCenterr  );
		}
	
	}


	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 23, 4, 23, height() -4 );
	painter.drawLine( width() -23, 4,width() -23, height() -4 );
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::SolidLine ) );
	painter.drawLine( m_nLocator, 4, m_nLocator, height() -4);
	painter.drawLine( 0, VCenterl, width(),VCenterl );
	painter.drawLine( 0, VCenterr, width(),VCenterr );

	QFont font;
	font.setWeight( 63 );
	painter.setFont( font );
//start frame pointer
	painter.setPen( QColor( 32, 173, 0, 200 ) );
	painter.drawLine( m_nStartFramePosition, 4, m_nStartFramePosition, height() -4 );	
	painter.drawText( m_nStartFramePosition -10, 250, 10,20, Qt::AlignRight, "S" );
//endframe pointer
	painter.setPen( QColor( 217, 68, 0, 200 ) );
	painter.drawLine( m_nEndFramePosition, 4, m_nEndFramePosition, height() -4 );
	painter.drawText( m_nEndFramePosition -10, 123, 10, 20, Qt::AlignRight, "E" );
//loopframe pointer
	painter.setPen( QColor( 93, 170, 254, 200 ) );
	painter.drawLine( m_nLoopFramePosition, 4, m_nLoopFramePosition, height() -4 );
	painter.drawText( m_nLoopFramePosition , 0, 10, 20, Qt::AlignLeft, "L" );


}



void MainSampleWaveDisplay::updateDisplayPointer()
{
	update();
}



void MainSampleWaveDisplay::updateDisplay( const QString& filename )
{

	auto pNewSample = Sample::load( filename );
	
	if ( pNewSample ) {

		int nSampleLength = pNewSample->get_frames();
		m_nSampleLength = nSampleLength;
		float nScaleFactor = nSampleLength / (width() -50);
		if ( nScaleFactor < 1 ){
			nScaleFactor = 1;
		}

		float fGain = height() / 4.0 * 1.0;

		auto pSampleDatal = pNewSample->get_data_l();
		auto pSampleDatar = pNewSample->get_data_r();

		unsigned nSamplePos = 0;
		int nVall = 0;
		int nValr = 0;
		int newVall = 0;
		int newValr = 0;
		for ( int i = 0; i < width(); ++i ){
			for ( int j = 0; j < nScaleFactor; ++j ) {
				if ( j < nSampleLength && nSamplePos < nSampleLength) {
					if ( pSampleDatal[ nSamplePos ] && pSampleDatar[ nSamplePos ] ){
						newVall = static_cast<int>( pSampleDatal[ nSamplePos ] * fGain );
						newValr = static_cast<int>( pSampleDatar[ nSamplePos ] * fGain );
						nVall = newVall;
						nValr = newValr;
					}else
					{
						nVall = 0;	
						nValr = 0;
					}
				}
				++nSamplePos;
			}
			m_pPeakDatal[ i ] = nVall;
			m_pPeakDatar[ i ] = nValr;
		}
	}
	update();

}

void MainSampleWaveDisplay::mouseMoveEvent(QMouseEvent *ev)
{
	if (ev->buttons() && Qt::LeftButton) {
		testPosition( ev );
		update();
	}
}

void MainSampleWaveDisplay::mousePressEvent(QMouseEvent *ev)
{
	testPosition( ev );
	update();
}

void MainSampleWaveDisplay::testPosition( QMouseEvent *ev )
{
	assert(ev);
	
//startframepointer
	if  (ev->y()>=200 ) {
		m_nStartFramePosition = ev->x() ;
		m_bStartSliderIsMoved = true;
		if ( m_nStartFramePosition > m_nLoopFramePosition ){
			m_nLoopFramePosition = m_nStartFramePosition;
			m_bLoopSliderIsMoved = true;
		}
		if ( m_nStartFramePosition > m_nEndFramePosition ){
			m_nEndFramePosition = m_nStartFramePosition;
			m_bEndSliderIsmoved = true;
		}
//		update();
	}

//loopframeposition
	else if  (ev->y()<=65 ) {
		m_nLoopFramePosition = ev->x() ;
		m_bLoopSliderIsMoved = true;		
		if ( m_nLoopFramePosition < m_nStartFramePosition ){
			m_nStartFramePosition = m_nLoopFramePosition;
			m_bStartSliderIsMoved = true;
		}
		if ( m_nLoopFramePosition > m_nEndFramePosition ){
			m_nEndFramePosition = m_nLoopFramePosition;
			m_bEndSliderIsmoved = true;
		}
//		update();
	}
//endframeposition
	else if  ( ev->y() >= 86 && ev->y() <= 179  ) {
		m_nEndFramePosition = ev->x() ;
		m_bEndSliderIsmoved = true;
		if ( m_nEndFramePosition <  m_nLoopFramePosition ){
			m_nLoopFramePosition = m_nEndFramePosition;
			m_bLoopSliderIsMoved = true;
		}
		if ( m_nEndFramePosition <  m_nStartFramePosition ){
			m_nStartFramePosition = m_nEndFramePosition;
			m_bStartSliderIsMoved = true;
		}
//		update();
	}

	if ( ( m_nStartFramePosition ) >= width() -25 ) m_nStartFramePosition =width() -25;
	if ( ( m_nLoopFramePosition ) >= width() -25 ) m_nLoopFramePosition =width() -25;
	if ( ( m_nEndFramePosition ) >= width() -25 ) m_nEndFramePosition =width() -25;
	if ( ( m_nStartFramePosition ) <= 25 ) m_nStartFramePosition = 25;
	if ( ( m_nLoopFramePosition ) <= 25 ) m_nLoopFramePosition = 25;
	if ( ( m_nEndFramePosition ) <= 25 ) m_nEndFramePosition = 25;
}


void MainSampleWaveDisplay::mouseReleaseEvent(QMouseEvent *ev)
{
	update();
	bool test = HydrogenApp::get_instance()->getSampleEditor()->returnAllMainWaveDisplayValues();

	if (test){
		m_bStartSliderIsMoved = false;
		m_bLoopSliderIsMoved = false;
		m_bEndSliderIsmoved = false;
	}
}



