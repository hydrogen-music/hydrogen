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

#include "Mixer.h"

#include "LadspaFXLine.h"
#include "MixerLine.h"
#include "MasterLine.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../MainToolBar/MainToolBar.h"
#include "../Widgets/Button.h"
#include "../Widgets/PixmapWidget.h"
#include "MixerSettingsDialog.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/Basics/Note.h>
#include <core/CoreActionController.h>
#include <core/FX/Effects.h>
#include <core/Globals.h>

using namespace H2Core;

#include <cassert>

Mixer::Mixer( QWidget* pParent )
 : QWidget( pParent )
{
	setWindowTitle( tr( "Mixer" ) );

	auto pPref = Preferences::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	const int nFXFrameWidth = 213;
	const int nFixedHeight = MasterLine::nHeight;

	const int nScrollBarMarginX = 8;
	const int nScrollBarMarginY = 6;
	setMinimumSize(
		Mixer::nMinimumFaderPanelWidth + nFXFrameWidth + MasterLine::nWidth +
		nScrollBarMarginX,
		nFixedHeight + nScrollBarMarginY );

// fader Panel
	m_pFaderHBox = new QHBoxLayout();
	m_pFaderHBox->setSpacing( 0 );
	m_pFaderHBox->setContentsMargins( 0, 0, 0, 0 );

	m_pFaderPanel = new QWidget( nullptr );
	m_pFaderPanel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	m_pFaderPanel->setMinimumSize( Mixer::nMinimumFaderPanelWidth, nFixedHeight );
	m_pFaderPanel->setMaximumSize( 16777215, nFixedHeight );
	m_pFaderPanel->setLayout( m_pFaderHBox );

	m_pFaderScrollArea = new QScrollArea( nullptr );
	m_pFaderScrollArea->setFrameShape( QFrame::NoFrame );
	m_pFaderScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pFaderScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_pFaderScrollArea->setWidget( m_pFaderPanel );
// ~ fader panel

// fX frame
#ifdef H2CORE_HAVE_LADSPA
	auto pEffects = Effects::get_instance();
#endif
	m_pFXFrame = new PixmapWidget( nullptr );
	m_pFXFrame->setObjectName( "MixerFXRack" );
	m_pFXFrame->setFixedSize( nFXFrameWidth, nFixedHeight );
	m_pFXFrame->setPixmap( "/mixerPanel/background_FX.png" );
	for ( int nnFX = 0; nnFX < MAX_FX; nnFX++ ) {
#ifdef H2CORE_HAVE_LADSPA
		auto pFX = pEffects->getLadspaFX( nnFX );
		auto ppLine = new LadspaFXLine( m_pFXFrame, pFX, nnFX );
#else
		auto ppLine = new LadspaFXLine( m_pFXFrame, nullptr, nnFX );
#endif
		ppLine->setObjectName( "LadspaFXMixerLine" );
		ppLine->move( 13, 43 * nnFX + 84 );
		m_ladspaFXLines.push_back( ppLine );
	}

	if ( pPref->isFXTabVisible() ) {
		m_pFXFrame->show();
	}
	else {
		m_pFXFrame->hide();
	}
// ~ fX frame

// Master frame
	m_pMasterLine = new MasterLine( nullptr );
	m_pMasterLine->setObjectName( "MasterMixerLine" );

	m_pOpenMixerSettingsBtn = new Button(
		m_pMasterLine, QSize( 17, 17 ), Button::Type::Push, "cog.svg", "",
		QSize( 13, 13 ), tr( "Mixer Settings" ) );
	m_pOpenMixerSettingsBtn->setObjectName( "MixerSettingsButton" );
	m_pOpenMixerSettingsBtn->move( 96, 6 );
	connect( m_pOpenMixerSettingsBtn, SIGNAL( clicked() ), this,
			 SLOT( openMixerSettingsDialog() ) );

	m_pShowFXPanelBtn = new Button(
		m_pMasterLine, QSize( 49, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getFXButton(), QSize(), tr( "Show FX panel" ) );
	m_pShowFXPanelBtn->setObjectName( "MixerShowFXButton" );
	m_pShowFXPanelBtn->move( 63, 243 );
	m_pShowFXPanelBtn->setChecked( pPref->isFXTabVisible() );
	connect( m_pShowFXPanelBtn, SIGNAL( clicked() ),
			 this, SLOT( showFXPanelClicked() ));

#ifndef H2CORE_HAVE_LADSPA
	m_pShowFXPanelBtn->hide();
#endif

	m_pShowPeaksBtn = new Button(
		m_pMasterLine, QSize( 49, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getPeakButton(), QSize(),
		tr( "Show instrument peaks" ) );
	m_pShowPeaksBtn->setObjectName( "MixerShowPeaksButton" );
	m_pShowPeaksBtn->move( 63, 259 );
	m_pShowPeaksBtn->setChecked( pPref->showInstrumentPeaks() );
	connect( m_pShowPeaksBtn, SIGNAL( clicked() ),
			 this, SLOT( showPeaksBtnClicked() ));
// ~ Master frame

	// LAYOUT!
	QHBoxLayout *pLayout = new QHBoxLayout();
	pLayout->setSpacing( 0 );
	pLayout->setContentsMargins( 0, 0, 0, 0 );

	pLayout->addWidget( m_pFaderScrollArea );
	pLayout->addWidget( m_pFXFrame );
	pLayout->addWidget( m_pMasterLine );

	QWidget* pMainWidget = new QWidget();
	pMainWidget->setLayout( pLayout );
	pMainWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	pMainWidget->setMinimumSize( nMinimumFaderPanelWidth +
								 nFXFrameWidth + MasterLine::nWidth, nFixedHeight );
	pMainWidget->setMaximumSize( 16777215, nFixedHeight );

	auto pMainScrollArea = new QScrollArea();
	pMainScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	pMainScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	pMainScrollArea->setWidget( pMainWidget );
	pMainScrollArea->setWidgetResizable( true );

	auto pMainLayout = new QHBoxLayout();
	pMainLayout->setSpacing( 0 );
	pMainLayout->setContentsMargins( 0, 0, 0, 0 );
	pMainLayout->addWidget( pMainScrollArea );
	setLayout( pMainLayout );


	m_pUpdateTimer = new QTimer( this );
	connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updatePeaks() ) );
	m_pUpdateTimer->start( std::chrono::milliseconds( Mixer::nPeakTimeoutMs ) );

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &Mixer::onPreferencesChanged );

	HydrogenApp::get_instance()->addEventListener( this );
	resizeFaderPanel();
	updateMixer();
}

