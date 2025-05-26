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
#include "MixerLine.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../LadspaFXProperties.h"
#include "../InstrumentEditor/InstrumentEditorPanel.h"
#include "../Widgets/Button.h"
#include "../Widgets/PixmapWidget.h"
#include "MixerSettingsDialog.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/Basics/Note.h>
#include <core/FX/Effects.h>
using namespace H2Core;

#include <cassert>

Mixer::Mixer( QWidget* pParent )
 : QWidget( pParent )
{
	setWindowTitle( tr( "Mixer" ) );

	const int nMinimumFaderPanelWidth = MixerLine::nWidth * 4;
	const int nFXFrameWidth = 213;
	const int nFixedHeight = MasterMixerLine::nHeight;

	setMinimumSize( nMinimumFaderPanelWidth +
					nFXFrameWidth + MasterMixerLine::nWidth +
					8, // Small margin for scrollbar
					nFixedHeight + 6 );

// fader Panel
	m_pFaderHBox = new QHBoxLayout();
	m_pFaderHBox->setSpacing( 0 );
	m_pFaderHBox->setContentsMargins( 0, 0, 0, 0 );

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

	for ( uint i = 0; i < MAX_INSTRUMENTS; ++i ) {
		m_pMixerLine[ i ] = nullptr;
	}

// ~ fader panel


// fX frame
#ifdef H2CORE_HAVE_LADSPA
	auto pEffects = Effects::get_instance();
#endif
	m_pFXFrame = new PixmapWidget( nullptr );
	m_pFXFrame->setObjectName( "MixerFXRack" );
	m_pFXFrame->setFixedSize( nFXFrameWidth, nFixedHeight );
	m_pFXFrame->setPixmap( "/mixerPanel/background_FX.png" );
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXLine[nFX] = new LadspaFXMixerLine( m_pFXFrame );
		m_pLadspaFXLine[nFX]->setObjectName( "LadspaFXMixerLine" );
		m_pLadspaFXLine[nFX]->move( 13, 43 * nFX + 84 );
#ifdef H2CORE_HAVE_LADSPA
		if ( pEffects != nullptr ) {
			auto pFx = pEffects->getLadspaFX( nFX );
			if ( pFx != nullptr ) {
				m_pLadspaFXLine[nFX]->setFxBypassed( pEffects->getLadspaFX( nFX )->isEnabled() );
			}
		}
#endif
		connect( m_pLadspaFXLine[nFX], SIGNAL( bypassBtnClicked(LadspaFXMixerLine*) ), this, SLOT( ladspaBypassBtnClicked( LadspaFXMixerLine*) ) );
		connect( m_pLadspaFXLine[nFX], SIGNAL( editBtnClicked(LadspaFXMixerLine*) ), this, SLOT( ladspaEditBtnClicked( LadspaFXMixerLine*) ) );
		connect( m_pLadspaFXLine[nFX], SIGNAL( volumeChanged(LadspaFXMixerLine*) ), this, SLOT( ladspaVolumeChanged( LadspaFXMixerLine*) ) );
	}

	if ( Preferences::get_instance()->isFXTabVisible() ) {
		m_pFXFrame->show();
	}
	else {
		m_pFXFrame->hide();
	}
// ~ fX frame


// Master frame
	m_pMasterLine = new MasterMixerLine( nullptr );
	m_pMasterLine->setObjectName( "MasterMixerLine" );
	connect( m_pMasterLine, SIGNAL( volumeChanged(MasterMixerLine*) ), this, SLOT( masterVolumeChanged(MasterMixerLine*) ) );
	
	m_pOpenMixerSettingsBtn = new Button( m_pMasterLine, QSize( 17, 17 ), Button::Type::Push, "cog.svg", "", false, QSize( 13, 13 ), tr( "Mixer Settings" ) );
	m_pOpenMixerSettingsBtn->setObjectName( "MixerSettingsButton" );
	m_pOpenMixerSettingsBtn->move( 96, 6 );
	connect( m_pOpenMixerSettingsBtn, SIGNAL( clicked() ), this, SLOT( openMixerSettingsDialog() ) );


	m_pShowFXPanelBtn = new Button( m_pMasterLine, QSize( 49, 15 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getFXButton(), false, QSize(), tr( "Show FX panel" ) );
	m_pShowFXPanelBtn->setObjectName( "MixerShowFXButton" );
	m_pShowFXPanelBtn->move( 63, 243 );
	m_pShowFXPanelBtn->setChecked(false);
	connect( m_pShowFXPanelBtn, SIGNAL( clicked() ), this, SLOT( showFXPanelClicked() ));
	m_pShowFXPanelBtn->setChecked( Preferences::get_instance()->isFXTabVisible() );

#ifndef H2CORE_HAVE_LADSPA
	m_pShowFXPanelBtn->hide();
#endif

	m_pShowPeaksBtn = new Button( m_pMasterLine, QSize( 49, 15 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getPeakButton(), false, QSize(), tr( "Show instrument peaks" ) );
	m_pShowPeaksBtn->setObjectName( "MixerShowPeaksButton" );
	m_pShowPeaksBtn->move( 63, 259 );
	m_pShowPeaksBtn->setChecked( (Preferences::get_instance())->showInstrumentPeaks() );
	connect( m_pShowPeaksBtn, SIGNAL( clicked() ), this, SLOT( showPeaksBtnClicked() ));
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
								 nFXFrameWidth + MasterMixerLine::nWidth, nFixedHeight );
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
	connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateMixer() ) );
	m_pUpdateTimer->start(50);

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &Mixer::onPreferencesChanged );


	HydrogenApp::get_instance()->addEventListener( this );
}

