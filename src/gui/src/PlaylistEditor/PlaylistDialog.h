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

#ifndef PLAYLIST_DIALOG_H
#define PLAYLIST_DIALOG_H


#include <QDialog>
#include "ui_PlaylistDialog_UI.h"
#include <hydrogen/object.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/playlist.h>


class Button;
class ToggleButton;
class PixmapWidget;

/**
 * This dialog is used to use the H2PlayList
 *
 * \ingroup docGUI
 */
class PlaylistDialog : public QDialog, public Ui_PlaylistDialog_UI, public H2Core::Object

{
	Q_OBJECT
	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }

		PlaylistDialog( QWidget* pParent );
		~PlaylistDialog();

		bool loadListByFileName( QString filename);


	private slots:
		void keyPressEvent( QKeyEvent* ev );
		void closeEvent( QCloseEvent* ev );
		void addSong();
		void addCurrentSong();
		void removeFromList();
		void removeScript();
		void clearPlaylist();
		void loadList();
		void saveListAs();
		void saveList();
		void loadScript();
		void ffWDBtnClicked(Button* ref);
		void nodePlayBTN( Button* ref );
		void nodeStopBTN( Button* ref );
		void rewindBtnClicked(Button *ref);
		void editScript();
		void newScript();
		void on_m_pPlaylistTree_itemClicked ( QTreeWidgetItem * item, int column );
		void o_upBClicked();
		void o_downBClicked();
		void on_m_pPlaylistTree_itemDoubleClicked ();
		void updateActiveSongNumber();
		bool eventFilter ( QObject *o, QEvent *e );


	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;

		void updatePlayListNode( QString file );
		void updatePlayListVector();
		void setFirstItemCurrent();
		Button *zoom_in_btn;
		QTimer *timer;

		Button *m_pRwdBtn;
		ToggleButton *m_pPlayBtn;
		Button *m_pStopBtn;
		Button *m_pFfwdBtn;
};


#endif
