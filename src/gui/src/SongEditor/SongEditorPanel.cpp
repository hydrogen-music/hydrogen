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
#include "SongEditorPanel.h"
#include "PlaybackTrackWaveDisplay.h"

#include "../AudioFileBrowser/AudioFileBrowser.h"
#include "../HydrogenApp.h"
#include "../PatternPropertiesDialog.h"
#include "../SongPropertiesDialog.h"
#include "../Widgets/AutomationPathView.h"
#include "../Widgets/Button.h"
#include "../Widgets/Fader.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/PixmapWidget.h"
#include "../WidgetScrollArea.h"

#include "SongEditor.h"
#include "UndoActions.h"
#include "CommonStrings.h"

#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/PatternList.h>
#include <core/IO/JackAudioDriver.h>
#include <core/EventQueue.h>

#ifdef WIN32
#include <time.h>
#endif
using namespace H2Core;


SongEditorPanel::SongEditorPanel(QWidget *pParent)
 : QWidget( pParent )
 {
	m_nInitialWidth = 600;
	m_nInitialHeight = 250;
	
	Preferences *pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	Hydrogen*	pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> 		pSong = pHydrogen->getSong();

	setWindowTitle( tr( "Song Editor" ) );

	// background
	PixmapWidget *pBackPanel = new PixmapWidget( nullptr );
	pBackPanel->setObjectName( "SongEditorBackPanel" );
	pBackPanel->setFixedSize( 196, 49 );
	pBackPanel->setPixmap( "/songEditor/bg_topPanel.png" );

	// time line toggle button
	m_pTimelineBtn = new Button( pBackPanel, QSize( 98, 17 ), Button::Type::Toggle, "",
								 pCommonStrings->getTimelineBigButton(), false, QSize(),
								 pCommonStrings->getTimelineEnabled() );
	m_pTimelineBtn->move( 94, 4 );
	m_pTimelineBtn->setObjectName( "TimelineBtn" );
	connect( m_pTimelineBtn, SIGNAL( pressed() ), this, SLOT( timelineBtnPressed() ) );
	m_bLastIsTimelineActivated = pSong->getIsTimelineActivated();
	if ( pHydrogen->getJackTimebaseState() == JackAudioDriver::Timebase::Slave ) {
		m_pTimelineBtn->setToolTip( pCommonStrings->getTimelineDisabledTimebaseSlave() );
		m_pTimelineBtn->setIsActive( false );
	} else if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
		m_pTimelineBtn->setToolTip( pCommonStrings->getTimelineDisabledPatternMode() );
		m_pTimelineBtn->setIsActive( false );
	} else {
		m_pTimelineBtn->setChecked( m_bLastIsTimelineActivated );
	}

	// clear sequence button
	m_pClearPatternSeqBtn = new Button( pBackPanel,	QSize( 60, 19 ), Button::Type::Push, "", pCommonStrings->getClearButton(), false, QSize(), tr("Clear pattern sequence") );
	m_pClearPatternSeqBtn->move( 2, 26 );
	connect( m_pClearPatternSeqBtn, SIGNAL( pressed() ), this, SLOT( clearSequence() ) );

	// new pattern button
	Button *newPatBtn = new Button( pBackPanel,	QSize( 20, 19 ), Button::Type::Push, "plus.svg", "", false, QSize( 11, 11 ), tr("Create new pattern") );
	newPatBtn->move( 64, 26 );
	connect( newPatBtn, SIGNAL( pressed() ), this, SLOT( newPatBtnClicked() ) );

	// down button
	m_pDownBtn = new Button( pBackPanel, QSize( 20, 19 ), Button::Type::Push, "down.svg", "", false, QSize( 11, 11 ), tr("Move the selected pattern down") );
	m_pDownBtn->move( 87, 26 );
	connect( m_pDownBtn, SIGNAL( pressed() ), this, SLOT( downBtnClicked() ) );

	// up button
	m_pUpBtn = new Button( pBackPanel, QSize( 20, 19 ), Button::Type::Push, "up.svg", "", false, QSize( 11, 11 ), tr("Move the selected pattern up") );
	m_pUpBtn->move( 106, 26 );
	connect( m_pUpBtn, SIGNAL( pressed() ), this, SLOT( upBtnClicked() ) );

	// select toggle button
	m_pSelectionModeBtn = new Button( pBackPanel, QSize( 20, 19 ), Button::Type::Toggle, "select.svg", "", false, QSize( 15, 12 ), tr( "Select mode" ) );
	m_pSelectionModeBtn->move( 128, 26 );
	connect( m_pSelectionModeBtn, SIGNAL( pressed() ), this, SLOT( selectionModeBtnPressed() ) );

	// draw toggle button
	m_pDrawModeBtn = new Button( pBackPanel, QSize( 20, 19 ), Button::Type::Toggle, "draw.svg", "", false, QSize( 15, 12 ), tr( "Draw mode") );
	m_pDrawModeBtn->move( 147, 26 );
	connect( m_pDrawModeBtn, SIGNAL( pressed() ), this, SLOT( drawModeBtnPressed() ) );

	if ( pSong->getActionMode() == H2Core::Song::ActionMode::selectMode ) {
		m_pSelectionModeBtn->setChecked( true );
		m_pDrawModeBtn->setChecked( false );
	} else {
		m_pSelectionModeBtn->setChecked( false );
		m_pDrawModeBtn->setChecked( true );
	}

	// Two buttons sharing the same position and either of them is
	// shown unpressed.
	m_pModeActionSingleBtn = new Button( pBackPanel, QSize( 23, 19 ), Button::Type::Push, "single_layer.svg", "", false, QSize( 15, 11 ), tr( "single pattern mode") );
	m_pModeActionSingleBtn->move( 170, 26 );
	m_pModeActionSingleBtn->setVisible( pPref->patternModePlaysSelected() );
	connect( m_pModeActionSingleBtn, SIGNAL( pressed() ), this, SLOT( modeActionBtnPressed() ) );

	m_pModeActionMultipleBtn = new Button( pBackPanel, QSize( 23, 19 ), Button::Type::Push, "multiple_layers.svg", "", false, QSize( 19, 15 ), tr( "stacked pattern mode") );
	m_pModeActionMultipleBtn->move( 170, 26 );
	m_pModeActionMultipleBtn->hide();
	m_pModeActionMultipleBtn->setVisible( pPref->patternModePlaysSelected() );
	connect( m_pModeActionMultipleBtn, SIGNAL( pressed() ), this, SLOT( modeActionBtnPressed() ) );
	setModeActionBtn( Preferences::get_instance()->patternModePlaysSelected() );

