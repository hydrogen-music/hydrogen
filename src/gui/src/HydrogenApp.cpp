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

#include <core/config.h>
#include <core/Version.h>
#include <core/Hydrogen.h>
#include <core/EventQueue.h>
#include <core/FX/LadspaFX.h>
#include <core/Preferences/Preferences.h>
#include <core/Helpers/Filesystem.h>

#include "HydrogenApp.h"
#include "CommonStrings.h"
#include "PreferencesDialog/PreferencesDialog.h"
#include "MainForm.h"
#include "PlayerControl.h"
#include "AudioEngineInfoForm.h"
#include "FilesystemInfoForm.h"
#include "LadspaFXProperties.h"
#include "InstrumentRack.h"
#include "Director.h"

#include "PatternEditor/PatternEditorPanel.h"
#include "PatternEditor/PatternEditorRuler.h"
#include "PatternEditor/NotePropertiesRuler.h"
#include "PatternEditor/PianoRollEditor.h"
#include "PatternEditor/DrumPatternEditor.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "PlaylistEditor/PlaylistDialog.h"
#include "SampleEditor/SampleEditor.h"
#include "Mixer/Mixer.h"
#include "Mixer/MixerLine.h"
#include "UndoActions.h"

#include <core/Basics/PatternList.h>
#include <core/Basics/InstrumentList.h>

#include "Widgets/AutomationPathView.h"
#include "Widgets/InfoBar.h"

#include <QtGui>
#include <QtWidgets>


using namespace H2Core;


HydrogenApp* HydrogenApp::m_pInstance = nullptr;

HydrogenApp::HydrogenApp( MainForm *pMainForm )
 : m_pMainForm( pMainForm )
 , m_pMixer( nullptr )
 , m_pPatternEditorPanel( nullptr )
 , m_pAudioEngineInfoForm( nullptr )
 , m_pSongEditorPanel( nullptr )
 , m_pPlayerControl( nullptr )
 , m_pPlaylistDialog( nullptr )
 , m_pSampleEditor( nullptr )
 , m_pDirector( nullptr )
 , m_nPreferencesUpdateTimeout( 100 )
 , m_bufferedChanges( H2Core::Preferences::Changes::None )
 , m_pMainScrollArea( new QScrollArea )
{
	m_pInstance = this;

	m_pEventQueueTimer = new QTimer(this);
	connect( m_pEventQueueTimer, SIGNAL( timeout() ), this, SLOT( onEventQueueTimer() ) );
	m_pEventQueueTimer->start( QUEUE_TIMER_PERIOD );

	// Wait for m_nPreferenceUpdateTimeout milliseconds of no update
	// signal before propagating the update. Else importing/resetting a
	// theme will slow down the GUI significantly.
	m_pPreferencesUpdateTimer = new QTimer( this );
	m_pPreferencesUpdateTimer->setSingleShot( true );
	connect( m_pPreferencesUpdateTimer, SIGNAL(timeout()), this, SLOT(propagatePreferences()) );

	m_pCommonStrings = std::make_shared<CommonStrings>();

	//setup the undo stack
	m_pUndoStack = new QUndoStack( this );

	updateWindowTitle();

	Preferences *pPref = Preferences::get_instance();

	setupSinglePanedInterface();

	// restore audio engine form properties
	m_pAudioEngineInfoForm = new AudioEngineInfoForm( nullptr );
	WindowProperties audioEngineInfoProp = pPref->getAudioEngineInfoProperties();
	setWindowProperties( m_pAudioEngineInfoForm, audioEngineInfoProp, SetX + SetY );
	
	m_pFilesystemInfoForm = new FilesystemInfoForm( nullptr );

	m_pPlaylistDialog = new PlaylistDialog( nullptr );
	WindowProperties playlistDialogProp = pPref->getPlaylistDialogProperties();
	setWindowProperties( m_pPlaylistDialog, playlistDialogProp, SetAll );

	m_pDirector = new Director( nullptr );
	WindowProperties directorProp = pPref->getDirectorProperties();
	setWindowProperties( m_pDirector, directorProp, SetAll );

	// Initially keyboard cursor is hidden.
	m_bHideKeyboardCursor = true;
	
	// Since HydrogenApp does implement some handler functions for
	// Events as well, it should be registered as an Eventlistener
	// itself.
	addEventListener( this );

	connect( this, &HydrogenApp::preferencesChanged,
			 m_pMainForm, &MainForm::onPreferencesChanged );
	connect( this, &HydrogenApp::preferencesChanged,
			 this, &HydrogenApp::onPreferencesChanged );
}

