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

#ifndef MAINFORM_H
#define MAINFORM_H

#include <QtGui>
#include <QtWidgets>

#include <map>
#include <unistd.h>

#include "EventListener.h"
#include "Widgets/WidgetWithScalableFont.h"

#include <core/config.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

class HydrogenApp;
class QUndoView;///debug only

///
/// Main window
///
/** \ingroup docGUI*/
class MainForm :  public QMainWindow, protected WidgetWithScalableFont<8, 10, 12>, public EventListener,  public H2Core::Object<MainForm>
{
		H2_OBJECT(MainForm)
	Q_OBJECT

	public:
		QApplication* m_pQApp;

	MainForm( QApplication * pQApplication, QString sSongFilename );
		~MainForm();

		virtual void errorEvent( int nErrorCode ) override;
		virtual void jacksessionEvent( int nValue) override;
		virtual void playlistLoadSongEvent(int nIndex) override;
		virtual void updateSongEvent( int nValue ) override;
	virtual void quitEvent( int ) override;

		/** Handles the loading and saving of the H2Core::Preferences
		 * from the core part of H2Core::Hydrogen.
		 *
		 * If \a nValue is 0 - the H2Core::Preferences should be saved
		 * - it triggers savePreferences() to write the state of the
		 * GUI into the H2Core::Preferences instance and write it
		 * subsequentially to disk using
		 * H2Core::Preferences::savePreferences(). If, on the other
		 * hand, \a nValue is 1 and the configuration file has been
		 * reloaded, it gets a fresh version of H2Core::Preferences
		 * and updates #m_pInstrumentAction and #m_pDrumkitAction to
		 * reflect the changes in the configuration.
		 *
		 * \param nValue If 0, H2Core::Preferences was save. If 1, it was
		 *     loaded.
		 */
		virtual void updatePreferencesEvent( int nValue ) override;
		virtual void undoRedoActionEvent( int nEvent ) override;
		static void usr1SignalHandler(int unused);

		bool eventFilter( QObject *o, QEvent *e ) override;

public slots:
		void showPreferencesDialog();
		void showUserManual();

		/**
		 * Project > New handling function.
		 *
		 * Creates an empty Song and set it as the current one.
		 *
		 * When Hydrogen is under session management (NSM) this
		 * function will assume that there is already a Song present
		 * (which is the case). Else it will return without doing
		 * anything. It uses the current Song to assign the it file
		 * path to the empty one since the name provided by the NSM
		 * server must be used or the restart of the session fails.
		 */
		void action_file_new();
		
		/**
		 * Project > Open / Import into Session handling function.
		 *
		 * Opens an existing Song.
		 *
		 * When Hydrogen is under session management (NSM) this
		 * function will assume that there is already a Song present
		 * (which is the case). Else it will return without doing
		 * anything. It opens the chosen file and uses the current
		 * Song to assign its file path to the opened one since the
		 * name provided by the NSM server must be used or the restart
		 * of the session fails.
		 */
		void action_file_open();
		void action_file_openDemo();
	/**
	 * Saves the current song to disk.
	 *
	 * As Song::m_sFilename is not set by the GUI but by the core,
	 * this function serves both the "save as" functionality (with
	 * sNewFilename being non-empty) and the "save" one.
	 */
		bool action_file_save( const QString& sNewFilename );
	bool action_file_save();
		
		/**
		 * Project > Save As / Export from Session handling function.
		 *
		 * Saves the current Song in a different path.
		 *
		 * When Hydrogen is under session management (NSM) this
		 * function will store the Song in the chosen location but
		 * keeps its previous file path associated with it since the
		 * name provided by the NSM server must be used or the restart
		 * of the session fails.
		 */
		bool action_file_save_as();
		void action_file_openPattern();
		void action_file_export_pattern_as( int nPatternRow = -1 );
		bool action_file_exit();

		void action_file_export();
		void action_file_export_midi();
		void action_file_export_lilypond();
		void action_file_songProperties();

		void action_help_about();
		void action_report_bug();
		void action_donate();

		void action_instruments_addInstrument();
		void action_instruments_clearAll();
		void action_instruments_saveLibrary();
		void action_instruments_saveAsLibrary();
		void action_instruments_exportLibrary();
		void action_instruments_importLibrary();
		void action_instruments_onlineImportLibrary();
		void action_instruments_addComponent();

		void functionDeleteInstrument( int nInstrument );

		void action_banks_properties();
		void action_banks_open();
		
		void action_window_showMixer();
		void action_window_showPlaylistDialog();
		void action_window_show_DirectorWidget();
		void action_window_showSongEditor();
		void action_window_showPatternEditor();
		void action_window_showInstrumentRack();
		void action_window_showAutomationArea();
		void action_window_showTimeline();
		void action_window_showPlaybackTrack();
		void action_window_toggleFullscreen();