// ZOOM
	m_pHScrollBar = new QScrollBar( Qt::Horizontal, nullptr );
	m_pHScrollBar->setObjectName( "SongEditorPanel HScrollBar" );
	connect( m_pHScrollBar, SIGNAL(valueChanged(int)), this, SLOT( hScrollTo(int) ) );

	// zoom-in btn
	Button* pZoomInBtn = new Button( nullptr, QSize( 19, 15 ), Button::Type::Push, "plus.svg", "", false, QSize( 9, 9 ) );
	connect( pZoomInBtn, SIGNAL( pressed() ), this, SLOT( zoomOutBtnPressed() ) );



	// zoom-out btn
	Button* pZoomOutBtn = new Button( nullptr, QSize( 19, 15 ), Button::Type::Push, "minus.svg", "", false, QSize( 9, 9 ) );
	connect( pZoomOutBtn, SIGNAL( pressed() ), this, SLOT( zoomInBtnPressed() ) );

	// view playback track toggle button
	m_pViewPlaybackBtn = new Button( nullptr, QSize( 19, 15 ), Button::Type::Toggle, "", pCommonStrings->getPlaybackTrackButton(), false, QSize(), tr( "View playback track" ) );
	m_pViewPlaybackBtn->setObjectName( "ViewPlaybackBtn" );
	connect( m_pViewPlaybackBtn, SIGNAL( pressed() ), this, SLOT( viewPlaybackTrackBtnPressed() ) );
	m_pViewPlaybackBtn->setChecked( false );
	
	// Playback Fader
	m_pPlaybackTrackFader = new Fader( pBackPanel, Fader::Type::Vertical, tr( "Playback track volume" ), false, false, 0.0, 1.5 );
	m_pPlaybackTrackFader->move( 6, 1 );
	m_pPlaybackTrackFader->setValue( pSong->getPlaybackTrackVolume() );
	m_pPlaybackTrackFader->hide();
	m_pPlaybackTrackFader->setIsActive( ! H2Core::Hydrogen::get_instance()->getSong()->getPlaybackTrackFilename().isEmpty() );
	connect( m_pPlaybackTrackFader, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( faderChanged( WidgetWithInput* ) ) );

	// mute playback track toggle button
	m_pMutePlaybackBtn = new Button( pBackPanel, QSize( 34, 17 ), Button::Type::Toggle, "", pCommonStrings->getBigMuteButton(), true, QSize(), tr( "Mute playback track" ) );
	m_pMutePlaybackBtn->move( 158, 4 );
	m_pMutePlaybackBtn->hide();
	connect( m_pMutePlaybackBtn, SIGNAL( pressed() ), this, SLOT( mutePlaybackTrackBtnPressed() ) );
	m_pMutePlaybackBtn->setChecked( !pSong->getPlaybackTrackEnabled() );
	
	// edit playback track toggle button
	m_pEditPlaybackBtn = new Button( pBackPanel, QSize( 34, 17 ), Button::Type::Push, "", pCommonStrings->getEditButton(), false, QSize(), tr( "Choose playback track") );
	m_pEditPlaybackBtn->move( 123, 4 );
	m_pEditPlaybackBtn->hide();
	connect( m_pEditPlaybackBtn, SIGNAL( pressed() ), this, SLOT( editPlaybackTrackBtnPressed() ) );
	m_pEditPlaybackBtn->setChecked( false );

	// timeline view toggle button
	m_pViewTimelineBtn = new Button( nullptr, QSize( 19, 15 ), Button::Type::Toggle, "", pCommonStrings->getTimelineButton(), false, QSize(), tr( "View timeline" ) );
	connect( m_pViewTimelineBtn, SIGNAL( pressed() ), this, SLOT( viewTimelineBtnPressed() ) );
	m_pViewTimelineBtn->setChecked( true );
	
	
	QHBoxLayout *pHZoomLayout = new QHBoxLayout();
	pHZoomLayout->setSpacing( 0 );
	pHZoomLayout->setMargin( 0 );
	pHZoomLayout->addWidget( m_pViewPlaybackBtn );
	pHZoomLayout->addWidget( m_pViewTimelineBtn );
	pHZoomLayout->addWidget( m_pHScrollBar );
	pHZoomLayout->addWidget( pZoomInBtn );
	pHZoomLayout->addWidget( pZoomOutBtn );

	QWidget *pHScrollbarPanel = new QWidget();
	pHScrollbarPanel->setLayout( pHZoomLayout );

	//~ ZOOM

	// PATTERN LIST
	m_pPatternListScrollView = new WidgetScrollArea( nullptr );
	m_pPatternListScrollView->setFrameShape( QFrame::NoFrame );
	m_pPatternListScrollView->setFixedWidth( m_nPatternListWidth );
	m_pPatternListScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPatternListScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	connect( m_pPatternListScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( vScrollTo(int) ) );

	m_pPatternList = new SongEditorPatternList( m_pPatternListScrollView->viewport() );
	m_pPatternList->setFocusPolicy( Qt::ClickFocus );
	m_pPatternListScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pPatternListScrollView->setWidget( m_pPatternList );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, m_pPatternList, &SongEditorPatternList::onPreferencesChanged );

	// EDITOR
	m_pEditorScrollView = new WidgetScrollArea( nullptr );
	m_pEditorScrollView->setFrameShape( QFrame::NoFrame );
	m_pEditorScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pEditorScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pSongEditor = new SongEditor( m_pEditorScrollView->viewport(), m_pEditorScrollView, this );
	m_pEditorScrollView->setWidget( m_pSongEditor );
	m_pEditorScrollView->setFocusPolicy( Qt::ClickFocus );

	m_pPatternList->setFocusProxy( m_pSongEditor );

	connect( m_pEditorScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( hScrollTo(int) ) );
	connect( m_pEditorScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( vScrollTo(int) ) );


	// POSITION RULER
	m_pWidgetStack = new QStackedWidget( nullptr );
	m_pWidgetStack->setFixedHeight( 50 );
	
	m_pPositionRulerScrollView = new WidgetScrollArea( m_pWidgetStack );
	m_pPositionRulerScrollView->setFrameShape( QFrame::NoFrame );
	m_pPositionRulerScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPositionRulerScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPositionRulerScrollView->setFocusPolicy( Qt::NoFocus );
	m_pPositionRuler = new SongEditorPositionRuler( m_pPositionRulerScrollView->viewport() );
	m_pPositionRulerScrollView->setWidget( m_pPositionRuler );
	m_pPositionRulerScrollView->setFixedHeight( 50 );
	connect( m_pPositionRulerScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( hScrollTo(int) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, m_pPositionRuler, &SongEditorPositionRuler::onPreferencesChanged );

	m_pPositionRuler->setFocusPolicy( Qt::ClickFocus );
	m_pPositionRuler->setFocusProxy( m_pSongEditor );

	m_pPlaybackTrackScrollView = new WidgetScrollArea( m_pWidgetStack );
	m_pPlaybackTrackScrollView->setFrameShape( QFrame::NoFrame );
	m_pPlaybackTrackScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPlaybackTrackScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	
	auto pCompo = Hydrogen::get_instance()->getAudioEngine()->getSampler()->getPlaybackTrackInstrument()->get_components()->front();
	assert(pCompo);

	m_pPlaybackTrackWaveDisplay = new PlaybackTrackWaveDisplay( m_pPlaybackTrackScrollView->viewport() );
	m_pPlaybackTrackWaveDisplay->setSampleNameAlignment( Qt::AlignLeft );
	m_pPlaybackTrackWaveDisplay->resize( m_pPositionRuler->width() , 50);
	m_pPlaybackTrackWaveDisplay->setAcceptDrops( true );
	
	m_pPlaybackTrackScrollView->setWidget( m_pPlaybackTrackWaveDisplay );
	m_pPlaybackTrackScrollView->setFixedHeight( 50 );
	
	m_pAutomationPathScrollView = new WidgetScrollArea( nullptr );
	m_pAutomationPathScrollView->setFrameShape( QFrame::NoFrame );
	m_pAutomationPathScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pAutomationPathScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pAutomationPathView = new AutomationPathView( m_pAutomationPathScrollView->viewport() );
	m_pAutomationPathScrollView->setWidget( m_pAutomationPathView );
	m_pAutomationPathScrollView->setFixedHeight( 64 );
	connect( m_pAutomationPathView, SIGNAL( pointAdded(float, float) ), this, SLOT( automationPathPointAdded(float,float) ) );
	connect( m_pAutomationPathView, SIGNAL( pointRemoved(float, float) ), this, SLOT( automationPathPointRemoved(float,float) ) );
	connect( m_pAutomationPathView, SIGNAL( pointMoved(float, float, float, float) ), this, SLOT( automationPathPointMoved(float,float, float, float) ) );

	m_pAutomationCombo = new LCDCombo( nullptr, QSize( m_nPatternListWidth, 18 ) );
	m_pAutomationCombo->setToolTip( tr("Adjust parameter values in time") );
	m_pAutomationCombo->addItem( tr("Velocity") );
	m_pAutomationCombo->setCurrentIndex( 0 );

	m_pVScrollBar = new QScrollBar( Qt::Vertical, nullptr );
	m_pVScrollBar->setObjectName( "SongEditorPanel VScrollBar" );
	connect( m_pVScrollBar, SIGNAL(valueChanged(int)), this, SLOT( vScrollTo(int) ) );

	m_pWidgetStack->addWidget( m_pPositionRulerScrollView );
	m_pWidgetStack->addWidget( m_pPlaybackTrackScrollView );

	if( Preferences::get_instance()->getShowPlaybackTrack() ) {
		showPlaybackTrack();
	} else {
		showTimeline();
	}
	
	// ok...let's build the layout
	QGridLayout *pGridLayout = new QGridLayout();
	pGridLayout->setSpacing( 0 );
	pGridLayout->setMargin( 0 );

	pGridLayout->addWidget( pBackPanel, 0, 0 );
	pGridLayout->addWidget( m_pWidgetStack, 0, 1 );
	pGridLayout->addWidget( m_pPatternListScrollView, 1, 0 );
	pGridLayout->addWidget( m_pEditorScrollView, 1, 1 );
	pGridLayout->addWidget( m_pVScrollBar, 1, 2, 2, 1 );
	pGridLayout->addWidget( m_pAutomationPathScrollView, 2, 1);
	pGridLayout->addWidget( m_pAutomationCombo, 2, 0, Qt::AlignTop | Qt::AlignRight );
	pGridLayout->addWidget( pHScrollbarPanel, 3, 1 );
	if( !pPref->getShowAutomationArea() ){
		m_pAutomationPathScrollView->hide();
		m_pAutomationCombo->hide();
	}
	
	this->setLayout( pGridLayout );
	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Window, QColor( 58, 62, 72 ) );
	this->setPalette( defaultPalette );

	show();

	updateAll();

	HydrogenApp::get_instance()->addEventListener( this );

	m_pTimer = new QTimer(this);
	
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT( updatePlayHeadPosition() ) );
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT( updatePlaybackFaderPeaks() ) );
	// connect(HydrogenApp::get_instance()->getPlayerControl(), SIGNAL(songModeChanged()),
	// 		this, SLOT(onSongModeChanged()));
	
	m_pTimer->start(100);
}