void HydrogenApp::setWindowProperties( QWidget *pWindow, WindowProperties &prop, unsigned flags ) {
	if ( flags & SetVisible ) {
		if ( prop.visible) {
			pWindow->show();
		} else {
			pWindow->hide();
		}
	}

	// Preserve the current values of anything we're not setting.
	QRect oldGeometry = pWindow->geometry();
	if ( prop.m_geometry.size() == 0 ) {
		// No geometry saved in preferences. Most likely an old preferences file. Set the size and shape
		pWindow->resize( prop.width, prop.height );
		pWindow->move( prop.x, prop.y );
		prop.m_geometry = pWindow->saveGeometry();
	}

	// restore geometry will also ensure things are visible if screen geometry has changed.
	pWindow->restoreGeometry( prop.m_geometry );

	// For anything that we're not setting, restore the previous values.
	QRect newGeometry = pWindow->geometry();
	if ( !( flags & SetX ) ) {
		newGeometry.setX( oldGeometry.x() );
	}
	if ( !( flags & SetY ) ) {
		newGeometry.setY( oldGeometry.y() );
	}
	if ( !( flags & SetWidth ) ) {
		newGeometry.setWidth( oldGeometry.width() );
	}
	if ( !( flags & SetHeight ) ) {
		newGeometry.setHeight( oldGeometry.height() );
	}

	// If a window is fixed-size, don't restore it full-screen (macOS sometimes does this, annoyingly)
	if ( pWindow->minimumSize() == pWindow->maximumSize() ) {
		if ( pWindow->isFullScreen()) {
			pWindow->showNormal();
		}
	}

	if ( oldGeometry != newGeometry ) {
		pWindow->setGeometry( newGeometry );
	}
}


WindowProperties HydrogenApp::getWindowProperties( QWidget *pWindow ) {
	WindowProperties prop;
	prop.x = pWindow->x();
	prop.y = pWindow->y();
	prop.height = pWindow->height();
	prop.width = pWindow->width();
	prop.visible = pWindow->isVisible();
	prop.m_geometry = pWindow->saveGeometry();

	return prop;
}


HydrogenApp::~HydrogenApp()
{
	INFOLOG( "[~HydrogenApp]" );
	m_pEventQueueTimer->stop();


	//delete the undo tmp directory
	cleanupTemporaryFiles();

	delete m_pAudioEngineInfoForm;
	delete m_pFilesystemInfoForm;
	delete m_pMixer;
	delete m_pPlaylistDialog;
	delete m_pDirector;
	delete m_pSampleEditor;

	if ( m_pTab ) {
		delete m_pTab;
	}
	if ( m_pSplitter ) {
		delete m_pSplitter;
	}

	#ifdef H2CORE_HAVE_LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		delete m_pLadspaFXProperties[nFX];
	}
	#endif

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	if (pHydrogen) {
		// Hydrogen calls removeSong on from its destructor, so here we just delete the objects:
		delete pHydrogen;
	}

	m_pInstance = nullptr;

}



void HydrogenApp::setupSinglePanedInterface()
{
	Preferences *pPref = Preferences::get_instance();
	InterfaceTheme::Layout layout = pPref->getDefaultUILayout();

	// MAINFORM
	WindowProperties mainFormProp = pPref->getMainFormProperties();
	setWindowProperties( m_pMainForm, mainFormProp, SetDefault & ~SetVisible );

	m_pSplitter = new QSplitter( m_pMainScrollArea );
	m_pSplitter->setOrientation( Qt::Vertical );
	m_pSplitter->setOpaqueResize( true );

	m_pTab = new QTabWidget( m_pMainScrollArea );
	m_pTab->setObjectName( "TabbedInterface" );

	// SONG EDITOR
	if( layout == InterfaceTheme::Layout::SinglePane ) {
		m_pSongEditorPanel = new SongEditorPanel( m_pSplitter );
	} else {
		m_pSongEditorPanel = new SongEditorPanel( m_pTab );
	}
	// trigger a relocation to sync the transport position of the
	// editors in the panel.
	H2Core::Hydrogen::get_instance()->getCoreActionController()->locateToColumn( 0 );

	WindowProperties songEditorProp = pPref->getSongEditorProperties();
	setWindowProperties( m_pSongEditorPanel, songEditorProp, SetWidth + SetHeight );

	if( layout == InterfaceTheme::Layout::Tabbed ) {
		m_pTab->addTab( m_pSongEditorPanel, tr("Song Editor") );
	}

	// this HBox will contain the InstrumentRack and the Pattern editor
	QWidget *pSouthPanel = new QWidget( m_pSplitter );
	pSouthPanel->setObjectName( "SouthPanel" );
	QHBoxLayout *pEditorHBox = new QHBoxLayout();
	pEditorHBox->setSpacing( 5 );
	pEditorHBox->setMargin( 0 );
	pSouthPanel->setLayout( pEditorHBox );

	// INSTRUMENT RACK
	m_pInstrumentRack = new InstrumentRack( nullptr );
	WindowProperties instrumentRackProp = pPref->getInstrumentRackProperties();
	m_pInstrumentRack->setHidden( !instrumentRackProp.visible );

	if( layout == InterfaceTheme::Layout::Tabbed ){
		m_pTab->setMovable( false );
		m_pTab->setTabsClosable( false );
		m_pTab->addTab( pSouthPanel, tr( "Instrument + Pattern") );
	}

	// PATTERN EDITOR
	m_pPatternEditorPanel = new PatternEditorPanel( nullptr );
	// Sync the playhead position in all editors all objects are available.
	m_pPatternEditorPanel->getPatternEditorRuler()->updatePosition( true );
	WindowProperties patternEditorProp = pPref->getPatternEditorProperties();
	setWindowProperties( m_pPatternEditorPanel, patternEditorProp, SetWidth + SetHeight );

	pEditorHBox->addWidget( m_pPatternEditorPanel );
	pEditorHBox->addWidget( m_pInstrumentRack );

	// PLayer control
	m_pPlayerControl = new PlayerControl( nullptr );

	QWidget *mainArea = new QWidget( m_pMainForm );	// this is the main widget
	m_pMainForm->setCentralWidget( mainArea );

	// LAYOUT!!
	m_pMainVBox = new QVBoxLayout();
	m_pMainVBox->setSpacing( 1 );
	m_pMainVBox->setMargin( 0 );
	m_pMainVBox->addWidget( m_pPlayerControl );

	m_pMainVBox->addSpacing( 3 );

	if( layout == InterfaceTheme::Layout::SinglePane ) {
		m_pMainVBox->addWidget( m_pSplitter );
	} else {
		m_pMainVBox->addWidget( m_pTab );
	}

	mainArea->setLayout( m_pMainVBox );

	mainArea->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	mainArea->setMinimumSize( 1000,
							  180 + // menu bar, margins etc.
							  PlayerControl::m_nMinimumHeight +
							  SongEditorPanel::m_nMinimumHeight +
							  InstrumentRack::m_nMinimumHeight +
							  SongEditorPositionRuler::m_nMinimumHeight +
							  SongEditor::m_nMinimumHeight +
							  AutomationPathView::m_nMinimumHeight );

	m_pMainScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_pMainScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_pMainScrollArea->setWidget( mainArea );
	m_pMainScrollArea->setWidgetResizable( true );

	m_pMainForm->setCentralWidget( m_pMainScrollArea );

	// MIXER
	m_pMixer = new Mixer(nullptr);
	m_pMixer->setObjectName( "Mixer" );
	WindowProperties mixerProp = pPref->getMixerProperties();

	if ( layout != InterfaceTheme::Layout::SinglePane ) {
		mixerProp.visible = false;
	}
	setWindowProperties( m_pMixer, mixerProp );

	if( layout == InterfaceTheme::Layout::Tabbed ){
		m_pTab->addTab(m_pMixer,tr("Mixer"));
	}

	m_pMixer->updateMixer();

#ifdef H2CORE_HAVE_LADSPA
	// LADSPA FX
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX] = new LadspaFXProperties( nullptr, nFX );
		m_pLadspaFXProperties[nFX]->hide();
		WindowProperties prop = pPref->getLadspaProperties(nFX);
		setWindowProperties( m_pLadspaFXProperties[ nFX ], prop, SetWidth + SetHeight );
	}