		void update_mixer_checkbox();
		void update_instrument_checkbox( bool show );
		void update_automation_checkbox();
		void update_playback_track_group();
		void update_director_checkbox();
		void update_playlist_checkbox();

		void action_debug_printObjects();
		void action_debug_showAudioEngineInfo();
		void action_debug_showFilesystemInfo();
		void action_debug_openLogfile();
		

		void action_debug_logLevel_none();
		void action_debug_logLevel_error();
		void action_debug_logLevel_warn();
		void action_debug_logLevel_info();
		void action_debug_logLevel_debug();
		
		
		void closeEvent( QCloseEvent* ev ) override;

		void onPlayStopAccelEvent();
		void onRestartAccelEvent();
		void onBPMPlusAccelEvent();
		void onBPMMinusAccelEvent();

		void action_file_open_recent( QAction *pAction );
		void showDevelWarning();
		void onLashPollTimer();

		void openUndoStack();
		void action_undo();
		void action_redo();
		
		void action_inputMode_instrument();
		void action_inputMode_drumkit();

		void handleSigUsr1();
		/** Wrapper around savePreferences() and quit() method of
			#m_pQApp.*/
		void closeAll();
		/** Stores the current state of the GUI (position, width,
		 * height, and visibility of the widgets) in the
		 * H2Core::Preferences.
		 */
		void savePreferences();
		void checkMidiSetup();
		void checkMissingSamples();

		// Interface for screen grabs
		void setMainWindowSize( int w, int h ) {
			setFixedSize( w, h );
		}
	void onPreferencesChanged( H2Core::Preferences::Changes changes );


	private slots:
		void onAutoSaveTimer();
		void onPlaylistDisplayTimer();
		void onFixMidiSetup();
		void onFixMissingSamples();

	protected:
		// Returns true if handled, false if aborted.
		bool handleUnsavedChanges();

	private:
	void editDrumkitProperties( bool bDrumkitNameLocked );
		void updateRecentUsedSongList();

		HydrogenApp*	h2app;

		static int sigusr1Fd[2];
		QSocketNotifier *snUsr1;

		QMenu *		m_pLogLevelMenu;		
		QMenu *		m_pInputModeMenu;
		QAction *	m_pViewPlaylistEditorAction;
		QAction *	m_pViewDirectorAction;
		QAction *	m_pViewMixerAction;
		QAction *	m_pViewMixerInstrumentRackAction;
		QAction *	m_pViewAutomationPathAction;
		QAction *	m_pViewTimelineAction;
		QAction *	m_pViewPlaybackTrackAction;
		QActionGroup *	m_pViewPlaybackTrackActionGroup;
		QAction *	m_pInstrumentAction;
		QAction *	m_pDrumkitAction;

		QMenu *		m_pRecentFilesMenu;
		QAction *	m_pRecentFileAction0;
		QAction *	m_pRecentFileAction1;
		QAction *	m_pRecentFileAction2;
		QAction *	m_pRecentFileAction3;
		QAction *	m_pRecentFileAction4;

		QUndoView *	m_pUndoView;///debug only

	void startAutosaveTimer();
		QTimer		m_AutosaveTimer;

		/** Create the menubar */
		void createMenuBar();
		
		void checkNecessaryDirectories();

		std::map<int,int>  keycodeInstrumentMap;
		void initKeyInstMap();

		QString getAutoSaveFilename();
	#ifdef H2CORE_HAVE_LASH
		QTimer *lashPollTimer;
	#endif

		InfoBar *m_pMidiSetupInfoBar;
		InfoBar *m_pMissingSamplesInfoBar;

		bool handleSelectNextPrevSongOnPlaylist(int step);

		/**
		 * Relocates to current position of the cursor and starts
		 * playback if the transport isn't rolling yet.
		 *
		 * If triggered while focusing the song editor, the song will
		 * be set to H2Core::Song::SONE_MODE. Similarly,
		 * H2Core::Song::PATTERN_MODE will be activated if triggered
		 * in the pattern editor of note properties ruler.
		 *
		 * \param pObject Used to determine the focused part of the
		 * application.
		 */
		void startPlaybackAtCursor( QObject* pObject );

		QMenu* m_pFileMenu;
		QMenu* m_pUndoMenu;
		QMenu* m_pDrumkitsMenu;
		QMenu* m_pInstrumentsMenu;
		QMenu* m_pViewMenu;
		QMenu* m_pOptionsMenu;
		QMenu* m_pDebugMenu;
		QMenu* m_pInfoMenu;

	void openSongWithDialog( const QString& sWindowTitle, const QString& sPath, bool bIsDemo );
	bool prepareSongOpening();

	/** Since the filename of the current song does change whenever
		the users uses "Save As" multiple autosave files would be
		written unless we take care of them.*/
	QString m_sPreviousAutoSaveFilename;
};

#endif
