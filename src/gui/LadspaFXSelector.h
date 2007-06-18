/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: LadspaFXSelector.h,v 1.8 2005/05/01 19:50:58 comix Exp $
 *
 */

#ifndef LADSPA_FX_SELECTOR_H
#define LADSPA_FX_SELECTOR_H

#include "config.h"

#include "lib/Object.h"
#include "UI/LadspaFXSelector_UI.h"

#include <string>
using std::string;
#include <vector>
using std::vector;

#include <qlistview.h>

#include "lib/fx/LadspaFX.h"

class LadspaFXSelector : public LadspaFXSelector_UI, public Object
{
	Q_OBJECT

	public:
		LadspaFXSelector(int nLadspaFX);
		~LadspaFXSelector();

		string getSelectedFX();

		virtual void pluginSelected();
		virtual void groupSelected();

	private:
		string m_sSelectedPluginName;

		void buildLadspaGroups();
#ifdef LADSPA_SUPPORT
		void addGroup(QListViewItem *pItem, LadspaFXGroup *pGroup);

		vector<LadspaFXInfo*> findPluginsInGroup( string sSelectedGroup, LadspaFXGroup *pGroup );
#endif

};


#endif

