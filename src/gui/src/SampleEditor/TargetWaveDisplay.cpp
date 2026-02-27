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

using namespace H2Core;

TargetWaveDisplay::TargetWaveDisplay( SampleEditor* pParent )
	: WaveDisplay( pParent, WaveDisplay::Channel::Left ),
	  m_pSampleEditor( pParent ),
	  m_bEnabled( true ),
	  m_sSelectedEnvelopePointValue( "" ),
	  m_nSelectedEnvelopePointX( -10 ),
	  m_nSelectedEnvelopePointY( -10 ),
	  m_oldPoint( EnvelopePoint() ),
	  m_nSelectedEnvelopePoint( -1 ),
	  m_nSnapRadius( 6 )
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

static void paintEnvelope(
	const std::vector<EnvelopePoint>& envelope,
	QPainter& painter,
	int selected,
	const QColor& lineColor,
	const QColor& handleColor
)
{
	if ( envelope.empty() ) {
		return;
	}

	for ( int i = 0; i < static_cast<int>( envelope.size() ) - 1; i++ ) {
		painter.setPen( QPen( lineColor, 1, Qt::SolidLine ) );
		painter.drawLine(
			envelope[i].nFrame, envelope[i].nValue, envelope[i + 1].nFrame,
			envelope[i + 1].nValue
		);
		if ( i == selected ) {
			painter.setBrush( handleColor.lighter( 150 ) );
		}
		else {
			painter.setBrush( handleColor );
		}
		painter.drawEllipse(
			envelope[i].nFrame - 6 / 2, envelope[i].nValue - 6 / 2, 6, 6
		);
	}

	// draw first and last points as squares
	if ( 0 == selected ) {
		painter.setBrush( handleColor.lighter( 150 ) );
	}
	else {
		painter.setBrush( handleColor );
	}
	painter.drawRect(
		envelope[0].nFrame - 12 / 2, envelope[0].nValue - 6 / 2, 12, 6
	);

	if ( envelope.size() - 1 == selected ) {
		painter.setBrush( handleColor.lighter( 150 ) );
	}
	else {
		painter.setBrush( handleColor );
	}
	painter.drawRect(
		envelope[envelope.size() - 1].nFrame - 12 / 2,
		envelope[envelope.size() - 1].nValue - 6 / 2, 12, 6
	);
}