Mixer::~Mixer()
{
	m_pUpdateTimer->stop();
}

MixerLine* Mixer::createMixerLine( int nInstr )
{
	MixerLine *pMixerLine = new MixerLine( nullptr , nInstr);
	pMixerLine->setObjectName( "MixerLine" );
	pMixerLine->setVolume( 0.2 );
	pMixerLine->setMuteClicked( false );
	pMixerLine->setSoloClicked( false );

	connect( pMixerLine, SIGNAL( noteOnClicked(MixerLine*) ), this, SLOT( noteOnClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( noteOffClicked(MixerLine*) ), this, SLOT( noteOffClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( muteBtnClicked(MixerLine*) ), this, SLOT( muteClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( soloBtnClicked(MixerLine*) ), this, SLOT( soloClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( volumeChanged(MixerLine*) ), this, SLOT( volumeChanged(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( instrumentNameClicked(MixerLine*) ), this, SLOT( nameClicked(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( instrumentNameSelected(MixerLine*) ), this, SLOT( nameSelected(MixerLine*) ) );
	connect( pMixerLine, SIGNAL( panChanged(MixerLine*) ), this, SLOT( panChanged( MixerLine*) ) );
	connect( pMixerLine, SIGNAL( knobChanged(MixerLine*, int) ), this, SLOT( knobChanged( MixerLine*, int) ) );

	return pMixerLine;
}

void Mixer::closeEvent( QCloseEvent* ev )
{
	HydrogenApp::get_instance()->showMixer(false);
}


ComponentMixerLine* Mixer::createComponentMixerLine( int theCompoID )
{
	ComponentMixerLine *pMixerLine = new ComponentMixerLine( nullptr , theCompoID);
	pMixerLine->setObjectName( "ComponentMixerLine" );
	pMixerLine->setVolume( 0.2 );
	pMixerLine->setMuteClicked( false );
	pMixerLine->setSoloClicked( false );

	connect( pMixerLine, SIGNAL( muteBtnClicked(ComponentMixerLine*) ), this, SLOT( muteClicked(ComponentMixerLine*) ) );
	connect( pMixerLine, SIGNAL( soloBtnClicked(ComponentMixerLine*) ), this, SLOT( soloClicked(ComponentMixerLine*) ) );
	connect( pMixerLine, SIGNAL( volumeChanged(ComponentMixerLine*) ), this, SLOT( volumeChanged(ComponentMixerLine*) ) );

	return pMixerLine;
}


void Mixer::muteClicked(MixerLine* ref)
{
	int nLine = findMixerLineByRef(ref);
	bool isMuteClicked = ref->isMuteClicked();

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	CoreActionController* pController = pHydrogen->getCoreActionController();
	pHydrogen->setSelectedInstrumentNumber( nLine );

	pController->setStripIsMuted( nLine, isMuteClicked );
}

void Mixer::muteClicked(ComponentMixerLine* ref)
{
	auto pSong = Hydrogen::get_instance()->getSong();
	bool isMuteClicked = ref->isMuteClicked();

	auto pCompo = pSong->getComponent( ref->getComponentID() );

	pCompo->set_muted( isMuteClicked );
	Hydrogen::get_instance()->setIsModified( true );
}

void Mixer::soloClicked(ComponentMixerLine* ref)
{
	bool isSoloClicked = ref->isSoloClicked();
	int nLine = findCompoMixerLineByRef(ref);
	
	ComponentMixerLine* pComponentMixerLine = m_pComponentMixerLine[nLine];
	
	pComponentMixerLine->setSoloClicked( isSoloClicked );
	Hydrogen::get_instance()->setIsModified( true );
}

void Mixer::volumeChanged(ComponentMixerLine* ref)
{
	auto pSong = Hydrogen::get_instance()->getSong();
	float newVolume = ref->getVolume();

	auto pCompo = pSong->getComponent( ref->getComponentID() );

	pCompo->set_volume( newVolume );
	Hydrogen::get_instance()->setIsModified( true );
}

void Mixer::soloClicked(MixerLine* ref)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	CoreActionController* pController = pHydrogen->getCoreActionController();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pInstrList = pSong->getInstrumentList();
	int nInstruments = std::min( pInstrList->size(), MAX_INSTRUMENTS );

	int nLine = findMixerLineByRef(ref);

	pController->setStripIsSoloed( nLine, ref->isSoloClicked() );

	for ( int i = 0; i < nInstruments; ++i ) {
			if ( m_pMixerLine[i] != nullptr ){
				auto pInstrument = pInstrList->get(i);
				if ( pInstrument != nullptr ) {
					m_pMixerLine[i]->setSoloClicked( pInstrument->is_soloed() );
				}
			}
	}

	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}



/// used in PatternEditorInstrumentList
void Mixer::soloClicked(uint nLine)
{
	if ( nLine < 0 || nLine >= MAX_INSTRUMENTS ) {
		ERRORLOG( QString( "Selected MixerLine [%1] out of bound [0,%2)" )
				  .arg( nLine ).arg( MAX_INSTRUMENTS ) );
		return;
	}

	MixerLine * pMixerLine = m_pMixerLine[ nLine ];

	if( pMixerLine != nullptr ){
		pMixerLine->setSoloClicked( !pMixerLine->isSoloClicked() );
		soloClicked( pMixerLine );
	}
	

}

bool Mixer::isSoloClicked( uint nLine )
{
	if ( nLine < 0 || nLine >= MAX_INSTRUMENTS ) {
		ERRORLOG( QString( "Selected MixerLine [%1] out of bound [0,%2)" )
				  .arg( nLine ).arg( MAX_INSTRUMENTS ) );
		return false;
	}
	if ( m_pMixerLine[ nLine ] == nullptr ) {
		return false;
	}
	return m_pMixerLine[ nLine ]->isSoloClicked();
}

void Mixer::noteOnClicked( MixerLine* ref )
{
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
	
	Note *pNote = new Note( pSelectedInstrument, 0, 1.0 );
	pHydrogen->getAudioEngine()->getSampler()->noteOn(pNote);
}



/// Play sample button, right-clicked (note off)
 void Mixer::noteOffClicked( MixerLine* ref )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	
	int nLine = findMixerLineByRef( ref );
	pHydrogen->setSelectedInstrumentNumber( nLine );
	auto pSelectedInstrument = pHydrogen->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	Note *pNote = new Note( pSelectedInstrument, 0, 0.0 );
	pNote->set_note_off( true );
	pHydrogen->getAudioEngine()->getSampler()->noteOff(pNote);
}



uint Mixer::findMixerLineByRef(MixerLine* ref)
{
	for (uint i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pMixerLine[i] == ref) {
			return i;
		}
	}
	return 0;
}


uint Mixer::findCompoMixerLineByRef(ComponentMixerLine* ref)
{
	for (std::map<int, ComponentMixerLine*>::iterator it=m_pComponentMixerLine.begin(); it!=m_pComponentMixerLine.end(); ++it) {
		if(it->second == ref) {
			return it->first;
		}
	}

	return 0;
}


void Mixer::volumeChanged(MixerLine* ref)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	CoreActionController* pController = pHydrogen->getCoreActionController();

	int nLine = findMixerLineByRef(ref);
	pController->setStripVolume( nLine, ref->getVolume(), true );
}

void Mixer::masterVolumeChanged(MasterMixerLine* ref)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	CoreActionController* pController = pHydrogen->getCoreActionController();

	float Volume = ref->getVolume();
	pController->setMasterVolume( Volume );
}



void Mixer::updateMixer()
{
	if ( ! isVisible() ) {
		// Skip redundant updates if mixer is not visible.
		return;
	}
	Preferences *pPref = Preferences::get_instance();
	bool bShowPeaks = pPref->showInstrumentPeaks();

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	AudioEngine *pAudioEngine = pHydrogen->getAudioEngine();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pInstrList = pSong->getInstrumentList();
	auto pDrumkitComponentList = pSong->getComponents();

	uint nSelectedInstr = pHydrogen->getSelectedInstrumentNumber();

	float fallOff = pPref->getMixerFalloffSpeed();

	int nInstruments = pInstrList->size();
	int nCompo = pDrumkitComponentList->size();
	for ( unsigned nInstr = 0; nInstr < MAX_INSTRUMENTS; ++nInstr ) {

		if ( nInstr >= nInstruments ) {	// unused instrument! let's hide and destroy the mixerline!
			if ( m_pMixerLine[ nInstr ] ) {
				delete m_pMixerLine[ nInstr ];
				m_pMixerLine[ nInstr ] = nullptr;

				int newWidth = MixerLine::nWidth * ( nInstruments + nCompo );
				if ( m_pFaderPanel->width() != newWidth ) {
					m_pFaderPanel->resize( newWidth, height() );
				}
			}
			continue;
		}
		else {
			if ( m_pMixerLine[ nInstr ] == nullptr ) {
				// the mixerline doesn't exists..I'll create a new one!
				m_pMixerLine[ nInstr ] = createMixerLine( nInstr );
				m_pFaderHBox->insertWidget( nInstr, m_pMixerLine[ nInstr ] );

				int newWidth = MixerLine::nWidth * ( nInstruments + nCompo );
				if ( m_pFaderPanel->width() != newWidth ) {
					m_pFaderPanel->resize( newWidth, height() );
				}
			}
			MixerLine *pLine = m_pMixerLine[ nInstr ];

			auto pInstr = pInstrList->get( nInstr );
			assert( pInstr );

			float fNewPeak_L = pInstr->get_peak_l();
			pInstr->set_peak_l( 0.0f );	// reset instrument peak

			float fNewPeak_R = pInstr->get_peak_r();
			pInstr->set_peak_r( 0.0f );	// reset instrument peak

			QString sName = pInstr->get_name();

			// fader
			float fOldPeak_L = pLine->getPeak_L();
			float fOldPeak_R = pLine->getPeak_R();

			if (!bShowPeaks) {
				fNewPeak_L = 0.0f;
				fNewPeak_R = 0.0f;
			}

			if ( fNewPeak_L >= fOldPeak_L) {	// LEFT peak
				pLine->setPeak_L( fNewPeak_L );
			}
			else {
				pLine->setPeak_L( fOldPeak_L / fallOff );
			}
			if ( fNewPeak_R >= fOldPeak_R) {	// Right peak
				pLine->setPeak_R( fNewPeak_R );
			}
			else {
				pLine->setPeak_R( fOldPeak_R / fallOff );
			}

			// fader position
			float fNewVolume = pInstr->get_volume();
			float fOldVolume = pLine->getVolume();
			if ( fOldVolume != fNewVolume ) {
				pLine->setVolume( fNewVolume );
			}

			// mute / solo
			pLine->setMuteClicked( pInstr->is_muted() );
			pLine->setSoloClicked( pInstr->is_soloed() );

			// instr name
			pLine->setName( sName );

			// pan
			float fPan = pInstr->getPan();
			if ( fPan != pLine->getPan() ) {
				pLine->setPan( fPan );
			}

			// activity
			if ( pLine->getActivity() > 0 ) {
				pLine->setActivity( m_pMixerLine[ nInstr ]->getActivity() - 30 );
				pLine->setPlayClicked( true );
			}
			else {
				pLine->setPlayClicked( false );
			}

			for (uint nFX = 0; nFX < MAX_FX; nFX++) {
				pLine->setFXLevel( nFX, pInstr->get_fx_level( nFX ) );
			}

			pLine->setSelected( nInstr == nSelectedInstr );

			pLine->updateMixerLine();
		}
	}

	for (auto& pDrumkitComponent : *pDrumkitComponentList) {

		if( m_pComponentMixerLine.find(pDrumkitComponent->get_id()) == m_pComponentMixerLine.end() ) {
			// the mixerline doesn't exists..I'll create a new one!
			m_pComponentMixerLine[ pDrumkitComponent->get_id() ] = createComponentMixerLine( pDrumkitComponent->get_id() );
			m_pFaderHBox->addWidget( m_pComponentMixerLine[ pDrumkitComponent->get_id() ] );

			int newWidth = MixerLine::nWidth * ( nInstruments + nCompo );
			if ( m_pFaderPanel->width() != newWidth ) {
				m_pFaderPanel->resize( newWidth, height() );
			}
		}

		ComponentMixerLine *pLine = m_pComponentMixerLine[ pDrumkitComponent->get_id() ];

		float fNewPeak_L = pDrumkitComponent->get_peak_l();
		pDrumkitComponent->set_peak_l( 0.0f );	// reset instrument peak

		float fNewPeak_R = pDrumkitComponent->get_peak_r();
		pDrumkitComponent->set_peak_r( 0.0f );	// reset instrument peak

		bool bMuted = pDrumkitComponent->is_muted();

		QString sName = pDrumkitComponent->get_name();

		float fOldPeak_L = pLine->getPeak_L();
		float fOldPeak_R = pLine->getPeak_R();

		if (!bShowPeaks) {
			fNewPeak_L = 0.0f;
			fNewPeak_R = 0.0f;
		}

		if ( fNewPeak_L >= fOldPeak_L) {	// LEFT peak
			pLine->setPeak_L( fNewPeak_L );
		}
		else {
			pLine->setPeak_L( fOldPeak_L / fallOff );
		}
		if ( fNewPeak_R >= fOldPeak_R) {	// Right peak
			pLine->setPeak_R( fNewPeak_R );
		}
		else {
			pLine->setPeak_R( fOldPeak_R / fallOff );
		}

		// fader position
		float fNewVolume = pDrumkitComponent->get_volume();
		float fOldVolume = pLine->getVolume();
		if (fOldVolume != fNewVolume) {
			pLine->setVolume(fNewVolume);
		}

		// mute
		pLine->setMuteClicked( bMuted );

		// instr name
		pLine->setName( sName );

		pLine->updateMixerLine();
	}

	if( pDrumkitComponentList->size() < m_pComponentMixerLine.size() ) {
		std::vector<int> IdsToDelete;
		for (std::map<int, ComponentMixerLine*>::iterator it=m_pComponentMixerLine.begin(); it!=m_pComponentMixerLine.end(); ++it) {

			bool bFoundExistingRelatedComponent = false;
			for ( const auto& pComponent : *pDrumkitComponentList ) {
				if ( pComponent->get_id() == it->first ) {
					bFoundExistingRelatedComponent = true;
					break;
				}
			}
			if ( !bFoundExistingRelatedComponent ) {
				IdsToDelete.push_back( it->first ) ;
			}
		}

		for ( const int nCompoID : IdsToDelete ) {
			delete m_pComponentMixerLine[nCompoID];
			m_pComponentMixerLine.erase( nCompoID );

			int newWidth = MixerLine::nWidth * ( nInstruments + nCompo );
			if ( m_pFaderPanel->width() != newWidth ) {
				m_pFaderPanel->resize( newWidth, height() );
			}
		}
	}


	// update MasterPeak
	float fOldPeak_L = m_pMasterLine->getPeak_L();
	float fNewPeak_L = pAudioEngine->getMasterPeak_L();
	pAudioEngine->setMasterPeak_L(0.0);
	float fOldPeak_R = m_pMasterLine->getPeak_R();
	float fNewPeak_R = pAudioEngine->getMasterPeak_R();
	pAudioEngine->setMasterPeak_R(0.0);

	if (!bShowPeaks) {
		fNewPeak_L = 0.0;
		fNewPeak_R = 0.0;
	}

	if (fNewPeak_L >= fOldPeak_L) {
		m_pMasterLine->setPeak_L( fNewPeak_L );
	}
	else {
		m_pMasterLine->setPeak_L( fOldPeak_L / fallOff );
	}
	if (fNewPeak_R >= fOldPeak_R) {
		m_pMasterLine->setPeak_R(fNewPeak_R);
	}
	else {
		m_pMasterLine->setPeak_R( fOldPeak_R / fallOff );
	}


	// set master fader position
	float fNewVolume = pSong->getVolume();
	float fOldVolume = m_pMasterLine->getVolume();
	if (fOldVolume != fNewVolume) {
		m_pMasterLine->setVolume(fNewVolume);
	}
	m_pMasterLine->updateMixerLine();


#ifdef H2CORE_HAVE_LADSPA
	// LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( pFX ) {
			m_pLadspaFXLine[nFX]->setName( pFX->getPluginName() );
			float fNewPeak_L = 0.0;
			float fNewPeak_R = 0.0;

			float fOldPeak_L = 0.0;
			float fOldPeak_R = 0.0;
			m_pLadspaFXLine[nFX]->getPeaks( &fOldPeak_L, &fOldPeak_R );

			if (fNewPeak_L < fOldPeak_L)	fNewPeak_L = fOldPeak_L / fallOff;
			if (fNewPeak_R < fOldPeak_R)	fNewPeak_R = fOldPeak_R / fallOff;
			m_pLadspaFXLine[nFX]->setPeaks( fNewPeak_L, fNewPeak_R );
			m_pLadspaFXLine[nFX]->setFxBypassed( ! pFX->isEnabled() );
			m_pLadspaFXLine[nFX]->setVolume( pFX->getVolume() );
		}
		else {
			m_pLadspaFXLine[nFX]->setName( "No plugin" );
			m_pLadspaFXLine[nFX]->setFxBypassed( true );
			m_pLadspaFXLine[nFX]->setVolume( 0.0 );
		}
	}
	// ~LADSPA
#endif
}



/// show event
void Mixer::showEvent ( QShowEvent *ev )
{
	UNUSED( ev );
	updateMixer();
}



/// hide event
void Mixer::hideEvent ( QHideEvent *ev )
{
	UNUSED( ev );
}



void Mixer::nameClicked(MixerLine* ref)
{
	UNUSED( ref );
	InstrumentEditorPanel::get_instance()->show();
}



void Mixer::nameSelected(MixerLine* ref)
{
	int nLine = findMixerLineByRef(ref);
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );
}



