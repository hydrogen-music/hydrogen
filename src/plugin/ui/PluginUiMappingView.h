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

// The MIDI mapping view's renderer (proposal 0006 TU4.2): thin imgui draw calls
// over PluginUiMappingModel — no logic grows here (TU1.3); the window smoke
// covers it. Draws the exposed MIDI configuration foldable section (moved from
// the landing view), the note mapping as two foldable sections — Input (mode,
// global channel, the per-instrument rows) and Output (mode, global channel,
// the per-instrument rows) — and the Bus Output section: the instruments in
// a left column, the numbered output busses in a right one, drag&drop
// routing with béziers.

#ifndef H2C_PLUGIN_UI_MAPPING_VIEW_H
#define H2C_PLUGIN_UI_MAPPING_VIEW_H

#include <memory>

#include <imgui.h>

#include "PluginUiMappingModel.h"

namespace H2Core {

class CommonStrings;
class HydrogenPlugin;
struct PluginUiPalette;

/** The last drawn frame's MIDI config widget geometry and values, in
 *  screen space. The window smoke's white-box hook: imgui items are
 *  not queryable by id once the frame is over, so the view records
 *  what its section drew and the smoke asserts on it — the rects are
 *  the interaction probe's targets, the values pin the engine-change
 *  adoption (mirroring the mixer view's layout()). */
struct PluginUiMappingLayout
{
	/** The MIDI config section drew this frame (header open). */
	bool bValid = false;
	/** The section's three widgets' rects. */
	ImVec2 noteOffIgnoreMin{}, noteOffIgnoreMax{};
	ImVec2 actionChannelMin{}, actionChannelMax{};
	ImVec2 sendNoteOffMin{}, sendNoteOffMax{};
	/** The values the three widgets displayed. */
	bool bNoteOffIgnore = false;
	int nActionChannel = -1;
	int nSendNoteOff = 0;
};

class PluginUiMappingView
{
public:
	static constexpr float nSpacing = 12;

	explicit PluginUiMappingView( HydrogenPlugin* pPlugin );

	/** One frame of the mapping view's widget tree, painted with
	 *  @a palette. */
	void draw( const PluginUiPalette& palette );

	/** The view's model — the window polls it (mapping diff) to
	 *  decide frame requests. */
	PluginUiMappingModel& model() { return m_model; }

	/** The last frame's recorded layout — the smoke test's hook. */
	const PluginUiMappingLayout& layout() const { return m_layout; }

private:
	/** Re-reads the exposed MIDI config from the engine into the widget
	 *  state — on construction, after every apply, and once per frame
	 *  while the user is not interacting with the widgets (external
	 *  changes adopt; an open dropdown or held edit must not snap
	 *  back mid-edit). */
	void refreshMidiConfigWidgets();
	/** Applies the MIDI config widgets' current values with the model's
	 * atomic command and resyncs them to the engine afterwards — the
	 * per-widget apply-on-change path, matching the rest of the tab. */
	void applyMidiConfigWidgets();

	PluginUiMappingModel m_model;
	std::shared_ptr<CommonStrings> m_pCommonStrings;
	// Exposed MIDI config widget state.
	bool m_bNoteOffIgnore = false;
	int m_nActionChannel = -1;
	int m_nSendNoteOff = 0;
	/** True while the user is interacting with one of the MIDI config
	 *  widgets (held checkbox, open dropdown) — engine-side changes
	 *  wait it out instead of snapping the widget back. */
	bool m_bMidiConfigInteraction = false;
	PluginUiMappingLayout m_layout;
};

} // namespace H2Core

#endif // H2C_PLUGIN_UI_MAPPING_VIEW_H
