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

#include "hydrogen/config.h"
#if defined(H2CORE_HAVE_LADSPA) || _DOXYGEN_

#include <hydrogen/globals.h>
#include <hydrogen/object.h>
#include <hydrogen/fx/LadspaFX.h>
#include <hydrogen/fx/LV2FX.h>
#include <hydrogen/fx/H2FX.h>

#include <vector>
#include <cassert>

#include <lilv-0/lilv/lilv.h>

namespace H2Core
{
class Effects : public H2Core::Object
{
	H2_OBJECT
public:
	/**
	 * If #__instance equals 0, a new Effects
	 * singleton will be created and stored in it.
	 *
	 * It is called in Hydrogen::audioEngine_init().
	 */
	static void create_instance();
	/**
	 * Returns a pointer to the current Effects singleton
	 * stored in #__instance.
	 */
	static Effects* get_instance() { assert(__instance); return __instance; }
	~Effects();

	H2FX* getLadspaFX( int nFX );
	void  setLadspaFX( H2FX* pFX, int nFX );
	
	void                       fillLV2PluginList();
	void                       fillLadspaPluginList();

	std::vector<H2FXInfo*>     getPluginList();

	
	H2FXGroup* getLadspaFXGroup();

	Lv2FX* m_pLv2FX;

private:
	/**
	 * Object holding the current Effects singleton. It is
	 * initialized with NULL, set with create_instance(), and
	 * accessed with get_instance().
	 */
	static Effects* __instance;
	
	std::vector<H2FXInfo*> m_pluginList;
	
	H2FXGroup* m_pRootGroup;
	H2FXGroup* m_pRecentGroup;

	void updateRecentGroup();

	H2FX* m_FXList[ MAX_FX ];

	Effects();

	void RDFDescend( const QString& sBase, H2FXGroup *pGroup, std::vector<H2FXInfo*> pluginList );
	void getRDF( H2FXGroup *pGroup, std::vector<H2FXInfo*> pluginList );

};

};

#endif


#endif
