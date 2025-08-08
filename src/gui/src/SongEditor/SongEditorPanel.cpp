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
#include "SongEditorPanel.h"

#include "PlaybackTrackWaveDisplay.h"
#include "SongEditor.h"

#include "../AudioFileBrowser/AudioFileBrowser.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../PatternPropertiesDialog.h"
#include "../MainToolBar/MainToolBar.h"
#include "../Skin.h"
#include "../SongPropertiesDialog.h"
#include "../UndoActions.h"
#include "../Widgets/AutomationPathView.h"
#include "../Widgets/Button.h"
#include "../Widgets/Fader.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/MidiLearnableToolButton.h"
#include "../Widgets/PixmapWidget.h"
#include "../WidgetScrollArea.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/PatternList.h>
#include <core/IO/JackAudioDriver.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#ifdef WIN32
#include <time.h>
#endif
using namespace H2Core;


SongEditorPanel::SongEditorPanel( QWidget *pParent ) : QWidget( pParent ) {
	const auto pPref = Preferences::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	const bool bShowPlaybackTrack = pPref->getShowPlaybackTrack();

	Hydrogen*	pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> 		pSong = pHydrogen->getSong();

	assert( pSong );

	setWindowTitle( tr( "Song Editor" ) );

	// background
	auto pBackPanel = new QWidget( nullptr );
	pBackPanel->setObjectName( "SongEditorBackPanel" );
	pBackPanel->setFixedSize( SongEditorPatternList::nWidth,
							  SongEditorPanel::nMinimumHeight / 2 );

	// time line toggle button
	m_bLastIsTimelineActivated = pSong->getIsTimelineActivated();
	m_pTimelineBtn = new Button(
		pBackPanel, QSize( 98, 17 ), Button::Type::Toggle, "",
		pCommonStrings->getTimelineBigButton(), QSize(),
		pCommonStrings->getTimelineEnabled() );
	m_pTimelineBtn->move( 94, 4 );
	m_pTimelineBtn->setObjectName( "TimelineBtn" );
	m_pTimelineBtn->setChecked( m_bLastIsTimelineActivated );
	connect( m_pTimelineBtn, &QPushButton::clicked, [=]() {
		CoreActionController::activateTimeline( m_pTimelineBtn->isChecked() );

		const QString sMessage = QString( "%1 = %2" )
			.arg( pCommonStrings->getTimelineBigButton() )
			.arg( m_pTimelineBtn->isChecked() ? pCommonStrings->getStatusOn() :
				  pCommonStrings->getStatusOff() );
		HydrogenApp::get_instance()->showStatusBarMessage( sMessage );
		Hydrogen::get_instance()->setIsModified( true );
	} );

	////////////////////////////////////////////////////////////////////////////
	m_pToolBar = new QToolBar( nullptr );
	m_pToolBar->setFixedSize( SongEditorPatternList::nWidth,
							  SongEditorPanel::nMinimumHeight / 2 );
	m_pToolBar->setFocusPolicy( Qt::ClickFocus );

	auto createAction = [&]( const QString& sText, bool bCheckable ) {
		auto pAction = new QAction( m_pToolBar );
		pAction->setCheckable( bCheckable );
		pAction->setIconText( sText );
		pAction->setToolTip( sText );

		return pAction;
	};

	m_pClearAction = createAction( tr( "Clear pattern sequence" ), false );
	connect( m_pClearAction, &QAction::triggered, [=]() {
		clearSequence();
	});
	m_pToolBar->addAction( m_pClearAction );

	m_pToolBar->addSeparator();

	m_pNewPatternAction = createAction( tr( "Create new pattern" ), false );
	connect( m_pNewPatternAction, &QAction::triggered, [=]() {
		newPatBtnClicked();
	});
	m_pToolBar->addAction( m_pNewPatternAction );

	m_pToolBar->addSeparator();

	auto pPatternModeGroup = new QButtonGroup( m_pToolBar );
	pPatternModeGroup->setExclusive( true );

	m_pSinglePatternModeButton = new MidiLearnableToolButton(
		m_pToolBar, tr( "selected pattern mode" ) );
	m_pSinglePatternModeButton->setCheckable( true );
	connect( m_pSinglePatternModeButton, &QToolButton::clicked, [=]() {
		Hydrogen::get_instance()->setPatternMode( Song::PatternMode::Selected );
	});
	pPatternModeGroup->addButton( m_pSinglePatternModeButton );
	m_pToolBar->addWidget( m_pSinglePatternModeButton );

	m_pStackedPatternModeButton = new MidiLearnableToolButton(
		m_pToolBar, tr( "stacked pattern mode" ) );
	m_pStackedPatternModeButton->setCheckable( true );
	connect( m_pStackedPatternModeButton, &QToolButton::clicked, [=]() {
		Hydrogen::get_instance()->setPatternMode( Song::PatternMode::Stacked );
	});
	pPatternModeGroup->addButton( m_pStackedPatternModeButton );
	m_pToolBar->addWidget( m_pStackedPatternModeButton );

	m_pToolBar->addSeparator();

	m_pPatternEditorLockedButton = new MidiLearnableToolButton(
		m_pToolBar, pCommonStrings->getPatternEditorLocked() );
	m_pPatternEditorLockedButton->setCheckable( true );
	m_pPatternEditorLockedButton->setObjectName( "PatternEditorLockedButton" );
	connect( m_pPatternEditorLockedButton, &QToolButton::clicked, [=](){
		Hydrogen::get_instance()->setIsPatternEditorLocked(
			m_pPatternEditorLockedButton->isChecked() );
	});
	m_pToolBar->addWidget( m_pPatternEditorLockedButton );

// ZOOM
	m_pHScrollBar = new QScrollBar( Qt::Horizontal, nullptr );
	m_pHScrollBar->setObjectName( "SongEditorPanel HScrollBar" );
	connect( m_pHScrollBar, SIGNAL(valueChanged(int)), this, SLOT( hScrollTo(int) ) );

	// zoom-in btn
	Button* pZoomInBtn = new Button(
		nullptr, QSize( 19, 15 ), Button::Type::Push, "plus.svg", "",
		QSize( 9, 9 ) );
	connect( pZoomInBtn, SIGNAL( clicked() ), this, SLOT( zoomOutBtnClicked() ) );



	// zoom-out btn
	Button* pZoomOutBtn = new Button(
		nullptr, QSize( 19, 15 ), Button::Type::Push, "minus.svg", "",
		QSize( 9, 9 ) );
	connect( pZoomOutBtn, SIGNAL( clicked() ), this, SLOT( zoomInBtnClicked() ) );

	// view playback track toggle button
	m_pViewPlaybackBtn = new Button(
		nullptr, QSize( 19, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getPlaybackTrackButton(), QSize(),
		tr( "View playback track" ) );
	m_pViewPlaybackBtn->setObjectName( "ViewPlaybackBtn" );
	connect( m_pViewPlaybackBtn, SIGNAL( clicked() ),
			 this, SLOT( viewPlaybackTrackBtnClicked() ) );
	m_pViewPlaybackBtn->setChecked( pPref->getShowPlaybackTrack() );
	
	// Playback Fader
	m_pPlaybackTrackFader = new Fader(
		pBackPanel, Fader::Type::Vertical, tr( "Playback track volume" ), false,
		false, 0.0, 1.5 );
	m_pPlaybackTrackFader->setObjectName( "SongEditorPlaybackTrackFader" );
	m_pPlaybackTrackFader->move( 6, 1 );
	m_pPlaybackTrackFader->setValue( pSong->getPlaybackTrackVolume() );
	m_pPlaybackTrackFader->hide();
	connect( m_pPlaybackTrackFader, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( faderChanged( WidgetWithInput* ) ) );

	// mute playback track toggle button
	m_pMutePlaybackBtn = new Button(
		pBackPanel, QSize( 34, 17 ), Button::Type::Toggle, "",
		pCommonStrings->getBigMuteButton(), QSize(), tr( "Mute playback track" ),
		true );
	m_pMutePlaybackBtn->setObjectName( "SongEditorPlaybackTrackMuteButton" );
	m_pMutePlaybackBtn->move( 158, 4 );
	m_pMutePlaybackBtn->hide();
	connect( m_pMutePlaybackBtn, &QPushButton::clicked, [=](bool bChecked){
		Hydrogen::get_instance()->mutePlaybackTrack( ! bChecked );
	});

	if ( pHydrogen->getPlaybackTrackState() == Song::PlaybackTrack::Unavailable ) {
		m_pPlaybackTrackFader->setIsActive( false );
		m_pMutePlaybackBtn->setChecked( true );
		m_pMutePlaybackBtn->setIsActive( false );
	} else {
		m_pMutePlaybackBtn->setChecked( ! pSong->getPlaybackTrackEnabled() );
	}
	
	// edit playback track toggle button
	m_pEditPlaybackBtn = new Button(
		pBackPanel, QSize( 34, 17 ), Button::Type::Push, "",
		pCommonStrings->getEditButton(), QSize(), tr( "Choose playback track") );
	m_pEditPlaybackBtn->setObjectName( "SongEditorPlaybackTrackEditButton" );
	m_pEditPlaybackBtn->move( 123, 4 );
	m_pEditPlaybackBtn->hide();
	connect( m_pEditPlaybackBtn, SIGNAL( clicked() ), this, SLOT( editPlaybackTrackBtnClicked() ) );
	m_pEditPlaybackBtn->setChecked( false );

	// timeline view toggle button
	m_pViewTimelineBtn = new Button(
		nullptr, QSize( 19, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getTimelineButton(), QSize(), tr( "View timeline" ) );
	m_pViewTimelineBtn->setObjectName( "TimeLineToggleBtn" );
	connect( m_pViewTimelineBtn, SIGNAL( clicked() ), this, SLOT( viewTimelineBtnClicked() ) );
	m_pViewTimelineBtn->setChecked( ! pPref->getShowPlaybackTrack() );

	m_pTimelineBtn->setVisible( ! bShowPlaybackTrack );
	m_pMutePlaybackBtn->setVisible( bShowPlaybackTrack );
	m_pEditPlaybackBtn->setVisible( bShowPlaybackTrack );
	m_pPlaybackTrackFader->setVisible( bShowPlaybackTrack );
	m_pViewPlaybackBtn->setChecked( bShowPlaybackTrack );
	m_pViewTimelineBtn->setChecked( ! bShowPlaybackTrack );

	QHBoxLayout *pHZoomLayout = new QHBoxLayout();
	pHZoomLayout->setSpacing( 0 );
	pHZoomLayout->setContentsMargins( 0, 0, 0, 0 );
	pHZoomLayout->addWidget( m_pViewPlaybackBtn );
	pHZoomLayout->addWidget( m_pViewTimelineBtn );
	pHZoomLayout->addWidget( m_pHScrollBar );
	pHZoomLayout->addWidget( pZoomInBtn );
	pHZoomLayout->addWidget( pZoomOutBtn );

	QWidget *pHScrollbarPanel = new QWidget();
	pHScrollbarPanel->setLayout( pHZoomLayout );

	updatePatternEditorLocked();
	updatePatternMode();
	updateTimeline();

	// ~ ZOOM

	// PATTERN LIST
	m_pPatternListScrollView = new WidgetScrollArea( nullptr );
	m_pPatternListScrollView->setObjectName( "PatternListScrollView" );
	m_pPatternListScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pPatternListScrollView->setFrameShape( QFrame::NoFrame );
	m_pPatternListScrollView->setFixedWidth( m_nPatternListWidth );
	m_pPatternListScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPatternListScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	connect( m_pPatternListScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( vScrollTo(int) ) );

	m_pPatternList = new SongEditorPatternList( m_pPatternListScrollView->viewport() );
	m_pPatternList->setFocusPolicy( Qt::ClickFocus );
	m_pPatternListScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pPatternListScrollView->setWidget( m_pPatternList );

	// EDITOR
	m_pEditorScrollView = new WidgetScrollArea( nullptr );
	m_pEditorScrollView->setObjectName( "EditorScrollView" );
	m_pEditorScrollView->setFocusPolicy( Qt::ClickFocus );
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
	m_pWidgetStack->setFixedHeight( SongEditorPanel::nMinimumHeight );
	
	m_pPositionRulerScrollView = new WidgetScrollArea( m_pWidgetStack );
	m_pPositionRulerScrollView->setObjectName( "PositionRulerScrollView" );
	m_pPositionRulerScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pPositionRulerScrollView->setFrameShape( QFrame::NoFrame );
	m_pPositionRulerScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPositionRulerScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPositionRulerScrollView->setFocusPolicy( Qt::NoFocus );
	m_pPositionRuler = new SongEditorPositionRuler( m_pPositionRulerScrollView->viewport() );
	m_pPositionRuler->setObjectName( "SongEditorPositionRuler" );
	m_pPositionRulerScrollView->setWidget( m_pPositionRuler );
	m_pPositionRulerScrollView->setFixedHeight( SongEditorPanel::nMinimumHeight );
	connect( m_pPositionRulerScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( hScrollTo(int) ) );

	m_pPositionRuler->setFocusPolicy( Qt::ClickFocus );
	m_pPositionRuler->setFocusProxy( m_pSongEditor );

	m_pPlaybackTrackScrollView = new WidgetScrollArea( m_pWidgetStack );
	m_pPlaybackTrackScrollView->setObjectName( "PlaybackTrackScrollView" );
	m_pPlaybackTrackScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pPlaybackTrackScrollView->setFrameShape( QFrame::NoFrame );
	m_pPlaybackTrackScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPlaybackTrackScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	auto pCompo = Hydrogen::get_instance()->getAudioEngine()->getSampler()->getPlaybackTrackInstrument()->getComponents()->front();
	assert(pCompo);

	m_pPlaybackTrackWaveDisplay = new PlaybackTrackWaveDisplay( m_pPlaybackTrackScrollView->viewport() );
	m_pPlaybackTrackWaveDisplay->setSampleNameAlignment( Qt::AlignLeft );
	m_pPlaybackTrackWaveDisplay->resize( m_pPositionRuler->width() , SongEditorPanel::nMinimumHeight);
	m_pPlaybackTrackWaveDisplay->setAcceptDrops( true );
	
	m_pPlaybackTrackScrollView->setWidget( m_pPlaybackTrackWaveDisplay );
	m_pPlaybackTrackScrollView->setFixedHeight( SongEditorPanel::nMinimumHeight );
	
	m_pAutomationPathScrollView = new WidgetScrollArea( nullptr );
	m_pAutomationPathScrollView->setObjectName( "AutomationPathScrollView" );
	m_pAutomationPathScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pAutomationPathScrollView->setFrameShape( QFrame::NoFrame );
	m_pAutomationPathScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pAutomationPathScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pAutomationPathView = new AutomationPathView( m_pAutomationPathScrollView->viewport() );
	m_pAutomationPathView->setObjectName( "SongEditorAutomationPathView" );
	m_pAutomationPathScrollView->setWidget( m_pAutomationPathView );
	m_pAutomationPathScrollView->setFixedHeight(
		AutomationPathView::m_nMinimumHeight );
	connect( m_pAutomationPathView, SIGNAL( pointAdded(float, float) ), this, SLOT( automationPathPointAdded(float,float) ) );
	connect( m_pAutomationPathView, SIGNAL( pointRemoved(float, float) ), this, SLOT( automationPathPointRemoved(float,float) ) );
	connect( m_pAutomationPathView, SIGNAL( pointMoved(float, float, float, float) ), this, SLOT( automationPathPointMoved(float,float, float, float) ) );

	m_pAutomationCombo = new LCDCombo( nullptr, QSize( m_nPatternListWidth, 18 ), true );
	m_pAutomationCombo->setToolTip( tr("Adjust parameter values in time") );
	m_pAutomationCombo->addItem( pCommonStrings->getNotePropertyVelocity() );
	m_pAutomationCombo->setCurrentIndex( 0 );

	m_pVScrollBar = new QScrollBar( Qt::Vertical, nullptr );
	m_pVScrollBar->setObjectName( "SongEditorPanel VScrollBar" );
	connect( m_pVScrollBar, SIGNAL(valueChanged(int)), this, SLOT( vScrollTo(int) ) );

	m_pWidgetStack->addWidget( m_pPositionRulerScrollView );
	m_pWidgetStack->addWidget( m_pPlaybackTrackScrollView );

	if ( bShowPlaybackTrack ) {
		m_pWidgetStack->setCurrentWidget( m_pPlaybackTrackScrollView );
	} else {
		m_pWidgetStack->setCurrentWidget( m_pPositionRulerScrollView );
	}

	QWidget *pMainPanel = new QWidget();
	pMainPanel->setObjectName( "SongEditorPanel" );
	QGridLayout *pGridLayout = new QGridLayout();
	pGridLayout->setSpacing( 0 );
	pGridLayout->setContentsMargins( 0, 0, 0, 0 );

	pGridLayout->addWidget( pBackPanel, 0, 0 );
	pGridLayout->addWidget( m_pToolBar, 1, 0 );
	pGridLayout->addWidget( m_pWidgetStack, 0, 1, 2, 1 );
	pGridLayout->addWidget( m_pPatternListScrollView, 2, 0 );
	pGridLayout->addWidget( m_pEditorScrollView, 2, 1 );
	pGridLayout->addWidget( m_pVScrollBar, 2, 2, 2, 1 );
	pGridLayout->addWidget( m_pAutomationPathScrollView, 3, 1);
	pGridLayout->addWidget( m_pAutomationCombo, 3, 0,
							Qt::AlignVCenter | Qt::AlignRight );
	pGridLayout->addWidget( pHScrollbarPanel, 4, 1 );
	if( !pPref->getShowAutomationArea() ){
		m_pAutomationPathScrollView->hide();
		m_pAutomationCombo->hide();
	}

	pMainPanel->setLayout( pGridLayout );

	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setContentsMargins( 0, 0, 0, 0 );
	setLayout( pVBox );
	pVBox->addWidget( pMainPanel );

	updateIcons();
	updateStyleSheet();
	updateEditors( Editor::Update::Background );
	updateTimeline();

	HydrogenApp::get_instance()->addEventListener( this );

	m_pHighlightLockedTimer = new QTimer( this );
	m_pHighlightLockedTimer->setSingleShot( true );
	connect( m_pHighlightLockedTimer, &QTimer::timeout, [=](){
		m_pPatternEditorLockedButton->setStyleSheet( "" );
	});

	m_pTimer = new QTimer( this );
	
	connect( m_pTimer, SIGNAL(timeout()), this, SLOT( updatePlayHeadPosition() ) );
	connect( m_pTimer, SIGNAL(timeout()),
			 this, SLOT( updatePlaybackFaderPeaks() ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &SongEditorPanel::onPreferencesChanged );
	
	m_pTimer->start( 100 );
}

SongEditorPanel::~SongEditorPanel() {
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

		int nPlayHeadPosition = pAudioEngine->getTransportPosition()->getColumn() *
			m_pSongEditor->getGridWidth();

		int value = m_pEditorScrollView->horizontalScrollBar()->value();

		// Distance the grid will be moved left or right.
		int nIncrement = 100;
		if ( nIncrement > std::round( static_cast<float>(nWidth) / 3 ) ) {
			nIncrement = std::round( static_cast<float>(nWidth) / 3 );
		} else if ( nIncrement < 2 * Skin::nPlayheadWidth ) {
			nIncrement = 2 * Skin::nPlayheadWidth;
		}
		
		if ( nPlayHeadPosition > ( x + nWidth - std::floor( static_cast<float>( nIncrement ) / 2 ) ) ) {
			hScrollTo( value + nIncrement );
		}
		else if ( nPlayHeadPosition < x ) {
			hScrollTo( value - nIncrement );
		}
	}
}

void SongEditorPanel::highlightPatternEditorLocked() {
	const auto theme = Preferences::get_instance()->getTheme();
	m_pPatternEditorLockedButton->setStyleSheet( QString( "\
#PatternEditorLockedButton {\
    background-color: %1;\
}" ).arg( theme.m_color.m_buttonRedColor.name() ) );

	m_pHighlightLockedTimer->start( 250 );
}

void SongEditorPanel::updatePlaybackFaderPeaks()
{
	Sampler*		pSampler = Hydrogen::get_instance()->getAudioEngine()->getSampler();
	const auto pPref = Preferences::get_instance();
	auto		pInstrument = pSampler->getPlaybackTrackInstrument();

	
	bool bShowPeaks = pPref->showInstrumentPeaks();
	float fallOff = pPref->getTheme().m_interface.m_fMixerFalloffSpeed;
	
	// fader
	float fOldPeak_L = m_pPlaybackTrackFader->getPeak_L();
	float fOldPeak_R = m_pPlaybackTrackFader->getPeak_R();
	
	float fNewPeak_L = pInstrument->getPeak_L();
	pInstrument->setPeak_L( 0.0f );	// reset instrument peak

	float fNewPeak_R = pInstrument->getPeak_R();
	pInstrument->setPeak_R( 0.0f );	// reset instrument peak

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

void SongEditorPanel::ensureCursorIsVisible() {
	const int nSelectedPattern =
		Hydrogen::get_instance()->getSelectedPatternNumber();

	// Make sure currently selected pattern is visible.
	if ( nSelectedPattern != -1 ) {
		const int nGridHeight = m_pPatternList->getGridHeight();
		m_pPatternListScrollView->ensureVisible(
			0, nSelectedPattern * nGridHeight + nGridHeight / 2, 0, nGridHeight );
	}
}

void SongEditorPanel::updateEditors( Editor::Update update )
{
	m_pPatternList->updateEditor();
	m_pSongEditor->updateEditor( update );
	m_pPositionRuler->updateEditor();
	m_pAutomationPathView->updateAutomationPath();
	updatePlaybackTrack();

	resyncExternalScrollBar();
}

void SongEditorPanel::updatePlaybackTrack()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	if ( ! Preferences::get_instance()->getShowPlaybackTrack() ) {
		return;
	}

	if ( pHydrogen->getPlaybackTrackState() == Song::PlaybackTrack::Unavailable ||
		 pSong == nullptr ) {
		// No playback track chosen (stored in the current song).
		m_pPlaybackTrackFader->setIsActive( false );
		m_pMutePlaybackBtn->setChecked( true );
		m_pMutePlaybackBtn->setIsActive( false );

		m_pPlaybackTrackWaveDisplay->updateDisplay( nullptr );
	}
	else {
		// Playback track was selected by the user and is ready to
		// use.
		m_pPlaybackTrackFader->setIsActive( true );
		m_pMutePlaybackBtn->setIsActive( true );
		if ( pHydrogen->getPlaybackTrackState() == Song::PlaybackTrack::Muted ) {
			m_pMutePlaybackBtn->setChecked( true );
		} else {
			m_pMutePlaybackBtn->setChecked( false );
		}

		auto pPlaybackCompo = pHydrogen->getAudioEngine()->getSampler()->
			getPlaybackTrackInstrument()->getComponents()->front();
			
		m_pPlaybackTrackWaveDisplay->updateDisplay( pPlaybackCompo->getLayer(0) );
	}
}

///
/// Create a new pattern
///
void SongEditorPanel::newPatBtnClicked()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();
	auto pNewPattern = std::make_shared<Pattern>(
		tr( "Pattern %1" ).arg( pPatternList->size() + 1 ) );
	pNewPattern->setAuthor( pSong->getAuthor() );
	pNewPattern->setLicense( pSong->getLicense() );
	PatternPropertiesDialog *pDialog = new PatternPropertiesDialog( this, pNewPattern, 0, true );

	if ( pDialog->exec() == QDialog::Accepted ) {
		int nRow;
		if ( pHydrogen->getSelectedPatternNumber() == -1 ) {
			nRow = pPatternList->size();
		} else {
			nRow = pHydrogen->getSelectedPatternNumber() + 1;
		}
		SE_insertPatternAction* pAction = new SE_insertPatternAction(
			nRow, std::make_shared<Pattern>( pNewPattern ) );
		HydrogenApp::get_instance()->pushUndoCommand( pAction );
	}

	delete pDialog;
}

void SongEditorPanel::clearSequence() {
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	if ( QMessageBox::information(
			 this, "Hydrogen",
			 tr( "Warning, this will erase your pattern sequence.\nAre you sure?"),
			 QMessageBox::Ok | QMessageBox::Cancel,
			 QMessageBox::Cancel ) == QMessageBox::Cancel ) {
		return;
	}

	const QString sFilename = Filesystem::tmp_file_path( "SEQ.xml" );
	auto pAction = new SE_deletePatternSequenceAction( sFilename );
	HydrogenApp::get_instance()->pushUndoCommand( pAction );
}

bool SongEditorPanel::hasSongEditorFocus() const{
	return hasFocus() ||
		m_pEditorScrollView->hasFocus() ||
		m_pPatternListScrollView->hasFocus() ||
		m_pPositionRulerScrollView->hasFocus() ||
		m_pPlaybackTrackScrollView->hasFocus() ||
		m_pVScrollBar->hasFocus() ||
		m_pHScrollBar->hasFocus() ||
		m_pWidgetStack->hasFocus() ||
		m_pSongEditor->hasFocus() ||
		m_pPatternList->hasFocus() ||
		m_pPositionRuler->hasFocus() ||
		m_pPlaybackTrackWaveDisplay->hasFocus();
}

void SongEditorPanel::restoreGroupVector( const QString& filename )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pAudioEngine = pHydrogen->getAudioEngine();
	//clear the old sequese
	auto pPatternGroupsVect = pSong->getPatternGroupVector();
	for (uint i = 0; i < pPatternGroupsVect->size(); i++) {
		auto pPatternList = (*pPatternGroupsVect)[i];
		pPatternList->clear();
	}
	pPatternGroupsVect->clear();

	pAudioEngine->lock( RIGHT_HERE );
	pSong->loadTempPatternList( filename );
	pHydrogen->updateSongSize();
	pHydrogen->updateSelectedPattern( false );
	pAudioEngine->unlock();

	m_pPositionRuler->updateSongSize();
	updateEditors( Editor::Update::Content );
}

