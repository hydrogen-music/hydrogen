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
 * $Id: HydrogenApp.cpp,v 1.30 2005/05/09 18:10:54 comix Exp $
 *
 *  Changed DATA_PATH to a DataPath::getDataPath() call to accommodate Mac OS X
 *  application bundles (2005/01/06 Jonathan Dempsey)
 */

#include "HydrogenApp.h"
#include "Skin.h"

#include "lib/Hydrogen.h"
#include "lib/EventQueue.h"
#include "lib/fx/LadspaFX.h"
#include "lib/Preferences.h"


#include "PreferencesDialog.h"
#include "MainForm.h"
#include "PlayerControl.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "InstrumentEditor/InstrumentEditor.h"
#include "SongEditor/SongEditor.h"
#include "Mixer/Mixer.h"
#include "AudioEngineInfoForm.h"
#include "HelpBrowser.h"
#include "DrumkitManager.h"
#include "LadspaFXProperties.h"
#include "SongEditor/SongEditorPanel.h"

#include <qdockwindow.h>
#include <qsplitter.h>
#include <qhbox.h>
#include <qdir.h>

HydrogenApp* HydrogenApp::m_pInstance = NULL;

HydrogenApp::HydrogenApp( MainForm *pMainForm, Song *pFirstSong )
 : Object("HydrogenApp")
 , m_pStatusBar( NULL )
 , m_pMainForm( pMainForm )
 , m_pMixer( NULL )
 , m_pPatternEditorPanel( NULL )
 , m_pAudioEngineInfoForm( NULL )
 , m_pSongEditorPanel( NULL )
 , m_pHelpBrowser( NULL )
 , m_pPlayerControl( NULL )
 , m_pFirstTimeInfo( NULL )
{
	m_pInstance = this;

#ifdef LADSPA_SUPPORT
	// Load the LADSPA plugin list
	m_pluginList = LadspaFX::getPluginList();
	m_pFXRootGroup = LadspaFX::getLadspaFXGroup();
#endif

	m_pEventQueueTimer = new QTimer(this);
	connect( m_pEventQueueTimer, SIGNAL( timeout() ), this, SLOT( onEventQueueTimer() ) );
	m_pEventQueueTimer->start(50);	// update at 20 fps



	// Create the audio engine :)
	Hydrogen::getInstance();

	(Hydrogen::getInstance())->setSong( pFirstSong );
	( Preferences::getInstance() )->setLastSongFilename( pFirstSong->getFilename() );

	// set initial title
	QString qsSongName( pFirstSong->m_sName.c_str() );
	m_pMainForm->setCaption( ( "Hydrogen " + QString(VERSION) + QString( " - " ) + qsSongName ) );
	m_pMainForm->setDockMenuEnabled(false);

	Preferences *pPref = Preferences::getInstance();

	switch ( pPref->getInterfaceMode() ) {
		case Preferences::MDI:
			setupMDIInterface();
			break;

		case Preferences::TOP_LEVEL:
			setupTopLevelInterface();
			break;

		case Preferences::SINGLE_PANED:
			setupSinglePanedInterface();
			break;

		default:
			errorLog( "Bad interface mode: " + toString( pPref->getInterfaceMode() ) );
	}

	// restore audio engine form properties
	m_pAudioEngineInfoForm = new AudioEngineInfoForm( 0 );
	WindowProperties audioEngineInfoProp = pPref->getAudioEngineInfoProperties();
	m_pAudioEngineInfoForm->move( audioEngineInfoProp.x, audioEngineInfoProp.y );
	if ( audioEngineInfoProp.visible ) {
		m_pAudioEngineInfoForm->show();
	}
	else {
		m_pAudioEngineInfoForm->hide();
	}


	m_pStatusBar = new QStatusBar( m_pMainForm );
	m_pStatusBar->setSizeGripEnabled( false );



	// First time information
	showInfoSplash();
}



