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
#include <hydrogen/hydrogen.h>

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
//	setAttribute(Qt::WA_NoBackground);

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
	m_pvmove = false;
	m_info = "";
	m_x = -10;
	m_y = -10;
	m_plocator = -1;
	m_pupdateposi = false;
}




TargetWaveDisplay::~TargetWaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakDatal;
	delete[] m_pPeakDatar;
}



void TargetWaveDisplay::paintEvent(QPaintEvent *ev)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
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
//	painter.setPen( QColor( 99, 175, 254, 200 ) );
//	painter.drawLine( m_pFadeOutFramePosition, 4, m_pFadeOutFramePosition, height() -4 );	
//	painter.drawText( m_pFadeOutFramePosition , 1, 10,20, Qt::AlignRight, "F" );

	for ( int i = 0; i < static_cast<int>(pEngine->m_volumen.size()) -1; i++){
		//volume line
		painter.setPen( QPen(QColor( 255, 255, 255, 200 ) ,1 , Qt::SolidLine) );
		painter.drawLine( pEngine->m_volumen[i].m_hxframe, pEngine->m_volumen[i].m_hyvalue, pEngine->m_volumen[i + 1].m_hxframe, pEngine->m_volumen[i +1].m_hyvalue );
		painter.setBrush(QColor( 99, 160, 233 ));
		painter.drawEllipse ( pEngine->m_volumen[i].m_hxframe - 6/2, pEngine->m_volumen[i].m_hyvalue  - 6/2, 6, 6 );
	}

	for ( int i = 0; i < static_cast<int>(pEngine->m_pan.size()) -1; i++){
		//pan line
		painter.setPen( QPen(QColor( 249, 235, 116, 200 ) ,1 , Qt::SolidLine) );
		painter.drawLine( pEngine->m_pan[i].m_hxframe, pEngine->m_pan[i].m_hyvalue, pEngine->m_pan[i + 1].m_hxframe, pEngine->m_pan[i +1].m_hyvalue );
		painter.setBrush(QColor( 77, 189, 55 ));
		painter.drawEllipse ( pEngine->m_pan[i].m_hxframe - 6/2, pEngine->m_pan[i].m_hyvalue  - 6/2, 6, 6 );
	}


	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::SolidLine ) );
	painter.drawLine( m_plocator, 4, m_plocator, height() -4);

	//volume line
	//first rect 
	painter.setPen( QPen(QColor( 255, 255, 255, 200 ) ,1 , Qt::SolidLine) );
	painter.setBrush(QColor( 99, 160, 233 ));
	painter.drawRect ( pEngine->m_volumen[0].m_hxframe - 12/2, pEngine->m_volumen[0].m_hyvalue  - 6/2, 12, 6 );
	//last rect 
	painter.drawRect ( pEngine->m_volumen[pEngine->m_volumen.size() -1].m_hxframe - 12/2, pEngine->m_volumen[pEngine->m_volumen.size() -1].m_hyvalue  - 6/2, 12, 6 );

	//pan line
	//first rect 
	painter.setPen( QPen(QColor( 249, 235, 116, 200 ) ,1 , Qt::SolidLine) );
	painter.setBrush(QColor( 77, 189, 55 ));
	painter.drawRect ( pEngine->m_pan[0].m_hxframe - 12/2, pEngine->m_pan[0].m_hyvalue  - 6/2, 12, 6 );
	//last rect 
	painter.drawRect ( pEngine->m_pan[pEngine->m_pan.size() -1].m_hxframe - 12/2, pEngine->m_pan[pEngine->m_pan.size() -1].m_hyvalue  - 6/2, 12, 6 );


	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 0, lcenter, 841, lcenter );	
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 0, rcenter, 841, rcenter );

	if (m_y < 50){
		if (m_x < 790){
			painter.drawText( m_x +5, m_y, 60, 20, Qt::AlignLeft, QString( m_info ) );
		}else
		{
			painter.drawText( m_x - 65, m_y, 60, 20, Qt::AlignRight, QString( m_info ) );
		}
		
	}else
	{
		if (m_x < 790){
			painter.drawText( m_x +5, m_y -20, 60, 20, Qt::AlignLeft, QString( m_info ) );
		}else
		{
			painter.drawText( m_x - 65, m_y -20, 60, 20, Qt::AlignRight, QString( m_info ) );
		}
	}

}


void TargetWaveDisplay::updateDisplayPointer()
{
	update();
}

