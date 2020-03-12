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

#include <memory>

#include "HydrogenApp.h"
#include "SampleEditor.h"

using namespace H2Core;

#define UI_WIDTH   841
#define UI_HEIGHT   91

#include <vector>
#include <algorithm>
#include "TargetWaveDisplay.h"
#include "../Skin.h"

const char* TargetWaveDisplay::__class_name = "TargetWaveDisplay";

TargetWaveDisplay::TargetWaveDisplay(QWidget* pParent)
 : QWidget( pParent )
 , Object( __class_name )
 , m_sSampleName( "" )
{
//	setAttribute(Qt::WA_NoBackground);

	//INFOLOG( "INIT" );
	int w = UI_WIDTH;
	int h = UI_HEIGHT;
	resize( w, h );

	bool ok = m_Background.load( Skin::getImagePath() + "/waveDisplay/targetsamplewavedisplay.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	m_pPeakData_Left = new int[ w ];
	m_pPeakData_Right = new int[ w ];
	m_VMove = false;
	m_sInfo = "";
	m_nX = -10;
	m_nY = -10;
	m_nLocator = -1;
	m_UpdatePosition = false;
}




TargetWaveDisplay::~TargetWaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakData_Left;
	delete[] m_pPeakData_Right;
}



void TargetWaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );

	painter.setRenderHint( QPainter::HighQualityAntialiasing );
	painter.drawPixmap( ev->rect(), m_Background, ev->rect() );
	painter.setPen( QColor( 252, 142, 73 ));

	int VCenter = height() / 2;
	int LCenter = VCenter -4;
	int RCenter = VCenter +4;

	for ( int x = 0; x < width() - 1; x++ ) {
		painter.drawLine( x, LCenter, x, -m_pPeakData_Left[x +1] +LCenter  );
	}

	painter.setPen( QColor( 116, 186, 255 ));
	for ( int x = 0; x < width() - 1; x++ ) {
		painter.drawLine( x, RCenter, x, -m_pPeakData_Right[x +1] +RCenter  );
	}

	QFont Font;
	Font.setWeight( 63 );
	painter.setFont( Font );

	for ( int i = 0; i < static_cast<int>(m_VelocityEnvelope.size()) -1; i++){
		//volume line
		painter.setPen( QPen(QColor( 255, 255, 255, 200 ) ,1 , Qt::SolidLine) );
		painter.drawLine( m_VelocityEnvelope[i]->frame, m_VelocityEnvelope[i]->value, m_VelocityEnvelope[i + 1]->frame, m_VelocityEnvelope[i +1]->value );
		painter.setBrush(QColor( 99, 160, 233 ));
		painter.drawEllipse ( m_VelocityEnvelope[i]->frame - 6/2, m_VelocityEnvelope[i]->value  - 6/2, 6, 6 );
	}

	for ( int i = 0; i < static_cast<int>(m_PanEnvelope.size()) -1; i++){
		//pan line
		painter.setPen( QPen(QColor( 249, 235, 116, 200 ) ,1 , Qt::SolidLine) );
		painter.drawLine( m_PanEnvelope[i]->frame, m_PanEnvelope[i]->value, m_PanEnvelope[i + 1]->frame, m_PanEnvelope[i +1]->value );
		painter.setBrush(QColor( 77, 189, 55 ));
		painter.drawEllipse ( m_PanEnvelope[i]->frame - 6/2, m_PanEnvelope[i]->value  - 6/2, 6, 6 );
	}


	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::SolidLine ) );
	painter.drawLine( m_nLocator, 4, m_nLocator, height() -4);

	//volume line
	//first rect
	painter.setPen( QPen(QColor( 255, 255, 255, 200 ) ,1 , Qt::SolidLine) );
	painter.setBrush(QColor( 99, 160, 233 ));
	painter.drawRect ( m_VelocityEnvelope[0]->frame - 12/2, m_VelocityEnvelope[0]->value  - 6/2, 12, 6 );
	//last rect
	painter.drawRect ( m_VelocityEnvelope[m_VelocityEnvelope.size() -1]->frame - 12/2, m_VelocityEnvelope[m_VelocityEnvelope.size() -1]->value  - 6/2, 12, 6 );

	//pan line
	//first rect
	painter.setPen( QPen(QColor( 249, 235, 116, 200 ) ,1 , Qt::SolidLine) );
	painter.setBrush(QColor( 77, 189, 55 ));
	painter.drawRect ( m_PanEnvelope[0]->frame - 12/2, m_PanEnvelope[0]->value  - 6/2, 12, 6 );
	//last rect
	painter.drawRect ( m_PanEnvelope[m_PanEnvelope.size() -1]->frame - 12/2, m_PanEnvelope[m_PanEnvelope.size() -1]->value  - 6/2, 12, 6 );


	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 0, LCenter, UI_WIDTH, LCenter );
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 0, RCenter, UI_WIDTH, RCenter );

	if (m_nY < 50){	
		if (m_nX < 790){
			painter.drawText( m_nX +5, m_nY, 60, 20, Qt::AlignLeft, QString( m_sInfo ) );
		}
		else
		{
			painter.drawText( m_nX - 65, m_nY, 60, 20, Qt::AlignRight, QString( m_sInfo ) );
		}
		
	}else
	{
		if (m_nX < 790){
			painter.drawText( m_nX +5, m_nY -20, 60, 20, Qt::AlignLeft, QString( m_sInfo ) );
		}
		else
		{
			painter.drawText( m_nX - 65, m_nY -20, 60, 20, Qt::AlignRight, QString( m_sInfo ) );
		}
	}

}


