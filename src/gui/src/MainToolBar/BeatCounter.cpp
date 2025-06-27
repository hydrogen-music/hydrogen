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
#include "../Skin.h"
#include "../Widgets/Button.h"
#include "../Widgets/PanelGroupBox.h"

using namespace H2Core;

BeatCounter::BeatCounter( QWidget *pParent ) : QWidget( pParent )
											 , m_backgroundColor( Qt::red )
											 , m_borderColor( Qt::green )
{

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	const int nWidgetHeight = MainToolBar::nWidgetHeight -
		PanelGroupBox::nMarginVertical * 2 - PanelGroupBox::nBorder * 2;

	setFixedHeight( nWidgetHeight );
	setAttribute( Qt::WA_OpaquePaintEvent );
	setObjectName( "BeatCounter" );

	auto pOverallLayout = new QHBoxLayout( this );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	pOverallLayout->setSpacing( 0 );
	setLayout( pOverallLayout );

	auto pBackground = new QWidget( this );
	pBackground->setObjectName( "Background" );
	pOverallLayout->addWidget( pBackground );

	auto pMainLayout = new QHBoxLayout( pBackground );
	pMainLayout->setAlignment( Qt::AlignTop );
	pMainLayout->setContentsMargins(
		BeatCounter::nMargin, BeatCounter::nMargin, BeatCounter::nMargin,
		BeatCounter::nMargin );
	pMainLayout->setSpacing( BeatCounter::nMargin );
	pBackground->setLayout( pMainLayout );

	const int nSmallButtonHeight =
		nWidgetHeight / 2 - BeatCounter::nMargin;
	const auto smallButtonSize = QSize(
		static_cast<int>(std::round( nSmallButtonHeight *
									 Skin::fButtonWidthHeightRatio ) ),
		nSmallButtonHeight );
	const auto smallIconSize = QSize(
		smallButtonSize.height() - 2, smallButtonSize.height() - 2 );

	////////////////////////////////////////////////////////////////////////////
	auto pBeatLengthButtonsGroup = new QWidget( pBackground );
	pBeatLengthButtonsGroup->setFixedWidth( smallButtonSize.width() );
	pMainLayout->addWidget( pBeatLengthButtonsGroup );
	auto pBeatLengthButtonsGroupLayout = new QVBoxLayout( pBeatLengthButtonsGroup );
	pBeatLengthButtonsGroupLayout->setContentsMargins( 0, 0, 0, 0 );
	pBeatLengthButtonsGroupLayout->setSpacing( 0 );

	m_pBeatLengthUpBtn = new Button(
		pBeatLengthButtonsGroup, smallButtonSize, Button::Type::Push, "plus.svg",
		"", smallIconSize, "", false, true );
	connect( m_pBeatLengthUpBtn, &Button::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		float fBeatLength = pHydrogen->getBeatCounterBeatLength() * 2;
		if ( fBeatLength < 1 ) {
			fBeatLength = 8;
		}
		pHydrogen->setBeatCounterBeatLength( fBeatLength / 4 );
		updateBeatCounter();
	} );
	pBeatLengthButtonsGroupLayout->addWidget( m_pBeatLengthUpBtn );

	m_pBeatLengthDownBtn = new Button(
		pBeatLengthButtonsGroup, smallButtonSize, Button::Type::Push, "minus.svg",
		"", smallIconSize, "", false, true );
	connect( m_pBeatLengthDownBtn, &Button::clicked, [&](){
		auto pHydrogen = Hydrogen::get_instance();
		float fBeatLength = pHydrogen->getBeatCounterBeatLength() * 8;
		if ( fBeatLength > 8 ) {
			fBeatLength = 1;
		}
		pHydrogen->setBeatCounterBeatLength( fBeatLength / 4 );
		updateBeatCounter();
	} );
	pBeatLengthButtonsGroupLayout->addWidget( m_pBeatLengthDownBtn );
	pBeatLengthButtonsGroupLayout->addStretch();

	////////////////////////////////////////////////////////////////////////////
	auto pLabelsGroup = new QWidget( pBackground );
	pMainLayout->addWidget( pLabelsGroup );
	auto pLabelsLayout = new QHBoxLayout( pLabelsGroup );
	pLabelsLayout->setContentsMargins( 0, 0, 0, 0 );
	pLabelsLayout->setSpacing( 0 );

	m_pBeatLengthLabel = new QLabel( pLabelsGroup );
	m_pBeatLengthLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
	m_pBeatLengthLabel->setFixedHeight( nWidgetHeight );
	m_pBeatLengthLabel->setContentsMargins( 5, 0, 5, 0 );
	pLabelsLayout->addWidget( m_pBeatLengthLabel );

	m_pTotalBeatsLabel = new QLabel( pLabelsGroup );
	m_pTotalBeatsLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
	m_pTotalBeatsLabel->setFixedWidth( 45 );
	m_pTotalBeatsLabel->setFixedHeight( nWidgetHeight );
	m_pTotalBeatsLabel->setContentsMargins( 5, 0, 5, 0 );
	pLabelsLayout->addWidget( m_pTotalBeatsLabel );

	////////////////////////////////////////////////////////////////////////////
	auto pTotalBeatsButtonsGroup = new QWidget( pBackground );
	pTotalBeatsButtonsGroup->setFixedWidth( smallButtonSize.width() );
	pMainLayout->addWidget( pTotalBeatsButtonsGroup );
	auto pTotalBeatsButtonsLayout = new QVBoxLayout( pTotalBeatsButtonsGroup );
	pTotalBeatsButtonsLayout->setContentsMargins( 0, 0, 0, 0 );
	pTotalBeatsButtonsLayout->setSpacing( 0 );

	m_pTotalBeatsUpBtn = new Button(
		pTotalBeatsButtonsGroup, smallButtonSize, Button::Type::Push, "plus.svg",
		"", smallIconSize, "", false, true );
	connect( m_pTotalBeatsUpBtn, &Button::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		int nBeatsToCount = pHydrogen->getBeatCounterTotalBeats();
		nBeatsToCount++;
		if ( nBeatsToCount > 16 ) {
			nBeatsToCount = 2;
		}
		pHydrogen->setBeatCounterTotalBeats( nBeatsToCount );
		updateBeatCounter();
	} );
	pTotalBeatsButtonsLayout->addWidget( m_pTotalBeatsUpBtn );

	m_pTotalBeatsDownBtn = new Button(
		pTotalBeatsButtonsGroup, smallButtonSize, Button::Type::Push, "minus.svg",
		"", smallIconSize, "", false, true );
	connect( m_pTotalBeatsDownBtn, &Button::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		int nBeatsToCount = pHydrogen->getBeatCounterTotalBeats();
		nBeatsToCount--;
		if ( nBeatsToCount < 2 ) {
			nBeatsToCount = 16;
		}
		pHydrogen->setBeatCounterTotalBeats( nBeatsToCount );
		updateBeatCounter();
	} );
	pTotalBeatsButtonsLayout->addWidget( m_pTotalBeatsDownBtn );
	pTotalBeatsButtonsLayout->addStretch();

	////////////////////////////////////////////////////////////////////////////
	const int nButtonHeight = nWidgetHeight -
		BeatCounter::nMargin * 2;
	const int nButtonWidth = static_cast<int>(
		std::round( nButtonHeight * Skin::fButtonWidthHeightRatio ) );
	m_pSetPlayBtn = new Button(
		pBackground, QSize( nButtonWidth, nButtonHeight ), Button::Type::Push,
		"", pCommonStrings->getBeatCounterSetPlayButtonOff(), QSize(),
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
	pMainLayout->addWidget( m_pSetPlayBtn );

	////////////////////////////////////////////////////////////////////////////
	updateStyleSheet();
}

