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

#ifndef HYDROGEN_APP_H
#define HYDROGEN_APP_H

#include <core/config.h>
#include <core/Object.h>
#include <core/Globals.h>
#include <core/Preferences/Preferences.h>

#include "EventListener.h"

#include <iostream>
#include <cstdint>
#include <memory>
#include <set>
#include <vector>

#include <QtGui>
#include <QtWidgets>
#include <QStringList>

/** Amount of time to pass between successive calls to
 * HydrogenApp::onEventQueueTimer() in milliseconds.
 *
 * This causes the GUI to update at 20 frames per second.*/
constexpr uint16_t QUEUE_TIMER_PERIOD = 50;


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
class CommonStrings;

/** \ingroup docGUI*/
class HydrogenApp :  public QObject, public EventListener,  public H2Core::Object<HydrogenApp>
{
		H2_OBJECT(HydrogenApp)
	Q_OBJECT
	public:
		HydrogenApp( MainForm* pMainForm );

		/// Returns the instance of HydrogenApp class
		static HydrogenApp* get_instance();

		virtual ~HydrogenApp();

		/** 
		 * \param sFilename Absolute or relative path used to load the next #H2Core::Song.
		 * \return bool true on success
		 */
		static bool openSong( QString sFilename );
		static bool openSong( std::shared_ptr<H2Core::Song> pSong );
	/**
	 * Specialized version of openSong( QString sFilename ) trying to
	 * open the autosave file corresponding to current empty song.
	 *
	 * This will be used if the last set in Hydrogen was an empty one.
	 * If the user either decided to discard the changes or Hydrogen
	 * was terminated untimely, this function allows to restore all
	 * changes that would have been lost otherwise.
	 */
	static bool recoverEmptySong();

		void showPreferencesDialog();
		void updateMixerCheckbox();
		void showMixer(bool bShow);
		void showInstrumentPanel(bool);
		void showAudioEngineInfoForm();
		void showFilesystemInfoForm();
		void showPlaylistDialog();
		void showDirector();
		void showSampleEditor( QString name, int mSelectedComponemt, int mSelectedLayer );

		bool hideKeyboardCursor();
		void setHideKeyboardCursor( bool bHidden );

		Mixer*				getMixer();
		MainForm*			getMainForm();
		SongEditorPanel*		getSongEditorPanel();
		AudioEngineInfoForm*		getAudioEngineInfoForm();
		PlaylistDialog*			getPlayListDialog();
		Director*			getDirector();
		SampleEditor*			getSampleEditor();
		PatternEditorPanel*		getPatternEditorPanel();
		PlayerControl*			getPlayerControl();
		InstrumentRack*			getInstrumentRack();
	std::shared_ptr<CommonStrings>			getCommonStrings();
		InfoBar *			addInfoBar();

		QUndoStack*			m_pUndoStack;

	void showStatusBarMessage( const QString& sMessage, const QString& sCaller = "" );
		void updateWindowTitle();

#ifdef H2CORE_HAVE_LADSPA
		LadspaFXProperties* getLadspaFXProperties(uint nFX) {	return m_pLadspaFXProperties[nFX];	}
#endif
		void addEventListener( EventListener* pListener );
		void removeEventListener( EventListener* pListener );
		void closeFXProperties();

		void cleanupTemporaryFiles();

		enum SetPropertyFlag {
			SetX = 1 << 0,
			SetY = 1 << 1,
			SetWidth = 1 << 2,
			SetHeight = 1 << 3,
			SetVisible = 1 << 4,
			SetAll = SetX + SetY + SetWidth + SetHeight + SetVisible,
			SetDefault = SetAll
		};

		void setWindowProperties( QWidget *pWindow, H2Core::WindowProperties &prop, unsigned flags = SetAll );
		H2Core::WindowProperties getWindowProperties( QWidget *pWindow );

	static bool checkDrumkitLicense( std::shared_ptr<H2Core::Drumkit> pDrumkit );

signals:
	/** Propagates a change in the Preferences through the GUI.
	 *
	 * Triggered by the PreferencesDialog upon a change of the
	 * underlying options in the Preferences class.
	 *
	 * @param changes Or-able options indicating which part of the
	 * Preferences did change.*/
	void preferencesChanged( H2Core::Preferences::Changes changes );