HydrogenApp::~HydrogenApp()
{
	m_pEventQueueTimer->stop();

	delete m_pFirstTimeInfo;
	delete m_pInstrumentEditor;
	delete m_pPatternEditorPanel;
	delete m_pSongEditorPanel;
	delete m_pHelpBrowser;
	delete m_pAudioEngineInfoForm;

	if (m_pMixer) {
		//m_pMixer->updateStart(false);
		delete m_pMixer;
	}

	delete m_pDrumkitManager;

#ifdef LADSPA_SUPPORT
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		delete m_pLadspaFXProperties[nFX];
	}
	for (uint i = 0; i < m_pluginList.size(); i++) {
		delete m_pluginList[ i ];
	}
	delete m_pFXRootGroup;
#endif


	Hydrogen *engine = Hydrogen::getInstance();
	if (engine) {
		Song *song = engine->getSong();

		delete engine;
		engine = NULL;

		if (song) {
			delete song;
			song = NULL;
		}
	}

}



/// Return an HydrogenApp m_pInstance
HydrogenApp* HydrogenApp::getInstance() {
	if (m_pInstance == NULL) {
		cerr << "Error! HydrogenApp::getInstance (m_pInstance = NULL)" << endl;
	}
	return m_pInstance;
}



void HydrogenApp::setupTopLevelInterface()
{
	infoLog( "[setupTopLevelInterface]" );
	Preferences *pPref = Preferences::getInstance();

	// TOOLBAR
//	QToolBar * fileTools = new QToolBar( m_pMainForm, "file operations" );
	QToolBar * fileTools = new QToolBar( "Transport Toolbar", m_pMainForm, Qt::DockBottom );
	fileTools->setMovingEnabled( false );
	fileTools->setLabel( "Hydrogen control panel" );
	m_pPlayerControl = new PlayerControl( fileTools );

	// restore pattern editor properties
	m_pPatternEditorPanel = new PatternEditorPanel( 0 );
	WindowProperties patternEditorProp = pPref->getPatternEditorProperties();
	m_pPatternEditorPanel->resize( patternEditorProp.width, patternEditorProp.height );
	m_pPatternEditorPanel->move( patternEditorProp.x, patternEditorProp.y );
	if ( patternEditorProp.visible ) {
		m_pPatternEditorPanel->show();
	}
	else {
		m_pPatternEditorPanel->hide();
	}

	// restore m_pMixer properties
	m_pMixer = new Mixer(0);
	WindowProperties mixerProp = pPref->getMixerProperties();
	m_pMixer->resize( mixerProp.width, mixerProp.height );
	m_pMixer->move( mixerProp.x, mixerProp.y );
	if ( mixerProp.visible ) {
		m_pMixer->show();
	}
	else {
		m_pMixer->hide();
	}

	// restore song editor properties
	m_pSongEditorPanel = new SongEditorPanel( m_pMainForm );

	WindowProperties mainFormProp = pPref->getMainFormProperties();
	m_pMainForm->resize( mainFormProp.width, mainFormProp.height );
	m_pMainForm->move( mainFormProp.x, mainFormProp.y );
	m_pMainForm->setCentralWidget( m_pSongEditorPanel );

	// restore drumkit manager properties
	m_pDrumkitManager = new DrumkitManager( 0 );
	WindowProperties drumkitMngProp = pPref->getDrumkitManagerProperties();
	m_pDrumkitManager->move( drumkitMngProp.x, drumkitMngProp.y );
	if ( drumkitMngProp.visible ) {
		m_pDrumkitManager->show();
	}
	else {
		m_pDrumkitManager->hide();
	}

	string sDocPath = string( DataPath::getDataPath() ) + "/doc";
	string sDocURI = sDocPath + "/manual.html";
	m_pHelpBrowser = new SimpleHTMLBrowser( NULL, sDocPath, sDocURI, SimpleHTMLBrowser::MANUAL );

#ifdef LADSPA_SUPPORT
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX] = new LadspaFXProperties( NULL, nFX );
		m_pLadspaFXProperties[nFX]->hide();
		WindowProperties prop = pPref->getLadspaProperties(nFX);
		m_pLadspaFXProperties[nFX]->move( prop.x, prop.y );
		if ( prop.visible ) {
			m_pLadspaFXProperties[nFX]->show();
		}
		else {
			m_pLadspaFXProperties[nFX]->hide();
		}
	}
