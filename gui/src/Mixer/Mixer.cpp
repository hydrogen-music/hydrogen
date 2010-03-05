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
#include <hydrogen/instrument.h>
#include <hydrogen/Song.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/note.h>
#include <hydrogen/fx/Effects.h>
using namespace H2Core;

#include <cassert>

#define MIXER_STRIP_WIDTH	56
#define MASTERMIXER_STRIP_WIDTH	126


Mixer::Mixer( QWidget* pParent )
 : QWidget( pParent )
// : QWidget( pParent, Qt::WindowStaysOnTopHint )
// : QWidget( pParent, Qt::Tool )
 , Object( "Mixer" )
{
	setWindowTitle( trUtf8( "Mixer" ) );
	setMaximumHeight( 284 );
	setMinimumHeight( 284 );
	setFixedHeight( 284 );
	setWindowIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

// fader Panel
	m_pFaderHBox = new QHBoxLayout();
	m_pFaderHBox->setSpacing( 0 );
	m_pFaderHBox->setMargin( 0 );

	m_pFaderPanel = new QWidget( NULL );
	m_pFaderPanel->resize( MIXER_STRIP_WIDTH * MAX_INSTRUMENTS, height() );

	m_pFaderPanel->setLayout( m_pFaderHBox );

	m_pFaderScrollArea = new QScrollArea( NULL );
	m_pFaderScrollArea->setFrameShape( QFrame::NoFrame );
	m_pFaderScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pFaderScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_pFaderScrollArea->setMinimumWidth( MIXER_STRIP_WIDTH * 4 );
	m_pFaderScrollArea->setWidget( m_pFaderPanel );

	for ( uint i = 0; i < MAX_INSTRUMENTS; ++i ) {
		m_pMixerLine[ i ] = NULL;
	}

//~ fader panel


// fX frame
	m_pFXFrame = new PixmapWidget( NULL );
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
	m_pMasterLine = new MasterMixerLine( NULL );
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

#ifndef LADSPA_SUPPORT
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
	MixerLine *pMixerLine = new MixerLine( 0 , nInstr);
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


void Mixer::muteClicked(MixerLine* ref)
{
	int nLine = findMixerLineByRef(ref);
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );
	bool isMuteClicked = ref->isMuteClicked();

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *pInstr = instrList->get(nLine);
	pInstr->set_muted( isMuteClicked);
	//(HydrogenApp::get_instance())->setSelectedInstrument(nLine);
	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}



void Mixer::soloClicked(MixerLine* ref)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();
	int nInstruments = pInstrList->get_size();

	int nLine = findMixerLineByRef(ref);
	pEngine->setSelectedInstrumentNumber( nLine );
	bool isSoloClicked = ref->isSoloClicked();

	if (isSoloClicked) {
		for ( int i = 0; i < nInstruments; ++i ) {
			m_pMixerLine[i]->setSoloClicked( false );
			m_pMixerLine[i]->setMuteClicked( true );
			pInstrList->get( i )->set_muted( true );
		}
		m_pMixerLine[nLine]->setSoloClicked( true );
		m_pMixerLine[nLine]->setMuteClicked( false );
		pInstrList->get( nLine )->set_muted( false );
	}
	else {
		for ( int i = 0; i < nInstruments; ++i ) {
			m_pMixerLine[i]->setMuteClicked( false );
			m_pMixerLine[i]->setSoloClicked( false );
			pInstrList->get( i )->set_muted( false );
		}
	}

	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}



/// used in PatternEditorInstrumentList
void Mixer::soloClicked(uint nLine)
{
	MixerLine * L = m_pMixerLine[ nLine ];
	L->setSoloClicked( !L->isSoloClicked() );
	soloClicked( L );
// 	Hydrogen *pEngine = Hydrogen::get_instance();
// 	Song *pSong = pEngine->getSong();
// 	InstrumentList *pInstrList = pSong->get_instrument_list();
// 	int nInstruments = pInstrList->get_size();
// 
// 	bool isSoloClicked = m_pMixerLine[ nLine ]->isSoloClicked();
// 
// 	if (!isSoloClicked) {
// 		for ( int i = 0; i < nInstruments; i++ ) {
// 			m_pMixerLine[i]->setSoloClicked( false );
// 			m_pMixerLine[i]->setMuteClicked( true );
// 			pInstrList->get( i )->set_muted( true );
// 		}
// 		m_pMixerLine[nLine]->setSoloClicked( true );
// 		m_pMixerLine[nLine]->setMuteClicked( false );
// 		pInstrList->get( nLine )->set_muted( false );
// 	}
// 	else {
// 		for ( int i = 0; i < nInstruments; i++ ) {
// 			m_pMixerLine[i]->setMuteClicked( false );
// 			m_pMixerLine[i]->setSoloClicked( false );
// 			pInstrList->get( i )->set_muted( false );
// 		}
// 	}

}

bool Mixer::isSoloClicked( uint n )
{
	if ( n >= MAX_INSTRUMENTS || m_pMixerLine[ n ] == NULL ) {
		return false;
	}
	return m_pMixerLine[ n ]->isSoloClicked();
}

