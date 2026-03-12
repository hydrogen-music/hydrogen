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
#include "../UndoActions.h"
#include "../Widgets/AutomationPathView.h"
#include "../Widgets/Button.h"
#include "../Widgets/Fader.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/MidiLearnableToolButton.h"
#include "../Widgets/MuteButton.h"
#include "../WidgetScrollArea.h"
#include "SongEditor/SongEditorPatternList.h"
#include "SongEditor/SongEditorPositionRuler.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
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

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	assert( pSong );

	setWindowTitle( tr( "Song Editor" ) );

	auto createAction = [&]( QWidget* pParent, const QString& sText,
							 bool bCheckable ) {
		auto pAction = new QAction( pParent );
		pAction->setCheckable( bCheckable );
		pAction->setIconText( sText );
		pAction->setToolTip( sText );
		pParent->addAction( pAction );

		return pAction;
	};

	////////////////////////////////////////////////////////////////////////////

	m_pPlaybackTrackSidebar = new QWidget( this );
	m_pPlaybackTrackSidebar->setFixedWidth( SongEditorPatternList::nWidth );
	m_pPlaybackTrackSidebar->setStyleSheet( "border-right: 1px solid #000" );
	auto pPlaybackTrackSidebarLayout = new QVBoxLayout();
	pPlaybackTrackSidebarLayout->setAlignment( Qt::AlignBottom );
	pPlaybackTrackSidebarLayout->setContentsMargins( 0, 0, 0, 0 );
	pPlaybackTrackSidebarLayout->setSpacing( 0 );
	m_pPlaybackTrackSidebar->setLayout( pPlaybackTrackSidebarLayout );

	// Playback Fader
	const auto pPlaybackTrackInstrument = pSong->getPlaybackTrackInstrument();
	m_pPlaybackTrackFader = new Fader(
		m_pPlaybackTrackSidebar,
		QSize(
			SongEditorPatternList::nWidth,
			SongEditorPanel::nHeaderWidgetHeight / 2
		),
		Fader::Type::Horizonal, tr( "Playback track volume" ), false, false,
		0.0, 1.5
	);
	m_pPlaybackTrackFader->setObjectName( "SongEditorPlaybackTrackFader" );
	if ( pPlaybackTrackInstrument != nullptr ) {
		m_pPlaybackTrackFader->setValue( pPlaybackTrackInstrument->getVolume()
		);
	}
	connect(
		m_pPlaybackTrackFader, SIGNAL( valueChanged( WidgetWithInput* ) ), this,
		SLOT( faderChanged( WidgetWithInput* ) )
	);
	pPlaybackTrackSidebarLayout->addWidget( m_pPlaybackTrackFader );

	auto pPlaybackTrackToolBarContainer = new QWidget( m_pPlaybackTrackSidebar );
	pPlaybackTrackToolBarContainer->setObjectName(
		"PlaybackTrackToolBarContainer"
	);
	pPlaybackTrackSidebarLayout->addWidget( pPlaybackTrackToolBarContainer );
	auto pPlaybackTrackToolBarContainerLayout = new QHBoxLayout();
	pPlaybackTrackToolBarContainerLayout->setContentsMargins( 1, 1, 1, 1 );
	pPlaybackTrackToolBarContainerLayout->setSpacing( 0 );
	pPlaybackTrackToolBarContainer->setLayout(
		pPlaybackTrackToolBarContainerLayout
	);

	pPlaybackTrackToolBarContainerLayout->addStretch();

	auto pPlaybackTrackToolBar = new QToolBar( pPlaybackTrackToolBarContainer );
	pPlaybackTrackToolBar->setFixedHeight(
		SongEditorPanel::nHeaderWidgetHeight / 2
	);
	pPlaybackTrackToolBarContainerLayout->addWidget( pPlaybackTrackToolBar );

	m_pLoadPlaybackTrackAction = createAction(
		pPlaybackTrackToolBar, pCommonStrings->getActionAddPlaybackTrack(),
		false
	);
	m_pLoadPlaybackTrackAction->setObjectName(
		"SongEditorPlaybackTrackEditButton"
	);
	connect( m_pLoadPlaybackTrackAction, &QAction::triggered, [=]() {
		auto pPref = Preferences::get_instance();
		auto pHydrogen = Hydrogen::get_instance();
		auto pSong = pHydrogen->getSong();
		if ( pSong == nullptr ) {
			return;
		}
		QString sPath, sFileName;
		if ( pSong->getPlaybackTrackInstrument() != nullptr ) {
			sFileName = pSong->getPlaybackTrackInstrument()
							->getComponent( 0 )
							->getLayer( 0 )
							->getSample()
							->getFilePath();
			QFileInfo fileInfo( sFileName );
			sPath = fileInfo.absoluteDir().absolutePath();
		}
		else {
			sFileName = "";
			sPath = pPref->getLastOpenPlaybackTrackDirectory();
		}

		if ( !Filesystem::dir_readable( sPath, false ) ) {
			sPath = QDir::homePath();
		}

		QStringList filenameList;
		// use AudioFileBrowser, but don't allow multi-select. Also, hide all no
		// necessary controls.
		auto pFileBrowser =
			new AudioFileBrowser( nullptr, false, false, sPath, sFileName );
		if ( pFileBrowser->exec() == QDialog::Accepted ) {
			// Only overwrite the default directory if we didn't start
			// from an existing file or the final directory differs from
			// the starting one.
			if ( sFileName.isEmpty() ||
				 sPath != pFileBrowser->getSelectedDirectory() ) {
				pPref->setLastOpenPlaybackTrackDirectory(
					pFileBrowser->getSelectedDirectory()
				);
			}
			filenameList = pFileBrowser->getSelectedFiles();
		}
		delete pFileBrowser;

		if ( filenameList.size() != 3 || filenameList[2].isEmpty() ) {
			return;
		}
		pHydrogen->loadPlaybackTrack( filenameList[2] );
	} );

	m_pEditPlaybackTrackAction = createAction(
		pPlaybackTrackToolBar, pCommonStrings->getActionEditPlaybackTrack(),
		false
	);
	connect( m_pEditPlaybackTrackAction, &QAction::triggered, [=]() {
		auto pSong = Hydrogen::get_instance()->getSong();
		if ( pSong == nullptr ) {
			return;
		}
		auto pInstrument = pSong->getPlaybackTrackInstrument();
		if ( pInstrument == nullptr ||
			 pInstrument->getComponent( 0 ) == nullptr ||
			 pInstrument->getComponent( 0 )->getLayer( 0 ) == nullptr ) {
			return;
		}
		auto pComponent = pInstrument->getComponent( 0 );
		if ( pComponent == nullptr || pComponent->getLayer( 0 ) == nullptr ) {
			return;
		}
		HydrogenApp::get_instance()->showSampleEditor(
			pComponent->getLayer( 0 ), pComponent, pInstrument
		);
	} );

	// mute playback track toggle button
	m_pMutePlaybackTrackButton = new MuteButton(
		pPlaybackTrackToolBar, QSize( 30, 26 ), tr( "Mute playback track" ),
		true
	);
	m_pMutePlaybackTrackButton->setObjectName(
		"SongEditorPlaybackTrackMuteButton"
	);
	connect(
		m_pMutePlaybackTrackButton, &QPushButton::clicked,
		[=]( bool bChecked ) {
			auto pSong = Hydrogen::get_instance()->getSong();
			if ( pSong == nullptr ) {
				return;
			}
			auto pInstrument = pSong->getPlaybackTrackInstrument();
			if ( pInstrument != nullptr ) {
				pInstrument->setMuted( bChecked );
				if ( pInstrument->getComponent( 0 ) != nullptr &&
					 pInstrument->getComponent( 0 )->getLayer( 0 ) !=
						 nullptr ) {
					pInstrument->getComponent( 0 )->getLayer( 0 )->setIsMuted(
						bChecked
					);
				}
				m_pPlaybackTrackWaveDisplay->updateBackground();
			}
		}
	);
	pPlaybackTrackToolBar->addWidget( m_pMutePlaybackTrackButton );

	if ( pPlaybackTrackInstrument == nullptr ) {
		m_pPlaybackTrackFader->setIsActive( false );
		m_pMutePlaybackTrackButton->setChecked( true );
		m_pMutePlaybackTrackButton->setIsActive( false );
	}
	else {
		m_pMutePlaybackTrackButton->setChecked(
			pPlaybackTrackInstrument->isMuted()
		);
	}

	m_pPlaybackTrackSidebar->setVisible( bShowPlaybackTrack );

	////////////////////////////////////////////////////////////////////////////

	auto pToolBarContainer = new QWidget( this );
	auto pToolBarContainerLayout = new QVBoxLayout();
	pToolBarContainerLayout->setAlignment( Qt::AlignBottom );
	pToolBarContainerLayout->setContentsMargins( 0, 0, 0, 0 );
    pToolBarContainer->setFixedWidth( SongEditorPatternList::nWidth );
    pToolBarContainerLayout->setSpacing( 0 );
	pToolBarContainer->setLayout( pToolBarContainerLayout );
    pToolBarContainer->setStyleSheet( "border-right: 1px solid #000" );

	m_pTimelineToolBarContainer = new QWidget( pToolBarContainer );
    m_pTimelineToolBarContainer->setObjectName( "TimelineToolBarContainer" );
    pToolBarContainerLayout->addWidget( m_pTimelineToolBarContainer );
	auto pTimelineToolBarContainerLayout = new QHBoxLayout();
	pTimelineToolBarContainerLayout->setContentsMargins( 1, 1, 1, 1 );
    pTimelineToolBarContainerLayout->setSpacing( 0 );
	m_pTimelineToolBarContainer->setLayout( pTimelineToolBarContainerLayout );

    pTimelineToolBarContainerLayout->addStretch();

    auto pTimelineToolBar = new QToolBar( m_pTimelineToolBarContainer );
	pTimelineToolBarContainerLayout->addWidget( pTimelineToolBar );
	pTimelineToolBar->setFixedHeight(
		SongEditorPanel::nHeaderWidgetHeight / 2 - 2
	);
	pTimelineToolBar->setFocusPolicy( Qt::ClickFocus );

	m_pEnableTimelineAction = createAction( pTimelineToolBar,
		pCommonStrings->getTimelineEnabled(), false
	);
	m_pEnableTimelineAction->setObjectName( "TimelineBtn" );
	m_pEnableTimelineAction->setChecked( pSong->getIsTimelineActivated() );
	connect( m_pEnableTimelineAction, &QAction::triggered, [=]() {
		auto pSong = Hydrogen::get_instance()->getSong();
		if ( pSong == nullptr ) {
			return;
		}
		CoreActionController::activateTimeline( !pSong->getIsTimelineActivated()
		);
		updateTimeline();

		const QString sMessage =
			QString( "%1 = %2" )
				.arg( pCommonStrings->getTimelineBigButton() )
				.arg(
					m_pEnableTimelineAction->isChecked()
						? pCommonStrings->getStatusOn()
						: pCommonStrings->getStatusOff()
				);
		HydrogenApp::get_instance()->showStatusBarMessage( sMessage );
		Hydrogen::get_instance()->setIsModified( true );
	} );

	m_pTagAction = createAction( pTimelineToolBar, "", false );
	connect( m_pTagAction, &QAction::triggered, [=]() {
        m_pPositionRuler->showTagWidget( 0 );
    });
    m_pTempoMarkerAction = createAction( pTimelineToolBar, "", false );
	connect( m_pTempoMarkerAction, &QAction::triggered, [=]() {
        m_pPositionRuler->showBpmWidget( 0 );
    });

	m_pSongEditorToolBar = new QToolBar( pToolBarContainer );
	pToolBarContainerLayout->addWidget( m_pSongEditorToolBar );
	m_pSongEditorToolBar->setFixedSize(
		SongEditorPatternList::nWidth, SongEditorPanel::nHeaderWidgetHeight / 2
	);
	m_pSongEditorToolBar->setFocusPolicy( Qt::ClickFocus );

	m_pClearAction = createAction(
		m_pSongEditorToolBar, tr( "Clear pattern sequence" ), false
	);
	connect( m_pClearAction, &QAction::triggered, [=]() { clearSequence(); } );

	m_pSongEditorToolBar->addSeparator();

	m_pNewPatternAction =
		createAction( m_pSongEditorToolBar, tr( "Create new pattern" ), false );
	connect( m_pNewPatternAction, &QAction::triggered, [=]() {
		addNewPattern();
	} );

	m_pSongEditorToolBar->addSeparator();

	auto pPatternModeGroup = new QButtonGroup( m_pSongEditorToolBar );
	pPatternModeGroup->setExclusive( true );

	m_pSinglePatternModeButton = new MidiLearnableToolButton(
		m_pSongEditorToolBar, tr( "selected pattern mode" ) );
	m_pSinglePatternModeButton->setCheckable( true );
	connect( m_pSinglePatternModeButton, &QToolButton::clicked, [=]() {
		Hydrogen::get_instance()->setPatternMode( Song::PatternMode::Selected );
	});
	pPatternModeGroup->addButton( m_pSinglePatternModeButton );
	m_pSongEditorToolBar->addWidget( m_pSinglePatternModeButton );

	m_pStackedPatternModeButton = new MidiLearnableToolButton(
		m_pSongEditorToolBar, tr( "stacked pattern mode" ) );
	m_pStackedPatternModeButton->setCheckable( true );
	connect( m_pStackedPatternModeButton, &QToolButton::clicked, [=]() {
		Hydrogen::get_instance()->setPatternMode( Song::PatternMode::Stacked );
	});
	pPatternModeGroup->addButton( m_pStackedPatternModeButton );
	m_pSongEditorToolBar->addWidget( m_pStackedPatternModeButton );

	m_pSongEditorToolBar->addSeparator();

	m_pPatternEditorLockedButton = new MidiLearnableToolButton(
		m_pSongEditorToolBar, pCommonStrings->getPatternEditorLocked() );
	m_pPatternEditorLockedButton->setCheckable( true );
	m_pPatternEditorLockedButton->setObjectName( "PatternEditorLockedButton" );
	connect( m_pPatternEditorLockedButton, &QToolButton::clicked, [=](){
		auto pHydrogen = Hydrogen::get_instance();
		pHydrogen->setIsPatternEditorLocked(
			! pHydrogen->isPatternEditorLocked() );
	});
	m_pSongEditorToolBar->addWidget( m_pPatternEditorLockedButton );

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


	QHBoxLayout *pHZoomLayout = new QHBoxLayout();
	pHZoomLayout->setSpacing( 0 );
	pHZoomLayout->setContentsMargins( 0, 0, 0, 0 );
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
	m_pPositionRulerScrollView = new WidgetScrollArea( nullptr );
	m_pPositionRulerScrollView->setObjectName( "PositionRulerScrollView" );
	m_pPositionRulerScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pPositionRulerScrollView->setFrameShape( QFrame::NoFrame );
	m_pPositionRulerScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPositionRulerScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPositionRulerScrollView->setFocusPolicy( Qt::NoFocus );
	m_pPositionRuler = new SongEditorPositionRuler( m_pPositionRulerScrollView->viewport() );
	m_pPositionRuler->setObjectName( "SongEditorPositionRuler" );
	m_pPositionRulerScrollView->setWidget( m_pPositionRuler );
	m_pPositionRulerScrollView->setFixedHeight( SongEditorPanel::nHeaderWidgetHeight );
	connect( m_pPositionRulerScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( hScrollTo(int) ) );

	m_pPositionRuler->setFocusPolicy( Qt::ClickFocus );
	m_pPositionRuler->setFocusProxy( m_pSongEditor );

	m_pPlaybackTrackScrollView = new WidgetScrollArea( nullptr );
	m_pPlaybackTrackScrollView->setObjectName( "PlaybackTrackScrollView" );
	m_pPlaybackTrackScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pPlaybackTrackScrollView->setFrameShape( QFrame::NoFrame );
	m_pPlaybackTrackScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPlaybackTrackScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	m_pPlaybackTrackWaveDisplay = new PlaybackTrackWaveDisplay( m_pPlaybackTrackScrollView->viewport() );
	m_pPlaybackTrackWaveDisplay->setSampleNameAlignment( Qt::AlignLeft );
	m_pPlaybackTrackWaveDisplay->resize( m_pPositionRuler->width() , SongEditorPanel::nHeaderWidgetHeight);
	m_pPlaybackTrackWaveDisplay->setAcceptDrops( true );
	
	m_pPlaybackTrackScrollView->setWidget( m_pPlaybackTrackWaveDisplay );
	m_pPlaybackTrackScrollView->setFixedHeight( SongEditorPanel::nHeaderWidgetHeight );
	m_pPlaybackTrackScrollView->setVisible( bShowPlaybackTrack );
	
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

	QWidget *pMainPanel = new QWidget();
	pMainPanel->setObjectName( "SongEditorPanel" );
	QGridLayout *pGridLayout = new QGridLayout();
	pGridLayout->setSpacing( 0 );
	pGridLayout->setContentsMargins( 0, 0, 0, 0 );

	pGridLayout->addWidget( m_pPlaybackTrackSidebar, 0, 0 );
	pGridLayout->addWidget( pToolBarContainer, 1, 0 );
	pGridLayout->addWidget( m_pPlaybackTrackScrollView, 0, 1 );
	pGridLayout->addWidget( m_pPositionRulerScrollView, 1, 1 );
	pGridLayout->addWidget( m_pPatternListScrollView, 3, 0 );
	pGridLayout->addWidget( m_pEditorScrollView, 3, 1 );
	pGridLayout->addWidget( m_pVScrollBar, 3, 2, 2, 1 );
	pGridLayout->addWidget( m_pAutomationPathScrollView, 4, 1);
	pGridLayout->addWidget( m_pAutomationCombo, 4, 0,
							Qt::AlignVCenter | Qt::AlignRight );
	pGridLayout->addWidget( pHScrollbarPanel, 5, 1 );
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

        const int nWidth = m_pPositionRulerScrollView->viewport()->width();

		QPoint pos = m_pPositionRuler->pos();
		int x = -pos.x();

		int nPlayHeadPosition = pAudioEngine->getPlayhead()->getColumn() *
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
	m_pPatternEditorLockedButton->setStyleSheet( QString( "\
#PatternEditorLockedButton {\
    background-color: %1;\
}" ).arg( Preferences::get_instance()->getColorTheme()->m_buttonRedColor.name() ) );

	m_pHighlightLockedTimer->start( 250 );
}

