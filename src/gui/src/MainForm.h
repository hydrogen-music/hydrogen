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

#ifndef MAINFORM_H
#define MAINFORM_H

#include <QtNetwork>
#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

#include <map>
#include <unistd.h>

#include "EventListener.h"

#include <hydrogen/config.h>
#include <hydrogen/object.h>

class HydrogenApp;
class QUndoView;///debug only

///
/// Main window
///
class MainForm : public QMainWindow, public EventListener, public H2Core::Object
{
		H2_OBJECT
	Q_OBJECT

	public:
		QApplication* m_pQApp;

		MainForm( QApplication *app, const QString& songFilename, const bool bLoadSong );
		~MainForm();

		void updateRecentUsedSongList();

		virtual void errorEvent( int nErrorCode );
		virtual void jacksessionEvent( int nValue);
		virtual void playlistLoadSongEvent(int nIndex);
		virtual void undoRedoActionEvent( int nEvent );
		static void usr1SignalHandler(int unused);


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
		void action_file_save();
		
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
		void action_file_save_as();
		void action_file_openPattern();
		void action_file_export_pattern_as();
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

		void action_banks_properties();
		void action_banks_open();

		void action_window_showMixer();
		void action_window_showPlaylistDialog();
		void action_window_show_DirectorWidget();
		void action_window_showSongEditor();
		void action_window_showPatternEditor();
		void action_window_showDrumkitManagerPanel();
		void action_window_showAutomationArea();
		void action_window_showTimeline();
		void action_window_showPlaybackTrack();
		void action_window_toggleFullscreen();

		void update_mixer_checkbox();
		void update_instrument_checkbox( bool show );
		void update_automation_checkbox();
		void update_director_checkbox();
		void update_playlist_checkbox();

		void action_debug_printObjects();
		void action_debug_showAudioEngineInfo();

		void closeEvent( QCloseEvent* ev );

		void onPlayStopAccelEvent();
		void onRestartAccelEvent();
		void onBPMPlusAccelEvent();
		void onBPMMinusAccelEvent();
		void onSaveAsAccelEvent();
		void onSaveAccelEvent();
		void onOpenAccelEvent();

		void action_file_open_recent( QAction *pAction );
		void showDevelWarning();
		void onLashPollTimer();

		void openUndoStack();
		void action_undo();
		void action_redo();

		void action_toggle_input_mode();

		void handleSigUsr1();
		void closeAll();

	private slots:
		void onAutoSaveTimer();
		void onPlaylistDisplayTimer();
		void onFixMidiSetup();

	protected:
		// Returns true if handled, false if aborted.
		bool handleUnsavedChanges();

	private:
		HydrogenApp*	h2app;

		static int sigusr1Fd[2];
		QSocketNotifier *snUsr1;

		void functionDeleteInstrument(int instrument);

		QMenu *		m_pInputModeMenu;
		QAction *	m_pViewPlaylistEditorAction;
		QAction *	m_pViewDirectorAction;
		QAction *	m_pViewMixerAction;
		QAction *	m_pViewMixerInstrumentRackAction;
		QAction *	m_pViewAutomationPathAction;
		QAction *	m_pViewTimelineAction;
		QAction *	m_pViewPlaybackTrackAction;
		QAction *	m_pInstrumentAction;
		QAction *	m_pDrumkitAction;

		QMenu *		m_pRecentFilesMenu;
		QAction *	m_pRecentFileAction0;
		QAction *	m_pRecentFileAction1;
		QAction *	m_pRecentFileAction2;
		QAction *	m_pRecentFileAction3;
		QAction *	m_pRecentFileAction4;

		QUndoView *	m_pUndoView;///debug only

		QTimer		m_AutosaveTimer;

		/** Create the menubar */
		void createMenuBar();

		void openSongFile( const QString& sFilename );
		void checkMidiSetup();

		bool eventFilter( QObject *o, QEvent *e );

		std::map<int,int>  keycodeInstrumentMap;
		void initKeyInstMap();

		QString getAutoSaveFilename();
	#ifdef H2CORE_HAVE_LASH
		QTimer *lashPollTimer;
	#endif


		bool handleSelectNextPrevSongOnPlaylist(int step);

};

#endif
