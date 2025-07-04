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

#include "BpmTap.h"

#include "MainToolBar.h"

#include <core/Hydrogen.h>
#include <core/Midi/MidiAction.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../Widgets/MidiLearnableToolButton.h"

using namespace H2Core;

BpmTap::BpmTap( QWidget *pParent ) : QWidget( pParent )
								   , m_backgroundColor( Qt::red )
{

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_sJackActiveToolTip =
		tr( "In the presence of an external JACK Timebase controller the BeatCounter can not be used" );
	m_sTimelineActiveToolTip =
		tr( "Please deactivate the Timeline first in order to use the BeatCounter" );

	////////////////////////////////////////////////////////////////////////////
	const int nWidgetHeight = MainToolBar::nWidgetHeight -
		MainToolBar::nBorder * 2;

	setFixedHeight( nWidgetHeight );
	setAttribute( Qt::WA_OpaquePaintEvent );
	setObjectName( "BpmTap" );

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
		BpmTap::nMargin, BpmTap::nMargin, BpmTap::nMargin, BpmTap::nMargin );
	pMainLayout->setSpacing( BpmTap::nMargin );
	pBackground->setLayout( pMainLayout );

	const int nSmallButtonHeight = nWidgetHeight / 2 - BpmTap::nMargin;
	const auto smallButtonSize = QSize(
		static_cast<int>(std::round( nSmallButtonHeight *
									 Skin::fButtonWidthHeightRatio ) ),
		nSmallButtonHeight );
	const auto smallIconSize = QSize(
		smallButtonSize.height() - 2, smallButtonSize.height() - 2 );

	////////////////////////////////////////////////////////////////////////////
	const int nButtonHeight = nWidgetHeight - BpmTap::nMargin * 2;
	const int nButtonWidth = static_cast<int>(
		std::round( nButtonHeight * Skin::fButtonWidthHeightRatio ) );

	m_pTapTempoAction = new QAction( this );
	m_pTapTempoAction->setText(
		pCommonStrings->getTapTempoToolTip() );
	connect( m_pTapTempoAction, &QAction::triggered, [=](){
		auto pPref = Preferences::get_instance();
		if ( pPref->m_bpmTap != Preferences::BpmTap::TapTempo ) {
			pPref->m_bpmTap = Preferences::BpmTap::TapTempo;
			auto pHydrogenApp = HydrogenApp::get_instance();
			pHydrogenApp->showStatusBarMessage(
				pHydrogenApp->getCommonStrings()->getTapTempoToolTip() );
			Hydrogen::get_instance()->updateBeatCounterSettings();
		}
	} );

	m_pBeatCounterTapAction = new QAction( this );
	m_pBeatCounterTapAction->setText(
		pCommonStrings->getBeatCounterTapToolTip() );
	connect( m_pBeatCounterTapAction, &QAction::triggered, [=](){
		auto pPref = Preferences::get_instance();
		bool bChange = false;
		if ( pPref->m_bpmTap != Preferences::BpmTap::BeatCounter ) {
			pPref->m_bpmTap = Preferences::BpmTap::BeatCounter;
			bChange = true;
		}
		if ( pPref->m_beatCounter != Preferences::BeatCounter::Tap ) {
			pPref->m_beatCounter = Preferences::BeatCounter::Tap;
			bChange = true;
		}
		if ( bChange ) {
			auto pHydrogenApp = HydrogenApp::get_instance();
			pHydrogenApp->showStatusBarMessage(
				pHydrogenApp->getCommonStrings()->getBeatCounterTapToolTip() );
			Hydrogen::get_instance()->updateBeatCounterSettings();
		}
	} );

	m_pBeatCounterTapAndPlayAction = new QAction( this );
	m_pBeatCounterTapAndPlayAction->setText(
		pCommonStrings->getBeatCounterTapAndPlayToolTip() );
	connect( m_pBeatCounterTapAndPlayAction, &QAction::triggered, [=](){
		auto pPref = Preferences::get_instance();
		bool bChange = false;
		if ( pPref->m_bpmTap != Preferences::BpmTap::BeatCounter ) {
			pPref->m_bpmTap = Preferences::BpmTap::BeatCounter;
			bChange = true;
		}
		if ( pPref->m_beatCounter != Preferences::BeatCounter::TapAndPlay ) {
			pPref->m_beatCounter = Preferences::BeatCounter::TapAndPlay;
			bChange = true;
		}
		if ( bChange ) {
			auto pHydrogenApp = HydrogenApp::get_instance();
			pHydrogenApp->showStatusBarMessage(
				pHydrogenApp->getCommonStrings()->getBeatCounterTapAndPlayToolTip() );
			Hydrogen::get_instance()->updateBeatCounterSettings();
		}
	} );

	m_pTapTempoMidiAction = std::make_shared<MidiAction>( "TAP_TEMPO" );
	m_pBeatCounterMidiAction = std::make_shared<MidiAction>( "BEATCOUNTER" );

	m_pTapButton = new MidiLearnableToolButton( pBackground, "" );
	m_pTapButton->setFixedSize( nButtonWidth, nButtonHeight );
	m_pTapButton->addAction( m_pTapTempoAction );
	m_pTapButton->addAction( m_pBeatCounterTapAction );
	m_pTapButton->addAction( m_pBeatCounterTapAndPlayAction );
	m_pTapButton->setBaseToolTip( tr( "Set BPM / Set BPM and play" ) );
	m_pTapButton->setObjectName( "BpmTapTapButton" );
	connect( m_pTapButton, &QToolButton::clicked, [&]() {
		if ( Preferences::get_instance()->m_bpmTap ==
			 Preferences::BpmTap::TapTempo ) {
			Hydrogen::get_instance()->onTapTempoAccelEvent();
		} else {
			Hydrogen::get_instance()->handleBeatCounter();
		}
		// For instantaneous update.
		updateBpmTap();
	} );
	pMainLayout->addWidget( m_pTapButton );

	////////////////////////////////////////////////////////////////////////////
	m_pBeatLengthButtonsGroup = new QWidget( pBackground );
	m_pBeatLengthButtonsGroup->setFixedWidth( smallButtonSize.width() );
	pMainLayout->addWidget( m_pBeatLengthButtonsGroup );
	auto pBeatLengthButtonsGroupLayout =
		new QVBoxLayout( m_pBeatLengthButtonsGroup );
	pBeatLengthButtonsGroupLayout->setContentsMargins( 0, 0, 0, 0 );
	pBeatLengthButtonsGroupLayout->setSpacing( 0 );

	m_pBeatLengthUpBtn = new QToolButton( m_pBeatLengthButtonsGroup );
	connect( m_pBeatLengthUpBtn, &QToolButton::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		float fBeatLength = pHydrogen->getBeatCounterBeatLength() * 2;
		if ( fBeatLength < 1 ) {
			fBeatLength = 8;
		}
		pHydrogen->setBeatCounterBeatLength( fBeatLength / 4 );
		updateBpmTap();
	} );
	pBeatLengthButtonsGroupLayout->addWidget( m_pBeatLengthUpBtn );

	m_pBeatLengthDownBtn = new QToolButton( m_pBeatLengthButtonsGroup );
	connect( m_pBeatLengthDownBtn, &QToolButton::clicked, [&](){
		auto pHydrogen = Hydrogen::get_instance();
		float fBeatLength = pHydrogen->getBeatCounterBeatLength() * 8;
		if ( fBeatLength > 8 ) {
			fBeatLength = 1;
		}
		pHydrogen->setBeatCounterBeatLength( fBeatLength / 4 );
		updateBpmTap();
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
	/*: Tool tip for the left label in the beat counter within the main
	 *  toolbar. */
	m_pBeatLengthLabel->setToolTip(
		tr( "Indicates the type of note you are tapping" ) );
	pLabelsLayout->addWidget( m_pBeatLengthLabel );

	m_pTotalBeatsLabel = new QLabel( pLabelsGroup );
	m_pTotalBeatsLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
	m_pTotalBeatsLabel->setFixedWidth( 45 );
	m_pTotalBeatsLabel->setFixedHeight( nWidgetHeight );
	m_pTotalBeatsLabel->setContentsMargins( 5, 0, 5, 0 );
	/*: Tool tip for the left label in the beat counter within the main
	 *  toolbar. */
	m_pTotalBeatsLabel->setToolTip(
		tr( "Current vs. total number of taps to average" ) );
	pLabelsLayout->addWidget( m_pTotalBeatsLabel );

	////////////////////////////////////////////////////////////////////////////
	m_pTotalBeatsButtonsGroup = new QWidget( pBackground );
	m_pTotalBeatsButtonsGroup->setFixedWidth( smallButtonSize.width() );
	pMainLayout->addWidget( m_pTotalBeatsButtonsGroup );
	auto pTotalBeatsButtonsLayout = new QVBoxLayout( m_pTotalBeatsButtonsGroup );
	pTotalBeatsButtonsLayout->setContentsMargins( 0, 0, 0, 0 );
	pTotalBeatsButtonsLayout->setSpacing( 0 );

	m_pTotalBeatsUpBtn = new QToolButton( m_pTotalBeatsButtonsGroup );
	connect( m_pTotalBeatsUpBtn, &QToolButton::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		int nBeatsToCount = pHydrogen->getBeatCounterTotalBeats();
		nBeatsToCount++;
		if ( nBeatsToCount > 16 ) {
			nBeatsToCount = 2;
		}
		pHydrogen->setBeatCounterTotalBeats( nBeatsToCount );
		updateBpmTap();
	} );
	pTotalBeatsButtonsLayout->addWidget( m_pTotalBeatsUpBtn );

	m_pTotalBeatsDownBtn = new QToolButton( m_pTotalBeatsButtonsGroup );
	connect( m_pTotalBeatsDownBtn, &QToolButton::clicked, [&]() {
		auto pHydrogen = Hydrogen::get_instance();
		int nBeatsToCount = pHydrogen->getBeatCounterTotalBeats();
		nBeatsToCount--;
		if ( nBeatsToCount < 2 ) {
			nBeatsToCount = 16;
		}
		pHydrogen->setBeatCounterTotalBeats( nBeatsToCount );
		updateBpmTap();
	} );
	pTotalBeatsButtonsLayout->addWidget( m_pTotalBeatsDownBtn );
	pTotalBeatsButtonsLayout->addStretch();

	////////////////////////////////////////////////////////////////////////////
	updateBpmTap();
	updateIcons();
	updateStyleSheet();
}

