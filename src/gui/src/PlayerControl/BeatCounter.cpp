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

#include "BeatCounter.h"

#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Widgets/Button.h"

using namespace H2Core;

BeatCounter::BeatCounter( QWidget *pParent ) : QWidget( pParent ) {

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setAttribute( Qt::WA_OpaquePaintEvent );
	setFixedWidth( BeatCounter::nWidth );
	setObjectName( "BeatCounter" );

	auto pOverallLayout = new QHBoxLayout( this );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	pOverallLayout->setSpacing( 0 );
	setLayout( pOverallLayout );

	auto pBackground = new QWidget( this );
	pBackground->setObjectName( "Background" );
	pOverallLayout->addWidget( pBackground );

	auto pMainLayout = new QHBoxLayout( pBackground );
	pMainLayout->setContentsMargins( 1, 1, 1, 1 );
	pMainLayout->setSpacing( 0 );
	pMainLayout->setAlignment( Qt::AlignLeft );
	pBackground->setLayout( pMainLayout );

	const auto smallButtonSize = QSize(
		BeatCounter::nButtonWidth, BeatCounter::nButtonHeight );
	const auto smallIconSize = QSize( 8, 8 );

	////////////////////////////////////////////////////////////////////////////
	auto pLeftMargin = new QWidget( pBackground );
	pLeftMargin->setFixedWidth( BeatCounter::nButtonWidth );
	pMainLayout->addWidget( pLeftMargin );
	auto pLeftLayout = new QVBoxLayout( pLeftMargin );
	pLeftLayout->setContentsMargins( 0, 0, 0, 0 );
	pLeftLayout->setSpacing( 0 );

	m_pBeatLengthUpBtn = new Button(
		pLeftMargin, smallButtonSize, Button::Type::Push, "plus.svg", "",
		smallIconSize, "", false, true );
	connect( m_pBeatLengthUpBtn, &Button::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		float fBeatLength = pHydrogen->getBeatCounterBeatLength() * 2;
		if ( fBeatLength < 1 ) {
			fBeatLength = 8;
		}
		pHydrogen->setBeatCounterBeatLength( fBeatLength / 4 );
		update();
	} );
	pLeftLayout->addWidget( m_pBeatLengthUpBtn );

	m_pBeatLengthDownBtn = new Button(
		pLeftMargin, smallButtonSize, Button::Type::Push, "minus.svg", "",
		smallIconSize, "", false, true );
	connect( m_pBeatLengthDownBtn, &Button::clicked, [&](){
		auto pHydrogen = Hydrogen::get_instance();
		float fBeatLength = pHydrogen->getBeatCounterBeatLength() * 8;
		if ( fBeatLength > 8 ) {
			fBeatLength = 1;
		}
		pHydrogen->setBeatCounterBeatLength( fBeatLength / 4 );
		update();
	} );
	pLeftLayout->addWidget( m_pBeatLengthDownBtn );
	pLeftLayout->addStretch();

	////////////////////////////////////////////////////////////////////////////

	pMainLayout->addStretch();

	////////////////////////////////////////////////////////////////////////////
	auto pRightMargin = new QWidget( pBackground );
	pRightMargin->setFixedWidth( BeatCounter::nButtonWidth );
	pMainLayout->addWidget( pRightMargin );
	auto pRightLayout = new QVBoxLayout( pRightMargin );
	pRightLayout->setContentsMargins( 0, 0, 0, 0 );
	pRightLayout->setSpacing( 0 );

	m_pTotalBeatsUpBtn = new Button(
		pRightMargin, smallButtonSize, Button::Type::Push, "plus.svg", "",
		smallIconSize, "", false, true );
	connect( m_pTotalBeatsUpBtn, &Button::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		int nBeatsToCount = pHydrogen->getBeatCounterTotalBeats();
		nBeatsToCount++;
		if ( nBeatsToCount > 16 ) {
			nBeatsToCount = 2;
		}
		pHydrogen->setBeatCounterTotalBeats( nBeatsToCount );
		update();
	} );
	pRightLayout->addWidget( m_pTotalBeatsUpBtn );

	m_pTotalBeatsDownBtn = new Button(
		pRightMargin, smallButtonSize, Button::Type::Push, "minus.svg", "",
		smallIconSize, "", false, true );
	connect( m_pTotalBeatsDownBtn, &Button::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		int nBeatsToCount = pHydrogen->getBeatCounterTotalBeats();
		nBeatsToCount--;
		if ( nBeatsToCount < 2 ) {
			nBeatsToCount = 16;
		}
		pHydrogen->setBeatCounterTotalBeats( nBeatsToCount );
		update();
	} );
	pRightLayout->addWidget( m_pTotalBeatsDownBtn );

	m_pSetPlayBtn = new Button(
		pRightMargin, QSize( BeatCounter::nButtonWidth, 15 ),
		Button::Type::Push, "",
		pCommonStrings->getBeatCounterSetPlayButtonOff(), QSize(),
		tr("Set BPM / Set BPM and play"), false, true );
	m_pSetPlayBtn->setObjectName( "BeatCounterSetPlayButton" );
	connect( m_pSetPlayBtn, &Button::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		auto pPref = Preferences::get_instance();
		auto pHydrogenApp = HydrogenApp::get_instance();
		if ( pPref->m_bBeatCounterSetPlay !=
			 Preferences::BEAT_COUNTER_SET_PLAY_ON ) {
			pPref->m_bBeatCounterSetPlay = Preferences::BEAT_COUNTER_SET_PLAY_ON;
			pHydrogenApp->showStatusBarMessage( tr(" Count BPM and start PLAY") );
		}
		else {
			pPref->m_bBeatCounterSetPlay = Preferences::BEAT_COUNTER_SET_PLAY_OFF;
			pHydrogenApp->showStatusBarMessage( tr(" Count and set BPM") );
		}
		pHydrogen->updateBeatCounterSettings();
		// For instantaneous text update.
		updateBeatCounter();
	} );
	pRightLayout->addWidget( m_pSetPlayBtn );
	pRightLayout->addStretch();
}