void Mixer::noteOnClicked( MixerLine* ref )
{
	int nLine = findMixerLineByRef( ref );
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	const float fPitch = 0.0f;
	Note *note = new Note( instrList->get(nLine), 0, 1.0, 0.5f, 0.5f, -1, fPitch );
	AudioEngine::get_instance()->get_sampler()->note_on(note);

	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}



/// Play sample button, right-clicked (note off)
 void Mixer::noteOffClicked( MixerLine* ref )
{
	int nLine = findMixerLineByRef( ref );
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	const float fPitch = 0.0f;
	Note *note = new Note( instrList->get( nLine ), 0, 1.0, 0.5, 0.5, -1, fPitch );
	AudioEngine::get_instance()->get_sampler()->note_off(note);

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



void Mixer::volumeChanged(MixerLine* ref)
{
	int nLine = findMixerLineByRef(ref);
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get(nLine);

	instr->set_volume( ref->getVolume() );

	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}




void Mixer::masterVolumeChanged(MasterMixerLine* ref)
{
	float volume = ref->getVolume();
	Song *song = Hydrogen::get_instance()->getSong();
	song->set_volume(volume);
}



void Mixer::updateMixer()
{
	Preferences *pPref = Preferences::get_instance();
	bool bShowPeaks = pPref->showInstrumentPeaks();

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();

	uint nSelectedInstr = pEngine->getSelectedInstrumentNumber();

	float fallOff = pPref->getMixerFalloffSpeed();

	uint nMuteClicked = 0;
	uint nInstruments = pInstrList->get_size();
	for ( unsigned nInstr = 0; nInstr < MAX_INSTRUMENTS; ++nInstr ) {

		if ( nInstr >= nInstruments ) {	// unused instrument! let's hide and destroy the mixerline!
			if ( m_pMixerLine[ nInstr ] ) {
				delete m_pMixerLine[ nInstr ];
				m_pMixerLine[ nInstr ] = NULL;

				int newWidth = MIXER_STRIP_WIDTH * nInstruments;
				if ( m_pFaderPanel->width() != newWidth ) {
					m_pFaderPanel->resize( newWidth, height() );
				}
			}
			continue;
		}
		else {
			if ( m_pMixerLine[ nInstr ] == NULL ) {
				// the mixerline doesn't exists..I'll create a new one!
				m_pMixerLine[ nInstr ] = createMixerLine( nInstr );
				m_pFaderHBox->addWidget( m_pMixerLine[ nInstr ] );

				int newWidth = MIXER_STRIP_WIDTH * nInstruments;
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

	if (nMuteClicked == nInstruments - 1 ) {
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


#ifdef LADSPA_SUPPORT
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
	float panValue = ref->getPan();

	float pan_L;
	float pan_R;

	if (panValue >= 0.5) {
		pan_L = (1.0 - panValue) * 2;
		pan_R = 1.0;
	}
	else {
		pan_L = 1.0;
		pan_R = panValue * 2;
	}

	int nLine = findMixerLineByRef(ref);
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *instr = instrList->get(nLine);
	instr->set_pan_l( pan_L );
	instr->set_pan_r( pan_R );


	Hydrogen::get_instance()->setSelectedInstrumentNumber(nLine);
}



void Mixer::knobChanged(MixerLine* ref, int nKnob) {
	int nLine = findMixerLineByRef(ref);
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->get_instrument_list();
	Instrument *pInstr = instrList->get(nLine);
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
/*
	uint nMaster_X = width() - MASTERMIXER_STRIP_WIDTH;
	int nFXFrameWidth = 213;

	if ( m_pFXFrame->isVisible() ) {
		int w = width() - MASTERMIXER_STRIP_WIDTH - nFXFrameWidth;
		m_pFaderScrollArea->resize( w, m_nMixerHeight );
	}
	else {
		int w = width() - MASTERMIXER_STRIP_WIDTH;
		m_pFaderScrollArea->resize( w, m_nMixerHeight );
	}

	m_pFXFrame->move( nMaster_X - nFXFrameWidth, 0 );

	m_pMasterLine->move( nMaster_X, 0);
	m_pShowPeaksBtn->move( nMaster_X + 67, 242 );
	m_pShowFXPanelBtn->move( nMaster_X + 67, 258 );

	resize(width(), m_nMixerHeight);	// qt bug workaround
*/
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

	resizeEvent( NULL ); 	// force an update
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
#ifdef LADSPA_SUPPORT
	bool bActive = ref->isFxActive();

	//Hydrogen *engine = Hydrogen::get_instance();
	//Song *song = engine->getSong();

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			LadspaFX *pFX = Effects::get_instance()->getLadspaFX(nFX);
			if (pFX) {
				pFX->setEnabled( bActive );
			}
			break;
		}
	}
#endif
}



void Mixer::ladspaEditBtnClicked( LadspaFXMixerLine *ref )
{
#ifdef LADSPA_SUPPORT

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			HydrogenApp::get_instance()->getLadspaFXProperties(nFX)->hide();
			HydrogenApp::get_instance()->getLadspaFXProperties(nFX)->show();
		}
	}
	Hydrogen::get_instance()->getSong()->__is_modified = true;
#endif
}



void Mixer::ladspaVolumeChanged( LadspaFXMixerLine* ref)
{
#ifdef LADSPA_SUPPORT
	Song *pSong = (Hydrogen::get_instance() )->getSong();
	pSong->__is_modified = true;

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
