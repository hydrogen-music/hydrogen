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
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

#include <plugin/HydrogenPlugin.h>

#include <QtCore/QFileInfo>

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

std::shared_ptr<Preferences> PluginUiLandingModel::preferences() const
{
	return m_pPlugin != nullptr && m_pPlugin->getHydrogen() != nullptr
		? m_pPlugin->getHydrogen()->getPreferences() : nullptr;
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

	// The dialog's OK counts as load intent even when the load below
	// fails — the next dialog opens in the picked file's folder.
	auto pPreferences = preferences();
	if ( pPreferences != nullptr ) {
		pPreferences->setLastOpenSongDirectory(
			QFileInfo( sPath ).absolutePath() );
	}

	auto pSong = pCAC->loadSong( sPath );
	if ( pSong == nullptr ) {
		return false;
	}

	if ( ! pCAC->setSong( pSong ) ) {
		___ERRORLOG( "Unable to assign song" );
		return false;
	}

	if ( ! pCAC->activateSongMode( true ) ) {
		___ERRORLOG( "Unable to switch to song mode" );
		return false;
	}

	return true;
}

bool PluginUiLandingModel::loadPattern( const QString& sPath )
{
	auto pCAC = m_pPlugin != nullptr
		? m_pPlugin->getHydrogen()->getCoreActionController() : nullptr;
	if ( pCAC == nullptr ) {
		return false;
	}
	Logger::Scope loggerScope( m_pPlugin->getHydrogen()->getLogger() );

	// The dialog's OK counts as load intent even when the load below
	// fails — the next dialog opens in the picked file's folder.
	auto pPreferences = preferences();
	if ( pPreferences != nullptr ) {
		pPreferences->setLastOpenPatternDirectory(
			QFileInfo( sPath ).absolutePath() );
	}

	auto pPattern = pCAC->loadPattern( sPath );
	if ( pPattern == nullptr ) {
		return false;
	}

	// Within this simple UI we support loading/playing either and entire song
	// or just a single pattern. For the latter, we add it to an new and empty
	// song to ensure we neither modify the current previous song nor have its
	// properties leak into the pattern. We only support selected pattern mode
	// in here.
	auto pSong = Song::getEmptySong( m_pPlugin->getHydrogen() );
	if ( pSong == nullptr ) {
		___ERRORLOG( "Unable to retrieve empty song" );
		return false;
	}
	if ( ! pCAC->setSong( pSong ) ) {
		___ERRORLOG( "Unable to assign empty song" );
		return false;
	}

	const int nPatternNumber = 0;

	if ( ! pCAC->setPattern( pPattern, nPatternNumber, /*replace*/true ) ) {
		___ERRORLOG( "Unable to assign loaded pattern" );
		return false;
	}

	if ( ! pCAC->activateSongMode( false ) ) {
		___ERRORLOG( "Unable to switch to pattern mode" );
		return false;
	}

	m_pPlugin->getHydrogen()->setPatternMode( Song::PatternMode::Selected );

	if ( ! pCAC->selectPattern( nPatternNumber ) ) {
		___ERRORLOG( "Unable to select new pattern" );
		return false;
	}

	return true;
}

bool PluginUiLandingModel::clearSong()
{
	auto pCAC = m_pPlugin != nullptr
		? m_pPlugin->getHydrogen()->getCoreActionController() : nullptr;
	if ( pCAC == nullptr ) {
		return false;
	}
	Logger::Scope loggerScope( m_pPlugin->getHydrogen()->getLogger() );

	auto pSong = Song::getEmptySong( m_pPlugin->getHydrogen() );
	if ( pSong == nullptr ) {
		return false;
	}

	if ( ! pCAC->setSong( pSong ) ) {
		___ERRORLOG( "Unable to assign empty song" );
		return false;
	}

	if ( ! pCAC->activateSongMode( true ) ) {
		// It doesn't matter whether we in song or pattern mode since there are
		// no notes to play back. But it might be more intuitive for users to
		// read "Mode: Song" within the info section after clearing the song.
		___ERRORLOG( "Unable to switch to song mode" );
		return false;
	}

	return true;
}

QString PluginUiLandingModel::songDialogDefaultPath() const
{
	auto pPreferences = preferences();
	if ( pPreferences == nullptr ) {
		return Filesystem::userSongsDir();
	}
	const QString sRemembered = pPreferences->getLastOpenSongDirectory();
	// A remembered folder can vanish between sessions (removable
	// drive, relocated user dir); the browser then falls back to the
	// stock user dir instead of opening nowhere.
	if ( ! Filesystem::dirReadable( sRemembered, /*bSilent=*/false ) ) {
		return Filesystem::userSongsDir();
	}
	return sRemembered;
}

QString PluginUiLandingModel::patternDialogDefaultPath() const
{
	auto pPreferences = preferences();
	if ( pPreferences == nullptr ) {
		return Filesystem::userPatternsDir();
	}
	const QString sRemembered = pPreferences->getLastOpenPatternDirectory();
	// A remembered folder can vanish between sessions (removable
	// drive, relocated user dir); the browser then falls back to the
	// stock user dir instead of opening nowhere.
	if ( ! Filesystem::dirReadable( sRemembered, /*bSilent=*/false ) ) {
		return Filesystem::userPatternsDir();
	}
	return sRemembered;
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

bool PluginUiLandingModel::pluginStateEmbedSamples() const
{
	auto pPreferences = preferences();
	if ( pPreferences == nullptr ) {
		// Without a plugin (or preferences) the default behavior
		// applies: a saved state embeds its samples.
		return true;
	}
	return pPreferences->getPluginStateEmbedSamples();
}

bool PluginUiLandingModel::setPluginStateEmbedSamples( bool bEmbedSamples )
{
	auto pPreferences = preferences();
	if ( pPreferences == nullptr ) {
		return false;
	}
	pPreferences->setPluginStateEmbedSamples( bEmbedSamples );
	return true;
}

} // namespace H2Core
