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

#ifndef LADSPA_FX_SELECTOR_H
#define LADSPA_FX_SELECTOR_H


#include "ui_LadspaFXSelector_UI.h"

#include <hydrogen/config.h>
#include <hydrogen/object.h>

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif
#include <string>
#include <vector>

namespace H2Core {
	class LadspaFXInfo;
	class LadspaFXGroup;
}

class LadspaFXSelector : public QDialog, public Ui_LadspaFXSelector_UI, public H2Core::Object
{
	Q_OBJECT

	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }
		LadspaFXSelector(int nLadspaFX);
		~LadspaFXSelector();

		QString getSelectedFX();

	private slots:
		void on_m_pGroupsListView_currentItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous );
		void pluginSelected();

	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;
		QTreeWidgetItem* m_pCurrentItem;
		QString m_sSelectedPluginName;
		void buildLadspaGroups();

#ifdef H2CORE_HAVE_LADSPA
		void addGroup(QTreeWidgetItem *parent, H2Core::LadspaFXGroup *pGroup);
		void addGroup( QTreeWidget *parent, H2Core::LadspaFXGroup *pGroup );
		void buildGroup(QTreeWidgetItem *pNewItem, H2Core::LadspaFXGroup *pGroup);

		std::vector<H2Core::LadspaFXInfo*> findPluginsInGroup( const QString& sSelectedGroup, H2Core::LadspaFXGroup *pGroup );
#endif

};


#endif

