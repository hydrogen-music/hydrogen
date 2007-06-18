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
 * $Id: MainForm.h,v 1.16 2005/06/17 15:10:43 comix Exp $
 *
 */


#ifndef MAINFORM_H
#define MAINFORM_H


#include <qapplication.h>
#include <qmainwindow.h>
#include <qaction.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qstatusbar.h>
#include <qtoolbutton.h>
#include <qworkspace.h>
#include <qvbox.h>
#include <qhttp.h>

#include <map>
using namespace std;

#include "HydrogenApp.h"
#include "EventListener.h"

#include "lib/Object.h"


class HydrogenApp;

///
/// Main window
///
class MainForm : public QMainWindow, public EventListener, public Object
{
	Q_OBJECT

	public:
		QWorkspace *workspace;
		QApplication* m_pQApp;

		MainForm( QApplication *app,  string songFilename );
		~MainForm();

		void updateRecentUsedSongList();

		virtual void errorEvent( int nErrorCode );

	public slots:
		void action_file_new();
		void action_file_open();
		void action_file_openDemo();
		void action_file_save();
		void action_file_save_as();
		void action_file_exit();
		void action_file_preferences();
		void action_file_export();
		void action_file_export_midi();
		void action_file_songProperties();

		void action_help_about();
		void action_help_manual();

		void action_window_showMixer();
		void action_window_showSongEditor();
		void action_window_showPatternEditor();
		void action_window_showDrumkitManager();
		void action_window_showInstrumentEditor();

		void action_debug_printObjects();
		void action_debug_debugCommand();
		void action_debug_showAudioEngineInfo();

		void closeEvent( QCloseEvent* ev );

		void onPlayStopAccelEvent();
		void onRestartAccelEvent();
		void onBPMPlusAccelEvent();
		void onBPMMinusAccelEvent();
		void onSaveAsAccelEvent();
		void onSaveAccelEvent();
		void onOpenAccelEvent();
		void onTapTempoAccelEvent();

		void action_file_open_recent0();
		void action_file_open_recent1();
		void action_file_open_recent2();
		void action_file_open_recent3();
		void action_file_open_recent4();
		
		void latestVersionDone(bool bError);

	private:
		static const int topLevel_width = 250;
		static const int topLevel_height= 400;

		HydrogenApp* h2app;

		QStatusBar *statusBar;

		QMenuBar *m_pMenubar;
		QPopupMenu *m_pFilePopupMenu;
		QPopupMenu *m_pHelpPopupMenu;
		QPopupMenu *m_pWindowPopupMenu;
		QPopupMenu *m_pRecentFilesPopupMenu;
		QPopupMenu *m_pDebugPopupMenu;

		QAction* menuItem_file_open;
		QAction* menuItem_file_openDemo;
		QAction* menuItem_file_quit;
		QAction* menuItem_file_new;
		QAction* menuItem_file_save;
		QAction* menuItem_file_save_as;
		QAction* menuItem_file_export;
		QAction* menuItem_file_export_midi;
		QAction* menuItem_file_preferences;
		QAction* menuItem_file_songProperties;

		QAction *m_pRecentFileAction0;
		QAction *m_pRecentFileAction1;
		QAction *m_pRecentFileAction2;
		QAction *m_pRecentFileAction3;
		QAction *m_pRecentFileAction4;

		QAction* menuItem_help_about;
		QAction* menuItem_help_manual;

		QAction* menuItem_window_showMixer;
		QAction* menuItem_window_showSongEditor;
		QAction* menuItem_window_showInstrumentEditor;
		QAction* menuItem_window_showPatternEditor;
		QAction* menuItem_window_showDrumkitManager;

		QAction* menuItem_debug_printObjects;
		QAction* menuItem_debug_debugCommand;
		QAction* menuItem_debug_showAudioEngineInfo;

		QHttp m_http;
		
		/** Create the menubar */
		void createMenuBar();

		void closeAll();
		void openSongFile( string sFilename );

		bool eventFilter( QObject *o, QEvent *e );

		map<int,int>  keycodeInstrumentMap;
		void initKeyInstMap();
		
		void getLatestVersion();

};

#endif