SongEditorPanel::~SongEditorPanel()
{
	m_pTimer->stop();
}



void SongEditorPanel::updatePlayHeadPosition()
{
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	if ( Preferences::get_instance()->m_bFollowPlayhead &&
		 pHydrogen->getMode() == Song::Mode::Song ) {
		if ( pAudioEngine->getState() != H2Core::AudioEngine::State::Playing ) {
			return;
		}

		int nWidth;
		if ( m_pViewPlaybackBtn->isChecked() ) {
			nWidth = m_pPlaybackTrackScrollView->viewport()->width();
		} else {
			nWidth = m_pPositionRulerScrollView->viewport()->width();
		}

		QPoint pos = m_pPositionRuler->pos();
		int x = -pos.x();

		int nPlayHeadPosition = pAudioEngine->getColumn() *
			m_pSongEditor->getGridWidth();

		int value = m_pEditorScrollView->horizontalScrollBar()->value();

		// Distance the grid will be moved left or right.
		int nIncrement = 100;
		if ( nIncrement > std::round( static_cast<float>(nWidth) / 3 ) ) {
			nIncrement = std::round( static_cast<float>(nWidth) / 3 );
		} else if ( nIncrement < 2 * m_pPositionRuler->getPlayheadWidth() ) {
			nIncrement = 2 * m_pPositionRuler->getPlayheadWidth();
		}
		
		if ( nPlayHeadPosition > ( x + nWidth - std::floor( static_cast<float>( nIncrement ) / 2 ) ) ) {
			hScrollTo( value + nIncrement );
		}
		else if ( nPlayHeadPosition < x ) {
			hScrollTo( value - nIncrement );
		}
	}
}