#endif

	if( layout == InterfaceTheme::Layout::Tabbed ){
		m_pTab->setCurrentIndex( Preferences::get_instance()->getLastOpenTab() );
		QObject::connect(m_pTab, SIGNAL(currentChanged(int)),this,SLOT(currentTabChanged(int)));
	}
}


InfoBar *HydrogenApp::addInfoBar() {
	InfoBar *pInfoBar = new InfoBar();
	m_pMainVBox->insertWidget( 1, pInfoBar );
	return pInfoBar;
}



void HydrogenApp::currentTabChanged(int index)
{
	Preferences::get_instance()->setLastOpenTab( index );
}

void HydrogenApp::closeFXProperties()
{
#ifdef H2CORE_HAVE_LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX]->close();
	}
#endif
}

bool HydrogenApp::openSong( QString sFilename ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();

	// Check whether there is an autosave file next to it
	// containing newer content.
	QFileInfo fileInfo( sFilename );

	// Ensure the path to the file is not relative.
	if ( fileInfo.isRelative() ) {
		sFilename = fileInfo.absoluteFilePath();
	}

	// In case the user did open a hidden file, the baseName()
	// will be an empty string.
	QString sBaseName( fileInfo.completeBaseName() );
	if ( sBaseName.startsWith( "." ) ) {
		sBaseName.remove( 0, 1 );
	}
	
	// Hidden autosave file (recent version)
	QFileInfo autoSaveFileRecent( QString( "%1/.%2.autosave.h2song" )
								  .arg( fileInfo.absoluteDir().absolutePath() )
								  .arg( sBaseName ) );
	// Visible autosave file (older version)
	QFileInfo autoSaveFileOld( QString( "%1/%2.autosave.h2song" )
							   .arg( fileInfo.absoluteDir().absolutePath() )
							   .arg( sBaseName ) );
	QString sRecoverFilename = "";
	if ( autoSaveFileRecent.exists() &&
		 autoSaveFileRecent.lastModified() >
		 fileInfo.lastModified() ) {
		sRecoverFilename = autoSaveFileRecent.absoluteFilePath();
	} else if ( autoSaveFileOld.exists() &&
				autoSaveFileOld.lastModified() >
				fileInfo.lastModified()  ) {
		sRecoverFilename = autoSaveFileOld.absoluteFilePath();
	}

	if ( ! sRecoverFilename.isEmpty() ) {
		QMessageBox msgBox;
		// Not commonized in CommmonStrings as it is required before
		// HydrogenApp was instantiated.
		msgBox.setText( tr( "There are unsaved changes." ) );
		msgBox.setInformativeText( tr( "Do you want to recover them?" ) );
		msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Discard );
		msgBox.setDefaultButton( QMessageBox::Discard );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Question );
		int nRet = msgBox.exec();

		if ( nRet == QMessageBox::Discard ) {
			sRecoverFilename = "";
		}
	}
	
	if ( ! pCoreActionController->openSong( sFilename, sRecoverFilename ) ) {
		QMessageBox msgBox;
		// Not commonized in CommmonStrings as it is required before
		// HydrogenApp was instantiated.
		msgBox.setText( tr( "Error loading song." ) );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Warning );
		msgBox.exec();
		return false;
	}

	return true;
}

