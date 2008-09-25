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

#include "config.h"

#include <QtGui>

#include <vector>

#include <hydrogen/Object.h>

namespace H2Core
{
	class Song;
	class Drumkit;
	class SoundLibrary;
}

class SoundLibraryTree;
class ToggleButton;

class SoundLibraryPanel : public QWidget, private Object
{
Q_OBJECT
public:
	SoundLibraryPanel( QWidget* parent );
	~SoundLibraryPanel();

	void updateDrumkitList();
	void test_expandedItems();
	void update_background_color();

private slots:
	void on_DrumkitList_ItemChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous );
	void on_DrumkitList_itemActivated( QTreeWidgetItem* item, int column );
	void on_DrumkitList_leftClicked( QPoint pos );
	void on_DrumkitList_rightClicked( QPoint pos );
	void on_DrumkitList_mouseMove( QMouseEvent* event );

	void on_drumkitLoadAction();
	void on_drumkitDeleteAction();
	void on_drumkitPropertiesAction();
	void on_drumkitExportAction();
	void on_instrumentDeleteAction();
	void on_songLoadAction();
	void on_patternLoadAction();
	void on_patternDeleteAction();

private:
	SoundLibraryTree *__sound_library_tree;
	//FileBrowser *m_pFileBrowser;

	QPoint __start_drag_position;
	QMenu* __drumkit_menu;
	QMenu* __instrument_menu;
	QMenu* __song_menu;
	QMenu* __pattern_menu;
	QMenu* __pattern_menu_list;

	QTreeWidgetItem* __system_drumkits_item;
	QTreeWidgetItem* __user_drumkits_item;
	QTreeWidgetItem* __song_item;
	QTreeWidgetItem* __pattern_item;
	QTreeWidgetItem* __pattern_item_list;

	std::vector<H2Core::Drumkit*> __system_drumkit_info_list;
	std::vector<H2Core::Drumkit*> __user_drumkit_info_list;
	bool __expand_pattern_list;
	bool __expand_songs_list;
	void restore_background_color();
	void change_background_color();

};

#endif
