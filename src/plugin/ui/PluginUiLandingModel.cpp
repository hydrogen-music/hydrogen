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
#include <core/Midi/MidiInstrumentMap.h>

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

	return m_pPlugin->openEditor( bLaunchProcess );
}

// The MIDI config applies go through the engine's granular install commands
// (ADR 0030/0022): the plugin's engine preferences adopt them live. Neither
// command persists — and the plugin never calls a preferences save on top —
// so a host session cannot clobber the user's ~/.hydrogen.

bool PluginUiLandingModel::applyMidiControlSettings(
	bool bNoteOffIgnore, Midi::Channel actionChannel, bool bEnableFeedback,
	bool bTransportInputHandling, bool bTransportOutputSend,
	Midi::Channel feedbackChannel, Preferences::MidiSendNoteOff sendNoteOff )
{
	auto pCAC = m_pPlugin != nullptr
		? m_pPlugin->getHydrogen()->getCoreActionController() : nullptr;
	if ( pCAC == nullptr ) {
		return false;
	}

	return pCAC->setMidiControlSettings(
		bNoteOffIgnore, actionChannel, bEnableFeedback,
		bTransportInputHandling, bTransportOutputSend, feedbackChannel,
		sendNoteOff );
}

bool PluginUiLandingModel::applyMidiInstrumentMap(
	std::shared_ptr<MidiInstrumentMap> pMap )
{
	auto pCAC = m_pPlugin != nullptr
		? m_pPlugin->getHydrogen()->getCoreActionController() : nullptr;
	if ( pCAC == nullptr || pMap == nullptr ) {
		return false;
	}

	return pCAC->setMidiInstrumentMap( pMap );
}

bool PluginUiLandingModel::applyExposedMidiControlSettings(
	bool bNoteOffIgnore, Midi::Channel actionChannel,
	Preferences::MidiSendNoteOff sendNoteOff )
{
	auto pPref = preferences();
	if ( pPref == nullptr ) {
		return false;
	}

	// The plugin UI does not expose feedback or transport; the apply
	// preserves their current engine values.
	return applyMidiControlSettings(
		bNoteOffIgnore, actionChannel, pPref->m_bEnableMidiFeedback,
		pPref->getMidiTransportInputHandling(),
		pPref->getMidiTransportOutputSend(),
		pPref->getMidiFeedbackChannel(), sendNoteOff );
}

bool PluginUiLandingModel::midiNoteOffIgnore() const
{
	auto pPref = preferences();
	// The engine default (true) when the engine is unavailable.
	return pPref != nullptr ? pPref->m_bMidiNoteOffIgnore : true;
}

Midi::Channel PluginUiLandingModel::midiActionChannel() const
{
	auto pPref = preferences();
	// The engine default (all channels) when the engine is unavailable.
	return pPref != nullptr ? pPref->m_midiActionChannel : Midi::ChannelAll;
}

Preferences::MidiSendNoteOff PluginUiLandingModel::midiSendNoteOff() const
{
	auto pPref = preferences();
	// The engine default (always) when the engine is unavailable.
	return pPref != nullptr ? pPref->getMidiSendNoteOff()
							: Preferences::MidiSendNoteOff::Always;
}

std::shared_ptr<Preferences> PluginUiLandingModel::preferences() const
{
	return m_pPlugin != nullptr && m_pPlugin->getHydrogen() != nullptr
		? m_pPlugin->getHydrogen()->getPreferences() : nullptr;
}

} // namespace H2Core
