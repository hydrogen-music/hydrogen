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

#ifndef EFFECTS_H
#define EFFECTS_H

#include <core/config.h>
#if defined(H2CORE_HAVE_LADSPA) || _DOXYGEN_

#include <core/Globals.h>
#include <core/Object.h>
#include <core/FX/LadspaFX.h>

#include <vector>
#include <cassert>

namespace H2Core
{
/** \ingroup docCore docAudioEngine */
class Effects : public H2Core::Object<Effects>
{
	H2_OBJECT(Effects)
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

	LadspaFX* getLadspaFX( int nFX ) const;
	void  setLadspaFX( LadspaFX* pFX, int nFX );

	std::vector<LadspaFXInfo*> getPluginList();
	LadspaFXGroup* getLadspaFXGroup();


private:
	/**
	 * Object holding the current Effects singleton. It is
	 * initialized with NULL, set with create_instance(), and
	 * accessed with get_instance().
	 */
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
