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

#include "HydrogenApp.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/PatternList.h>
#include <core/config.h>
#include <core/EventQueue.h>
#include <core/FX/LadspaFX.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Version.h>

#include "AudioEngineInfoForm.h"
#include "CommonStrings.h"
#include "Director.h"
#include "Footer/Footer.h"
#include "FilesystemInfoForm.h"
#include "InstrumentRack.h"
#include "LadspaFXProperties.h"
#include "MainForm.h"
#include "Mixer/Mixer.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "PatternEditor/PatternEditorRuler.h"
#include "MainToolBar/MainToolBar.h"
#include "PlaylistEditor/PlaylistEditor.h"
#include "PreferencesDialog/PreferencesDialog.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "SampleEditor/SampleEditor.h"
#include "UndoActions.h"
#include "Widgets/AutomationPathView.h"
#include "Widgets/EditorDefs.h"
#include "Widgets/InfoBar.h"


#include <QtGui>
#include <QtWidgets>


using namespace H2Core;


HydrogenApp* HydrogenApp::m_pInstance = nullptr;

HydrogenApp::HydrogenApp( MainForm *pMainForm, QUndoStack* pUndoStack )
 : m_pMainForm( pMainForm )
 , m_pMixer( nullptr )
 , m_pPatternEditorPanel( nullptr )
 , m_pAudioEngineInfoForm( nullptr )
 , m_pSongEditorPanel( nullptr )
 , m_pMainToolBar( nullptr )
 , m_pPlaylistEditor( nullptr )
 , m_pSampleEditor( nullptr )
 , m_pDirector( nullptr )
 , m_nPreferencesUpdateTimeout( 100 )
 , m_bufferedChanges( H2Core::Preferences::Changes::None )
 , m_pMainScrollArea( new QScrollArea )
 , m_pUndoStack( pUndoStack )
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
	connect( m_pPreferencesUpdateTimer, SIGNAL(timeout()),
			 this, SLOT(propagatePreferences()) );

	m_pCommonStrings = std::make_shared<CommonStrings>();

	updateWindowTitle();

	const auto pPref = Preferences::get_instance();

	setupSinglePanedInterface();

	// restore audio engine form properties
	m_pAudioEngineInfoForm = new AudioEngineInfoForm( nullptr );
	WindowProperties audioEngineInfoProp = pPref->getAudioEngineInfoProperties();
	setWindowProperties( m_pAudioEngineInfoForm, audioEngineInfoProp, SetX + SetY );
	
	m_pFilesystemInfoForm = new FilesystemInfoForm( nullptr );

	// This must be done _after_ the creation of m_pCommonStrings.
	m_pPlaylistEditor = new PlaylistEditor( nullptr );
	WindowProperties playlistEditorProp = pPref->getPlaylistEditorProperties();
	setWindowProperties( m_pPlaylistEditor, playlistEditorProp, SetAll );

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

void HydrogenApp::setWindowProperties( QWidget *pWindow, WindowProperties& prop, unsigned flags ) {
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
	delete m_pPlaylistEditor;
	delete m_pDirector;
	delete m_pFooter;
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
	if ( pHydrogen != nullptr ) {
		delete pHydrogen;
	}

	m_pInstance = nullptr;

}



