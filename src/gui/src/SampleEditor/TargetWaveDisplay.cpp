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

#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "../CommonStrings.h"
#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../UndoActions.h"
#include "SampleEditor.h"
#include "Widgets/EditorDefs.h"

using namespace H2Core;

TargetWaveDisplay::TargetWaveDisplay( SampleEditor* pParent )
	: WaveDisplay( pParent, WaveDisplay::Channel::Left ),
	  m_pSampleEditor( pParent ),
	  m_bEnabled( true ),
	  m_sSelectedEnvelopePointValue( "" ),
	  m_pHoveredPoint( nullptr ),
	  m_pDragPoint( nullptr ),
	  m_nDragStartX( 0 ),
	  m_nDragStartY( 0 )
{
	setFixedSize( TargetWaveDisplay::nWidth, TargetWaveDisplay::nHeight );

	m_label = WaveDisplay::Label::Fallback;
	m_sFallbackLabel = "";

	m_peakDataL.resize( width() );
	m_peakDataR.resize( width() );

	setMouseTracking( true );
}

TargetWaveDisplay::~TargetWaveDisplay()
{
}

void TargetWaveDisplay::drawLine(
	QPainter& p,
	const std::vector<EnvelopePoint>& envelope,
	QColor color,
	Style style
)
{
	if ( envelope.empty() ) {
		return;
	}
	QPen pen( color );
	if ( style == Style::Background ) {
		pen.setStyle( Qt::DotLine );
	}

	p.setPen( pen );
	for ( int i = 0; i < static_cast<int>( envelope.size() ) - 1; i++ ) {
		p.drawLine(
			envelope[i].nFrame, envelope[i].nValue, envelope[i + 1].nFrame,
			envelope[i + 1].nValue
		);
	}
}

void TargetWaveDisplay::drawPoint(
	QPainter& p,
	const EnvelopePoint& point,
	QColor color,
	Style style
)
{
	QColor borderColor( Qt::black );
	QPen pen( Qt::black );
	QBrush brush( color );
	if ( style == Style::Background ) {
		pen.setStyle( Qt::DotLine );
		brush.setStyle( Qt::Dense4Pattern );
	}

	if ( style == Style::Hovered || style == Style::Selected ) {
		const auto pColorTheme = Preferences::get_instance()->getColorTheme();
		QColor highlightColor = pColorTheme->m_selectionHighlightColor;
		if ( style == Style::Hovered ) {
			if ( Skin::moreBlackThanWhite( highlightColor ) ) {
				highlightColor = highlightColor.lighter( 125 );
			}
			else {
				highlightColor = highlightColor.darker( 125 );
			}
		}

		p.setPen( highlightColor );
		p.setBrush( highlightColor );
		if ( point.nFrame == 0 || point.nFrame == TargetWaveDisplay::nWidth ) {
			p.drawRect(
				point.nFrame - TargetWaveDisplay::nPointWidth - 2,
				point.nValue - TargetWaveDisplay::nPointWidth / 2 - 2,
				TargetWaveDisplay::nPointWidth * 2 + 4,
				TargetWaveDisplay::nPointWidth + 4
			);
		}
		else {
			p.drawEllipse(
				point.nFrame - TargetWaveDisplay::nPointWidth / 2 - 2,
				point.nValue - TargetWaveDisplay::nPointWidth / 2 - 2,
				TargetWaveDisplay::nPointWidth + 4,
				TargetWaveDisplay::nPointWidth + 4
			);
		}
	}

	p.setPen( pen );
	p.setBrush( brush );
	if ( point.nFrame == 0 || point.nFrame == TargetWaveDisplay::nWidth ) {
		p.drawRect(
			point.nFrame - TargetWaveDisplay::nPointWidth,
			point.nValue - TargetWaveDisplay::nPointWidth / 2,
			TargetWaveDisplay::nPointWidth * 2, TargetWaveDisplay::nPointWidth
		);
	}
	else {
		p.drawEllipse(
			point.nFrame - TargetWaveDisplay::nPointWidth / 2,
			point.nValue - TargetWaveDisplay::nPointWidth / 2,
			TargetWaveDisplay::nPointWidth, TargetWaveDisplay::nPointWidth
		);
	}
}