bool HydrogenApp::openSong( std::shared_ptr<Song> pSong ) {

	auto pCoreActionController = Hydrogen::get_instance()->getCoreActionController();
	if ( ! pCoreActionController->openSong( pSong ) ) {
		QMessageBox msgBox;
		// Not commonized in CommmonStrings as it is required before
		// HydrogenApp was instantiated.
		msgBox.setText( tr( "Error loading song." ) );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Warning );
		msgBox.exec();
		return false;
	}

	return true;
}

bool HydrogenApp::recoverEmptySong() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();

	// Check whether there is an autosave file next to it
	// containing newer content.
	QString sFilename( H2Core::Filesystem::empty_song_path() );
	QFileInfo fileInfo( sFilename );

	// In case the user did open a hidden file, the baseName()
	// will be an empty string.
	QString sBaseName( fileInfo.completeBaseName() );
	if ( sBaseName.startsWith( "." ) ) {
		sBaseName.remove( 0, 1 );
	}
	
	QFileInfo autoSaveFile( QString( "%1/.%2.autosave.h2song" )
								  .arg( fileInfo.absoluteDir().absolutePath() )
								  .arg( sBaseName ) );
	QString sRecoverFilename = "";

	// Since there is no original file we can not check whether these
	// changes have been done "recently". It's up to the calling
	// function to ensure the corresponding empty song was indeed the
	// last one opened by the user.
	if ( autoSaveFile.exists() ) {
		sRecoverFilename = autoSaveFile.absoluteFilePath();
	}

	if ( ! sRecoverFilename.isEmpty() ) {
		QMessageBox msgBox;
		// Not commonized in CommmonStrings as it is required before
		// HydrogenApp was instantiated.
		msgBox.setText( tr( "There are unsaved changes." ) );
		msgBox.setInformativeText( tr( "Do you want to recover them?" ) );
		msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Discard );
		msgBox.setDefaultButton( QMessageBox::Discard );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Question );
		int nRet = msgBox.exec();

		if ( nRet == QMessageBox::Discard ) {
			sRecoverFilename = "";
		}
	}

	if ( sRecoverFilename.isEmpty() ) {
		return false;
	}
	
	if ( ! pCoreActionController->openSong( sFilename, sRecoverFilename ) ) {
		QMessageBox msgBox;
		// Not commonized in CommmonStrings as it is required before
		// HydrogenApp was instantiated.
		msgBox.setText( tr( "Error loading song." ) );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Warning );
		msgBox.exec();
		return false;
	}

	// The song has not been properly saved yet. Also this prevents
	// the autosave file we just loaded from being removed in case the
	// user decides to quit and reopen Hydrogen right after this call
	// without introducing any changes.
	pHydrogen->setIsModified( true );

	return true;
}

void HydrogenApp::showMixer(bool show)
{
	/*
		 *   Switch to Mixer tab with alt+m in tabbed mode,
		 *   otherwise open mixer window
		 */

	InterfaceTheme::Layout layout = Preferences::get_instance()->getDefaultUILayout();

	if ( layout == InterfaceTheme::Layout::Tabbed ) {
		m_pTab->setCurrentIndex( 2 );
	} else {
		m_pMixer->setVisible( show );
	}

	m_pMainForm->update_mixer_checkbox();
}

void HydrogenApp::showInstrumentPanel(bool show)
{
	/*
		 *   Switch to pattern editor/instrument tab in tabbed mode,
		 *   otherwise hide instrument panel
		 */

	InterfaceTheme::Layout layout = Preferences::get_instance()->getDefaultUILayout();

	if ( layout == InterfaceTheme::Layout::Tabbed ) {
		m_pTab->setCurrentIndex( 1 );
		getInstrumentRack()->setHidden( show );
	} else {
		getInstrumentRack()->setHidden( show );
	}
	m_pMainForm->update_instrument_checkbox( !show );
}



void HydrogenApp::showPreferencesDialog()
{
	PreferencesDialog preferencesDialog(m_pMainForm);
	preferencesDialog.exec();
}




void HydrogenApp::showStatusBarMessage( const QString& sMessage, const QString& sCaller )
{
	if ( m_pPlayerControl != nullptr ) {
		m_pPlayerControl->showStatusBarMessage( sMessage, sCaller );
	}
}

void HydrogenApp::XRunEvent() {
	const auto pAudioDriver = Hydrogen::get_instance()->getAudioOutput();
	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return;
	}
	showStatusBarMessage( QString( "XRUNS [%1]!!!" ) .arg( pAudioDriver->getXRuns() ) );
}

void HydrogenApp::updateWindowTitle()
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "invalid song" );
		assert(pSong);
		return;
	}

	QString sTitle = Filesystem::untitled_song_name();

	QString sSongName( pSong->getName() );
	QString sFilePath( pSong->getFilename() );

	if ( sFilePath == Filesystem::empty_song_path() ||
		 sFilePath.isEmpty() ) {
		// An empty song is _not_ associated with a file. Therefore,
		// we mustn't show the file name.
		if ( ! sSongName.isEmpty() ) {
			sTitle = sSongName;
		}
	} else {
		QFileInfo fileInfo( sFilePath );

		if ( sSongName == Filesystem::untitled_song_name() ||
			 sSongName == fileInfo.completeBaseName() ) {
			// The user did not alter the default name of the song or
			// set the song name but also named the corresponding file
			// accordingly. We'll just show the file name to avoid
			// duplication.
			sTitle = fileInfo.fileName();

		} else {
			// The user did set the song name but used a different
			// name for the corresponding file. We'll show both to
			// make this mismatch transparent.
			sTitle = QString( "%1 [%2]" ).arg( sSongName )
				.arg( fileInfo.fileName() );
		}
	}

	if( pSong->getIsModified() ){
		sTitle.append( " (" + tr( "modified" ) + ")" );
	}

	m_pMainForm->setWindowTitle( ( "Hydrogen " + QString( get_version().c_str()) +
								   QString( " - " ) + sTitle ) );
}


