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

#include "WaveDisplay.h"

#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Preferences/Theme.h>

#include "../HydrogenApp.h"
#include "../Skin.h"

using namespace H2Core;

WaveDisplay::WaveDisplay( QWidget* pParent )
	: QWidget( pParent ),
	  m_nActiveWidth( -1 ),
	  m_sSampleName( "" ),
	  m_pLayer( nullptr ),
	  m_SampleNameAlignment( Qt::AlignCenter )
{
	setAttribute( Qt::WA_OpaquePaintEvent );

	m_peakData.resize( width() );

	const qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap =
		new QPixmap( width() * pixelRatio, height() * pixelRatio );
	m_pPeakDataPixmap =
		new QPixmap( width() * pixelRatio, height() * pixelRatio );

	connect(
		HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this,
		&WaveDisplay::onPreferencesChanged
	);
}

WaveDisplay::~WaveDisplay()
{
	if ( m_pBackgroundPixmap != nullptr ) {
		delete m_pBackgroundPixmap;
	}
	if ( m_pPeakDataPixmap != nullptr ) {
		delete m_pPeakDataPixmap;
	}
}

void WaveDisplay::paintEvent( QPaintEvent* ev )
{
	UNUSED( ev );

	if ( !isVisible() ) {
		return;
	}

	const qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pPeakDataPixmap->devicePixelRatio() ) {
		createBackground();
		drawPeakData();
	}

	QPainter painter( this );
	painter.drawPixmap(
		ev->rect(), *m_pPeakDataPixmap,
		QRectF(
			pixelRatio * ev->rect().x(), pixelRatio * ev->rect().y(),
			pixelRatio * ev->rect().width(), pixelRatio * ev->rect().height()
		)
	);
}

void WaveDisplay::createBackground()
{
	auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();

	const QColor borderColor = Qt::black;
	QColor textColor, backgroundColor;
	if ( m_pLayer != nullptr && m_pLayer->getIsMuted() ) {
		textColor = pColorTheme->m_muteTextColor;
		backgroundColor = pColorTheme->m_muteColor;
	}
	else if ( m_pLayer != nullptr && m_pLayer->getIsSoloed() ) {
		textColor = pColorTheme->m_soloTextColor;
		backgroundColor = pColorTheme->m_soloColor;
	}
	else {
		textColor = pColorTheme->m_accentTextColor;
		backgroundColor = pColorTheme->m_accentColor;
	}
	textColor.setAlpha( 200 );

	const qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->width() != width() ||
		 m_pBackgroundPixmap->height() != height() ||
		 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap =
			new QPixmap( width() * pixelRatio, height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
		delete m_pPeakDataPixmap;
		m_pPeakDataPixmap =
			new QPixmap( width() * pixelRatio, height() * pixelRatio );
		m_pPeakDataPixmap->setDevicePixelRatio( pixelRatio );
	}

	QPainter p( m_pBackgroundPixmap );
	p.setRenderHint( QPainter::Antialiasing );

	QLinearGradient backgroundGradient(
		QPointF( 0, 0 ), QPointF( 0, height() / 2 )
	);
	backgroundGradient.setColorAt(
		0, backgroundColor.darker( WaveDisplay::nGradientScaling )
	);
	backgroundGradient.setColorAt(
		1, backgroundColor.lighter( WaveDisplay::nGradientScaling )
	);
	backgroundGradient.setSpread( QGradient::ReflectSpread );

	p.fillRect( 0, 0, width(), height(), QBrush( backgroundGradient ) );

	if ( !m_sSampleName.isEmpty() ) {
		QFont font(
			pPref->getFontTheme()->m_sApplicationFontFamily,
			getPointSize( pPref->getFontTheme()->m_fontSize )
		);
		font.setWeight( QFont::Bold );
		p.setFont( font );
		p.setPen( textColor );

		if ( m_SampleNameAlignment == Qt::AlignCenter ) {
			p.drawText(
				0, 0, width(), 20, m_SampleNameAlignment, m_sSampleName
			);
		}
		else if ( m_SampleNameAlignment == Qt::AlignLeft ) {
			// Use a small offnset iso. starting directly at the left border
			p.drawText(
				20, 0, width(), 20, m_SampleNameAlignment, m_sSampleName
			);
		}
	}

	// Border
	p.setPen( QPen( borderColor ) );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, 0, 0, height() );
	p.drawLine( 0, height(), width(), height() );
	p.drawLine( width(), 0, width(), height() );
}