#endif

	m_pInstrumentEditor = new InstrumentEditor( 0 );
	WindowProperties instrumentEditorProperties = pPref->getInstrumentEditorProperties();
	m_pInstrumentEditor->move( instrumentEditorProperties.x, instrumentEditorProperties.y );
	if ( instrumentEditorProperties.visible ) {
		m_pInstrumentEditor->show();
	}
	else {
		m_pInstrumentEditor->hide();
	}
}



void HydrogenApp::setupMDIInterface()
{
	Preferences *pPref = Preferences::getInstance();

	// TOOLBAR
	QToolBar * fileTools = new QToolBar( "Transport Toolbar", m_pMainForm, Qt::DockBottom );
	fileTools->setMovingEnabled( false );
	fileTools->setLabel( "Hydrogen control panel" );
	m_pPlayerControl = new PlayerControl( fileTools );

	// create the workspace
	m_pMainForm->workspace = new QWorkspace( m_pMainForm );
	m_pMainForm->workspace->setScrollBarsEnabled( true );
	string mdiBackground_path = Skin::getImagePath() + string( "/mdiBackground.png" );
	m_pMainForm->workspace->setPaletteBackgroundPixmap( QPixmap(mdiBackground_path.c_str()) );

	// restore m_pMainForm properties
	WindowProperties mainFormProp = pPref->getMainFormProperties();
//	m_pMainForm->setMinimumSize( QSize( mainFormProp.width, mainFormProp.height ) );
	m_pMainForm->resize( mainFormProp.width, mainFormProp.height );
	m_pMainForm->move( mainFormProp.x, mainFormProp.y );
	m_pMainForm->setCentralWidget( m_pMainForm->workspace );

	// restore PatternEditorPanel properties
	m_pPatternEditorPanel = new PatternEditorPanel(m_pMainForm->workspace);
	WindowProperties patternEditorProp = pPref->getPatternEditorProperties();
	m_pPatternEditorPanel->resize( patternEditorProp.width, patternEditorProp.height );
	m_pPatternEditorPanel->move( patternEditorProp.x, patternEditorProp.y );
	if ( patternEditorProp.visible ) {
		m_pPatternEditorPanel->show();
	}
	else {
		m_pPatternEditorPanel->hide();
	}

	// m_pMixer test
//	QDockWindow *pMixerDock = new QDockWindow( m_pMainForm, "MixerDockWindow");
//	pMixerDock->setResizeEnabled(true);
//	pMixerDock->setMovingEnabled(false);

	// restore m_pMixer properties
	m_pMixer = new Mixer( m_pMainForm->workspace );
//	m_pMixer = new Mixer( pMixerDock );
	WindowProperties mixerProp = pPref->getMixerProperties();
	m_pMixer->resize( mixerProp.width, mixerProp.height );
	m_pMixer->move( mixerProp.x, mixerProp.y );
	if ( mixerProp.visible ) {
		m_pMixer->show();
	}
	else {
		m_pMixer->hide();
	}
//	pMixerDock->setWidget(m_pMixer);
//	m_pMainForm->moveDockWindow( pMixerDock, Qt::DockBottom );

	// restore song editor properties
	m_pSongEditorPanel = new SongEditorPanel( m_pMainForm->workspace );
	WindowProperties songEditorProp = pPref->getSongEditorProperties();
	m_pSongEditorPanel->resize( songEditorProp.width, songEditorProp.height );
	m_pSongEditorPanel->move( songEditorProp.x, songEditorProp.y );
	if ( songEditorProp.visible ) {
		m_pSongEditorPanel->show();
	}
	else {
		m_pSongEditorPanel->hide();
	}

	// restore drumkit manager properties
	m_pDrumkitManager = new DrumkitManager( m_pMainForm->workspace );
	WindowProperties drumkitMngProp = pPref->getDrumkitManagerProperties();
	m_pDrumkitManager->move( drumkitMngProp.x, drumkitMngProp.y );
	m_pDrumkitManager->hide();

	string sDocPath = string( DataPath::getDataPath() ) + "/doc";
	string sDocURI = sDocPath + "/manual.html";
	m_pHelpBrowser = new SimpleHTMLBrowser( NULL, sDocPath, sDocURI, SimpleHTMLBrowser::MANUAL );

#ifdef LADSPA_SUPPORT
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		WindowProperties prop = pPref->getLadspaProperties(nFX);
		m_pLadspaFXProperties[nFX] = new LadspaFXProperties( m_pMainForm->workspace, nFX );
		m_pLadspaFXProperties[nFX]->move( prop.x, prop.y );
		if ( prop.visible ) {
			m_pLadspaFXProperties[nFX]->show();
		}
		else {
			m_pLadspaFXProperties[nFX]->hide();
		}
	}
