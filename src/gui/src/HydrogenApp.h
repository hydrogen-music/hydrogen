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

#ifndef HYDROGEN_APP_H
#define HYDROGEN_APP_H

#include <core/config.h>
#include <core/Object.h>
#include <core/Globals.h>

#include "EventListener.h"

#include <iostream>
#include <vector>
#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif
 #include <QStringList>

//#include <QUndoStack>

/** Amount of time to pass between successive calls to
 * HydrogenApp::onEventQueueTimer() in milliseconds.
 *
 * This causes the GUI to update at 20 frames per second.*/
#define QUEUE_TIMER_PERIOD 50


namespace H2Core
{
	class Song;
}

class SongEditorPanel;
class MainForm;
class PlayerControl;
class PatternEditorPanel;
class InstrumentEditorPanel;
class SongEditor;
class Mixer;
class AudioEngineInfoForm;
class FilesystemInfoForm;
class SimpleHTMLBrowser;
class LadspaFXProperties;
class LadspaFXInfo;
class LadspaFXGroup;
class InstrumentRack;
class PlaylistDialog;
class SampleEditor;
class Director;
class InfoBar;

class HydrogenApp : public QObject, public EventListener, public H2Core::Object
{
		H2_OBJECT
	Q_OBJECT
	public:
		HydrogenApp( MainForm* pMainForm, H2Core::Song *pFirstSong );

		/// Returns the instance of HydrogenApp class
		static HydrogenApp* get_instance();

		virtual ~HydrogenApp();

		/** 
		 * \param sFilename Absolute path used to load the next Song.
		 * \return bool true on success
		 */
		bool openSong( const QString sFilename );
		bool openSong( H2Core::Song* pSong );

		void showPreferencesDialog();
		void updateMixerCheckbox();
		void showMixer(bool bShow);
		void showInstrumentPanel(bool);
		void showAudioEngineInfoForm();
		void showFilesystemInfoForm();
		void showPlaylistDialog();
		void showDirector();
		void showSampleEditor( QString name, int mSelectedComponemt, int mSelectedLayer );

		Mixer*				getMixer();
		MainForm*			getMainForm();
		SongEditorPanel*		getSongEditorPanel();
		AudioEngineInfoForm*		getAudioEngineInfoForm();
		PlaylistDialog*			getPlayListDialog();
		Director*			getDirector();
		SampleEditor*			getSampleEditor();
		SimpleHTMLBrowser*		getHelpBrowser();
		PatternEditorPanel*		getPatternEditorPanel();
		PlayerControl*			getPlayerControl();
		InstrumentRack*			getInstrumentRack();
		InfoBar *			addInfoBar();

		QUndoStack*			m_pUndoStack;

		void setStatusBarMessage( const QString& msg, int msec = 0 );
		void setScrollStatusBarMessage( const QString& msg, int msec = 0, bool test = true );
		void updateWindowTitle();

#ifdef H2CORE_HAVE_LADSPA
		LadspaFXProperties* getLadspaFXProperties(uint nFX) {	return m_pLadspaFXProperties[nFX];	}
#endif
		void addEventListener( EventListener* pListener );
		void removeEventListener( EventListener* pListener );
		void closeFXProperties();

		void onDrumkitLoad( QString name );
		void enableDestructiveRecMode();

		void cleanupTemporaryFiles();

	public slots:
		/**
		 * Function called every #QUEUE_TIMER_PERIOD
		 * millisecond to pop all Events from the EventQueue
		 * and invoke the corresponding functions.
		 *
		 * Depending on the H2Core::EventType, the following members
		 * of EventListener will be called:
		 * - H2Core::EVENT_STATE -> 
		     EventListener::stateChangedEvent()
		 * - H2Core::EVENT_PATTERN_CHANGED -> 
		     EventListener::patternChangedEvent()
		 * - H2Core::EVENT_PATTERN_MODIFIED -> 
		     EventListener::patternModifiedEvent()
		 * - H2Core::EVENT_SONG_MODIFIED -> 
		     EventListener::songModifiedEvent()
		 * - H2Core::EVENT_SELECTED_PATTERN_CHANGED -> 
		     EventListener::selectedPatternChangedEvent()
		 * - H2Core::EVENT_SELECTED_INSTRUMENT_CHANGED -> 
		     EventListener::selectedInstrumentChangedEvent()
		 * - H2Core::EVENT_PARAMETERS_INSTRUMENT_CHANGED -> 
		     EventListener::parametersInstrumentChangedEvent()
		 * - H2Core::EVENT_MIDI_ACTIVITY -> 
		     EventListener::midiActivityEvent()
		 * - H2Core::EVENT_NOTEON -> 
		     EventListener::noteOnEvent()
		 * - H2Core::EVENT_ERROR -> 
		     EventListener::errorEvent()
		 * - H2Core::EVENT_XRUN -> 
		     EventListener::XRunEvent()
		 * - H2Core::EVENT_METRONOME -> 
		     EventListener::metronomeEvent()
		 * - H2Core::EVENT_RECALCULATERUBBERBAND -> 
		     EventListener::rubberbandbpmchangeEvent()
		 * - H2Core::EVENT_PROGRESS -> 
		     EventListener::progressEvent()
		 * - H2Core::EVENT_JACK_SESSION -> 
		     EventListener::jacksessionEvent()
		 * - H2Core::EVENT_PLAYLIST_LOADSONG -> 
		     EventListener::playlistLoadSongEvent()
		 * - H2Core::EVENT_UNDO_REDO -> 
		     EventListener::undoRedoActionEvent()
		 * - H2Core::EVENT_TEMPO_CHANGED -> 
		     EventListener::tempoChangedEvent()
		 * - H2Core::EVENT_UPDATE_PREFERENCES -> 
		     EventListener::updatePreferencesEvent()
		 * - H2Core::EVENT_UPDATE_SONG -> 
		     EventListener::updateSongEvent()
		 * - H2Core::EVENT_NONE -> nothing
		 *
		 * In addition, all MIDI notes in
		 * H2Core::EventQueue::m_addMidiNoteVector will converted into
		 * actions via SE_addNoteAction() and deleted from the
		 * former array.
		*/
		void onEventQueueTimer();
		void currentTabChanged(int);

