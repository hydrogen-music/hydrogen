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

// Window smoke (proposal 0006 TU2.1/TU2.2): a PluginUiWindow embedded on a
// test-created native parent — the child must be valid, mapped and parented,
// idle frames must not crash, and teardown must leave no child behind.
// Skips with a message when the platform has no display to create a parent
// on, or no usable GL/Metal to embed with (headless or virtualized CI); the
// manual spike gate (TU2.4) answers the real-host questions CI cannot.

#include "PluginUiLandingView.h"
#include "PluginUiMappingView.h"
#include "PluginUiMixerView.h"
#include "PluginUiWindow.h"

#include <plugin/HydrogenPlugin.h>

#include <imgui.h>

#include <cstdio>

#if defined( _WIN32 )
#include <windows.h>
#elif defined( __APPLE__ )
#import <Cocoa/Cocoa.h>
#else
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif

namespace {

bool g_bOk = true;

void check( bool bCondition, const char* sMessage ) {
	if ( ! bCondition ) {
		g_bOk = false;
		std::fprintf( stderr, "PLUGIN UI SMOKE: %s\n", sMessage );
	}
}

} // namespace

int main() {
#if defined( _WIN32 )
	// A desktop session is required to create the test parent. Creation
	// failure is indistinguishable from a headless session here, so it
	// skips rather than fails.
	const WNDCLASSW wc = { 0, DefWindowProcW, 0, 0, GetModuleHandle( nullptr ),
						   nullptr, nullptr, nullptr, nullptr,
						   L"h2pluginuismoke" };
	if ( RegisterClassW( &wc ) == 0 ) {
		std::printf( "PLUGIN UI SMOKE: SKIP — cannot register a window class\n" );
		return 0;
	}
	const HWND hParent = CreateWindowExW( 0, L"h2pluginuismoke", L"smoke parent",
										 WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0,
										 300, 200, nullptr, nullptr,
										 GetModuleHandle( nullptr ), nullptr );
	if ( hParent == nullptr ) {
		std::printf( "PLUGIN UI SMOKE: SKIP — no desktop to create a parent\n" );
		return 0;
	}
	{
		H2Core::PluginUiWindow window(
			reinterpret_cast<std::uintptr_t>( hParent ), 200, 150 );
		if ( ! window.isValid() ) {
			// Virtualized sessions may expose only the generic GDI
			// OpenGL 1.1 implementation; the GL3 backend's loader
			// cannot resolve against it. That is an environment gap,
			// not a verdict (TU2.2) — the manual spike gate covers
			// real Windows hosts.
			std::printf( "PLUGIN UI SMOKE: SKIP — no usable OpenGL for the embed\n" );
			DestroyWindow( hParent );
			return 0;
		}
		check( window.nativeHandle() != 0, "null native handle (TU2.2)" );
		for ( int i = 0; i < 5; ++i ) {
			window.idle();
		}
	}
	// The real views (UI-4): a plugin-backed window must construct the
	// three views and render frames without crashing — the renderers'
	// coverage (TU1.3: no logic, so the smoke is their test).
	{
		H2Core::HydrogenPlugin plugin( 44100, 512, 2 );
		plugin.activate( 44100, 512 );
		H2Core::PluginUiWindow viewWindow(
			reinterpret_cast<std::uintptr_t>( hParent ), 200, 150,
			&plugin );
		check( viewWindow.isValid(), "plugin-backed window invalid (UI-4)" );
		for ( int i = 0; i < 5; ++i ) {
			viewWindow.idle();
		}
	}
	DestroyWindow( hParent );
#elif defined( __APPLE__ )
	// A window server is required to create the test parent. Without one
	// (headless CI) the embed cannot be verified and skips; the manual
	// spike gate (TU2.4) covers real macOS hosts.
	NSWindow* pParent = [[NSWindow alloc]
		initWithContentRect: NSMakeRect( 0, 0, 300, 200 )
				  styleMask: NSWindowStyleMaskTitled
					backing: NSBackingStoreBuffered
					  defer: NO];
	if ( pParent == nullptr || pParent.contentView == nullptr ) {
		std::printf( "PLUGIN UI SMOKE: SKIP — no window server to create a parent\n" );
		return 0;
	}
	{
		H2Core::PluginUiWindow window(
			reinterpret_cast<std::uintptr_t>( pParent.contentView ), 200, 150 );
		if ( ! window.isValid() ) {
			// Metal may be unavailable in virtualized/headless sessions;
			// that is an environment gap, not a verdict (TU2.2).
			std::printf( "PLUGIN UI SMOKE: SKIP — no Metal device for the embed\n" );
			[pParent close];
			return 0;
		}
		check( window.nativeHandle() != 0, "null native handle (TU2.2)" );
		for ( int i = 0; i < 5; ++i ) {
			window.idle();
		}
	}
	// The real views (UI-4): a plugin-backed window must construct the
	// three views and render frames without crashing — the renderers'
	// coverage (TU1.3: no logic, so the smoke is their test).
	{
		H2Core::HydrogenPlugin plugin( 44100, 512, 2 );
		plugin.activate( 44100, 512 );
		H2Core::PluginUiWindow viewWindow(
			reinterpret_cast<std::uintptr_t>( pParent.contentView ), 200,
			150, &plugin );
		check( viewWindow.isValid(), "plugin-backed window invalid (UI-4)" );
		for ( int i = 0; i < 5; ++i ) {
			viewWindow.idle();
		}
	}
	[pParent close];
#else
	Display* pDisplay = XOpenDisplay( nullptr );
	if ( pDisplay == nullptr ) {
		std::printf( "PLUGIN UI SMOKE: SKIP — no DISPLAY\n" );
		return 0;
	}
	const Window parent = XCreateSimpleWindow(
		pDisplay, DefaultRootWindow( pDisplay ), 0, 0, 300, 200, 0,
		BlackPixel( pDisplay, DefaultScreen( pDisplay ) ),
		WhitePixel( pDisplay, DefaultScreen( pDisplay ) ) );
	XMapWindow( pDisplay, parent );
	XFlush( pDisplay );
	{
		H2Core::PluginUiWindow window(
			reinterpret_cast<std::uintptr_t>( parent ), 200, 150 );
		check( window.isValid(), "window invalid (TU2.1)" );
		const std::uintptr_t nChild = window.nativeHandle();
		check( nChild != 0, "null native handle (TU2.1)" );
		if ( nChild != 0 ) {
			// The child must be mapped and a direct child of the parent.
			XWindowAttributes attr = {};
			XGetWindowAttributes( pDisplay, static_cast<Window>( nChild ),
								  &attr );
			check( attr.map_state == IsViewable, "child not mapped (TU2.1)" );
			Window root = None;
			Window parentOfChild = None;
			Window* pChildren = nullptr;
			unsigned nChildren = 0;
			XQueryTree( pDisplay, static_cast<Window>( nChild ), &root,
						&parentOfChild, &pChildren, &nChildren );
			check( parentOfChild == parent,
				   "child not parented on the host parent (TU2.1)" );
			if ( pChildren != nullptr ) {
				XFree( pChildren );
			}
		}
		for ( int i = 0; i < 5; ++i ) {
			window.idle();
		}
		// Input translation (UI-4): synthetic events through the real
		// pump — the translated mouse state must track them. Real-host
		// pointer/keyboard routing stays with the manual spike gate
		// (TU2.4/TU6.2).
		if ( nChild != 0 ) {
			const Window child = static_cast<Window>( nChild );
			auto sendEvent = [ pDisplay, child ]( XEvent& ev ) {
				XSendEvent( pDisplay, child, False, 0, &ev );
				// The smoke's connection did not create the child, so the
				// event lands on the backend's connection; sync so it is
				// queued there before idle() pumps.
				XSync( pDisplay, False );
			};
			XEvent ev = {};
			ev.xmotion.type = MotionNotify;
			ev.xmotion.window = child;
			ev.xmotion.x = 30;
			ev.xmotion.y = 40;
			sendEvent( ev );
			window.idle();
			float fX = 0.0f;
			float fY = 0.0f;
			int nButtons = 0;
			window.mouseState( fX, fY, nButtons );
			check( fX == 30.0f && fY == 40.0f && nButtons == 0,
				   "motion not translated (UI-4)" );
			ev = {};
			ev.xbutton.type = ButtonPress;
			ev.xbutton.window = child;
			ev.xbutton.x = 30;
			ev.xbutton.y = 40;
			ev.xbutton.button = 1;
			sendEvent( ev );
			ev = {};
			ev.xkey.type = KeyPress;
			ev.xkey.window = child;
			ev.xkey.keycode = XKeysymToKeycode( pDisplay, XK_a );
			sendEvent( ev );
			window.idle();
			window.mouseState( fX, fY, nButtons );
			check( fX == 30.0f && fY == 40.0f && ( nButtons & 1 ) != 0,
				   "button/key press not translated (UI-4)" );
			ev = {};
			ev.xbutton.type = ButtonRelease;
			ev.xbutton.window = child;
			ev.xbutton.x = 30;
			ev.xbutton.y = 40;
			ev.xbutton.button = 1;
			sendEvent( ev );
			ev = {};
			ev.xkey.type = KeyRelease;
			ev.xkey.window = child;
			ev.xkey.keycode = XKeysymToKeycode( pDisplay, XK_a );
			sendEvent( ev );
			ev = {};
			ev.xcrossing.type = LeaveNotify;
			ev.xcrossing.window = child;
			ev.xcrossing.x = 30;
			ev.xcrossing.y = 40;
			ev.xcrossing.mode = NotifyNormal;
			ev.xcrossing.detail = NotifyAncestor;
			sendEvent( ev );
			window.idle();
			window.mouseState( fX, fY, nButtons );
			check( fX < 0.0f && fY < 0.0f && nButtons == 0,
				   "release/leave not translated (UI-4)" );
		}
	}
	// The real views (UI-4): a plugin-backed window must construct the
	// three views and render frames without crashing — the renderers'
	// coverage (TU1.3: no logic, so the smoke is their test).
	{
		H2Core::HydrogenPlugin plugin( 44100, 512, 2 );
		plugin.activate( 44100, 512 );
		H2Core::PluginUiWindow viewWindow(
			reinterpret_cast<std::uintptr_t>( parent ), 200, 150,
			&plugin );
		check( viewWindow.isValid(), "plugin-backed window invalid (UI-4)" );
		for ( int i = 0; i < 5; ++i ) {
			viewWindow.idle();
		}
	}
	// Teardown must leave no child window behind.
	XSync( pDisplay, false );
	{
		Window root = None;
		Window parentOfParent = None;
		Window* pChildren = nullptr;
		unsigned nChildren = 0;
		XQueryTree( pDisplay, parent, &root, &parentOfParent, &pChildren,
					&nChildren );
		check( nChildren == 0,
			   "child window outlived the window object (TU2.1)" );
		if ( pChildren != nullptr ) {
			XFree( pChildren );
		}
	}
	XDestroyWindow( pDisplay, parent );
	XCloseDisplay( pDisplay );
#endif

	// Direct view draws (UI-4): the tab bar gates the views inside the
	// window, so the embedded frames above never leave the default tab —
	// the REAPER spike crashed drawing the mixer view, unreachable there.
	// This block draws each view directly in a throwaway ImGui context:
	// no native window, no GL — draw-time asserts fire long before any
	// rendering. Each view gets its own full-size window and a
	// held-button sweep over its whole area: imgui asserts drag sources
	// on ID-less items only under mouse-down + hover, so the sweep
	// reproduces the crash condition wherever the widget sits.
	{
		H2Core::HydrogenPlugin plugin( 44100, 512, 2 );
		plugin.activate( 44100, 512 );
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.LogFilename = nullptr;
		io.DisplaySize = ImVec2( 400.0f, 300.0f );
		// No render backend here, so the atlas never builds via a
		// backend NewFrame — build the default bitmap font headless.
		io.Fonts->AddFontDefault();
		io.Fonts->Build();
		H2Core::PluginUiLandingView landingView( &plugin );
		H2Core::PluginUiMappingView mappingView( &plugin );
		H2Core::PluginUiMixerView mixerView( &plugin );
		// The scroll range of the last drawn frame — captured inside
		// the frame (GetScrollMaxX needs its window current) for the
		// mixer's scrollability check below.
		float nFrameScrollMaxX = -1.0f;
		const auto runFrame = [ &io, &nFrameScrollMaxX ]( auto&& draw ) {
			ImGui::NewFrame();
			ImGui::SetNextWindowPos( ImVec2( 0.0f, 0.0f ) );
			ImGui::SetNextWindowSize( io.DisplaySize );
			ImGui::Begin( "##directViews", nullptr,
						  ImGuiWindowFlags_NoTitleBar |
							  ImGuiWindowFlags_NoResize |
							  ImGuiWindowFlags_NoMove );
			draw();
			nFrameScrollMaxX = ImGui::GetScrollMaxX();
			ImGui::End();
			ImGui::Render();
		};
		const auto sweep = [ &io, &runFrame ]( auto&& draw ) {
			// The y range spans the window height — the strip columns
			// push the bus indicators deeper than the old row layout.
			for ( int nY = 0; nY <= 24; ++nY ) {
				for ( int nX = 0; nX <= 10; ++nX ) {
					io.AddMousePosEvent(
						20.0f + 36.0f * static_cast<float>( nX ),
						15.0f + 12.0f * static_cast<float>( nY ) );
					runFrame( draw );
				}
			}
		};
		// A warm-up frame before the press: a press ahead of the
		// first seen mouse position is "owned by the application"
		// (imgui click ownership) and suppresses hovering for the
		// whole drag. The window must also exist by then, so the
		// warm-up parks the mouse on the landing view's first
		// button — the press activates it and it stays active
		// across the sweeps, the state imgui asserts drag sources
		// under.
		io.AddMousePosEvent( 20.0f, 15.0f );
		runFrame( [ & ]() { landingView.draw(); } );
		// Held, never released: no widget fires (imgui triggers on
		// release), so the sweep only drags — the state it moves belongs
		// to this throwaway plugin instance. A 2D grid: the strip rows
		// are as narrow as the kit is small, so a single x would sail
		// past them, and the y stride stays under the bus indicator
		// row's height so the sweep cannot jump over it.
		io.AddMouseButtonEvent( 0, true );
		sweep( [ & ]() { landingView.draw(); } );
		sweep( [ & ]() { mappingView.draw(); } );
		sweep( [ & ]() { mixerView.draw(); } );
		// The mixer's bus row must stack below the strips, not chain
		// onto their line: the last item of its draw is the Master
		// bus slot, and it has to sit inside the window's width. The
		// height overflow is the vertical scrollbar's domain.
		check( ImGui::GetItemRectMax().x <= io.DisplaySize.x,
			   "mixer bus row beyond the window (UI-4)" );
		// The strips overflow any sane window width — the overflow
		// must stay reachable: once the mixer drew, the window has to
		// be horizontally scrollable.
		check( nFrameScrollMaxX > 0.0f,
			   "mixer overflow not horizontally scrollable (UI-4)" );
		ImGui::DestroyContext();
	}

	std::printf( g_bOk ? "PLUGIN UI SMOKE: PASSED\n"
					   : "PLUGIN UI SMOKE: FAILED\n" );
	return g_bOk ? 0 : 1;
}