void HydrogenApp::setupSinglePanedInterface()
{
	const auto pPref = Preferences::get_instance();
	InterfaceTheme::Layout layout = pPref->getTheme().m_interface.m_layout;

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
	H2Core::CoreActionController::locateToColumn( 0 );

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
	pEditorHBox->setContentsMargins( 0, 0, 0, 0 );
	pSouthPanel->setLayout( pEditorHBox );

	// INSTRUMENT RACK
	m_pInstrumentRack = new InstrumentRack( nullptr );
	m_pInstrumentRack->getInstrumentEditorPanel()->updateEditors();
	WindowProperties instrumentRackProp = pPref->getInstrumentRackProperties();
	m_pInstrumentRack->setVisible( instrumentRackProp.visible );

	if( layout == InterfaceTheme::Layout::Tabbed ){
		m_pTab->setMovable( false );
		m_pTab->setTabsClosable( false );
		m_pTab->addTab( pSouthPanel, tr( "Instrument + Pattern") );
	}

	// PATTERN EDITOR
	m_pPatternEditorPanel = new PatternEditorPanel( nullptr );
	m_pPatternEditorPanel->createEditors();
	// Sync the playhead position in all editors all objects are available.
	m_pPatternEditorPanel->getPatternEditorRuler()->updatePosition( true );
	WindowProperties patternEditorProp = pPref->getPatternEditorProperties();
	setWindowProperties( pSouthPanel, patternEditorProp, SetHeight );

	pEditorHBox->addWidget( m_pPatternEditorPanel );
	pEditorHBox->addWidget( m_pInstrumentRack );

	m_pMainToolBar = new MainToolBar( nullptr );

	m_pFooter = new Footer( nullptr );

	QWidget *mainArea = new QWidget( m_pMainForm );	// this is the main widget
	m_pMainForm->setCentralWidget( mainArea );

	// LAYOUT!!
	m_pMainVBox = new QVBoxLayout();
	m_pMainVBox->setSpacing( 1 );
	m_pMainVBox->setContentsMargins( 0, 0, 0, 0 );
	m_pMainVBox->addWidget( m_pMainToolBar );

	if( layout == InterfaceTheme::Layout::SinglePane ) {
		m_pMainVBox->addWidget( m_pSplitter );
	} else {
		m_pMainVBox->addWidget( m_pTab );
	}

	m_pMainVBox->addWidget( m_pFooter );

	mainArea->setLayout( m_pMainVBox );

	mainArea->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	mainArea->setMinimumSize( HydrogenApp::nMinimumWidth,
							  180 + // menu bar, margins etc.
							  MainToolBar::nHeight +
							  SongEditorPanel::nMinimumHeight +
							  InstrumentRack::m_nMinimumHeight +
							  SongEditorPositionRuler::m_nMinimumHeight +
							  SongEditor::nMinimumHeight +
							  Footer::nHeight +
							  AutomationPathView::m_nMinimumHeight );

	m_pMainScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_pMainScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_pMainScrollArea->setFocusPolicy( Qt::ClickFocus );
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
		m_pTab->setCurrentIndex( pPref->getLastOpenTab() );
		QObject::connect(m_pTab, SIGNAL(currentChanged(int)),this,SLOT(currentTabChanged(int)));
	}
}


InfoBar *HydrogenApp::addInfoBar() {
	InfoBar *pInfoBar = new InfoBar();
	m_pMainVBox->insertWidget( 1, pInfoBar );
	return pInfoBar;
}

void HydrogenApp::pushUndoCommand( QUndoCommand* pCommand,
								   const QString& sContext ) {
	if ( pCommand == nullptr ) {
		return;
	}

	handleUndoContext( sContext, pCommand->text() );

	m_pUndoStack->push( pCommand );
}

void HydrogenApp::beginUndoMacro( const QString& sText, const QString& sContext ) {
	handleUndoContext( sContext, sText );

	if ( sContext.isEmpty() && m_pUndoStack->count() > 0 &&
		 ! m_pUndoStack->canUndo() && ! m_pUndoStack->canRedo() ) {
		// There is most probably already a macro which has not been ended yet.
		// We do not support nested marcos yet, because we a) do not need them
		// yet and b) want to ensure all beginMacro() and endMacro() are
		// properly balanced.
		WARNINGLOG( "There was already an unbalanced macro. Ending it first." );
		m_pUndoStack->endMacro();
	}

	m_pUndoStack->beginMacro( sText );
}

void HydrogenApp::endUndoMacro( const QString& sContext ) {
	handleUndoContext( sContext, "endUndoMacro" );

	m_pUndoStack->endMacro();
}

void HydrogenApp::endUndoContext() {
	handleUndoContext( "", "" );
}

void HydrogenApp::handleUndoContext( const QString& sContext,
									 const QString& sText ) {
	if ( sContext == m_sLastUndoContext ) {
		return;
	}

	// Close the previous batch of nested macros corresponding to an action.
	if ( sContext != m_sLastUndoContext && ! m_sLastUndoContext.isEmpty() &&
		 m_pUndoStack->count() > 0 ) {
		if ( m_pUndoStack->canUndo() || m_pUndoStack->canRedo() ) {
			WARNINGLOG( QString( "Undo stack does not seem to be in last context [%1]. Trying to end regardlessly." )
						.arg( m_sLastUndoContext ) );
		}
		m_pUndoStack->endMacro();

		if ( ! m_pUndoStack->canUndo() && ! m_pUndoStack->canRedo() ) {
			ERRORLOG( QString( "Undo stack was in nested macro while ending last context [%1]" )
					  .arg( m_sLastUndoContext ) );
			m_pUndoStack->endMacro();
		}
	}

	// Start a new macro for this context
	if ( ! sContext.isEmpty() ) {
		m_pUndoStack->beginMacro( sText );
	}

	m_sLastUndoContext = sContext;
}

