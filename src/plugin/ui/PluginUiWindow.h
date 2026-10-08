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

// The embedded basic plugin UI's window (ADR 0035, proposal 0006 TU2.3): a
// native child window on a host-provided parent, a per-instance Dear ImGui
// context, and one frame per idle(). The platform mechanism lives behind
// PluginUiBackend (X11 child window + GLX adapted from DPF/Pugl, Win32 child
// HWND, NSView + Metal).
//
// Per-instance ImGui context discipline (ADR 0035's recorded obligation):
// ImGui contexts are thread-local and hosts idle multiple UI instances
// sequentially on one thread, so every entry point switches the context
// first.

#pragma once

#include <cstdint>

namespace H2Core {

class HydrogenPlugin;

class PluginUiWindow {
public:
	// parentNativeHandle is the host-provided native parent (X11 Window on
	// Linux, HWND on Windows, NSView* on macOS). pPlugin is the plugin
	// engine the views ride on (UI-4); null keeps the spike panel (the
	// smoke's plugin-less path).
	PluginUiWindow( std::uintptr_t parentNativeHandle, int width, int height,
					HydrogenPlugin* pPlugin = nullptr );
	~PluginUiWindow();

	PluginUiWindow( const PluginUiWindow& ) = delete;
	PluginUiWindow& operator=( const PluginUiWindow& ) = delete;

	// False when the native window, the draw context, or the ImGui context
	// could not be created; idle() is a safe no-op then.
	bool isValid() const;
	// The native child to hand back to the host (the LV2 widget / CLAP
	// window handle). 0 while invalid.
	std::uintptr_t nativeHandle() const;
	// Host-DPI scale factor (Xft.dpi / 96 on X11); 1.0 when unknown.
	double scaleFactor() const;
	// Native input events observed since construction — live evidence for
	// the spike gate's focus/IME routing question (proposal 0006 TU2.4).
	unsigned inputEventCount() const;
	// The last translated mouse state (window-local position + held-button
	// bitmask); (-1, -1, 0) while off-window or untranslated.
	void mouseState( float& x, float& y, int& nButtons ) const;

	// One frame: pump native events, drive the per-instance ImGui context,
	// render, present.
	void idle();

private:
	// The platform seam and the ImGui context live in the implementation;
	// this header stays free of platform and imgui types so plugin-format
	// shims can include it anywhere.
	class Impl;
	Impl* m_pImpl;
};

} // namespace H2Core