void SongEditorPanel::updatePlaybackFaderPeaks()
{
	Sampler*		pSampler = Hydrogen::get_instance()->getAudioEngine()->getSampler();
	Preferences *	pPref = Preferences::get_instance();
	auto		pInstrument = pSampler->getPlaybackTrackInstrument();

	
	bool bShowPeaks = pPref->showInstrumentPeaks();
	float fallOff = pPref->getMixerFalloffSpeed();
	
	// fader
	float fOldPeak_L = m_pPlaybackTrackFader->getPeak_L();
	float fOldPeak_R = m_pPlaybackTrackFader->getPeak_R();
	
	float fNewPeak_L = pInstrument->get_peak_l();
	pInstrument->set_peak_l( 0.0f );	// reset instrument peak

	float fNewPeak_R = pInstrument->get_peak_r();
	pInstrument->set_peak_r( 0.0f );	// reset instrument peak

	if (!bShowPeaks) {
		fNewPeak_L = 0.0f;
		fNewPeak_R = 0.0f;
	}

	if ( fNewPeak_L >= fOldPeak_L) {	// LEFT peak
		m_pPlaybackTrackFader->setPeak_L( fNewPeak_L );
	}
	else {
		m_pPlaybackTrackFader->setPeak_L( fOldPeak_L / fallOff );
	}
	if ( fNewPeak_R >= fOldPeak_R) {	// Right peak
		m_pPlaybackTrackFader->setPeak_R( fNewPeak_R );
	}
	else {
		m_pPlaybackTrackFader->setPeak_R( fOldPeak_R / fallOff );
	}
}