void HydrogenApp::currentTabChanged(int index)
{
	Preferences::get_instance()->setLastOpenTab( index );
	m_pMainToolBar->updateActions();
}

void HydrogenApp::closeFXProperties()
{
#ifdef H2CORE_HAVE_LADSPA
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX]->close();
	}
#endif
}

QString HydrogenApp::findAutoSaveFile( const Filesystem::Type& type,
									   const QString& sBaseFile ) {
	QString sExtension, sEmpty;
	switch ( type ) {
	case Filesystem::Type::Song:
		sExtension = Filesystem::songs_ext;
		/*: Object containing unsaved changes.*/
		sEmpty = tr( "New Song" );
		break;

	case Filesystem::Type::Playlist:
		sExtension = Filesystem::playlist_ext;
		/*: Object containing unsaved changes.*/
		sEmpty = tr( "New Playlist" );
		break;

	default:
		ERRORLOG( QString( "Unsupported file type: [%1]" )
				  .arg( Filesystem::TypeToQString( type ) ) );
		return "";
	}

	// Check whether there is an autosave file next to it
	// containing newer content.
	QFileInfo fileInfo( sBaseFile );

	// In case the user did open a hidden file, the baseName()
	// will be an empty string.
	QString sBaseName( fileInfo.completeBaseName() );
	if ( sBaseName.startsWith( "." ) ) {
		sBaseName.remove( 0, 1 );
	}

	// Hidden autosave file (recent version)
	QFileInfo autoSaveFileRecent( QString( "%1/.%2.autosave%3" )
								  .arg( fileInfo.absoluteDir().absolutePath() )
								  .arg( sBaseName ).arg( sExtension ) );
	// Visible autosave file (older version)
	QFileInfo autoSaveFileOld( QString( "%1/%2.autosave%3" )
							   .arg( fileInfo.absoluteDir().absolutePath() )
							   .arg( sBaseName ).arg( sExtension ) );
	QString sRecoverFilename = "";
	if ( autoSaveFileRecent.exists() &&
		 autoSaveFileRecent.lastModified() > fileInfo.lastModified() ) {
		sRecoverFilename = autoSaveFileRecent.absoluteFilePath();
	}
	else if ( autoSaveFileOld.exists() &&
				autoSaveFileOld.lastModified() > fileInfo.lastModified() ) {
		sRecoverFilename = autoSaveFileOld.absoluteFilePath();
	}
	else if ( sBaseFile == Filesystem::empty_path( type ) &&
			  autoSaveFileRecent.exists() ) {
		sRecoverFilename = autoSaveFileRecent.absoluteFilePath();
	}

	if ( sRecoverFilename.isEmpty() ) {
		return "";
	}

	QString sFile;
	if ( sBaseFile == Filesystem::empty_path( type ) ) {
		sFile = sEmpty;
	} else {
		sFile = sBaseFile;
	}

	QMessageBox msgBox;
	// Not commonized in CommmonStrings as it is required before
	// HydrogenApp was instantiated.
	msgBox.setText( QString( "%1\n[%2]" )
					.arg( tr( "There are unsaved changes." ) ).arg( sFile ) );
	msgBox.setInformativeText( tr( "Do you want to recover them?" ) );
	msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::No );
	msgBox.setDefaultButton( QMessageBox::No );
	msgBox.setWindowTitle( "Hydrogen" );
	msgBox.setIcon( QMessageBox::Question );
	int nRet = msgBox.exec();

	if ( nRet == QMessageBox::Ok ) {
		return sRecoverFilename;
	}
	else {
		return "";
	}
}

