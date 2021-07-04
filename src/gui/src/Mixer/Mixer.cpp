/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/AudioEngine.h>
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

#define MIXER_STRIP_WIDTH	56
#define MASTERMIXER_STRIP_WIDTH	126

const char* Mixer::__class_name = "Mixer";

Mixer::Mixer( QWidget* pParent )
 : QWidget( pParent )
 , Object( __class_name )
{
	setWindowTitle( tr( "Mixer" ) );
	setMaximumHeight( 284 );
	setMinimumHeight( 284 );
	setFixedHeight( 284 );

// fader Panel
	m_pFaderHBox = new QHBoxLayout();
	m_pFaderHBox->setSpacing( 0 );
	m_pFaderHBox->setMargin( 0 );

	m_pFaderPanel = new QWidget( nullptr );
	m_pFaderPanel->resize( MIXER_STRIP_WIDTH * MAX_INSTRUMENTS, height() );

	m_pFaderPanel->setLayout( m_pFaderHBox );

	m_pFaderScrollArea = new QScrollArea( nullptr );
	m_pFaderScrollArea->setFrameShape( QFrame::NoFrame );
	m_pFaderScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pFaderScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_pFaderScrollArea->setMinimumWidth( MIXER_STRIP_WIDTH * 4 );
	m_pFaderScrollArea->setWidget( m_pFaderPanel );

	for ( uint i = 0; i < MAX_INSTRUMENTS; ++i ) {
		m_pMixerLine[ i ] = nullptr;
	}

//~ fader panel


// fX frame
	m_pFXFrame = new PixmapWidget( nullptr );
	m_pFXFrame->setFixedSize( 213, height() );
	m_pFXFrame->setPixmap( "/mixerPanel/background_FX.png" );
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXLine[nFX] = new LadspaFXMixerLine( m_pFXFrame );
		m_pLadspaFXLine[nFX]->move( 13, 43 * nFX + 84 );
		connect( m_pLadspaFXLine[nFX], SIGNAL( activeBtnClicked(LadspaFXMixerLine*) ), this, SLOT( ladspaActiveBtnClicked( LadspaFXMixerLine*) ) );
		connect( m_pLadspaFXLine[nFX], SIGNAL( editBtnClicked(LadspaFXMixerLine*) ), this, SLOT( ladspaEditBtnClicked( LadspaFXMixerLine*) ) );
		connect( m_pLadspaFXLine[nFX], SIGNAL( volumeChanged(LadspaFXMixerLine*) ), this, SLOT( ladspaVolumeChanged( LadspaFXMixerLine*) ) );
	}

	if ( Preferences::get_instance()->isFXTabVisible() ) {
		m_pFXFrame->show();
	}
	else {
		m_pFXFrame->hide();
	}
//~ fX frame