void TargetWaveDisplay::updateDisplayPointer()
{
	update();
}

void TargetWaveDisplay::paintLocatorEventTargetDisplay( int pos, bool updateposi)
{
	m_UpdatePosition = updateposi;
	if ( !updateposi ){
		m_nLocator = -1;
	}
	else
	{
		m_nLocator = pos;
	}
	update();
}

void TargetWaveDisplay::updateDisplay( H2Core::InstrumentLayer *pLayer )
{
	if ( pLayer && pLayer->get_sample() ) {

		int nSampleLength = pLayer->get_sample()->get_frames();
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
			m_pPeakData_Left[ i ] = nVall;
			m_pPeakData_Right[ i ] = nValr;
		}
	}

	update();

}


void TargetWaveDisplay::mouseMoveEvent(QMouseEvent *ev)
{
	int snapradius = 10;
	int editType = HydrogenApp::get_instance()->getSampleEditor()->EditTypeComboBox->currentIndex();


	///edit volume points
	if( editType == 0 ){
		m_VMove = true;

		if ( ev->x() <= 0 || ev->x() >= UI_WIDTH || ev->y() < 0 || ev->y() > UI_HEIGHT ){
			update();
			m_VMove = false;
			return;
		}
		float info = (UI_HEIGHT - ev->y()) / (float)UI_HEIGHT;
		m_sInfo.setNum( info, 'g', 2 );
		m_nX = ev->x();
		m_nY = ev->y();

		for ( int i = 0; i < static_cast<int>(m_VelocityEnvelope.size()); i++){
			if ( m_VelocityEnvelope[i]->frame >= ev->x() - snapradius && m_VelocityEnvelope[i]->frame <= ev->x() + snapradius ) {
				m_VelocityEnvelope.erase( m_VelocityEnvelope.begin() + i);
				int Frame = 0;
				int Value = 0;
				
				if ( i == 0 ){
					Frame = 0;
					Value = ev->y();
				} else if ( i == static_cast<int>(m_VelocityEnvelope.size()) ) {
					Frame = m_VelocityEnvelope[i]->frame;
					Value = ev->y();
				} else {
					Frame = ev->x();
					Value = ev->y();
				}
				m_VelocityEnvelope.push_back( std::make_unique<EnvelopePoint>( Frame, Value) );
				sort( m_VelocityEnvelope.begin(), m_VelocityEnvelope.end(), EnvelopePoint::Comparator() );
				update();
				return;
			}else
			{
				m_VMove = false;
			}
		}
		///edit panorama points
	}else if( editType == 1 ){
		m_VMove = true;

		if ( ev->x() <= 0 || ev->x() >= UI_WIDTH || ev->y() < 0 || ev->y() > UI_HEIGHT ){
			update();
			m_VMove = false;
			return;
		}
		float info = (UI_HEIGHT/2 - ev->y()) / (UI_HEIGHT/2.0);
		m_sInfo.setNum( info, 'g', 2 );
		m_nX = ev->x();
		m_nY = ev->y();

		for ( int i = 0; i < static_cast<int>(m_PanEnvelope.size()); i++){
			if ( m_PanEnvelope[i]->frame >= ev->x() - snapradius && m_PanEnvelope[i]->frame <= ev->x() + snapradius ) {
				m_PanEnvelope.erase( m_PanEnvelope.begin() + i);
				int Frame = 0;
				int Value = 0;
				if ( i == 0 ){
					Frame = 0;
					Value = ev->y();
				} else if ( i == static_cast<int>(m_PanEnvelope.size()) ) {
					Frame = m_PanEnvelope[i]->frame;
					Value = ev->y();
				} else {
					Frame = ev->x();
					Value = ev->y();
				}
				m_PanEnvelope.push_back( std::make_unique<EnvelopePoint>(Frame, Value) );
				sort( m_PanEnvelope.begin(), m_PanEnvelope.end(), EnvelopePoint::Comparator() );
				update();
				return;
			}else
			{
				m_VMove = false;
			}
		}
	}

	update();
	HydrogenApp::get_instance()->getSampleEditor()->setTrue();
}