bool HydrogenApp::openFile( const Filesystem::Type& type, const QString& sFilename ) {

	QString sText;
	switch( type ) {
	case Filesystem::Type::Song:
		sText = tr( "Error loading song." );
		break;

	case Filesystem::Type::Playlist:
		sText = tr( "Error loading playlist." );
		break;

	default:
		ERRORLOG( QString( "Unsupported type [%1]" )
				  .arg( Filesystem::TypeToQString( type ) ) );
		return false;
	}

	QString sPath;
	if ( sFilename.isEmpty() ) {
		sPath = H2Core::Filesystem::empty_path( type );
	}
	else {
		sPath = H2Core::Filesystem::absolute_path( sFilename );
	}
	const auto sRecoverFilename = findAutoSaveFile( type, sPath );

	bool bRet;
	// Ensure the path to the file is not relative.
	if ( type == Filesystem::Type::Song ) {
		std::shared_ptr<Song> pSong;
		if ( sFilename.isEmpty() && sRecoverFilename.isEmpty() ) {
			pSong = Song::getEmptySong();
		} else {
			pSong = CoreActionController::loadSong( sPath, sRecoverFilename );
		}

		bRet = CoreActionController::setSong( pSong );
	}
	else {
		std::shared_ptr<Playlist> pPlaylist;
		if ( sFilename.isEmpty() && sRecoverFilename.isEmpty() ) {
			pPlaylist = std::make_shared<Playlist>();
		} else {
			pPlaylist = CoreActionController::loadPlaylist( sPath, sRecoverFilename );
		}

		bRet = CoreActionController::setPlaylist( pPlaylist );
	}

	if ( ! bRet ) {
		QMessageBox msgBox;
		// Not commonized in CommmonStrings as it is required before
		// HydrogenApp was instantiated.
		msgBox.setText( QString( "%1\n[%2]" ).arg( sText ).arg( sPath ) );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Warning );
		msgBox.exec();
		return false;
	}

	return true;
}

bool HydrogenApp::openSong( std::shared_ptr<Song> pSong ) {

	if ( ! CoreActionController::setSong( pSong ) ) {
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

// Returns true if unsaved changes are successfully handled (saved, discarded, etc.)
// Returns false if not (i.e. Cancel)
bool HydrogenApp::handleUnsavedChanges( const H2Core::Filesystem::Type& type )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPlaylist = pHydrogen->getPlaylist();

	bool bIsModified = false;
	QString sText;

	switch( type ) {
	case Filesystem::Type::Song:
		if ( pSong != nullptr ) {
			bIsModified = pSong->getIsModified();
		}
		/*: The symbols `<b>` and `</b>` correspond to HTML code
		  printing the enclosed `Song` in bold letters. Please do not alter them
		  but translate the enclosed `Song` instead.*/
		sText = tr( "The current <b>Song</b> contains unsaved changes." );
		break;

	case Filesystem::Type::Playlist:
		if ( pPlaylist != nullptr ) {
			bIsModified = pPlaylist->getIsModified();
		}
		/*: The symbols `<b>` and `</b>` correspond to HTML code
		  printing the enclosed `Playlist` in bold letters. Please do not alter
		  them but translate the enclosed `Playlist` instead.*/
		sText = tr( "The current <b>Playlist</b> contains unsaved changes." );
		break;

	default:
		ERRORLOG( QString( "Unsupported type [%1]" )
				  .arg( Filesystem::TypeToQString( type ) ) );
		return false;
	}

	auto pCommonStrings = pHydrogenApp->getCommonStrings();
	auto newDialog = [=]( const QString& sText ) {
		QMessageBox msgBox;
		// Not commonized in CommmonStrings as it is required before
		// HydrogenApp was instantiated.
		msgBox.setText( sText );
		msgBox.setInformativeText( pCommonStrings->getSavingChanges() );
		msgBox.setStandardButtons( QMessageBox::Save | QMessageBox::No |
									QMessageBox::Cancel );
		msgBox.setDefaultButton( QMessageBox::Save );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Question );
		return msgBox.exec();
	};

	if ( bIsModified ) {
		const int nRet = newDialog( sText );

		switch( nRet ) {
		case QMessageBox::Save:
			bool bOk;

			if ( type == Filesystem::Type::Song ) {
				if ( ! pSong->getFilename().isEmpty() ) {
					bOk = pHydrogenApp->getMainForm()->action_file_save();
				} else {
					// never been saved
					bOk = pHydrogenApp->getMainForm()->action_file_save_as();
				}
			}
			else {
				if ( ! pPlaylist->getFilename().isEmpty() ) {
					bOk = pHydrogenApp->getPlaylistEditor()->savePlaylist();
				} else {
					// never been saved
					bOk = pHydrogenApp->getPlaylistEditor()->savePlaylistAs();
				}
			}

			if ( ! bOk ) {
				ERRORLOG( QString( "Unable to save current %1" )
						  .arg( Filesystem::TypeToQString( type ) ) );
				return false;
			}
			break;

		case QMessageBox::No:
			break;

		case QMessageBox::Cancel:
			INFOLOG( QString( "Writing unsave changes to %1 canceled." )
						  .arg( Filesystem::TypeToQString( type ) ) );
			return false;

		default:
			ERRORLOG( QString( "Unhandled return code for %1 [%2]" )
						  .arg( Filesystem::TypeToQString( type ) ).arg( nRet ) );
		}
	}

	return true;
}

