/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Globals.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/IEngineAccess.h>

#include "EventListener.h"
#include "MainForm.h"

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
	class EditorSession;
	class EventQueue;
	class Hydrogen;
	class Instrument;
	class InstrumentComponent;
	class InstrumentLayer;
	class LocalEngineAccess;
	class Song;
}

class AudioEngineInfoForm;
class CommonStrings;
class ComponentEditor;
class Director;
class FilesystemInfoForm;
class Footer;
class InfoBar;
class InstrumentEditor;
class Rack;
class MainToolBar;
class Mixer;
class PatternEditorPanel;
class PlaylistEditor;
class SampleEditor;
class SongEditorPanel;
class SoundLibraryPanel;

/** \ingroup docGUI*/
class HydrogenApp : public QObject,
					public EventListener,
					public H2Core::Object<HydrogenApp> {
	H2_OBJECT( HydrogenApp )
	Q_OBJECT
   public:
	static constexpr int nMinimumWidth = 1040;

	static QString sMimeDragInstrument;
	static QString sMimeDragPattern;
	static QString sMimeSeparator;
	static QString sMimeSubSeparator;

	HydrogenApp( MainForm* pMainForm, QUndoStack* pUndoStack );

	/// Returns the instance of HydrogenApp class
	static HydrogenApp* get_instance();

	/** @name Engine accessors (ADR 0015/0016)
	 * The GUI reaches its engine instance through HydrogenApp instead of the
	 * Hydrogen/Preferences/EventQueue::get_instance() singletons. In
	 * standalone this is the local engine; ADR 0016 later swaps it for an
	 * IPC proxy in editor mode. @{ */
	H2Core::Hydrogen* getHydrogen() const { return m_pHydrogen; }
	std::shared_ptr<H2Core::Preferences> getPreferences() const;
	H2Core::EventQueue* getEventQueue() const;

	/** Null-safe static engine accessors used throughout the GUI. Before
	 * HydrogenApp has been constructed (early startup: MainForm and the
	 * widgets HydrogenApp itself builds run before its instance pointer is
	 * set) these fall back to the bootstrap handles injected by main() via
	 * setBootstrap(), so construction never dereferences a not-yet-ready app.
	 * Once the app exists they return its held instance. The GUI is
	 * inherently single-instance, so these bootstrap handles are a GUI-local
	 * convenience and not a core singleton (ADR 0015/0016). */
	static H2Core::Hydrogen* pHydrogen();
	static std::shared_ptr<H2Core::Preferences> pPreferences();
	static H2Core::EventQueue* pEventQueue();

	/** The engine-access handle the GUI reads state and issues commands
	 * through (ADR 0016). Standalone: a #H2Core::LocalEngineAccess over the
	 * local engine; editor mode (P5): an IPC proxy. The GUI fans out from
	 * this instead of a concrete #H2Core::Hydrogen so it is agnostic to where
	 * the engine lives. */
	static H2Core::IEngineAccess* pEngine();

	/** True when the GUI connects to a headless engine via IPC (started via
	 * `-c/--connect-via-ipc`, injected through setEditorBootstrap()); false
	 * in standalone. Used to present (remote) headless engine-owned
	 * configuration (audio/MIDI driver, JACK, OSC) read-only (ADR
	 * 0016/0022/0032). */
	static bool isConnectViaIpcMode() { return m_bConnectViaIpcMode; }

	/** Actual connection state of the IPC channel (not the startup flag
	 * #isConnectViaIpcMode). Returns false in standalone mode or when the
	 * channel has been closed (user-initiated disconnect or connection loss). */
	static bool isIpcConnected();

	/** Endpoint of the current (or last) IPC session. Used for reconnection
	 * without re-parsing the CLI. Empty in standalone mode. */
	QString getIpcEndpoint() const { return m_sIpcEndpoint; }

	/** Establishes an IPC connection to @a sEndpoint. Tears down any existing
	 * session first. Shows an error dialog on failure. Only valid in IPC mode
	 * (see #isConnectViaIpcMode). */
	void connectToIpcEngine( const QString& sEndpoint );

	/** Cleanly disconnects from the remote engine. The mirror engine remains
	 * readable; writes silently fail until reconnect. Only valid in IPC mode. */
	void disconnectFromIpcEngine();

	/** Pull the full engine state from the headless engine via IPC requests
	 * and apply it to the mirror: Song, Playlist, selected pattern/instrument,
	 * record state, core Preferences, and SoundLibrary info. Called on
	 * initial startup and on reconnect. */
	void syncViaIpc();

	/** Injects the engine + preferences handles used by the static
	 * accessors during early startup, before HydrogenApp exists. main()
	 * calls this with the loaded Preferences first (engine still null),
	 * then again once the Hydrogen instance has been created. */
	static void setBootstrap(
		H2Core::Hydrogen* pHydrogen,
		std::shared_ptr<H2Core::Preferences> pPreferences
	);

	/** Editor-mode bootstrap (P5, ADR 0016): inject the headless mirror engine
	 * @a pMirror (the GUI reads its live objects) together with surroundign
	 * session. The latter is used to create IPC-backed engine-access handle @a
	 * pEngineAccess (writes are forwarded to the authoritative (remote)
	 * headless engine). Used by main() in place of setBootstrap() when started
	 * with `--connect-via-ipc`; takes ownership of the access handle, and (like
	 * standalone) of the mirror engine. */
	static void setEditorBootstrap(
		H2Core::Hydrogen* pMirror,
		std::unique_ptr<H2Core::EditorSession> pEngineSession,
		std::shared_ptr<H2Core::Preferences> pPreferences
	);
	/** @} */

	virtual ~HydrogenApp();

	/**
	 * Specfial routine for song or playlist loading with failure dialog,
	 * which is save to call even in case the GUI is not fully initialized.
	 *
	 * \param sFileName Absolute or relative path used to load the next
	 *   #H2Core::Song or #H2Core::Playlist.
	 * \return bool true on success
	 */
	static bool openFile(
		const H2Core::Filesystem::Artifact& type,
		const QString& sFileName
	);
	static bool openSong( std::shared_ptr<H2Core::Song> pSong );
	static QString findAutoSaveFile(
		const H2Core::Filesystem::Artifact& type,
		const QString& sBaseFile
	);

	/** Checks whether there are unsaved changes in the current song (for
	 * H2Core::Filesystem::Artifact::Song) or playlist (for
	 * H2Core::Filesystem::Artifact::Playlist).
	 *
	 * @return `true` if handled, `false` if aborted. */
	static bool handleUnsavedChanges( const H2Core::Filesystem::Artifact& type
	);

	void showPreferencesDialog();
	void updateMixerCheckbox();
	void showMixer( bool bShow );
	void showRack( bool bShow );
	void showAudioEngineInfoForm();
	void showFilesystemInfoForm();
	void showPlaylistEditor();
	void showDirector();
	void showSampleEditor(
		std::shared_ptr<H2Core::InstrumentLayer> pLayer,
		std::shared_ptr<H2Core::InstrumentComponent> pComponent,
		std::shared_ptr<H2Core::Instrument> pInstrument
	);

	bool hideKeyboardCursor();
	void setHideKeyboardCursor( bool bHidden );

	AudioEngineInfoForm* getAudioEngineInfoForm() const;
	std::shared_ptr<CommonStrings> getCommonStrings() const;
	ComponentEditor* getComponentEditor() const;
	Director* getDirector() const;
	Footer* getFooter() const;
	InstrumentEditor* getInstrumentEditor() const;
	MainForm* getMainForm() const;
	MainToolBar* getMainToolBar() const;
	Mixer* getMixer() const;
	PatternEditorPanel* getPatternEditorPanel() const;
	PlaylistEditor* getPlaylistEditor() const;
	Rack* getRack() const;
	SampleEditor* getSampleEditor() const;
	SongEditorPanel* getSongEditorPanel() const;
	SoundLibraryPanel* getSoundLibraryPanel() const;

	InfoBar* addInfoBar();

	/** If a non-empty @a sContext is provided, all successive undo commands
	 * and macros using the same context will be enclosed by a single
	 * macro. */
	void
	pushUndoCommand( QUndoCommand* pCommand, const QString& sContext = "" );
	/** If a non-empty @a sContext is provided, all successive undo commands
	 * and macros using the same context will be enclosed by a single
	 * macro. */
	void beginUndoMacro( const QString& sText, const QString& sContext = "" );
	void endUndoMacro( const QString& sContext = "" );
	/** Ensure #m_sLastUndoContext is set to an empty string and all
	 * additional undo macros are ended. */
	void endUndoContext();

	void showStatusBarMessage(
		const QString& sMessage,
		const QString& sCaller = ""
	);
	void updateWindowTitle();

	void addEventListener( EventListener* pListener );
	void removeEventListener( EventListener* pListener );

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

	void setWindowProperties(
		QWidget* pWindow,
		H2Core::WindowProperties& prop,
		unsigned flags = SetAll
	);
	H2Core::WindowProperties getWindowProperties( QWidget* pWindow );

	static bool checkDrumkitLicense( std::shared_ptr<H2Core::Drumkit> pDrumkit
	);

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
	void currentTabChanged( int );

	/** Propagates a change in the Preferences through the GUI.
	 *
	 * Triggered by the PreferencesDialog upon a change of the
	 * underlying options in the Preferences class.
	 */
	void changePreferences( const H2Core::Preferences::Changes& changes );
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

   private slots:
	void propagatePreferences();
	void onIpcConnectionLost();

	friend class MainForm;

   private:
	void updateEventListeners();
	/** Re-fetch AudioDriverInfo from the authoritative engine via IPC and
	 * cache it in the mirror Hydrogen. Called during syncViaIpc() and on
	 * AudioDriverChanged / JackTimebaseStateChanged events (ADR 0029). */
	void refreshCachedAudioDriverInfo();
	/** Re-fetch MidiDriverInfo from the authoritative engine via IPC and
	 * cache it in the mirror Hydrogen. Called during syncViaIpc() and on
	 * MidiDriverChanged events (ADR 0029). */
	void refreshCachedMidiDriverInfo();

	static HydrogenApp* m_pInstance;  ///< HydrogenApp instance

	/** Bootstrap handles for the static accessors during early startup (see
	 * setBootstrap()). GUI-local, single-instance; not a core singleton. */
	static H2Core::Hydrogen* m_pBootstrapHydrogen;
	static std::shared_ptr<H2Core::Preferences> m_pBootstrapPreferences;
	/** GUI-local engine-access handle backing pEngine() (ADR 0016). Standalone:
	 * a LocalEngineAccess over the (single) GUI engine, created by
	 * setBootstrap() once the engine exists. Editor mode (P5): an IPC-backed
	 * IpcEngineAccess injected by setEditorBootstrap(). Nulled when the IPC
	 * connection is lost or torn down; pEngine() then falls back to
	 * #m_pLocalFallback so the GUI stays responsive (ADR 0016). */
	static std::unique_ptr<H2Core::IEngineAccess> m_pEngineAccess;
	/** Local fallback backing pEngine() when the IPC handle is absent
	 * (connection lost or reconnect in progress). Wraps the same mirror engine
	 * the IpcEngineAccess read from; commands apply to the mirror only
	 * (degraded mode — no audio, ADR 0016). Lazily created by pEngine(). */
	static std::unique_ptr<H2Core::IEngineAccess> m_pLocalFallback;
	/** Set once by setEditorBootstrap(); backs isConnectViaIpcMode(). */
	static bool m_bConnectViaIpcMode;
	/** Overall session this HydrogenApp instance was bootstrapped in. Use
	 * to manage the connection to the connected peer. */
	static std::unique_ptr<H2Core::EditorSession> m_pEditorSession;

	/** Endpoint of the current (or last) IPC session, for reconnection. */
	QString m_sIpcEndpoint;
	/** When true, an upcoming IpcChannel::disconnected() signal is expected
	 * (user-initiated disconnect or session teardown) and #onIpcConnectionLost
	 * suppresses the warning dialog. */
	bool m_bIpcDisconnectExpected = false;

	/** Connects the current session's channel #disconnected signal to
	 * #onIpcConnectionLost. Call after every (re)connect. */
	void wireIpcDisconnectSignal();

	/** Used for accessibility reasons to show scroll bars in case Hydrogen
	 * has to be shrunk below its minimum size - magnified using the Qt
	 * scale factor so its efficitive size is below the minimum one. */
	QScrollArea* m_pMainScrollArea;
	QVBoxLayout* m_pMainVBox;
	QTabWidget* m_pTab;
	QSplitter* m_pSplitter;

	MainForm* m_pMainForm;

	AudioEngineInfoForm* m_pAudioEngineInfoForm;
	Director* m_pDirector;
	FilesystemInfoForm* m_pFilesystemInfoForm;
	Footer* m_pFooter;
	Rack* m_pRack;
	Mixer* m_pMixer;
	PatternEditorPanel* m_pPatternEditorPanel;
	MainToolBar* m_pMainToolBar;
	PlaylistEditor* m_pPlaylistEditor;
	SampleEditor* m_pSampleEditor;
	SongEditorPanel* m_pSongEditorPanel;

	QTimer* m_pEventQueueTimer;
	std::vector<EventListener*> m_eventListeners;
	std::set<EventListener*> m_eventListenersToAdd;
	std::set<EventListener*> m_eventListenersToRemove;
	std::shared_ptr<CommonStrings> m_pCommonStrings;

	bool m_bHideKeyboardCursor;
	QTimer* m_pPreferencesUpdateTimer;
	int m_nPreferencesUpdateTimeout;
	H2Core::Preferences::Changes m_bufferedChanges;

	// implement EngineListener interface
	void engineError( uint nErrorCode );

	void setupSinglePanedInterface();

	///////////////////////////// EventListener ////////////////////////////////

	void audioDriverChangedEvent() override;
	void jackTimebaseStateChangedEvent( int nValue ) override;
	void midiDriverChangedEvent() override;
	void playlistChangedEvent( int nValue ) override;
	void playlistLoadSongEvent() override;
	void songIsModifiedEvent() override;

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
	void updatePreferencesEvent( int nValue ) override;
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
	void updateSongEvent( int nValue ) override;
	void XRunEvent() override;

	////////////////////////////////////////////////////////////////////////////

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
	QUndoStack* m_pUndoStack;
	/** The engine instance this GUI drives (ADR 0015/0016). Set at
	 * construction; reached by the GUI via getHydrogen()/getPreferences()/
	 * getEventQueue() instead of the get_instance() shims. */
	H2Core::Hydrogen* m_pHydrogen;
	/** If non-empty, indicates that there is currently an additional undo
	 * macro layer enclosing all new undo macros and commands. See
	 * handleUndoContext(). */
	QString m_sLastUndoContext;
};