void HydrogenApp::showAudioEngineInfoForm()
{
	m_pAudioEngineInfoForm->hide();
	m_pAudioEngineInfoForm->show();
}

void HydrogenApp::showFilesystemInfoForm()
{
	m_pFilesystemInfoForm->hide();
	m_pFilesystemInfoForm->show();
}

void HydrogenApp::showPlaylistDialog()
{
	if ( m_pPlaylistDialog->isVisible() ) {
		m_pPlaylistDialog->hide();
	} else {
		m_pPlaylistDialog->show();
	}
	m_pMainForm->update_playlist_checkbox();
}


void HydrogenApp::showDirector()
{
	if ( m_pDirector->isVisible() ) {
		m_pDirector->hide();
	} else {
		m_pDirector->show();
	}
	m_pMainForm->update_director_checkbox();
}


void HydrogenApp::showSampleEditor( QString name, int mSelectedComponemt, int mSelectedLayer )
{

	if ( m_pSampleEditor ){
		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_pSampleEditor->close();
		delete m_pSampleEditor;
		m_pSampleEditor = nullptr;
		QApplication::restoreOverrideCursor();
	}
	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_pSampleEditor = new SampleEditor( nullptr, mSelectedComponemt, mSelectedLayer, name );
	m_pSampleEditor->show();
	QApplication::restoreOverrideCursor();
}

void HydrogenApp::drumkitLoadedEvent(){
	showStatusBarMessage( QString( tr( "Drumkit [%1] loaded from [%2]" )
								  .arg( Hydrogen::get_instance()->getLastLoadedDrumkitName() )
								  .arg( Hydrogen::get_instance()->getLastLoadedDrumkitPath() ) ) );
}

void HydrogenApp::songModifiedEvent()
{
	updateWindowTitle();
}

