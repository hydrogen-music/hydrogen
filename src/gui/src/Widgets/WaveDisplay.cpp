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
	  m_nCurrentWidth( 0 ),
	  m_nActiveWidth( -1 ),
	  m_sSampleName( "" ),
	  m_pLayer( nullptr ),
	  m_SampleNameAlignment( Qt::AlignCenter )
{
	setAttribute( Qt::WA_OpaquePaintEvent );

	m_peakData.resize( width() );

	connect(
		HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this,
		&WaveDisplay::onPreferencesChanged
	);
}

WaveDisplay::~WaveDisplay()
{
}

void WaveDisplay::paintEvent( QPaintEvent* ev )
{
	UNUSED( ev );
	QPainter painter( this );

	createBackground( &painter );
}

void WaveDisplay::createBackground( QPainter* painter )
{
	auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();

	const QColor borderColor = Qt::black;
	QColor textColor, backgroundColor, waveFormColor, waveFormInactiveColor;
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

	if ( Skin::moreBlackThanWhite( backgroundColor ) ) {
		waveFormColor = Qt::white;
		waveFormInactiveColor = pColorTheme->m_lightColor;
	}
	else {
		waveFormColor = Qt::black;
		waveFormInactiveColor = pColorTheme->m_darkColor;
	}

	painter->setRenderHint( QPainter::Antialiasing );

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

	painter->fillRect( 0, 0, width(), height(), QBrush( backgroundGradient ) );

	if ( m_pLayer != nullptr ) {
		painter->setPen( waveFormColor );
		const int nVerticalCenter = height() / 2;

		if ( m_nActiveWidth == -1 ) {
			// Display does not support distinction between active and inactive
			// region.
			for ( int x = 0; x < width(); x++ ) {
				painter->drawLine(
					x, nVerticalCenter, x, m_peakData[x] + nVerticalCenter
				);
				painter->drawLine(
					x, nVerticalCenter, x, -m_peakData[x] + nVerticalCenter
				);
			}
		}
		else {
			for ( int x = 0; x < m_nActiveWidth; x++ ) {
				painter->drawLine(
					x, nVerticalCenter, x, m_peakData[x] + nVerticalCenter
				);
				painter->drawLine(
					x, nVerticalCenter, x, -m_peakData[x] + nVerticalCenter
				);
			}
			painter->setPen( waveFormInactiveColor );
			for ( int x = m_nActiveWidth; x < width(); x++ ) {
				painter->drawLine(
					x, nVerticalCenter, x, m_peakData[x] + nVerticalCenter
				);
				painter->drawLine(
					x, nVerticalCenter, x, -m_peakData[x] + nVerticalCenter
				);
			}
		}
	}

	if ( !m_sSampleName.isEmpty() ) {
		QFont font(
			pPref->getFontTheme()->m_sApplicationFontFamily,
			getPointSize( pPref->getFontTheme()->m_fontSize )
		);
		font.setWeight( QFont::Bold );
		painter->setFont( font );
		painter->setPen( textColor );

		if ( m_SampleNameAlignment == Qt::AlignCenter ) {
			painter->drawText(
				0, 0, width(), 20, m_SampleNameAlignment, m_sSampleName
			);
		}
		else if ( m_SampleNameAlignment == Qt::AlignLeft ) {
			// Use a small offnset iso. starting directly at the left border
			painter->drawText(
				20, 0, width(), 20, m_SampleNameAlignment, m_sSampleName
			);
		}
	}

	// Border
	painter->setPen( QPen( borderColor ) );
	painter->drawLine( 0, 0, width(), 0 );
	painter->drawLine( 0, 0, 0, height() );
	painter->drawLine( 0, height(), width(), height() );
	painter->drawLine( width(), 0, width(), height() );
}

void WaveDisplay::resizeEvent( QResizeEvent* event )
{
	updateDisplay( m_pLayer );
}

void WaveDisplay::updateDisplay( std::shared_ptr<H2Core::InstrumentLayer> pLayer
)
{
	const int nCurrentWidth = width();

	if ( pLayer == nullptr || nCurrentWidth <= 0 ) {
		m_pLayer = nullptr;
		m_sSampleName = "";

		for ( int ii = 0; ii < m_peakData.size(); ++ii ) {
			m_peakData[ii] = 0;
		}

		update();
		return;
	}

	if ( nCurrentWidth != m_nCurrentWidth ) {
		m_peakData.resize( nCurrentWidth );

		m_nCurrentWidth = nCurrentWidth;
	}

	if ( pLayer != nullptr && pLayer->getSample() != nullptr ) {
		m_pLayer = pLayer;
		m_sSampleName = pLayer->getSample()->getFileName();

		const int nSampleLength = pLayer->getSample()->getFrames();
		const int nScaleFactor = nSampleLength / m_nCurrentWidth;

		const float fGain = height() / 2.0 * pLayer->getGain();

		auto pSampleData = pLayer->getSample()->getData_L();

		int nSamplePos = 0;
		int nVal;
		for ( int ii = 0; ii < m_peakData.size(); ++ii ) {
			nVal = 0;
			for ( int jj = 0; jj < nScaleFactor; ++jj ) {
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

	update();
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
