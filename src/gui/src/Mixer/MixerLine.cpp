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

#include "MixerLine.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/MidiAction.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "../Widgets/Button.h"
#include "../Widgets/Fader.h"
#include "../Widgets/InstrumentNameWidget.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LED.h"
#include "../Widgets/Rotary.h"
#include "../Widgets/WidgetWithInput.h"

using namespace H2Core;

MixerLine::MixerLine(QWidget* pParent, std::shared_ptr<Instrument> pInstrument )
	: PixmapWidget( pParent )
	, m_pInstrument( pInstrument )
	, m_fOldMaxPeak( 0.0 )
	, m_nCycleSampleActivation( 0 )
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	resize( MixerLine::nWidth, MixerLine::nHeight );
	setFixedSize( MixerLine::nWidth, MixerLine::nHeight );

	setPixmap( "/mixerPanel/mixerline_background.png" );
	setObjectName( "MixerLine" );

	// Play sample button
	m_pPlaySampleBtn = new Button(
		this, QSize( 20, 15 ), Button::Type::Push, "play.svg", "", false,
		QSize( 7, 7 ), tr( "[Left click]: Play sample\n[Right click]: Stop all samples" ) );
	m_pPlaySampleBtn->move( 6, 1 );
	m_pPlaySampleBtn->setObjectName( "PlaySampleButton" );
	connect( m_pPlaySampleBtn, &Button::clicked, [&]() {
		const int nLine = retrieveLineNumber();
		if ( nLine != -1 ) {
			auto pHydrogen = Hydrogen::get_instance();
			pHydrogen->setSelectedInstrumentNumber( nLine );

			auto pNote = std::make_shared<Note>( m_pInstrument, 0, 1.0 );
			pHydrogen->getAudioEngine()->getSampler()->noteOn( pNote );
		}
	});
	connect( m_pPlaySampleBtn, &Button::rightClicked, [&]() {
		const int nLine = retrieveLineNumber();
		if ( nLine != -1 ) {
			auto pHydrogen = Hydrogen::get_instance();
			pHydrogen->setSelectedInstrumentNumber( nLine );

			auto pNote = std::make_shared<Note>( m_pInstrument, 0, 0.0 );
			pNote->setNoteOff( true );
			pHydrogen->getAudioEngine()->getSampler()->noteOn( pNote );
		}
	});

	// Trigger sample LED
	m_pTriggerSampleLED = new LED( this, QSize( 5, 13 ) );
	m_pTriggerSampleLED->move( 26, 2 );
	m_pTriggerSampleLED->setObjectName( "TriggerSampleLED" );
	
	// LED indicating that this particular mixerline is selected
	m_pSelectionLED = new LED( this, QSize( 11, 9 ) );
	m_pSelectionLED->move( 39, 2 );
	m_pSelectionLED->setObjectName( "SelectionLED" );

	// Mute button
	m_pMuteBtn = new Button(
		this, QSize( 22, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getSmallMuteButton(), true, QSize(), tr( "Mute" ) );
	m_pMuteBtn->move( 5, 16 );
	m_pMuteBtn->setObjectName( "MixerMuteButton" );
	connect( m_pMuteBtn, &Button::clicked, [&]() {
		const int nLine = retrieveLineNumber();
		if ( nLine != -1 ) {
			CoreActionController::setStripIsMuted(
				nLine, m_pMuteBtn->isChecked(), true );
		}
	});

	// Solo button
	m_pSoloBtn = new Button(
		this, QSize( 22, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getSmallSoloButton(), false, QSize(), tr( "Solo" ) );
	m_pSoloBtn->move( 28, 16 );
	m_pSoloBtn->setObjectName( "MixerSoloButton" );
	connect( m_pSoloBtn, &Button::clicked, [&]() {
		const int nLine = retrieveLineNumber();
		if ( nLine != -1 ) {
			CoreActionController::setStripIsSoloed(
				nLine, m_pSoloBtn->isChecked(), true );
		}
	});

	// pan rotary
	m_pPanRotary = new Rotary(
		this, Rotary::Type::Center, pCommonStrings->getNotePropertyPan(), false,
		PAN_MIN, PAN_MAX );
	m_pPanRotary->setObjectName( "PanRotary" );
	m_pPanRotary->move( 6, 32 );
	connect( m_pPanRotary, &Rotary::valueChanged, [&]() {
		const int nLine = retrieveLineNumber();
		if ( nLine != -1 ) {
			CoreActionController::setStripPanSym(
				nLine, m_pPanRotary->getValue(), true );
		}
	});

	// FX send
	int nnRow = 0;
	for ( int ii = 0; ii < MAX_FX; ii++ ) {
		auto pRotary = new Rotary(
			this, Rotary::Type::Small, tr( "FX %1 send" ).arg( ii + 1 ), false );
		pRotary->setObjectName( "FXRotary" );
		if ( (ii % 2) == 0 ) {
			pRotary->move( 9, 63 + (20 * nnRow) );
		}
		else {
			pRotary->move( 30, 63 + (20 * nnRow) );
			nnRow++;
		}
		connect( pRotary, &Rotary::valueChanged, [=]() {
			const int nLine = retrieveLineNumber();
			if ( nLine != -1 ) {
				CoreActionController::setStripEffectLevel(
					nLine, ii, pRotary->getValue(), true );
			}
		});
		m_fxRotaries.push_back( pRotary );
	}

	// instrument name widget
	m_pNameWidget = new InstrumentNameWidget( this );
	m_pNameWidget->move( 6, 128 );
	connect( m_pNameWidget, &InstrumentNameWidget::clicked, [&]() {
		const int nLine = retrieveLineNumber();
		if ( nLine != -1 ) {
			Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );
		}
	});

	// m_pFader
	m_pFader = new Fader( this, Fader::Type::Normal, tr( "Volume" ), false,
						  false, 0.0, 1.5 );
	m_pFader->move( 23, 128 );
	connect( m_pFader, &Fader::valueChanged, [&]() {
		const int nLine = retrieveLineNumber();
		if ( nLine != -1 ) {
			CoreActionController::setStripVolume(
				nLine, m_pFader->getValue(), true );
		}
	});

	m_pPeakLCD = new LCDDisplay( this, QSize( 41, 19 ), false, false );
	m_pPeakLCD->move( 8, 105 );
	m_pPeakLCD->setText( "0.00" );
	m_pPeakLCD->setToolTip( tr( "Peak" ) );
	QPalette lcdPalette;
	lcdPalette.setColor( QPalette::Window, QColor( 49, 53, 61 ) );
	m_pPeakLCD->setPalette( lcdPalette );

	updateActions();
	updateLine();
}

