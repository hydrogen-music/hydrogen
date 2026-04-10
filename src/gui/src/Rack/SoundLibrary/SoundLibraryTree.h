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

#include <QtGui>
#include <QtWidgets>

#include <core/Helpers/Filesystem.h>
#include <core/Object.h>

#include "../../Widgets/WidgetWithScalableFont.h"

namespace H2Core {
class SoundLibraryInfo;
}

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
		H2Core::Filesystem::Artifact artifact,
		bool bStandAlone
	);

	const std::map<QTreeWidgetItem*, std::shared_ptr<H2Core::SoundLibraryInfo>>&
	getRegistry() const;
	void updateRegistry();

   signals:
	void leftClicked( const QPoint& pos );
	void rightClicked( const QPoint& pos );

   protected:
	virtual void mousePressEvent( QMouseEvent* event ) override;
	virtual void mouseMoveEvent( QMouseEvent* event ) override;

   private:
	SoundLibraryPanel* m_pSoundLibraryPanel;

	H2Core::Filesystem::Artifact m_artifact;

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
};

inline const std::
	map<QTreeWidgetItem*, std::shared_ptr<H2Core::SoundLibraryInfo>>&
	SoundLibraryTree::getRegistry() const
{
	return m_registry;
}

#endif