void HydrogenApp::onEventQueueTimer()
{
	// use the timer to do schedule instrument slaughter;
	EventQueue *pQueue = EventQueue::get_instance();

	Event event;
	while ( ( event = pQueue->pop_event() ).type != EVENT_NONE ) {
		
		// Provide the event to all EventListeners registered to
		// HydrogenApp. By registering itself as EventListener and
		// implementing at least on the methods used below a
		// particular GUI component can react on specific events.
		for (int i = 0; i < (int)m_EventListeners.size(); i++ ) {
			EventListener *pListener = m_EventListeners[ i ];

			switch ( event.type ) {
			case EVENT_STATE:
				pListener->stateChangedEvent( static_cast<H2Core::AudioEngine::State>(event.value) );
				break;

			case EVENT_PLAYING_PATTERNS_CHANGED:
				pListener->playingPatternsChangedEvent();
				break;
				
			case EVENT_NEXT_PATTERNS_CHANGED:
				pListener->nextPatternsChangedEvent();
				break;

			case EVENT_PATTERN_MODIFIED:
				pListener->patternModifiedEvent();
				break;

			case EVENT_SONG_MODIFIED:
				pListener->songModifiedEvent();
				break;

			case EVENT_SELECTED_PATTERN_CHANGED:
				pListener->selectedPatternChangedEvent();
				break;

			case EVENT_SELECTED_INSTRUMENT_CHANGED:
				pListener->selectedInstrumentChangedEvent();
				break;

			case EVENT_INSTRUMENT_PARAMETERS_CHANGED:
				pListener->instrumentParametersChangedEvent( event.value );
				break;

			case EVENT_MIDI_ACTIVITY:
				pListener->midiActivityEvent();
				break;

			case EVENT_NOTEON:
				pListener->noteOnEvent( event.value );
				break;

			case EVENT_ERROR:
				pListener->errorEvent( event.value );
				break;

			case EVENT_XRUN:
				pListener->XRunEvent();
				break;

			case EVENT_METRONOME:
				pListener->metronomeEvent( event.value );
				break;

			case EVENT_PROGRESS:
				pListener->progressEvent( event.value );
				break;

			case EVENT_JACK_SESSION:
				pListener->jacksessionEvent( event.value );
				break;

			case EVENT_PLAYLIST_LOADSONG:
				pListener->playlistLoadSongEvent( event.value );
				break;

			case EVENT_UNDO_REDO:
				pListener->undoRedoActionEvent( event.value );
				break;

			case EVENT_TEMPO_CHANGED:
				pListener->tempoChangedEvent( event.value );
				break;
				
			case EVENT_UPDATE_PREFERENCES:
				pListener->updatePreferencesEvent( event.value );
				break;
			
			case EVENT_UPDATE_SONG:
				pListener->updateSongEvent( event.value );
				break;
				
			case EVENT_QUIT:
				pListener->quitEvent( event.value );
				break;

			case EVENT_TIMELINE_ACTIVATION:
				pListener->timelineActivationEvent();
				break;

			case EVENT_TIMELINE_UPDATE:
				pListener->timelineUpdateEvent( event.value );
				break;

			case EVENT_JACK_TRANSPORT_ACTIVATION:
				pListener->jackTransportActivationEvent();
				break;

			case EVENT_JACK_TIMEBASE_STATE_CHANGED:
				pListener->jackTimebaseStateChangedEvent( event.value );
				break;
				
			case EVENT_SONG_MODE_ACTIVATION:
				pListener->songModeActivationEvent();
				break;

			case EVENT_STACKED_MODE_ACTIVATION:
				pListener->stackedModeActivationEvent( event.value );
				break;
				
			case EVENT_LOOP_MODE_ACTIVATION:
				pListener->loopModeActivationEvent();
				break;

			case EVENT_ACTION_MODE_CHANGE:
				pListener->actionModeChangeEvent( event.value );
				break;

			case EVENT_GRID_CELL_TOGGLED:
				pListener->gridCellToggledEvent();
				break;

			case EVENT_DRUMKIT_LOADED:
				pListener->drumkitLoadedEvent();
				break;

			case EVENT_PATTERN_EDITOR_LOCKED:
				pListener->patternEditorLockedEvent();
				break;
				
			case EVENT_RELOCATION:
				pListener->relocationEvent();
				break;
				
			case EVENT_BBT_CHANGED:
				pListener->bbtChangedEvent();
				break;

			case EVENT_SONG_SIZE_CHANGED:
				pListener->songSizeChangedEvent();
				break;

			case EVENT_DRIVER_CHANGED:
				pListener->driverChangedEvent();
				break;

			case EVENT_PLAYBACK_TRACK_CHANGED:
				pListener->playbackTrackChangedEvent();
				break;

			case EVENT_SOUND_LIBRARY_CHANGED:
				pListener->soundLibraryChangedEvent();
				break;

			case EVENT_NEXT_SHOT:
				pListener->nextShotEvent();
				break;

			case EVENT_MIDI_MAP_CHANGED:
				pListener->midiMapChangedEvent();
				break;

			default:
				ERRORLOG( QString("[onEventQueueTimer] Unhandled event: %1").arg( event.type ) );
			}
		}

	}

	// midi notes
	while( !pQueue->m_addMidiNoteVector.empty() ){
		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
		auto pInstrument = pSong->getInstrumentList()->
			get( pQueue->m_addMidiNoteVector[0].m_row );
		
		// find if a (pitch matching) note is already present
		Note* pOldNote = pSong->getPatternList()->get( pQueue->m_addMidiNoteVector[0].m_pattern )->
			find_note( pQueue->m_addMidiNoteVector[0].m_column,
					   pQueue->m_addMidiNoteVector[0].m_column,
					   pInstrument,
					   pQueue->m_addMidiNoteVector[0].nk_noteKeyVal,
					   pQueue->m_addMidiNoteVector[0].no_octaveKeyVal );
		
		auto pUndoStack = HydrogenApp::get_instance()->m_pUndoStack;
		pUndoStack->beginMacro( tr( "Input Midi Note" ) );
		if( pOldNote ) { // note found => remove it
			SE_addOrDeleteNoteAction *action = new SE_addOrDeleteNoteAction( pOldNote->get_position(),
																	 pOldNote->get_instrument_id(),
																	 pQueue->m_addMidiNoteVector[0].m_pattern,
																	 pOldNote->get_length(),
																	 pOldNote->get_velocity(),
																	 pOldNote->getPan(),
																	 pOldNote->get_lead_lag(),
																	 pOldNote->get_key(),
																	 pOldNote->get_octave(),
																	 pOldNote->get_probability(),
																	 /*isDelete*/ true,
																	 /*hearNote*/ false,
																	 /*isMidi*/ false,
																	 /*isInstrumentMode*/ false,
																	 /*isNoteOff*/ false );
			pUndoStack->push( action );
		}
		
		// add the new note
		SE_addOrDeleteNoteAction *action = new SE_addOrDeleteNoteAction( pQueue->m_addMidiNoteVector[0].m_column,
																	 pQueue->m_addMidiNoteVector[0].m_row,
																	 pQueue->m_addMidiNoteVector[0].m_pattern,
																	 pQueue->m_addMidiNoteVector[0].m_length,
																	 pQueue->m_addMidiNoteVector[0].f_velocity,
																	 pQueue->m_addMidiNoteVector[0].f_pan,
																	 0.0,
																	 pQueue->m_addMidiNoteVector[0].nk_noteKeyVal,
																	 pQueue->m_addMidiNoteVector[0].no_octaveKeyVal,
																	 1.0f,
																	 /*isDelete*/ false,
																	 /*hearNote*/ false,
																	 pQueue->m_addMidiNoteVector[0].b_isMidi,
																	 pQueue->m_addMidiNoteVector[0].b_isInstrumentMode,
																	 /*isNoteOff*/ false );
		pUndoStack->push( action );
		pUndoStack->endMacro();
		pQueue->m_addMidiNoteVector.erase( pQueue->m_addMidiNoteVector.begin() );
	}
}


void HydrogenApp::addEventListener( EventListener* pListener )
{
	if (pListener) {
		m_EventListeners.push_back( pListener );
	}
}


