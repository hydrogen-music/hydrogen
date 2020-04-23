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
#include "../Skin.h"
#include "../WidgetScrollArea.h"

#include "SongEditor.h"
#include "UndoActions.h"

#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/pattern_list.h>
#ifdef WIN32
#include <time.h>
#endif
using namespace H2Core;
using namespace std;

const char* SongEditorPanel::__class_name = "SongEditorPanel";


SongEditorPanel::SongEditorPanel(QWidget *pParent)
 : QWidget( pParent )
 , Object( __class_name )
 , m_actionMode( DRAW_ACTION )
{
	m_nInitialWidth = 600;
	m_nInitialHeight = 250;
	
	Preferences *pPref = Preferences::get_instance();

	Hydrogen*	pEngine = Hydrogen::get_instance();
	Song*		pSong = pEngine->getSong();

	setWindowTitle( tr( "Song Editor" ) );

	// background
	PixmapWidget *pBackPanel = new PixmapWidget( nullptr );
	pBackPanel->setFixedSize( 196, 49 );
	pBackPanel->setPixmap( "/songEditor/bg_topPanel.png" );

	// time line toggle button
	m_pTimeLineToggleBtn = new ToggleButton(
			pBackPanel,
			"/songEditor/btn_bpm_on.png",
			"/songEditor/btn_bpm_off.png",
			"/songEditor/btn_bpm_over.png",
			QSize( 54, 13 )
	);
	m_pTimeLineToggleBtn->move( 133, 6 );
	m_pTimeLineToggleBtn->setToolTip( tr( "Enable time line edit") );
	connect( m_pTimeLineToggleBtn, SIGNAL( clicked( Button* ) ), this, SLOT( timeLineBtnPressed(Button* ) ) );
	m_pTimeLineToggleBtn->setPressed( pPref->getUseTimelineBpm() );


	// clear sequence button
	m_pClearPatternSeqBtn = new Button(
			pBackPanel,
			"/songEditor/btn_clear_on.png",
			"/songEditor/btn_clear_off.png",
			"/songEditor/btn_clear_over.png",
			QSize(53,13)
	);
	m_pClearPatternSeqBtn->move( 6, 5 + 25 );
	m_pClearPatternSeqBtn->setToolTip( tr("Clear pattern sequence") );
	connect( m_pClearPatternSeqBtn, SIGNAL( clicked( Button* ) ), this, SLOT( clearSequence(Button*) ) );

	// new pattern button
	Button *newPatBtn = new Button(
			pBackPanel,
			"/songEditor/btn_new_on.png",
			"/songEditor/btn_new_off.png",
			"/songEditor/btn_new_over.png",
			QSize(19, 13)
	);
	newPatBtn->move( 64, 5 + 25);
	newPatBtn->setToolTip( tr("Create new pattern") );
	connect( newPatBtn, SIGNAL( clicked( Button* ) ), this, SLOT( newPatBtnClicked( Button* ) ) );

	// down button
	m_pDownBtn = new Button(
			pBackPanel,
			"/songEditor/btn_down_on.png",
			"/songEditor/btn_down_off.png",
			"/songEditor/btn_down_over.png",
			QSize(18,13)
	);
	m_pDownBtn->move( 89, 5 + 25);
	m_pDownBtn->setToolTip( tr("Move the selected pattern down") );
	connect( m_pDownBtn, SIGNAL( clicked( Button* ) ), this, SLOT( downBtnClicked( Button* ) ) );

	// up button
	m_pUpBtn = new Button(
			pBackPanel,
			"/songEditor/btn_up_on.png",
			"/songEditor/btn_up_off.png",
			"/songEditor/btn_up_over.png",
			QSize(18,13)
	);
	m_pUpBtn->move( 106, 5 + 25 );
	m_pUpBtn->setToolTip( tr("Move the selected pattern up") );
	connect( m_pUpBtn, SIGNAL( clicked( Button* ) ), this, SLOT( upBtnClicked( Button* ) ) );

	// select toggle button
	m_pPointerActionBtn = new ToggleButton(
			pBackPanel,
			"/songEditor/btn_select_on.png",
			"/songEditor/btn_select_off.png",
			"/songEditor/btn_select_over.png",
			QSize( 18, 13 )
	);
	m_pPointerActionBtn->move( 128, 5 + 25 );
	m_pPointerActionBtn->setToolTip( tr( "Select mode" ) );
	connect( m_pPointerActionBtn, SIGNAL( clicked( Button* ) ), this, SLOT( pointerActionBtnPressed(Button*) ) );

	// draw toggle button
	m_pDrawActionBtn = new ToggleButton(
			pBackPanel,
			"/songEditor/btn_draw_on.png",
			"/songEditor/btn_draw_off.png",
			"/songEditor/btn_draw_over.png",
			QSize( 18, 13 )
	);
	m_pDrawActionBtn->move( 147, 5 + 25 );
	m_pDrawActionBtn->setToolTip( tr( "Draw mode") );
	connect( m_pDrawActionBtn, SIGNAL( clicked( Button* ) ), this, SLOT( drawActionBtnPressed(Button* ) ) );
	m_pDrawActionBtn->setPressed( true );

	m_pModeActionBtn = new ToggleButton(
			pBackPanel,
			"/songEditor/btn_mode_on.png",
			"/songEditor/btn_mode_off.png",
			"/songEditor/btn_mode_over.png",
			QSize( 18, 13 )
	);
	m_pModeActionBtn->move( 169, 5 + 25 );
	m_pModeActionBtn->setToolTip( tr( "stacked mode") );
	m_pModeActionBtn->setPressed(  pPref->patternModePlaysSelected() );
	connect( m_pModeActionBtn, SIGNAL( clicked( Button* ) ), this, SLOT( modeActionBtnPressed() ) );

// ZOOM
	m_pHScrollBar = new QScrollBar( Qt::Horizontal, nullptr );
	m_pHScrollBar->setObjectName( "SongEditorPanel HScrollBar" );
	connect( m_pHScrollBar, SIGNAL(valueChanged(int)), this, SLOT( hScrollTo(int) ) );

	// zoom-in btn
	Button* pZoomInBtn = new Button(
			nullptr,
			"/songEditor/btn_new_on.png",
			"/songEditor/btn_new_off.png",
			"/songEditor/btn_new_over.png",
			QSize( 19, 13 )
	);
	connect( pZoomInBtn, SIGNAL( clicked( Button* ) ), this, SLOT( zoomOutBtnPressed(Button* ) ) );



	// zoom-out btn
	Button* pZoomOutBtn = new Button(
			nullptr,
			"/songEditor/btn_minus_on.png",
			"/songEditor/btn_minus_off.png",
			"/songEditor/btn_minus_over.png",
			QSize( 19, 13 )
	);
	connect( pZoomOutBtn, SIGNAL( clicked( Button* ) ), this, SLOT( zoomInBtnPressed(Button* ) ) );

	// view playback track toggle button
	m_pViewPlaybackToggleBtn = new ToggleButton(
			nullptr,
			"/songEditor/btn_viewPL_on.png",
			"/songEditor/btn_viewPL_off.png",
			"/songEditor/btn_viewPL_over.png",
			QSize( 19, 13 )
	);
	m_pViewPlaybackToggleBtn->setToolTip( tr( "View playback track") );
	connect( m_pViewPlaybackToggleBtn, SIGNAL( clicked( Button* ) ), this, SLOT( viewPlaybackTrackBtnPressed(Button* ) ) );
	m_pViewPlaybackToggleBtn->setPressed( false );
	
	// Playback Fader
	m_pPlaybackTrackFader = new VerticalFader( pBackPanel, false, false );
	m_pPlaybackTrackFader->move( 6, 2 );
	m_pPlaybackTrackFader->setMinValue( 0.0 );
	m_pPlaybackTrackFader->setMaxValue( 1.5 );
	m_pPlaybackTrackFader->setValue( pSong->get_playback_track_volume() );
	m_pPlaybackTrackFader->hide();
	connect( m_pPlaybackTrackFader, SIGNAL( valueChanged(Fader*) ), this, SLOT( faderChanged(Fader*) ) );

	// mute playback track toggle button
	m_pMutePlaybackToggleBtn = new ToggleButton(
			pBackPanel,
			"/mixerPanel/master_mute_on.png",
			"/mixerPanel/master_mute_off.png",
			"/mixerPanel/master_mute_over.png",
			QSize( 42, 13 )
	);
	m_pMutePlaybackToggleBtn->setToolTip( tr( "Mute playback track") );
	m_pMutePlaybackToggleBtn->move( 151, 6 );
	m_pMutePlaybackToggleBtn->hide();
	connect( m_pMutePlaybackToggleBtn, SIGNAL( clicked( Button* ) ), this, SLOT( mutePlaybackTrackBtnPressed(Button* ) ) );
	m_pMutePlaybackToggleBtn->setPressed( !pSong->get_playback_track_enabled() );
	
	// edit playback track toggle button
	m_pEditPlaybackBtn = new Button(
			pBackPanel,
			"/mixerPanel/edit_on.png",
			"/mixerPanel/edit_off.png",
			"/mixerPanel/edit_over.png",
			QSize( 42, 13 )
	);
	m_pEditPlaybackBtn->setToolTip( tr( "Choose playback track") );
	m_pEditPlaybackBtn->move( 124, 6 );
	m_pEditPlaybackBtn->hide();
	connect( m_pEditPlaybackBtn, SIGNAL( clicked( Button* ) ), this, SLOT( editPlaybackTrackBtnPressed(Button* ) ) );
	m_pEditPlaybackBtn->setPressed( false );

	// timeline view toggle button
	m_pViewTimeLineToggleBtn = new ToggleButton(
			nullptr,
			"/songEditor/btn_viewTL_on.png",
			"/songEditor/btn_viewTL_off.png",
			"/songEditor/btn_viewTL_over.png",
			QSize( 19, 13 )
	);
	m_pViewTimeLineToggleBtn->setToolTip( tr( "View timeline") );
	connect( m_pViewTimeLineToggleBtn, SIGNAL( clicked( Button* ) ), this, SLOT( viewTimeLineBtnPressed(Button* ) ) );
	m_pViewTimeLineToggleBtn->setPressed( true );
	
	
	QHBoxLayout *pHZoomLayout = new QHBoxLayout();
	pHZoomLayout->setSpacing( 0 );
	pHZoomLayout->setMargin( 0 );
	pHZoomLayout->addWidget( m_pViewPlaybackToggleBtn );
	pHZoomLayout->addWidget( m_pViewTimeLineToggleBtn );
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
	m_pPatternListScrollView->setWidget( m_pPatternList );
	m_pPatternListScrollView->setFocusProxy( m_pPatternList );


	// EDITOR
	m_pEditorScrollView = new WidgetScrollArea( nullptr );
	m_pEditorScrollView->setFrameShape( QFrame::NoFrame );
	m_pEditorScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pEditorScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pSongEditor = new SongEditor( m_pEditorScrollView->viewport(), m_pEditorScrollView );
	m_pEditorScrollView->setWidget( m_pSongEditor );
	m_pEditorScrollView->setFocusProxy( m_pSongEditor );

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
	m_pPositionRuler = new SongEditorPositionRuler( m_pPositionRulerScrollView->viewport() );
	m_pPositionRulerScrollView->setWidget( m_pPositionRuler );
	m_pPositionRulerScrollView->setFixedHeight( 50 );
	connect( m_pPositionRulerScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( hScrollTo(int) ) );
	m_pPositionRuler->setFocusProxy( m_pSongEditor );
	m_pPositionRuler->setFocusPolicy( Qt::ClickFocus );
	
	m_pPlaybackTrackScrollView = new WidgetScrollArea( m_pWidgetStack );
	m_pPlaybackTrackScrollView->setFrameShape( QFrame::NoFrame );
	m_pPlaybackTrackScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPlaybackTrackScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	
	InstrumentComponent* pCompo = AudioEngine::get_instance()->get_sampler()->__playback_instrument->get_components()->front();
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
	connect( m_pAutomationPathView, SIGNAL( valueChanged() ), this, SLOT( automationPathChanged() ) );
	connect( m_pAutomationPathView, SIGNAL( pointAdded(float, float) ), this, SLOT( automationPathPointAdded(float,float) ) );
	connect( m_pAutomationPathView, SIGNAL( pointRemoved(float, float) ), this, SLOT( automationPathPointRemoved(float,float) ) );
	connect( m_pAutomationPathView, SIGNAL( pointMoved(float, float, float, float) ), this, SLOT( automationPathPointMoved(float,float, float, float) ) );

	m_pAutomationCombo = new LCDCombo( nullptr, 22 );
	m_pAutomationCombo->setToolTip( tr("Adjust parameter values in time") );
	m_pAutomationCombo->addItem( tr("Velocity") );
	m_pAutomationCombo->select( 0 );

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
	defaultPalette.setColor( QPalette::Background, QColor( 58, 62, 72 ) );
	this->setPalette( defaultPalette );

	show();

	updateAll();

	HydrogenApp::get_instance()->addEventListener( this );

	m_pTimer = new QTimer(this);
	
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT( updatePlayHeadPosition() ) );
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT( updatePlaybackFaderPeaks() ) );
	
	m_pTimer->start(100);
}



