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

// Internal platform seam of PluginUiWindow (proposal 0006 TU2.3). One
// implementation per platform: X11 (Linux), Win32, macOS (NSView + Metal).
// The interface is deliberately small: native window, native events, draw
// context plumbing, and the imgui render-backend dispatch. The ImGui
// *context* is owned by PluginUiWindow::Impl, not by the backend.

#pragma once

#include <imgui.h>

#include <cstdint>
#include <memory>

namespace H2Core {

class PluginUiBackend {
public:
	virtual ~PluginUiBackend() = default;

	// False when the native window or draw context could not be created;
	// the destructor still releases whatever partial state exists.
	virtual bool valid() const = 0;
	// The native child window/view id handed back to the host.
	virtual std::uintptr_t nativeHandle() const = 0;
	// Host-DPI scale factor (1.0 when unknown).
	virtual double scaleFactor() const = 0;
	virtual void getSize( int& width, int& height ) const = 0;
	// Resizes the window (CLAP guiSetSize); the tracked size follows with
	// the next pumpEvents() at the latest (the resize's ConfigureNotify).
	virtual void setSize( int width, int height ) = 0;
	// Native input events observed (spike evidence, TU2.4).
	virtual unsigned inputEventCount() const = 0;
	// The last translated mouse state: window-local position plus a
	// bitmask of held buttons (1 left, 2 right, 4 middle — imgui's
	// button order, ImGuiMouseButton_*). (-1, -1, 0) while the pointer
	// is off the window or nothing was translated yet. Spike
	// diagnostics and the smoke's synthetic-input check.
	virtual void mouseState( float& x, float& y, int& nButtons ) const {
		x = -1.0f;
		y = -1.0f;
		nButtons = 0;
	}

	// Pump native events: resize bookkeeping and input translation into
	// the current ImGui context's IO. X11 drains its own queue here;
	// Win32 and macOS record into a queue from their WndProc/NSView
	// responder (which run on the host's pump, outside the per-instance
	// context discipline) and this drains it — same contract on every
	// platform: the caller guarantees the per-instance ImGui context is
	// current.
	virtual void pumpEvents() = 0;

	// True when native events asked for a frame since the last take —
	// input, resize, exposure/visibility on the window. Consumed by the
	// window's idle() to request a frame from its gate; latched, so one
	// take serves every event since the previous one.
	virtual bool takeFrameRequest() = 0;
	// False while the window cannot present a frame (unmapped, fully
	// obscured, hidden ancestor — e.g. the host's hidden tab): idle()
	// skips rendering but keeps the pending request for when it becomes
	// presentable again.
	virtual bool isPresentable() const = 0;

	// False for embedded child windows: they never close on their own
	// (the host owns their lifecycle). True for the parentless fallback
	// window (UI-5, TU5.2) after the user asked the window manager to
	// close it (WM_DELETE_WINDOW / WM_CLOSE / windowShouldClose) — the
	// owner's idle loop then tears the UI down.
	virtual bool shouldClose() const {
		return false;
	}

	// Draw-context + render-backend plumbing. The caller guarantees the
	// per-instance ImGui context is current whenever these run.
	virtual bool initRenderBackend() = 0;
	virtual void shutdownRenderBackend() = 0;
	// Make the draw target current and run the render backend's NewFrame.
	virtual void beginFrame() = 0;
	virtual void renderFrame( ImDrawData* data ) = 0;
	// Present the frame and release the draw target.
	virtual void endFrame() = 0;
};

// Creates the platform backend for a child window on parentNativeHandle; a
// zero parentNativeHandle creates a self-managed top-level window instead
// (the showInterface fallback, UI-5 TU5.2). The result may be invalid
// (partial native state is released by its destructor); callers check
// valid().
std::unique_ptr<PluginUiBackend> createPluginUiBackend(
	std::uintptr_t parentNativeHandle, int width, int height );

} // namespace H2Core