/// Return an HydrogenApp m_pInstance
inline HydrogenApp* HydrogenApp::get_instance()
{
	return m_pInstance;
}

inline AudioEngineInfoForm* HydrogenApp::getAudioEngineInfoForm() const
{
	return m_pAudioEngineInfoForm;
}
inline std::shared_ptr<CommonStrings> HydrogenApp::getCommonStrings() const
{
	return m_pCommonStrings;
}
inline Director* HydrogenApp::getDirector() const
{
	return m_pDirector;
}
inline Footer* HydrogenApp::getFooter() const
{
	return m_pFooter;
}
inline MainForm* HydrogenApp::getMainForm() const
{
	return m_pMainForm;
}
inline MainToolBar* HydrogenApp::getMainToolBar() const
{
	return m_pMainToolBar;
}
inline Mixer* HydrogenApp::getMixer() const
{
	return m_pMixer;
}
inline PatternEditorPanel* HydrogenApp::getPatternEditorPanel() const
{
	return m_pPatternEditorPanel;
}
inline PlaylistEditor* HydrogenApp::getPlaylistEditor() const
{
	return m_pPlaylistEditor;
}
inline Rack* HydrogenApp::getRack() const
{
	return m_pRack;
}
inline SampleEditor* HydrogenApp::getSampleEditor() const
{
	return m_pSampleEditor;
}
inline SongEditorPanel* HydrogenApp::getSongEditorPanel() const
{
	return m_pSongEditorPanel;
}

inline bool HydrogenApp::hideKeyboardCursor()
{
	return m_bHideKeyboardCursor;
}

inline void HydrogenApp::setHideKeyboardCursor( bool bHidden )
{
	if ( bHidden && !m_bHideKeyboardCursor ) {
		if ( getPreferences()->getHideKeyboardCursor() ) {
			m_bHideKeyboardCursor = true;
		}
	}
	else {
		m_bHideKeyboardCursor = bHidden;
	}
}

#endif