MixerLine::~MixerLine() {
}

void MixerLine::updateLine() {
	if ( m_pInstrument == nullptr ) {
		m_pFader->setIsActive( false );
		m_pPanRotary->setIsActive( false );
		m_pNameWidget->setText( "null" );
		m_pMuteBtn->setChecked( false );
		m_pMuteBtn->setIsActive( false );
		m_pSoloBtn->setChecked( false );
		m_pSoloBtn->setIsActive( false );
		m_pPlaySampleBtn->setIsActive( false );
		for ( auto& ppFxRotary : m_fxRotaries ) {
			ppFxRotary->setIsActive( false );
		}

		return;
	}
	else {
		m_pFader->setIsActive( true );
		m_pPanRotary->setIsActive( true );
		m_pMuteBtn->setIsActive( true );
		m_pSoloBtn->setIsActive( true );
		m_pPlaySampleBtn->setIsActive( true );
		for ( auto& ppFxRotary : m_fxRotaries ) {
			ppFxRotary->setIsActive( true );
		}
	}

	m_pNameWidget->setText( m_pInstrument->getName() );
	m_pFader->setValue( m_pInstrument->getVolume(), false,
						Event::Trigger::Suppress );
	m_pPanRotary->setValue(m_pInstrument->getPan(), false,
						Event::Trigger::Suppress );
	m_pMuteBtn->setChecked( m_pInstrument->isMuted() );
	m_pSoloBtn->setChecked( m_pInstrument->isSoloed() );
	for ( int ii = 0; ii < m_fxRotaries.size(); ++ii ) {
		auto ppFxRotary = m_fxRotaries[ ii ];
		ppFxRotary->setValue( m_pInstrument->getFxLevel( ii ), false,
							  Event::Trigger::Suppress );
	}

	updateSelected();
}

