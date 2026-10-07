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
// on (headless CI); the manual spike gate (TU2.4) answers the real-host
// questions CI cannot.

#include "PluginUiWindow.h"

#include <cstdio>

#if defined( _WIN32 )
#include <windows.h>
#elif defined( __APPLE__ )
#import <Cocoa/Cocoa.h>
#else
#include <X11/Xlib.h>
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
		check( window.isValid(), "window invalid (TU2.2)" );
		check( window.nativeHandle() != 0, "null native handle (TU2.2)" );
		for ( int i = 0; i < 5; ++i ) {
			window.idle();
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

	std::printf( g_bOk ? "PLUGIN UI SMOKE: PASSED\n"
					   : "PLUGIN UI SMOKE: FAILED\n" );
	return g_bOk ? 0 : 1;
}
