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

#ifndef SOUND_LIBRARY_PANEL_H
#define SOUND_LIBRARY_PANEL_H

#include <map>
#include <memory>

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

#include "../../EventListener.h"
#include "../../Widgets/WidgetWithScalableFont.h"

namespace H2Core {
	class SoundLibraryInfo;
	class Drumkit;
}

class InfoView;
class SoundLibraryTree;
class ToggleButton;

/** \ingroup docGUI*/
class SoundLibraryPanel : public QWidget,
						  protected WidgetWithScalableFont<8, 10, 12>,
						  private H2Core::Object<SoundLibraryPanel>,
						  public EventListener {
	H2_OBJECT( SoundLibraryPanel )
	Q_OBJECT
   public:
	static constexpr int nHeaderHeight = 26;

	SoundLibraryPanel(
		QWidget* parent,
		std::shared_ptr<H2Core::SoundLibraryInfo::Type> pOpenType
	);
	~SoundLibraryPanel();

	/** Somewhat low-level function for drumkit switching. In case drumkit
	 * switching is triggered by the user, #MainForm::switchDrumkit() should
	 * be used as entry point. */
	static void switchDrumkit(
		std::shared_ptr<H2Core::Drumkit> pNewDrumkit,
		std::shared_ptr<H2Core::Drumkit> pOldDrumkit
	);

	SoundLibraryTree* getCurrentTree();

	/** Populates the detail view at the bottom with the metadata of the
	 * currently selected item in the active tab's tree. */
	void updateInfoView( std::shared_ptr<H2Core::SoundLibraryInfo> pInfo );

	void soundLibraryChangedEvent() override;
	void updateSongEvent( int nValue ) override;

   private slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

	/** Called when the active tab in m_pTabWidget changes. */
	void onTabChanged( int nIndex );
	/** Called when the search field text changes. Filters all trees. */
	void onSearchTextChanged( const QString& sText );
	/** Called when the rescan button is clicked. */
	void onRescanClicked();

   signals:
	void itemChanged( bool bSelected );

   private:
	void updateIcons();
	void updateStyleSheet();
	/** Convenience wrapper that calls all three update methods. */
	void updateTree();

	/** Recursively show/hide items in @a pTree based on @a sFilter. */
	void filterTree( SoundLibraryTree* pTree, const QString& sFilter );
	bool filterTreeRecursive(
		SoundLibraryTree* pTree,
		QTreeWidgetItem* pItem,
		const QString& sFilter
	);
	void hideRecursive( QTreeWidgetItem* pItem, bool bHidden );

	// --- Top-level layout widgets ---
	QLineEdit* m_pSearchField;
	QToolButton* m_pRescanButton;
	QTabWidget* m_pTabWidget;

	// --- Per-tab trees ---
	SoundLibraryTree* m_pDrumkitTree;  // tab 0
	SoundLibraryTree* m_pPatternTree;  // tab 1
	SoundLibraryTree* m_pSongTree;	   // tab 2

	InfoView* m_pInfoView;

	/** Whether the dialog was constructed as part of an Open from Library
	 * dialog for the corresponding artifact via a click in the MainForm or as
	 * part of the main window. */
	std::shared_ptr<H2Core::SoundLibraryInfo::Type> m_pOpenType;
};

#endif
