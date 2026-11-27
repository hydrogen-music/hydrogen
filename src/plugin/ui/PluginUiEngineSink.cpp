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

#include "PluginUiEngineSink.h"

#include "PluginUiLandingModel.h"
#include "PluginUiMixerModel.h"

namespace H2Core {

PluginUiEngineSink::PluginUiEngineSink(
	PluginUiLandingModel& rLandingModel, PluginUiMixerModel& rMixerModel )
	: m_rLandingModel( rLandingModel ), m_rMixerModel( rMixerModel )
{
}

void PluginUiEngineSink::onEvent( Event::Type, int, long )
{
	// Deliberately empty: the UI is pull-rendered — change detection
	// is the window's poll diff over the cached snapshots, not event
	// bookkeeping (see the class comment).
}

void PluginUiEngineSink::onMeterSnapshot(
	const EngineTelemetrySnapshot& snapshot )
{
	m_rMixerModel.acceptMeterSnapshot( snapshot );
}

void PluginUiEngineSink::onInfoSnapshot(
	const EngineInfoSnapshot& snapshot )
{
	m_rLandingModel.acceptInfoSnapshot( snapshot );
}

} // namespace H2Core