void HydrogenApp::showMixer(bool show)
{
	/*
		 *   Switch to Mixer tab with alt+m in tabbed mode,
		 *   otherwise open mixer window
		 */

	auto layout = Preferences::get_instance()->
		getTheme().m_interface.m_layout;

	if ( layout == InterfaceTheme::Layout::Tabbed ) {
		m_pTab->setCurrentIndex( 2 );
	} else {
		m_pMixer->setVisible( show );
	}

	// Update visibility button.
	m_pMainToolBar->updateActions();
	m_pMainForm->updateMenuBar();
}

void HydrogenApp::showInstrumentRack(bool show)
{
	/*
		 *   Switch to pattern editor/instrument tab in tabbed mode,
		 *   otherwise hide instrument panel
		 */
	auto layout = Preferences::get_instance()->
		getTheme().m_interface.m_layout;

	if ( layout == InterfaceTheme::Layout::Tabbed ) {
		m_pTab->setCurrentIndex( 1 );
		m_pInstrumentRack->setVisible( show );
	}
	else {
		m_pInstrumentRack->setVisible( show );
	}

	// Update visibility button.
	m_pMainToolBar->updateActions();
	m_pMainForm->updateMenuBar();
}

void HydrogenApp::showPreferencesDialog() {
	m_pMainToolBar->setPreferencesVisibilityState( true );

	PreferencesDialog preferencesDialog(m_pMainForm);
	preferencesDialog.exec();
}

void HydrogenApp::showStatusBarMessage( const QString& sMessage, const QString& sCaller )
{
	if ( m_pFooter != nullptr ) {
		m_pFooter->showStatusBarMessage( sMessage, sCaller );
	}
}

