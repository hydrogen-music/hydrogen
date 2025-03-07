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
#include "MainForm.h"

#include <iostream>
#include <cstdint>
#include <vector>
#include <memory>

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
class PlayerControl;
class PatternEditorPanel;
class SongEditor;
class Mixer;
class AudioEngineInfoForm;
class FilesystemInfoForm;
class SimpleHTMLBrowser;
class LadspaFXProperties;
class LadspaFXInfo;
class LadspaFXGroup;
class InstrumentRack;
class PlaylistEditor;
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
		HydrogenApp( MainForm* pMainForm, QUndoStack* pUndoStack );

		/// Returns the instance of HydrogenApp class
		static HydrogenApp* get_instance();

		virtual ~HydrogenApp();

		/**
		 * Specfial routine for song or playlist loading with failure dialog,
		 * which is save to call even in case the GUI is not fully initialized.
		 *
		 * \param sFilename Absolute or relative path used to load the next
		 *   #H2Core::Song or #H2Core::Playlist.
		 * \return bool true on success
		 */
		static bool openFile( const H2Core::Filesystem::Type& type,
							  const QString& sFilename );
		static bool openSong( std::shared_ptr<H2Core::Song> pSong );
		static QString findAutoSaveFile( const H2Core::Filesystem::Type& type,
										 const QString& sBaseFile );

		/** Checks whether there are unsaved changes in the current song (for
		* H2Core::Filesystem::FileType::Song) or playlist (for
		* H2Core::Filesystem::FileType::Playlist).
		*
		* @return `true` if handled, `false` if aborted. */
		static bool handleUnsavedChanges( const H2Core::Filesystem::Type& type );

		void showPreferencesDialog();
		void updateMixerCheckbox();
		void showMixer(bool bShow);
		void showInstrumentRack(bool bShow);
		void showAudioEngineInfoForm();
		void showFilesystemInfoForm();
		void showPlaylistEditor();
		void showDirector();
		void showSampleEditor( const QString& name, int mSelectedComponemt,
							   int mSelectedLayer );

		bool hideKeyboardCursor();
		void setHideKeyboardCursor( bool bHidden );

		AudioEngineInfoForm*		getAudioEngineInfoForm();
		Director*			getDirector();
		Mixer*				getMixer();
		MainForm*			getMainForm();
		SongEditorPanel*		getSongEditorPanel();
		PlaylistEditor*			getPlaylistEditor();
		SampleEditor*			getSampleEditor();
		PatternEditorPanel*		getPatternEditorPanel();
		PlayerControl*			getPlayerControl();
		InstrumentRack*			getInstrumentRack();
	std::shared_ptr<CommonStrings>			getCommonStrings();
		InfoBar *			addInfoBar();

		/** If a non-empty @a sContext is provided, all successive undo commands
		 * and macros using the same context will be enclosed by a single
		 * macro. */
		void pushUndoCommand( QUndoCommand* pCommand, const QString& sContext = "" );
		/** If a non-empty @a sContext is provided, all successive undo commands
		 * and macros using the same context will be enclosed by a single
		 * macro. */
		void beginUndoMacro( const QString& sText, const QString& sContext = "" );
		void endUndoMacro( const QString& sContext = "" );
		/** Ensure #m_sLastUndoContext is set to an empty string and all
		 * additional undo macros are ended. */
		void endUndoContext();

	void showStatusBarMessage( const QString& sMessage, const QString& sCaller = "" );
		void updateWindowTitle();

#ifdef H2CORE_HAVE_LADSPA
		LadspaFXProperties* getLadspaFXProperties( int nFX) {	return m_pLadspaFXProperties[nFX];	}
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
	void changePreferences( const H2Core::Preferences::Changes& changes );
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

private slots:
	void propagatePreferences();

		friend class MainForm;
	private:
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
		PlaylistEditor *			m_pPlaylistEditor;
		SampleEditor *				m_pSampleEditor;
		Director *					m_pDirector;
		QTimer *					m_pEventQueueTimer;
		std::vector<EventListener*> 	m_EventListeners;
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
		void playlistChangedEvent( int nValue ) override;
		void playlistLoadSongEvent() override;

		/** Begins and ends nested macros based on @a sContext.
		 *
		 * The intended way to group and compress undo commands in Qt is by
		 * either enclose individual undo commands in macros or by providing the
		 * derived undo command classes with id() and mergeWidth() methods.
		 *
		 * But both of them does not work with the action design of the
		 * #NotePropertiesRuler. Multiple wheel or keypress events on the same
		 * set of notes should be grouped together. But since we do not know in
		 * advance whether the user will trigger another wheel/keypress event,
		 * we can not enclose them in trivial macros. Merges, on the other hand,
		 * are not feasible either. If an event acts on multiple notes, each
		 * event will result in multiple undo commands. But macros can not be
		 * merged and the design of our undo action to act on single notes is
		 * very clear and easy to maintain. We do not want to break it.
		 *
		 * What we do instead is introducing our own cooked up solution, the
		 * undo context. If an undo command or macro is associated with an
		 * non-empty context string, it will be group with all successive
		 * commands/macros associated with the same context. The undo text is
		 * determined the by the first command/macro introducing a new context
		 * and whenever a command/macro is pushed with a different context, the
		 * former one will be ended cleanly. */
		void handleUndoContext( const QString& sContext, const QString& sText );
		QUndoStack*			m_pUndoStack;
		/** If non-empty, indicates that there is currently an additional undo
		 * macro layer enclosing all new undo macros and commands. See
		 * handleUndoContext(). */
		QString m_sLastUndoContext;
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

inline PlaylistEditor* HydrogenApp::getPlaylistEditor()
{
	return m_pPlaylistEditor;
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
		if ( H2Core::Preferences::get_instance()->hideKeyboardCursor() ) {
			m_bHideKeyboardCursor = true;
		}
	} else {
		m_bHideKeyboardCursor = bHidden;
	}
}

#endif
