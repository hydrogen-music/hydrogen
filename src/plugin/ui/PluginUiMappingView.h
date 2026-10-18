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

// The MIDI mapping view's renderer (proposal 0006 TU4.2): thin imgui
// draw calls over PluginUiMappingModel — no logic grows here (TU1.3);
// the window smoke covers it. Draws the exposed MIDI configuration
// foldable section (moved from the landing view) and the note mapping
// as two foldable sections — Input (mode, global channel, the
// per-instrument rows) and Output (mode, global channel, the
// per-instrument rows).

#ifndef H2C_PLUGIN_UI_MAPPING_VIEW_H
#define H2C_PLUGIN_UI_MAPPING_VIEW_H

#include <memory>

#include "PluginUiMappingModel.h"

namespace H2Core {

class CommonStrings;
class HydrogenPlugin;

class PluginUiMappingView
{
public:
	static constexpr float nSpacing = 12;

	explicit PluginUiMappingView( HydrogenPlugin* pPlugin );

	/** One frame of the mapping view's widget tree. */
	void draw();

private:
	/** Re-reads the exposed MIDI config from the engine into the widget
	 * state — on construction and after every apply, never per frame
	 * (typing into a widget must not snap back mid-edit). */
	void refreshMidiConfigWidgets();

	PluginUiMappingModel m_model;
	std::shared_ptr<CommonStrings> m_pCommonStrings;
	// Exposed MIDI config widget state.
	bool m_bNoteOffIgnore = false;
	int m_nActionChannel = -1;
	int m_nSendNoteOff = 0;
};

} // namespace H2Core

#endif // H2C_PLUGIN_UI_MAPPING_VIEW_H