void WaveDisplay::drawPeakData()
{
	const qreal pixelRatio = devicePixelRatio();
	QPainter p( m_pPeakDataPixmap );
	// copy the background image
	p.drawPixmap( rect(), *m_pBackgroundPixmap,
						QRectF( pixelRatio * rect().x(),
								pixelRatio * rect().y(),
								pixelRatio * rect().width(),
								pixelRatio * rect().height() ) );

	auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();

	const QColor borderColor = Qt::black;
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

	if ( m_pLayer != nullptr ) {
		p.setPen( waveFormColor );
		const int nVerticalCenter = height() / 2;

		if ( m_nActiveWidth == -1 ) {
			// Display does not support distinction between active and inactive
			// region.
			for ( int x = 0; x < width(); x++ ) {
				p.drawLine(
					x, nVerticalCenter, x, m_peakData[x] + nVerticalCenter
				);
				p.drawLine(
					x, nVerticalCenter, x, -m_peakData[x] + nVerticalCenter
				);
			}
		}
		else {
			for ( int x = 0; x < m_nActiveWidth; x++ ) {
				p.drawLine(
					x, nVerticalCenter, x, m_peakData[x] + nVerticalCenter
				);
				p.drawLine(
					x, nVerticalCenter, x, -m_peakData[x] + nVerticalCenter
				);
			}
			p.setPen( waveFormInactiveColor );
			for ( int x = m_nActiveWidth; x < width(); x++ ) {
				p.drawLine(
					x, nVerticalCenter, x, m_peakData[x] + nVerticalCenter
				);
				p.drawLine(
					x, nVerticalCenter, x, -m_peakData[x] + nVerticalCenter
				);
			}
		}
	}
}

void WaveDisplay::resizeEvent( QResizeEvent* event )
{
	updateWidth();
}

void WaveDisplay::updatePeakData(
	std::shared_ptr<H2Core::InstrumentLayer> pLayer
)
{
	if ( pLayer == nullptr || pLayer->getSample() == nullptr ) {
		m_pLayer = nullptr;
		m_sSampleName = "";

		for ( int ii = 0; ii < m_peakData.size(); ++ii ) {
			m_peakData[ii] = 0;
		}

        drawPeakData();
		update();
		return;
	}

	m_pLayer = pLayer;
	m_sSampleName = pLayer->getSample()->getFileName();

	const int nSampleLength = pLayer->getSample()->getFrames();
	auto pSampleData = pLayer->getSample()->getData_L();
	const float fGain = height() / 2.0 * pLayer->getGain();

	if ( nSampleLength > m_peakData.size() ) {
		const int nScaleFactor = nSampleLength / m_peakData.size();

		int nSamplePos = 0;
		int nVal;
		for ( int ii = 0; ii < m_peakData.size(); ++ii ) {
			nVal = 0;
			for ( int jj = 0; jj < nScaleFactor; ++jj ) {
				if ( nSamplePos >= nSampleLength ) {
					break;
				}

				if ( jj < nSampleLength ) {
					const int nNewVal =
						static_cast<int>( pSampleData[nSamplePos] * fGain );
					if ( nNewVal > nVal ) {
						nVal = nNewVal;
					}
				}
				++nSamplePos;
			}
			m_peakData[ii] = nVal;
		}
	}
	else {
		for ( int ii = 0; ii < nSampleLength; ++ii ) {
			m_peakData[ii] = static_cast<int>( pSampleData[ii] * fGain );
		}
		for ( int ii = nSampleLength; ii < m_peakData.size(); ++ii ) {
			m_peakData[ii] = 0;
		}
	}

	drawPeakData();
	update();
}

void WaveDisplay::updateWidth()
{
	if ( width() <= 0 || width() == m_peakData.size() ) {
		return;
	}

	m_peakData.resize( width() );
    createBackground();
	updatePeakData( m_pLayer );
}

void WaveDisplay::mouseDoubleClickEvent( QMouseEvent* ev )
{
	if ( ev->button() == Qt::LeftButton ) {
		emit doubleClicked( this );
	}
}

void WaveDisplay::onPreferencesChanged(
	const H2Core::Preferences::Changes& changes
)
{
	if ( changes & H2Core::Preferences::Changes::Font ) {
		update();
	}
}