SongEditorPanel::~SongEditorPanel()
{
	m_pTimer->stop();
}



void SongEditorPanel::updatePlayHeadPosition()
{
	Song *pSong = Hydrogen::get_instance()->getSong();

	if ( Preferences::get_instance()->m_bFollowPlayhead && pSong->get_mode() == Song::SONG_MODE) {
		if ( Hydrogen::get_instance()->getState() != STATE_PLAYING ) {
			return;
		}

		QPoint pos = m_pPositionRuler->pos();
		int x = -pos.x();
		int w = m_pPositionRulerScrollView->viewport()->width();

		int nPlayHeadPosition = Hydrogen::get_instance()->getPatternPos() * m_pSongEditor->getGridWidth();

		int value = m_pEditorScrollView->horizontalScrollBar()->value();
		if ( nPlayHeadPosition > ( x + w - 50 ) ) {
			hScrollTo( value + 100 );
		}
		else if ( nPlayHeadPosition < x ) {
			hScrollTo( value - 100 );
		}
	}
}

void SongEditorPanel::updatePlaybackFaderPeaks()
{
	Sampler*		pSampler = AudioEngine::get_instance()->get_sampler();
	Preferences *	pPref = Preferences::get_instance();
	Instrument*		pInstrument = pSampler->__playback_instrument;

	
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
	Hydrogen *	pEngine = Hydrogen::get_instance();
	Song *		pSong = pEngine->getSong();
	
	updatePlaybackTrackIfNecessary();

	m_pPatternList->createBackground();
	m_pPatternList->update();

	m_pSongEditor->cleanUp();

	m_pSongEditor->createBackground();
	m_pSongEditor->update();

 	m_pAutomationPathView->setAutomationPath( pSong->get_velocity_automation_path() );

	resyncExternalScrollBar();
}

