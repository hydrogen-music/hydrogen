/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "TargetWaveDisplay.h"

#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentLayer.h>

#include <memory>

#include "../Compatibility/MouseEvent.h"
#include "HydrogenApp.h"
#include "SampleEditor.h"

using namespace H2Core;

#define UI_WIDTH   841
#define UI_HEIGHT   91

#include <vector>
#include <algorithm>
#include "../Skin.h"

static TargetWaveDisplay::EnvelopeEditMode getEnvelopeEditMode();

TargetWaveDisplay::TargetWaveDisplay(QWidget* pParent)
 : QWidget( pParent )
 , m_sSampleName( "" )
{
//	setAttribute(Qt::WA_OpaquePaintEvent);

	//
	int w = UI_WIDTH;
	int h = UI_HEIGHT;
	resize( w, h );

	bool ok = m_Background.load( Skin::getImagePath() + "/waveDisplay/targetsamplewavedisplay.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	m_EditMode = EnvelopeEditMode::VELOCITY;
	m_pPeakData_Left = new int[ w ];
	m_pPeakData_Right = new int[ w ];
	m_sInfo = "";
	m_nX = -10;
	m_nY = -10;
	m_nLocator = -1;
	m_UpdatePosition = false;
	m_nSelectedEnvelopePoint = -1;
	m_nSnapRadius = 6;
	setMouseTracking(true);
}




TargetWaveDisplay::~TargetWaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakData_Left;
	delete[] m_pPeakData_Right;
}

static void paintEnvelope(Sample::VelocityEnvelope &envelope, QPainter &painter,
	int selected, const QColor & lineColor, const QColor & handleColor, const QColor & selectedColor)
{
	if (envelope.empty()) {
		return;
	}
	
	for ( int i = 0; i < static_cast<int>(envelope.size()) -1; i++){
		painter.setPen( QPen(lineColor, 1 , Qt::SolidLine) );
		painter.drawLine( envelope[i].frame, envelope[i].value, envelope[i + 1].frame, envelope[i +1].value );
		if ( i == selected ) {
			painter.setBrush( selectedColor );
		} else {
			painter.setBrush( handleColor );
		}
		painter.drawEllipse ( envelope[i].frame - 6/2, envelope[i].value  - 6/2, 6, 6 );
	}
	
	// draw first and last points as squares
	if ( 0 == selected ) {
		painter.setBrush( selectedColor );
	} else {
		painter.setBrush( handleColor );
	}
	painter.drawRect ( envelope[0].frame - 12/2, envelope[0].value  - 6/2, 12, 6 );

	if ( envelope.size() - 1 == selected ) {
		painter.setBrush( selectedColor );
	} else {
		painter.setBrush( handleColor );
	}
	painter.drawRect ( envelope[envelope.size() -1].frame - 12/2, envelope[envelope.size() -1].value  - 6/2, 12, 6 );
}

void TargetWaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );

	m_EditMode = getEnvelopeEditMode();

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

	QFont font;
	font.setWeight( QFont::Bold );
	painter.setFont( font );

	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::SolidLine ) );
	painter.drawLine( m_nLocator, 4, m_nLocator, height() -4);

	QColor volumeLineColor = QColor( 255, 255, 255, 200);
	QColor volumeHandleColor = QColor( 99, 160, 233);
	QColor panLineColor = QColor( 249, 235, 116, 200 );
	QColor panHandleColor = QColor( 77, 189, 55 );
	QColor selectedtHandleColor = QColor( 255, 100, 90 );
	//volume line

	paintEnvelope(m_VelocityEnvelope, painter, m_EditMode == TargetWaveDisplay::VELOCITY ? m_nSelectedEnvelopePoint : -1,
		volumeLineColor, volumeHandleColor, selectedtHandleColor);
	//pan line
	paintEnvelope(m_PanEnvelope, painter, m_EditMode == TargetWaveDisplay::PAN ? m_nSelectedEnvelopePoint : -1,
		panLineColor, panHandleColor, selectedtHandleColor);

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

void TargetWaveDisplay::updateDisplay( std::shared_ptr<H2Core::InstrumentLayer> pLayer )
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

