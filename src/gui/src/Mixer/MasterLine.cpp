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

#include "MasterLine.h"

#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/Fader.h"
#include "../Widgets/Rotary.h"
#include "../Widgets/Button.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/WidgetWithInput.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/MidiAction.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

using namespace H2Core;

MasterLine::MasterLine( QWidget* pParent )
	: PixmapWidget( pParent )
	, m_nCycleKeepPeakText( 0 )
	, m_fOldMaxPeak( 0 )
{
	setMinimumSize( MasterLine::nWidth, MasterLine::nHeight );
	setMaximumSize( MasterLine::nWidth, MasterLine::nHeight );
	resize( MasterLine::nWidth, MasterLine::nHeight );

	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Window, QColor( 58, 62, 72 ) );
	this->setPalette( defaultPalette );

	// Background image
	setPixmap( "/mixerPanel/masterMixerline_background.png" );

	const auto pPref = Preferences::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pFader = new Fader(
		this, Fader::Type::Master, tr( "Master volume" ), false, false, 0.0, 1.5 );
	m_pFader->move( 24, 75 );
	connect( m_pFader, &Fader::valueChanged, [&]() {
		CoreActionController::setMasterVolume( m_pFader->getValue() );
		HydrogenApp::get_instance()->showStatusBarMessage(
			tr( "Set master volume [%1]" ).arg( m_pFader->getValue(), 0, 'f', 2 ),
			QString( "%1:faderChanged" ).arg( class_name() ) );
	});

	auto pAction = std::make_shared<Action>("MASTER_VOLUME_ABSOLUTE");
	m_pFader->setAction( pAction );

	m_pPeakLCD = new LCDDisplay( this, QSize( 38, 18 ), false, false );
	m_pPeakLCD->move( 22, 51 );
	m_pPeakLCD->setText( "0.00" );
	m_pPeakLCD->setToolTip( tr( "Peak" ) );

	QPalette lcdPalette;
	lcdPalette.setColor( QPalette::Window, QColor( 49, 53, 61 ) );
	m_pPeakLCD->setPalette( lcdPalette );

	m_pHumanizeVelocityRotary = new Rotary(
		this, Rotary::Type::Normal, tr( "Humanize velocity" ), false );
	m_pHumanizeVelocityRotary->move( 66, 88 );
	connect( m_pHumanizeVelocityRotary, &Rotary::valueChanged, [&]() {
		CoreActionController::setHumanizeVelocity(
			m_pHumanizeVelocityRotary->getValue() );
		HydrogenApp::get_instance()->showStatusBarMessage(
			tr( "Set humanize vel. param [%1]" )
			.arg( m_pHumanizeVelocityRotary->getValue(), 0, 'f', 2 ),
			QString( "%1:humanizeVelocity" ).arg( class_name() ) );
	});

	m_pHumanizeTimeRotary = new Rotary(
		this, Rotary::Type::Normal, tr( "Humanize time" ), false );
	m_pHumanizeTimeRotary->move( 66, 125 );
	connect( m_pHumanizeTimeRotary, &Rotary::valueChanged, [&]() {
		CoreActionController::setHumanizeTime(
			m_pHumanizeTimeRotary->getValue() );
		HydrogenApp::get_instance()->showStatusBarMessage(
			tr( "Set humanize time param [%1]" )
			.arg( m_pHumanizeTimeRotary->getValue(), 0, 'f', 2 ),
			QString( "%1:humanizeTime" ).arg( class_name() ) );
	});

	m_pSwingRotary = new Rotary(
		this,  Rotary::Type::Normal, tr( "16th-note Swing" ), false );
	m_pSwingRotary->move( 66, 162 );
	connect( m_pSwingRotary, &Rotary::valueChanged, [&]() {
		CoreActionController::setSwing( m_pSwingRotary->getValue() );
		HydrogenApp::get_instance()->showStatusBarMessage(
			tr( "Set swing factor [%1]")
			.arg( m_pSwingRotary->getValue(), 0, 'f', 2 ),
			QString( "%1:humanizeSwing" ).arg( class_name() ) );
	});

	// Mute btn
	m_pMuteBtn = new Button( this, QSize( 42, 17 ), Button::Type::Toggle, "",
							 pCommonStrings->getBigMuteButton(), true );
	m_pMuteBtn->setObjectName( "MixerMasterMuteButton" );
	m_pMuteBtn->move( 20, 31 );
	pAction = std::make_shared<Action>("MUTE_TOGGLE");
	m_pMuteBtn->setAction( pAction );
	connect( m_pMuteBtn, &Button::clicked, [&]() {
		CoreActionController::setMasterIsMuted( m_pMuteBtn->isChecked() );
	});

	m_pMasterLbl = new ClickableLabel(
		this, QSize( 55, 15 ), pCommonStrings->getMasterLabel(),
		ClickableLabel::Color::Dark );
	m_pMasterLbl->move( 14, 8 );
	m_pHumanizeLbl = new ClickableLabel(
		this, QSize( 51, 9 ), pCommonStrings->getHumanizeLabel(),
		ClickableLabel::Color::Dark );
	m_pHumanizeLbl->move( 62, 79 );
	m_pSwingLbl = new ClickableLabel(
		this, QSize( 51, 9 ), pCommonStrings->getNotePropertyVelocity(),
		ClickableLabel::Color::Dark );
	m_pSwingLbl->move( 62, 116 );
	m_pTimingLbl = new ClickableLabel(
		this, QSize( 51, 9 ), pCommonStrings->getTimingLabel(),
		ClickableLabel::Color::Dark );
	m_pTimingLbl->move( 62, 153 );
	m_pVelocityLbl = new ClickableLabel(
		this, QSize( 51, 9 ), pCommonStrings->getSwingLabel(),
		ClickableLabel::Color::Dark );
	m_pVelocityLbl->move( 62, 190 );
}