void TargetWaveDisplay::mousePressEvent(QMouseEvent *ev)
{
	int SnapRadius = 6;
	bool NewPoint = true;

	// add new point
	int EditType = HydrogenApp::get_instance()->getSampleEditor()->EditTypeComboBox->currentIndex();

	///edit volume points
	if( EditType == 0 ){

		// test if there is already a point
		for ( int i = 0; i < static_cast<int>(m_VelocityEnvelope.size()); ++i){
			if ( m_VelocityEnvelope[i]->frame >= ev->x() - SnapRadius && m_VelocityEnvelope[i]->frame <= ev->x() + SnapRadius ){
				NewPoint = false;
			}
		}

		int x = ev->x();
		int y = ev->y();
		if (ev->button() == Qt::LeftButton && !m_VMove && NewPoint){
			float info = (UI_HEIGHT - ev->y()) / (float)UI_HEIGHT;
			m_sInfo.setNum( info, 'g', 2 );
			m_nX = ev->x();
			m_nY = ev->y();
			if ( ev->y() <= 0 ) y = 0;
			if ( ev->y() >= UI_HEIGHT ) y = UI_HEIGHT;
			if ( ev->x() <= SnapRadius ) x = SnapRadius;
			if ( ev->x() >= UI_WIDTH-SnapRadius ) x = UI_WIDTH-SnapRadius;
			m_VelocityEnvelope.push_back( std::make_unique<EnvelopePoint>( x, y ) );
			sort( m_VelocityEnvelope.begin(), m_VelocityEnvelope.end(), EnvelopePoint::Comparator() );
		}

		//remove point
		SnapRadius = 10;
		if (ev->button() == Qt::RightButton ){

			if ( ev->x() <= 0 || ev->x() >= UI_WIDTH ){
				update();
				return;
			}
			m_sInfo = "";

			for ( int i = 0; i < static_cast<int>(m_VelocityEnvelope.size()); i++){
				if ( m_VelocityEnvelope[i]->frame >= ev->x() - SnapRadius && m_VelocityEnvelope[i]->frame <= ev->x() + SnapRadius ){
					if ( m_VelocityEnvelope[i]->frame == 0 || m_VelocityEnvelope[i]->frame == UI_WIDTH) return;
					m_VelocityEnvelope.erase( m_VelocityEnvelope.begin() +  i);
				}
			}
		}
	}
	///edit panorama points
	else if( EditType == 1 ){
		// test if there is already a point
		for ( int i = 0; i < static_cast<int>(m_PanEnvelope.size()); ++i){
			if ( m_PanEnvelope[i]->frame >= ev->x() - SnapRadius && m_PanEnvelope[i]->frame <= ev->x() + SnapRadius ){
				NewPoint = false;
			}
		}
		int x = ev->x();
		int y = ev->y();
		if (ev->button() == Qt::LeftButton && !m_VMove && NewPoint){
			float info = (UI_HEIGHT/2 - ev->y()) / (UI_HEIGHT/2.0);
			m_sInfo.setNum( info, 'g', 2 );
			m_nX = ev->x();
			m_nY = ev->y();
			if ( ev->y() <= 0 ) y = 0;
			if ( ev->y() >= UI_HEIGHT ) y = UI_HEIGHT;
			if ( ev->x() <= SnapRadius ) x = SnapRadius;
			if ( ev->x() >= UI_WIDTH-SnapRadius ) x = UI_WIDTH-SnapRadius;
			m_PanEnvelope.push_back( std::make_unique<EnvelopePoint>( x, y ) );
			sort( m_PanEnvelope.begin(), m_PanEnvelope.end(), EnvelopePoint::Comparator() );
		}


		//remove point
		SnapRadius = 10;
		if (ev->button() == Qt::RightButton ){

			if ( ev->x() <= 0 || ev->x() >= UI_WIDTH ){
				update();
				return;
			}
			m_sInfo = "";

			for ( int i = 0; i < static_cast<int>(m_PanEnvelope.size()); i++){
				if ( m_PanEnvelope[i]->frame >= ev->x() - SnapRadius && m_PanEnvelope[i]->frame <= ev->x() + SnapRadius ){
					if ( m_PanEnvelope[i]->frame == 0 || m_PanEnvelope[i]->frame == UI_WIDTH) return;
					m_PanEnvelope.erase( m_PanEnvelope.begin() +  i);
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
