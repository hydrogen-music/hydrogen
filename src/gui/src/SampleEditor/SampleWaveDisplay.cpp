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
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../UndoActions.h"
#include "../Widgets/EditorDefs.h"

using namespace H2Core;

SampleWaveDisplay::SampleWaveDisplay(
	SampleEditor* pParent,
	WaveDisplay::Channel channel
)
	: WaveDisplay( pParent, channel ),
	  m_pSampleEditor( pParent ),
	  m_bSlidersLocked( false ),
	  m_nOldFrame( 0 ),
	  m_bDragStarted( false )
{
	setFixedSize( SampleWaveDisplay::nWidth, SampleWaveDisplay::nHeight );
	setMouseTracking( true );

	m_label = WaveDisplay::Label::Fallback;
	m_sFallbackLabel = "";
	m_bRenderPlayhead = true;
}

SampleWaveDisplay::~SampleWaveDisplay()
{
}

void SampleWaveDisplay::leaveEvent( QEvent* pEv )
{
	m_pSampleEditor->setHoveredSlider( SampleEditor::Slider::None );
}

void SampleWaveDisplay::mouseMoveEvent( QMouseEvent* ev )
{
	if ( m_bSlidersLocked ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );
	const auto slider = intersectWith( pEv->position() );
	if ( !( ev->buttons() & Qt::LeftButton ) ||
		 ( !m_bDragStarted && slider == SampleEditor::Slider::None ) ) {
		if ( slider != m_pSampleEditor->getHoveredSlider() ) {
			m_pSampleEditor->setHoveredSlider( slider );
		}
		return;
	}
	const long long nFrame = xToFrame(
		std::clamp( static_cast<int>( pEv->position().x() ), 0, width() - 1 )
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
			return;
	}
}

void SampleWaveDisplay::mousePressEvent( QMouseEvent* ev )
{
	if ( m_bSlidersLocked ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );
	if ( !( ev->buttons() & Qt::LeftButton ) ) {
		return;
	}

	m_pSampleEditor->setSelectedSlider( intersectWith( pEv->position() ) );
	m_nOldFrame = xToFrame(
		std::clamp( static_cast<int>( pEv->position().x() ), 0, width() - 1 )
	);
	m_bDragStarted = true;
}

void SampleWaveDisplay::mouseReleaseEvent( QMouseEvent* ev )
{
	if ( m_bSlidersLocked ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );
	auto nNewFrame = xToFrame(
		std::clamp( static_cast<int>( pEv->position().x() ), 0, width() - 1 )
	);

	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_changeSliderAction(
			m_pSampleEditor->getSelectedSlider(), m_nOldFrame, nNewFrame
		),
		QString( "SampleEditor::slider::%1" )
			.arg( SampleEditor::SliderToQString(
				m_pSampleEditor->getSelectedSlider()
			) )
	);

	m_bDragStarted = false;
}

void SampleWaveDisplay::paintEvent( QPaintEvent* ev )
{
	if ( !isVisible() ) {
		return;
	}

	m_nPlayheadX = frameToX( static_cast<long long>(
		std::round( m_pSampleEditor->getPlayheadMain() )
	) );

	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	WaveDisplay::paintEvent( ev );

	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing );

	renderSlider( &p, SampleEditor::Slider::Start );
	renderSlider( &p, SampleEditor::Slider::Loop );
	renderSlider( &p, SampleEditor::Slider::End );
}

