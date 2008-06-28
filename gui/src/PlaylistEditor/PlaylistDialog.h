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

#include "config.h"

#include <QDialog>
#include "ui_PlaylistDialog_UI.h"
#include <hydrogen/Object.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/playlist.h>


//#include <vector>
class Button;

///
/// This dialog is used to use the H2PlayList
///
class PlaylistDialog : public QDialog, public Ui_PlaylistDialog_UI, public Object

{
	Q_OBJECT
	public:
		
		PlaylistDialog( QWidget* pParent );
		~PlaylistDialog();
	


	private slots:
		void on_addSongBTN_clicked();
		void on_removeFromListBTN_clicked();
		void on_removeScriptBTN_clicked();
		void on_loadListBTN_clicked();
		void on_saveListBTN_clicked();
		void on_loadScriptBTN_clicked();
		void on_nodePlayBTN_clicked();
		void on_nodeStopBTN_clicked();
		void on_editScriptBTN_clicked();
		void on_m_pPlaylistTree_itemClicked ( QTreeWidgetItem * item, int column );
		void on_useMidicheckBox_clicked();
		void on_upBTN_clicked();
		void on_downBTN_clicked();
		void on_m_pPlaylistTree_itemDoubleClicked ();
		void updateActiveSongNumber();


	private:
//		std::vector<HPlayList> m_PlayList;
//		H2Core::Preferences *pPref;

		void updatePlayListNode( QString file );
		void updatePlayListVector();
		void setFirstItemCurrent();
		Button *zoom_in_btn;
};


#endif