void HydrogenApp::removeEventListener( EventListener* pListener )
{
	for ( uint i = 0; i < m_EventListeners.size(); i++ ) {
		if ( pListener == m_EventListeners[ i ] ) {
			m_EventListeners.erase( m_EventListeners.begin() + i );
		}
	}
}

/**
 * Removes temporary files that were created
 * for undo'ing things.
 */
void HydrogenApp::cleanupTemporaryFiles()
{
	Filesystem::rm( Filesystem::tmp_dir(), true );
}

void HydrogenApp::updatePreferencesEvent( int nValue ) {
	
	QString sPreferencesFilename;
	
	// Local path of the preferences used during session management.
	const QString sPreferencesOverwritePath = 
		H2Core::Filesystem::getPreferencesOverwritePath();
	if ( sPreferencesOverwritePath.isEmpty() ) {
		sPreferencesFilename = Filesystem::usr_config_path();
	} else {
		sPreferencesFilename = sPreferencesOverwritePath;
	}
		
	if ( nValue == 0 ) {
		showStatusBarMessage( tr("Preferences saved.") + 
							  QString(" Into: ") + sPreferencesFilename );
	} else if ( nValue == 1 ) {
		
		// Since the Preferences have changed, we also have to reflect
		// these changes in the GUI - its format, colors, fonts,
		// selections etc.
		// But we won't change the layout!
		Preferences *pPref = Preferences::get_instance();
		InterfaceTheme::Layout layout = pPref->getDefaultUILayout();

		WindowProperties audioEngineInfoProp = pPref->getAudioEngineInfoProperties();
		setWindowProperties( m_pAudioEngineInfoForm, audioEngineInfoProp, SetWidth + SetHeight );

		// MAINFORM
		WindowProperties mainFormProp = pPref->getMainFormProperties();
		setWindowProperties( m_pMainForm, mainFormProp );

		m_pSplitter->setOrientation( Qt::Vertical );
		m_pSplitter->setOpaqueResize( true );

		// SONG EDITOR
		WindowProperties songEditorProp = pPref->getSongEditorProperties();
		setWindowProperties( m_pSongEditorPanel, songEditorProp, SetWidth + SetHeight );

		// PATTERN EDITOR
		WindowProperties patternEditorProp = pPref->getPatternEditorProperties();
		setWindowProperties( m_pPatternEditorPanel, patternEditorProp, SetWidth + SetHeight );
		
		WindowProperties instrumentRackProp = pPref->getInstrumentRackProperties();
		m_pInstrumentRack->setHidden( !instrumentRackProp.visible );

		WindowProperties mixerProp = pPref->getMixerProperties();
		if ( layout != InterfaceTheme::Layout::SinglePane ) {
			mixerProp.visible = false;
		}
		setWindowProperties( m_pMixer, mixerProp );

		m_pMixer->updateMixer();

		WindowProperties playlistDialogProp = pPref->getPlaylistDialogProperties();
		setWindowProperties( m_pPlaylistDialog, playlistDialogProp, SetAll );

		WindowProperties directorProp = pPref->getDirectorProperties();
		setWindowProperties( m_pDirector, directorProp, SetAll );

#ifdef H2CORE_HAVE_LADSPA
		// LADSPA FX
		for (uint nFX = 0; nFX < MAX_FX; nFX++) {
			m_pLadspaFXProperties[nFX]->hide();
			WindowProperties prop = pPref->getLadspaProperties(nFX);
			setWindowProperties( m_pLadspaFXProperties[ nFX ], prop, SetX + SetY );
		}
#endif

		// Inform the user about which file was loaded.
		showStatusBarMessage( tr("Preferences loaded.") + 
							  QString(" From: ") + sPreferencesFilename );

	
	} else {
		ERRORLOG( QString( "Unknown event parameter [%1] in HydrogenApp::updatePreferencesEvent" )
				  .arg( nValue ) );
	}
	
}

void HydrogenApp::updateSongEvent( int nValue ) {

	auto pHydrogen = Hydrogen::get_instance();	
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	
	if ( nValue == 0 ) {
		// Cleanup
		closeFXProperties();
		m_pUndoStack->clear();
		
		// Update GUI components
		updateWindowTitle();
		
	} else if ( nValue == 1 ) {
		
		QString sFilename = pSong->getFilename();
		
		// Song was saved.
		showStatusBarMessage( tr("Song saved as: ") + sFilename );
		updateWindowTitle();
		
	} else if ( nValue == 2 ) {

		// The event was triggered before the Song was fully loaded by
		// the core. It's most likely to be present by now, but it's
		// probably better to avoid displaying its path just to be
		// sure.
		QMessageBox::information( m_pMainForm, "Hydrogen", tr("Song is read-only.\nUse 'Save as' to enable autosave." ) );
	}
}

void HydrogenApp::changePreferences( H2Core::Preferences::Changes changes ) {
	if ( m_pPreferencesUpdateTimer->isActive() ) {
		m_pPreferencesUpdateTimer->stop();
	}
	m_pPreferencesUpdateTimer->start( m_nPreferencesUpdateTimeout );
	// Ensure the provided changes will be propagated too.

	if ( ! ( m_bufferedChanges & changes ) ) {
		m_bufferedChanges = static_cast<H2Core::Preferences::Changes>(m_bufferedChanges | changes);

	}
}

void HydrogenApp::propagatePreferences() {
	emit preferencesChanged( m_bufferedChanges );
	m_bufferedChanges = H2Core::Preferences::Changes::None;
}

