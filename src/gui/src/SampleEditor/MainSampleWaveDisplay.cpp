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
#include "HydrogenApp.h"
#include "SampleEditor.h"
using namespace H2Core;

#include "MainSampleWaveDisplay.h"
#include "../Skin.h"

MainSampleWaveDisplay::MainSampleWaveDisplay(QWidget* pParent)
 : QWidget( pParent )
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
	m_nLocator = -1;
	m_bUpdatePosition = false;
	m_nSampleLength = 0;

	m_SelectedSlider = SampleEditor::Slider::NoSlider;
	m_pEditedSample = nullptr;
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

static void set_paint_color(QPainter & painter, const QColor & color, bool selected, SampleEditor::Slider which)
{
	if (!selected) {
		painter.setPen( color );
	} else {
		QColor highlight = QColor(std::min(255, color.red() + 20 + 20 * (which == SampleEditor::EndSlider)),
				std::min(255, color.green() + 20 + 20 * (which == SampleEditor::StartSlider)),
				std::min(255, color.blue() + 20 + 20 * (which == SampleEditor::LoopSlider)));

		painter.setPen ( highlight );
	}
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

	QColor startColor = QColor( 32, 173, 0, 200 );
	QColor endColor = QColor( 217, 68, 0, 200 );
	QColor loopColor =  QColor( 93, 170, 254, 200 );
	font.setWeight( 63 );
	painter.setFont( font );
//start frame pointer
	set_paint_color(painter, startColor, m_SelectedSlider == SampleEditor::Slider::StartSlider, m_SelectedSlider);
	painter.drawLine( m_nStartFramePosition, 4, m_nStartFramePosition, height() -4 );
	painter.drawText( m_nStartFramePosition -10, 250, 10,20, Qt::AlignRight, "S" );
//endframe pointer
	set_paint_color(painter, endColor, m_SelectedSlider == SampleEditor::Slider::EndSlider, m_SelectedSlider);
	painter.drawLine( m_nEndFramePosition, 4, m_nEndFramePosition, height() -4 );
	painter.drawText( m_nEndFramePosition -10, 123, 10, 20, Qt::AlignRight, "E" );
//loopframe pointer
	set_paint_color(painter, loopColor, m_SelectedSlider == SampleEditor::Slider::LoopSlider, m_SelectedSlider);
	painter.drawLine( m_nLoopFramePosition, 4, m_nLoopFramePosition, height() -4 );
	painter.drawText( m_nLoopFramePosition , 0, 10, 20, Qt::AlignLeft, "L" );


}



void MainSampleWaveDisplay::updateDisplayPointer()
{
	update();
}



std::shared_ptr<H2Core::Sample> MainSampleWaveDisplay::loadSampleAndUpdateDisplay( const QString& filename )
{

	auto pNewSample = Sample::load( filename );
	if ( ! pNewSample ) {
		update();
		return nullptr;
	}
	m_pEditedSample = pNewSample;
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
	update();
	return pNewSample;
}

void MainSampleWaveDisplay::mouseMoveEvent(QMouseEvent *ev)
{
	if ( ev->buttons() && Qt::LeftButton ) {
		if ( m_SelectedSlider != SampleEditor::Slider::NoSlider ) {
			testPosition( ev );
			emit sliderEdited( m_SelectedSlider );
		}
	} else {
		chooseSlider( ev );
	}
	update();
}

void MainSampleWaveDisplay::mousePressEvent(QMouseEvent *ev)
{
	chooseSlider( ev );
	if ( m_SelectedSlider != SampleEditor::Slider::NoSlider ) {
		testPosition( ev );
		emit sliderEdited( m_SelectedSlider );
	}
	update();
}

void MainSampleWaveDisplay::testPosition( QMouseEvent *ev )
{
	assert(ev);
//startframepointer
	int x = std::min(width() - 25, std::max(25, ev->x()));

	if  ( m_SelectedSlider == SampleEditor::Slider::StartSlider ) {
		m_nStartFramePosition = x;
		if ( m_nStartFramePosition > m_nLoopFramePosition ){
			m_nLoopFramePosition = m_nStartFramePosition;
		}
		if ( m_nStartFramePosition > m_nEndFramePosition ){
			m_nEndFramePosition = m_nStartFramePosition;
		}
	}

//loopframeposition
	else if  ( m_SelectedSlider == SampleEditor::Slider::LoopSlider ) {
		if (x >= m_nStartFramePosition && x <= m_nEndFramePosition ) {
			m_nLoopFramePosition = x ;
		}
	}
//endframeposition
	else if  ( m_SelectedSlider == SampleEditor::Slider::EndSlider) {
		if (x >= m_nStartFramePosition) {
			m_nEndFramePosition = x ;
		}
		if ( m_nEndFramePosition <  m_nLoopFramePosition ){
			m_nLoopFramePosition = m_nEndFramePosition;
		}
	}
}


void MainSampleWaveDisplay::mouseReleaseEvent(QMouseEvent *ev)
{
	if ( m_SelectedSlider != SampleEditor::Slider::NoSlider ) {
		emit doneEditingSlider( m_SelectedSlider );
	}
	m_SelectedSlider = SampleEditor::Slider::NoSlider;
	update();
}




void MainSampleWaveDisplay::chooseSlider(QMouseEvent * ev)
{
	int ds = std::abs(ev->x() - m_nStartFramePosition);
	int de = std::abs(ev->x() - m_nEndFramePosition);
	int dl = std::abs(ev->x() - m_nLoopFramePosition);

	if (ds < de && ds < dl) {
		m_SelectedSlider = SampleEditor::Slider::StartSlider;
	} else if (de < ds && de < dl) {
		m_SelectedSlider = SampleEditor::Slider::EndSlider;
	} else if (dl < ds && dl < de) {
		m_SelectedSlider = SampleEditor::Slider::LoopSlider;
	} else {
		QPoint start = QPoint(m_nStartFramePosition, height());
		QPoint end = QPoint(m_nEndFramePosition, height() / 2);
		QPoint loop = QPoint(m_nLoopFramePosition, 0);

		ds = (ev->pos() - start).manhattanLength();
		de = (ev->pos() - end).manhattanLength();
		dl = (ev->pos() - loop).manhattanLength();
		if (ds <= de && ds <= dl) {
			m_SelectedSlider = SampleEditor::Slider::StartSlider;
		} else if (de < ds && de <= dl) {
			m_SelectedSlider = SampleEditor::Slider::EndSlider;
		} else if (dl < ds && dl < de) {
			m_SelectedSlider = SampleEditor::Slider::LoopSlider;
		} else {
			m_SelectedSlider = SampleEditor::Slider::NoSlider; // should never occur
		}
	}
}

