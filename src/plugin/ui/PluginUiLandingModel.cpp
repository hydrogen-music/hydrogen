/*
 * Hydrogen
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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "PluginUiLandingModel.h"

#include <core/Basics/Drumkit.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>

#include <plugin/HydrogenPlugin.h>

namespace H2Core {

PluginUiLandingModel::PluginUiLandingModel( HydrogenPlugin* pPlugin )
	: m_pPlugin( pPlugin )
{
}

PluginUiLandingModel::~PluginUiLandingModel()
{
}

// The load commands follow the engine's two-step shape: the load* commands
// only parse, the set* companions install into the engine. A path that does
// not load fails the first step and leaves the engine's current state
// untouched — the landing view must never install a half-loaded object.

bool PluginUiLandingModel::loadSong( const QString& sPath )
{
	auto pCAC = m_pPlugin != nullptr
		? m_pPlugin->getHydrogen()->getCoreActionController() : nullptr;
	if ( pCAC == nullptr ) {
		return false;
	}
	Logger::Scope loggerScope( m_pPlugin->getHydrogen()->getLogger() );

	auto pSong = pCAC->loadSong( sPath );
	if ( pSong == nullptr ) {
		return false;
	}

	return pCAC->setSong( pSong );
}

bool PluginUiLandingModel::loadPattern( const QString& sPath,
										int nPatternNumber, bool bReplace )
{
	auto pCAC = m_pPlugin != nullptr
		? m_pPlugin->getHydrogen()->getCoreActionController() : nullptr;
	if ( pCAC == nullptr ) {
		return false;
	}
	Logger::Scope loggerScope( m_pPlugin->getHydrogen()->getLogger() );

	auto pPattern = pCAC->loadPattern( sPath );
	if ( pPattern == nullptr ) {
		return false;
	}

	return pCAC->setPattern( pPattern, nPatternNumber, bReplace );
}

bool PluginUiLandingModel::loadDrumkit( const QString& sPath )
{
	auto pHydrogen = m_pPlugin != nullptr ? m_pPlugin->getHydrogen()
										  : nullptr;
	auto pCAC = pHydrogen != nullptr
		? pHydrogen->getCoreActionController() : nullptr;
	if ( pCAC == nullptr ) {
		return false;
	}
	Logger::Scope loggerScope( m_pPlugin->getHydrogen()->getLogger() );

	// setDrumkit installs a full Drumkit object (not a path), so the kit is
	// loaded first — upgraded like the sound library database loads it.
	auto pDrumkit = Drumkit::load( sPath, true, nullptr, false, pHydrogen );
	if ( pDrumkit == nullptr ) {
		return false;
	}

	return pCAC->setDrumkit( pDrumkit );
}

bool PluginUiLandingModel::openEditor( bool bLaunchProcess )
{
	if ( m_pPlugin == nullptr ) {
		return false;
	}
	Logger::Scope loggerScope( m_pPlugin->getHydrogen()->getLogger() );

	return m_pPlugin->openEditor( bLaunchProcess );
}

} // namespace H2Core