SampleEditor::Slider SampleWaveDisplay::intersectWith( const QPointF& point
) const
{
	// The handle of a slider has higher priority than the rod of the others.
	// The vertical space between the handles is big enough for them to not
	// fighting for hovering.
	if ( m_channel == WaveDisplay::Channel::Left ) {
		QRectF startHandleRect(
			std::max(
				0, frameToX( m_pSampleEditor->getLoopStartFrame() ) -
					   Editor::nDefaultCursorMargin
			),
			std::max(
				0, SampleWaveDisplay::nHandleMargin +
					   SampleWaveDisplay::nHandleSlope -
					   Editor::nDefaultCursorMargin
			),
			SampleWaveDisplay::nHandleWidth + 2 * Editor::nDefaultCursorMargin,
			SampleWaveDisplay::nHandleHeight +
				2 * SampleWaveDisplay::nHandleSlope +
				2 * Editor::nDefaultCursorMargin
		);
		if ( startHandleRect.contains( point ) ) {
			return SampleEditor::Slider::Start;
		}

		QRectF loopHandleRect(
			std::max(
				0, frameToX( m_pSampleEditor->getLoopLoopFrame() ) -
					   Editor::nDefaultCursorMargin
			),
			std::min(
				height(), height() - SampleWaveDisplay::nHandleMargin -
							  3 * SampleWaveDisplay::nHandleSlope -
							  SampleWaveDisplay::nHandleHeight -
							  Editor::nDefaultCursorMargin
			),
			SampleWaveDisplay::nHandleWidth + 2 * Editor::nDefaultCursorMargin,
			SampleWaveDisplay::nHandleHeight +
				2 * SampleWaveDisplay::nHandleSlope +
				2 * Editor::nDefaultCursorMargin
		);
		if ( loopHandleRect.contains( point ) ) {
			return SampleEditor::Slider::Loop;
		}
	}
	else {
		QRectF endHandleRect(
			std::max(
				0, frameToX( m_pSampleEditor->getLoopEndFrame() ) -
					   SampleWaveDisplay::nHandleWidth -
					   Editor::nDefaultCursorMargin
			),
			std::min(
				height(), height() - SampleWaveDisplay::nHandleMargin -
							  3 * SampleWaveDisplay::nHandleSlope -
							  SampleWaveDisplay::nHandleHeight -
							  Editor::nDefaultCursorMargin
			),
			SampleWaveDisplay::nHandleWidth + 2 * Editor::nDefaultCursorMargin,
			SampleWaveDisplay::nHandleHeight +
				2 * SampleWaveDisplay::nHandleSlope +
				2 * Editor::nDefaultCursorMargin
		);
		if ( endHandleRect.contains( point ) ) {
			return SampleEditor::Slider::End;
		}
	}

	const int nDistanceStart = std::abs(
		frameToX( m_pSampleEditor->getLoopStartFrame() ) - point.x()
	);
	const int nDistanceLoop =
		std::abs( frameToX( m_pSampleEditor->getLoopLoopFrame() ) - point.x() );
	const int nDistanceEnd =
		std::abs( frameToX( m_pSampleEditor->getLoopEndFrame() ) - point.x() );

	if ( nDistanceStart < nDistanceLoop && nDistanceStart < nDistanceEnd ) {
		if ( nDistanceStart < Editor::nDefaultCursorMargin ) {
			return SampleEditor::Slider::Start;
		}
	}
	else if ( nDistanceLoop < nDistanceStart && nDistanceLoop < nDistanceEnd ) {
		if ( nDistanceLoop < Editor::nDefaultCursorMargin ) {
			return SampleEditor::Slider::Loop;
		}
	}
	else if ( nDistanceEnd < nDistanceStart && nDistanceEnd < nDistanceLoop ) {
		if ( nDistanceEnd < Editor::nDefaultCursorMargin ) {
			return SampleEditor::Slider::End;
		}
	}

	return SampleEditor::Slider::None;
}

