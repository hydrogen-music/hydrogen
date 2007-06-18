/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef LADSPA_FX_SELECTOR_H
#define LADSPA_FX_SELECTOR_H

#include <hydrogen/Object.h>
#include "ui_LadspaFXSelector_UI.h"

#include <string>
#include <vector>

#include <QDialog>

#include <hydrogen/fx/LadspaFX.h>

class LadspaFXSelector : public QDialog, public Ui_LadspaFXSelector_UI, public Object
{
	Q_OBJECT

	public:
		LadspaFXSelector(int nLadspaFX);
		~LadspaFXSelector();

		std::string getSelectedFX();

	private slots:
		void on_m_pGroupsListView_currentItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous );
		void pluginSelected();

	private:
		std::string m_sSelectedPluginName;

		void buildLadspaGroups();
#ifdef LADSPA_SUPPORT
		void addGroup(QTreeWidgetItem* pItem, H2Core::LadspaFXGroup *pGroup);

		std::vector<H2Core::LadspaFXInfo*> findPluginsInGroup( const std::string& sSelectedGroup, H2Core::LadspaFXGroup *pGroup );
#endif

};


#endif