void TargetWaveDisplay::paintEvent( QPaintEvent* ev )
{
	if ( !isVisible() ) {
		return;
	}

	const auto pColorTheme = Preferences::get_instance()->getColorTheme();
	QColor velocityColor( pColorTheme->m_sampleEditor_velocityEnvelopeColor );
	QColor panColor( pColorTheme->m_sampleEditor_panEnvelopeColor );
	const QColor selectionColor = pColorTheme->m_selectionHighlightColor;
	QColor hoverColor;
	if ( Skin::moreBlackThanWhite( selectionColor ) ) {
		hoverColor = selectionColor.lighter( 115 );
	}
	else {
		hoverColor = selectionColor.darker( 115 );
	}

	if ( !m_bEnabled ) {
		velocityColor = Skin::makeWidgetColorInactive( velocityColor );
		panColor = Skin::makeWidgetColorInactive( panColor );
	}

	bool bVelocity = m_pSampleEditor->getEnvelopeType() ==
					 SampleEditor::EnvelopeType::Velocity;
	const auto envelopeBackground =
		bVelocity ? m_pSampleEditor->getPanEnvelope()
				  : m_pSampleEditor->getVelocityEnvelope();
	auto envelopeForeground = bVelocity ? m_pSampleEditor->getVelocityEnvelope()
										: m_pSampleEditor->getPanEnvelope();
    const auto colorForeground = bVelocity ? velocityColor : panColor;
    const auto colorBackground = bVelocity ? panColor : velocityColor;
	if ( m_pDragPoint != nullptr ) {
		const int nFrame = m_nDragStartX;

		// Make a deep pcopy of the envelope and add the new point. This is
		// expensive, but we do not want this transient point to leak into it.
		std::vector<EnvelopePoint> newEnvelope;
		for ( const auto& ppoint : envelopeForeground ) {
			if ( ppoint.nFrame != nFrame ) {
				newEnvelope.push_back( ppoint );
			}
		}
		newEnvelope.push_back(
			EnvelopePoint( m_pDragPoint->nFrame, m_pDragPoint->nValue )
		);
		sort(
			newEnvelope.begin(), newEnvelope.end(), EnvelopePoint::Comparator()
		);
		envelopeForeground = newEnvelope;
	}

	// Draw background
	WaveDisplay::paintEvent( ev );

	QPainter p( this );
	p.setRenderHint( QPainter::Antialiasing );

	// Draw playhead
	p.setPen(
		QPen( pColorTheme->m_sampleEditor_playheadColor, 1, Qt::SolidLine )
	);
	const int nTotalFrames = m_pSampleEditor->getTotalPlaybackFrames();
	int nPlayheadX;
	if ( nTotalFrames > 0 ) {
		nPlayheadX =
			m_pSampleEditor->getPlayheadTarget() * width() / nTotalFrames;
	}
	else {
		nPlayheadX = 0;
	}
	p.drawLine( nPlayheadX, 4, nPlayheadX, height() - 4 );

	// Draw envelopes (start with the background one)
	drawLine( p, envelopeBackground, colorBackground, Style::Background );
	for ( const auto& ppoint : envelopeBackground ) {
		drawPoint( p, ppoint, colorBackground, Style::Background );
	}

	drawLine( p, envelopeForeground, colorForeground, Style::None );
	for ( const auto& ppoint : envelopeForeground ) {
		drawPoint( p, ppoint, colorForeground, Style::None );
	}

	// Draw highlight of dragged or hovered note
	if ( m_pDragPoint != nullptr ) {
		drawPoint( p, *m_pDragPoint.get(), colorForeground, Style::Selected );
	}
	else if ( m_pHoveredPoint != nullptr ) {
		// Since the dragged point is always hovered, we can separate the two.
		drawPoint( p, *m_pHoveredPoint.get(), colorForeground, Style::Hovered );
	}

	if ( m_pDragPoint != nullptr || m_pHoveredPoint != nullptr ) {
		QPoint point;
		if ( m_pDragPoint != nullptr ) {
			point = QPoint( m_pDragPoint->nFrame, m_pDragPoint->nValue );
		}
		else if ( m_pHoveredPoint != nullptr ) {
			point = QPoint( m_pHoveredPoint->nFrame, m_pHoveredPoint->nValue );
		}

		QFont font;
		font.setWeight( QFont::Bold );
		p.setFont( font );
		p.setPen( colorForeground );

		const QString sText = QString( "%1" ).arg(
			static_cast<float>( TargetWaveDisplay::nHeight - point.y() ) /
				static_cast<float>( TargetWaveDisplay::nHeight ),
			0, 'g', 2
		);

		if ( point.y() < 50 ) {
			if ( point.x() < 790 ) {
				p.drawText(
					point.x() + 5, point.y(), 60, 20, Qt::AlignLeft,
					QString( sText )
				);
			}
			else {
				p.drawText(
					point.x() - 65, point.y(), 60, 20, Qt::AlignRight,
					QString( sText )
				);
			}
		}
		else {
			if ( point.x() < 790 ) {
				p.drawText(
					point.x() + 5, point.y() - 20, 60, 20, Qt::AlignLeft,
					QString( sText )
				);
			}
			else {
				p.drawText(
					point.x() - 65, point.y() - 20, 60, 20, Qt::AlignRight,
					QString( sText )
				);
			}
		}
	}
}

