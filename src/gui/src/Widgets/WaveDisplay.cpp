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

WaveDisplay::WaveDisplay( QWidget* pParent, Channel channel )
	: QWidget( pParent ),
	  m_bEnabled( true ),
	  m_channel( channel ),
	  m_label( Label::SampleName ),
	  m_type( Type::Wave ),
	  m_nActiveWidth( -1 ),
	  m_sSampleName( "" ),
	  m_sFallbackLabel( "" ),
	  m_pLayer( nullptr ),
	  m_SampleNameAlignment( Qt::AlignCenter )
{
	setAttribute( Qt::WA_OpaquePaintEvent );

	m_peakData.resize( width() );
	m_peakDataMin.resize( width() );

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

void WaveDisplay::setEnabled( bool bEnabled )
{
	if ( m_bEnabled != bEnabled ) {
		m_bEnabled = bEnabled;
		drawPeakData();
		update();
	}
}

void WaveDisplay::setLayer( std::shared_ptr<H2Core::InstrumentLayer> pLayer )
{
	if ( pLayer == nullptr || pLayer->getSample() == nullptr ) {
		m_pLayer = nullptr;
		m_sSampleName = m_sFallbackLabel;
	}
	else {
		m_pLayer = pLayer;
		m_sSampleName = pLayer->getSample()->getFileName();
	}

	updateBackground();
	updatePeakData();
}

void WaveDisplay::updateBackground()
{
	auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();

	const QColor borderColor = Qt::black;
	QColor backgroundColor;
	if ( m_pLayer != nullptr && m_pLayer->getIsMuted() ) {
		backgroundColor = pColorTheme->m_muteColor;
	}
	else if ( m_pLayer != nullptr && m_pLayer->getIsSoloed() ) {
		backgroundColor = pColorTheme->m_soloColor;
	}
	else {
		backgroundColor = pColorTheme->m_accentColor;
	}

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

	// Border
	p.setPen( QPen( borderColor ) );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, 0, 0, height() );
	p.drawLine( 0, height(), width(), height() );
	p.drawLine( width(), 0, width(), height() );

	// Base line
	const int nVerticalCenter = height() / 2;
    QColor baseLineColor( borderColor );
    baseLineColor.setAlpha( 80 );
	p.setPen( QPen( baseLineColor, 1, Qt::DotLine ) );
	p.drawLine( 0, nVerticalCenter, width(), nVerticalCenter );

	// Propagate changes.
	drawPeakData();
	update();
}