BeatCounter::~BeatCounter(){
}

void BeatCounter::paintEvent( QPaintEvent* pEvent )
{
	if ( !isVisible() ) {
		return;
	}

	const auto pHydrogen = Hydrogen::get_instance();

	const auto theme = Preferences::get_instance()->getTheme();
	const QColor colorBackground =
		theme.m_color.m_windowColor.lighter( 134 ).lighter( 130 );
	const QColor colorCanvas = theme.m_color.m_windowColor;

	QPainter painter(this);

	// Background
	painter.fillRect( 0, 0, width(), height(), colorBackground );

	// Canvas
	painter.fillRect( BeatCounter::nButtonWidth + 2, 1,
					  BeatCounter::nWidth - BeatCounter::nButtonWidth * 2 - 4,
					  height() - 2, colorCanvas );

	painter.setPen( theme.m_color.m_windowTextColor );
	const QFont font( theme.m_font.m_sLevel3FontFamily,
					  getPointSize( theme.m_font.m_fontSize ) );
	painter.setFont( font );

	const int nMarginY = 3;
	const int nXLeftColumn = BeatCounter::nButtonWidth + 6;
	const int nXRightColumn = nXLeftColumn + 16;
	const int nTextHeight = height() / 2 - nMarginY - 2;

	painter.drawText( nXLeftColumn, nMarginY, 9, nTextHeight, Qt::AlignCenter, "1" );
	painter.drawText( nXLeftColumn, height() / 2 - 1, 9, 3, Qt::AlignCenter,
					  QChar( 0x2015 ) );

	// beat length
	painter.drawText( nXLeftColumn, height() - nMarginY - nTextHeight,
					  9, nTextHeight, Qt::AlignCenter,
					  QString::number( pHydrogen->getBeatCounterBeatLength() * 4 ) );

	// status
	QString sStatus;
	const int nEventCount = pHydrogen->getBeatCounterEventCount();
	if ( nEventCount == 1 ) {
		sStatus = "R";
	} else {
		sStatus = QString( "%1" ).arg( nEventCount - 1, 2, 10, QChar( 0x0030 ) );
	}
	painter.drawText( nXRightColumn, nMarginY, 23, nTextHeight,
					  Qt::AlignCenter, sStatus );

	// total beats
	painter.drawText( nXRightColumn, height() - nMarginY - nTextHeight,
					  23, nTextHeight, Qt::AlignCenter,
					  QString( "%1" ).arg( pHydrogen->getBeatCounterTotalBeats(),
										   2, 10, QChar( 0x0030 ) ) );
}

void BeatCounter::updateBeatCounter() {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pPref = Preferences::get_instance();

	if ( pPref->m_bBeatCounterSetPlay == Preferences::BEAT_COUNTER_SET_PLAY_ON ) {
		m_pSetPlayBtn->setText(
			pCommonStrings->getBeatCounterSetPlayButtonOn() );
	}
	else {
		m_pSetPlayBtn->setText(
			pCommonStrings->getBeatCounterSetPlayButtonOff() );
	}

	update();
}
