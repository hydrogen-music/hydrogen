/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program; if not, see https://www.gnu.org/licenses
 *
 */

#include "PluginUiMappingModel.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#include <plugin/HydrogenPlugin.h>

namespace H2Core {

PluginUiMappingModel::PluginUiMappingModel( HydrogenPlugin* pPlugin )
	: m_pPlugin( pPlugin )
{
}

PluginUiMappingModel::~PluginUiMappingModel()
{
}

std::shared_ptr<CoreActionController> PluginUiMappingModel::controller() const
{
	return m_pPlugin != nullptr ? m_pPlugin->getHydrogen()->getCoreActionController()
								: nullptr;
}

std::shared_ptr<MidiInstrumentMap> PluginUiMappingModel::liveMap() const
{
	if ( m_pPlugin == nullptr || m_pPlugin->getHydrogen() == nullptr ) {
		return nullptr;
	}
	return m_pPlugin->getHydrogen()->getPreferences()->getMidiInstrumentMap();
}

std::shared_ptr<MidiInstrumentMap> PluginUiMappingModel::mapCopy() const
{
	auto pLive = liveMap();
	if ( pLive == nullptr ) {
		return nullptr;
	}
	return std::make_shared<MidiInstrumentMap>( pLive );
}

bool PluginUiMappingModel::applyMap(
	std::shared_ptr<MidiInstrumentMap> pMap )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	return pCAC->setMidiInstrumentMap( pMap );
}

bool PluginUiMappingModel::setInputMode( MidiInstrumentMap::Input mode )
{
	auto pMap = mapCopy();
	if ( pMap == nullptr ) {
		return false;
	}
	pMap->setInput( mode );
	return applyMap( pMap );
}

bool PluginUiMappingModel::setOutputMode( MidiInstrumentMap::Output mode )
{
	auto pMap = mapCopy();
	if ( pMap == nullptr ) {
		return false;
	}
	pMap->setOutput( mode );
	return applyMap( pMap );
}

bool PluginUiMappingModel::setUseGlobalInputChannel( bool bUse )
{
	auto pMap = mapCopy();
	if ( pMap == nullptr ) {
		return false;
	}
	pMap->setUseGlobalInputChannel( bUse );
	return applyMap( pMap );
}

bool PluginUiMappingModel::setGlobalInputChannel( Midi::Channel channel )
{
	auto pMap = mapCopy();
	if ( pMap == nullptr ) {
		return false;
	}
	pMap->setGlobalInputChannel( channel );
	return applyMap( pMap );
}

bool PluginUiMappingModel::setUseGlobalOutputChannel( bool bUse )
{
	auto pMap = mapCopy();
	if ( pMap == nullptr ) {
		return false;
	}
	pMap->setUseGlobalOutputChannel( bUse );
	return applyMap( pMap );
}

bool PluginUiMappingModel::setGlobalOutputChannel( Midi::Channel channel )
{
	auto pMap = mapCopy();
	if ( pMap == nullptr ) {
		return false;
	}
	pMap->setGlobalOutputChannel( channel );
	return applyMap( pMap );
}

bool PluginUiMappingModel::insertCustomInputMapping(
	std::shared_ptr<Instrument> pInstrument, Midi::Note note,
	Midi::Channel channel )
{
	if ( pInstrument == nullptr ) {
		return false;
	}
	auto pMap = mapCopy();
	if ( pMap == nullptr ) {
		return false;
	}
	pMap->insertCustomInputMapping( pInstrument, note, channel );
	return applyMap( pMap );
}

bool PluginUiMappingModel::insertCustomInputMapping(
	int nInstrument, Midi::Note note, Midi::Channel channel )
{
	return insertCustomInputMapping( instrument( nInstrument ), note,
									 channel );
}

bool PluginUiMappingModel::setInstrumentMidiOutNote( int nInstrument,
													 Midi::Note note )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	// No event correlation id: the plugin UI does not deduplicate the
	// UpdateEvents its own commands emit.
	return pCAC->setInstrumentMidiOutNote( nInstrument, note, 0 );
}

bool PluginUiMappingModel::setInstrumentMidiOutChannel( int nInstrument,
														Midi::Channel channel )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	// No event correlation id: the plugin UI does not deduplicate the
	// UpdateEvents its own commands emit.
	return pCAC->setInstrumentMidiOutChannel( nInstrument, channel, 0 );
}

int PluginUiMappingModel::instrumentCount() const
{
	if ( m_pPlugin == nullptr || m_pPlugin->getHydrogen() == nullptr ) {
		return 0;
	}
	const auto pSong = m_pPlugin->getHydrogen()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ||
		 pSong->getDrumkit()->getInstruments() == nullptr ) {
		return 0;
	}
	return pSong->getDrumkit()->getInstruments()->size();
}

QString PluginUiMappingModel::instrumentName( int nInstrument ) const
{
	const auto pInstrument = instrument( nInstrument );
	return pInstrument != nullptr ? pInstrument->getName() : QString();
}

Midi::Note PluginUiMappingModel::instrumentMidiOutNote(
	int nInstrument ) const
{
	const auto pInstrument = instrument( nInstrument );
	return pInstrument != nullptr ? pInstrument->getMidiOutNote()
								  : Midi::NoteDefault;
}

Midi::Channel PluginUiMappingModel::instrumentMidiOutChannel(
	int nInstrument ) const
{
	const auto pInstrument = instrument( nInstrument );
	return pInstrument != nullptr ? pInstrument->getMidiOutChannel()
								  : Midi::ChannelOff;
}

MidiInstrumentMap::NoteRef PluginUiMappingModel::inputMapping(
	int nInstrument ) const
{
	auto pMap = liveMap();
	const auto pInstrument = instrument( nInstrument );
	if ( pMap == nullptr || pInstrument == nullptr || m_pPlugin == nullptr ||
		 m_pPlugin->getHydrogen() == nullptr ) {
		return MidiInstrumentMap::NoteRef();
	}
	const auto pSong = m_pPlugin->getHydrogen()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return MidiInstrumentMap::NoteRef();
	}
	return pMap->getInputMapping( pInstrument, pSong->getDrumkit(),
								  m_pPlugin->getHydrogen() );
}

std::shared_ptr<Instrument> PluginUiMappingModel::instrument(
	int nInstrument ) const
{
	if ( m_pPlugin == nullptr || m_pPlugin->getHydrogen() == nullptr ) {
		return nullptr;
	}
	const auto pSong = m_pPlugin->getHydrogen()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return nullptr;
	}
	const auto pInstruments = pSong->getDrumkit()->getInstruments();
	// Guarded range check: a UI polling loop must not spam the log
	// when the kit shrank between two frames.
	if ( pInstruments == nullptr || nInstrument < 0 ||
		 nInstrument >= pInstruments->size() ) {
		return nullptr;
	}
	return pInstruments->get( nInstrument );
}

} // namespace H2Core
