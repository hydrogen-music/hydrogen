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

#include <algorithm>
#include <memory>
#include <vector>

#include "../Compatibility/MouseEvent.h"
#include "../Skin.h"
#include "SampleEditor.h"

using namespace H2Core;

TargetWaveDisplay::TargetWaveDisplay( SampleEditor* pParent )
	: WaveDisplay( pParent, WaveDisplay::Channel::Left ),
	  m_pSampleEditor( pParent ),
	  m_sSelectedEnvelopePointValue( "" ),
	  m_nSelectedEnvelopePointX( -10 ),
	  m_nSelectedEnvelopePointY( -10 ),
	  m_nLocator( -1 ),
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
	const QColor& handleColor,
	const QColor& selectedColor
)
{
	if ( envelope.empty() ) {
		return;
	}

	for ( int i = 0; i < static_cast<int>( envelope.size() ) - 1; i++ ) {
		painter.setPen( QPen( lineColor, 1, Qt::SolidLine ) );
		painter.drawLine(
			envelope[i].frame, envelope[i].value, envelope[i + 1].frame,
			envelope[i + 1].value
		);
		if ( i == selected ) {
			painter.setBrush( selectedColor );
		}
		else {
			painter.setBrush( handleColor );
		}
		painter.drawEllipse(
			envelope[i].frame - 6 / 2, envelope[i].value - 6 / 2, 6, 6
		);
	}

	// draw first and last points as squares
	if ( 0 == selected ) {
		painter.setBrush( selectedColor );
	}
	else {
		painter.setBrush( handleColor );
	}
	painter.drawRect(
		envelope[0].frame - 12 / 2, envelope[0].value - 6 / 2, 12, 6
	);

	if ( envelope.size() - 1 == selected ) {
		painter.setBrush( selectedColor );
	}
	else {
		painter.setBrush( handleColor );
	}
	painter.drawRect(
		envelope[envelope.size() - 1].frame - 12 / 2,
		envelope[envelope.size() - 1].value - 6 / 2, 12, 6
	);
}

void TargetWaveDisplay::paintEvent( QPaintEvent* ev )
{
	if ( !isVisible() ) {
		return;
	}

	WaveDisplay::paintEvent( ev );

	QPainter p( this );

	p.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::SolidLine ) );
	p.drawLine( m_nLocator, 4, m_nLocator, height() - 4 );

	QColor volumeLineColor = QColor( 255, 255, 255, 200 );
	QColor volumeHandleColor = QColor( 99, 160, 233 );
	QColor panLineColor = QColor( 249, 235, 116, 200 );
	QColor panHandleColor = QColor( 77, 189, 55 );
	QColor selectedtHandleColor = QColor( 255, 100, 90 );

	paintEnvelope(
		m_pSampleEditor->getVelocityEnvelope(), p,
		m_pSampleEditor->getEnvelope() == SampleEditor::Envelope::Velocity
			? m_nSelectedEnvelopePoint
			: -1,
		volumeLineColor, volumeHandleColor, selectedtHandleColor
	);
	paintEnvelope(
		m_pSampleEditor->getPanEnvelope(), p,
		m_pSampleEditor->getEnvelope() == SampleEditor::Envelope::Pan
			? m_nSelectedEnvelopePoint
			: -1,
		panLineColor, panHandleColor, selectedtHandleColor
	);

	if ( !m_sSelectedEnvelopePointValue.isEmpty() ) {
		QFont font;
		font.setWeight( QFont::Bold );
		p.setFont( font );

		if ( m_nSelectedEnvelopePointY < 50 ) {
			if ( m_nSelectedEnvelopePointX < 790 ) {
				p.drawText(
					m_nSelectedEnvelopePointX + 5, m_nSelectedEnvelopePointY, 60, 20, Qt::AlignLeft, QString( m_sSelectedEnvelopePointValue )
				);
			}
			else {
				p.drawText(
					m_nSelectedEnvelopePointX - 65, m_nSelectedEnvelopePointY, 60, 20, Qt::AlignRight, QString( m_sSelectedEnvelopePointValue )
				);
			}
		}
		else {
			if ( m_nSelectedEnvelopePointX < 790 ) {
				p.drawText(
					m_nSelectedEnvelopePointX + 5, m_nSelectedEnvelopePointY - 20, 60, 20, Qt::AlignLeft,
					QString( m_sSelectedEnvelopePointValue )
				);
			}
			else {
				p.drawText(
					m_nSelectedEnvelopePointX - 65, m_nSelectedEnvelopePointY - 20, 60, 20, Qt::AlignRight,
					QString( m_sSelectedEnvelopePointValue )
				);
			}
		}
	}
}

