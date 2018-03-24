/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Mixer.h"
#include "MixerLine.h"

#include "../Skin.h"
#include "../HydrogenApp.h"
#include "../LadspaFXProperties.h"
#include "../InstrumentEditor/InstrumentEditorPanel.h"
#include "../widgets/Button.h"
#include "../widgets/PixmapWidget.h"

#include <hydrogen/audio_engine.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/fx/Effects.h>
using namespace H2Core;

#include <cassert>

#define MIXER_STRIP_WIDTH	56
#define MASTERMIXER_STRIP_WIDTH	126

const char* Mixer::__class_name = "Mixer";

Mixer::Mixer( QWidget* pParent )
 : QWidget( pParent )
 , Object( __class_name )
{
	setWindowTitle( trUtf8( "Mixer" ) );
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

	m_pShowFXPanelBtn = new ToggleButton(
			m_pMasterLine,
			"/mixerPanel/showFX_on.png",
			"/mixerPanel/showFX_off.png",
			"/mixerPanel/showFX_over.png",
			QSize(42, 13)
	);
	m_pShowFXPanelBtn->move( 67, 242 );
	m_pShowFXPanelBtn->setPressed(false);
	m_pShowFXPanelBtn->setToolTip( trUtf8( "Show FX panel" ) );
	connect( m_pShowFXPanelBtn, SIGNAL(clicked(Button*)), this, SLOT( showFXPanelClicked(Button*)));
	m_pShowFXPanelBtn->setPressed( Preferences::get_instance()->isFXTabVisible() );

#ifndef H2CORE_HAVE_LADSPA
	m_pShowFXPanelBtn->hide();
#endif

	m_pShowPeaksBtn = new ToggleButton(
			m_pMasterLine,
			"/mixerPanel/showPeaks_on.png",
			"/mixerPanel/showPeaks_off.png",
			"/mixerPanel/showPeaks_over.png",
			QSize(42, 13)
	);
	m_pShowPeaksBtn->move( 67, 258 );
	m_pShowPeaksBtn->setPressed( (Preferences::get_instance())->showInstrumentPeaks() );
	m_pShowPeaksBtn->setToolTip( trUtf8( "Show instrument peaks" ) );
	connect( m_pShowPeaksBtn, SIGNAL(clicked(Button*)), this, SLOT( showPeaksBtnClicked(Button*)));
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

	Hydrogen *pEngine = Hydrogen::get_instance();
	CoreActionController* pController = pEngine->getCoreActionController();
	pEngine->setSelectedInstrumentNumber( nLine );

	pController->setStripIsMuted( nLine, isMuteClicked );
}

void Mixer::muteClicked(ComponentMixerLine* ref)
{
	bool isMuteClicked = ref->isMuteClicked();

	DrumkitComponent *pCompo = Hydrogen::get_instance()->getSong()->get_component( ref->getCompoID() );

	pCompo->set_muted( isMuteClicked );
}

void Mixer::soloClicked(ComponentMixerLine* ref)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	std::vector<DrumkitComponent*> pCompoList = *(pSong->get_components());
	int nComponents = pCompoList.size();

	bool isSoloClicked = ref->isSoloClicked();
	int nLine = findCompoMixerLineByRef(ref);

	if ( isSoloClicked ) {
		for ( int i = 0; i < nComponents ; ++i ) {
			ComponentMixerLine* p_tmpCompoMixer = m_pComponentMixerLine[i];
			p_tmpCompoMixer->setSoloClicked( false );
			p_tmpCompoMixer->setMuteClicked( true );
			DrumkitComponent* p_tmpCompo = pCompoList[i];
			p_tmpCompo->set_muted( true );
		}
		ComponentMixerLine* p_tmpCompoMixer = m_pComponentMixerLine[nLine];
		p_tmpCompoMixer->setSoloClicked( true );
		p_tmpCompoMixer->setMuteClicked( false );
		DrumkitComponent* p_tmpCompo = pCompoList[nLine];
		p_tmpCompo->set_muted( false );
	}
	else {
		for ( int i = 0; i < nComponents ; ++i ) {
			ComponentMixerLine* p_tmpCompoMixer = m_pComponentMixerLine[i];
			p_tmpCompoMixer->setSoloClicked( false );
			p_tmpCompoMixer->setMuteClicked( false );
			DrumkitComponent* p_tmpCompo = pCompoList[i];
			p_tmpCompo->set_muted( false );
		}
	}
}