void WaveDisplay::mouseDoubleClickEvent( QMouseEvent* ev )
{
	if ( ev->button() == Qt::LeftButton ) {
		emit doubleClicked( this );
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
		updateBackground();
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

void WaveDisplay::resizeEvent( QResizeEvent* event )
{
	if ( width() <= 0 ||
		 ( width() == m_peakData.size() && width() == m_peakDataMin.size() ) ) {
		return;
	}

	updateBackground();
	updatePeakData();
}

void WaveDisplay::onPreferencesChanged(
	const H2Core::Preferences::Changes& changes
)
{
	if ( changes & H2Core::Preferences::Changes::Font ) {
		update();
	}
}

void WaveDisplay::drawPeakData()
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

	if ( m_pLayer == nullptr || m_pLayer->getSample() == nullptr ) {
		return;
	}

	auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();

	QColor backgroundColor, textColor;
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

	QColor waveFormColor = pColorTheme->m_waveFormColor;
	const QColor waveFormInactiveColor =
		Skin::makeWidgetColorInactive( waveFormColor );

	if ( !m_bEnabled ) {
		textColor = Skin::makeTextColorInactive( textColor );
		waveFormColor = waveFormInactiveColor;
	}

	p.setPen( waveFormColor );
	const int nVerticalCenter = height() / 2;
	if ( m_nActiveWidth == -1 ) {
		// Display does not support distinction between active and inactive
		// region.
		if ( m_type == Type::Wave ) {
			QPointF peaks[width()];
			for ( int x = 0; x < width(); x++ ) {
				peaks[x] = QPointF( x, -m_peakData[x] + nVerticalCenter );
			}
			p.drawPolyline( peaks, width() );
		}
		else {
			QPointF peaks[width() + m_peakDataMin.size()];
			for ( int x = 0; x < width(); x++ ) {
				peaks[x] = QPointF( x, -m_peakData[x] + nVerticalCenter );
			}
			int ii = width();
			for ( int x = m_peakDataMin.size() - 1; x >= 0; --x ) {
				peaks[ii] = QPointF( x, -m_peakDataMin[x] + nVerticalCenter );
				++ii;
			}
			p.setBrush( waveFormColor );
			p.drawPolygon( peaks, width() + m_peakDataMin.size() );
		}
	}
	else {
		// Active part
		if ( m_type == Type::Wave ) {
			QPointF peaks[m_nActiveWidth];
			for ( int x = 0; x < m_nActiveWidth; x++ ) {
				peaks[x] = QPointF( x, -m_peakData[x] + nVerticalCenter );
			}
			p.drawPolyline( peaks, m_nActiveWidth );
		}
		else {
			QPointF peaks[2 * m_nActiveWidth];
			for ( int x = 0; x < m_nActiveWidth; x++ ) {
				peaks[x] = QPointF( x, -m_peakData[x] + nVerticalCenter );
			}
			int ii = m_nActiveWidth;
			for ( int x = m_nActiveWidth - 1; x >= 0; --x ) {
				peaks[ii] = QPointF( x, -m_peakDataMin[x] + nVerticalCenter );
				++ii;
			}
			p.setBrush( waveFormColor );
			p.drawPolygon( peaks, 2 * m_nActiveWidth );
		}

		// Inactive part
		p.setPen( waveFormInactiveColor );
		if ( m_type == Type::Wave ) {
			QPointF peaks[width() - m_nActiveWidth];
			int ii = 0;
			for ( int x = m_nActiveWidth; x < width(); x++ ) {
				peaks[ii++] = QPointF( x, -m_peakData[x] + nVerticalCenter );
			}
			p.drawPolyline( peaks, width() - m_nActiveWidth );
		}
		else {
			const auto nSize = m_peakData.size() - m_nActiveWidth +
							   m_peakDataMin.size() - m_nActiveWidth;
			QPointF peaks[nSize];
			int ii = 0;
			for ( int x = m_nActiveWidth; x < width(); x++ ) {
				peaks[ii] = QPointF( x, -m_peakData[x] + nVerticalCenter );
				++ii;
			}
			for ( int x = m_peakDataMin.size() - 1; x >= m_nActiveWidth; --x ) {
				peaks[ii] = QPointF( x, -m_peakDataMin[x] + nVerticalCenter );
				++ii;
			}
			p.setBrush( waveFormInactiveColor );
			p.drawPolygon( peaks, nSize );
		}
	}

	QString sText;
	if ( m_label == Label::SampleName ) {
		sText = m_sSampleName;
	}
	else {
		sText = m_sFallbackLabel;
	}

	if ( !sText.isEmpty() ) {
		QFont font(
			pPref->getFontTheme()->m_sApplicationFontFamily,
			getPointSize( pPref->getFontTheme()->m_fontSize )
		);
		font.setWeight( QFont::Bold );
		p.setFont( font );
		p.setPen( textColor );

		if ( m_SampleNameAlignment == Qt::AlignLeft ) {
			// Use a small offset instead of starting directly at the left
			// border
			p.drawText( 20, 0, width(), 20, m_SampleNameAlignment, sText );
		}
		else {
			p.drawText( 0, 0, width(), 20, m_SampleNameAlignment, sText );
		}
	}
}

void WaveDisplay::updatePeakData()
{
	if ( width() != m_peakData.size() || width() != m_peakDataMin.size() ) {
		m_peakData.resize( width() );
		m_peakDataMin.resize( width() );
	}

	if ( m_pLayer == nullptr || m_pLayer->getSample() == nullptr ) {
		for ( int ii = 0; ii < m_peakData.size(); ++ii ) {
			m_peakData[ii] = 0;
			m_peakDataMin[ii] = 0;
		}

		m_type = Type::Wave;

		drawPeakData();
		update();
		return;
	}

	const long long nSampleLength = m_pLayer->getSample()->getFrames();
	const auto pSampleData = m_channel == Channel::Left
								 ? m_pLayer->getSample()->getData_L()
								 : m_pLayer->getSample()->getData_R();
	const float fGain = height() / 2.0 * m_pLayer->getGain();

	if ( nSampleLength > m_peakData.size() ) {
		m_type = Type::Envelope;

		const long long nScaleFactor = std::ceil(
			static_cast<float>( nSampleLength ) /
			static_cast<float>( m_peakData.size() )
		);

		long long nSamplePos = 0;
		int nMin, nMax;
		for ( long long ii = 0; ii < m_peakData.size(); ++ii ) {
			nMin = 0;
			nMax = 0;
			for ( long long jj = 0; jj < nScaleFactor; ++jj ) {
				if ( nSamplePos >= nSampleLength ) {
					break;
				}

				if ( jj < nSampleLength ) {
					const int nNewVal =
						static_cast<int>( pSampleData[nSamplePos] * fGain );
					if ( nNewVal > nMax ) {
						nMax = nNewVal;
					}
					if ( nNewVal < nMin ) {
						nMin = nNewVal;
					}
				}
				++nSamplePos;
			}
			m_peakData[ii] = nMax;
			m_peakDataMin[ii] = nMin;
		}
	}
	else {
		m_type = Type::Wave;

		for ( long long ii = 0; ii < nSampleLength; ++ii ) {
			m_peakData[ii] = static_cast<int>( pSampleData[ii] * fGain );
		}
		for ( long long ii = nSampleLength; ii < m_peakData.size(); ++ii ) {
			m_peakData[ii] = 0;
		}
	}

	drawPeakData();
	update();
}