// Master frame
	m_pMasterLine = new MasterMixerLine( nullptr );
	m_pMasterLine->move( 0, 0 );
	connect( m_pMasterLine, SIGNAL( volumeChanged(MasterMixerLine*) ), this, SLOT( masterVolumeChanged(MasterMixerLine*) ) );
	
	m_pOpenMixerSettingsBtn = new Button( m_pMasterLine, QSize( 17, 17 ), Button::Type::Push, "cog.svg", "", false, QSize( 13, 13 ), tr( "Mixer Settings" ) );
	m_pOpenMixerSettingsBtn->move( 96, 6 );
	connect( m_pOpenMixerSettingsBtn, SIGNAL( pressed() ), this, SLOT( openMixerSettingsDialog() ) );


	m_pShowFXPanelBtn = new Button( m_pMasterLine, QSize( 49, 15 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getFXButton(), false, QSize(), tr( "Show FX panel" ) );
	m_pShowFXPanelBtn->move( 63, 243 );
	m_pShowFXPanelBtn->setChecked(false);
	connect( m_pShowFXPanelBtn, SIGNAL( pressed() ), this, SLOT( showFXPanelClicked() ));
	m_pShowFXPanelBtn->setChecked( Preferences::get_instance()->isFXTabVisible() );

#ifndef H2CORE_HAVE_LADSPA
	m_pShowFXPanelBtn->hide();
#endif

	m_pShowPeaksBtn = new Button( m_pMasterLine, QSize( 49, 15 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getPeakButton(), false, QSize(), tr( "Show instrument peaks" ) );
	m_pShowPeaksBtn->move( 63, 259 );
	m_pShowPeaksBtn->setChecked( (Preferences::get_instance())->showInstrumentPeaks() );
	connect( m_pShowPeaksBtn, SIGNAL( pressed() ), this, SLOT( showPeaksBtnClicked() ));
//~ Master frame


	// LAYOUT!
	QHBoxLayout *pLayout = new QHBoxLayout();
	pLayout->setSpacing( 0 );
	pLayout->setMargin( 0 );

	pLayout->addWidget( m_pFaderScrollArea );
	pLayout->addWidget( m_pFXFrame );
	pLayout->addWidget( m_pMasterLine );
	this->setLayout( pLayout );


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
	bool isMuteClicked = ref->isMuteClicked();

	DrumkitComponent *pCompo = Hydrogen::get_instance()->getSong()->getComponent( ref->getComponentID() );

	pCompo->set_muted( isMuteClicked );
}

void Mixer::soloClicked(ComponentMixerLine* ref)
{
	bool isSoloClicked = ref->isSoloClicked();
	int nLine = findCompoMixerLineByRef(ref);
	
	ComponentMixerLine* pComponentMixerLine = m_pComponentMixerLine[nLine];
	
	pComponentMixerLine->setSoloClicked( isSoloClicked );
}

void Mixer::volumeChanged(ComponentMixerLine* ref)
{
	float newVolume = ref->getVolume();

	DrumkitComponent *pCompo = Hydrogen::get_instance()->getSong()->getComponent( ref->getComponentID() );

	pCompo->set_volume( newVolume );
}

void Mixer::soloClicked(MixerLine* ref)
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	CoreActionController* pController = pHydrogen->getCoreActionController();
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	int nInstruments = pInstrList->size();

	int nLine = findMixerLineByRef(ref);

	pController->setStripIsSoloed( nLine, ref->isSoloClicked() );

	for ( int i = 0; i < nInstruments; ++i ) {
			if( m_pMixerLine[i] ){
				m_pMixerLine[i]->setSoloClicked( pInstrList->get(i)->is_soloed() );
			}
	}

	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}



/// used in PatternEditorInstrumentList
void Mixer::soloClicked(uint nLine)
{
	MixerLine * pMixerLine = m_pMixerLine[ nLine ];

	if( pMixerLine ){
		pMixerLine->setSoloClicked( !pMixerLine->isSoloClicked() );
		soloClicked( pMixerLine );
	}
	

}

bool Mixer::isSoloClicked( uint n )
{
	if ( n >= MAX_INSTRUMENTS || m_pMixerLine[ n ] == nullptr ) {
		return false;
	}
	return m_pMixerLine[ n ]->isSoloClicked();
}

void Mixer::noteOnClicked( MixerLine* ref )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();

	int nLine = findMixerLineByRef( ref );
	pHydrogen->setSelectedInstrumentNumber( nLine );

	auto pInstr = Hydrogen::get_instance()->getSong()->getInstrumentList()->get( nLine );
	
	const float fPitch = pInstr->get_pitch_offset();
	Note *pNote = new Note( pInstr, 0, 1.0, 0.f, -1, fPitch );
	pHydrogen->getAudioEngine()->getSampler()->noteOn(pNote);
}



