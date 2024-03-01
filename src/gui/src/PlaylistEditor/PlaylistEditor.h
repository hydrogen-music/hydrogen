/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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


#include <QMenuBar>
#include <QDialog>
#include "ui_PlaylistEditor_UI.h"
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Hydrogen.h>
#include <core/Basics/Playlist.h>
#include "../Widgets/WidgetWithScalableFont.h"
#include "../EventListener.h"

class Button;
class PixmapWidget;

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

		void ffWDBtnClicked();
		void nodePlayBTN();
		void nodeStopBTN();
		void rewindBtnClicked();
		void on_m_pPlaylistTree_itemClicked( QTreeWidgetItem * item, int column );
		void o_upBClicked();
		void o_downBClicked();
		void on_m_pPlaylistTree_itemDoubleClicked();


	private:

		void updatePlaylistTree();
		void loadCurrentItem();
	void populateMenuBar();
	bool handleKeyEvent( QKeyEvent* pKeyEvent );
		Button *	zoom_in_btn;
		QMenuBar *	m_pMenubar;
		QMenu *		m_pPlaylistMenu;
		std::shared_ptr<H2Core::PlaylistEntry> m_pLastSelectedEntry;

		/** Using the up and down buttons a song of the playlist can be moved to
		 * the next or previous position. This variable helps to accompany them
		 * with a nice UX by moving the selection focus along with the item. */
		bool m_bOmitNextSelection;
#ifndef WIN32
	//no scripts under windows
		QMenu *		m_pScriptMenu;
#endif

		Button *	m_pRwdBtn;
		Button *	m_pPlayBtn;
		Button *	m_pStopBtn;
		Button *	m_pFfwdBtn;
};


#endif
