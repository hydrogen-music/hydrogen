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

#ifndef SOUND_LIBRARY_PANEL_H
#define SOUND_LIBRARY_PANEL_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QMouseEvent>

#include <vector>

#include <hydrogen/Song.h>
#include <hydrogen/Object.h>
#include <hydrogen/SoundLibrary.h>

class SoundLibraryTree;
class ToggleButton;

class SoundLibraryPanel : public QWidget, private Object
{
	Q_OBJECT
	public:
		SoundLibraryPanel( QWidget *pParent );
		~SoundLibraryPanel();

		void updateDrumkitList();


	private slots:
		void on_DrumkitList_ItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous );
		void on_DrumkitList_itemActivated( QTreeWidgetItem * item, int column );
		void on_DrumkitList_leftClicked( QPoint pos );
		void on_DrumkitList_rightClicked( QPoint pos );
		void on_DrumkitList_mouseMove( QMouseEvent *event );

		void on_drumkitLoadAction();
		void on_drumkitDeleteAction();
		void on_drumkitRenameAction();
		void on_drumkitExportAction();
		void on_instrumentDeleteAction();

	private:
		SoundLibraryTree *m_pSoundLibraryTree;
		//FileBrowser *m_pFileBrowser;

		QPoint m_startDragPosition;
		QMenu* m_pDrumkitMenu;
		QMenu* m_pInstrumentMenu;
		QTreeWidgetItem* m_pSystemDrumkitsItem;
		QTreeWidgetItem* m_pUserDrumkitsItem;
		std::vector<H2Core::Drumkit*> m_systemDrumkitInfoList;
		std::vector<H2Core::Drumkit*> m_userDrumkitInfoList;


};

#endif