/// Play sample button, right-clicked (note off)
 void Mixer::noteOffClicked( MixerLine* ref )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	
	int nLine = findMixerLineByRef( ref );
	pHydrogen->setSelectedInstrumentNumber( nLine );

	auto pInstr = Hydrogen::get_instance()->getSong()->getInstrumentList()->get( nLine );

	const float fPitch = 0.0f;
	Note *pNote = new Note( pInstr, 0, 1.0, 0.f,-1, fPitch );
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
	Preferences *pPref = Preferences::get_instance();
	bool bShowPeaks = pPref->showInstrumentPeaks();

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	AudioEngine *pAudioEngine = pHydrogen->getAudioEngine();
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	std::vector<DrumkitComponent*>* pDrumkitComponentList = pSong->getComponents();

	uint nSelectedInstr = pHydrogen->getSelectedInstrumentNumber();

	float fallOff = pPref->getMixerFalloffSpeed();

	int nInstruments = pInstrList->size();
	int nCompo = pDrumkitComponentList->size();
	for ( unsigned nInstr = 0; nInstr < MAX_INSTRUMENTS; ++nInstr ) {

		if ( nInstr >= nInstruments ) {	// unused instrument! let's hide and destroy the mixerline!
			if ( m_pMixerLine[ nInstr ] ) {
				delete m_pMixerLine[ nInstr ];
				m_pMixerLine[ nInstr ] = nullptr;

				int newWidth = MIXER_STRIP_WIDTH * ( nInstruments + nCompo );
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

				int newWidth = MIXER_STRIP_WIDTH * ( nInstruments + nCompo );
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

			int newWidth = MIXER_STRIP_WIDTH * ( nInstruments + nCompo );
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
			for ( std::vector<DrumkitComponent*>::iterator it2 = pDrumkitComponentList->begin() ; it2 != pDrumkitComponentList->end(); ++it2 ) {
				DrumkitComponent* pComponent = *it2;
				if( pComponent->get_id() == it->first ) {
					bFoundExistingRelatedComponent = true;
					break;
				}
			}
			if( !bFoundExistingRelatedComponent ) {
				IdsToDelete.push_back( it->first ) ;
			}
		}

		for ( const int nCompoID : IdsToDelete ) {
			delete m_pComponentMixerLine[nCompoID];
			m_pComponentMixerLine.erase( nCompoID );

			int newWidth = MIXER_STRIP_WIDTH * ( nInstruments + nCompo );
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
			m_pLadspaFXLine[nFX]->setFxActive( pFX->isEnabled() );
			m_pLadspaFXLine[nFX]->setVolume( pFX->getVolume() );
		}
		else {
			m_pLadspaFXLine[nFX]->setName( "No plugin" );
			m_pLadspaFXLine[nFX]->setFxActive( false );
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
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	auto pInstr = pInstrList->get(nLine);
	pInstr->set_fx_level( ref->getFXLevel(nKnob), nKnob );
	QString sInfo = tr( "Set FX %1 level ").arg( nKnob + 1 );
	( HydrogenApp::get_instance() )->setStatusBarMessage( sInfo+ QString( "[%1]" ).arg( ref->getFXLevel(nKnob), 0, 'f', 2 ), 2000 );

	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}



void Mixer::noteOnEvent( int nInstrument )
{
	if ( m_pMixerLine[ nInstrument ] ) {
		m_pMixerLine[ nInstrument ]->setActivity( 100 );
	}
}



void Mixer::resizeEvent ( QResizeEvent *ev )
{
	UNUSED( ev );
}

void Mixer::showFXPanelClicked()
{
	if ( ! m_pShowFXPanelBtn->isChecked() ) {
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

	if ( ! m_pShowPeaksBtn->isChecked() ) {
		pPref->setInstrumentPeaks( true );
		( HydrogenApp::get_instance() )->setStatusBarMessage( tr( "Show instrument peaks = On"), 2000 );
	} else {
		pPref->setInstrumentPeaks( false );
		( HydrogenApp::get_instance() )->setStatusBarMessage( tr( "Show instrument peaks = Off"), 2000 );
	}
}



void Mixer::ladspaActiveBtnClicked( LadspaFXMixerLine* ref )
{
#ifdef H2CORE_HAVE_LADSPA
	bool bActive = ref->isFxActive();

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
	Hydrogen::get_instance()->getSong()->setIsModified( true );
#else
	QMessageBox::critical( this, "Hydrogen", tr("LADSPA effects are not available in this version of Hydrogen.") );
#endif
}



void Mixer::ladspaVolumeChanged( LadspaFXMixerLine* ref)
{
#ifdef H2CORE_HAVE_LADSPA
	Song *pSong = (Hydrogen::get_instance() )->getSong();
	pSong->setIsModified( true );

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			LadspaFX *pFX = Effects::get_instance()->getLadspaFX(nFX);
			if (pFX) {
				pFX->setVolume( ref->getVolume() );
				QString sInfo = tr( "Set LADSPA FX ( %1 ) volume").arg( QString(pFX->getPluginName() ) );
				HydrogenApp::get_instance()->setStatusBarMessage( sInfo+ QString( " [%1]" ).arg( ref->getVolume(), 0, 'f', 2 ), 2000 );
			}
		}
	}
#endif
}




void Mixer::getPeaksInMixerLine( uint nMixerLine, float& fPeak_L, float& fPeak_R )
{
	if ( nMixerLine < MAX_INSTRUMENTS ) {
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