	private:
		static HydrogenApp *		m_pInstance;	///< HydrogenApp instance

#ifdef H2CORE_HAVE_LADSPA
		LadspaFXProperties *		m_pLadspaFXProperties[MAX_FX];
#endif

		MainForm *					m_pMainForm;
		Mixer *						m_pMixer;
		PatternEditorPanel*			m_pPatternEditorPanel;
		AudioEngineInfoForm *		m_pAudioEngineInfoForm;
		FilesystemInfoForm *		m_pFilesystemInfoForm;
		SongEditorPanel *			m_pSongEditorPanel;
		SimpleHTMLBrowser *			m_pHelpBrowser;
		SimpleHTMLBrowser *			m_pFirstTimeInfo;
		InstrumentRack*				m_pInstrumentRack;
		PlayerControl *				m_pPlayerControl;
		PlaylistDialog *			m_pPlaylistDialog;
		SampleEditor *				m_pSampleEditor;
		Director *					m_pDirector;
		QTimer *					m_pEventQueueTimer;
		std::vector<EventListener*> 	m_EventListeners;
		QTabWidget *				m_pTab;
		QSplitter *					m_pSplitter;
		QVBoxLayout *				m_pMainVBox;

		// implement EngineListener interface
		void engineError(uint nErrorCode);

		void setupSinglePanedInterface();
		virtual void songModifiedEvent();

		/** Handles the loading and saving of the H2Core::Preferences
		 * from the core part of H2Core::Hydrogen.
		 *
		 * If \a nValue is 0 - the H2Core::Preferences got saved - it
		 * triggers the display of a message in the status bar. If, on
		 * the other hand, \a nValue is 1 and the configuration file
		 * has been reloaded, it gets a fresh version of
		 * H2Core::Preferences and updates all widgets and setting to
		 * reflect the changes in the configuration.
		 *
		 * \param nValue If 0, Preferences was save. If 1, it was
		 *     loaded.
		 */
		virtual void updatePreferencesEvent( int nValue );
		/**
		 * Refreshes and updates the GUI after the Song was changed in
		 * the core part of Hydrogen.
		 *
		 * When using session management or changing the Song using
		 * an OSC message, this command will get core and GUI in sync
		 * again. 
		 *
		 * \param nValue If 0 or 1, loads the Song stored in
		 *     H2Core::Hydrogen::m_pNextSong. If 1, also restarts the
		 *     audio driver via H2Core::Hydrogen::restartDrivers(). If
		 *     2, a message in the status bar will be displayed
		 *     notifying the user about the saving of the current Song
		 *     to the config file.
		 */
		virtual void updateSongEvent( int nValue );
		/**
		 * Calls closeAll() to shutdown Hydrogen.
		 *
		 * \param nValue unused
		 */
		virtual void quitEvent( int nValue );
	
};


inline Mixer* HydrogenApp::getMixer()
{
	return m_pMixer;	
}

inline MainForm* HydrogenApp::getMainForm()
{	
	return m_pMainForm;	
}

inline SongEditorPanel* HydrogenApp::getSongEditorPanel()
{
	return m_pSongEditorPanel;
}

inline AudioEngineInfoForm* HydrogenApp::getAudioEngineInfoForm()
{
	return m_pAudioEngineInfoForm;
}

inline PlaylistDialog* HydrogenApp::getPlayListDialog()
{
	return m_pPlaylistDialog;
}

inline Director* HydrogenApp::getDirector()
{
	return m_pDirector;
}

inline SampleEditor* HydrogenApp::getSampleEditor()
{
	return m_pSampleEditor;	
}

inline SimpleHTMLBrowser* HydrogenApp::getHelpBrowser()
{
	return m_pHelpBrowser;
}

inline PatternEditorPanel* HydrogenApp::getPatternEditorPanel()
{
	return m_pPatternEditorPanel;
}

inline PlayerControl* HydrogenApp::getPlayerControl()
{
	return m_pPlayerControl;
}

inline InstrumentRack* HydrogenApp::getInstrumentRack()
{
	return m_pInstrumentRack;
}

#endif