void SongEditorPanel::gridCellToggledEvent() {
	updateEditors( Editor::Update::Content );
}

void SongEditorPanel::jackTimebaseStateChangedEvent( int ) {
	updateJacktimebaseState();
	m_pPositionRuler->updateEditor();
}

void SongEditorPanel::midiClockActivationEvent() {
	updateTimeline();
}

void SongEditorPanel::nextPatternsChangedEvent() {
	m_pPatternList->updateEditor();
}

void SongEditorPanel::patternEditorLockedEvent() {
	updatePatternEditorLocked();

	if ( Hydrogen::get_instance()->isPatternEditorLocked() ) {
		m_pPatternList->updateEditor();
	}
}

void SongEditorPanel::patternModifiedEvent() {
	updateEditors( Editor::Update::Content );
}

void SongEditorPanel::playbackTrackChangedEvent() {
	updatePlaybackTrack();
}

void SongEditorPanel::playingPatternsChangedEvent() {
	// Triggered every time the column of the SongEditor grid
	// changed. Either by rolling transport or by relocation.
	// In Song mode, we may scroll to change position in the Song Editor.
	if ( Hydrogen::get_instance()->getMode() == Song::Mode::Song ) {

		// Scroll vertically to keep currently playing patterns in view
		int nPatternInView = -1;
		int scroll = m_pSongEditor->yScrollTarget( m_pEditorScrollView, &nPatternInView );
		vScrollTo( scroll );
	}

	m_pPatternList->updateEditor();
	m_pPositionRuler->update();
}

