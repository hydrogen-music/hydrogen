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

#include "SampleWaveDisplay.h"

#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Sample.h>

#include "../Compatibility/MouseEvent.h"

using namespace H2Core;

SampleWaveDisplay::SampleWaveDisplay(
	SampleEditor* pParent,
	WaveDisplay::Channel channel
)
	: WaveDisplay( pParent, channel ), m_pSampleEditor( pParent )
{
	setFixedSize( SampleWaveDisplay::nWidth, SampleWaveDisplay::nHeight );

	m_label = WaveDisplay::Label::Fallback;
	m_sFallbackLabel = "";

	setMouseTracking( true );
}

SampleWaveDisplay::~SampleWaveDisplay()
{
}

static void set_paint_color(
	QPainter& painter,
	const QColor& color,
	bool selected,
	SampleEditor::Slider which
)
{
	if ( !selected ) {
		painter.setPen( color );
	}
	else {
		QColor highlight = QColor(
			std::min(
				255,
				color.red() + 20 + 20 * ( which == SampleEditor::Slider::End )
			),
			std::min(
				255, color.green() + 20 +
						 20 * ( which == SampleEditor::Slider::Start )
			),
			std::min(
				255,
				color.blue() + 20 + 20 * ( which == SampleEditor::Slider::Loop )
			)
		);

		painter.setPen( highlight );
	}
}

void SampleWaveDisplay::paintEvent( QPaintEvent* ev )
{
	if ( !isVisible() ) {
		return;
	}

	WaveDisplay::paintEvent( ev );

	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing );

	// Render playhead
	p.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::SolidLine ) );
	p.drawLine(
		m_pSampleEditor->getFramePosition(), 4,
		m_pSampleEditor->getFramePosition(), height() - 4
	);

	// Render loop sliders
	QColor startColor = QColor( 32, 173, 0, 200 );
	QColor endColor = QColor( 217, 68, 0, 200 );
	QColor loopColor = QColor( 93, 170, 254, 200 );
	QFont font;
	font.setWeight( QFont::Bold );
	p.setFont( font );

	const auto nStart = frameToX( m_pSampleEditor->getLoopStartFrame() );
	set_paint_color(
		p, startColor,
		m_pSampleEditor->getSelectedSlider() == SampleEditor::Slider::Start,
		m_pSampleEditor->getSelectedSlider()
	);
	p.drawLine( nStart, 4, nStart, height() - 4 );
	p.drawText( nStart, 0, 10, 20, Qt::AlignRight, "S" );

	const auto nLoop = frameToX( m_pSampleEditor->getLoopLoopFrame() );
	set_paint_color(
		p, loopColor,
		m_pSampleEditor->getSelectedSlider() == SampleEditor::Slider::Loop,
		m_pSampleEditor->getSelectedSlider()
	);
	p.drawLine( nLoop, 4, nLoop, height() - 4 );
	p.drawText( nLoop, height() / 2, 10, 20, Qt::AlignLeft, "L" );

	const auto nEnd = frameToX( m_pSampleEditor->getLoopEndFrame() );
	set_paint_color(
		p, endColor,
		m_pSampleEditor->getSelectedSlider() == SampleEditor::Slider::End,
		m_pSampleEditor->getSelectedSlider()
	);
	p.drawLine( nEnd, 4, nEnd, height() - 4 );
	p.drawText( nEnd - 10, height() - 30, 10, 20, Qt::AlignRight, "E" );
}

void SampleWaveDisplay::mouseMoveEvent( QMouseEvent* ev )
{
	if ( !( ev->buttons() & Qt::LeftButton ) ) {
		return;
	}
	auto pEv = static_cast<MouseEvent*>( ev );
	const int nFrame = xToFrame(
		std::clamp( static_cast<int>( pEv->position().x() ), 0, width() )
	);
	switch ( m_pSampleEditor->getSelectedSlider() ) {
		case SampleEditor::Slider::Start:
			m_pSampleEditor->setLoopStartFrame( nFrame );
			break;
		case SampleEditor::Slider::Loop:
			m_pSampleEditor->setLoopLoopFrame( nFrame );
			break;
		case SampleEditor::Slider::End:
			m_pSampleEditor->setLoopEndFrame( nFrame );
			break;
		case SampleEditor::Slider::None:
			// TODO
			DEBUGLOG( "not handled yet" );
	}
}

void SampleWaveDisplay::mousePressEvent( QMouseEvent* ev )
{
	auto pEv = static_cast<MouseEvent*>( ev );

	const QPoint start =
		QPoint( frameToX( m_pSampleEditor->getLoopStartFrame() ), height() );
	const QPoint end = QPoint(
		frameToX( m_pSampleEditor->getLoopStartFrame() ), height() / 2
	);
	const QPoint loop =
		QPoint( frameToX( m_pSampleEditor->getLoopStartFrame() ), 0 );

	const int nDistanceStartHandle =
		( pEv->position() - start ).manhattanLength();
	const int nDistanceLoopHandle =
		( pEv->position() - loop ).manhattanLength();
	const int nDistanceEndHandle = ( pEv->position() - end ).manhattanLength();

	if ( nDistanceStartHandle <= nDistanceEndHandle &&
		 nDistanceStartHandle <= nDistanceLoopHandle ) {
		m_pSampleEditor->setSelectedSlider( SampleEditor::Slider::Start );
	}
	else if ( nDistanceLoopHandle < nDistanceStartHandle && nDistanceLoopHandle < nDistanceEndHandle ) {
		m_pSampleEditor->setSelectedSlider( SampleEditor::Slider::Loop );
	}
	else if ( nDistanceEndHandle < nDistanceStartHandle && nDistanceEndHandle <= nDistanceLoopHandle ) {
		m_pSampleEditor->setSelectedSlider( SampleEditor::Slider::End );
	}
	else {
		m_pSampleEditor->setSelectedSlider( SampleEditor::Slider::None );
	}
}

int SampleWaveDisplay::frameToX( int nFrame ) const
{
	return nFrame * width() / m_pLayer->getSample()->getFrames();
}

int SampleWaveDisplay::xToFrame( int nX ) const
{
	return nX * m_pLayer->getSample()->getFrames() / width();
}