void Mixer::panChanged(MixerLine* ref) {
	float	fPan = ref->getPan();
	int		nLine = findMixerLineByRef(ref);

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	CoreActionController* pController = pHydrogen->getCoreActionController();

	pController->setStripPanSym( nLine, fPan, true );
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



void Mixer::noteOnEvent( int nInstrument )
{
	if ( nInstrument >= 0 && nInstrument < MAX_INSTRUMENTS ) {
		if ( m_pMixerLine[ nInstrument ] != nullptr ) {
			m_pMixerLine[ nInstrument ]->setActivity( 100 );
		}
	} else {
		ERRORLOG( QString( "Selected MixerLine [%1] out of bound [0,%2)" )
				  .arg( nInstrument ).arg( MAX_INSTRUMENTS ) );
	}
}



void Mixer::resizeEvent ( QResizeEvent *ev )
{
	UNUSED( ev );
}

void Mixer::showFXPanelClicked()
{
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
	Preferences *pPref = Preferences::get_instance();

	if ( m_pShowPeaksBtn->isChecked() ) {
		pPref->setInstrumentPeaks( true );
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Show instrument peaks = On") );
	} else {
		pPref->setInstrumentPeaks( false );
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Show instrument peaks = Off") );
	}
}



void Mixer::ladspaBypassBtnClicked( LadspaFXMixerLine* ref )
{
#ifdef H2CORE_HAVE_LADSPA
	bool bActive = ! ref->isFxBypassed();

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
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



void Mixer::ladspaEditBtnClicked( LadspaFXMixerLine *ref )
{
#ifdef H2CORE_HAVE_LADSPA

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			HydrogenApp::get_instance()->getLadspaFXProperties(nFX)->hide();
			HydrogenApp::get_instance()->getLadspaFXProperties(nFX)->show();
		}
	}
	Hydrogen::get_instance()->setIsModified( true );
#else
	QMessageBox::critical( this, "Hydrogen", tr("LADSPA effects are not available in this version of Hydrogen.") );
#endif
}

void Mixer::ladspaVolumeChanged( LadspaFXMixerLine* ref)
{
#ifdef H2CORE_HAVE_LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
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

void Mixer::getPeaksInMixerLine( uint nMixerLine, float& fPeak_L, float& fPeak_R )
{
	if ( nMixerLine < 0 || nMixerLine >= MAX_INSTRUMENTS ) {
		ERRORLOG( QString( "Selected MixerLine [%1] out of bound [0,%2)" )
				  .arg( nMixerLine ).arg( MAX_INSTRUMENTS ) );
		return;
	}
	if ( m_pMixerLine[ nMixerLine ] != nullptr ) {
		fPeak_L = m_pMixerLine[ nMixerLine ]->getPeak_L();
		fPeak_R = m_pMixerLine[ nMixerLine ]->getPeak_R();
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


void Mixer::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		setFont( QFont( pPref->getApplicationFontFamily(), 10 ) );
	}
}