void SongEditorPanel::vScrollTo( int value )
{
	static bool inside = false;
	if ( !inside ) {
		inside = true;
		m_pVScrollBar->setValue( value );
		m_pPatternListScrollView->verticalScrollBar()->setValue( value );
		m_pEditorScrollView->verticalScrollBar()->setValue( value );
		inside = false;
	}
}

void SongEditorPanel::hScrollTo( int value )
{
	static bool inside = false;
	if ( !inside ) {
		inside = true;
		m_pHScrollBar->setValue( value );
		m_pEditorScrollView->horizontalScrollBar()->setValue( value );
		m_pPlaybackTrackScrollView->horizontalScrollBar()->setValue( value );
		m_pPositionRulerScrollView->horizontalScrollBar()->setValue( value );
		m_pAutomationPathScrollView->horizontalScrollBar()->setValue( value );
		inside = false;
	}
}

///
/// Update and redraw all...
///
void SongEditorPanel::updateAll()
{
	Hydrogen *	pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> 		pSong = pHydrogen->getSong();
	
	updatePlaybackTrackIfNecessary();

	m_pPatternList->createBackground();
	m_pPatternList->update();

	m_pSongEditor->cleanUp();

	m_pSongEditor->createBackground();
	m_pSongEditor->update();

	updatePositionRuler();

 	m_pAutomationPathView->setAutomationPath( pSong->getVelocityAutomationPath() );

	resyncExternalScrollBar();

	m_pPlaybackTrackFader->setIsActive( ! H2Core::Hydrogen::get_instance()->getSong()->getPlaybackTrackFilename().isEmpty() );
}

void SongEditorPanel::updatePlaybackTrackIfNecessary()
{
	if( Preferences::get_instance()->getShowPlaybackTrack() ) {
		auto pCompo = Hydrogen::get_instance()->getAudioEngine()->getSampler()->getPlaybackTrackInstrument()->get_components()->front();
		m_pPlaybackTrackWaveDisplay->updateDisplay( pCompo->get_layer(0) );
	}
}


void SongEditorPanel::updatePositionRuler()
{
	m_pPositionRuler->createBackground();
}

///
/// Create a new pattern
///
void SongEditorPanel::newPatBtnClicked()
{
	Hydrogen	*pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	Pattern		*pNewPattern = new Pattern( tr("Pattern %1").arg(pPatternList->size()+1));
	PatternPropertiesDialog *pDialog = new PatternPropertiesDialog( this, pNewPattern, 0, true );

	if ( pDialog->exec() == QDialog::Accepted ) {
		SE_insertPatternAction* pAction =
				new SE_insertPatternAction( pHydrogen->getSelectedPatternNumber() + 1, new Pattern( pNewPattern->get_name() , pNewPattern->get_info(), pNewPattern->get_category() ) );
		HydrogenApp::get_instance()->m_pUndoStack->push(  pAction );
	}

	delete pNewPattern;
	delete pDialog;
}

///
/// Move up a pattern in the patternList
///
void SongEditorPanel::upBtnClicked()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	if( pHydrogen->getSelectedPatternNumber() < 0 || !pHydrogen->getSelectedPatternNumber() ) {
		return;
	}
	int nSelectedPatternPos = pHydrogen->getSelectedPatternNumber();

	SE_movePatternListItemAction *pAction = new SE_movePatternListItemAction( nSelectedPatternPos, nSelectedPatternPos -1 ) ;
	HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
}



///
/// Move down a pattern in the patternList
///
void SongEditorPanel::downBtnClicked()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	if( pHydrogen->getSelectedPatternNumber() +1 >=  pPatternList->size() ) { 
		return;
	}
	
	int nSelectedPatternPos = pHydrogen->getSelectedPatternNumber();

	SE_movePatternListItemAction *pAction = new SE_movePatternListItemAction( nSelectedPatternPos, nSelectedPatternPos +1 ) ;
	HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
}