#endif

	m_pInstrumentEditor = new InstrumentEditor( m_pMainForm->workspace );
	WindowProperties instrumentEditorProperties = pPref->getInstrumentEditorProperties();
	m_pInstrumentEditor->move( instrumentEditorProperties.x, instrumentEditorProperties.y );
	if ( instrumentEditorProperties.visible ) {
		m_pInstrumentEditor->show();
	}
	else {
		m_pInstrumentEditor->hide();
	}
}



void HydrogenApp::setupSinglePanedInterface()
{
	Preferences *pPref = Preferences::getInstance();

	// MAINFORM
	WindowProperties mainFormProp = pPref->getMainFormProperties();
//	m_pMainForm->setMinimumSize( QSize( 640, 480 ) );
	m_pMainForm->resize( mainFormProp.width, mainFormProp.height );
	m_pMainForm->move( mainFormProp.x, mainFormProp.y );

	// TOOLBAR
	QToolBar * fileTools = new QToolBar( "Transport Toolbar", m_pMainForm, Qt::DockBottom );
	fileTools->setMovingEnabled( false );
	fileTools->setLabel( "Hydrogen control panel" );
	m_pPlayerControl = new PlayerControl( fileTools );


	QSplitter *pSplitter = new QSplitter( m_pMainForm );
	pSplitter->setOrientation( QSplitter::Vertical );
	pSplitter->setOpaqueResize( true );

	m_pMainForm->setCentralWidget( pSplitter );

	// SONG EDITOR
	m_pSongEditorPanel = new SongEditorPanel( pSplitter );
	WindowProperties songEditorProp = pPref->getSongEditorProperties();
	m_pSongEditorPanel->resize( songEditorProp.width, songEditorProp.height );

	// this HBox will contain the Instrument Properties editor and the Pattern editor
	QHBox *pEditorHBox = new QHBox(pSplitter);

	// set the background color for the HBox
	pEditorHBox->setEraseColor( QColor( 58, 62, 72 ) );


	// PATTERN EDITOR
	m_pPatternEditorPanel = new PatternEditorPanel( pEditorHBox );
	WindowProperties patternEditorProp = pPref->getPatternEditorProperties();
	m_pPatternEditorPanel->resize( patternEditorProp.width, patternEditorProp.height );

	// INSTRUMENT EDITOR
	m_pInstrumentEditor = new InstrumentEditor( pEditorHBox );
	WindowProperties instrumentEditorProperties = pPref->getInstrumentEditorProperties();
	if ( instrumentEditorProperties.visible ) {
		m_pInstrumentEditor->show();
	}
	else {
		m_pInstrumentEditor->hide();
	}

	// MIXER
	m_pMixer = new Mixer(0);
	WindowProperties mixerProp = pPref->getMixerProperties();
	m_pMixer->resize( mixerProp.width, mixerProp.height );
	m_pMixer->move( mixerProp.x, mixerProp.y );
	if ( mixerProp.visible ) {
		m_pMixer->show();
	}
	else {
		m_pMixer->hide();
	}


	// DRUMKIT MANAGER
	m_pDrumkitManager = new DrumkitManager( 0 );
	WindowProperties drumkitMngProp = pPref->getDrumkitManagerProperties();
	m_pDrumkitManager->move( drumkitMngProp.x, drumkitMngProp.y );
	if ( drumkitMngProp.visible ) {
		m_pDrumkitManager->show();
	}
	else {
		m_pDrumkitManager->hide();
	}

	// HELP BROWSER
	string sDocPath = string( DataPath::getDataPath() ) + "/doc";
	string sDocURI = sDocPath + "/manual.html";
	m_pHelpBrowser = new SimpleHTMLBrowser( NULL, sDocPath, sDocURI, SimpleHTMLBrowser::MANUAL );

#ifdef LADSPA_SUPPORT
	// LADSPA FX
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX] = new LadspaFXProperties( NULL, nFX );
		m_pLadspaFXProperties[nFX]->hide();
		WindowProperties prop = pPref->getLadspaProperties(nFX);
		m_pLadspaFXProperties[nFX]->move( prop.x, prop.y );
		if ( prop.visible ) {
			m_pLadspaFXProperties[nFX]->show();
		}
		else {
			m_pLadspaFXProperties[nFX]->hide();
		}
	}