MasterLine::~MasterLine() {
}

void MasterLine::updateLine() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		m_pFader->setIsActive( false );
		m_pHumanizeTimeRotary->setIsActive( false );
		m_pHumanizeVelocityRotary->setIsActive( false );
		m_pMuteBtn->setIsActive( false );
		m_pSwingRotary->setIsActive( false );

		return;
	}
	else {
		m_pFader->setIsActive( true );
		m_pHumanizeTimeRotary->setIsActive( true );
		m_pHumanizeVelocityRotary->setIsActive( true );
		m_pMuteBtn->setIsActive( true );
		m_pSwingRotary->setIsActive( true );
	}

	m_pFader->setValue( pSong->getVolume(), false, Event::Trigger::Suppress );
	m_pHumanizeTimeRotary->setValue( pSong->getHumanizeTimeValue(), false,
									 Event::Trigger::Suppress );
	m_pHumanizeVelocityRotary->setValue( pSong->getHumanizeVelocityValue(),
										 false, Event::Trigger::Suppress );
	m_pMuteBtn->setChecked( pSong->getIsMuted() );
	m_pSwingRotary->setValue( pSong->getSwingFactor(), false,
							  Event::Trigger::Suppress );
}

void MasterLine::updatePeaks() {
	const auto pPref = Preferences::get_instance();
	const float fFallOffSpeed =
		pPref->getTheme().m_interface.m_fMixerFalloffSpeed;
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	float fNewPeak_L = pAudioEngine->getMasterPeak_L();
	float fNewPeak_R = pAudioEngine->getMasterPeak_R();
	if ( ! pPref->showInstrumentPeaks() ) {
		fNewPeak_L = 0.0;
		fNewPeak_R = 0.0;
	}

	const float fOldPeak_L = m_pFader->getPeak_L();
	const float fOldPeak_R = m_pFader->getPeak_R();

	// reset master peak
	pAudioEngine->setMasterPeak_L( 0.0 );
	pAudioEngine->setMasterPeak_R( 0.0 );

	if ( fNewPeak_L < fOldPeak_L ) {
		fNewPeak_L = fOldPeak_L / fFallOffSpeed;
	}
	if ( fNewPeak_R < fOldPeak_R ) {
		fNewPeak_R = fOldPeak_R / fFallOffSpeed;
	}

	if ( fNewPeak_L != fOldPeak_L ) {
		m_pFader->setPeak_L( fNewPeak_L );
	}
	if ( fNewPeak_R != fOldPeak_R ) {
		m_pFader->setPeak_R( fNewPeak_R );
	}

	// Update textual representation of peak level
	float fNewMaxPeak = std::max( fNewPeak_L, fNewPeak_R );
	QString sNewMaxPeak;
	if ( fNewMaxPeak >= m_fOldMaxPeak ) {
		if ( fNewMaxPeak < 0.1 ) {
			fNewMaxPeak = 0;
		}

		// We got a new maximum. We display it right away. In case all
		// subsequent peaks a smaller, we keep the value a couple of cycles for
		// better readability.
		sNewMaxPeak = QString( "%1" ).arg( fNewMaxPeak, 0, 'f', 2 );
		m_nCycleKeepPeakText = static_cast<int>(fFallOffSpeed * 20 - 2);
		m_fOldMaxPeak = fNewMaxPeak;
	}
	else if ( m_nCycleKeepPeakText < 0 ) {
		// We kept the value of the peak long enough. Time to fade it out.
		if ( m_fOldMaxPeak > 0.05f ) {
			m_fOldMaxPeak = m_fOldMaxPeak - 0.05f;
		}
		else {
			m_fOldMaxPeak = 0.0f;
		}
		sNewMaxPeak = QString( "%1" ).arg( fNewMaxPeak, 0, 'f', 2 );
	}

	if ( ! sNewMaxPeak.isEmpty() && sNewMaxPeak != m_pPeakLCD->text() ) {
		m_pPeakLCD->setText( sNewMaxPeak );

		// Indicate levels near clipping.
		m_pPeakLCD->setUseRedFont( m_fOldMaxPeak > 1.0 );
	}
}