BpmTap::~BpmTap(){
}

void BpmTap::updateBpmTap() {
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

	if ( pPref->m_bpmTap == Preferences::BpmTap::BeatCounter ) {
		m_pBeatLengthLabel->setVisible( true );
		m_pTotalBeatsLabel->setVisible( true );
		m_pBeatLengthButtonsGroup->setVisible( true );
		m_pTotalBeatsButtonsGroup->setVisible( true );

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

		QAction* pBeatCounterAction;
		if ( pPref->m_beatCounter == Preferences::BeatCounter::TapAndPlay ) {
			pBeatCounterAction = m_pBeatCounterTapAndPlayAction;
		}
		else {
			pBeatCounterAction = m_pBeatCounterTapAction;
		}
		if ( m_pTapButton->defaultAction() != pBeatCounterAction ) {
			m_pTapButton->setDefaultAction( pBeatCounterAction );
		}
		m_pTapButton->setMidiAction( m_pBeatCounterMidiAction );
	}
	else {
		// Widgets disabled
		m_pBeatLengthLabel->setVisible( false );
		m_pTotalBeatsLabel->setVisible( false );
		m_pBeatLengthButtonsGroup->setVisible( false );
		m_pTotalBeatsButtonsGroup->setVisible( false );

		if ( m_pTapButton->defaultAction() != m_pTapTempoAction ) {
			m_pTapButton->setDefaultAction( m_pTapTempoAction );
		}
		m_pTapButton->setMidiAction( m_pTapTempoMidiAction );
	}

	// Tool tip
	switch ( pHydrogen->getTempoSource() ) {
	case H2Core::Hydrogen::Tempo::Jack:
		m_pTapButton->setBaseToolTip( m_sJackActiveToolTip );
		m_pTapButton->setEnabled( false );
		break;
	case H2Core::Hydrogen::Tempo::Timeline:
		m_pTapButton->setBaseToolTip( m_sTimelineActiveToolTip );
		m_pTapButton->setEnabled( false );
		break;
	default:
		if ( pPref->m_bpmTap == Preferences::BpmTap::TapTempo ) {
			m_pTapButton->setBaseToolTip(
				pCommonStrings->getTapTempoToolTip() );
		}
		else if ( pPref->m_beatCounter == Preferences::BeatCounter::Tap ) {
			m_pTapButton->setBaseToolTip(
				pCommonStrings->getBeatCounterTapToolTip() );
		} else {
			m_pTapButton->setBaseToolTip(
				pCommonStrings->getBeatCounterTapAndPlayToolTip() );
		}
		m_pTapButton->setEnabled( true );
	}

}

