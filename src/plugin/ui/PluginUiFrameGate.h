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

// The embedded basic UI's frame gate (ADR 0035's amendment): the pure-logic
// decision when a host idle tick owes a frame. Rendering is request-driven —
// input/resize/visibility events or an explicit requestFrame() — coalesced
// onto a ceiling interval; the first frame is owed unconditionally. Pure
// logic, no imgui types (proposal 0006 TU1.3): the unit suite links it
// headless through hydrogen-plugin-ui-models.

#pragma once

#include <chrono>

namespace H2Core {

class PluginUiFrameGate {
public:
	// nThresholdInterval: the ceiling between rendered frames. The window
	// passes 27 ms for the 30 Hz target (33 ms) minus slack — a 33 ms gate
	// would beat against the hosts' 30 ms idle timers down to ~17 Hz.
	explicit PluginUiFrameGate( std::chrono::milliseconds nThresholdInterval );

	// A frame is owed: an input/resize/visibility event arrived, or a view
	// with live state changed. Requests accumulate — one frame serves all.
	void request();

	// True when the tick at `now` (a steady-clock timestamp, monotonic)
	// owes the frame: a request is pending and either no frame was ever
	// rendered (the first frame is unconditional) or the interval since
	// the last rendered frame has elapsed.
	bool shouldRender( std::chrono::milliseconds now ) const;

	// The frame was rendered at `now` — serves every pending request and
	// advances the interval's reference.
	void markRendered( std::chrono::milliseconds now );

private:
	const std::chrono::milliseconds m_nThresholdInterval;
	// Pending starts true: a freshly opened window owes its first frame.
	bool m_bPendingRequest = true;
	bool m_bEverRendered = false;
	std::chrono::milliseconds m_nLastRenderTime = std::chrono::milliseconds( 0 );
};

} // namespace H2Core
