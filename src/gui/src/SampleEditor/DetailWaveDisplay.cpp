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

#include "DetailWaveDisplay.h"

#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Sample.h>

#include "../Skin.h"

using namespace H2Core;

DetailWaveDisplay::DetailWaveDisplay(
	SampleEditor* pSampleEditor,
	WaveDisplay::Channel channel
)
	: WaveDisplay( pSampleEditor, channel ),
	  m_pSampleEditor( pSampleEditor )
{
	m_label = WaveDisplay::Label::Fallback;
	m_sFallbackLabel = "";
	setFixedSize( DetailWaveDisplay::nWidth, DetailWaveDisplay::nHeight );
}

DetailWaveDisplay::~DetailWaveDisplay()
{
}

void DetailWaveDisplay::paintEvent( QPaintEvent* ev )
{
	if ( !isVisible() ) {
		return;
	}

	drawPeakData();

	WaveDisplay::paintEvent( ev );

	QColor color;
	if ( m_pSampleEditor->getSelectedSlider() == SampleEditor::Slider::Start ) {
		color = QColor( 32, 173, 0 );
	}
	else if ( m_pSampleEditor->getSelectedSlider() == SampleEditor::Slider::Loop ) {
		color = QColor( 93, 170, 254 );
	}
	else if ( m_pSampleEditor->getSelectedSlider() == SampleEditor::Slider::End ) {
		color = QColor( 217, 68, 0 );
	}
	else {
		color = QColor( 255, 255, 255 );
	}

	QPainter p( this );

	p.setPen( QPen( color, 1, Qt::SolidLine ) );
	p.drawLine( 90, 0, 90, 265 );
}

void DetailWaveDisplay::drawPeakData()
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

	QColor backgroundColor, waveFormColor, waveFormInactiveColor;
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
		waveFormColor = Qt::white;
		waveFormInactiveColor = pColorTheme->m_lightColor;
	}
	else {
		waveFormColor = Qt::black;
		waveFormInactiveColor = pColorTheme->m_darkColor;
	}

	p.setPen( waveFormColor );
	p.setRenderHint( QPainter::Antialiasing );
	const int nVerticalCenter = height() / 2;

	int nStartPosition =
		m_pSampleEditor->getFramePosition() - DetailWaveDisplay::nWidth / 2;

	for ( int ii = 0; ii < width(); ii++ ) {
		if ( nStartPosition > 0 && nStartPosition < m_peakData.size() ) {
			p.drawLine(
				ii,
				( -m_peakData[nStartPosition - 1] *
				  m_pSampleEditor->getZoomFactor() ) +
					nVerticalCenter,
				ii,
				( -m_peakData[nStartPosition] * m_pSampleEditor->getZoomFactor()
				) + nVerticalCenter
			);
		}
		else {
			p.drawLine( ii, 0 + nVerticalCenter, ii, 0 + nVerticalCenter );
		}
		nStartPosition++;
	}
}

void DetailWaveDisplay::updatePeakData()
{
	if ( m_pLayer == nullptr || m_pLayer->getSample() == nullptr ) {
		for ( int ii = 0; ii < m_peakData.size(); ++ii ) {
			m_peakData[ii] = 0;
		}

		drawPeakData();
		update();
		return;
	}

	const int nSampleLength = m_pLayer->getSample()->getFrames();
	const int nNewLength = nSampleLength + DetailWaveDisplay::nWidth / 2;
	const float fGain = height() / 4.0 * 1.0;

	m_peakData.clear();
	m_peakData.resize( nNewLength );

	for ( int ii = nSampleLength; ii < nNewLength; ii++ ) {
		m_peakData[ii] = 0;
	}

	float* pSampleData;
	if ( m_channel == WaveDisplay::Channel::Left ) {
		pSampleData = m_pLayer->getSample()->getData_L();
	}
	else {
		pSampleData = m_pLayer->getSample()->getData_R();
	}

	for ( int ii = 0; ii < nSampleLength; ii++ ) {
		m_peakData[ii] = static_cast<int>( pSampleData[ii] * fGain );
	}

	drawPeakData();
	update();
}