void SongEditorPanel::relocationEvent() {
	if ( Hydrogen::get_instance()->isPatternEditorLocked() ) {
		m_pSongEditor->updateEditor( Editor::Update::Background );
		m_pPatternList->updateEditor();
	}

	m_pPositionRuler->updatePosition();
}

void SongEditorPanel::selectedPatternChangedEvent() {
	ensureCursorIsVisible();
	updateEditors( Editor::Update::Background );
}

void SongEditorPanel::songModeActivationEvent() {
	updateTimeline();
	updatePatternEditorLocked();
	updatePatternMode();
	updateStyleSheet();

	m_pPatternList->updateEditor();
	m_pPositionRuler->updatePosition();
	m_pPositionRuler->updateEditor();
}

void SongEditorPanel::songSizeChangedEvent() {
	m_pSongEditor->updateEditor( Editor::Update::Content );
	m_pPositionRuler->updateSongSize();
	m_pPositionRuler->updateEditor();
}

void SongEditorPanel::stackedModeActivationEvent( int ) {
	updatePatternMode();
	m_pPatternList->updateEditor();
}

void SongEditorPanel::stateChangedEvent( const H2Core::AudioEngine::State& ) {
	// The lock button is highlighted when transport is rolling.
	updatePatternEditorLocked();

	if ( Hydrogen::get_instance()->isPatternEditorLocked() ) {
		m_pPatternList->updateEditor();
	}
}

