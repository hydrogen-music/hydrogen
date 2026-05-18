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

#ifndef EFFECTS_H
#define EFFECTS_H

#include <core/config.h>
#include <core/Object.h>

#if defined(H2CORE_HAVE_LADSPA) || _DOXYGEN_

#include <core/FX/LadspaFX.h>
#include <core/Globals.h>

#include <cassert>
#include <memory>
#include <vector>

namespace H2Core {
/** \ingroup docCore docAudioEngine */
class Effects : public H2Core::Object<Effects> {
	H2_OBJECT( Effects )
   public:
	Effects();
	~Effects();

	std::shared_ptr<LadspaFX> getLadspaFX( int nFX ) const;
	void setLadspaFX( std::shared_ptr<LadspaFX> pFX, int nFX );

	std::vector<std::shared_ptr<LadspaFXInfo> > getPluginList();
	std::shared_ptr<LadspaFXGroup> getLadspaFXGroup();

   private:
	void getRDF(
		std::shared_ptr<LadspaFXGroup> pGroup,
		std::vector<std::shared_ptr<LadspaFXInfo> > pluginList
	);
	void RDFDescend(
		const QString& sBase,
		std::shared_ptr<LadspaFXGroup> pGroup,
		std::vector<std::shared_ptr<LadspaFXInfo> > pluginList
	);
	void updateRecentGroup();

	std::vector<std::shared_ptr<LadspaFXInfo> > m_pluginList;
	std::shared_ptr<LadspaFXGroup> m_pRootGroup;
	std::shared_ptr<LadspaFXGroup> m_pRecentGroup;
	std::vector<std::shared_ptr<LadspaFX> > m_FXs;
};

};	// namespace H2Core

#else  // H2CORE_HAVE_LADSPA
// LADSPA is disabled

namespace H2Core {
/** \ingroup docCore */
class Effects : public H2Core::Object<Effects> {
	H2_OBJECT( Effects )
   public:
	/**
	 * Fallback version of the Effects in case
	 * #H2CORE_HAVE_LADSPA was not defined during the configuration
	 * and the usage of LADSPA plugins is not intended by
	 * the user.
	 */
	Effects() {}
	~Effects(){};
};

};	// namespace H2Core

#endif	// H2CORE_HAVE_LADSPA

#endif