void HydrogenApp::XRunEvent() {
	const auto pAudioDriver = Hydrogen::get_instance()->getAudioOutput();
	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return;
	}
	showStatusBarMessage(
		QString( "XRUNS [%1]!!!" ).arg( pAudioDriver->getXRuns() ),
		"HydrogenApp::XRunEvent" );
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

	if ( sFilePath == Filesystem::empty_path( Filesystem::Type::Song ) ||
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
		sTitle.append( " (" + m_pCommonStrings->getIsModified() + ")" );
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

void HydrogenApp::showPlaylistEditor()
{
	if ( m_pPlaylistEditor->isVisible() ) {
		m_pPlaylistEditor->hide();
	} else {
		m_pPlaylistEditor->show();
	}
	m_pMainForm->update_playlist_checkbox();

	// Update visibility button.
	m_pMainToolBar->updateActions();
}


void HydrogenApp::showDirector()
{
	auto pHydrogen = Hydrogen::get_instance();

	if ( m_pDirector->isVisible() ) {
		m_pDirector->hide();
		pHydrogen->setSendBbtChangeEvents( false );
	}
	else {
		pHydrogen->setSendBbtChangeEvents( true );
		m_pDirector->show();
	}
	m_pMainForm->update_director_checkbox();

	// Update visibility button.
	m_pMainToolBar->updateActions();
}


void HydrogenApp::showSampleEditor( const QString& name, int nSelectedComponent,
									int nSelectedLayer )
{

	if ( m_pSampleEditor != nullptr ){
		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_pSampleEditor->close();
		delete m_pSampleEditor;
		m_pSampleEditor = nullptr;
		QApplication::restoreOverrideCursor();
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_pSampleEditor = new SampleEditor(
		nullptr, nSelectedComponent, nSelectedLayer, name );
	m_pSampleEditor->show();
	QApplication::restoreOverrideCursor();
}

void HydrogenApp::songModifiedEvent()
{
	updateWindowTitle();
}

void HydrogenApp::playlistLoadSongEvent() {
	showStatusBarMessage(
		tr( "Playlist: Set song No. %1" )
		.arg( Hydrogen::get_instance()->getPlaylist()->getActiveSongNumber() + 1 ) );
}

void HydrogenApp::onEventQueueTimer()
{
	// use the timer to do schedule instrument slaughter;
	EventQueue *pQueue = EventQueue::get_instance();

	while ( true ) {
		auto pEvent = pQueue->popEvent();
		if ( pEvent == nullptr ) {
			break;
		}

		if ( m_eventListenersToAdd.size() > 0 ||
			 m_eventListenersToRemove.size() > 0 ) {
			updateEventListeners();
		}
		
		// Provide the event to all EventListeners registered to
		// HydrogenApp. By registering itself as EventListener and
		// implementing at least on the methods used below a
		// particular GUI component can react on specific events.
		for ( const auto& ppEventListener : m_eventListeners ) {
			if ( m_eventListenersToRemove.size() > 0 &&
				 m_eventListenersToRemove.find( ppEventListener ) !=
				 m_eventListenersToRemove.end() ) {
				// This listener was scheduled to be removed. Most probably the
				// corresponding object is already destructed and we would risk
				// a segfault when attempting to call its methods.
				continue;
			}

			switch ( pEvent->getType() ) {
			case Event::Type::ActionModeChanged:
				ppEventListener->actionModeChangeEvent( pEvent->getValue() );
				break;

			case Event::Type::AudioDriverChanged:
				ppEventListener->audioDriverChangedEvent();
				break;

			case Event::Type::BbtChanged:
				ppEventListener->bbtChangedEvent();
				break;

			case Event::Type::BeatCounter:
				ppEventListener->beatCounterEvent();
				break;

			case Event::Type::DrumkitLoaded:
				ppEventListener->drumkitLoadedEvent();
				break;

			case Event::Type::EffectChanged:
				ppEventListener->effectChangedEvent();
				break;

			case Event::Type::Error:
				ppEventListener->errorEvent( pEvent->getValue() );
				break;

			case Event::Type::GridCellToggled:
				ppEventListener->gridCellToggledEvent();
				break;

			case Event::Type::InstrumentMuteSoloChanged:
				ppEventListener->instrumentMuteSoloChangedEvent( pEvent->getValue() );
				break;

			case Event::Type::InstrumentParametersChanged:
				ppEventListener->instrumentParametersChangedEvent( pEvent->getValue() );
				break;

			case Event::Type::JackTimebaseStateChanged:
				ppEventListener->jackTimebaseStateChangedEvent( pEvent->getValue() );
				break;

			case Event::Type::JackTransportActivation:
				ppEventListener->jackTransportActivationEvent();
				break;

			case Event::Type::LoopModeActivation:
				ppEventListener->loopModeActivationEvent();
				break;

			case Event::Type::Metronome:
				ppEventListener->metronomeEvent( pEvent->getValue() );
				break;

			case Event::Type::MidiDriverChanged:
				ppEventListener->midiDriverChangedEvent();
				break;

			case Event::Type::MidiInput:
				ppEventListener->midiInputEvent();
				break;

			case Event::Type::MidiMapChanged:
				ppEventListener->midiMapChangedEvent();
				break;

			case Event::Type::MidiOutput:
				ppEventListener->midiOutputEvent();
				break;

			case Event::Type::MixerSettingsChanged:
				ppEventListener->mixerSettingsChangedEvent();
				break;

			case Event::Type::NextPatternsChanged:
				ppEventListener->nextPatternsChangedEvent();
				break;

			case Event::Type::NextShot:
				ppEventListener->nextShotEvent();
				break;

			case Event::Type::NoteOn:
				ppEventListener->noteOnEvent( pEvent->getValue() );
				break;

			case Event::Type::Quit:
				ppEventListener->quitEvent( pEvent->getValue() );
				break;

			case Event::Type::PatternEditorLocked:
				ppEventListener->patternEditorLockedEvent();
				break;

			case Event::Type::PatternModified:
				ppEventListener->patternModifiedEvent();
				break;

			case Event::Type::PlaybackTrackChanged:
				ppEventListener->playbackTrackChangedEvent();
				break;

			case Event::Type::PlayingPatternsChanged:
				ppEventListener->playingPatternsChangedEvent();
				break;

			case Event::Type::PlaylistChanged:
				ppEventListener->playlistChangedEvent( pEvent->getValue() );
				break;

			case Event::Type::PlaylistLoadSong:
				ppEventListener->playlistLoadSongEvent();
				break;

			case Event::Type::Progress:
				ppEventListener->progressEvent( pEvent->getValue() );
				break;

			case Event::Type::RecordModeChanged:
				ppEventListener->recordingModeChangedEvent();
				break;

			case Event::Type::Relocation:
				ppEventListener->relocationEvent();
				break;

			case Event::Type::SelectedPatternChanged:
				ppEventListener->selectedPatternChangedEvent();
				break;

			case Event::Type::SelectedInstrumentChanged:
				ppEventListener->selectedInstrumentChangedEvent();
				break;

			case Event::Type::SongModeActivation:
				ppEventListener->songModeActivationEvent();
				break;

			case Event::Type::SongModified:
				ppEventListener->songModifiedEvent();
				break;

			case Event::Type::SongSizeChanged:
				ppEventListener->songSizeChangedEvent();
				break;

			case Event::Type::State:
				ppEventListener->stateChangedEvent( static_cast<H2Core::AudioEngine::State>(pEvent->getValue()) );
				break;

			case Event::Type::StackedModeActivation:
				ppEventListener->stackedModeActivationEvent( pEvent->getValue() );
				break;

			case Event::Type::SoundLibraryChanged:
				ppEventListener->soundLibraryChangedEvent();
				break;

			case Event::Type::TempoChanged:
				ppEventListener->tempoChangedEvent( pEvent->getValue() );
				break;

			case Event::Type::TimelineActivation:
				ppEventListener->timelineActivationEvent();
				break;

			case Event::Type::UpdateTimeline:
				ppEventListener->timelineUpdateEvent( pEvent->getValue() );
				break;

			case Event::Type::UndoRedo:
				ppEventListener->undoRedoActionEvent( pEvent->getValue() );
				break;

			case Event::Type::UpdatePreferences:
				ppEventListener->updatePreferencesEvent( pEvent->getValue() );
				break;

			case Event::Type::UpdateSong:
				ppEventListener->updateSongEvent( pEvent->getValue() );
				break;

			case Event::Type::Xrun:
				ppEventListener->XRunEvent();
				break;

			default:
				ERRORLOG( QString("[onEventQueueTimer] Unhandled event: [%1]")
						  .arg( pEvent->toQString() ) );
			}
		}

	}

	// midi notes
	while( !pQueue->m_addMidiNoteVector.empty() ){
		auto pSong = Hydrogen::get_instance()->getSong();
		if ( pSong == nullptr ) {
			return;
		}

		// The core registers the ID of the instrument the note is associated
		// with. We have to correlate it to a specific row in the DB of the
		// pattern editor.
		bool bFound = false;
		int nRow = 0;
		DrumPatternRow row;
		for ( const auto& rrow : m_pPatternEditorPanel->getDB() ) {
			if ( rrow.nInstrumentID ==
				 pQueue->m_addMidiNoteVector[0].m_instrumentId ) {
				row = rrow;
				bFound = true;
				break;
			}
			++nRow;
		}

		if ( ! bFound ) {
			ERRORLOG( QString( "Could not find row in Pattern Editor corresponding to instrument ID [%1]" )
					  .arg( pQueue->m_addMidiNoteVector[0].m_instrumentId ) );
			return;
		}
		
		// find if a (pitch matching) note is already present
		const auto pOldNote = pSong->getPatternList()->
			get( pQueue->m_addMidiNoteVector[0].m_pattern )->
			findNote( pQueue->m_addMidiNoteVector[0].m_column,
					  row.nInstrumentID, row.sType,
					  pQueue->m_addMidiNoteVector[0].nk_noteKeyVal,
					  pQueue->m_addMidiNoteVector[0].no_octaveKeyVal );
		
		beginUndoMacro( tr( "Input Midi Note" ) );
		if ( pOldNote != nullptr ) { // note found => remove it
			pushUndoCommand( new SE_addOrRemoveNoteAction(
								 pOldNote->getPosition(),
								 pOldNote->getInstrumentId(),
								 pOldNote->getType(),
								 pQueue->m_addMidiNoteVector[0].m_pattern,
								 pOldNote->getLength(),
								 pOldNote->getVelocity(),
								 pOldNote->getPan(),
								 pOldNote->getLeadLag(),
								 pOldNote->getKey(),
								 pOldNote->getOctave(),
								 pOldNote->getProbability(),
								 Editor::Action::Delete,
								 /*isNoteOff*/ false,
								 pOldNote->getInstrument() != nullptr,
								 Editor::ActionModifier::None ) );
		}
		
		// add the new note
		pushUndoCommand( new SE_addOrRemoveNoteAction(
							 pQueue->m_addMidiNoteVector[0].m_column,
							 row.nInstrumentID,
							 row.sType,
							 pQueue->m_addMidiNoteVector[0].m_pattern,
							 pQueue->m_addMidiNoteVector[0].m_length,
							 pQueue->m_addMidiNoteVector[0].f_velocity,
							 pQueue->m_addMidiNoteVector[0].f_pan,
							 LEAD_LAG_DEFAULT,
							 pQueue->m_addMidiNoteVector[0].nk_noteKeyVal,
							 pQueue->m_addMidiNoteVector[0].no_octaveKeyVal,
							 PROBABILITY_DEFAULT,
							 Editor::Action::Add,
							 /*isNoteOff*/ false,
							 row.bMappedToDrumkit,
							 Editor::ActionModifier::Playback ) );
		endUndoMacro();

		pQueue->m_addMidiNoteVector.erase( pQueue->m_addMidiNoteVector.begin() );
	}
}


void HydrogenApp::addEventListener( EventListener* pListener ) {
	if ( pListener != nullptr ) {
		m_eventListenersToAdd.insert( pListener );
	}

	if ( m_eventListenersToRemove.find( pListener ) !=
		 m_eventListenersToRemove.end() ) {
		// Listener was already scheduled to be removed. Last action wins.
		for ( auto it = m_eventListenersToRemove.begin();
			  it != m_eventListenersToRemove.end(); ) {
			if ( *it == pListener ) {
				it = m_eventListenersToRemove.erase( it );
				break;
			} else {
				++it;
			}
		}
	}
}

void HydrogenApp::removeEventListener( EventListener* pListener ) {
	if ( pListener != nullptr ) {
		m_eventListenersToRemove.insert( pListener );
	}

	if ( m_eventListenersToAdd.find( pListener ) !=
		 m_eventListenersToAdd.end() ) {
		for ( auto it = m_eventListenersToAdd.begin();
			  it != m_eventListenersToAdd.end(); ) {
			if ( *it == pListener ) {
				it = m_eventListenersToAdd.erase( it );
				break;
			} else {
				++it;
			}
		}
	}
}

void HydrogenApp::updateEventListeners() {
	for ( const auto& ppEventListener : m_eventListenersToRemove ) {
		for ( auto it = m_eventListeners.begin();
			  it != m_eventListeners.end(); ) {
			if ( *it == ppEventListener ) {
				it = m_eventListeners.erase( it );
				break;
			} else {
				++it;
			}
		}
	}
	m_eventListenersToRemove.clear();

	for ( const auto& ppEventListener : m_eventListenersToAdd ) {
		m_eventListeners.push_back( ppEventListener );
	}
	m_eventListenersToAdd.clear();
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
		const auto pPref = Preferences::get_instance();
		auto layout = pPref->getTheme().m_interface.m_layout;

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

		WindowProperties playlistEditorProp = pPref->getPlaylistEditorProperties();
		setWindowProperties( m_pPlaylistEditor, playlistEditorProp, SetAll );

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

		m_pMainToolBar->updateActions();

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
		// Song was saved.
		updateWindowTitle();
		
	}
	else if ( nValue == 2 ) {
		QMessageBox::information(
			m_pMainForm, "Hydrogen", QString( "%1\n%2" )
			.arg( tr("Song is read-only." ) )
			.arg( m_pCommonStrings->getReadOnlyAdvice() ) );
	}
}

void HydrogenApp::playlistChangedEvent( int nValue ) {
	if ( nValue == 2 ) {
		QMessageBox::information(
			m_pMainForm, "Hydrogen", QString( "%1\n%2" )
			.arg( tr("Playlist is read-only." ) )
			.arg( m_pCommonStrings->getReadOnlyAdvice() ) );
	}
}

void HydrogenApp::changePreferences( const H2Core::Preferences::Changes& changes ) {
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

	const auto pPref = H2Core::Preferences::get_instance();

	if ( ! pPref->m_bShowExportDrumkitLicenseWarning &&
		 ! pPref->m_bShowExportDrumkitCopyleftWarning &&
		 ! pPref->m_bShowExportDrumkitAttributionWarning ) {
		return true;
	}
	
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	auto drumkitLicense = pDrumkit->getLicense();
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

void HydrogenApp::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	if ( changes & H2Core::Preferences::Changes::AudioTab ) {
		H2Core::Hydrogen::get_instance()->getAudioEngine()->
			getMetronomeInstrument()->setVolume(
				Preferences::get_instance()->m_fMetronomeVolume );
	}
}
