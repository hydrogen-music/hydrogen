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

#ifndef PLAYLIST_DIALOG_H
#define PLAYLIST_DIALOG_H


#include <QMenuBar>
#include <QDialog>
#include "ui_PlaylistDialog_UI.h"
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Hydrogen.h>
#include <core/Basics/Playlist.h>
#include "../Widgets/WidgetWithScalableFont.h"

class Button;
class PixmapWidget;

///
/// This dialog is used to use the H2PlayList
///
/** \ingroup docGUI*/
class PlaylistDialog :  public QDialog, protected WidgetWithScalableFont<8, 10, 12>, public Ui_PlaylistDialog_UI,  public H2Core::Object<PlaylistDialog>

{
		H2_OBJECT(PlaylistDialog)
	Q_OBJECT
	public:

		explicit PlaylistDialog( QWidget* pParent );
		~PlaylistDialog();

		bool loadListByFileName( QString filename);

public slots:
	void onPreferencesChanged( H2Core::Preferences::Changes changes );

	private slots:
		virtual void keyPressEvent( QKeyEvent* ev ) override;
		virtual void closeEvent( QCloseEvent* ev ) override;
		virtual bool eventFilter ( QObject *o, QEvent *e ) override;
	
		void addSong();
		void addCurrentSong();
		void removeFromList();
		void removeScript();
		void clearPlaylist();
		void loadList();
		void saveListAs();
		void saveList();
		void loadScript();
		void ffWDBtnClicked();
		void nodePlayBTN();
		void nodeStopBTN();
		void rewindBtnClicked();
		void editScript();
		void newScript();
		void on_m_pPlaylistTree_itemClicked ( QTreeWidgetItem * item, int column );
		void o_upBClicked();
		void o_downBClicked();
		void on_m_pPlaylistTree_itemDoubleClicked ();
		void updateActiveSongNumber();


	private:

		void updatePlayListNode( QString file );
		void updatePlayListVector();
		void setFirstItemCurrent();
		Button *	zoom_in_btn;
		QTimer *	m_pTimer;
		QMenuBar *	m_pMenubar;
		QMenu *		m_pPlaylistMenu;
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
