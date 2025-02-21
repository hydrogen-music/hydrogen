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
#include "../LadspaFXProperties.h"
#include "../InstrumentEditor/InstrumentEditorPanel.h"
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
#include <core/EventQueue.h>
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

	const int nMinimumFaderPanelWidth = MixerLine::nWidth * 4;
	const int nFXFrameWidth = 213;
	const int nFixedHeight = MasterLine::nHeight;

	const int nScrollBarMarginX = 8;
	const int nScrollBarMarginY = 6;
	setMinimumSize(
		nMinimumFaderPanelWidth + nFXFrameWidth + MasterLine::nWidth +
		nScrollBarMarginX,
		nFixedHeight + nScrollBarMarginY );

// fader Panel
	m_pFaderHBox = new QHBoxLayout();
	m_pFaderHBox->setSpacing( 0 );
	m_pFaderHBox->setMargin( 0 );

	m_pFaderPanel = new QWidget( nullptr );
	m_pFaderPanel->resize( MixerLine::nWidth * MAX_INSTRUMENTS, nFixedHeight );
	m_pFaderPanel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	m_pFaderPanel->setMinimumSize( nMinimumFaderPanelWidth, nFixedHeight );
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
		auto ppLine = new LadspaFXLine( m_pFXFrame );
		ppLine->setObjectName( "LadspaFXMixerLine" );
		ppLine->move( 13, 43 * nnFX + 84 );
#ifdef H2CORE_HAVE_LADSPA
		if ( pEffects != nullptr ) {
			auto pFx = pEffects->getLadspaFX( nnFX );
			if ( pFx != nullptr ) {
				ppLine->setFxBypassed(
					pEffects->getLadspaFX( nnFX )->isEnabled() );
			}
		}
#endif
		connect( ppLine, SIGNAL( bypassBtnClicked(LadspaFXLine*) ),
				 this, SLOT( ladspaBypassBtnClicked( LadspaFXLine*) ) );
		connect( ppLine, SIGNAL( editBtnClicked(LadspaFXLine*) ),
				 this, SLOT( ladspaEditBtnClicked( LadspaFXLine*) ) );
		connect( ppLine, SIGNAL( volumeChanged(LadspaFXLine*) ),
				 this, SLOT( ladspaVolumeChanged( LadspaFXLine*) ) );
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
	connect( m_pMasterLine, SIGNAL( volumeChanged(MasterLine*) ),
			 this, SLOT( masterVolumeChanged(MasterLine*) ) );
	
	m_pOpenMixerSettingsBtn = new Button(
		m_pMasterLine, QSize( 17, 17 ), Button::Type::Push, "cog.svg", "",
		false, QSize( 13, 13 ), tr( "Mixer Settings" ) );
	m_pOpenMixerSettingsBtn->setObjectName( "MixerSettingsButton" );
	m_pOpenMixerSettingsBtn->move( 96, 6 );
	connect( m_pOpenMixerSettingsBtn, SIGNAL( clicked() ), this,
			 SLOT( openMixerSettingsDialog() ) );

	m_pShowFXPanelBtn = new Button(
		m_pMasterLine, QSize( 49, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getFXButton(), false, QSize(), tr( "Show FX panel" ) );
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
		pCommonStrings->getPeakButton(), false, QSize(),
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
	pLayout->setMargin( 0 );

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
	pMainLayout->setMargin( 0 );
	pMainLayout->addWidget( pMainScrollArea );
	setLayout( pMainLayout );


	m_pUpdateTimer = new QTimer( this );
	connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updatePeaks() ) );
	m_pUpdateTimer->start(50);

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &Mixer::onPreferencesChanged );

	HydrogenApp::get_instance()->addEventListener( this );
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
		if ( nnIndex >= m_mixerLines.size() ) {
			// the mixerline doesn't exists. Create a new one.
			m_mixerLines.push_back( createMixerLine( nnIndex ) );
			m_pFaderHBox->insertWidget( nnIndex, m_mixerLines[ nnIndex ] );
			bResize = true;
		}

		auto pMixerLine = m_mixerLines[ nnIndex ];
		auto pInstrument = pInstrumentList->get( nnIndex );
		if ( pMixerLine == nullptr || pInstrument == nullptr ) {
			ERRORLOG( QString( "Invalid line [%1]" ).arg( nnIndex ) );
			continue;
		}

		// fader position
		const float fNewVolume = pInstrument->get_volume();
		if ( pMixerLine->getVolume() != fNewVolume ) {
			pMixerLine->setVolume( fNewVolume, Event::Trigger::Suppress );
		}

		// mute / solo
		pMixerLine->setMuteClicked( pInstrument->is_muted() );
		pMixerLine->setSoloClicked( pInstrument->is_soloed() );

		// instr name
		pMixerLine->setName( pInstrument->get_name() );

		// pan
		const float fPan = pInstrument->getPan();
		if ( fPan != pMixerLine->getPan() ) {
			pMixerLine->setPan( fPan, Event::Trigger::Suppress );
		}

		for ( int nnFX = 0; nnFX < MAX_FX; nnFX++ ) {
			pMixerLine->setFXLevel( nnFX, pInstrument->get_fx_level( nnFX ),
									Event::Trigger::Suppress );
		}

		pMixerLine->setSelected( nnIndex == nSelectedInstrument );
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

	// set master fader position
	const float fNewVolume = pSong->getVolume();
	if ( m_pMasterLine->getVolume() != fNewVolume ) {
		m_pMasterLine->setVolume( fNewVolume, Event::Trigger::Suppress );
	}

	m_pMasterLine->setHumanizeTime( pSong->getHumanizeTimeValue(),
									Event::Trigger::Suppress );
	m_pMasterLine->setHumanizeVelocity( pSong->getHumanizeVelocityValue(),
										Event::Trigger::Suppress );
	m_pMasterLine->setSwing( pSong->getSwingFactor(), Event::Trigger::Suppress );
	m_pMasterLine->setMuteChecked( pSong->getIsMuted() );

