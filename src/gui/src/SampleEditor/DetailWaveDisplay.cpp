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
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

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

	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	drawPeakData();

	WaveDisplay::paintEvent( ev );

	QPainter p( this );

	QColor color;
	if ( m_pSampleEditor->getSelectedSlider() == SampleEditor::Slider::Start ) {
		color = pColorTheme->m_sampleEditor_startSliderColor;
		p.setPen( QPen( color, 1, Qt::SolidLine ) );
	}
	else if ( m_pSampleEditor->getSelectedSlider() == SampleEditor::Slider::Loop ) {
		color = pColorTheme->m_sampleEditor_loopSliderColor;
		p.setPen( QPen( color, 1, Qt::SolidLine ) );
	}
	else if ( m_pSampleEditor->getSelectedSlider() == SampleEditor::Slider::End ) {
		color = pColorTheme->m_sampleEditor_endSliderColor;
		p.setPen( QPen( color, 1, Qt::SolidLine ) );
	}
	else {
		Skin::setPlayheadPen( &p, false );
	}

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

	long long nnFrame;
	switch ( m_pSampleEditor->getSelectedSlider() ) {
		case SampleEditor::Slider::Start:
			nnFrame = m_pSampleEditor->getLoopStartFrame();
			break;
		case SampleEditor::Slider::Loop:
			nnFrame = m_pSampleEditor->getLoopLoopFrame();
			break;
		case SampleEditor::Slider::End:
			nnFrame = m_pSampleEditor->getLoopEndFrame();
			break;
		case SampleEditor::Slider::None:
			nnFrame = m_pSampleEditor->getPlayheadMain();
			break;
	}
	nnFrame -= static_cast<long long>(DetailWaveDisplay::nWidth / 2);

	QPointF peaks[width()];
	for ( int ii = 0; ii < width(); ++ii ) {
		if ( nnFrame >= 0 && nnFrame < m_peakData.size() ) {
			peaks[ii] = QPointF(
				ii, ( -m_peakData[nnFrame] * m_pSampleEditor->getZoomFactor()
					) + nVerticalCenter
			);
		}
		else {
			peaks[ii] = QPointF( ii, nVerticalCenter );
		}
		nnFrame++;
	}

	p.drawPolyline( peaks, width() );
}

void DetailWaveDisplay::updatePeakData()
{
	if ( m_pLayer == nullptr || m_pLayer->getSample() == nullptr ) {
		for ( long long ii = 0; ii < m_peakData.size(); ++ii ) {
			m_peakData[ii] = 0;
		}

		drawPeakData();
		update();
		return;
	}

	const long long nSampleLength = m_pLayer->getSample()->getFrames();
	const float fGain = height() / 2.0;

	m_peakData.clear();
	m_peakData.resize( nSampleLength );

	float* pSampleData;
	if ( m_channel == WaveDisplay::Channel::Left ) {
		pSampleData = m_pLayer->getSample()->getData_L();
	}
	else {
		pSampleData = m_pLayer->getSample()->getData_R();
	}

	for ( long long ii = 0; ii < nSampleLength; ii++ ) {
		m_peakData[ii] = static_cast<long long>( pSampleData[ii] * fGain );
	}

	drawPeakData();
	update();
}