	public slots:
		/**
		 * Function called every #QUEUE_TIMER_PERIOD
		 * millisecond to pop all Events from the EventQueue
		 * and invoke the corresponding functions.
		 *
		 * In addition, all MIDI notes in
		 * H2Core::EventQueue::m_addMidiNoteVector will converted into
		 * actions via SE_addNoteAction() and deleted from the
		 * former array.
		*/
		void onEventQueueTimer();
		void currentTabChanged(int);

	/** Propagates a change in the Preferences through the GUI.
	 *
	 * Triggered by the PreferencesDialog upon a change of the
	 * underlying options in the Preferences class.
	 */
	void changePreferences( H2Core::Preferences::Changes changes );
	void onPreferencesChanged( H2Core::Preferences::Changes changes );

	private slots:
		void propagatePreferences();

	private:
		void updateEventListeners();

		static HydrogenApp *		m_pInstance;	///< HydrogenApp instance

#ifdef H2CORE_HAVE_LADSPA
		LadspaFXProperties *		m_pLadspaFXProperties[MAX_FX];
#endif

		/** Used for accessibility reasons to show scroll bars in case Hydrogen
		 * has to be shrunk below its minimum size - magnified using the Qt
		 * scale factor so its efficitive size is below the minimum one. */
		QScrollArea*				m_pMainScrollArea;
		MainForm *					m_pMainForm;
		Mixer *						m_pMixer;
		PatternEditorPanel*			m_pPatternEditorPanel;
		AudioEngineInfoForm *		m_pAudioEngineInfoForm;
		FilesystemInfoForm *		m_pFilesystemInfoForm;
		SongEditorPanel *			m_pSongEditorPanel;
		InstrumentRack*				m_pInstrumentRack;
		PlayerControl *				m_pPlayerControl;
		PlaylistDialog *			m_pPlaylistDialog;
		SampleEditor *				m_pSampleEditor;
		Director *					m_pDirector;
		QTimer *					m_pEventQueueTimer;
		std::vector<EventListener*> m_eventListeners;
		std::set<EventListener*> m_eventListenersToAdd;
		std::set<EventListener*> m_eventListenersToRemove;
		QTabWidget *				m_pTab;
		QSplitter *					m_pSplitter;
		QVBoxLayout *				m_pMainVBox;
		std::shared_ptr<CommonStrings>				m_pCommonStrings;

		bool						m_bHideKeyboardCursor;
		QTimer *					m_pPreferencesUpdateTimer;
		int						    m_nPreferencesUpdateTimeout;
		H2Core::Preferences::Changes m_bufferedChanges;

		// implement EngineListener interface
		void engineError(uint nErrorCode);

		void setupSinglePanedInterface();
		virtual void songModifiedEvent() override;
		virtual void XRunEvent() override;

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
		virtual void updatePreferencesEvent( int nValue ) override;
		/**
		 * Refreshes and updates the GUI after the Song was changed in
		 * the core part of Hydrogen.
		 *
		 * When using session management or changing the Song using
		 * an OSC message, this command will get core and GUI in sync
		 * again. 
		 *
		 * \param nValue If 0, update the GUI to represent the new song. If
		 *     1, a message in the status bar will be displayed
		 *     notifying the user about the saving of the current
		 *     Song. If 2, notifies the user that the current song is
		 *     opened in read-only mode.
		 */
		virtual void updateSongEvent( int nValue ) override;
	virtual void drumkitLoadedEvent() override;
	
};


/// Return an HydrogenApp m_pInstance
inline HydrogenApp* HydrogenApp::get_instance() {
	return m_pInstance;
}

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

inline std::shared_ptr<CommonStrings> HydrogenApp::getCommonStrings()
{
	return m_pCommonStrings;
}

inline bool HydrogenApp::hideKeyboardCursor()
{
	return m_bHideKeyboardCursor;
}

inline void HydrogenApp::setHideKeyboardCursor( bool bHidden )
{
	if ( bHidden && ! m_bHideKeyboardCursor ) {
		H2Core::Preferences *pPref = H2Core::Preferences::get_instance();
		if ( pPref->hideKeyboardCursor() ) {
			m_bHideKeyboardCursor = true;
		}
	} else {
		m_bHideKeyboardCursor = bHidden;
	}
}


#endif