void SongEditorPanel::tempoChangedEvent( int nValue ) {
	auto pTimeline = Hydrogen::get_instance()->getTimeline();
	if ( ! pTimeline->isFirstTempoMarkerSpecial() ) {
		return;
	}

	// There is just the special tempo marker -> no tempo markers set
	// by the user. In this case the special marker isn't drawn and
	// doesn't need to be update.
	if ( pTimeline->getAllTempoMarkers().size() == 1 ) {
		return;
	}

	m_pPositionRuler->updateEditor();
}

void SongEditorPanel::timelineActivationEvent(){
	updateTimeline();
	m_pPositionRuler->updateEditor();
}

void SongEditorPanel::timelineUpdateEvent( int nValue ) {
	updateEditors( Editor::Update::Transient );
}

void SongEditorPanel::updateSongEvent( int nValue ) {

	if ( nValue != 0 ) {
		return;
	}

	// Song loaded

	ensureCursorIsVisible();
	updatePatternMode();
	updateJacktimebaseState();
	updatePatternEditorLocked();
	updateTimeline();
	updateStyleSheet();

	m_pPositionRuler->updatePosition();
	m_pPositionRuler->updateSongSize();
	updateEditors( Editor::Update::Background );
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

	// Update visibility buttons.
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
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

	updatePlaybackTrack();

	// Update visibility buttons.
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
}