void TargetWaveDisplay::paintLocatorEventTargetDisplay( int nPosition )
{
	m_nLocator = nPosition;
	update();
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
		QPoint mousePoint( m_nSelectedEnvelopePointX, m_nSelectedEnvelopePointY );
		int selection = -1;
		int min_distance = 1000000;
		for ( int i = 0; i < static_cast<int>( envelope.size() ); i++ ) {
			if ( envelope[i].frame >= m_nSelectedEnvelopePointX - m_nSnapRadius &&
				 envelope[i].frame <= m_nSelectedEnvelopePointX + m_nSnapRadius ) {
				QPoint envelopePoint( envelope[i].frame, envelope[i].value );
				int delta = ( mousePoint - envelopePoint ).manhattanLength();
				if ( delta < min_distance ) {
					min_distance = delta;
					selection = i;
				}
			}
		}
		m_nSelectedEnvelopePoint = selection;
	}
	if ( m_nSelectedEnvelopePoint == -1 ) {
		m_sSelectedEnvelopePointValue = "";
	}
	else {
		float info = ( TargetWaveDisplay::nHeight -
					   envelope[m_nSelectedEnvelopePoint].value ) /
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
	m_pSampleEditor->moveEnvelopePoint(
		envelope[m_nSelectedEnvelopePoint],
		EnvelopePoint( m_nSelectedEnvelopePointX, m_nSelectedEnvelopePointY ),
		m_pSampleEditor->getEnvelope()
	);

	// Refresh the selection
	const auto envelopeUpdated = m_pSampleEditor->getCurrentEnvelope();
	for ( int ii = 0; ii < envelopeUpdated.size(); ++ii ) {
		if ( envelopeUpdated[ii].frame == m_nSelectedEnvelopePointX &&
			 envelopeUpdated[ii].value == m_nSelectedEnvelopePointY ) {
			m_nSelectedEnvelopePoint = ii;
		}
	}
}

void TargetWaveDisplay::mouseMoveEvent( QMouseEvent* ev )
{
	updateMouseSelection( ev );

	if ( !( ev->buttons() & Qt::LeftButton ) ) {
		return;
	}
	updateEnvelope();
	updateMouseSelection( ev );
}

void TargetWaveDisplay::mousePressEvent( QMouseEvent* ev )
{
	const auto envelope = m_pSampleEditor->getCurrentEnvelope();

	updateMouseSelection( ev );

	if ( ev->button() == Qt::LeftButton ) {
		// add or move point
		if ( m_nSelectedEnvelopePoint == -1 ) {
			if ( envelope.empty() ) {
				m_pSampleEditor->editEnvelopePoint(
					EnvelopePoint( 0, m_nSelectedEnvelopePointY ),
					m_pSampleEditor->getEnvelope(), Editor::Action::Add
				);
				m_pSampleEditor->editEnvelopePoint(
					EnvelopePoint(
						TargetWaveDisplay::nWidth, m_nSelectedEnvelopePointY
					),
					m_pSampleEditor->getEnvelope(), Editor::Action::Add
				);
			}
			else {
				m_pSampleEditor->editEnvelopePoint(
					EnvelopePoint(
						m_nSelectedEnvelopePointX, m_nSelectedEnvelopePointY
					),
					m_pSampleEditor->getEnvelope(), Editor::Action::Add
				);
			}
		}
		else {
			// move old point to new position
			updateEnvelope();
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
			m_pSampleEditor->editEnvelopePoint(
				envelope[1], m_pSampleEditor->getEnvelope(),
				Editor::Action::Delete
			);
			m_pSampleEditor->editEnvelopePoint(
				envelope[0], m_pSampleEditor->getEnvelope(),
				Editor::Action::Delete
			);
		}
		else {
			m_pSampleEditor->editEnvelopePoint(
				envelope[m_nSelectedEnvelopePoint],
				m_pSampleEditor->getEnvelope(), Editor::Action::Delete
			);
		}
	}

	updateMouseSelection( ev );
}