Mixer::~Mixer() {
	m_pUpdateTimer->stop();
}

void Mixer::updateMixer()
{
	if ( ! isVisible() ) {
		// Skip redundant updates if mixer is not visible.
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	const int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();

	bool bResize = false;
	for ( int nnIndex = 0; nnIndex < pInstrumentList->size(); ++nnIndex ) {
		auto ppInstrument = pInstrumentList->get( nnIndex );

		if ( nnIndex >= m_mixerLines.size() ) {
			// the mixerline doesn't exists. Create a new one.
			m_mixerLines.push_back( new MixerLine( this, ppInstrument ) );
			m_pFaderHBox->insertWidget( nnIndex, m_mixerLines[ nnIndex ] );
			bResize = true;
		}
		else {
			// Update existing line.
			auto pMixerLine = m_mixerLines[ nnIndex ];
			if ( pMixerLine == nullptr ) {
				ERRORLOG( QString( "Invalid line [%1]" ).arg( nnIndex ) );
				continue;
			}

			if ( pMixerLine->getInstrument() != ppInstrument ) {
				pMixerLine->setInstrument( ppInstrument );
			}
			pMixerLine->updateLine();
		}
	}

	// Remove superfluous instrument lines
	while ( m_mixerLines.size() > pInstrumentList->size() &&
			pInstrumentList->size() > 0 ) {
		delete m_mixerLines.back();
		m_mixerLines.pop_back();
		bResize = true;
	}

	const int nNewWidth = MixerLine::nWidth * m_mixerLines.size();
	if ( m_pFaderPanel->width() != nNewWidth ) {
		m_pFaderPanel->resize( nNewWidth, height() );
	}

	m_pMasterLine->updateLine();

#ifdef H2CORE_HAVE_LADSPA
	// LADSPA
	for ( int nnFX = 0; nnFX < MAX_FX; nnFX++ ) {
		auto pFX = Effects::get_instance()->getLadspaFX( nnFX );
		auto pFxLine = m_ladspaFXLines[ nnFX ];
		if ( pFxLine->getFX() != pFX ) {
			pFxLine->setFX( pFX );
		}

		pFxLine->updateLine();
	}
	// ~LADSPA
#endif
}

void Mixer::closeEvent( QCloseEvent* ev ) {
	HydrogenApp::get_instance()->showMixer( false );
}

void Mixer::updatePeaks()
{
	if ( ! isVisible() ) {
		// Skip redundant updates if mixer is not visible.
		return;
	}

	for ( auto& ppMixerLine : m_mixerLines ) {
		ppMixerLine->updatePeaks();
	}
	m_pMasterLine->updatePeaks();
}

void Mixer::showEvent ( QShowEvent *ev ) {
	UNUSED( ev );

	// Update visibility button
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();

	updateMixer();
}


void Mixer::hideEvent( QHideEvent *ev ) {
	UNUSED( ev );

	// Update visibility button
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
}
void Mixer::resizeEvent( QResizeEvent *ev ) {
	UNUSED( ev );
}

void Mixer::showFXPanelClicked() {
	if ( m_pShowFXPanelBtn->isChecked() ) {
		m_pFXFrame->show();
		Preferences::get_instance()->setFXTabVisible( true );
	} else {
		m_pFXFrame->hide();
		Preferences::get_instance()->setFXTabVisible( false );
	}

	resizeEvent( nullptr ); 	// force an update
}

void Mixer::showPeaksBtnClicked()
{
	auto pPref = Preferences::get_instance();

	if ( m_pShowPeaksBtn->isChecked() ) {
		pPref->setInstrumentPeaks( true );
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Show instrument peaks = On") );
	} else {
		pPref->setInstrumentPeaks( false );
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Show instrument peaks = Off") );
	}
}

void Mixer::openMixerSettingsDialog() {
	MixerSettingsDialog mixerSettingsDialog( this ); // use this as *parent because button makes smaller fonts
	mixerSettingsDialog.exec();
}

void Mixer::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Colors ) {
		for ( auto& ppMixerLine : m_mixerLines ) {
			ppMixerLine->updateColors();
		}
		for ( auto& ppLadspaLine : m_ladspaFXLines ) {
			ppLadspaLine->updateColors();
		}
		m_pMasterLine->updateColors();
	}

	if ( changes & H2Core::Preferences::Changes::Font ) {
		setFont( QFont( pPref->getFontTheme()->m_sApplicationFontFamily, 10 ) );
	}
}