void SongEditorPanel::viewTimelineBtnClicked()
{
	if ( m_pViewTimelineBtn->isChecked() ){
		showTimeline();
	} else {
		showPlaybackTrack();
	}
}

void SongEditorPanel::viewPlaybackTrackBtnClicked()
{
	if ( m_pViewPlaybackBtn->isChecked() ){
		showPlaybackTrack();
	} else {
		showTimeline();
	}
}

void SongEditorPanel::editPlaybackTrackBtnClicked()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pHydrogen->getAudioEngine()->getState() ==
		 H2Core::AudioEngine::State::Playing ) {
		pHydrogen->sequencerStop();
	}

	QString sPath, sFilename;

	if ( ! pSong->getPlaybackTrackFilename().isEmpty() ) {
		QFileInfo fileInfo( pSong->getPlaybackTrackFilename() );
		sFilename = pSong->getPlaybackTrackFilename();
		sPath = fileInfo.absoluteDir().absolutePath();
	} else {
		sFilename = "";
		sPath = Preferences::get_instance()->getLastOpenPlaybackTrackDirectory();
	}
	
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = QDir::homePath();
	}
	
	//use AudioFileBrowser, but don't allow multi-select. Also, hide all no necessary controls.
	AudioFileBrowser *pFileBrowser =
		new AudioFileBrowser( nullptr, false, false, sPath, sFilename );
	
	QStringList filenameList;
	
	if ( pFileBrowser->exec() == QDialog::Accepted ) {

		// Only overwrite the default directory if we didn't start
		// from an existing file or the final directory differs from
		// the starting one.
		if ( sFilename.isEmpty() ||
			 sPath != pFileBrowser->getSelectedDirectory() ) {
			Preferences::get_instance()->setLastOpenPlaybackTrackDirectory( pFileBrowser->getSelectedDirectory() );
		}
		filenameList = pFileBrowser->getSelectedFiles();
	}

	delete pFileBrowser;

	if( filenameList.size() != 3 ) {
		return;
	}
	
	if ( filenameList[2].isEmpty() ) {
		return;
	}

	pHydrogen->loadPlaybackTrack( filenameList[2] );
}