void TargetWaveDisplay::updateMouseSelection(QMouseEvent *ev)
{
	auto pEv = static_cast<MouseEvent*>( ev );

	m_EditMode = getEnvelopeEditMode();
	const Sample::VelocityEnvelope & envelope = (m_EditMode == TargetWaveDisplay::VELOCITY) ? m_VelocityEnvelope : m_PanEnvelope;

	m_nX = std::min(UI_WIDTH, std::max(0, static_cast<int>(pEv->position().x())));
	m_nY = std::min(UI_HEIGHT, std::max(0, static_cast<int>(pEv->position().y())));

	if ( !(ev->buttons() & Qt::LeftButton) || m_nSelectedEnvelopePoint == -1) {
		QPoint mousePoint(m_nX, m_nY);
		int selection = -1;
		int min_distance = 1000000;
		for ( int i = 0; i < static_cast<int>(envelope.size()); i++){
			if ( envelope[i].frame >= m_nX - m_nSnapRadius && envelope[i].frame <= m_nX + m_nSnapRadius ) {
				QPoint envelopePoint(envelope[i].frame, envelope[i].value);
				int delta = (mousePoint - envelopePoint).manhattanLength();
				if (delta < min_distance) {
					min_distance = delta;
					selection = i;
				}
			}
		}
		m_nSelectedEnvelopePoint = selection;
	}
	if (m_nSelectedEnvelopePoint == -1) {
		m_sInfo = "";
	} else {
		float info = (UI_HEIGHT - envelope[m_nSelectedEnvelopePoint].value) / (float)UI_HEIGHT;
		m_sInfo.setNum( info, 'g', 2 );
	}
}

void TargetWaveDisplay::updateEnvelope()
{
	if ( m_nSelectedEnvelopePoint == -1 ) {
		return;
	}
	m_EditMode = getEnvelopeEditMode();
	Sample::VelocityEnvelope & envelope = (m_EditMode == TargetWaveDisplay::VELOCITY) ? m_VelocityEnvelope : m_PanEnvelope;
	envelope.erase( envelope.begin() + m_nSelectedEnvelopePoint );
	if ( m_nSelectedEnvelopePoint == 0 ){
		m_nX = 0;
	} else if ( m_nSelectedEnvelopePoint == static_cast<int>(envelope.size()) ) {
		m_nX = UI_WIDTH;
	}
	envelope.push_back(  EnvelopePoint(  m_nX, m_nY  )  );
	sort( envelope.begin(), envelope.end(), EnvelopePoint::Comparator() );
	for (int i = 0; i < envelope.size() - 1; ++i) {
		if (envelope[i].frame == envelope[i+1].frame) {
			envelope.erase( envelope.begin() + i);
			if (i + 1 == m_nSelectedEnvelopePoint) {
				m_nSelectedEnvelopePoint = i;
			}
		}
	}
}

void TargetWaveDisplay::mouseMoveEvent(QMouseEvent *ev)
{
	updateMouseSelection(ev);

	if ( ! (ev->buttons() & Qt::LeftButton) ) {
		// we are not dragging any point
		update();
		return;
	}
	updateEnvelope();
	updateMouseSelection(ev);
	update();
	HydrogenApp::get_instance()->getSampleEditor()->setUnclean();
}



void TargetWaveDisplay::mousePressEvent(QMouseEvent *ev)
{
	m_EditMode = getEnvelopeEditMode();
	Sample::VelocityEnvelope & envelope = (m_EditMode == TargetWaveDisplay::VELOCITY) ? m_VelocityEnvelope : m_PanEnvelope;

	updateMouseSelection(ev);

	if (ev->button() == Qt::LeftButton) {
		// add or move point
		bool NewPoint = false;

		if ( m_nSelectedEnvelopePoint == -1 ) {
			NewPoint = true;
		}

		if (NewPoint){
			if (envelope.empty()) {
				envelope.push_back(  EnvelopePoint( 0, m_nY )  );
				envelope.push_back(  EnvelopePoint( UI_WIDTH, m_nY ) );
			} else {
				envelope.push_back(  EnvelopePoint(  m_nX, m_nY  )  );
			}
			sort( envelope.begin(), envelope.end(), EnvelopePoint::Comparator() );
		} else {
			// move old point to new position
			updateEnvelope();
		}
	} else if (ev->button() == Qt::RightButton ) {
		//remove point

		if ( m_nSelectedEnvelopePoint == -1 ||
			 envelope.size() > 2 &&
			 (m_nSelectedEnvelopePoint == 0 || m_nSelectedEnvelopePoint == envelope.size() - 1) ) {
			// do nothing if no point is selected
			// don't remove first or last point if more than 2 points in envelope
			update();
			return;
		} else if (envelope.size() == 2) {
			// if only 2 points, remove them both
			envelope.clear();
		} else {
			envelope.erase( envelope.begin() +  m_nSelectedEnvelopePoint );
		}
	}

	updateMouseSelection(ev);
	update();
	HydrogenApp::get_instance()->getSampleEditor()->setUnclean();
}





void TargetWaveDisplay::mouseReleaseEvent(QMouseEvent *ev)
{
	updateMouseSelection(ev);
	update();
	HydrogenApp::get_instance()->getSampleEditor()->returnAllTargetDisplayValues();
}


static TargetWaveDisplay::EnvelopeEditMode getEnvelopeEditMode()
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