void TargetWaveDisplay::drawPeakData()
{
	const qreal pixelRatio = devicePixelRatio();
	QPainter p( m_pPeakDataPixmap );
	// copy the background image
	p.drawPixmap(
		rect(), *m_pBackgroundPixmap,
		QRectF(
			pixelRatio * rect().x(), pixelRatio * rect().y(),
			pixelRatio * rect().width(), pixelRatio * rect().height()
		)
	);

	auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();

	QColor backgroundColor, waveFormColorL, waveFormColorR,
		waveFormInactiveColor;
	if ( m_pLayer != nullptr && m_pLayer->getIsMuted() ) {
		backgroundColor = pColorTheme->m_muteColor;
	}
	else if ( m_pLayer != nullptr && m_pLayer->getIsSoloed() ) {
		backgroundColor = pColorTheme->m_soloColor;
	}
	else {
		backgroundColor = pColorTheme->m_accentColor;
	}

	if ( Skin::moreBlackThanWhite( backgroundColor ) ) {
		waveFormColorL = Qt::white;
		waveFormColorR = waveFormColorL.darker( 150 );
		waveFormInactiveColor = pColorTheme->m_lightColor;
	}
	else {
		waveFormColorL = Qt::black;
		waveFormColorR = waveFormColorL.lighter( 150 );
		waveFormInactiveColor = pColorTheme->m_darkColor;
	}

	p.setPen( waveFormColorL );
	const int nVerticalCenter = height() / 2;
	if ( m_pLayer != nullptr ) {
		for ( int x = 0; x < width(); x++ ) {
			p.drawLine(
				x, nVerticalCenter, x, -m_peakDataL[x] + nVerticalCenter
			);
		}
		p.setPen( waveFormColorR );
		for ( int x = 0; x < width(); x++ ) {
			p.drawLine(
				x, nVerticalCenter, x, m_peakDataR[x] + nVerticalCenter
			);
		}
	}
	else {
		p.drawLine( 0, nVerticalCenter, width(), nVerticalCenter );
	}
}

