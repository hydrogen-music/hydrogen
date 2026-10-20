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

#ifndef H2C_PLUGIN_UI_ENGINE_SINK_H
#define H2C_PLUGIN_UI_ENGINE_SINK_H

#include <core/IPC/EngineEventSink.h>

namespace H2Core {

class PluginUiLandingModel;
class PluginUiMixerModel;

/** The plugin UI's #EngineEventSink adapter (ADR 0035 UI-3): forwards
 *  the engine session's publish-cycle deliveries into the view models
 *  — meter snapshots to the mixer model, info snapshots to the landing
 *  model — so the embedded UI sees exactly the data the IPC editor
 *  sees, without ever touching the engine or its locks. The models
 *  cache; the window polls and diffs them on its UI thread to decide
 *  frame requests.
 *
 * onEvent is a no-op: the UI is pull-rendered, change detection is the
 * window's poll diff over the cached snapshots, not event
 * bookkeeping.
 *
 * Owned by the plugin UI window; the window unregisters it (via
 * HydrogenPlugin::clearEventSink) before the models it forwards into
 * go away. Pure logic, no imgui types (TU1.3). */
class PluginUiEngineSink : public EngineEventSink
{
public:
	PluginUiEngineSink( PluginUiLandingModel& rLandingModel,
					   PluginUiMixerModel& rMixerModel );

	void onEvent( Event::Type type, int nValue, long nMilliseconds ) override;
	void onMeterSnapshot( const EngineTelemetrySnapshot& snapshot ) override;
	void onInfoSnapshot( const EngineInfoSnapshot& snapshot ) override;

private:
	PluginUiLandingModel& m_rLandingModel;
	PluginUiMixerModel& m_rMixerModel;
};

} // namespace H2Core

#endif // H2C_PLUGIN_UI_ENGINE_SINK_H
