/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Mixer.cpp,v 1.20 2005/07/09 21:43:11 wsippel Exp $
 *
 */

#include "Mixer.h"
#include "MixerLine.h"

#include "gui/Skin.h"
#include "gui/widgets/Button.h"
#include "gui/HydrogenApp.h"
#include "gui/LadspaFXProperties.h"
#include "gui/PatternEditor/PatternEditorPanel.h"
#include "gui/InstrumentEditor/InstrumentEditor.h"

#include "lib/Hydrogen.h"
#include "lib/Song.h"
#include "lib/Preferences.h"
#include "lib/fx/LadspaFX.h"

#include <qinputdialog.h>

#define MIXER_STRIP_WIDTH		56
#define MASTERMIXER_STRIP_WIDTH	126


Mixer::Mixer( QWidget* pParent )
 : QWidget( pParent, "Mixer", Qt::WStyle_DialogBorder )
 , Object( "Mixer" )
{
	unsigned nMinimumVisibleFadersSize;

	m_nMixerHeight = 284;
	nMinimumVisibleFadersSize = 5 + MIXER_STRIP_WIDTH * 16 + 5;
	uint fadersSize = 5 + MIXER_STRIP_WIDTH * MAX_INSTRUMENTS;

	m_nMixerWidth = 5 + MIXER_STRIP_WIDTH * 4 + 70;	// 4 instruments visible
	setMinimumSize( m_nMixerWidth, m_nMixerHeight );

	setMaximumSize( fadersSize +  70, m_nMixerHeight );
	m_nMixerWidth = 5 + MIXER_STRIP_WIDTH * 16 + 70;	// 16 instruments visible
	resize( m_nMixerWidth, m_nMixerHeight );
	setCaption( trUtf8( "Mixer" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	// Background image
	string background_path = Skin::getImagePath().c_str() + string( "/mixerPanel/mixer_background.png" );
	bool ok = m_backgroundPixmap.load( background_path.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap " + background_path );
	}

	// FX background image
	string fx_background_path = Skin::getImagePath().c_str() + string( "/mixerPanel/background_FX.png" );
	ok = m_fxBackgroundPixmap.load( fx_background_path.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap " + background_path );
	}


	// fader scroll view
	m_pFaderScrollView = new QScrollView( this );
	m_pFaderScrollView->setFrameShape( QFrame::NoFrame );
	m_pFaderScrollView->move( 0, 0 );
	m_pFaderScrollView->setVScrollBarMode( QScrollView::AlwaysOff );
	m_pFaderScrollView->setHScrollBarMode( QScrollView::AlwaysOn );
	m_pFaderScrollView->resize( nMinimumVisibleFadersSize, m_nMixerHeight - 16 );

	// fader frame
	m_pFaderFrame = new QFrame( m_pFaderScrollView->viewport() );
	m_pFaderFrame->resize( fadersSize, m_nMixerHeight - 16 );
	m_pFaderScrollView->addChild( m_pFaderFrame );
	m_pFaderFrame->setPaletteBackgroundPixmap( m_backgroundPixmap );

	// fX frame
	m_pFXFrame = new QFrame( this );
	m_pFXFrame->resize( 213, m_nMixerHeight );
	m_pFXFrame->setPaletteBackgroundPixmap( m_fxBackgroundPixmap );

	setPaletteBackgroundPixmap( m_fxBackgroundPixmap );

	m_pFaderScrollView->show();
	m_pFXFrame->hide();

	setupMixer();

	QTimer *pTimer = new QTimer( this );
	connect( pTimer, SIGNAL( timeout() ), this, SLOT( updateMixer() ) );
	pTimer->start(50);

	HydrogenApp::getInstance()->addEventListener( this );
}



Mixer::~Mixer()
{
}



/// Setup the mixer strips
void Mixer::setupMixer()
{
	uint nMaster_X = m_nMixerWidth - MASTERMIXER_STRIP_WIDTH;

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->getInstrumentList();

	uint xPos = 0;
	uint nInstruments = instrList->getSize();
	for ( uint i = 0; i < MAX_INSTRUMENTS; ++i ) {	// MIXER LINE
		xPos = 5 + MIXER_STRIP_WIDTH * i;

		float volume = 0.2f;
		bool mute = false;
		bool solo = false;

		Instrument *instr = NULL;
		if (i < nInstruments) {
			instr = instrList->get( i );
			volume = instr->m_fVolume;
			mute = instr->m_bIsMuted;
		}

		m_pMixerLine[i] = new MixerLine( m_pFaderFrame );
		m_pMixerLine[i]->move( xPos, 7 );
		m_pMixerLine[i]->setVolume( volume );
		m_pMixerLine[i]->setMuteClicked( mute );
		m_pMixerLine[i]->setSoloClicked( solo );
		m_pMixerLine[i]->updateMixerLine();

		connect( m_pMixerLine[i], SIGNAL( noteOnClicked(MixerLine*) ), this, SLOT( noteOnClicked(MixerLine*) ) );
		connect( m_pMixerLine[i], SIGNAL( noteOffClicked(MixerLine*) ), this, SLOT( noteOffClicked(MixerLine*) ) );
		connect( m_pMixerLine[i], SIGNAL( muteBtnClicked(MixerLine*) ), this, SLOT( muteClicked(MixerLine*) ) );
		connect( m_pMixerLine[i], SIGNAL( soloBtnClicked(MixerLine*) ), this, SLOT( soloClicked(MixerLine*) ) );
		connect( m_pMixerLine[i], SIGNAL( volumeChanged(MixerLine*) ), this, SLOT( volumeChanged(MixerLine*) ) );
		connect( m_pMixerLine[i], SIGNAL( instrumentNameClicked(MixerLine*) ), this, SLOT( nameClicked(MixerLine*) ) );
		connect( m_pMixerLine[i], SIGNAL( instrumentNameSelected(MixerLine*) ), this, SLOT( nameSelected(MixerLine*) ) );
		connect( m_pMixerLine[i], SIGNAL( panChanged(MixerLine*) ), this, SLOT( panChanged( MixerLine*) ) );
		connect( m_pMixerLine[i], SIGNAL( knobChanged(MixerLine*, int) ), this, SLOT( knobChanged( MixerLine*, int) ) );
	}

	// LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXLine[nFX] = new LadspaFXMixerLine( m_pFXFrame );
		m_pLadspaFXLine[nFX]->move( 13, 43 * nFX + 84 );
		connect( m_pLadspaFXLine[nFX], SIGNAL( activeBtnClicked(LadspaFXMixerLine*) ), this, SLOT( ladspaActiveBtnClicked( LadspaFXMixerLine*) ) );
		connect( m_pLadspaFXLine[nFX], SIGNAL( editBtnClicked(LadspaFXMixerLine*) ), this, SLOT( ladspaEditBtnClicked( LadspaFXMixerLine*) ) );
		connect( m_pLadspaFXLine[nFX], SIGNAL( volumeChanged(LadspaFXMixerLine*) ), this, SLOT( ladspaVolumeChanged( LadspaFXMixerLine*) ) );
	}
	// ~LADSPA

	m_pFXFrame->move( nMaster_X - MIXER_STRIP_WIDTH * 3 + 5, 0 );

	m_pMasterLine = new MasterMixerLine( this );
	m_pMasterLine->move( nMaster_X, 0 );
	connect( m_pMasterLine, SIGNAL( volumeChanged(MasterMixerLine*) ), this, SLOT( masterVolumeChanged(MasterMixerLine*) ) );

	string showFX_on_path = Skin::getImagePath().c_str() + string( "/mixerPanel/showFX_on.png");
	string showFX_off_path = Skin::getImagePath().c_str() + string( "/mixerPanel/showFX_off.png");
	string showFX_over_path = Skin::getImagePath().c_str() + string( "/mixerPanel/showFX_over.png");
	m_pShowFXPanelBtn = new ToggleButton( this, QSize(42, 13), showFX_on_path, showFX_off_path, showFX_over_path );
	m_pShowFXPanelBtn->setPressed(false);
	QToolTip::add( m_pShowFXPanelBtn, trUtf8( "Show FX panel" ) );
	connect( m_pShowFXPanelBtn, SIGNAL(clicked(Button*)), this, SLOT( showFXPanelClicked(Button*)));

	string showPeaks_on_path = Skin::getImagePath().c_str() + string( "/mixerPanel/showPeaks_on.png");
	string showPeaks_off_path = Skin::getImagePath().c_str() + string( "/mixerPanel/showPeaks_off.png");
	string showPeaks_over_path = Skin::getImagePath().c_str() + string( "/mixerPanel/showPeaks_over.png");
	m_pShowPeaksBtn = new ToggleButton( this, QSize(42, 13), showPeaks_on_path, showPeaks_off_path, showPeaks_over_path );
	m_pShowPeaksBtn->setPressed( (Preferences::getInstance())->showInstrumentPeaks() );
	QToolTip::add( m_pShowPeaksBtn, trUtf8( "Show instrument peaks" ) );
	connect( m_pShowPeaksBtn, SIGNAL(clicked(Button*)), this, SLOT( showPeaksBtnClicked(Button*)));
}



void Mixer::muteClicked(MixerLine* ref) {
	int nLine = findMixerLineByRef(ref);
	Hydrogen::getInstance()->setSelectedInstrumentNumber( nLine );
	bool isMuteClicked = ref->isMuteClicked();

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->getInstrumentList();

	Instrument *pInstr = instrList->get(nLine);
	pInstr->m_bIsMuted = isMuteClicked;
	//(HydrogenApp::getInstance())->setSelectedInstrument(nLine);
	Hydrogen::getInstance()->setSelectedInstrumentNumber(nLine);
}



void Mixer::soloClicked(MixerLine* ref) {
	Hydrogen *pEngine = Hydrogen::getInstance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();


	int nLine = findMixerLineByRef(ref);
	pEngine->setSelectedInstrumentNumber( nLine );
	bool isSoloClicked = ref->isSoloClicked();

	if (isSoloClicked) {
		for ( uint i = 0; i < MAX_INSTRUMENTS; ++i ) {
			m_pMixerLine[i]->setSoloClicked( false );
			m_pMixerLine[i]->setMuteClicked( true );
			pInstrList->get( i )->m_bIsMuted = true;
		}
		m_pMixerLine[nLine]->setSoloClicked( true );
		m_pMixerLine[nLine]->setMuteClicked( false );
		pInstrList->get( nLine )->m_bIsMuted = false;
	}
	else {
		for ( uint i = 0; i < MAX_INSTRUMENTS; ++i ) {
			m_pMixerLine[i]->setMuteClicked( false );
			m_pMixerLine[i]->setSoloClicked( false );
			pInstrList->get( i )->m_bIsMuted = false;
		}
	}

	Hydrogen::getInstance()->setSelectedInstrumentNumber(nLine);
}



/// used in PatternEditorInstrumentList
void Mixer::soloClicked(uint nLine)
{
	Hydrogen *pEngine = Hydrogen::getInstance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	bool isSoloClicked = m_pMixerLine[ nLine ]->isSoloClicked();

	if (!isSoloClicked) {
		for (uint i = 0; i < MAX_INSTRUMENTS; i++) {
			m_pMixerLine[i]->setSoloClicked( false );
			m_pMixerLine[i]->setMuteClicked( true );
			pInstrList->get( i )->m_bIsMuted = true;
		}
		m_pMixerLine[nLine]->setSoloClicked( true );
		m_pMixerLine[nLine]->setMuteClicked( false );
		pInstrList->get( nLine )->m_bIsMuted = false;
	}
	else {
		for (uint i = 0; i < MAX_INSTRUMENTS; i++) {
			m_pMixerLine[i]->setMuteClicked( false );
			m_pMixerLine[i]->setSoloClicked( false );
			pInstrList->get( i )->m_bIsMuted = false;
		}
	}

}



void Mixer::noteOnClicked( MixerLine* ref )
{
	int nLine = findMixerLineByRef( ref );
	Hydrogen::getInstance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->getInstrumentList();

	const float fPitch = 0.0f;
	Note *note = new Note( instrList->get(nLine), 0, 1.0, 1.0f, 1.0f, -1, fPitch );
	engine->noteOn( note );

	Hydrogen::getInstance()->setSelectedInstrumentNumber(nLine);
}



/// Play sample button, right-clicked (note off)
 void Mixer::noteOffClicked( MixerLine* ref )
{
	int nLine = findMixerLineByRef( ref );
	Hydrogen::getInstance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->getInstrumentList();

	const float fPitch = 0.0f;
	Note *note = new Note( instrList->get( nLine ), 0, 1.0, 1.0, 1.0, -1, fPitch );
	engine->noteOff( note );

	Hydrogen::getInstance()->setSelectedInstrumentNumber(nLine);
}



uint Mixer::findMixerLineByRef(MixerLine* ref) {
	for (uint i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pMixerLine[i] == ref) {
			return i;
		}
	}
	return 0;
}