#endif
}



void HydrogenApp::setSong(Song* song) {
	//m_pMixer->updateStart(false);

#ifdef LADSPA_SUPPORT
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pLadspaFXProperties[nFX]->hide();
	}
#endif

	Song* oldSong = (Hydrogen::getInstance())->getSong();
	if (oldSong != NULL) {
		(Hydrogen::getInstance())->removeSong();
		delete oldSong;
		oldSong = NULL;
	}

	(Hydrogen::getInstance())->setSong(song);
	( Preferences::getInstance() )->setLastSongFilename( song->getFilename() );

/*	if (m_pMixer->isVisible()) {
		m_pMixer->updateStart(true);
	}*/
	m_pSongEditorPanel->updateAll();

	QString songName( song->m_sName.c_str() );
	m_pMainForm->setCaption( ( "Hydrogen " + QString(VERSION) + QString( " - " ) + songName ) );

	m_pMainForm->updateRecentUsedSongList();

//	QMessageBox::information( m_pMainForm, "Hydrogen", m_pMainForm->m_pQApp->translate("HydrogenApp", "Song Info:     ") + QString("\n\n") + QString( song->getNotes().c_str() ) + QString("\n") );
}



Song *HydrogenApp::getSong() {
	return (Hydrogen::getInstance())->getSong();
}



void HydrogenApp::showMixer(bool show) {
	if (show) {
		m_pMixer->show();
	}
	else {
		m_pMixer->hide();
	}
}



void HydrogenApp::showPreferencesDialog() {
	PreferencesDialog preferencesDialog(m_pMainForm);
	preferencesDialog.exec();
}




void HydrogenApp::setStatusBarMessage(QString msg, int msec) {
	if (msec != 0) {
		m_pStatusBar->message(msg, msec);
	}
	else {
		m_pStatusBar->message(msg, msec);
	}
}





void HydrogenApp::showAudioEngineInfoForm() {
	m_pAudioEngineInfoForm->hide();
	m_pAudioEngineInfoForm->show();
}