void TargetWaveDisplay::updatePeakData()
{
	if ( width() != m_peakDataL.size() ) {
		m_peakDataL.resize( width() );
	}
	if ( width() != m_peakDataR.size() ) {
		m_peakDataR.resize( width() );
	}

	if ( m_pLayer == nullptr || m_pLayer->getSample() == nullptr ) {
		for ( int ii = 0; ii < m_peakDataL.size(); ++ii ) {
			m_peakDataL[ii] = 0;
			m_peakDataR[ii] = 0;
		}

		drawPeakData();
		update();
		return;
	}

	const int nSampleLength = m_pLayer->getSample()->getFrames();
	const auto pSampleDataL = m_pLayer->getSample()->getData_L();
	const auto pSampleDataR = m_pLayer->getSample()->getData_R();
	const float fGain = height() / 2.0 * m_pLayer->getGain();

	if ( nSampleLength > m_peakDataL.size() ) {
		const int nScaleFactor = nSampleLength / m_peakDataL.size();

		int nSamplePos = 0;
		int nValL, nValR;
		for ( int ii = 0; ii < m_peakDataL.size(); ++ii ) {
			nValL = 0;
			nValR = 0;
			for ( int jj = 0; jj < nScaleFactor; ++jj ) {
				if ( nSamplePos >= nSampleLength ) {
					break;
				}

				if ( jj < nSampleLength ) {
					const int nNewValL =
						static_cast<int>( pSampleDataL[nSamplePos] * fGain );
					const int nNewValR =
						static_cast<int>( pSampleDataR[nSamplePos] * fGain );
					if ( nNewValL > nValL ) {
						nValL = nNewValL;
					}
					if ( nNewValR > nValR ) {
						nValR = nNewValR;
					}
				}
				++nSamplePos;
			}
			m_peakDataL[ii] = std::max( nValL, 0 );
			m_peakDataR[ii] = std::max( nValR, 0 );
		}
	}
	else {
		for ( int ii = 0; ii < nSampleLength; ++ii ) {
			m_peakDataL[ii] =
				std::max( 0, static_cast<int>( pSampleDataL[ii] * fGain ) );
			m_peakDataR[ii] =
				std::max( 0, static_cast<int>( pSampleDataR[ii] * fGain ) );
		}
		for ( int ii = nSampleLength; ii < m_peakDataL.size(); ++ii ) {
			m_peakDataL[ii] = 0;
			m_peakDataR[ii] = 0;
		}
	}

	drawPeakData();
	update();
}

