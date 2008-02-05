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


#ifndef DRUMKIT_MANAGER_H
#define DRUMKIT_MANAGER_H
#include <QTimer>
#include <QWidget>
#include <QListWidget>

#include <vector>

#include "ui_DrumkitManager_UI.h"
#include <hydrogen/Object.h>
#include <hydrogen/Song.h>
#include <hydrogen/SoundLibrary.h>

///
/// Drumkit manager
///
class OldDrumkitManager : public QWidget, public Ui_DrumkitManager_UI, public Object
{
	Q_OBJECT

	public:
		OldDrumkitManager( QWidget* parent );
		~OldDrumkitManager();

	private slots:
		void on_loadTab_loadDrumkitBtn_clicked();
		void on_loadTabDrumkitListBox_currentRowChanged(int row);
		void on_saveTab_saveBtn_clicked();
		void on_saveTab_nameTxt_textChanged(QString str);
		void on_importTab_drumkitPathTxt_textChanged(QString str);
		void on_importTab_browseBtn_clicked();
		void on_importTab_importBtn_clicked();
		void on_exportTab_browseBtn_clicked();
		void on_exportTab_drumkitPathTxt_textChanged( QString str );
		void on_exportTab_exportBtn_clicked();
		void on_loadTab_deleteDrumkitBtn_clicked();


	private:
		std::vector<H2Core::Drumkit*> drumkitInfoList;

		void updateDrumkitList();

		void okBtnClicked();


};


#endif