void TargetWaveDisplay::paintLocatorEventTargetDisplay( int pos, bool updateposi)
{
	m_pupdateposi = updateposi;
	if ( !updateposi ){
		m_plocator = -1;
	}else
	{
		m_plocator = pos;
	}
	update();
}

void TargetWaveDisplay::updateDisplay( H2Core::InstrumentLayer *pLayer )
{
	if ( pLayer && pLayer->get_sample() ) {

		int nSampleLength = pLayer->get_sample()->get_n_frames();
		float nScaleFactor = nSampleLength / width();

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
				if ( j < nSampleLength ) {
					if ( pSampleDatal[ nSamplePos ] < 0 ){
						int newVal = static_cast<int>( pSampleDatal[ nSamplePos ] * -fGain );
						nVall = newVal;
					}else
					{
						int newVal = static_cast<int>( pSampleDatal[ nSamplePos ] * fGain );
						nVall = newVal;
					}
					if ( pSampleDatar[ nSamplePos ] > 0 ){
						int newVal = static_cast<int>( pSampleDatar[ nSamplePos ] * -fGain );
						nValr = newVal;
					}else
					{
						int newVal = static_cast<int>( pSampleDatar[ nSamplePos ] * fGain );
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
	Hydrogen *pEngine = Hydrogen::get_instance();
	int snapradius = 10;
	QString editType = HydrogenApp::get_instance()->getSampleEditor()->EditTypeComboBox->currentText();



	///edit volume points
	if( editType == "volume" ){
		m_pvmove = true;
	
		if ( ev->x() <= 0 || ev->x() >= 841 || ev->y() < 0 || ev->y() > 91 ){
			update();
			m_pvmove = false;
			return;
		}
		float info = (91 - ev->y()) / 91.0;
		m_info.setNum( info, 'g', 2 );
		m_x = ev->x();
		m_y = ev->y();
	
		for ( int i = 0; i < static_cast<int>(pEngine->m_volumen.size()); i++){
			if ( pEngine->m_volumen[i].m_hxframe >= ev->x() - snapradius && pEngine->m_volumen[i].m_hxframe <= ev->x() + snapradius ){
				pEngine->m_volumen.erase( pEngine->m_volumen.begin() + i);
				Hydrogen::HVeloVector velovector;
				if ( i == 0 ){
					velovector.m_hxframe = 0;
					velovector.m_hyvalue = ev->y();
				}
				else if ( i == static_cast<int>(pEngine->m_volumen.size()) ){
					velovector.m_hxframe = pEngine->m_volumen[i].m_hxframe;
					velovector.m_hyvalue = ev->y();
					
				}else
				{
					velovector.m_hxframe = ev->x();
					velovector.m_hyvalue = ev->y();
				}
	
				pEngine->m_volumen.push_back( velovector );
				pEngine->sortVolVectors();	
				update();
				return;
			}else
			{
				m_pvmove = false;	
			}
		}
	///edit panorama points
	}else if( editType == "panorama" ){
		m_pvmove = true;
	
		if ( ev->x() <= 0 || ev->x() >= 841 || ev->y() < 0 || ev->y() > 91 ){
			update();
			m_pvmove = false;
			return;
		}
		float info = (45 - ev->y()) / 45.0;
		m_info.setNum( info, 'g', 2 );
		m_x = ev->x();
		m_y = ev->y();
	
		for ( int i = 0; i < static_cast<int>(pEngine->m_pan.size()); i++){
			if ( pEngine->m_pan[i].m_hxframe >= ev->x() - snapradius && pEngine->m_pan[i].m_hxframe <= ev->x() + snapradius ){
				pEngine->m_pan.erase( pEngine->m_pan.begin() + i);
				Hydrogen::HPanVector panvector;
				if ( i == 0 ){
					panvector.m_hxframe = 0;
					panvector.m_hyvalue = ev->y();
				}
				else if ( i == static_cast<int>(pEngine->m_pan.size()) ){
					panvector.m_hxframe = pEngine->m_pan[i].m_hxframe;
					panvector.m_hyvalue = ev->y();
					
				}else
				{
					panvector.m_hxframe = ev->x();
					panvector.m_hyvalue = ev->y();
				}
	
				pEngine->m_pan.push_back( panvector );
				pEngine->sortPanVectors();	
				update();
				return;
			}else
			{
				m_pvmove = false;	
			}
		}
	}

	update();
	HydrogenApp::get_instance()->getSampleEditor()->setTrue();
}



void TargetWaveDisplay::mousePressEvent(QMouseEvent *ev)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	int snapradius = 6;
	bool newpoint = true;

	// add new point
	QString editType = HydrogenApp::get_instance()->getSampleEditor()->EditTypeComboBox->currentText();


	///edit volume points
	if( editType == "volume" ){
		// test if there is already a point
		for ( int i = 0; i < static_cast<int>(pEngine->m_volumen.size()); ++i){
			if ( pEngine->m_volumen[i].m_hxframe >= ev->x() - snapradius && pEngine->m_volumen[i].m_hxframe <= ev->x() + snapradius ){
				newpoint = false;
			}
		}
		int x = ev->x();
		int y = ev->y();	
		if (ev->button() == Qt::LeftButton && !m_pvmove && newpoint){
			float info = (91 - ev->y()) / 91.0;
			m_info.setNum( info, 'g', 2 );
			m_x = ev->x();
			m_y = ev->y();
			Hydrogen::HVeloVector velovector;
			if ( ev->y() <= 0 ) y = 0;
			if ( ev->y() >= 91 ) y = 91;
			if ( ev->x() <= 6 ) x = 6;
			if ( ev->x() >= 835 ) x = 835;
			velovector.m_hxframe = x;
			velovector.m_hyvalue = y;
			pEngine->m_volumen.push_back( velovector );
			pEngine->sortVolVectors();		
		}
	
	
		//remove point
		snapradius = 10;
		if (ev->button() == Qt::RightButton ){
	
			if ( ev->x() <= 0 || ev->x() >= 841 ){
				update();
				return;
			}
			m_info = "";

			for ( int i = 0; i < static_cast<int>(pEngine->m_volumen.size()); i++){
				if ( pEngine->m_volumen[i].m_hxframe >= ev->x() - snapradius && pEngine->m_volumen[i].m_hxframe <= ev->x() + snapradius ){
					if ( pEngine->m_volumen[i].m_hxframe == 0 || pEngine->m_volumen[i].m_hxframe == 841) return;
					pEngine->m_volumen.erase( pEngine->m_volumen.begin() +  i);
				}
			}	
		}
	}
	///edit panorama points
	else if( editType == "panorama" ){
		// test if there is already a point
		for ( int i = 0; i < static_cast<int>(pEngine->m_pan.size()); ++i){
			if ( pEngine->m_pan[i].m_hxframe >= ev->x() - snapradius && pEngine->m_pan[i].m_hxframe <= ev->x() + snapradius ){
				newpoint = false;
			}
		}
		int x = ev->x();
		int y = ev->y();	
		if (ev->button() == Qt::LeftButton && !m_pvmove && newpoint){
			float info = (45 - ev->y()) / 45.0;
			m_info.setNum( info, 'g', 2 );
			m_x = ev->x();
			m_y = ev->y();
			Hydrogen::HPanVector panvector;
			if ( ev->y() <= 0 ) y = 0;
			if ( ev->y() >= 91 ) y = 91;
			if ( ev->x() <= 6 ) x = 6;
			if ( ev->x() >= 835 ) x = 835;
			panvector.m_hxframe = x;
			panvector.m_hyvalue = y;
			pEngine->m_pan.push_back( panvector );
			pEngine->sortPanVectors();		
		}
	
	
		//remove point
		snapradius = 10;
		if (ev->button() == Qt::RightButton ){
	
			if ( ev->x() <= 0 || ev->x() >= 841 ){
				update();
				return;
			}
			m_info = "";

			for ( int i = 0; i < static_cast<int>(pEngine->m_pan.size()); i++){
				if ( pEngine->m_pan[i].m_hxframe >= ev->x() - snapradius && pEngine->m_pan[i].m_hxframe <= ev->x() + snapradius ){
					if ( pEngine->m_pan[i].m_hxframe == 0 || pEngine->m_pan[i].m_hxframe == 841) return;
					pEngine->m_pan.erase( pEngine->m_pan.begin() +  i);
				}
			}	
		}
	}

	update();
	HydrogenApp::get_instance()->getSampleEditor()->setTrue();
}





void TargetWaveDisplay::mouseReleaseEvent(QMouseEvent *ev)
{
	update();
	HydrogenApp::get_instance()->getSampleEditor()->returnAllTargetDisplayValues();
}
