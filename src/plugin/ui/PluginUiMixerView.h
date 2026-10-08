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

// The mixer view's renderer (proposal 0006 TU4.3/TU4.4): thin imgui draw
// calls over PluginUiMixerModel — no logic grows here (TU1.3); the
// window smoke covers it.

#ifndef H2C_PLUGIN_UI_MIXER_VIEW_H
#define H2C_PLUGIN_UI_MIXER_VIEW_H

#include "PluginUiMixerModel.h"

namespace H2Core {

class HydrogenPlugin;

class PluginUiMixerView
{
public:
	explicit PluginUiMixerView( HydrogenPlugin* pPlugin );

	/** One frame of the mixer view's widget tree. */
	void draw();

private:
	/** A vertical fader with an L/R meter pair beside it; true when
	 * moved (the new value in @a fVolume). */
	bool drawFaderWithMeter( const char* sId, float& fVolume, float fPeakL,
							 float fPeakR );

	PluginUiMixerModel m_model;
};

} // namespace H2Core

#endif // H2C_PLUGIN_UI_MIXER_VIEW_H
