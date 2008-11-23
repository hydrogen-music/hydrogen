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

#include <vector>
#include <algorithm>
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

	m_pPeakDatal = new int[ w ];
	m_pPeakDatar = new int[ w ];

}




TargetWaveDisplay::~TargetWaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakDatal;
	delete[] m_pPeakDatar;
}



void TargetWaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::HighQualityAntialiasing );
	painter.drawPixmap( ev->rect(), m_background, ev->rect() );

	painter.setPen( QColor( 252, 142, 73 ));
	int VCenter = height() / 2;
	int lcenter = VCenter -4;
	int rcenter = VCenter +4;
	for ( int x = 0; x < width(); x++ ) {
		painter.drawLine( x, lcenter, x, -m_pPeakDatal[x +1] +lcenter  );
	}

	painter.setPen( QColor( 116, 186, 255 ));
	for ( int x = 0; x < width(); x++ ) {
		painter.drawLine( x, rcenter, x, -m_pPeakDatar[x +1] +rcenter  );
	}

	QFont font;
	font.setWeight( 63 );
	painter.setFont( font );
//start frame pointer
	painter.setPen( QColor( 99, 175, 254, 200 ) );
	painter.drawLine( m_pFadeOutFramePosition, 4, m_pFadeOutFramePosition, height() -4 );	
	painter.drawText( m_pFadeOutFramePosition , 1, 10,20, Qt::AlignRight, "F" );

	for ( int i = 0; i < HydrogenApp::getInstance()->getSampleEditor()->m_volumen.size(); i++){
		painter.setPen( QPen(QColor( 255, 255, 255, 200 ) ,2 , Qt::SolidLine) );
		painter.drawLine( HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i - 1].m_hxframe, HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i - 1].m_hyvalue,HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i].m_hxframe, HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i].m_hyvalue );
		painter.setPen( QPen(QColor( 255, 255, 255, 200 ) ,1 , Qt::SolidLine) );
		painter.setBrush(QColor( 99, 160, 233 ));
		if ( HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i].m_hxframe == 0 ||  
		     HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i].m_hxframe == 841 ){
			painter.drawEllipse ( HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i].m_hxframe - 12/2, HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i].m_hyvalue  - 12/2, 12, 12 );
		}else
		{
			painter.drawEllipse ( HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i].m_hxframe - 6/2, HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i].m_hyvalue  - 6/2, 6, 6 );
		}
	}

	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 0, lcenter, 841, lcenter );	
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 0, rcenter, 841, rcenter );

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

		float fGain = (height() - 8) / 2.0 * pLayer->get_gain();

		float *pSampleDatal = pLayer->get_sample()->get_data_l();
		float *pSampleDatar = pLayer->get_sample()->get_data_r();
		int nSamplePos = 0;
		int nVall;
		int nValr;
		for ( int i = 0; i < width(); ++i ){
			nVall = 0;
			nValr = 0;
			for ( int j = 0; j < nScaleFactor; ++j ) {
				if ( j < nSampleLenght ) {
					if ( pSampleDatal[ nSamplePos ] < 0 ){
						int newVal = (int)( pSampleDatal[ nSamplePos ] * -fGain );
						nVall = newVal;
					}else
					{
						int newVal = (int)( pSampleDatal[ nSamplePos ] * fGain );
						nVall = newVal;
					}
					if ( pSampleDatar[ nSamplePos ] > 0 ){
						int newVal = (int)( pSampleDatar[ nSamplePos ] * -fGain );
						nValr = newVal;
					}else
					{
						int newVal = (int)( pSampleDatar[ nSamplePos ] * fGain );
						nValr = newVal;
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


void TargetWaveDisplay::mouseMoveEvent(QMouseEvent *ev)
{
	testPosition( ev );
	update();
}



void TargetWaveDisplay::mousePressEvent(QMouseEvent *ev)
{
	testPosition( ev );

	if (ev->button() == Qt::LeftButton ){
		SampleEditor::HVeloVector velovector;
		velovector.m_hxframe = ev->x();
		velovector.m_hyvalue = ev->y();
		HydrogenApp::getInstance()->getSampleEditor()->m_volumen.push_back( velovector );
		HydrogenApp::getInstance()->getSampleEditor()->sortVectors();		
	}
	if (ev->button() == Qt::RightButton ){

		if ( ev->x() == 0 || ev->x() == 841 ){
			update();
			return;
		}

		for ( int i = 1; i < HydrogenApp::getInstance()->getSampleEditor()->m_volumen.size(); i++){
			if ( ev->x() > HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i].m_hxframe && 
			     ev->x() < HydrogenApp::getInstance()->getSampleEditor()->m_volumen[i + 1].m_hxframe){
				HydrogenApp::getInstance()->getSampleEditor()->m_volumen.erase(HydrogenApp::getInstance()->getSampleEditor()->m_volumen.begin() +  i);
			}
		}	
	}
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