#ifdef H2CORE_HAVE_LADSPA
	// LADSPA
	for ( int nnFX = 0; nnFX < MAX_FX; nnFX++ ) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nnFX );
		if ( pFX != nullptr ) {
			m_ladspaFXLines[nnFX]->setName( pFX->getPluginName() );
			m_ladspaFXLines[nnFX]->setFxBypassed( ! pFX->isEnabled() );
			m_ladspaFXLines[nnFX]->setVolume( pFX->getVolume(),
											 Event::Trigger::Suppress );
		}
		else {
			m_ladspaFXLines[nnFX]->setName( "No plugin" );
			m_ladspaFXLines[nnFX]->setFxBypassed( true );
			m_ladspaFXLines[nnFX]->setVolume( 0.0, Event::Trigger::Suppress );
		}
	}
	// ~LADSPA
#endif
}


MixerLine* Mixer::createMixerLine( int nInstr )
{
	MixerLine *pMixerLine = new MixerLine( nullptr , nInstr);
	pMixerLine->setObjectName( "MixerLine" );
	pMixerLine->setVolume( 0.2, Event::Trigger::Suppress );
	pMixerLine->setMuteClicked( false );
	pMixerLine->setSoloClicked( false );

	connect( pMixerLine, SIGNAL( noteOnClicked(MixerLine*) ),
			 this, SLOT( noteOnClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( noteOffClicked(MixerLine*) ),
			 this, SLOT( noteOffClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( muteBtnClicked(MixerLine*) ),
			 this, SLOT( muteClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( soloBtnClicked(MixerLine*) ),
			 this, SLOT( soloClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( volumeChanged(MixerLine*) ),
			 this, SLOT( volumeChanged(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( instrumentNameClicked(MixerLine*) ),
			 this, SLOT( nameClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( instrumentNameSelected(MixerLine*) ),
			 this, SLOT( nameSelected(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( panChanged(MixerLine*) ),
			 this, SLOT( panChanged( MixerLine*) ) );
	connect( pMixerLine, SIGNAL( knobChanged(MixerLine*, int) ),
			 this, SLOT( knobChanged( MixerLine*, int) ) );

	return pMixerLine;
}

void Mixer::closeEvent( QCloseEvent* ev ) {
	HydrogenApp::get_instance()->showMixer( false );
}

void Mixer::muteClicked( MixerLine* ref ) {
	const int nLine = findMixerLineByRef(ref);
	const bool bIsMuteClicked = ref->isMuteClicked();

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	pHydrogen->setSelectedInstrumentNumber( nLine );

	CoreActionController::setStripIsMuted( nLine, bIsMuteClicked );
}

void Mixer::soloClicked( MixerLine* ref ) {
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pInstrList = pSong->getDrumkit()->getInstruments();
	int nInstruments = std::min( pInstrList->size(), MAX_INSTRUMENTS );

	int nLine = findMixerLineByRef(ref);

	CoreActionController::setStripIsSoloed( nLine, ref->isSoloClicked() );

	for ( int i = 0; i < nInstruments; ++i ) {
			if ( m_mixerLines[i] != nullptr ){
				auto pInstrument = pInstrList->get(i);
				if ( pInstrument != nullptr ) {
					m_mixerLines[i]->setSoloClicked( pInstrument->is_soloed() );
				}
			}
	}

	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );
}

void Mixer::soloClicked( int nLine ) {
	if ( nLine < 0 || nLine >= MAX_INSTRUMENTS ) {
		ERRORLOG( QString( "Selected MixerLine [%1] out of bound [0,%2)" )
				  .arg( nLine ).arg( MAX_INSTRUMENTS ) );
		return;
	}

	MixerLine * pMixerLine = m_mixerLines[ nLine ];

	if( pMixerLine != nullptr ){
		pMixerLine->setSoloClicked( !pMixerLine->isSoloClicked() );
		soloClicked( pMixerLine );
	}
}

bool Mixer::isSoloClicked( int nLine ) {
	if ( nLine < 0 || nLine >= MAX_INSTRUMENTS ) {
		ERRORLOG( QString( "Selected MixerLine [%1] out of bound [0,%2)" )
				  .arg( nLine ).arg( MAX_INSTRUMENTS ) );
		return false;
	}
	if ( m_mixerLines[ nLine ] == nullptr ) {
		return false;
	}
	return m_mixerLines[ nLine ]->isSoloClicked();
}

void Mixer::noteOnClicked( MixerLine* ref ) {
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	int nLine = findMixerLineByRef( ref );
	pHydrogen->setSelectedInstrumentNumber( nLine );
	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}
	
	if ( ! pSelectedInstrument->hasSamples() ) {
		INFOLOG( QString( "Instrument [%1] does not contain any samples. No preview available" )
				 .arg( pSelectedInstrument->get_name() ) );
		return;
	}
	
	auto pNote = std::make_shared<Note>( pSelectedInstrument, 0, 1.0 );
	pHydrogen->getAudioEngine()->getSampler()->noteOn( pNote );
}

 void Mixer::noteOffClicked( MixerLine* ref ) {
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	
	int nLine = findMixerLineByRef( ref );
	pHydrogen->setSelectedInstrumentNumber( nLine );
	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	auto pNote = std::make_shared<Note>( pSelectedInstrument, 0, 0.0 );
	pNote->setNoteOff( true );
	pHydrogen->getAudioEngine()->getSampler()->noteOff( pNote );
}

int Mixer::findMixerLineByRef(MixerLine* ref) {
	for (int i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_mixerLines[i] == ref) {
			return i;
		}
	}
	return 0;
}

void Mixer::volumeChanged(MixerLine* ref) {
	int nLine = findMixerLineByRef(ref);
	CoreActionController::setStripVolume( nLine, ref->getVolume(), true );
}

void Mixer::masterVolumeChanged(MasterLine* ref) {
	float Volume = ref->getVolume();
	CoreActionController::setMasterVolume( Volume );
}

void Mixer::updatePeaks()
{
	if ( ! isVisible() ) {
		// Skip redundant updates if mixer is not visible.
		return;
	}
	const auto pPref = Preferences::get_instance();
	const bool bShowPeaks = pPref->showInstrumentPeaks();
	const float fFallOffSpeed =
		pPref->getTheme().m_interface.m_fMixerFalloffSpeed;

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pInstrumentList = pSong->getDrumkit()->getInstruments();

	for ( auto& ppMixerLine : m_mixerLines ) {
		auto pInstrument = pInstrumentList->get( findMixerLineByRef( ppMixerLine ) );
		if ( pInstrument == nullptr ) {
			ERRORLOG( "Invalid instrument" );
			continue;
		}

		float fNewPeak_L = pInstrument->get_peak_l();
		pInstrument->set_peak_l( 0.0f );	// reset instrument peak

		float fNewPeak_R = pInstrument->get_peak_r();
		pInstrument->set_peak_r( 0.0f );	// reset instrument peak

		// fader
		float fOldPeak_L = ppMixerLine->getPeak_L();
		float fOldPeak_R = ppMixerLine->getPeak_R();

		if ( ! bShowPeaks ) {
			fNewPeak_L = 0.0f;
			fNewPeak_R = 0.0f;
		}

		if ( fNewPeak_L >= fOldPeak_L) {	// LEFT peak
			ppMixerLine->setPeak_L( fNewPeak_L );
		}
		else {
			ppMixerLine->setPeak_L( fOldPeak_L / fFallOffSpeed );
		}

		if ( fNewPeak_R >= fOldPeak_R) {	// Right peak
			ppMixerLine->setPeak_R( fNewPeak_R );
		}
		else {
			ppMixerLine->setPeak_R( fOldPeak_R / fFallOffSpeed );
		}

		// activity
		if ( ppMixerLine->getActivity() > 0 ) {
			ppMixerLine->setActivity( ppMixerLine->getActivity() - 30 );
			ppMixerLine->setPlayClicked( true );
		}
		else {
			ppMixerLine->setPlayClicked( false );
		}

		ppMixerLine->updateMixerLine();
	}

	// update MasterPeak
	float fOldPeak_L = m_pMasterLine->getPeak_L();
	float fNewPeak_L = pAudioEngine->getMasterPeak_L();
	pAudioEngine->setMasterPeak_L(0.0);

	float fOldPeak_R = m_pMasterLine->getPeak_R();
	float fNewPeak_R = pAudioEngine->getMasterPeak_R();
	pAudioEngine->setMasterPeak_R(0.0);

	if ( ! bShowPeaks ) {
		fNewPeak_L = 0.0;
		fNewPeak_R = 0.0;
	}

	if ( fNewPeak_L >= fOldPeak_L ) {
		m_pMasterLine->setPeak_L( fNewPeak_L );
	}
	else {
		m_pMasterLine->setPeak_L( fOldPeak_L / fFallOffSpeed );
	}

	if ( fNewPeak_R >= fOldPeak_R ) {
		m_pMasterLine->setPeak_R( fNewPeak_R );
	}
	else {
		m_pMasterLine->setPeak_R( fOldPeak_R / fFallOffSpeed );
	}
	m_pMasterLine->updateMixerLine();
}

void Mixer::showEvent ( QShowEvent *ev ) {
	UNUSED( ev );
	updateMixer();
}

void Mixer::nameClicked(MixerLine* ref)
{
	UNUSED( ref );
	InstrumentEditorPanel::get_instance()->show();
}

void Mixer::nameSelected(MixerLine* ref) {
	int nLine = findMixerLineByRef(ref);
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );
}

void Mixer::panChanged(MixerLine* ref) {
	float	fPan = ref->getPan();
	int		nLine = findMixerLineByRef(ref);
	CoreActionController::setStripPanSym( nLine, fPan, true );
}

void Mixer::knobChanged(MixerLine* ref, int nKnob) {
	int nLine = findMixerLineByRef(ref);
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	pHydrogen->setSelectedInstrumentNumber( nLine );
	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	pSelectedInstrument->set_fx_level( ref->getFXLevel(nKnob), nKnob );
	
	QString sMessage = tr( "Set FX %1 level [%2] of instrument" )
		.arg( nKnob )
		.arg( ref->getFXLevel(nKnob), 0, 'f', 2 );
	sMessage.append( QString( " [%1]" )
					 .arg( ref->getName() ) );
	QString sCaller = QString( "%1:knobChanged:%2" )
		.arg( class_name() ).arg( ref->getName() );
	
	( HydrogenApp::get_instance() )->
		showStatusBarMessage( sMessage, sCaller );

	pHydrogen->setIsModified( true );
}

void Mixer::noteOnEvent( int nInstrument ) {
	if ( nInstrument >= 0 && nInstrument < MAX_INSTRUMENTS ) {
		if ( m_mixerLines[ nInstrument ] != nullptr ) {
			m_mixerLines[ nInstrument ]->setActivity( 100 );
		}
	} else {
		ERRORLOG( QString( "Selected MixerLine [%1] out of bound [0,%2)" )
				  .arg( nInstrument ).arg( MAX_INSTRUMENTS ) );
	}
}


void Mixer::hideEvent( QHideEvent *ev ) {
	UNUSED( ev );
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

void Mixer::ladspaBypassBtnClicked( LadspaFXLine* ref ) {
#ifdef H2CORE_HAVE_LADSPA
	bool bActive = ! ref->isFxBypassed();

	for (int nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_ladspaFXLines[ nFX ] ) {
			LadspaFX *pFX = Effects::get_instance()->getLadspaFX(nFX);
			if (pFX) {
				pFX->setEnabled( bActive );
			}
			break;
		}
	}
#else
	QMessageBox::critical( this, "Hydrogen", tr("LADSPA effects are not available in this version of Hydrogen.") );
#endif
}

void Mixer::ladspaEditBtnClicked( LadspaFXLine *ref ) {
#ifdef H2CORE_HAVE_LADSPA

	for (int nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_ladspaFXLines[ nFX ] ) {
			HydrogenApp::get_instance()->getLadspaFXProperties(nFX)->hide();
			HydrogenApp::get_instance()->getLadspaFXProperties(nFX)->show();
		}
	}
	Hydrogen::get_instance()->setIsModified( true );
#else
	QMessageBox::critical( this, "Hydrogen", tr("LADSPA effects are not available in this version of Hydrogen.") );
#endif
}

void Mixer::ladspaVolumeChanged( LadspaFXLine* ref) {
#ifdef H2CORE_HAVE_LADSPA
	for (int nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_ladspaFXLines[ nFX ] ) {
			LadspaFX *pFX = Effects::get_instance()->getLadspaFX(nFX);
			if ( pFX != nullptr ) {
				pFX->setVolume( ref->getVolume() );

				QString sMessage = tr( "Set volume [%1] of FX" )
					.arg( ref->getVolume(), 0, 'f', 2 );
				sMessage.append( QString( " [%1]" ).arg( pFX->getPluginName() ) );
				QString sCaller = QString( "%1:rotaryChanged:%2" )
					.arg( class_name() ).arg( pFX->getPluginName() );
	
				HydrogenApp::get_instance()->
					showStatusBarMessage( sMessage, sCaller );

				Hydrogen::get_instance()->setIsModified( true );
			}
		}
	}
#endif
}

void Mixer::getPeaksInMixerLine( int nMixerLine, float& fPeak_L, float& fPeak_R )
{
	if ( nMixerLine < 0 || nMixerLine >= MAX_INSTRUMENTS ) {
		ERRORLOG( QString( "Selected MixerLine [%1] out of bound [0,%2)" )
				  .arg( nMixerLine ).arg( MAX_INSTRUMENTS ) );
		return;
	}
	if ( m_mixerLines[ nMixerLine ] != nullptr ) {
		fPeak_L = m_mixerLines[ nMixerLine ]->getPeak_L();
		fPeak_R = m_mixerLines[ nMixerLine ]->getPeak_R();
	}
	else {
		fPeak_L = 0;
		fPeak_R = 0;
	}
}

void Mixer::openMixerSettingsDialog() {
	MixerSettingsDialog mixerSettingsDialog( this ); // use this as *parent because button makes smaller fonts
	mixerSettingsDialog.exec();
}

void Mixer::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		setFont( QFont( pPref->getTheme().m_font.m_sApplicationFontFamily, 10 ) );
	}
}
