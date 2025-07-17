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

#ifndef PLAYLIST_EDITOR_H
#define PLAYLIST_EDITOR_H

#include <QtGui>
#include <QtWidgets>

#include <memory>
#include <vector>

#include <core/Basics/Playlist.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "../EventListener.h"
#include "../Widgets/WidgetWithScalableFont.h"

class MidiLearnableToolButton;

class PlaylistTableWidget : public QTableWidget,
							public H2Core::Object<PlaylistTableWidget> {
	H2_OBJECT(PlaylistTableWidget)
	Q_OBJECT
	public:
		explicit PlaylistTableWidget( QWidget* pParent );
		void update();
		void loadCurrentRow();

	private:
		virtual void dragEnterEvent( QDragEnterEvent* pEvent ) override;
		virtual void dropEvent( QDropEvent* pEvent ) override;
		virtual void mousePressEvent( QMouseEvent* pEvent ) override;
		virtual void mouseMoveEvent( QMouseEvent* pEvent ) override;
		virtual void mouseDoubleClickEvent( QMouseEvent* pEvent ) override;

		QPoint m_dragStartPosition;
		std::shared_ptr<H2Core::PlaylistEntry> m_pLastSelectedEntry;
};

#include "ui_PlaylistEditor_UI.h"

///
/// This dialog is used to use the H2PlayList
///
/** \ingroup docGUI*/
class PlaylistEditor :  public QDialog,
						public EventListener,
						protected WidgetWithScalableFont<8, 10, 12>,
						public Ui_PlaylistEditor_UI,
						public H2Core::Object<PlaylistEditor>

{
		H2_OBJECT(PlaylistEditor)
	Q_OBJECT
	public:

		explicit PlaylistEditor( QWidget* pParent );
		~PlaylistEditor();

		void playlistChangedEvent( int nValue ) override;
		void playlistLoadSongEvent() override;
		void moveRow( int nFrom, int nTo );

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );
		void addSong();
		void addCurrentSong();
		void removeSong();
		void removeScript();
		void newPlaylist();
		void openPlaylist();
		bool savePlaylistAs();
		bool savePlaylist();
		void loadScript();
		void editScript();
		void newScript();

	private slots:
		virtual void closeEvent( QCloseEvent* ev ) override;
		virtual bool eventFilter( QObject *o, QEvent *e ) override;

		void playButtonClicked();
		void on_m_pPlaylistTable_itemClicked( QTableWidgetItem* item );
		void o_upBClicked();
		void o_downBClicked();
		void updateMenuActivation();

		void undo();
		void redo();
		void showUndoHistory();


	private:

		void update();
		void updateIcons();
		void updateStyleSheet();
		void updateWindowTitle();
	void populateMenuBar();
	bool handleKeyEvent( QKeyEvent* pKeyEvent );

		/** Holds all actions which only have meaning when acting on a selected
		 * row. They are disabled in case none is selected. */
		std::vector<QAction*> m_actionsSelected;

		/** Holds all actions which only have meaning when acting on a
		 * selected row containing a script.*/
		std::vector<QAction*> m_actionsSelectedScript;

		QMenuBar *	m_pMenubar;
		QMenu *		m_pPlaylistMenu;
		QMenu* m_pUndoMenu;
#ifndef WIN32
	//no scripts under windows
		QMenu *		m_pScriptMenu;
#endif
		QToolBar* m_pToolBar;

		MidiLearnableToolButton*	m_pRwdButton;
		MidiLearnableToolButton*	m_pPlayButton;
		MidiLearnableToolButton*	m_pStopButton;
		MidiLearnableToolButton*	m_pFfwdButton;

		QUndoStack* m_pUndoStack;
		QUndoView* m_pUndoView;
};

#endif
