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
// window smoke covers it. Master and instrument strips (fader, pan
// knob, mute/solo, meters) — the master fixed beside the strips'
// horizontally scrollable lane; the bus routing lives in the mapping
// view's "Bus Output" section. Below the mute controls every strip
// carries a randomization section in its own recessed well — the
// strips the instrument's random pitch, the master the song-global
// humanize triple (velocity, timing, swing) — the shared well look
// sets the randomization controls apart from the level/pan/mute
// zone. The master column is wide enough for its labels, its knobs
// align with the strips' random-pitch knobs (the "Humanize" header
// heads the whole band), a 1px seam caps every well — the seams
// share one height across the band — the wells fill their panels'
// lower sections down to the bottom border (the same 1px panel rim
// as on their sides), and the scale labels render in
// the smaller companion font.

#ifndef H2C_PLUGIN_UI_MIXER_VIEW_H
#define H2C_PLUGIN_UI_MIXER_VIEW_H

#include <memory>

#include <imgui.h>
#include <QString>

#include "PluginUiMixerModel.h"

namespace H2Core {

class CommonStrings;
class HydrogenPlugin;
struct PluginUiPalette;

/** The last drawn frame's key mixer geometry, in screen space. The
 *  window smoke's white-box hook: imgui items are not queryable by
 *  id once the frame is over, so the view records the rects its
 *  layout decided and the smoke asserts the relations between them
 *  (the mute/solo pair's line, the master mute's span, the pan
 *  scale's tuck, the name truncation). */
struct PluginUiMixerLayout
{
	/** A frame drew at least one instrument strip. */
	bool bValid = false;
	/** Strip 0's column extent (its full-width spacer row). */
	ImVec2 stripMin{}, stripMax{};
	/** Strip 0's name text. */
	ImVec2 nameMin{}, nameMax{};
	/** Strip 0's mute and solo buttons. */
	ImVec2 muteMin{}, muteMax{}, soloMin{}, soloMax{};
	/** The master strip's column extent and its mute button. */
	ImVec2 masterStripMin{}, masterStripMax{}, masterMuteMin{},
		masterMuteMax{};
	/** Strip 0's pan knob and the L/R scale labels beside it. */
	ImVec2 knobMin{}, knobMax{}, labelLMin{}, labelLMax{}, labelRMin{},
		labelRMax{};
	/** Strip 0's randomization well, the random-pitch knob in it and
	 *  the "Pitch" label below. */
	ImVec2 randomMin{}, randomMax{}, randomKnobMin{}, randomKnobMax{},
		pitchLabelMin{}, pitchLabelMax{};
	/** The master's randomization well and its first knob (the
	 *  humanize velocity one). */
	ImVec2 masterRandomMin{}, masterRandomMax{}, masterRandomKnobMin{},
		masterRandomKnobMax{};
	/** The 1px seam capping each randomization well (strip 0's and
	 *  the master's). */
	ImVec2 seamMin{}, seamMax{}, masterSeamMin{}, masterSeamMax{};
	/** The master's "Humanize" header and the "Velocity" label — the
	 *  smoke asserts the widened master column fits them. */
	ImVec2 masterHeaderMin{}, masterHeaderMax{},
		masterVelocityLabelMin{}, masterVelocityLabelMax{};
	/** The first strip name wider than its column: the truncated
	 *  form shown and the full name (both empty when no name
	 *  overflows). */
	QString sNameShown, sNameFull;
	/** The randomization values the last frame showed — the smoke's
	 *  adoption probe diff base (engine-side changes must surface in
	 *  the next frame). */
	float fHumanizeVelocityShown = -1.0f;
	float fRandomPitchShown = -1.0f;
};

class PluginUiMixerView
{
public:
	explicit PluginUiMixerView( HydrogenPlugin* pPlugin );

	/** One frame of the mixer view's widget tree, painted with
	 *  @a palette. */
	void draw( const PluginUiPalette& palette );

	/** The view's model — the window polls it (meter diff) to decide
	 *  frame requests. */
	PluginUiMixerModel& model() { return m_model; }

	/** The last frame's recorded layout — the smoke test's hook. */
	const PluginUiMixerLayout& layout() const { return m_layout; }

private:
	/** A vertical fader with an L/R meter pair beside it; true when
	 * moved (the new value in @a fVolume). */
	bool drawFaderWithMeter( const char* sId, float& fVolume, float fPeakL,
							 float fPeakR, const PluginUiPalette& palette );
	/** A knob in the GUI Rotary's look (src/gui/src/Widgets/Rotary.cpp):
	 *  @a bCenter selects Type::Center (center marker, offset arc —
	 *  the pan knob) over Type::Normal (value arc from the lower
	 *  left), @a fSize the knob's diameter; true when turned (the new
	 *  value in @a fValue). */
	bool drawRotary( const char* sId, float& fValue, float fMin, float fMax,
					 bool bCenter, float fSize,
					 const PluginUiPalette& palette );

	PluginUiMixerModel m_model;
	std::shared_ptr<CommonStrings> m_pCommonStrings;
	PluginUiMixerLayout m_layout;
};

} // namespace H2Core

#endif // H2C_PLUGIN_UI_MIXER_VIEW_H