void SongEditorPanel::clearSequence()
{
	int res = QMessageBox::information( this, "Hydrogen", tr( "Warning, this will erase your pattern sequence.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 );
	if ( res == 1 ) {
		return;
	}
	
	QString filename = Filesystem::tmp_file_path( "SEQ.xml" );
	SE_deletePatternSequenceAction *pAction = new SE_deletePatternSequenceAction( filename );
	HydrogenApp *pH2App = HydrogenApp::get_instance();

	pH2App->m_pUndoStack->push( pAction );
}


void SongEditorPanel::restoreGroupVector( QString filename )
{
	//clear the old sequese
	std::vector<PatternList*> *pPatternGroupsVect = Hydrogen::get_instance()->getSong()->getPatternGroupVector();
	for (uint i = 0; i < pPatternGroupsVect->size(); i++) {
		PatternList *pPatternList = (*pPatternGroupsVect)[i];
		pPatternList->clear();
		delete pPatternList;
	}
	pPatternGroupsVect->clear();

	Hydrogen::get_instance()->getSong()->readTempPatternList( filename );
	m_pSongEditor->updateEditorandSetTrue();
	updateAll();
}


void SongEditorPanel::resyncExternalScrollBar()
{
	m_pHScrollBar->setMinimum( m_pEditorScrollView->horizontalScrollBar()->minimum() );
	m_pHScrollBar->setMaximum( m_pEditorScrollView->horizontalScrollBar()->maximum() );
	m_pHScrollBar->setSingleStep( m_pEditorScrollView->horizontalScrollBar()->singleStep() );
	m_pHScrollBar->setPageStep( m_pEditorScrollView->horizontalScrollBar()->pageStep() );
	m_pHScrollBar->setValue( m_pEditorScrollView->horizontalScrollBar()->value() );

	m_pVScrollBar->setMinimum( m_pEditorScrollView->verticalScrollBar()->minimum() );
	m_pVScrollBar->setMaximum( m_pEditorScrollView->verticalScrollBar()->maximum() );
	m_pVScrollBar->setSingleStep( m_pEditorScrollView->verticalScrollBar()->singleStep() );
	m_pVScrollBar->setPageStep( m_pEditorScrollView->verticalScrollBar()->pageStep() );
	m_pVScrollBar->setValue( m_pEditorScrollView->verticalScrollBar()->value() );
}


void SongEditorPanel::resizeEvent( QResizeEvent *ev )
{
	UNUSED( ev );
	resyncExternalScrollBar();
}

void SongEditorPanel::actionModeChangeEvent( int nValue ) {

	if ( nValue == 0 ) {
		if ( ! m_pSelectionModeBtn->isDown() ) {
			m_pSelectionModeBtn->setChecked( true );
		}
		m_pDrawModeBtn->setChecked( false );
	} else if ( nValue == 1 ) {
		m_pSelectionModeBtn->setChecked( false );
		if ( ! m_pDrawModeBtn->isDown() ) {
			m_pDrawModeBtn->setChecked( true );
		}
	} else {
		ERRORLOG( QString( "Unknown EVENT_ACTION_MODE_CHANGE value" ) );
	}
}

void SongEditorPanel::selectionModeBtnPressed()
{
	if ( Hydrogen::get_instance()->getSong()->getActionMode() != H2Core::Song::ActionMode::selectMode ) {
		Hydrogen::get_instance()->getSong()->setActionMode( H2Core::Song::ActionMode::selectMode );
	} else {
		m_pSelectionModeBtn->setChecked( false );
	}
}

void SongEditorPanel::drawModeBtnPressed()
{
	if ( Hydrogen::get_instance()->getSong()->getActionMode() != H2Core::Song::ActionMode::drawMode ) {
		Hydrogen::get_instance()->getSong()->setActionMode( H2Core::Song::ActionMode::drawMode );
	} else {
		m_pDrawModeBtn->setChecked( false );
	}
}


void SongEditorPanel::timelineBtnPressed() {
	DEBUGLOG(! m_pTimelineBtn->isChecked());
	setTimelineActive( ! m_pTimelineBtn->isChecked() );
}

void SongEditorPanel::showTimeline()
{
	m_pWidgetStack->setCurrentWidget( m_pPositionRulerScrollView );
	m_pTimelineBtn->show();
	m_pMutePlaybackBtn->hide();
	m_pEditPlaybackBtn->hide();
	m_pPlaybackTrackFader->hide();
	if ( ! m_pViewPlaybackBtn->isDown() ) {
		m_pViewPlaybackBtn->setChecked( false );
	}
	if ( ! m_pViewTimelineBtn->isDown() ) {
		m_pViewTimelineBtn->setChecked( true );
	}
	Preferences::get_instance()->setShowPlaybackTrack( false );
}


void SongEditorPanel::showPlaybackTrack()
{
	m_pWidgetStack->setCurrentWidget( m_pPlaybackTrackScrollView );
	m_pTimelineBtn->hide();
	m_pMutePlaybackBtn->show();
	m_pEditPlaybackBtn->show();
	m_pPlaybackTrackFader->show();
	if ( ! m_pViewTimelineBtn->isDown() ) {
		m_pViewTimelineBtn->setChecked( false );
	}
	if ( ! m_pViewPlaybackBtn->isDown() ) {
		m_pViewPlaybackBtn->setChecked( true );
	}
	Preferences::get_instance()->setShowPlaybackTrack( true );
}

void SongEditorPanel::viewTimelineBtnPressed()
{
	if ( ! m_pViewTimelineBtn->isChecked() ){
		showTimeline();
	} else {
		showPlaybackTrack();
	}
}

void SongEditorPanel::viewPlaybackTrackBtnPressed()
{
	if ( ! m_pViewPlaybackBtn->isChecked() ){
		showPlaybackTrack();
	} else {
		showTimeline();
	}
}


void SongEditorPanel::mutePlaybackTrackBtnPressed()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();

	bool bState = ! m_pMutePlaybackBtn->isChecked();

	bState = pHydrogen->setPlaybackTrackState( bState );
	m_pMutePlaybackBtn->setChecked( !bState );
}

void SongEditorPanel::editPlaybackTrackBtnPressed()
{
	if ( Hydrogen::get_instance()->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	QString sPath = Preferences::get_instance()->getLastOpenPlaybackTrackDirectory();
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = QDir::homePath();
	}
	
	//use AudioFileBrowser, but don't allow multi-select. Also, hide all no necessary controls.
	AudioFileBrowser *pFileBrowser = new AudioFileBrowser( nullptr, false, false, sPath );
	
	QStringList filenameList;
	
	if ( pFileBrowser->exec() == QDialog::Accepted ) {
		Preferences::get_instance()->setLastOpenPlaybackTrackDirectory( pFileBrowser->getSelectedDirectory() );
		filenameList = pFileBrowser->getSelectedFiles();
	}

	delete pFileBrowser;

	if( filenameList.size() != 3 ) {
		return;
	}
	
	if ( filenameList[2].isEmpty() ) {
		return;
	}

	Hydrogen::get_instance()->loadPlaybackTrack( filenameList[2] );
	
	updateAll();
}

void SongEditorPanel::modeActionBtnPressed( )
{
	bool bWasStacked = m_pModeActionSingleBtn->isVisible();
	if( bWasStacked ){
		m_pModeActionSingleBtn->hide();
		m_pModeActionMultipleBtn->show();
	} else {
		m_pModeActionSingleBtn->show();
		m_pModeActionMultipleBtn->hide();
	}
	Hydrogen::get_instance()->setPlaysSelected( bWasStacked );
	EventQueue::get_instance()->push_event( EVENT_STACKED_MODE_ACTIVATION, bWasStacked ? 0 : 1 );
	updateAll();
}

void SongEditorPanel::setModeActionBtn( bool mode )
{
	if( mode ){
		m_pModeActionSingleBtn->hide();
		m_pModeActionMultipleBtn->show();
	} else {
		m_pModeActionSingleBtn->show();
		m_pModeActionMultipleBtn->hide();
	}
	// Set disabled or enabled
	if ( Hydrogen::get_instance()->getMode() == Song::Mode::Song ) {
		m_pModeActionMultipleBtn->setDisabled( true );
		m_pModeActionSingleBtn->setDisabled( true );
	} else {
		m_pModeActionMultipleBtn->setDisabled( false );
		m_pModeActionSingleBtn->setDisabled( false );
	}
}

void SongEditorPanel::zoomInBtnPressed()
{
	unsigned width = m_pSongEditor->getGridWidth();
	--width;
	m_pSongEditor->setGridWidth( width );
	m_pPositionRuler->setGridWidth( width );
	m_pAutomationPathView->setGridWidth( width );

	Preferences::get_instance()->setSongEditorGridWidth( width );
	Preferences::get_instance()->setSongEditorGridHeight( m_pSongEditor->getGridHeight() );
	
	updateAll();
}


void SongEditorPanel::zoomOutBtnPressed()
{
	unsigned width = m_pSongEditor->getGridWidth();
	++width;
	m_pSongEditor->setGridWidth( width );
	m_pPositionRuler->setGridWidth( width );
	m_pAutomationPathView->setGridWidth( width );

	Preferences::get_instance()->setSongEditorGridWidth( width );
	Preferences::get_instance()->setSongEditorGridHeight( m_pSongEditor->getGridHeight() );
	
	updateAll();
}

void SongEditorPanel::faderChanged( WidgetWithInput *pRef )
{
	Hydrogen *	pHydrogen = Hydrogen::get_instance();
	Fader* pFader = dynamic_cast<Fader*>( pRef );
	std::shared_ptr<Song> 		pSong = pHydrogen->getSong();
	
	if( pSong ){
		pSong->setPlaybackTrackVolume( pFader->getValue() );
	}
}


void SongEditorPanel::selectedPatternChangedEvent()
{
	setModeActionBtn( Preferences::get_instance()->patternModePlaysSelected() );
	updateAll();

	// Make sure currently selected pattern is visible.
	int nGridHeight = m_pPatternList->getGridHeight();
	m_pPatternListScrollView->ensureVisible( 0, (Hydrogen::get_instance()->getSelectedPatternNumber()
												 * nGridHeight + nGridHeight/2 ),
											 0, nGridHeight );
}

void SongEditorPanel::automationPathPointAdded(float x, float y)
{
	H2Core::AutomationPath *pPath = m_pAutomationPathView->getAutomationPath();
	SE_automationPathAddPointAction *pUndoAction = new SE_automationPathAddPointAction(pPath, x, y);
	HydrogenApp::get_instance()->m_pUndoStack->push( pUndoAction );
}


void SongEditorPanel::automationPathPointRemoved(float x, float y)
{
	H2Core::AutomationPath *pPath = m_pAutomationPathView->getAutomationPath();
	SE_automationPathRemovePointAction *pUndoAction = new SE_automationPathRemovePointAction(pPath, x, y);
	HydrogenApp::get_instance()->m_pUndoStack->push( pUndoAction );
}


void SongEditorPanel::automationPathPointMoved(float ox, float oy, float tx, float ty)
{
	H2Core::AutomationPath *pPath = m_pAutomationPathView->getAutomationPath();
	SE_automationPathMovePointAction *pUndoAction = new SE_automationPathMovePointAction(pPath, ox, oy, tx, ty);
	HydrogenApp::get_instance()->m_pUndoStack->push( pUndoAction );
}

void SongEditorPanel::toggleAutomationAreaVisibility()
{
	Preferences *pPref = Preferences::get_instance();
	
	if(!pPref->getShowAutomationArea())
	{
		m_pAutomationPathScrollView->show();
		m_pAutomationCombo->show();
		pPref->setShowAutomationArea( true );
	} else {
		m_pAutomationPathScrollView->hide();
		m_pAutomationCombo->hide();
		pPref->setShowAutomationArea( false );
	}
}


void SongEditorPanel::jackTimebaseStateChangedEvent( int ) {
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getJackTimebaseState() == JackAudioDriver::Timebase::Slave ) {
		setTimelineEnabled( false );
		m_pTimelineBtn->setToolTip( pCommonStrings->getTimelineDisabledTimebaseSlave() );
	} else if ( pHydrogen->getMode() != Song::Mode::Pattern ) {
		setTimelineEnabled( true );
		m_pTimelineBtn->setToolTip( pCommonStrings->getTimelineEnabled() );
	}
}