void MixerLine::updatePeaks()
{
	if ( m_pInstrument == nullptr ) {
		return;
	}
	auto pPref = Preferences::get_instance();
	const float fFallOffSpeed =
		pPref->getTheme().m_interface.m_fMixerFalloffSpeed;

	float fNewPeak_L = m_pInstrument->getPeak_L();
	float fNewPeak_R = m_pInstrument->getPeak_R();
	if ( ! pPref->showInstrumentPeaks() ) {
		fNewPeak_L = 0.0f;
		fNewPeak_R = 0.0f;
	}

	const float fOldPeak_L = m_pFader->getPeak_L();
	const float fOldPeak_R = m_pFader->getPeak_R();

	// reset instrument peak
	m_pInstrument->setPeak_L( 0.0f );
	m_pInstrument->setPeak_R( 0.0f );

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
	else {
		--m_nCycleKeepPeakText;
	}

	if ( ! sNewMaxPeak.isEmpty() && sNewMaxPeak != m_pPeakLCD->text() ) {
		m_pPeakLCD->setText( sNewMaxPeak );

		// Indicate levels near clipping.
		m_pPeakLCD->setUseRedFont( m_fOldMaxPeak > 1.0 );
	}

	// We highlight a note on event for a particular instrument by flashing a
	// LED widget.
	if ( m_nCycleSampleActivation > 0 ) {
		--m_nCycleSampleActivation;
		m_pTriggerSampleLED->setActivated( true );
	}
	else {
		m_pTriggerSampleLED->setActivated( false );
	}
}

void MixerLine::updateSelected() {
	m_pSelectionLED->setActivated(
		Hydrogen::get_instance()->getSelectedInstrument() == m_pInstrument );
}

void MixerLine::setInstrument( std::shared_ptr<H2Core::Instrument> pInstrument ) {
	if ( pInstrument != m_pInstrument ) {
		m_pInstrument = pInstrument;
		updateActions();
	}
}

void MixerLine::triggerSampleLED() {
	m_nCycleSampleActivation = MixerLine::nCyclesSampleActivationLED;
}

int MixerLine::retrieveLineNumber() const {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ||
		 m_pInstrument == nullptr ) {
		return -1;
	}

	return pSong->getDrumkit()->getInstruments()->index( m_pInstrument );
}

void MixerLine::updateActions() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( m_pInstrument == nullptr || pSong == nullptr ||
		 pSong->getDrumkit() == nullptr ) {
		m_pMuteBtn->setAction( nullptr );
		m_pSoloBtn->setAction( nullptr );
		m_pPanRotary->setAction( nullptr );
		m_pFader->setAction( nullptr );
		for ( auto& ppFxRotary : m_fxRotaries ) {
			ppFxRotary->setAction( nullptr );
		}

		return;
	}

	std::shared_ptr<Action> pAction = nullptr;
	const int nInstrument =
		pSong->getDrumkit()->getInstruments()->index( m_pInstrument );

	pAction = std::make_shared<Action>( "STRIP_MUTE_TOGGLE" );
	pAction->setParameter1( QString::number( nInstrument ));
	m_pMuteBtn->setAction( pAction );

	pAction = std::make_shared<Action>( "STRIP_SOLO_TOGGLE" );
	pAction->setParameter1( QString::number( nInstrument ));
	m_pSoloBtn->setAction( pAction );

	pAction = std::make_shared<Action>( "PAN_ABSOLUTE" );
	pAction->setParameter1( QString::number( nInstrument ) );
	pAction->setValue( QString::number( 0 ));
	m_pPanRotary->setAction( pAction );

	// FX send
	for ( int ii = 0; ii < m_fxRotaries.size(); ii++ ) {
		auto ppFxRotary = m_fxRotaries[ ii ];
		ppFxRotary->setObjectName( "FXRotary" );
		pAction = std::make_shared<Action>( "EFFECT_LEVEL_ABSOLUTE" );
		pAction->setParameter1( QString::number( nInstrument ) );
		pAction->setParameter2( QString::number( ii ) );
		ppFxRotary->setAction( pAction );
	}

	pAction = std::make_shared<Action>( "STRIP_VOLUME_ABSOLUTE" );
	pAction->setParameter1( QString::number( nInstrument ) );
	m_pFader->setAction( pAction );
}
