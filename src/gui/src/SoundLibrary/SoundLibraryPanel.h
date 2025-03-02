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

#ifndef SOUND_LIBRARY_PANEL_H
#define SOUND_LIBRARY_PANEL_H


#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "../Widgets/WidgetWithScalableFont.h"
#include "../EventListener.h"

class SoundLibraryTree;
class ToggleButton;

/** \ingroup docGUI*/
class SoundLibraryPanel : public QWidget, protected WidgetWithScalableFont<8, 10, 12>, private H2Core::Object<SoundLibraryPanel>, public EventListener
{
	H2_OBJECT(SoundLibraryPanel)
Q_OBJECT
public:
	SoundLibraryPanel( QWidget* parent, bool bInItsOwnDialog );
	~SoundLibraryPanel();

	QString getDrumkitLabel( const QString& sDrumkitPath ) const;
	QString getDrumkitPath( const QString& sDrumkitLabel ) const;

		/** Somewhat low-level function for drumkit switching. In case drumkit
		 * switching is triggered by the user, #MainForm::switchDrumkit() should
		 * be used as entry point. */
		static void switchDrumkit( std::shared_ptr<H2Core::Drumkit> pNewDrumkit,
								   std::shared_ptr<H2Core::Drumkit> pOldDrumkit );
	
	virtual void soundLibraryChangedEvent() override;

public slots:
	void on_drumkitLoadAction();

private slots:
	void on_DrumkitList_ItemChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous );
	void on_DrumkitList_itemActivated( QTreeWidgetItem* item, int column );
	void on_DrumkitList_leftClicked( const QPoint& pos );
	void on_DrumkitList_rightClicked( const QPoint& pos );
	void on_DrumkitList_mouseMove( QMouseEvent* event );

	void on_drumkitDeleteAction();
	void on_drumkitExportAction();
	void on_songLoadAction();
	void on_patternLoadAction();
	void on_patternDeleteAction();
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

signals:
	void item_changed(bool bDrumkitSelected);

private:
		void editDrumkitProperties( bool bDuplicate );
	void updateTree();
	void test_expandedItems();

	SoundLibraryTree *__sound_library_tree;

	QPoint __start_drag_position;

	QMenu* __drumkit_menu;
	QMenu* __drumkit_menu_system;
	QMenu* __song_menu;
	QMenu* __pattern_menu;
	QMenu* __pattern_menu_list;

	QTreeWidgetItem* m_pTreeSystemDrumkitsItem;
	QTreeWidgetItem* m_pTreeUserDrumkitsItem;
	QTreeWidgetItem* m_pTreeSessionDrumkitsItem;
	QTreeWidgetItem* __song_item;
	QTreeWidgetItem* __pattern_item;
	QTreeWidgetItem* __pattern_item_list;

	/**
	 * Used to uniquely identify the drumkit corresponding to an item
	 * in the tree. It maps the name used as label (key) to the
	 * absolute path of the drumkit (value) also used as unique ID in
	 * H2Core::Hydrogen::SoundLibraryDatabase::m_drumkitDatabase.
	 */
	std::map<QString,QString> m_drumkitRegister;
	/** List of all labels used for drumkits in the tree.
	 *
	 * Used to ensure uniqueness.*/
	QStringList m_drumkitLabels;

	/** Whether the dialog was constructed via a click in the MainForm
	 * or as part of the GUI.
	 */
	bool m_bInItsOwnDialog;
};

#endif