void HydrogenApp::showInfoSplash()
{
	QString sDocPath( DataPath::getDataPath().append( "/doc/infoSplash" ).c_str() );

	QDir dir(sDocPath);
	if ( !dir.exists() ) {
		errorLog( string("[showInfoSplash] Directory ").append( sDocPath.ascii() ).append( " not found." ) );
		return;
	}

	string sFilename = "";
	int nNewsID = 0;
	const QFileInfoList *pList = dir.entryInfoList();
	QFileInfoListIterator it( *pList );
	QFileInfo *pFileInfo;

	while ( (pFileInfo = it.current()) != 0 ) {
		string sFile = pFileInfo->fileName().latin1();

		if ( sFile == "." || sFile == ".." ) {
			++it;
			continue;
		}

		int nPos = sFile.rfind("-");
		string sNewsID = sFile.substr( nPos + 1, sFile.length() - nPos - 1 );
		int nID = atoi( sNewsID.c_str() );
		if ( nID > nNewsID ) {
			sFilename = sFile;
		}
//		infoLog( "news: " + sFilename + " id: " + sNewsID );
		++it;
	}
	infoLog( "[showInfoSplash] Selected news: " + sFilename );

	string sLastRead = Preferences::getInstance()->getLastNews();
	if ( sLastRead != sFilename && sFilename != "" ) {
		string sDocURI = sDocPath.ascii();
		sDocURI.append( "/" ).append( sFilename.c_str() );
		SimpleHTMLBrowser *m_pFirstTimeInfo = new SimpleHTMLBrowser( m_pMainForm, sDocPath.ascii(), sDocURI, SimpleHTMLBrowser::WELCOME );
		if ( m_pFirstTimeInfo->exec() == QDialog::Accepted ) {
			Preferences::getInstance()->setLastNews( sFilename );
		}
		else {
		}
	}
}



void HydrogenApp::onEventQueueTimer()
{
	EventQueue *pQueue = EventQueue::getInstance();

	Event event;
	while ( ( event = pQueue->popEvent() ).m_type != EVENT_NONE ) {
		for (int i = 0; i < m_eventListeners.size(); i++ ) {
			EventListener *pListener = m_eventListeners[ i ];

			switch ( event.m_type ) {
				case EVENT_STATE:
					pListener->stateChangedEvent( event.m_nValue );
					break;

				case EVENT_PATTERN_CHANGED:
					pListener->patternChangedEvent();
					break;

				case EVENT_PATTERN_MODIFIED:
					pListener->patternModifiedEvent();
					break;

				case EVENT_SELECTED_PATTERN_CHANGED:
					pListener->selectedPatternChangedEvent();
					break;

				case EVENT_SELECTED_INSTRUMENT_CHANGED:
					pListener->selectedInstrumentChangedEvent();
					break;

				case EVENT_MIDI_ACTIVITY:
					pListener->midiActivityEvent();
					break;

				case EVENT_NOTEON:
					pListener->noteOnEvent( event.m_nValue );
					break;

				case EVENT_ERROR:
					pListener->errorEvent( event.m_nValue );
					break;

				case EVENT_XRUN:
					pListener->XRunEvent();
					break;

				case EVENT_METRONOME:
					pListener->metronomeEvent( event.m_nValue );
					break;

				case EVENT_PROGRESS:
					pListener->progressEvent( event.m_nValue );
					break;

				default:
					errorLog( "[onEventQueueTimer] Unhandled event: " + toString( event.m_type ) );
			}

		}
	}
}


void HydrogenApp::addEventListener( EventListener* pListener )
{
	if (pListener) {
		m_eventListeners.push_back( pListener );
	}
}


void HydrogenApp::removeEventListener( EventListener* pListener )
{
	for ( int i = 0; i < m_eventListeners.size(); i++ ) {
		if ( pListener == m_eventListeners[ i ] ) {
			m_eventListeners.erase( m_eventListeners.begin() + i );
		}
	}
}



