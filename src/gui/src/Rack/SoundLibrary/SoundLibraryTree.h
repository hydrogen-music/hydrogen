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

#ifndef SOUND_LIBRARY_TREE_H
#define SOUND_LIBRARY_TREE_H

#include <memory>
#include <vector>

#include <QtGui>
#include <QtWidgets>

#include <core/Helpers/Filesystem.h>
#include <core/Object.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

#include "../../Widgets/WidgetWithScalableFont.h"

class SoundLibraryPanel;

/** \ingroup docGUI*/
class SoundLibraryTree : public QTreeWidget,
						 protected WidgetWithScalableFont<8, 10, 12>,
						 private H2Core::Object<SoundLibraryTree> {
	H2_OBJECT( SoundLibraryTree )
	Q_OBJECT
   public:
	explicit SoundLibraryTree(
		SoundLibraryPanel* pParent,
		H2Core::SoundLibraryInfo::Type type,
		bool bStandAlone
	);

	const std::map<QTreeWidgetItem*, std::shared_ptr<H2Core::SoundLibraryInfo>>&
	getRegistry() const;

	void updateFont();
	void updateRegistry();

   public slots:
	void actionLoad();
	void actionProperties();
	void actionDuplicate();
	void actionDelete();
	void actionExport();
	void actionImport();
	void actionOnlineImport();

   signals:
	void itemChanged( bool bSelected );

   protected:
	QItemSelectionModel::SelectionFlags selectionCommand(
		const QModelIndex& index, const QEvent* event
	) const override;

   private:
	/** Items in the tree are arranged alpha-numerically with subfolders shown
	 * first followed by files within the folder. This function will be called
	 * recursively in order to account for nested folders in the user data
	 * directory. */
	void addNodes(
		QTreeWidgetItem* pParent,
		std::vector<std::shared_ptr<H2Core::SoundLibraryInfo>> infos,
		const QString& sBasePath
	);
	void recursivelyUpdateFont( QTreeWidgetItem* pItem );

	void mousePressEvent( QMouseEvent* event ) override;
	void mouseMoveEvent( QMouseEvent* event ) override;

	SoundLibraryPanel* m_pSoundLibraryPanel;

	H2Core::SoundLibraryInfo::Type m_type;

	/** Whether the widget is created as part of the main window or as part of
	 * the Open From Library dialog of the corresponding aritfact. */
	bool m_bStandAlone;

	/** Maps pattern tree items to their SoundLibraryInfo for pattern
	 * operations (load, delete, drag-and-drop). */
	std::map<QTreeWidgetItem*, std::shared_ptr<H2Core::SoundLibraryInfo>>
		m_registry;

	QTreeWidgetItem* m_pSessionItem;
	QTreeWidgetItem* m_pSystemItem;
	QTreeWidgetItem* m_pUserItem;

	QMenu* m_pPopupMenu;
	QMenu* m_pPopupMenuReadOnly;
	/** Second version of the menu backed by the same actions but with
	 * slightly different naming. This hints that both drumkit and song do
	 * replace the current one when loaded and instrument and pattern are
	 * appended to the corresponding list of the current drumkit/song. */
	QMenu* m_pPopupMenuAdd;
	QMenu* m_pPopupMenuAddReadOnly;

	QPoint m_dragStartPosition;
};

inline const std::
	map<QTreeWidgetItem*, std::shared_ptr<H2Core::SoundLibraryInfo>>&
	SoundLibraryTree::getRegistry() const
{
	return m_registry;
}

#endif