void SongEditorPanel::zoomInBtnClicked()
{
	auto pPref = Preferences::get_instance();

	unsigned width = m_pSongEditor->getGridWidth();
	--width;
	m_pSongEditor->setGridWidth( width );
	m_pPositionRuler->setGridWidth( width );
	m_pAutomationPathView->setGridWidth( width );

	pPref->setSongEditorGridWidth( width );
	pPref->setSongEditorGridHeight( m_pSongEditor->getGridHeight() );
	
	updateEditors( Editor::Update::Background );
}

void SongEditorPanel::zoomOutBtnClicked()
{
	auto pPref = Preferences::get_instance();

	unsigned width = m_pSongEditor->getGridWidth();
	++width;
	m_pSongEditor->setGridWidth( width );
	m_pPositionRuler->setGridWidth( width );
	m_pAutomationPathView->setGridWidth( width );

	pPref->setSongEditorGridWidth( width );
	pPref->setSongEditorGridHeight( m_pSongEditor->getGridHeight() );
	
	updateEditors( Editor::Update::Background );
}

void SongEditorPanel::faderChanged( WidgetWithInput *pRef )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		return;
	}
	
	Fader* pFader = dynamic_cast<Fader*>( pRef );
	const float fNewValue = std::round( pFader->getValue() * 100 ) / 100;

	if ( pSong->getPlaybackTrackVolume() != fNewValue ) {
		pSong->setPlaybackTrackVolume( fNewValue );
		HydrogenApp::get_instance()->showStatusBarMessage(
			tr( "Playback volume set to" )
			.append( QString( " [%1]" ).arg( fNewValue ) ),
			"SongEditorPanel:PlaybackTrackVolume" );
	}
}