void TargetWaveDisplay::mouseMoveEvent( QMouseEvent* ev )
{
	if ( !m_bEnabled ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

	const auto envelope = m_pSampleEditor->getCurrentEnvelope();

	if ( m_pDragPoint == nullptr ) {
		const auto hoveredPoints = getElementsAtPoint( ev->pos() );
		if ( hoveredPoints.size() == 0 ) {
			m_pHoveredPoint = nullptr;
		}
		else {
			m_pHoveredPoint =
				std::make_shared<EnvelopePoint>( hoveredPoints[0] );
		}
		m_sSelectedEnvelopePointValue = "";
	}
	else {
		QPoint targetPoint(
			std::clamp(
				static_cast<int>( pEv->position().x() ), 0,
				TargetWaveDisplay::nWidth
			),
			std::clamp(
				static_cast<int>( pEv->position().y() ), 0,
				TargetWaveDisplay::nHeight
			)
		);

		if ( ( QPoint( m_pDragPoint->nFrame, m_pDragPoint->nValue ) -
			   targetPoint )
				 .manhattanLength() <= QApplication::startDragDistance() ) {
			update();
			return;
		}

		// The left and right-most points are anchored and can only be moved up
		// and down.
		if ( m_pDragPoint->nFrame == 0 ) {
			targetPoint.setX( 0 );
		}
		else if ( m_pDragPoint->nFrame == TargetWaveDisplay::nWidth ) {
			targetPoint.setX( TargetWaveDisplay::nWidth );
		}

		// Only a single point is allowed per frame in the envelope. Ensure,
		// that we do not overwrite another while moving. Deleting should be an
		// explicit action.
		if ( m_pDragPoint->nFrame != targetPoint.x() ) {
			for ( const auto& ppoint : envelope ) {
				if ( ppoint.nFrame == targetPoint.x() ) {
					update();
					return;
				}
			}
		}

		m_pDragPoint->nFrame = targetPoint.x();
		m_pDragPoint->nValue = targetPoint.y();
	}
	update();
}

void TargetWaveDisplay::mousePressEvent( QMouseEvent* ev )
{
	if ( !m_bEnabled ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto envelope = m_pSampleEditor->getCurrentEnvelope();

	const auto hoveredPoints = getElementsAtPoint( ev->pos() );

	if ( ev->button() == Qt::LeftButton && hoveredPoints.size() > 0 ) {
		// Start dragging
		m_pDragPoint = std::make_shared<EnvelopePoint>( hoveredPoints[0] );
		m_nDragStartX = m_pDragPoint->nFrame;
		m_nDragStartY = m_pDragPoint->nValue;
	}
	else if ( ev->button() == Qt::LeftButton && hoveredPoints.size() == 0 ) {
		// add point
		if ( envelope.empty() ) {
			pHydrogenApp->beginUndoMacro(
				pHydrogenApp->getCommonStrings()->getActionEditEnvelopePoint()
			);
			pHydrogenApp->pushUndoCommand( new SE_editEnvelopePointAction(
				EnvelopePoint( 0, ev->pos().y() ),
				m_pSampleEditor->getEnvelopeType(), Editor::Action::Add
			) );
			pHydrogenApp->pushUndoCommand( new SE_editEnvelopePointAction(
				EnvelopePoint( TargetWaveDisplay::nWidth, ev->pos().y() ),
				m_pSampleEditor->getEnvelopeType(), Editor::Action::Add
			) );
			pHydrogenApp->endUndoMacro();
		}
		else {
			pHydrogenApp->pushUndoCommand( new SE_editEnvelopePointAction(
				EnvelopePoint( ev->pos().x(), ev->pos().y() ),
				m_pSampleEditor->getEnvelopeType(), Editor::Action::Add
			) );
		}
	}
	else if ( ev->button() == Qt::RightButton ) {
		// remove point

		if ( hoveredPoints.size() == 0 ||
			 envelope.size() > 2 &&
				 ( hoveredPoints[0].nFrame == 0 ||
				   hoveredPoints[0].nFrame == TargetWaveDisplay::nWidth ) ) {
			// do nothing if no point is selected
			// don't remove first or last point if more than 2 points in
			// envelope
			update();
			return;
		}
		else if ( envelope.size() == 2 ) {
			// if only 2 points, remove them both in order to reset the
			// envelope.
			pHydrogenApp->beginUndoMacro(
				pHydrogenApp->getCommonStrings()->getActionEditEnvelopePoint()
			);
			pHydrogenApp->pushUndoCommand( new SE_editEnvelopePointAction(
				envelope[1], m_pSampleEditor->getEnvelopeType(),
				Editor::Action::Delete
			) );
			pHydrogenApp->pushUndoCommand( new SE_editEnvelopePointAction(
				envelope[0], m_pSampleEditor->getEnvelopeType(),
				Editor::Action::Delete
			) );
			pHydrogenApp->endUndoMacro();
		}
		else {
			pHydrogenApp->pushUndoCommand( new SE_editEnvelopePointAction(
				hoveredPoints[0], m_pSampleEditor->getEnvelopeType(),
				Editor::Action::Delete
			) );
		}
	}
	update();
}

void TargetWaveDisplay::mouseReleaseEvent( QMouseEvent* ev )
{
	if ( !m_bEnabled || m_pDragPoint == nullptr ) {
		m_pDragPoint = nullptr;
		return;
	}

	UNUSED( ev );

	const auto oldPoint = EnvelopePoint( m_nDragStartX, m_nDragStartY );
	const auto newPoint =
		EnvelopePoint( m_pDragPoint->nFrame, m_pDragPoint->nValue );

	const QPoint start( oldPoint.nFrame, oldPoint.nValue );
	const QPoint end( newPoint.nFrame, newPoint.nValue );
	if ( ( end - start ).manhattanLength() <=
		 QApplication::startDragDistance() ) {
		m_pDragPoint = nullptr;
		m_pHoveredPoint = nullptr;
		update();
		return;
	}

	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_moveEnvelopePointAction(
			oldPoint, newPoint, m_pSampleEditor->getEnvelopeType()
		)
	);

	m_pDragPoint = nullptr;
	m_pHoveredPoint = nullptr;
	update();
}

std::vector<H2Core::EnvelopePoint> TargetWaveDisplay::getElementsAtPoint(
	const QPoint& point
)
{
	// We do not need a vector in here. Envelope points are guarantueed to be
	// unique in terms of x position.
	std::vector<EnvelopePoint> elementsAtPoint;

	const auto envelope = m_pSampleEditor->getCurrentEnvelope();

	int nLastDistance = width();

	for ( const auto& eelement : envelope ) {
		const QPoint ppoint( eelement.nFrame, eelement.nValue );
		const int nDistance = ( point - ppoint ).manhattanLength();
		if ( nDistance <= Editor::nDefaultCursorMargin &&
			 nDistance < nLastDistance ) {
			elementsAtPoint.clear();
			elementsAtPoint.push_back( eelement );
			nLastDistance = nDistance;
		}
	}

	return std::move( elementsAtPoint );
}