void Mixer::volumeChanged(ComponentMixerLine* ref)
{
	float newVolume = ref->getVolume();

	DrumkitComponent *pCompo = Hydrogen::get_instance()->getSong()->get_component( ref->getCompoID() );

	pCompo->set_volume( newVolume );
}

void Mixer::unmuteAll( bool findSelectedInstr )
{
	if(findSelectedInstr)
		unmuteAll( Hydrogen::get_instance()->getSelectedInstrumentNumber() );
	else
		unmuteAll( 0 );
}

void Mixer::unmuteAll( int selectedInstrument )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();
	int nInstruments = pInstrList->size();
	for ( int i = 0; i < nInstruments; ++i ) {
		m_pMixerLine[i]->setMuteClicked( false );
		m_pMixerLine[i]->setSoloClicked( false );
		pInstrList->get( i )->set_muted( false );
	}
	// select first instrument after unmute all
	Hydrogen::get_instance()->setSelectedInstrumentNumber(selectedInstrument);
}

void Mixer::soloClicked(MixerLine* ref)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	CoreActionController* pController = pEngine->getCoreActionController();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();
	int nInstruments = pInstrList->size();

	int nLine = findMixerLineByRef(ref);

	pController->setStripIsSoloed( nLine, ref->isSoloClicked() );

	for ( int i = 0; i < nInstruments; ++i ) {
			if( m_pMixerLine[i] ){
				m_pMixerLine[i]->setSoloClicked( pInstrList->get(i)->is_soloed() );
				m_pMixerLine[i]->setMuteClicked( pInstrList->get(i)->is_muted() );
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
	int nLine = findMixerLineByRef( ref );
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *song = pEngine->getSong();
	InstrumentList *pInstrList = song->get_instrument_list();

	const float fPitch = 0.0f;
	Note *pNote = new Note( pInstrList->get(nLine), 0, 1.0, 0.5f, 0.5f, -1, fPitch );
	AudioEngine::get_instance()->get_sampler()->note_on(pNote);

	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}



/// Play sample button, right-clicked (note off)
 void Mixer::noteOffClicked( MixerLine* ref )
{
	int nLine = findMixerLineByRef( ref );
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *instrList = pSong->get_instrument_list();

	const float fPitch = 0.0f;
	Note *pNote = new Note( instrList->get( nLine ), 0, 1.0, 0.5, 0.5, -1, fPitch );
	AudioEngine::get_instance()->get_sampler()->note_off(pNote);

	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
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
		if(it->second == ref)
			return it->first;
	}

	return 0;
}


void Mixer::volumeChanged(MixerLine* ref)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	CoreActionController* pController = pEngine->getCoreActionController();

	int nLine = findMixerLineByRef(ref);
	pController->setStripVolume( nLine, ref->getVolume() );
}

void Mixer::masterVolumeChanged(MasterMixerLine* ref)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	CoreActionController* pController = pEngine->getCoreActionController();

	float Volume = ref->getVolume();
	pController->setMasterVolume( Volume );
}