void SongEditorPanel::updatePlaybackFaderPeaks()
{
    auto pSong = Hydrogen::get_instance()->getSong();
    if ( pSong == nullptr || pSong->getPlaybackTrackInstrument() == nullptr ) {
        return;
    }
	const auto pPref = Preferences::get_instance();
	auto pInstrument = pSong->getPlaybackTrackInstrument();

	
	bool bShowPeaks = pPref->showInstrumentPeaks();
	float fallOff = pPref->getInterfaceTheme()->m_fMixerFalloffSpeed;
	
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

void SongEditorPanel::updateAutomationPathVisibility() {
	if ( Preferences::get_instance()->getShowAutomationArea() ) {
		m_pAutomationPathScrollView->show();
		m_pAutomationCombo->show();
	}
	else {
		m_pAutomationPathScrollView->hide();
		m_pAutomationCombo->hide();
	}

	HydrogenApp::get_instance()->getMainForm()->updateAutomationPathVisibility();

	// Update visibility buttons.
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
}

void SongEditorPanel::updatePlaybackTrack()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( !Preferences::get_instance()->getShowPlaybackTrack() ) {
		return;
	}

	if ( pSong == nullptr || pSong->getPlaybackTrackInstrument() == nullptr ) {
		// No playback track chosen (stored in the current song).
		m_pPlaybackTrackFader->setIsActive( false );
		m_pEditPlaybackTrackAction->setEnabled( false );
		m_pMutePlaybackTrackButton->setIsActive( false );

		m_pPlaybackTrackWaveDisplay->setLayer( nullptr );
	}
	else {
		auto pInstrument = pSong->getPlaybackTrackInstrument();
		// Playback track was selected by the user and is ready to
		// use.
		m_pPlaybackTrackFader->setIsActive( true );
		m_pPlaybackTrackFader->setValue( pInstrument->getVolume() );
		m_pEditPlaybackTrackAction->setEnabled( true );
		m_pMutePlaybackTrackButton->setIsActive( true );
		m_pMutePlaybackTrackButton->setChecked( pInstrument->isMuted() );
		if ( pInstrument->getComponent( 0 ) != nullptr &&
			 pInstrument->getComponent( 0 )->getLayer( 0 ) != nullptr ) {
			m_pPlaybackTrackWaveDisplay->setLayer(
				pInstrument->getComponent( 0 )->getLayer( 0 )
			);
		}
	}
}