void Mixer::volumeChanged(MixerLine* ref) {

	int nLine = findMixerLineByRef(ref);
	Hydrogen::getInstance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->getInstrumentList();

	Instrument *instr = instrList->get(nLine);

	instr->m_fVolume = ref->getVolume();

	Hydrogen::getInstance()->setSelectedInstrumentNumber(nLine);
}




void Mixer::masterVolumeChanged(MasterMixerLine* ref) {
	float volume = ref->getVolume();
	Song *song = (HydrogenApp::getInstance())->getSong();
	song->setVolume(volume);
}



void Mixer::updateMixer() {
	Preferences *pPref = Preferences::getInstance();
	bool bShowPeaks = pPref->showInstrumentPeaks();

	Hydrogen *pEngine = Hydrogen::getInstance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	uint nSelectedInstr = pEngine->getSelectedInstrumentNumber();

	float fallOff = pPref->getMixerFalloffSpeed();

	uint nMuteClicked = 0;
	uint nInstruments = pInstrList->getSize();
	for ( unsigned i = 0; i < MAX_INSTRUMENTS; ++i ) {

//		pEngine->lockEngine( "Mixer::updateMixer" );

		Instrument *pInstr = pInstrList->get(i);

		float fNewPeak_L = pInstr->m_fPeak_L;
		pInstr->m_fPeak_L = 0.0f;	// reset instrument peak

		float fNewPeak_R = pInstr->m_fPeak_R;
		pInstr->m_fPeak_R = 0.0f;	// reset instrument peak

		float fNewVolume = pInstr->m_fVolume;
		bool bMuted = pInstr->m_bIsMuted;

		string sName = pInstr->m_sName;
		float fPan_L = pInstr->m_fPan_L;
		float fPan_R = pInstr->m_fPan_R;

//		pEngine->unlockEngine();



		// fader
		float fOldPeak_L = m_pMixerLine[i]->getPeak_L();
		float fOldPeak_R = m_pMixerLine[i]->getPeak_R();

		if (!bShowPeaks) {
			fNewPeak_L = 0.0f;
			fNewPeak_R = 0.0f;
		}

		if ( fNewPeak_L >= fOldPeak_L) {	// LEFT peak
			m_pMixerLine[i]->setPeak_L( fNewPeak_L );
		}
		else {
			m_pMixerLine[i]->setPeak_L( fOldPeak_L / fallOff );
		}
		if ( fNewPeak_R >= fOldPeak_R) {	// Right peak
			m_pMixerLine[i]->setPeak_R( fNewPeak_R );
		}
		else {
			m_pMixerLine[i]->setPeak_R( fOldPeak_R / fallOff );
		}

		// fader position
		m_pMixerLine[i]->setVolume( fNewVolume );

		// mute
		if ( bMuted ) {
			nMuteClicked++;
		}
		m_pMixerLine[i]->setMuteClicked( bMuted );

		// instr name
		m_pMixerLine[i]->setName( sName.c_str() );

		// pan
		float fPanValue = 0.0;
		if (fPan_R == 1.0) {
			fPanValue = 1.0 - (fPan_L / 2.0);
		}
		else {
			fPanValue = fPan_R / 2.0;
		}
		fPanValue = fPanValue * 100.0;

		m_pMixerLine[i]->setPan( (int)fPanValue );	/// \todo perche' setPan prende un'intero???

		// activity
		if ( m_pMixerLine[i]->getActivity() > 0 ) {
			m_pMixerLine[i]->setActivity( m_pMixerLine[i]->getActivity() - 30 );
			m_pMixerLine[i]->setPlayClicked( true );
		}
		else {
			m_pMixerLine[i]->setPlayClicked( false );
		}

		for (uint nFX = 0; nFX < MAX_FX; nFX++) {
			m_pMixerLine[i]->setFXLevel( nFX, pInstr->getFXLevel( nFX ) );
		}

		if ( i == nSelectedInstr) {
			m_pMixerLine[i]->setSelected( true );
		}
		else {
			m_pMixerLine[i]->setSelected( false );
		}

		m_pMixerLine[i]->updateMixerLine();
	}

	if (nMuteClicked == MAX_INSTRUMENTS - 1) {
		// find the not muted button
		for (uint i = 0; i < nInstruments; i++) {
			Instrument *instr = pInstrList->get(i);
			if (instr->m_bIsMuted == false) {
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
	float newVolume = pSong->getVolume();
	float oldVolume = m_pMasterLine->getVolume();
	if (oldVolume != newVolume) {
		m_pMasterLine->setVolume(newVolume);
	}
	m_pMasterLine->updateMixerLine();


#ifdef LADSPA_SUPPORT
	// LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		LadspaFX *pFX = pSong->getLadspaFX( nFX );
		if ( pFX ) {
			m_pLadspaFXLine[nFX]->setName( pFX->getPluginName().c_str() );
			float fNewPeak_L = 0.0;
			float fNewPeak_R = 0.0;
//			pEngine->getLadspaFXPeak( nFX, &fNewPeak_L, &fNewPeak_R );
	//		pEngine->setLadspaFXPeak( nFX, 0.0, 0.0 );	// reset

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
// 			m_pLadspaFXLine[nFX]->setName( trUtf8("No plugin") );
			m_pLadspaFXLine[nFX]->setFxActive( false );
			m_pLadspaFXLine[nFX]->setVolume( 0.0 );
		}
	}
	// ~LADSPA
#endif
}



/// show event
void Mixer::showEvent ( QShowEvent *ev ) {
	updateMixer();
}



/// hide event
void Mixer::hideEvent ( QHideEvent *ev ) {
}



void Mixer::nameClicked(MixerLine* ref)
{
	( HydrogenApp::getInstance()->getInstrumentEditor() )->show();
}



void Mixer::nameSelected(MixerLine* ref)
{
	int nLine = findMixerLineByRef(ref);
	Hydrogen::getInstance()->setSelectedInstrumentNumber( nLine );

	Hydrogen::getInstance()->setSelectedInstrumentNumber(nLine);
}



void Mixer::panChanged(MixerLine* ref) {
	float panValue = ref->getPan();

	float pan_L = (100.0 - panValue) / 100.0;
	float pan_R = panValue / 100.0;

	panValue = panValue / 100.0;

	if (panValue >= 0.5) {
		pan_L = (1.0 - panValue) * 2;
		pan_R = 1.0;
	}
	else {
		pan_L = 1.0;
		pan_R = ( 1.0 - ( 1.0 - panValue) ) * 2;
	}

	int nLine = findMixerLineByRef(ref);
	Hydrogen::getInstance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->getInstrumentList();

	Instrument *instr = instrList->get(nLine);
	instr->m_fPan_L = pan_L;
	instr->m_fPan_R = pan_R;


	Hydrogen::getInstance()->setSelectedInstrumentNumber(nLine);
}



void Mixer::knobChanged(MixerLine* ref, int nKnob) {
	int nLine = findMixerLineByRef(ref);
	Hydrogen::getInstance()->setSelectedInstrumentNumber( nLine );

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->getInstrumentList();
	Instrument *pInstr = instrList->get(nLine);
	pInstr->setFXLevel( nKnob, ref->getFXLevel(nKnob) );
	QString sInfo = trUtf8( "Set FX %1 level ").arg( nKnob + 1 );
	( HydrogenApp::getInstance() )->setStatusBarMessage( sInfo+ QString( "[%1]" ).arg( ref->getFXLevel(nKnob), 0, 'f', 2 ), 2000 );

	Hydrogen::getInstance()->setSelectedInstrumentNumber(nLine);
}



void Mixer::noteOnEvent( int nInstrument )
{
	m_pMixerLine[ nInstrument ]->setActivity( 100 );
}



void Mixer::resizeEvent ( QResizeEvent *ev )
{
	uint nMaster_X = width() - MASTERMIXER_STRIP_WIDTH;
	int nFXFrameWidth = 213;

	if ( m_pFXFrame->isVisible() ) {
		int w = width() - MASTERMIXER_STRIP_WIDTH - nFXFrameWidth;
		m_pFaderScrollView->resize( w, m_nMixerHeight );
	}
	else {
		int w = width() - MASTERMIXER_STRIP_WIDTH;
		m_pFaderScrollView->resize( w, m_nMixerHeight );
	}

	m_pFXFrame->move( nMaster_X - nFXFrameWidth, 0 );

	m_pMasterLine->move( nMaster_X, 0);
	m_pShowPeaksBtn->move( nMaster_X + 67, 242 );
	m_pShowFXPanelBtn->move( nMaster_X + 67, 258 );

	resize(width(), m_nMixerHeight);	// qt bug workaround
}



void Mixer::showFXPanelClicked(Button* ref)
{
	if ( ref->isPressed() ) {
		m_pFXFrame->show();
	}
	else {
		m_pFXFrame->hide();
	}

	resizeEvent( NULL ); 	// force an update
}



void Mixer::showPeaksBtnClicked(Button* ref)
{
	Preferences *pPref = Preferences::getInstance();

	if ( ref->isPressed() ) {
		pPref->setInstrumentPeaks( true );
		( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Show instrument peaks = On"), 2000 );
	}
	else {
		pPref->setInstrumentPeaks( false );
		( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Show instrument peaks = Off"), 2000 );
	}
}



void Mixer::ladspaActiveBtnClicked( LadspaFXMixerLine* ref )
{
#ifdef LADSPA_SUPPORT
	bool bActive = ref->isFxActive();

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			LadspaFX *pFX = song->getLadspaFX(nFX);
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
			( HydrogenApp::getInstance() )->getLadspaFXProperties(nFX)->hide();
			( HydrogenApp::getInstance() )->getLadspaFXProperties(nFX)->show();
		}
	}
	(Hydrogen::getInstance() )->getSong()->m_bIsModified = true;
#endif
}



void Mixer::ladspaVolumeChanged( LadspaFXMixerLine* ref)
{
#ifdef LADSPA_SUPPORT
	Song *pSong = (Hydrogen::getInstance() )->getSong();
	pSong->m_bIsModified = true;

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		if (ref == m_pLadspaFXLine[ nFX ] ) {
			LadspaFX *pFX = pSong->getLadspaFX(nFX);
			if (pFX) {
				pFX->setVolume( ref->getVolume() );
				QString sInfo = trUtf8( "Set LADSPA FX ( %1 ) volume").arg( QString(pFX->getPluginName().c_str()) );
				( HydrogenApp::getInstance() )->setStatusBarMessage( sInfo+ QString( " [%1]" ).arg( ref->getVolume(), 0, 'f', 2 ), 2000 );
			}
		}
	}
#endif
}




void Mixer::getPeaksInMixerLine( uint nMixerLine, float& fPeak_L, float& fPeak_R )
{
	if ( (nMixerLine >= 0 ) && (nMixerLine < MAX_INSTRUMENTS) ) {
		fPeak_L = m_pMixerLine[ nMixerLine ]->getPeak_L();
		fPeak_R = m_pMixerLine[ nMixerLine ]->getPeak_R();
	}
	else {
		fPeak_L = 0;
		fPeak_R = 0;
	}
}