bool HydrogenApp::checkDrumkitLicense( std::shared_ptr<H2Core::Drumkit> pDrumkit ) {

	auto pPref = H2Core::Preferences::get_instance();

	if ( ! pPref->m_bShowExportDrumkitLicenseWarning &&
		 ! pPref->m_bShowExportDrumkitCopyleftWarning &&
		 ! pPref->m_bShowExportDrumkitAttributionWarning ) {
		return true;
	}
	
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	auto drumkitLicense = pDrumkit->get_license();
	auto contentVector = pDrumkit->summarizeContent();

	QStringList additionalLicenses;
	bool bCopyleftLicenseFound = drumkitLicense.isCopyleft();
	bool bAttributionRequired = drumkitLicense.hasAttribution();

	for ( const auto& ccontent : contentVector ) {
		QString sSampleLicense = QString( "%1 (by %2)" )
			.arg( ccontent->m_license.getLicenseString() )
			.arg( ccontent->m_license.getCopyrightHolder() );
		
		if ( ccontent->m_license != drumkitLicense &&
			 ! additionalLicenses.contains( sSampleLicense ) ) {
			additionalLicenses << sSampleLicense;

			bCopyleftLicenseFound = bCopyleftLicenseFound ||
				ccontent->m_license.isCopyleft();
			bAttributionRequired = bAttributionRequired ||
				ccontent->m_license.hasAttribution();
		}
	}

	if ( additionalLicenses.size() > 0 &&
		 pPref->m_bShowExportDrumkitLicenseWarning ) {

		QString sMsg = tr( "Some sample licenses deviate from the one assigned to the overall drumkit [%1] and will be overwritten. Are you sure?" )
			.arg( drumkitLicense.getLicenseString() );

		sMsg.append( "\n" );
		for ( const auto& sLicense : additionalLicenses ) {
			sMsg.append( QString( "\n- %1" ).arg( sLicense ) );
		}
		
		QMessageBox licenseWarning;
		licenseWarning.setWindowTitle( pCommonStrings->getLicenseWarningWindowTitle() );
		licenseWarning.setText( sMsg );
		licenseWarning.addButton( pCommonStrings->getButtonOk(),
								   QMessageBox::AcceptRole );
		auto pMuteButton =
			licenseWarning.addButton( pCommonStrings->getMutableDialog(),
									  QMessageBox::YesRole );
		auto pRejectButton =
			licenseWarning.addButton( pCommonStrings->getButtonCancel(),
									  QMessageBox::RejectRole );

		licenseWarning.exec();

		if ( licenseWarning.clickedButton() == pMuteButton ) {
			pPref->m_bShowExportDrumkitLicenseWarning = false;
		}
		else if ( licenseWarning.clickedButton() == pRejectButton ) {
			ERRORLOG( "Aborted on overwriting licenses" );
			return false;
		}
	}

	if ( bCopyleftLicenseFound &&
		 pPref->m_bShowExportDrumkitCopyleftWarning ) {
		QMessageBox copyleftWarning;
		copyleftWarning.setWindowTitle( pCommonStrings->getLicenseWarningWindowTitle() );
		copyleftWarning.setText( pCommonStrings->getLicenseCopyleftWarning() );
		copyleftWarning.addButton( pCommonStrings->getButtonOk(),
								   QMessageBox::AcceptRole );
		auto pMuteButton =
			copyleftWarning.addButton( pCommonStrings->getMutableDialog(),
									  QMessageBox::YesRole );
		auto pRejectButton =
			copyleftWarning.addButton( pCommonStrings->getButtonCancel(),
									  QMessageBox::RejectRole );

		copyleftWarning.exec();

		if ( copyleftWarning.clickedButton() == pMuteButton ) {
			pPref->m_bShowExportDrumkitCopyleftWarning = false;
		}
		else if ( copyleftWarning.clickedButton() == pRejectButton ) {
			ERRORLOG( "Aborted on copyleft licenses" );
			return false;
		}
	}

	if ( bAttributionRequired &&
		 pPref->m_bShowExportDrumkitAttributionWarning ) {
		QMessageBox attributionWarning;
		attributionWarning.setWindowTitle( pCommonStrings->getLicenseWarningWindowTitle() );
		attributionWarning.setText( pCommonStrings->getLicenseAttributionWarning() );
		attributionWarning.addButton( pCommonStrings->getButtonOk(),
								   QMessageBox::AcceptRole );
		auto pMuteButton =
			attributionWarning.addButton( pCommonStrings->getMutableDialog(),
									  QMessageBox::YesRole );
		auto pRejectButton =
			attributionWarning.addButton( pCommonStrings->getButtonCancel(),
									  QMessageBox::RejectRole );

		attributionWarning.exec();

		if ( attributionWarning.clickedButton() == pMuteButton ) {
			pPref->m_bShowExportDrumkitAttributionWarning = false;
		}
		else if ( attributionWarning.clickedButton() == pRejectButton ) {
			ERRORLOG( "Aborted on attribution licenses" );
			return false;
		}
	}
	
	return true;
}

void HydrogenApp::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	if ( changes & H2Core::Preferences::Changes::AudioTab ) {
		H2Core::Hydrogen::get_instance()->getAudioEngine()->
			getMetronomeInstrument()->set_volume(
				Preferences::get_instance()->m_fMetronomeVolume );
	}
}