BeatCounter::~BeatCounter(){
}

void BeatCounter::updateBeatCounter() {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pPref = Preferences::get_instance();
	const auto pHydrogen = Hydrogen::get_instance();

	auto toSuperScript = []( int nNumber ) {
		const QString sNumber = QString::number( nNumber );
		QString sResult;

		// We convert the number into a string and each separate digit into the
		// corresponding superscript.
		bool bOk;
		for ( const auto ssCharacter : sNumber ) {
			auto nNumber =
				static_cast<char>(QString(ssCharacter).toInt( &bOk, 10 ));
			if ( bOk ) {
				switch( nNumber ) {
				case 1:
					sResult.append( QChar( 0x00B9 ) );
					break;
				case 2:
					sResult.append( QChar( 0x00B2 ) );
					break;
				case 3:
					sResult.append( QChar( 0x00B3 ) );
					break;
				default:
					sResult.append( QChar( 0x2070 + nNumber ) );
					break;
				}
			}
		}
		return sResult;
	};

	auto toSubScript = []( int nNumber ) {
		const QString sNumber = QString::number( nNumber );
		QString sResult;

		// We convert the number into a string and each separate digit into the
		// corresponding subscript.
		bool bOk;
		for ( const auto ssCharacter : sNumber ) {
			auto nnNumber =
				static_cast<char>(QString(ssCharacter).toInt( &bOk, 10 ));
			if ( bOk ) {
				sResult.append( QChar( 0x2080 + nnNumber ) );
			}
		}
		return sResult;
	};

	m_pBeatLengthLabel->setText(
		QString( "%1%2%3" ).arg( toSuperScript( 1 ) ).arg( QChar( 0x2044 ) )
		.arg( toSubScript( pHydrogen->getBeatCounterBeatLength() * 4 ) ) );

	QString sStatus;
	const int nEventCount = pHydrogen->getBeatCounterEventCount();
	if ( nEventCount == 1 ) {
		// -
		sStatus = QChar( 0x207B );
	} else {
		sStatus = toSuperScript( nEventCount - 1 );
	}
	m_pTotalBeatsLabel->setText(
		QString( "%1%2%3" ).arg( sStatus ).arg( QChar( 0x2044 ) )
		.arg( toSubScript( pHydrogen->getBeatCounterTotalBeats() ) ) );

	if ( pPref->m_bBeatCounterSetPlay == Preferences::BEAT_COUNTER_SET_PLAY_ON ) {
		m_pSetPlayBtn->setText(
			pCommonStrings->getBeatCounterSetPlayButtonOn() );
	}
	else {
		m_pSetPlayBtn->setText(
			pCommonStrings->getBeatCounterSetPlayButtonOff() );
	}
}

void BeatCounter::updateStyleSheet() {

	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	const QColor colorText = colorTheme.m_windowTextColor;
	const QColor colorLabel = colorTheme.m_windowColor;

	setStyleSheet( QString( "\
QWidget#Background {\
     background-color: %1; \
     color: %2; \
     border: %3px solid %4;\
}")
				   .arg( m_backgroundColor.name() ).arg( colorText.name() )
				   .arg( MainToolBar::nBorder ).arg( m_borderColor.name() ) );

	const QString sLabelStyleSheet = QString( "\
QLabel {\
    background-color: %1;\
    color: %2;\
    font-size: %3px;\
}" )
		.arg( colorLabel.name() ).arg( colorText.name() )
		.arg( MainToolBar::nFontSize );
	m_pBeatLengthLabel->setStyleSheet( sLabelStyleSheet );
	m_pTotalBeatsLabel->setStyleSheet( sLabelStyleSheet );
}