void SongEditorPanel::updatePlaybackTrackIfNecessary()
{
	if( Preferences::get_instance()->getShowPlaybackTrack() ) {
		InstrumentComponent *pCompo = AudioEngine::get_instance()->get_sampler()->__playback_instrument->get_components()->front();
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
void SongEditorPanel::newPatBtnClicked( Button* btn )
{
	UNUSED( btn );
	Hydrogen	*pEngine = Hydrogen::get_instance();
	Song		*pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	Pattern		*pNewPattern = new Pattern( tr("Pattern %1").arg(pPatternList->size()+1));
	PatternPropertiesDialog *pDialog = new PatternPropertiesDialog( this, pNewPattern, 0, true );

	if ( pDialog->exec() == QDialog::Accepted ) {
		SE_insertPatternAction* pAction =
				new SE_insertPatternAction( pEngine->getSelectedPatternNumber() + 1, new Pattern( pNewPattern->get_name() , pNewPattern->get_info(), pNewPattern->get_category() ) );
		HydrogenApp::get_instance()->m_pUndoStack->push(  pAction );
	}

	delete pNewPattern;
	delete pDialog;
}


void SongEditorPanel::insertPattern( int idx, Pattern* pPattern )
{
	Hydrogen	*pEngine = Hydrogen::get_instance();
	Song		*pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();

	pPatternList->insert( idx, pPattern );
	pEngine->setSelectedPatternNumber( idx );
	pSong->set_is_modified( true );
	updateAll();
}

void SongEditorPanel::deletePattern( int idx )
{
	Hydrogen	*pEngine = Hydrogen::get_instance();
	Song		*pSong = 	pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();
	H2Core::Pattern *pPattern = pPatternList->get( idx );
	
	if( idx == 	pEngine->getSelectedPatternNumber() ) {
		pEngine->setSelectedPatternNumber( idx -1 );
	}
	
	pPatternList->del( pPattern );
	delete pPattern;
	pSong->set_is_modified( true );
	updateAll();
}

///
/// Move up a pattern in the patternList
///
void SongEditorPanel::upBtnClicked( Button* btn )
{
	UNUSED( btn );
	Hydrogen *pEngine = Hydrogen::get_instance();

	if( pEngine->getSelectedPatternNumber() < 0 || !pEngine->getSelectedPatternNumber() ) return;
	int nSelectedPatternPos = pEngine->getSelectedPatternNumber();

	SE_movePatternListItemAction *pAction = new SE_movePatternListItemAction( nSelectedPatternPos, nSelectedPatternPos -1 ) ;
	HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
}



///
/// Move down a pattern in the patternList
///
void SongEditorPanel::downBtnClicked( Button* btn )
{
	UNUSED( btn );
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->get_pattern_list();

	if( pEngine->getSelectedPatternNumber() +1 >=  pSong->get_pattern_list()->size() ) return;
	int nSelectedPatternPos = pEngine->getSelectedPatternNumber();

	SE_movePatternListItemAction *pAction = new SE_movePatternListItemAction( nSelectedPatternPos, nSelectedPatternPos +1 ) ;
	HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
}




void SongEditorPanel::clearSequence( Button* btn)
{
	UNUSED( btn );

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
	vector<PatternList*> *pPatternGroupsVect = Hydrogen::get_instance()->getSong()->get_pattern_group_vector();
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

	vScrollTo( m_pPatternList->getGridHeight() * Hydrogen::get_instance()->getSelectedPatternNumber() );
}


void SongEditorPanel::resizeEvent( QResizeEvent *ev )
{
	UNUSED( ev );
	resyncExternalScrollBar();
}

void SongEditorPanel::pointerActionBtnPressed( Button* pBtn )
{
	pBtn->setPressed( true );
	m_pDrawActionBtn->setPressed( false );
	m_actionMode = SELECT_ACTION;
}



void SongEditorPanel::drawActionBtnPressed( Button* pBtn )
{
	pBtn->setPressed( true );
	m_pPointerActionBtn->setPressed( false );
	m_actionMode = DRAW_ACTION;
}



void SongEditorPanel::timeLineBtnPressed( Button* pBtn )
{
	if( m_pTimeLineToggleBtn->isPressed() ){
		Preferences::get_instance()->setUseTimelineBpm( false );
		Hydrogen::get_instance()->setTimelineBpm();
	}
	else
	{
		Preferences::get_instance()->setUseTimelineBpm( true );
	}
	
	m_pPositionRuler->createBackground();	
}

void SongEditorPanel::showTimeline()
{
	m_pWidgetStack->setCurrentWidget( m_pPositionRulerScrollView );
	m_pTimeLineToggleBtn->show();
	m_pMutePlaybackToggleBtn->hide();
	m_pEditPlaybackBtn->hide();
	m_pPlaybackTrackFader->hide();
	m_pViewPlaybackToggleBtn->setPressed( false );
	m_pViewTimeLineToggleBtn->setPressed( true );
	Preferences::get_instance()->setShowPlaybackTrack( false );
}


void SongEditorPanel::showPlaybackTrack()
{
	m_pWidgetStack->setCurrentWidget( m_pPlaybackTrackScrollView );
	m_pTimeLineToggleBtn->hide();
	m_pMutePlaybackToggleBtn->show();
	m_pEditPlaybackBtn->show();
	m_pPlaybackTrackFader->show();
	m_pViewTimeLineToggleBtn->setPressed( false );
	m_pViewPlaybackToggleBtn->setPressed( true );
	Preferences::get_instance()->setShowPlaybackTrack( true );
}

void SongEditorPanel::viewTimeLineBtnPressed( Button* pBtn )
{
	if( pBtn->isPressed() ){
		showTimeline();
	}
	else
	{
		showPlaybackTrack();
	}
}

void SongEditorPanel::viewPlaybackTrackBtnPressed( Button* pBtn )
{
	if( pBtn->isPressed() ){
		showPlaybackTrack();
	}
	else
	{
		showTimeline();
	}
}


void SongEditorPanel::mutePlaybackTrackBtnPressed( Button* pBtn )
{
	Hydrogen* pEngine = Hydrogen::get_instance();

	bool state = !pBtn->isPressed();
	state = pEngine->setPlaybackTrackState( state );
	m_pMutePlaybackToggleBtn->setPressed( !state );
}

void SongEditorPanel::editPlaybackTrackBtnPressed( Button* pBtn )
{
	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}
	
	//use AudioFileBrowser, but don't allow multi-select. Also, hide all no necessary controls.
	AudioFileBrowser *pFileBrowser = new AudioFileBrowser( nullptr, false, false);
	
	QStringList filenameList;
	
	if ( pFileBrowser->exec() == QDialog::Accepted ) {
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
	if( m_pModeActionBtn->isPressed() ){
		m_pModeActionBtn->setToolTip( tr( "stacked pattern mode") );
	} else {
		m_pModeActionBtn->setToolTip( tr( "single pattern mode") );
	}
	Hydrogen::get_instance()->togglePlaysSelected();
	updateAll();
}

void SongEditorPanel::setModeActionBtn( bool mode )
{
	if( mode ){
		m_pModeActionBtn->setPressed( true );
		m_pModeActionBtn->setToolTip( tr( "stacked pattern mode") );
	} else {
		m_pModeActionBtn->setPressed( false );
		m_pModeActionBtn->setToolTip( tr( "single pattern mode") );
	}
}

void SongEditorPanel::zoomInBtnPressed( Button* pBtn )
{
	UNUSED( pBtn );
	unsigned width = m_pSongEditor->getGridWidth ();
	--width;
	m_pSongEditor->setGridWidth (width);
	m_pPositionRuler->setGridWidth (width);
	m_pAutomationPathView->setGridWidth (width);

	updateAll();
}


void SongEditorPanel::zoomOutBtnPressed( Button* pBtn )
{
	UNUSED( pBtn );
	unsigned width = m_pSongEditor->getGridWidth ();
	++width;
	m_pSongEditor->setGridWidth (width);
	m_pPositionRuler->setGridWidth (width);
	m_pAutomationPathView->setGridWidth (width);
	updateAll();
}

void SongEditorPanel::faderChanged(Fader *pFader)
{
	UNUSED( pFader );
	
	Hydrogen *	pHydrogen = Hydrogen::get_instance();
	Song*		pSong = pHydrogen->getSong();
	
	if( pSong ){
		pSong->set_playback_track_volume( pFader->getValue() );
	}
}


void SongEditorPanel::selectedPatternChangedEvent()
{
	m_pModeActionBtn->setPressed( Preferences::get_instance()->patternModePlaysSelected() );
	updateAll();
}


void SongEditorPanel::automationPathChanged()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	pSong->set_is_modified(true);
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