void BpmTap::updateIcons() {
	QColor color;
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getTheme().m_interface.m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
		color = Qt::white;
	} else {
		sIconPath.append( "/icons/black/" );
		color = Qt::black;
	}

	m_pBeatLengthUpBtn->setIcon( QIcon( sIconPath + "plus.svg" ) );
	m_pBeatLengthDownBtn->setIcon( QIcon( sIconPath + "minus.svg" ) );
	m_pTotalBeatsUpBtn->setIcon( QIcon( sIconPath + "plus.svg" ) );
	m_pTotalBeatsDownBtn->setIcon( QIcon( sIconPath + "minus.svg" ) );
	// We use the same icon for both tap tempo and regular beat counter. They
	// are still visually distinct because for the former all other buttons of
	// the widgets will be disabled and all labels will be left blank.
	m_pTapTempoAction->setIcon( QIcon( sIconPath + "beat-counter-tap.svg" ) );
	m_pBeatCounterTapAction->setIcon( QIcon( sIconPath + "beat-counter-tap.svg" ) );
	m_pBeatCounterTapAndPlayAction->setIcon(
		QIcon( sIconPath + "beat-counter-tap-and-play.svg" ) );
}

void BpmTap::updateStyleSheet() {

	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	const QColor colorText = colorTheme.m_windowTextColor;
	const QColor colorLabel = colorTheme.m_windowColor;

	QColor colorBackgroundPressed, colorBackgroundHovered;
	if ( Skin::moreBlackThanWhite( m_backgroundColor ) ) {
		colorBackgroundPressed = m_backgroundColor.lighter(
			Skin::nToolBarCheckedScaling );
		colorBackgroundHovered = m_backgroundColor.lighter(
			Skin::nToolBarHoveredScaling );
	}
	else {
		colorBackgroundPressed = m_backgroundColor.darker(
			Skin::nToolBarCheckedScaling );
		colorBackgroundHovered = m_backgroundColor.darker(
			Skin::nToolBarHoveredScaling );
	}


	setStyleSheet( QString( "\
QToolButton {\
    background-color: %1; \
}\
QToolButton:pressed {\
    background-color: %2; \
}\
QToolButton:hover {\
    background-color: %3; \
}\
QToolButton:hover, QToolButton:pressed {\
    background-color: %2; \
}\
QWidget#Background {\
     background-color: %1; \
     color: %4; \
     border: %5px solid #000;\
}\
QToolButton#BpmTapTapButton {\
     icon-size: 25px;\
}")
				   .arg( m_backgroundColor.name() )
				   .arg( colorBackgroundPressed.name() )
				   .arg( colorBackgroundHovered.name() )
				   .arg( colorText.name() )
				   .arg( MainToolBar::nBorder ) );

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
