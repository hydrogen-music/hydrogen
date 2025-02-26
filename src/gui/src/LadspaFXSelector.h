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

#ifndef LADSPA_FX_SELECTOR_H
#define LADSPA_FX_SELECTOR_H


#include "ui_LadspaFXSelector_UI.h"

#include <core/config.h>
#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

#include <memory>
#include <vector>

namespace H2Core {
	class LadspaFXInfo;
	class LadspaFXGroup;
}

/** \ingroup docGUI*/
class LadspaFXSelector :  public QDialog,
						  public Ui_LadspaFXSelector_UI,
						  public H2Core::Object<LadspaFXSelector>
{
    H2_OBJECT(LadspaFXSelector)
	Q_OBJECT

	public:
		explicit LadspaFXSelector(int nLadspaFX);
		~LadspaFXSelector();

		QString getSelectedFX();

	private slots:
		void on_m_pGroupsListView_currentItemChanged( QTreeWidgetItem* current,
													  QTreeWidgetItem* previous );
		void pluginSelected();

	private:
		QTreeWidgetItem* m_pCurrentItem;
		QString m_sSelectedPluginName;
		void buildLadspaGroups();

#ifdef H2CORE_HAVE_LADSPA
		void addGroup( QTreeWidgetItem* pParent,
					   std::shared_ptr<H2Core::LadspaFXGroup> pGroup );
		void addGroup( QTreeWidget* pParent,
					   std::shared_ptr<H2Core::LadspaFXGroup> pGroup );
		void buildGroup( QTreeWidgetItem* pNewItem,
						 std::shared_ptr<H2Core::LadspaFXGroup> pGroup );

		std::vector< std::shared_ptr<H2Core::LadspaFXInfo> >
		  findPluginsInGroup( const QString& sSelectedGroup,
							  std::shared_ptr<H2Core::LadspaFXGroup> pGroup );
#endif

};


#endif