///
/// Create a new pattern
///
void SongEditorPanel::addNewPattern()
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
	auto pDialog = new PatternPropertiesDialog( nullptr, pNewPattern, 0, true );

	if ( pDialog->exec() == QDialog::Accepted ) {
		int nRow;
		if ( pHydrogen->getSelectedPatternNumber() == -1 ) {
			nRow = pPatternList->size();
		}
		else {
			nRow = pHydrogen->getSelectedPatternNumber() + 1;
		}
		HydrogenApp::get_instance()->pushUndoCommand(
			new SE_insertPatternAction(
				SE_insertPatternAction::Type::New, nRow,
				std::make_shared<Pattern>( pNewPattern ), nullptr
			)
		);
	}

	delete pDialog;
}

void SongEditorPanel::clearSequence()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	const auto pSongPatternList = pSong->getPatternList();

	if ( QMessageBox::information(
			 this, "Hydrogen",
			 tr( "Warning, this will erase your pattern sequence.\nAre you "
				 "sure?" ),
			 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
		 ) == QMessageBox::Cancel ) {
		return;
	}
	auto pHydrogenApp = HydrogenApp::get_instance();
	pHydrogenApp->beginUndoMacro( tr( "Delete complete pattern-sequence" ) );

	const auto pPatternGroupVector = pSong->getPatternGroupVector();
	for ( int nnColumn = 0; nnColumn < pPatternGroupVector->size();
		  ++nnColumn ) {
		const auto ppPatternList = pPatternGroupVector->at( nnColumn );
		if ( ppPatternList == nullptr ) {
			continue;
		}
		std::vector<int> indices;
		for ( const auto& ppPattern : *ppPatternList ) {
			if ( ppPattern == nullptr ) {
				continue;
			}
			const int nIndex = pSongPatternList->index( ppPattern );
			if ( nIndex != -1 ) {
				indices.push_back( nIndex );
			}
		}

		for ( int ii = indices.size() - 1; ii >= 0; --ii ) {
			pHydrogenApp->pushUndoCommand( new SE_addOrRemovePatternCellAction(
				GridPoint( nnColumn, indices[ii] ), Editor::Action::Delete,
				Editor::ActionModifier::None
			) );
		}
	}

	pHydrogenApp->endUndoMacro();
}