void Mixer::updateMixer()
{
	Preferences *pPref = Preferences::get_instance();
	bool bShowPeaks = pPref->showInstrumentPeaks();

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();
	std::vector<DrumkitComponent*>* compoList = pSong->get_components();

	uint nSelectedInstr = pEngine->getSelectedInstrumentNumber();

	float fallOff = pPref->getMixerFalloffSpeed();

	uint nMuteClicked = 0;
	int nInstruments = pInstrList->size();
	int nCompo = compoList->size();
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

			Instrument *pInstr = pInstrList->get( nInstr );
			assert( pInstr );

			float fNewPeak_L = pInstr->get_peak_l();
			pInstr->set_peak_l( 0.0f );	// reset instrument peak

			float fNewPeak_R = pInstr->get_peak_r();
			pInstr->set_peak_r( 0.0f );	// reset instrument peak

			float fNewVolume = pInstr->get_volume();
			bool bMuted = pInstr->is_muted();

			QString sName = pInstr->get_name();
			float fPan_L = pInstr->get_pan_l();
			float fPan_R = pInstr->get_pan_r();


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
			pLine->setVolume( fNewVolume );

			// mute
			if ( bMuted ) {
				nMuteClicked++;
			}
			pLine->setMuteClicked( bMuted );

			// instr name
			pLine->setName( sName );

			// pan
			float fPanValue = 0.0;
			if (fPan_R == 1.0) {
				fPanValue = 1.0 - (fPan_L / 2.0);
			}
			else {
				fPanValue = fPan_R / 2.0;
			}

			pLine->setPan( fPanValue );

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

	for (std::vector<DrumkitComponent*>::iterator it = compoList->begin() ; it != compoList->end(); ++it) {
		DrumkitComponent* p_compo = *it;

		if( m_pComponentMixerLine.find(p_compo->get_id()) == m_pComponentMixerLine.end() ) {
			// the mixerline doesn't exists..I'll create a new one!
			m_pComponentMixerLine[ p_compo->get_id() ] = createComponentMixerLine( p_compo->get_id() );
			m_pFaderHBox->addWidget( m_pComponentMixerLine[ p_compo->get_id() ] );

			int newWidth = MIXER_STRIP_WIDTH * ( nInstruments + nCompo );
			if ( m_pFaderPanel->width() != newWidth ) {
				m_pFaderPanel->resize( newWidth, height() );
			}
		}

		ComponentMixerLine *pLine = m_pComponentMixerLine[ p_compo->get_id() ];

		float fNewPeak_L = p_compo->get_peak_l();
		p_compo->set_peak_l( 0.0f );	// reset instrument peak

		float fNewPeak_R = p_compo->get_peak_r();
		p_compo->set_peak_r( 0.0f );	// reset instrument peak

		float fNewVolume = p_compo->get_volume();
		bool bMuted = p_compo->is_muted();

		QString sName = p_compo->get_name();

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
		pLine->setVolume( fNewVolume );

		// mute
		if ( bMuted ) {
			nMuteClicked++;
		}
		pLine->setMuteClicked( bMuted );

		// instr name
		pLine->setName( sName );

		pLine->updateMixerLine();
	}

	if( compoList->size() < m_pComponentMixerLine.size() ) {
		std::vector<int>* p_ids_to_delete = new std::vector<int>();
		for (std::map<int, ComponentMixerLine*>::iterator it=m_pComponentMixerLine.begin(); it!=m_pComponentMixerLine.end(); ++it) {

			bool p_foundExistingRelatedComponent = false;
			for ( std::vector<DrumkitComponent*>::iterator it2 = compoList->begin() ; it2 != compoList->end(); ++it2 ) {
				DrumkitComponent* p_compo = *it2;
				if( p_compo->get_id() == it->first ) {
					p_foundExistingRelatedComponent = true;
					break;
				}
			}
			if( !p_foundExistingRelatedComponent )
				p_ids_to_delete->push_back( it->first ) ;
		}

		for ( std::vector<int>::iterator it = p_ids_to_delete->begin() ; it != p_ids_to_delete->end(); ++it ) {
			int p_compoID = *it;
			delete m_pComponentMixerLine[p_compoID];
			m_pComponentMixerLine.erase( p_compoID );

			int newWidth = MIXER_STRIP_WIDTH * ( nInstruments + nCompo );
			if ( m_pFaderPanel->width() != newWidth ) {
				m_pFaderPanel->resize( newWidth, height() );
			}
		}
	}

	if (nMuteClicked == nInstruments - 1) {
		// find the not muted button
		for (uint i = 0; i < nInstruments; i++) {
			Instrument *instr = pInstrList->get(i);
			if (instr->is_muted() == false) {
				m_pMixerLine[i]->setSoloClicked(true);
				break;
			}
		}
	}
	else {
		for (uint i = 0; i < nInstruments; i++) {
			m_pMixerLine[i]->setSoloClicked(false);
		}
	}


	// update MasterPeak
	float oldPeak_L = m_pMasterLine->getPeak_L();
	float newPeak_L = pEngine->getMasterPeak_L();
	pEngine->setMasterPeak_L(0.0);
	float oldPeak_R = m_pMasterLine->getPeak_R();
	float newPeak_R = pEngine->getMasterPeak_R();
	pEngine->setMasterPeak_R(0.0);

	if (!bShowPeaks) {
		newPeak_L = 0.0;
		newPeak_R = 0.0;
	}

	if (newPeak_L >= oldPeak_L) {
		m_pMasterLine->setPeak_L( newPeak_L );
	}
	else {
		m_pMasterLine->setPeak_L( oldPeak_L / fallOff );
	}
	if (newPeak_R >= oldPeak_R) {
		m_pMasterLine->setPeak_R(newPeak_R);
	}
	else {
		m_pMasterLine->setPeak_R( oldPeak_R / fallOff );
	}




	// set master fader position
	float newVolume = pSong->get_volume();
	float oldVolume = m_pMasterLine->getVolume();
	if (oldVolume != newVolume) {
		m_pMasterLine->setVolume(newVolume);
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

	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}



void Mixer::panChanged(MixerLine* ref) {
	float	panValue = ref->getPan();
	int		nLine = findMixerLineByRef(ref);

	Hydrogen *pEngine = Hydrogen::get_instance();
	CoreActionController* pController = pEngine->getCoreActionController();

	pController->setStripPan( nLine, panValue );
}



void Mixer::knobChanged(MixerLine* ref, int nKnob) {
	int nLine = findMixerLineByRef(ref);
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();
	Instrument *pInstr = pInstrList->get(nLine);
	pInstr->set_fx_level( ref->getFXLevel(nKnob), nKnob );
	QString sInfo = trUtf8( "Set FX %1 level ").arg( nKnob + 1 );
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



void Mixer::showFXPanelClicked(Button* ref)
{
	if ( ref->isPressed() ) {
		m_pFXFrame->show();
		Preferences::get_instance()->setFXTabVisible( true );
	}
	else {
		m_pFXFrame->hide();
		Preferences::get_instance()->setFXTabVisible( false );
	}

	resizeEvent( nullptr ); 	// force an update
}



void Mixer::showPeaksBtnClicked(Button* ref)
{
	Preferences *pPref = Preferences::get_instance();

	if ( ref->isPressed() ) {
		pPref->setInstrumentPeaks( true );
		( HydrogenApp::get_instance() )->setStatusBarMessage( trUtf8( "Show instrument peaks = On"), 2000 );
	}
	else {
		pPref->setInstrumentPeaks( false );
		( HydrogenApp::get_instance() )->setStatusBarMessage( trUtf8( "Show instrument peaks = Off"), 2000 );
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
	QMessageBox::critical( this, "Hydrogen", trUtf8("LADSPA effects are not available in this version of Hydrogen.") );
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
	Hydrogen::get_instance()->getSong()->set_is_modified( true );
#else
	QMessageBox::critical( this, "Hydrogen", trUtf8("LADSPA effects are not available in this version of Hydrogen.") );
#endif
}



void Mixer::ladspaVolumeChanged( LadspaFXMixerLine* ref)
{
#ifdef H2CORE_HAVE_LADSPA
	Song *pSong = (Hydrogen::get_instance() )->getSong();
	pSong->set_is_modified( true );

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			LadspaFX *pFX = Effects::get_instance()->getLadspaFX(nFX);
			if (pFX) {
				pFX->setVolume( ref->getVolume() );
				QString sInfo = trUtf8( "Set LADSPA FX ( %1 ) volume").arg( QString(pFX->getPluginName() ) );
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