void SongEditorPanel::songModeActivationEvent( int ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
		setTimelineEnabled( false );
		m_pTimelineBtn->setToolTip( pCommonStrings->getTimelineDisabledPatternMode() );
	} else if ( pHydrogen->getJackTimebaseState() != JackAudioDriver::Timebase::Slave ) {
		setTimelineEnabled( true );
		m_pTimelineBtn->setToolTip( pCommonStrings->getTimelineEnabled() );
	}
	setModeActionBtn( Preferences::get_instance()->patternModePlaysSelected() );
}

void SongEditorPanel::timelineActivationEvent( int ){
	auto pHydrogen = Hydrogen::get_instance();
	if ( ! pHydrogen->isTimelineEnabled() && m_pTimelineBtn->isChecked() ) {
		setTimelineActive( false );
	} else if ( pHydrogen->isTimelineEnabled() && ! m_pTimelineBtn->isChecked() ) {
		setTimelineActive( true );
	}
}

bool SongEditorPanel::getTimelineActive() const {
	return m_pTimelineBtn->isChecked();
}

void SongEditorPanel::setTimelineActive( bool bActive ){
	DEBUGLOG( bActive );
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	if ( ! m_pTimelineBtn->isDown() ) {
		m_pTimelineBtn->setChecked( bActive );
	}
	
	Hydrogen::get_instance()->setIsTimelineActivated( bActive );

	QString sMessage = QString( "%1 = %2" )
		.arg( pCommonStrings->getTimelineBigButton() )
		.arg( bActive ? pCommonStrings->getStatusOn() : pCommonStrings->getStatusOff() );
	HydrogenApp::get_instance()->setStatusBarMessage( sMessage, 5000);
}