void Mixer::drumkitLoadedEvent() {
	resizeFaderPanel();
	updateMixer();
}

void Mixer::effectChangedEvent() {
	updateMixer();
}

void Mixer::instrumentMuteSoloChangedEvent( int nInstrumentIndex ) {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}


	const auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	if ( nInstrumentIndex != -1 ) {
		for ( auto& ppLine : m_mixerLines ) {
			if ( ppLine != nullptr && ppLine->getInstrument() != nullptr &&
				 pInstrumentList->index( ppLine->getInstrument() ) ==
				 nInstrumentIndex ) {
				ppLine->updateLine();
				break;
			}
		}
	}
	else {
		updateMixer();
	}
}

void Mixer::instrumentParametersChangedEvent( int nInstrumentIndex ) {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	const auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	if ( nInstrumentIndex != -1 ) {
		for ( auto& ppLine : m_mixerLines ) {
			if ( ppLine != nullptr && ppLine->getInstrument() != nullptr &&
				 pInstrumentList->index( ppLine->getInstrument() ) ==
				 nInstrumentIndex ) {
				ppLine->updateLine();
				break;
			}
		}
	}
	else {
		updateMixer();
	}
}

void Mixer::mixerSettingsChangedEvent() {
	m_pMasterLine->updateLine();
}

void Mixer::noteRenderEvent( int nInstrumentIndex ) {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	const auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	if ( nInstrumentIndex != -1 ) {
		for ( auto& ppLine : m_mixerLines ) {
			if ( ppLine != nullptr && ppLine->getInstrument() != nullptr &&
				 pInstrumentList->index( ppLine->getInstrument() ) ==
				 nInstrumentIndex ) {
				ppLine->updateLine();
                ppLine->triggerSampleLED();
				break;
			}
		}
	}
}

void Mixer::selectedInstrumentChangedEvent() {
	updateMixer();
}

void Mixer::updatePreferencesEvent( int nValue ) {
	if ( nValue == 1 ) {
		// new preferences loaded within the core
		const auto pPref = H2Core::Preferences::get_instance();

		if ( pPref->isFXTabVisible() ) {
			m_pFXFrame->show();
		}
		else {
			m_pFXFrame->hide();
		}

		m_pShowFXPanelBtn->setChecked( pPref->isFXTabVisible() );
		m_pShowPeaksBtn->setChecked( pPref->showInstrumentPeaks() );
	}
}

void Mixer::updateSongEvent( int ) {
	resizeFaderPanel();
	updateMixer();
}

void Mixer::resizeFaderPanel() {
	int nWidth = Mixer::nMinimumFaderPanelWidth;

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		nWidth = std::max( pSong->getDrumkit()->getInstruments()->size() *
						   MixerLine::nWidth,
						  Mixer::nMinimumFaderPanelWidth );
	}

	m_pFaderPanel->resize( nWidth, MasterLine::nHeight );
}
