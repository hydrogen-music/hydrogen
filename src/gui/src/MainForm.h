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
#include <memory>

#include "EventListener.h"
#include "Widgets/WidgetWithScalableFont.h"

#include <core/config.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

class HydrogenApp;
class InfoBar;
class QUndoView;///debug only

namespace H2Core {
	class Drumkit;
	class Instrument;
}

///
/// Main window
///
/** \ingroup docGUI*/
class MainForm :  public QMainWindow,
				  protected WidgetWithScalableFont<8, 10, 12>,
				  public EventListener,
				  public H2Core::Object<MainForm>
{
		H2_OBJECT(MainForm)
	Q_OBJECT

	public:
		QApplication* m_pQApp;

	MainForm( QApplication * pQApplication, const QString& sSongFilename,
			  const QString& sPlaylistFilename = "" );
		~MainForm();

		void updateMenuBar();
		void updateAutomationPathVisibility();

		virtual void errorEvent( int nErrorCode ) override;
		virtual void updateSongEvent( int nValue ) override;
	virtual void quitEvent( int ) override;

		/** Handles the loading and saving of the H2Core::Preferences
		 * from the core part of H2Core::Hydrogen.
		 *
		 * If \a nValue is
		 *  - `0`: H2Core::Preferences should be saved
		 *  - `1`: H2Core::Preferences has been reloaded. Update its
		 *    representation. */
		virtual void updatePreferencesEvent( int nValue ) override;
		virtual void undoRedoActionEvent( int nEvent ) override;
		static void usr1SignalHandler(int unused);

		/** Due to limitations in `libarchive` we do not support UTF-8 encoding
		 * in drumkit import and export.
		 *
		 * This functions takes care of informing the user via a warning dialog
		 * and indicates success using its return value. */
		static bool checkDrumkitPathEncoding( const QString& sPath,
											  const QString& sContext );

		void setPreviousAutoSavePlaylistFile( const QString& sFile );

		static void exportDrumkit( std::shared_ptr<H2Core::Drumkit> pDrumkit );
		static bool switchDrumkit( std::shared_ptr<H2Core::Drumkit> pTargetKit );

		bool eventFilter( QObject *o, QEvent *e ) override;
		/** @param nInstrumentID If set to a value different than
		 *   #EMPTY_INSTR_ID, the corresponding line in the type tab will be
		 *   selected on startup. */
		static void editDrumkitProperties( bool bWriteToDisk,
										   bool bSaveToNsmSession,
										   int nInstrumentID = EMPTY_INSTR_ID );

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
		 * As Song::m_sFilename is not set by the GUI but by the core, this
		 * function serves both the "save as" functionality (with sNewFilename
		 * being non-empty) and the "save" one.
		 *
		 * Using @a bTriggerMessage the status message triggered by this method
		 * can be suppressed (e.g. when the calling routine wants to trigger a
		 * dedicated message instead).
		 */
		bool action_file_save( const QString& sNewFilename,
							   bool bTriggerMessage = true );
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
		void action_file_exit();

		void action_file_export();
		void action_file_export_midi();
		void action_file_export_lilypond();
		void action_file_songProperties();

		void action_help_about();
		void action_report_bug();
		void action_donate();

		void action_drumkit_new();
		void action_drumkit_properties();
		void action_drumkit_open();
		void action_drumkit_save();
		void action_drumkit_save_to_session();
		void action_drumkit_export();
		/** @param bLoad whether to just import the kit or, in addition, load
		 * the imported kit. */
		void action_drumkit_import( bool bLoad = true );
		void action_drumkit_onlineImport();

		static void action_drumkit_addInstrument(
			std::shared_ptr<H2Core::Instrument> pInstrument = nullptr );
		static void action_drumkit_deleteInstrument( int nInstrumentIndex );
		static void action_drumkit_renameInstrument( int nInstrumentIndex );

		void action_window_showMixer();
		void action_window_showPlaylistEditor();
		void action_window_show_DirectorWidget();
		void action_window_showSongEditor();
		void action_window_showPatternEditor();
		void action_window_showInstrumentRack();
		void action_window_showAutomationArea();
		void action_window_showTimeline();
		void action_window_showPlaybackTrack();
		void action_window_toggleFullscreen();

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

		void action_file_open_recent( QAction *pAction );
		void showDevelWarning();

		void openUndoStack();
		void action_undo();
		void action_redo();

		void action_inputMode_instrument();
		void action_inputMode_drumkit();

		void handleSigUsr1();
		void closeAll();
		/** Stores the current state of the GUI (position, width,
		 * height, and visibility of the widgets) in the
		 * H2Core::Preferences.
		 */
		void saveWindowProperties();
		void checkMidiSetup();
		void checkMissingSamples();

		// Interface for screen grabs
		void setMainWindowSize( int w, int h ) {
			setFixedSize( w, h );
		}
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

	private slots:
		void onAutoSaveTimer();
		void onPlaylistDisplayTimer();
		void onFixMidiSetup();
		void onFixMissingSamples();

	private:
		bool handleUnsavedChangesDuringShutdown();
		void updateRecentUsedSongList();

		void loadDrumkit( const QString& sFileName, bool bLoad );

		HydrogenApp*	h2app;

		static int sigusr1Fd[2];
		QSocketNotifier *snUsr1;

		QMenu *		m_pLogLevelMenu;
		QMenu *		m_pInputModeMenu;
		QAction *	m_pViewPlaylistEditorAction;
		QAction *	m_pViewDirectorAction;
		QAction *	m_pViewMixerAction;
		QAction *	m_pViewInstrumentRackAction;
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

		QUndoView *	m_pUndoView;
		QAction*	m_pUndoAction;
		QAction*	m_pRedoAction;

	void startAutosaveTimer();
		QTimer		m_AutosaveTimer;

		/** Create the menubar */
		void createMenuBar();

		void checkNecessaryDirectories();

		QString getAutoSaveFilename();

		InfoBar *m_pMidiSetupInfoBar;
		InfoBar *m_pMissingSamplesInfoBar;

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
		QMenu* m_pDrumkitMenu;
		QMenu* m_pViewMenu;
		QMenu* m_pOptionsMenu;
		QMenu* m_pDebugMenu;
		QMenu* m_pInfoMenu;

	void openSongWithDialog( const QString& sWindowTitle, const QString& sPath, bool bIsDemo );
	bool prepareSongOpening();

	/** Since the filename of the current song does change whenever
		the users uses "Save As" multiple autosave files would be
		written unless we take care of them.*/
	QString m_sPreviousAutoSaveSongFile;
	QString m_sPreviousAutoSavePlaylistFile;

		/** Whether unsaved changes in the current song and playlist have
		 * already been handled during shutdown. */
		bool m_bUnsavedChangesHandled;

	/**
	 * Maps an incoming @a pKeyEvent to actions via #Shortcuts
	 *
	 * @return Indicates whether or not key event was consumed. If
	 *   not, it will be passed on to other widgets.
	 */
	bool handleKeyEvent( QObject* pQObject, QKeyEvent* pKeyEvent );

	bool nullDriverCheck();
};
inline void MainForm::setPreviousAutoSavePlaylistFile( const QString& sFile ) {
	m_sPreviousAutoSavePlaylistFile = sFile;
}
#endif