void SampleWaveDisplay::renderSlider(
	QPainter* pPainter,
	SampleEditor::Slider slider
)
{
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	QFont font;
	font.setWeight( QFont::Bold );
	pPainter->setFont( font );

	int nX, nHandleX, nHandleY;
	int nHandleWidth = SampleWaveDisplay::nHandleWidth;
	bool bRenderHandle = false;
	QColor color;
	QString sLabel;
	switch ( slider ) {
		case SampleEditor::Slider::Start:
			nX = frameToX( m_pSampleEditor->getLoopStartFrame() );
			bRenderHandle = m_channel == WaveDisplay::Channel::Left;
			nHandleX = nX;
			nHandleY = SampleWaveDisplay::nHandleMargin +
					   SampleWaveDisplay::nHandleSlope;
			color = pColorTheme->m_sampleEditor_startSliderColor;
			/*: Single character used as a label of the loop start slider within
			 *  the sample editor. */
			sLabel = tr( "S" );
			break;
		case SampleEditor::Slider::Loop:
			nX = frameToX( m_pSampleEditor->getLoopLoopFrame() );
			bRenderHandle = m_channel == WaveDisplay::Channel::Left;
			nHandleX = nX;
			nHandleY = height() - SampleWaveDisplay::nHandleMargin -
					   SampleWaveDisplay::nHandleHeight -
					   SampleWaveDisplay::nHandleSlope * 2;
			color = pColorTheme->m_sampleEditor_loopSliderColor;
			/*: Single character used as a label of the loop onset slider within
			 *  the sample editor. */
			sLabel = tr( "L" );
			break;
		case SampleEditor::Slider::End:
			nX = frameToX( m_pSampleEditor->getLoopEndFrame() );
			bRenderHandle = m_channel == WaveDisplay::Channel::Right;
			nHandleX = nX - SampleWaveDisplay::nHandleWidth;
			nHandleY = height() - SampleWaveDisplay::nHandleMargin -
					   SampleWaveDisplay::nHandleHeight -
					   SampleWaveDisplay::nHandleSlope * 2;
			color = pColorTheme->m_sampleEditor_endSliderColor;
			/*: Single character used as a label of the loop end slider within
			 *  the sample editor. */
			sLabel = tr( "E" );

			// left leaning
			nHandleWidth = -1 * SampleWaveDisplay::nHandleWidth;
			break;
		case SampleEditor::Slider::None:
			return;
	}

	color.setAlpha( SampleEditor::nColorAlpha );

	if ( m_bSlidersLocked ) {
		color = Skin::makeWidgetColorInactive( color );
	}

	QColor colorHovered( color );
	colorHovered.setAlpha( SampleWaveDisplay::nHoveredAlpha );

	// Draw slider
	if ( slider == m_pSampleEditor->getHoveredSlider() ) {
		pPainter->setPen( QPen( colorHovered, SampleWaveDisplay::nHoveredHalo )
		);
		pPainter->drawLine( nX, 0, nX, height() );
	}
	pPainter->setPen( color );
	pPainter->drawLine( nX, 0, nX, height() );

	// Draw slider handle
	if ( bRenderHandle ) {
		const QPointF handlePoints[4] = {
			QPointF(
				nX, std::max( 0, nHandleY - SampleWaveDisplay::nHandleSlope )
			),
			QPointF( nX + nHandleWidth, std::max( 0, nHandleY ) ),
			QPointF(
				nX + nHandleWidth,
				std::max( 0, nHandleY + SampleWaveDisplay::nHandleHeight )
			),
			QPointF(
				nX, std::min(
						height(), nHandleY + SampleWaveDisplay::nHandleHeight +
									  SampleWaveDisplay::nHandleSlope
					)
			),
		};

		if ( slider == m_pSampleEditor->getHoveredSlider() ) {
			pPainter->setPen(
				QPen( colorHovered, SampleWaveDisplay::nHoveredHalo )
			);
			pPainter->setBrush( QBrush( colorHovered ) );
			pPainter->drawPolygon( handlePoints, 4 );
		}

		pPainter->setPen( color );
		pPainter->setBrush( color );
		pPainter->drawPolygon( handlePoints, 4 );

		pPainter->setPen( Qt::black );
		pPainter->drawText(
			nHandleX, nHandleY, SampleWaveDisplay::nHandleWidth,
			SampleWaveDisplay::nHandleHeight, Qt::AlignCenter, sLabel
		);
	}
}

int SampleWaveDisplay::frameToX( long long nFrame ) const
{
	return static_cast<int>(
		static_cast<double>( nFrame ) * static_cast<double>( width() ) /
		static_cast<double>( m_pLayer->getSample()->getFrames() )
	);
}

long long SampleWaveDisplay::xToFrame( int nX ) const
{
	return static_cast<long long>(
		static_cast<double>( nX ) *
		static_cast<double>( m_pLayer->getSample()->getFrames() ) /
		static_cast<double>( width() )
	);
}