void SongEditorPanel::automationPathPointAdded(float x, float y)
{
	H2Core::AutomationPath *pPath = m_pAutomationPathView->getAutomationPath();
	auto pUndoAction = new SE_automationPathAddPointAction(pPath, x, y);
	HydrogenApp::get_instance()->pushUndoCommand( pUndoAction );
}


void SongEditorPanel::automationPathPointRemoved(float x, float y)
{
	H2Core::AutomationPath *pPath = m_pAutomationPathView->getAutomationPath();
	auto pUndoAction = new SE_automationPathRemovePointAction(pPath, x, y);
	HydrogenApp::get_instance()->pushUndoCommand( pUndoAction );
}


void SongEditorPanel::automationPathPointMoved(float ox, float oy, float tx, float ty)
{
	H2Core::AutomationPath *pPath = m_pAutomationPathView->getAutomationPath();
	auto pUndoAction = new SE_automationPathMovePointAction(pPath, ox, oy, tx, ty);
	HydrogenApp::get_instance()->pushUndoCommand( pUndoAction );
}

void SongEditorPanel::toggleAutomationAreaVisibility()
{
	auto pPref = Preferences::get_instance();
	
	if ( ! pPref->getShowAutomationArea() )	{
		m_pAutomationPathScrollView->show();
		m_pAutomationCombo->show();
		pPref->setShowAutomationArea( true );
	}
	else {
		m_pAutomationPathScrollView->hide();
		m_pAutomationCombo->hide();
		pPref->setShowAutomationArea( false );
	}

	// Update visibility buttons.
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
}

void SongEditorPanel::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	const auto pPref = H2Core::Preferences::get_instance();
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateStyleSheet();
	}

	if ( changes & ( H2Core::Preferences::Changes::GeneralTab |
					 H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font |
					 H2Core::Preferences::Changes::AppearanceTab ) ) {
		const int nNewWidth = SongEditor::nMargin + pPref->getMaxBars() *
			m_pSongEditor->getGridWidth() ;
		m_pSongEditor->resize( nNewWidth, m_pSongEditor->height() );
		m_pPositionRuler->resize( nNewWidth, m_pPositionRuler->height() );

		updateEditors( Editor::Update::Background );
	}
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

void SongEditorPanel::updateIcons() {
	QColor color;
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getTheme().m_interface.m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
		color = Qt::white;
	} else {
		sIconPath.append( "/icons/black/" );
		color = Qt::black;
	}

	m_pClearAction->setIcon( QIcon( sIconPath + "bin.svg" ) );
	m_pNewPatternAction->setIcon( QIcon( sIconPath + "plus.svg" ) );
	m_pPatternEditorLockedButton->setIcon(
		QIcon( sIconPath +
			   ( Hydrogen::get_instance()->isPatternEditorLocked() ?
				 "lock_open.svg" : "lock_closed" ) ) );
	m_pSinglePatternModeButton->setIcon( QIcon( sIconPath + "single_layer.svg" ) );
	m_pStackedPatternModeButton->setIcon(
		QIcon( sIconPath + "multiple_layers.svg" ) );
}

void SongEditorPanel::updateJacktimebaseState() {
	updateTimeline();
}

