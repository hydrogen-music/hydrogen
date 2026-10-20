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
#include "PluginUiHtml.h"

#include <core/Basics/Drumkit.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

#include <plugin/HydrogenPlugin.h>

#include <algorithm>
#include <utility>

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
	auto pDatabase = pHydrogen != nullptr
		? pHydrogen->getSoundLibraryDatabase() : nullptr;
	if ( pCAC == nullptr || pDatabase == nullptr ) {
		return false;
	}
	Logger::Scope loggerScope( m_pPlugin->getHydrogen()->getLogger() );

	// The database is the canonical kit source: it canonicalizes the path
	// and serves already-loaded kits from its cache. A copy is installed so
	// the engine owns its kit outright and the cached object stays pristine.
	const auto pDrumkit = pDatabase->getDrumkit( sPath );
	if ( pDrumkit == nullptr ) {
		___ERRORLOG( QString( "Unable to retrieve drumkit [%1] from database" )
					 .arg( sPath ) );
		return false;
	}

	return pCAC->setDrumkit( std::make_shared<Drumkit>( pDrumkit ) );
}

QString PluginUiLandingModel::htmlToPlainText( const QString& sHtml )
{
	return pluginUiHtmlToPlainText( sHtml );
}

std::vector<PluginUiLandingModel::DrumkitSummary>
PluginUiLandingModel::drumkits() const
{
	std::vector<DrumkitSummary> summaries;
	if ( m_pPlugin == nullptr ) {
		return summaries;
	}

	Logger::Scope loggerScope( m_pPlugin->getHydrogen()->getLogger() );

	const auto pDatabase = m_pPlugin->getHydrogen()->getSoundLibraryDatabase();
	for ( const auto& ppInfo : pDatabase->getDrumkitInfos() ) {
		if ( ppInfo == nullptr ) {
			continue;
		}
		DrumkitSummary summary;
		summary.sName = ppInfo->getName();
		summary.sAuthor = ppInfo->getAuthor();
		summary.sLicense = ppInfo->getLicense().getLicenseString();
		summary.sInfo = htmlToPlainText( ppInfo->getInfo() );
		summary.nVersion = ppInfo->getVersion();
		summary.sPath = ppInfo->getPath();
		summaries.push_back( std::move( summary ) );
	}

	std::sort( summaries.begin(), summaries.end(),
			   []( const DrumkitSummary& a, const DrumkitSummary& b ) {
				   return a.sName.compare( b.sName, Qt::CaseInsensitive ) < 0;
			   } );
	return summaries;
}

EngineInfoSnapshot PluginUiLandingModel::infoSnapshot() const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_infoCache;
}

void PluginUiLandingModel::acceptInfoSnapshot(
	const EngineInfoSnapshot& snapshot )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	m_infoCache = snapshot;
}

bool PluginUiLandingModel::pollInfoChanged()
{
	EngineInfoSnapshot cache;
	{
		std::lock_guard<std::mutex> lock( m_mutex );
		cache = m_infoCache;
	}
	if ( cache == m_infoDisplayed ) {
		return false;
	}
	m_infoDisplayed = cache;
	return true;
}

QString PluginUiLandingModel::editorExecutable() const
{
	if ( m_pPlugin == nullptr ) {
		return QString();
	}
	return m_pPlugin->editorBinary();
}

bool PluginUiLandingModel::setEditorExecutable( const QString& sPath )
{
	if ( m_pPlugin == nullptr ) {
		return false;
	}
	m_pPlugin->setEditorBinary( sPath );
	return true;
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
