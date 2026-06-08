/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <map>
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
	void updateInfo();
	void updateRegistry();

	static void addDirToLibrary( const QString& sDirPath );
	static void removeDirFromLibrary( const QString& sDirPath );

   public slots:
	void actionAdd();
	void actionLoad();
	void actionProperties();
	void actionDuplicate();
	void actionDelete();
	void actionExport();
	void actionImport();
	void actionOnlineImport();
	void actionAddFolder();
	void actionRemoveFolder();

   signals:
	void itemChanged( bool bSelected );

   protected:
	QItemSelectionModel::SelectionFlags selectionCommand(
		const QModelIndex& index, const QEvent* event
	) const override;

   private:
	/** Intermediate, filesystem-backed representation of the artifact tree
	 * built in addNodes(). Each node holds its child folders and the artifacts
	 * (leaves) located directly within it. Using std::map keeps both sorted
	 * alphabetically, which is the order they are inserted into the tree. */
	struct PathNode {
		std::map<QString, PathNode> folders;
		std::map<QString, std::shared_ptr<H2Core::SoundLibraryInfo>> leaves;
	};

	/** Items in the tree are arranged alpha-numerically with subfolders shown
	 * first followed by files within the folder.
	 *
	 * All artifact paths are interpreted using Qt's filesystem abstraction
	 * (QDir/QFileInfo), which uses '/' as separator on all platforms. Each
	 * artifact is placed relative to @a sBasePath; artifacts contained in
	 * subfolders of arbitrary depth get dedicated folder nodes created for
	 * them. */
	void addNodes(
		QTreeWidgetItem* pParent,
		std::vector<std::shared_ptr<H2Core::SoundLibraryInfo>> infos,
		const QString& sBasePath
	);
	/** Recursively emits @a node and all its children as QTreeWidgetItems
	 * below @a pParent. Recursion is bounded by the (finite) depth of the
	 * already-built @a node, so it can not recurse indefinitely. */
	void addPathNode(
		QTreeWidgetItem* pParent,
		const PathNode& node,
		const QString& sIconPath,
		const QFont& dirFont
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

	std::vector<QTreeWidgetItem*> m_internalDirs;
	std::vector<QTreeWidgetItem*> m_customDirs;

	QMenu* m_pPopupMenu;
	QMenu* m_pPopupMenuReadOnly;
	/** Second version of the menu backed by the same actions but with
	 * slightly different naming. This hints that both drumkit and song do
	 * replace the current one when loaded and instrument and pattern are
	 * appended to the corresponding list of the current drumkit/song. */
	QMenu* m_pPopupMenuAdd;
	QMenu* m_pPopupMenuAddReadOnly;

	/** Menu for the top-level nodes / folders. */
	QMenu* m_pPopupMenuDir;
	QMenu* m_pPopupMenuDirReadOnly;

	/** Actions that remain available when multiple items are selected. All
	 * other actions will be hidden in multi-select context. */
	QSet<QAction*> m_multiSelectActions;

	QPoint m_dragStartPosition;
};

inline const std::
	map<QTreeWidgetItem*, std::shared_ptr<H2Core::SoundLibraryInfo>>&
	SoundLibraryTree::getRegistry() const
{
	return m_registry;
}

#endif
