/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentLayer.h>

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

static TargetWaveDisplay::EnvelopeEditMode get_current_edit_mode();

const char* TargetWaveDisplay::__class_name = "TargetWaveDisplay";

TargetWaveDisplay::TargetWaveDisplay(QWidget* pParent)
 : QWidget( pParent )
 , Object( __class_name )
 , m_sSampleName( "" )
{
//	setAttribute(Qt::WA_OpaquePaintEvent);

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
	m_sInfo = "";
	m_nX = -10;
	m_nY = -10;
	m_nLocator = -1;
	m_UpdatePosition = false;
	setMouseTracking(true);
}




TargetWaveDisplay::~TargetWaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakData_Left;
	delete[] m_pPeakData_Right;
}

static void envelope_paint(Sample::VelocityEnvelope &envelope, QPainter &painter,
	bool selection, int mouseX, int VCenter, int LCenter, int RCenter,
	const QColor & lineColor, const QColor & handleColor, const QColor & selectedColor)
{
	if (envelope.empty())
		return;
	for ( int i = 0; i < static_cast<int>(envelope.size()) -1; i++){
		painter.setPen( QPen(lineColor, 1 , Qt::SolidLine) );
		painter.drawLine( envelope[i]->frame, envelope[i]->value, envelope[i + 1]->frame, envelope[i +1]->value );
		if (selection && envelope[i]->frame >= mouseX - 3 && envelope[i]->frame <= mouseX + 3)
			painter.setBrush( selectedColor );
		else
			painter.setBrush( handleColor );
		painter.drawEllipse ( envelope[i]->frame - 6/2, envelope[i]->value  - 6/2, 6, 6 );
	}
	// draw first and last points as squares
	painter.drawRect ( envelope[0]->frame - 12/2, envelope[0]->value  - 6/2, 12, 6 );
	painter.drawRect ( envelope[envelope.size() -1]->frame - 12/2, envelope[envelope.size() -1]->value  - 6/2, 12, 6 );
}

void TargetWaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );

	m_EditMode = get_current_edit_mode();

	painter.setRenderHint( QPainter::Antialiasing );
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

	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::SolidLine ) );
	painter.drawLine( m_nLocator, 4, m_nLocator, height() -4);

	QColor volumeLineColor = QColor( 255, 255, 255, 200);
	QColor volumeHandleColor = QColor( 99, 160, 233);
	QColor panLineColor = QColor( 249, 235, 116, 200 );
	QColor panHandleColor = QColor( 77, 189, 55 );
	QColor selectedtHandleColor = QColor( 255, 100, 90 );
	//volume line

	envelope_paint(m_VelocityEnvelope, painter, m_EditMode == TargetWaveDisplay::VELOCITY, m_nX,
		VCenter, LCenter, RCenter, volumeLineColor, volumeHandleColor, selectedtHandleColor);
	//pan line
	envelope_paint(m_PanEnvelope, painter, m_EditMode == TargetWaveDisplay::PAN, m_nX,
		VCenter, LCenter, RCenter, panLineColor, panHandleColor, selectedtHandleColor);

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

		auto pSampleDatal = pLayer->get_sample()->get_data_l();
		auto pSampleDatar = pLayer->get_sample()->get_data_r();
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
	m_EditMode = get_current_edit_mode();

	Sample::VelocityEnvelope & envelope = (m_EditMode == TargetWaveDisplay::VELOCITY) ? m_VelocityEnvelope : m_PanEnvelope;

	if ( ev->x() <= 0 || ev->x() >= UI_WIDTH || ev->y() < 0 || ev->y() > UI_HEIGHT ){
		update();
		return;
	}
	float info = (UI_HEIGHT - ev->y()) / (float)UI_HEIGHT;
	m_sInfo.setNum( info, 'g', 2 );
	m_nX = ev->x();
	m_nY = ev->y();

	if ( ! (ev->buttons() & Qt::LeftButton) ) {
		// we are not dragging any point
		update();
		return;
	}
	for ( int i = 0; i < static_cast<int>(envelope.size()); i++){
		if ( envelope[i]->frame >= ev->x() - snapradius && envelope[i]->frame <= ev->x() + snapradius ) {
			envelope.erase( envelope.begin() + i);
			int Frame = ev->x();
			int Value = ev->y();

			if ( i == 0 ){
				Frame = 0;
			} else if ( i == static_cast<int>(envelope.size()) ) {
				Frame = UI_WIDTH;
			}
			envelope.push_back( std::make_unique<EnvelopePoint>( Frame, Value) );
			sort( envelope.begin(), envelope.end(), EnvelopePoint::Comparator() );
			update();
			return;
		}
	}

	update();
	HydrogenApp::get_instance()->getSampleEditor()->setTrue();
}



void TargetWaveDisplay::mousePressEvent(QMouseEvent *ev)
{
	int SnapRadius = 6;
	bool NewPoint = true;
	m_EditMode = get_current_edit_mode();

	// add new point
	Sample::VelocityEnvelope & envelope = (m_EditMode == TargetWaveDisplay::VELOCITY) ? m_VelocityEnvelope : m_PanEnvelope;

	///edit envelope points

	// test if there is already a point
	for ( int i = 0; i < static_cast<int>(envelope.size()); ++i){
		if ( envelope[i]->frame >= ev->x() - SnapRadius && envelope[i]->frame <= ev->x() + SnapRadius ){
			NewPoint = false;
		}
	}

	int x = ev->x();
	int y = ev->y();
	if (ev->button() == Qt::LeftButton && NewPoint){
		float info = (UI_HEIGHT - ev->y()) / (float)UI_HEIGHT;
		m_sInfo.setNum( info, 'g', 2 );
		m_nX = ev->x();
		m_nY = ev->y();
		if ( ev->y() <= 0 ) y = 0;
		if ( ev->y() >= UI_HEIGHT ) y = UI_HEIGHT;
		if ( ev->x() <= SnapRadius ) x = SnapRadius;
		if ( ev->x() >= UI_WIDTH-SnapRadius ) x = UI_WIDTH-SnapRadius;
		envelope.push_back( std::make_unique<EnvelopePoint>( x, y ) );
		sort( envelope.begin(), envelope.end(), EnvelopePoint::Comparator() );
	}

	//remove point
	SnapRadius = 10;
	if (ev->button() == Qt::RightButton ){

		if ( ev->x() <= 0 || ev->x() >= UI_WIDTH ){
			update();
			return;
		}
		m_sInfo = "";

		for ( int i = 0; i < static_cast<int>(envelope.size()); i++){
			if ( envelope[i]->frame >= ev->x() - SnapRadius && envelope[i]->frame <= ev->x() + SnapRadius ){
				if ( envelope[i]->frame == 0 || envelope[i]->frame == UI_WIDTH) return;
				envelope.erase( envelope.begin() +  i);
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


static TargetWaveDisplay::EnvelopeEditMode get_current_edit_mode()
{
	int editType = HydrogenApp::get_instance()->getSampleEditor()->EditTypeComboBox->currentIndex();
	if (editType == 0) {
		return TargetWaveDisplay::VELOCITY;
	} else if (editType == 1) {
		return TargetWaveDisplay::PAN;
	} else {
		// combo options added
		return TargetWaveDisplay::PAN;
	}
}