bool SongEditorPanel::getTimelineEnabled() const {
	return m_pTimelineBtn->getIsActive();
}

void SongEditorPanel::setTimelineEnabled( bool bEnabled ) {
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	if ( bEnabled ) {
		m_pTimelineBtn->setIsActive( true );
		// setTimelineActive( m_bLastIsTimelineActivated );
	} else {
		m_bLastIsTimelineActivated = Hydrogen::get_instance()->getSong()->getIsTimelineActivated();
		if ( m_pTimelineBtn->isChecked() ) {
			// setTimelineActive( false );
		}
		m_pTimelineBtn->setIsActive( false );
	}

	QString sMessage = QString( "%1 = %2" )
		.arg( pCommonStrings->getTimelineBigButton() )
		.arg( bEnabled ? pCommonStrings->getStatusEnabled() :
			  pCommonStrings->getStatusDisabled() );
	HydrogenApp::get_instance()->setStatusBarMessage( sMessage, 5000);
}

void SongEditorPanel::updateSongEditorEvent( int ) {
	updateAll();
}

void SongEditorPanel::columnChangedEvent( int ) {
	// In Song mode, we may scroll to change position in the Song Editor.
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();
	if ( pHydrogen->getMode() == Song::Mode::Song ) {

		// Scroll vertically to keep currently playing patterns in view
		int nPatternInView = -1;
		int scroll = m_pSongEditor->yScrollTarget( m_pEditorScrollView, &nPatternInView );
		vScrollTo( scroll );

		if ( pPref->patternFollowsSong() ) {
			// Selected pattern follows song.
			//
			// If the currently selected pattern is no longer one of those currently playing in the song, then
			// we select the suggested pattern from yScrollTarget.

			AudioEngine *pAudioEngine = pHydrogen->getAudioEngine();
			PatternList *pSongPatterns = pHydrogen->getSong()->getPatternList();
			int nSelectedPattern = pHydrogen->getSelectedPatternNumber();

			bool bFound = false;
			std::vector< Pattern* >patternList;
			pAudioEngine->lock( RIGHT_HERE );
			for ( auto pPattern : *pAudioEngine->getPlayingPatterns() ) {
				patternList.push_back( pPattern );
			}
			pAudioEngine->unlock();

			for ( auto pPattern : patternList ) {
				int nPattern = pSongPatterns->index( pPattern );
				if ( nPattern == nSelectedPattern ) {
					bFound = true;
					break;
				}
			}
			if ( !bFound && patternList.size() != 0 ) {
				assert( nPatternInView != -1 );
				pHydrogen->setSelectedPatternNumber( nPatternInView );
			}
		}
	}
}
