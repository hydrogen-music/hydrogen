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

#ifndef EFFECTS_H
#define EFFECTS_H

#ifdef LADSPA_SUPPORT

#include <hydrogen/globals.h>
#include <hydrogen/Object.h>
#include <hydrogen/fx/LadspaFX.h>

#include <vector>
#include <cassert>

namespace H2Core
{

/**
 *
 */
class Effects : public Object
{
public:
	static void create_instance();
	static Effects* get_instance() { assert(__instance); return __instance; }
	~Effects();

	LadspaFX* getLadspaFX( int nFX );
	void  setLadspaFX( LadspaFX* pFX, int nFX );

	std::vector<LadspaFXInfo*> getPluginList();
	LadspaFXGroup* getLadspaFXGroup();


private:
	static Effects* __instance;
	std::vector<LadspaFXInfo*> m_pluginList;
	LadspaFXGroup* m_pRootGroup;
	LadspaFXGroup* m_pRecentGroup;
	
	void updateRecentGroup();

	LadspaFX* m_FXList[ MAX_FX ];

	Effects();

	void RDFDescend( const QString& sBase, LadspaFXGroup *pGroup, std::vector<LadspaFXInfo*> pluginList );
	void getRDF( LadspaFXGroup *pGroup, std::vector<LadspaFXInfo*> pluginList );
	
};

};

#endif


#endif