void TargetWaveDisplay::paintEvent( QPaintEvent* ev )
{
	if ( !isVisible() ) {
		return;
	}

	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	WaveDisplay::paintEvent( ev );

	QPainter p( this );

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

	QColor velocityLineColor( pColorTheme->m_sampleEditor_velocityEnvelopeColor
	);
	velocityLineColor.setAlpha( SampleEditor::nColorAlpha );
	QColor panLineColor( pColorTheme->m_sampleEditor_panEnvelopeColor );
	panLineColor.setAlpha( SampleEditor::nColorAlpha );

	if ( !m_bEnabled ) {
		velocityLineColor = Skin::makeWidgetColorInactive( velocityLineColor );
		panLineColor = Skin::makeWidgetColorInactive( panLineColor );
	}

	paintEnvelope(
		m_pSampleEditor->getVelocityEnvelope(), p,
		m_pSampleEditor->getEnvelopeType() ==
				SampleEditor::EnvelopeType::Velocity
			? m_nSelectedEnvelopePoint
			: -1,
		velocityLineColor, pColorTheme->m_sampleEditor_velocityEnvelopeColor
	);
	paintEnvelope(
		m_pSampleEditor->getPanEnvelope(), p,
		m_pSampleEditor->getEnvelopeType() == SampleEditor::EnvelopeType::Pan
			? m_nSelectedEnvelopePoint
			: -1,
		panLineColor, pColorTheme->m_sampleEditor_panEnvelopeColor
	);

	if ( !m_sSelectedEnvelopePointValue.isEmpty() ) {
		QFont font;
		font.setWeight( QFont::Bold );
		p.setFont( font );
		p.setPen(
			m_pSampleEditor->getEnvelopeType() ==
					SampleEditor::EnvelopeType::Velocity
				? pColorTheme->m_sampleEditor_velocityEnvelopeColor
				: pColorTheme->m_sampleEditor_panEnvelopeColor
		);

		if ( m_nSelectedEnvelopePointY < 50 ) {
			if ( m_nSelectedEnvelopePointX < 790 ) {
				p.drawText(
					m_nSelectedEnvelopePointX + 5, m_nSelectedEnvelopePointY,
					60, 20, Qt::AlignLeft,
					QString( m_sSelectedEnvelopePointValue )
				);
			}
			else {
				p.drawText(
					m_nSelectedEnvelopePointX - 65, m_nSelectedEnvelopePointY,
					60, 20, Qt::AlignRight,
					QString( m_sSelectedEnvelopePointValue )
				);
			}
		}
		else {
			if ( m_nSelectedEnvelopePointX < 790 ) {
				p.drawText(
					m_nSelectedEnvelopePointX + 5,
					m_nSelectedEnvelopePointY - 20, 60, 20, Qt::AlignLeft,
					QString( m_sSelectedEnvelopePointValue )
				);
			}
			else {
				p.drawText(
					m_nSelectedEnvelopePointX - 65,
					m_nSelectedEnvelopePointY - 20, 60, 20, Qt::AlignRight,
					QString( m_sSelectedEnvelopePointValue )
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

void TargetWaveDisplay::updateMouseSelection( QMouseEvent* ev )
{
	auto pEv = static_cast<MouseEvent*>( ev );

	const auto envelope = m_pSampleEditor->getCurrentEnvelope();

	m_nSelectedEnvelopePointX = std::min(
		TargetWaveDisplay::nWidth,
		std::max( 0, static_cast<int>( pEv->position().x() ) )
	);
	m_nSelectedEnvelopePointY = std::min(
		TargetWaveDisplay::nHeight,
		std::max( 0, static_cast<int>( pEv->position().y() ) )
	);

	if ( !( ev->buttons() & Qt::LeftButton ) ||
		 m_nSelectedEnvelopePoint == -1 ) {
		QPoint mousePoint(
			m_nSelectedEnvelopePointX, m_nSelectedEnvelopePointY
		);
		int nSelection = -1;
		int nMinDistance = 1000000;
		for ( int ii = 0; ii < static_cast<int>( envelope.size() ); ii++ ) {
			if ( envelope[ii].nFrame >=
					 m_nSelectedEnvelopePointX - m_nSnapRadius &&
				 envelope[ii].nFrame <=
					 m_nSelectedEnvelopePointX + m_nSnapRadius ) {
				QPoint envelopePoint(
					envelope[ii].nFrame, envelope[ii].nValue
				);
				int nDelta = ( mousePoint - envelopePoint ).manhattanLength();
				if ( nDelta < nMinDistance ) {
					nMinDistance = nDelta;
					nSelection = ii;
				}
			}
		}

		m_nSelectedEnvelopePoint = nSelection;
		if ( nSelection != -1 ) {
			m_oldPoint = envelope[m_nSelectedEnvelopePoint];
		}
	}
	if ( m_nSelectedEnvelopePoint < 0 ||
		 m_nSelectedEnvelopePoint >= envelope.size() ) {
		m_sSelectedEnvelopePointValue = "";
	}
	else {
		float info = ( TargetWaveDisplay::nHeight -
					   envelope[m_nSelectedEnvelopePoint].nValue ) /
					 (float) TargetWaveDisplay::nHeight;
		m_sSelectedEnvelopePointValue.setNum( info, 'g', 2 );
	}
}

void TargetWaveDisplay::updateEnvelope()
{
	if ( m_nSelectedEnvelopePoint == -1 ) {
		return;
	}

	const auto envelope = m_pSampleEditor->getCurrentEnvelope();

	if ( m_nSelectedEnvelopePoint == 0 ) {
		m_nSelectedEnvelopePointX = 0;
	}
	else if ( m_nSelectedEnvelopePoint == envelope.size() - 1 ) {
		m_nSelectedEnvelopePointX = TargetWaveDisplay::nWidth;
	}

	// Only a single point is allowed per frame in the envelope. Ensure, that we
	// do not overwrite another while moving. Deleting should be an explicit
	// action.
	if ( envelope[m_nSelectedEnvelopePoint].nFrame !=
		 m_nSelectedEnvelopePointX ) {
		for ( const auto& ppoint : envelope ) {
			if ( ppoint.nFrame == m_nSelectedEnvelopePointX ) {
				return;
			}
		}
	}

	m_pSampleEditor->moveEnvelopePoint(
		envelope[m_nSelectedEnvelopePoint],
		EnvelopePoint( m_nSelectedEnvelopePointX, m_nSelectedEnvelopePointY ),
		m_pSampleEditor->getEnvelopeType()
	);

	// Refresh the selection
	const auto envelopeUpdated = m_pSampleEditor->getCurrentEnvelope();
	for ( int ii = 0; ii < envelopeUpdated.size(); ++ii ) {
		if ( envelopeUpdated[ii].nFrame == m_nSelectedEnvelopePointX &&
			 envelopeUpdated[ii].nValue == m_nSelectedEnvelopePointY ) {
			m_nSelectedEnvelopePoint = ii;
		}
	}
}

void TargetWaveDisplay::mouseMoveEvent( QMouseEvent* ev )
{
	if ( !m_bEnabled ) {
		return;
	}

	updateMouseSelection( ev );

	if ( !( ev->buttons() & Qt::LeftButton ) ) {
		return;
	}
	updateEnvelope();
	updateMouseSelection( ev );
}

void TargetWaveDisplay::mousePressEvent( QMouseEvent* ev )
{
	if ( !m_bEnabled ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto envelope = m_pSampleEditor->getCurrentEnvelope();

	updateMouseSelection( ev );

	if ( ev->button() == Qt::LeftButton && m_nSelectedEnvelopePoint == -1 ) {
		// add point
		if ( envelope.empty() ) {
			pHydrogenApp->beginUndoMacro(
				pHydrogenApp->getCommonStrings()->getActionEditEnvelopePoint()
			);
			pHydrogenApp->pushUndoCommand( new SE_editEnvelopePointAction(
				EnvelopePoint( 0, m_nSelectedEnvelopePointY ),
				m_pSampleEditor->getEnvelopeType(), Editor::Action::Add
			) );
			pHydrogenApp->pushUndoCommand( new SE_editEnvelopePointAction(
				EnvelopePoint(
					TargetWaveDisplay::nWidth, m_nSelectedEnvelopePointY
				),
				m_pSampleEditor->getEnvelopeType(), Editor::Action::Add
			) );
			pHydrogenApp->endUndoMacro();
		}
		else {
			pHydrogenApp->pushUndoCommand( new SE_editEnvelopePointAction(
				EnvelopePoint(
					m_nSelectedEnvelopePointX, m_nSelectedEnvelopePointY
				),
				m_pSampleEditor->getEnvelopeType(), Editor::Action::Add
			) );
		}
	}
	else if ( ev->button() == Qt::RightButton ) {
		// remove point

		if ( m_nSelectedEnvelopePoint == -1 ||
			 envelope.size() > 2 &&
				 ( m_nSelectedEnvelopePoint == 0 ||
				   m_nSelectedEnvelopePoint == envelope.size() - 1 ) ) {
			// do nothing if no point is selected
			// don't remove first or last point if more than 2 points in
			// envelope
			update();
			return;
		}
		else if ( envelope.size() == 2 ) {
			// if only 2 points, remove them both
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
				envelope[m_nSelectedEnvelopePoint],
				m_pSampleEditor->getEnvelopeType(), Editor::Action::Delete
			) );
		}
	}

	updateMouseSelection( ev );
}

void TargetWaveDisplay::mouseReleaseEvent( QMouseEvent* ev )
{
	if ( !m_bEnabled ) {
		return;
	}

	UNUSED( ev );

	const auto envelope = m_pSampleEditor->getCurrentEnvelope();

	auto newPoint = envelope[m_nSelectedEnvelopePoint];
	// Revert the last action and register an overall undo/redo action (this is
	// way more efficient than creating one for each mouse move event).
	m_pSampleEditor->moveEnvelopePoint(
		newPoint, m_oldPoint, m_pSampleEditor->getEnvelopeType()
	);

	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_moveEnvelopePointAction(
			m_oldPoint, newPoint, m_pSampleEditor->getEnvelopeType()
		)
	);
}