bool SongEditorPanel::hasSongEditorFocus() const{
	return hasFocus() ||
		m_pEditorScrollView->hasFocus() ||
		m_pPatternListScrollView->hasFocus() ||
		m_pPositionRulerScrollView->hasFocus() ||
		m_pPlaybackTrackScrollView->hasFocus() ||
		m_pVScrollBar->hasFocus() ||
		m_pHScrollBar->hasFocus() ||
		m_pSongEditor->hasFocus() ||
		m_pPatternList->hasFocus() ||
		m_pPositionRuler->hasFocus() ||
		m_pPlaybackTrackWaveDisplay->hasFocus();
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
	updatePlaybackTrack();
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
	updatePlaybackTrack();

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

void SongEditorPanel::updatePreferencesEvent( int nValue ) {
	if ( nValue == 1 ) {
		// new preferences loaded within the core
		const auto pPref = H2Core::Preferences::get_instance();
		auto pSongEditorPanel = HydrogenApp::get_instance()->getSongEditorPanel();
		if ( pPref->getShowAutomationArea() !=
			 pSongEditorPanel->getAutomationPathView()->isVisible() ) {
			pSongEditorPanel->updateAutomationPathVisibility();
		}

		const int nNewGridWidth = pPref->getSongEditorGridWidth();
		if ( m_pSongEditor->getGridWidth() != nNewGridWidth ) {
			m_pSongEditor->setGridWidth( nNewGridWidth );
			m_pPositionRuler->setGridWidth( nNewGridWidth );
			m_pAutomationPathView->setGridWidth( nNewGridWidth );
		}
		// MaxBars might have change.
		m_pSongEditor->updateWidth();
		updateEditors( Editor::Update::Background );

		updatePlaybackTrack();
	}
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
	updatePlaybackTrack();
	updateTimeline();
	updateStyleSheet();

	m_pPositionRuler->updatePosition();
	m_pPositionRuler->updateSongSize();
	updateEditors( Editor::Update::Background );
}

void SongEditorPanel::showPlaybackTrack( bool bVisible )
{
	m_pPlaybackTrackSidebar->setVisible( bVisible );
	m_pPlaybackTrackScrollView->setVisible( bVisible );

	Preferences::get_instance()->setShowPlaybackTrack( bVisible );

	updatePlaybackTrack();

	// Update visibility buttons.
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
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

	if ( pSong == nullptr || pSong->getPlaybackTrackInstrument() == nullptr ) {
		return;
	}

	auto pInstrument = pSong->getPlaybackTrackInstrument();

	auto pFader = dynamic_cast<Fader*>( pRef );
	const float fNewValue = std::round( pFader->getValue() * 100 ) / 100;

	if ( pInstrument->getVolume() != fNewValue ) {
		pInstrument->setVolume( fNewValue );
		HydrogenApp::get_instance()->showStatusBarMessage(
			tr( "Playback volume set to" )
				.append( QString( " [%1]" ).arg( fNewValue ) ),
			"SongEditorPanel:PlaybackTrackVolume"
		);
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

void SongEditorPanel::toggleAutomationAreaVisibility() {
	auto pPref = Preferences::get_instance();
	
	if ( ! pPref->getShowAutomationArea() )	{
		pPref->setShowAutomationArea( true );
	}
	else {
		pPref->setShowAutomationArea( false );
	}

	updateAutomationPathVisibility();
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

        updateIcons();
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
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
		color = Qt::white;
	} else {
		sIconPath.append( "/icons/black/" );
		color = Qt::black;
	}

	m_pLoadPlaybackTrackAction->setIcon( QIcon( sIconPath + "folder.svg" ) );
	m_pEditPlaybackTrackAction->setIcon(
		QIcon( sIconPath + "sample-editor.svg" )
	);

	auto pSong = Hydrogen::get_instance()->getSong();
	const bool bTimelineEnabled =
		pSong != nullptr ? pSong->getIsTimelineActivated() : false;

	if ( bTimelineEnabled ) {
		m_pEnableTimelineAction->setIcon( QIcon( sIconPath + "enabled.svg" ) );
	}
	else {
		m_pEnableTimelineAction->setIcon( QIcon( sIconPath + "disabled.svg" ) );
	}
	m_pTagAction->setIcon( QIcon( sIconPath + "tag.svg" ) );
	m_pTempoMarkerAction->setIcon( QIcon( sIconPath + "metronome.svg" ) );

	m_pClearAction->setIcon( QIcon( sIconPath + "bin.svg" ) );
	m_pNewPatternAction->setIcon( QIcon( sIconPath + "new.svg" ) );
	m_pSinglePatternModeButton->setIcon( QIcon( sIconPath + "single_layer.svg" )
	);
	m_pStackedPatternModeButton->setIcon(
		QIcon( sIconPath + "multiple_layers.svg" )
	);

	updatePatternEditorLocked();
}

void SongEditorPanel::updateJacktimebaseState() {
	updateTimeline();
}

void SongEditorPanel::updatePatternEditorLocked() {
	QColor color;
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
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
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();
	const QColor colorSongEditorToolBar =
		pColorTheme->m_songEditor_backgroundColor;
	QColor colorToolBarText;
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		colorToolBarText = Qt::white;
	}
	else {
		colorToolBarText = Qt::black;
	}
	const QColor colorPlaybackTrackToolBar =
		pColorTheme->m_accentColor.darker( 85 );
	const QColor colorTimelineToolBar =
		pColorTheme->m_songEditor_alternateRowColor.darker(
			SongEditorPositionRuler::nScalingTimeline
		);

	QColor backgroundInactiveColor;
	if ( Hydrogen::get_instance()->getMode() == Song::Mode::Song ) {
		backgroundInactiveColor = pColorTheme->m_windowColor.lighter(
		 	Skin::nEditorActiveScaling );
	}
	else {
		backgroundInactiveColor = pColorTheme->m_windowColor;
	}

	setStyleSheet( QString( "\
#PatternListScrollView, #EditorScrollView, #PositionRulerScrollView,\
#PlaybackTrackScrollView, #AutomationPathScrollView, #SongEditorPanel {\
     background-color: %1;\
}" )
				   .arg( backgroundInactiveColor.name() ) );

	auto sToolBarStyle = QString(
							 "\
QToolBar {\
     background-color: %1; \
     color: %2; \
     border: 1px solid #000;\
     spacing: 2px;\
}"
	)
							 .arg( colorSongEditorToolBar.name() )
							 .arg( colorToolBarText.name() );
	m_pSongEditorToolBar->setStyleSheet( sToolBarStyle );

	m_pPlaybackTrackSidebar->setStyleSheet( QString( "\
QWidget {\
     background-color: %1;                      \
}                                               \
QWidget#TimelineToolBarContainer {\
     border: 1px solid #000; \
}                                               \
QToolBar {\
     color: %2; \
     spacing: 2px;\
}" )
													.arg( colorPlaybackTrackToolBar.name() )
													.arg( colorToolBarText.name(
													) ) );

	m_pTimelineToolBarContainer->setStyleSheet( QString( "\
QWidget {\
     background-color: %1;                      \
}                                               \
QWidget#TimelineToolBarContainer {\
     border: 1px solid #000; \
}                                               \
QToolBar {\
     color: %2; \
     spacing: 2px;\
}" )
													.arg( colorTimelineToolBar.name() )
													.arg( colorToolBarText.name(
													) ) );

	m_pMutePlaybackTrackButton->setDefaultBackgroundColor(
		colorPlaybackTrackToolBar.lighter( 125 )
	);
	m_pMutePlaybackTrackButton->setCheckedBackgroundTextColor(
		pColorTheme->m_muteTextColor
	);
}

void SongEditorPanel::updateTimeline() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto tempoSource = pHydrogen->getTempoSource();

	bool bEnabled, bTempoEnabled;
	if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
		bEnabled = false;
		bTempoEnabled = false;
		m_pEnableTimelineAction->setToolTip(
			pCommonStrings->getTimelineDisabledPatternMode()
		);
	}
	else if ( tempoSource == Hydrogen::Tempo::Midi ) {
		bEnabled = true;
		bTempoEnabled = false;
		m_pEnableTimelineAction->setToolTip(
			pCommonStrings->getTimelineDisabledMidiClock()
		);
	}
	else if ( tempoSource == Hydrogen::Tempo::Jack ) {
		bEnabled = true;
		bTempoEnabled = false;
		m_pEnableTimelineAction->setToolTip(
			pCommonStrings->getTimelineDisabledTimebaseListener()
		);
	}
	else {
		bEnabled = true;
		bTempoEnabled = pSong->getIsTimelineActivated();
		m_pEnableTimelineAction->setToolTip( pCommonStrings->getTimelineEnabled(
		) );
	}

	m_pEnableTimelineAction->setEnabled( bEnabled );
	m_pTagAction->setEnabled( bEnabled );
	m_pTempoMarkerAction->setEnabled( bTempoEnabled );

	updateIcons();
}