void SongEditorPanel::updatePatternEditorLocked() {
	QColor color;
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getTheme().m_interface.m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
		color = Qt::white;
	} else {
		sIconPath.append( "/icons/black/" );
		color = Qt::black;
	}

	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getMode() == Song::Mode::Song ) {
		m_pPatternEditorLockedButton->setEnabled( true );
	}

	if ( pHydrogen->isPatternEditorLocked() ) {
		m_pPatternEditorLockedButton->setIcon(
			QIcon( sIconPath + "lock_closed" ) );
		m_pPatternEditorLockedButton->setChecked(
			pHydrogen->getAudioEngine()->getState() ==
			AudioEngine::State::Playing );
	}
	else {
		m_pPatternEditorLockedButton->setIcon(
			QIcon( sIconPath + "lock_open.svg" ) );
		m_pPatternEditorLockedButton->setChecked( false );
	}

	if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
		m_pPatternEditorLockedButton->setEnabled( false );
	}
}

void SongEditorPanel::updatePatternMode() {
	auto pHydrogen = Hydrogen::get_instance();

	// We access the raw variable in the song class since we do not
	// care whether Hydrogen is in song or pattern mode in here.
	if ( pHydrogen->getSong() == nullptr ||
		 pHydrogen->getSong()->getPatternMode() == Song::PatternMode::Selected ) {
		m_pSinglePatternModeButton->setChecked( true );
		m_pStackedPatternModeButton->setChecked( false );
	}
	else {
		m_pSinglePatternModeButton->setChecked( false );
		m_pStackedPatternModeButton->setChecked( true );
	}

	// Those buttons are only accessible in pattern mode.
	m_pSinglePatternModeButton->setDisabled(
		pHydrogen->getMode() == Song::Mode::Song );
	m_pStackedPatternModeButton->setDisabled(
		pHydrogen->getMode() == Song::Mode::Song );
}

void SongEditorPanel::updateStyleSheet() {
	const auto colorTheme = Preferences::get_instance()->getTheme().m_color;
	const QColor colorToolBar = colorTheme.m_songEditor_backgroundColor;
	const QColor colorToolBarText = colorTheme.m_songEditor_textColor;

	QColor backgroundInactiveColor;
	if ( Hydrogen::get_instance()->getMode() == Song::Mode::Song ) {
		backgroundInactiveColor = colorTheme.m_windowColor.lighter(
		 	Skin::nEditorActiveScaling );
	}
	else {
		backgroundInactiveColor = colorTheme.m_windowColor;
	}

	setStyleSheet( QString( "\
#PatternListScrollView, #EditorScrollView, #PositionRulerScrollView,\
#PlaybackTrackScrollView, #AutomationPathScrollView, #SongEditorPanel {\
     background-color: %1;\
}\
#SongEditorBackPanel {\
     background-color: %2;\
     border: 1px solid #000;\
}" )
				   .arg( backgroundInactiveColor.name() )
				   .arg( colorToolBar.name() ) );

	QColor colorToolBarChecked, colorToolBarHovered;
	if ( Skin::moreBlackThanWhite( colorToolBar ) ) {
		colorToolBarChecked = colorToolBar.lighter(
			Skin::nToolBarCheckedScaling );
		colorToolBarHovered = colorToolBar.lighter(
			Skin::nToolBarHoveredScaling );
	}
	else {
		colorToolBarChecked = colorToolBar.darker(
			Skin::nToolBarCheckedScaling );
		colorToolBarHovered = colorToolBar.darker(
			Skin::nToolBarHoveredScaling );
	}

	m_pToolBar->setStyleSheet( QString( "\
QToolBar {\
     background-color: %1; \
     color: %2; \
     border: 1px solid #000;\
     spacing: 2px;\
}\
QToolButton {\
    background-color: %1; \
}\
QToolButton:checked {\
    background-color: %3;\
}\
QToolButton:hover {\
    background-color: %4;\
}\
QToolButton:hover, QToolButton:checked {\
    background-color: %3;\
}\
QToolButton:hover, QToolButton:pressed {\
    background-color: %3;\
}")
							   .arg( colorToolBar.name() )
							   .arg( colorToolBarText.name() )
							   .arg( colorToolBarChecked.name() )
							   .arg( colorToolBarHovered.name() ) );

	m_pMutePlaybackBtn->setCheckedBackgroundColor( colorTheme.m_muteColor );
	m_pMutePlaybackBtn->setCheckedBackgroundTextColor(
		colorTheme.m_muteTextColor );

}

void SongEditorPanel::updateTimeline() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto tempoSource = pHydrogen->getTempoSource();

	if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
		m_pTimelineBtn->setEnabled( false );
		m_pTimelineBtn->setChecked( false );
		m_pTimelineBtn->setToolTip(
			pCommonStrings->getTimelineDisabledPatternMode() );
	}
	else if ( tempoSource == Hydrogen::Tempo::Midi ) {
		m_pTimelineBtn->setEnabled( false );
		m_pTimelineBtn->setChecked( false );
		m_pTimelineBtn->setToolTip(
			pCommonStrings->getTimelineDisabledMidiClock() );
	}
	else if ( tempoSource == Hydrogen::Tempo::Jack ) {
		m_pTimelineBtn->setEnabled( false );
		m_pTimelineBtn->setChecked( false );
		m_pTimelineBtn->setToolTip(
			pCommonStrings->getTimelineDisabledTimebaseListener() );
	}
	else {
		m_pTimelineBtn->setEnabled( true );
		m_pTimelineBtn->setChecked( pSong->getIsTimelineActivated() );
		m_pTimelineBtn->setToolTip( pCommonStrings->getTimelineEnabled() );
	}
}
