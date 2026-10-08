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

// The landing view's renderer (proposal 0006 TU4.1): thin imgui draw
// calls over PluginUiLandingModel — no logic grows here (TU1.3); the
// window smoke covers it.

#ifndef H2C_PLUGIN_UI_LANDING_VIEW_H
#define H2C_PLUGIN_UI_LANDING_VIEW_H

#include <ImGuiFileDialog.h>

#include "PluginUiLandingModel.h"

namespace H2Core {

class HydrogenPlugin;

class PluginUiLandingView
{
public:
	explicit PluginUiLandingView( HydrogenPlugin* pPlugin );

	/** One frame of the landing view's widget tree. */
	void draw();

private:
	/** Re-reads the exposed MIDI config from the engine into the widget
	 * state — on construction and after every apply, never per frame
	 * (typing into a widget must not snap back mid-edit). */
	void refreshMidiConfigWidgets();

	PluginUiLandingModel m_model;
	IGFD::FileDialog m_fileDialog;
	// Pattern-load target state (the pattern row's widgets).
	int m_nPatternNumber = 0;
	bool m_bReplacePattern = false;
	// Exposed MIDI config widget state.
	bool m_bNoteOffIgnore = false;
	int m_nActionChannel = -1;
	int m_nSendNoteOff = 0;
};

} // namespace H2Core

#endif // H2C_PLUGIN_UI_LANDING_VIEW_H
