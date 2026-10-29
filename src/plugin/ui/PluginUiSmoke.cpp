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
// idle frames must not crash, and teardown must leave no child behind — plus
// the parentless fallback window (UI-5, TU5.2) with its WM close protocol.
// Skips with a message when the platform has no display to create a parent
// on, or no usable GL/Metal to embed with (headless or virtualized CI); the
// manual spike gate (TU2.4) answers the real-host questions CI cannot.

#include "PluginUiLandingView.h"
#include "PluginUiMappingView.h"
#include "PluginUiMixerView.h"
#include "PluginUiWindow.h"

#include <plugin/HydrogenPlugin.h>

#include <core/config.h>
#include <core/CommonStrings.h>
#include <core/Hydrogen.h>
#include <core/Object.h>
#include <core/Helpers/Filesystem.h>
#include <core/Logger.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QTemporaryDir>

#include <imgui.h>

#include <chrono>
#include <cstdio>
#include <thread>

#if defined( _WIN32 )
#include <windows.h>
#elif defined( __APPLE__ )
#import <Cocoa/Cocoa.h>
#else
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
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

int main( int argc, char** argv ) {
	// Deterministic data roots, the unit suite's setupEnvironment
	// pattern: the HydrogenPlugin instances below bootstrap
	// Base/Filesystem on first construction, resolving the system data
	// folder from the baked install prefix — green only where hydrogen
	// happens to be installed (a developer box's dist/), and an empty
	// fallback kit on fresh checkouts, which left the mixer without
	// the strip overflow the UI-4 check requires. Bootstrapping first
	// wins: the plugin's bootstrap is a no-op in a process that
	// already did them. System data = the source data/ (GMRockKit
	// ships there); user data = a transient dir, so no host machine
	// state (drumkits, preferences) leaks into the render.
	QCoreApplication app( argc, argv );
	// The tmp dir must exist before the transient dir and the log file
	// below are placed in/next to it — the suite's lesson.
	if ( ! H2Core::Filesystem::mkdir( H2Core::Filesystem::tmpDir() ) ) {
		std::fprintf( stderr, "PLUGIN UI SMOKE: FAILED — no tmp dir\n" );
		return 1;
	}
	QTemporaryDir userDataDir(
		H2Core::Filesystem::tmpDir() + "-smoke-user-XXXXX" );
	userDataDir.setAutoRemove( false );
	if ( ! userDataDir.isValid() ) {
		std::fprintf( stderr,
					  "PLUGIN UI SMOKE: FAILED — no transient user dir\n" );
		return 1;
	}
	// A per-process log file in the plugin's own naming scheme — the
	// smoke is a plugin process; the plugin instances below find a
	// logger available and reuse it.
	const QFileInfo logInfo( H2Core::Filesystem::logFilePath() );
	const QString sLogPath = H2Core::Filesystem::tmpDir() + "/" +
		logInfo.completeBaseName() +
		QString( "_%1_plugin_ui_smoke." )
			.arg( QCoreApplication::applicationPid() ) +
		logInfo.suffix();
	H2Core::Logger* pLogger = H2Core::Logger::bootstrap(
		H2Core::Logger::Error | H2Core::Logger::Warning |
			H2Core::Logger::Info,
		sLogPath,
		H2Core::Logger::Option::Timestamps |
			H2Core::Logger::Option::Append );
	if ( pLogger == nullptr ) {
		std::fprintf( stderr, "PLUGIN UI SMOKE: FAILED — no logger\n" );
		return 1;
	}
	H2Core::Base::bootstrap( pLogger, false );
	if ( ! H2Core::Filesystem::bootstrap(
			 pLogger, CMAKE_SOURCE_DIR "/data", userDataDir.path(),
			 userDataDir.path() + "/hydrogen.conf", sLogPath ) ) {
		std::fprintf( stderr,
					  "PLUGIN UI SMOKE: FAILED — Filesystem bootstrap\n" );
		return 1;
	}
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

			// The trickle queue vs. the on-demand gate: a quick tap's
			// press+release land in one pump batch, and imgui's event
			// trickling (io.ConfigInputTrickleEventQueue, default on)
			// processes the press while deferring the release to a
			// later frame. That frame must actually happen — the gate's
			// frames are request-driven, and the release's arrival
			// already spent its request on the frame that ate the
			// press. A stranded release leaves the key stuck down in
			// io.KeysData, and its auto-repeat (io.KeyRepeatDelay) runs
			// away inside an active InputText — the caret drifts on its
			// own and every later edit lands at the moving position.
			ev = {};
			ev.xkey.type = KeyPress;
			ev.xkey.window = child;
			ev.xkey.keycode = XKeysymToKeycode( pDisplay, XK_Left );
			sendEvent( ev );
			ev.xkey.type = KeyRelease;
			sendEvent( ev );
			// Idles past the gate's interval: whatever frames the
			// pending state deserves get every chance to render.
			for ( int i = 0; i < 6; ++i ) {
				std::this_thread::sleep_for(
					std::chrono::milliseconds( 35 ) );
				window.idle();
			}
			bool bAnyKeyDown = false;
			ImGuiIO& io = ImGui::GetIO();
			for ( ImGuiKey key = ImGuiKey_NamedKey_BEGIN;
				  key < ImGuiKey_NamedKey_END;
				  key = static_cast<ImGuiKey>( key + 1 ) ) {
				if ( io.KeysData[ key - ImGuiKey_NamedKey_BEGIN ].Down ) {
					bAnyKeyDown = true;
					break;
				}
			}
			check( ! bAnyKeyDown,
				   "quick key tap leaves the key stuck down (trickle "
				   "release stranded by the on-demand gate)" );
		}
		// getSize/setSize (UI-5, TU5.3): the host sizes the embedded child
		// (CLAP guiSetSize); the tracked size must follow — the immediate
		// bookkeeping plus the ConfigureNotify the resize produces.
		int nWidth = 0;
		int nHeight = 0;
		window.getSize( nWidth, nHeight );
		check( nWidth == 200 && nHeight == 150,
			   "getSize() != initial size (TU5.3)" );
		window.setSize( 400, 300 );
		window.idle(); // pumps the ConfigureNotify the resize produced
		window.getSize( nWidth, nHeight );
		check( nWidth == 400 && nHeight == 300,
			   "getSize() != setSize() (TU5.3)" );
		// Frame governance (ADR 0035's amendment): idle() renders only owed
		// frames — requests (input/resize/visibility, or an explicit
		// requestFrame()) coalesced onto the 30 Hz ceiling. The settle
		// sequence first guarantees a fresh rendered frame, so the rapid
		// idles below sit inside the gate's interval whatever the resize
		// above left pending.
		const Window child = static_cast<Window>( nChild );
		auto sendVisibility = [ pDisplay, child ]( int state ) {
			if ( child == None ) {
				return;
			}
			XEvent ev = {};
			ev.xvisibility.type = VisibilityNotify;
			ev.xvisibility.window = child;
			ev.xvisibility.state = state;
			XSendEvent( pDisplay, child, False, 0, &ev );
			// The smoke's connection did not create the child, so the
			// event lands on the backend's connection; sync so it is
			// queued there before idle() pumps.
			XSync( pDisplay, False );
		};
		// A real desktop may deliver a fully-obscured VisibilityNotify at
		// map time (the child maps under whatever covers it) — force the
		// unobscured state so the assertions below are deterministic on a
		// developer box too (CI's Xvfb never obscures).
		sendVisibility( VisibilityUnobscured );
		std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
		window.requestFrame();
		window.idle();
		const unsigned nFramesRendered = window.renderedFrameCount();
		for ( int i = 0; i < 10; ++i ) {
			window.idle();
		}
		check( window.renderedFrameCount() == nFramesRendered,
			   "unrequested idles rendered frames (frame gate)" );
		std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
		window.requestFrame();
		window.idle();
		check( window.renderedFrameCount() == nFramesRendered + 1,
			   "requested frame not rendered after the interval (frame gate)" );
		window.idle();
		check( window.renderedFrameCount() == nFramesRendered + 1,
			   "unrequested idle after a render rendered a frame (frame gate)" );
		// Presentability gating: an obscured window renders nothing, and
		// its reveal owes exactly one frame — the backend's event latch
		// and the window's reveal transition both request (macOS has no
		// event to latch with; on X11 either path serves the frame).
		sendVisibility( VisibilityFullyObscured );
		window.idle();
		check( window.renderedFrameCount() == nFramesRendered + 1,
			   "obscured idle rendered a frame (frame gate)" );
		std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
		sendVisibility( VisibilityUnobscured );
		window.idle();
		check( window.renderedFrameCount() == nFramesRendered + 2,
			   "reveal did not render a frame (frame gate)" );
		window.idle();
		check( window.renderedFrameCount() == nFramesRendered + 2,
			   "unrequested idle after a reveal rendered a frame (frame gate)" );
		// Context discipline: a rendered frame leaves the per-instance GL
		// context current on the host's thread — no per-frame
		// make-current/uncurrent churn (the multi-instance host-thread
		// contract, ADR 0035).
		check( glXGetCurrentContext() != nullptr,
			   "GL context not current after a rendered frame" );
	}
	// The real views (UI-4): a plugin-backed window must construct the
	// three views and render frames without crashing — the renderers'
	// coverage (TU1.3: no logic, so the smoke is their test). On X11 it
	// also carries the live-data governance checks (round 7): the
	// publish cycle feeds the models through the window's
	// engine-event-sink adapter, and the idle poll turns engine data
	// changes into frame requests.
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

		// Live data (round 7): the same governance contract as
		// input/resize/visibility above, but driven by the engine
		// alone. The engine's selected pattern is the change driver —
		// a locked core setter (bNeedsLock) that renames the info
		// snapshot's "Pattern N" in the next publish cycle.
		const Window viewChild =
			static_cast<Window>( viewWindow.nativeHandle() );
		auto sendViewVisibility = [ pDisplay, viewChild ]( int state ) {
			if ( viewChild == None ) {
				return;
			}
			XEvent ev = {};
			ev.xvisibility.type = VisibilityNotify;
			ev.xvisibility.window = viewChild;
			ev.xvisibility.state = state;
			XSendEvent( pDisplay, viewChild, False, 0, &ev );
			XSync( pDisplay, False );
		};
		// Deterministic on a developer box too (see above).
		sendViewVisibility( VisibilityUnobscured );
		auto pHydrogen = plugin.getHydrogen();
		check( pHydrogen != nullptr, "plugin-backed window without engine" );

		// Settle: the first info snapshot (one publish cycle after the
		// adapter registered, at the latest) is itself a change — let
		// it arrive and render before counting.
		std::this_thread::sleep_for( std::chrono::milliseconds( 120 ) );
		viewWindow.requestFrame();
		viewWindow.idle();
		const unsigned nDataFrames = viewWindow.renderedFrameCount();
		viewWindow.idle();
		check( viewWindow.renderedFrameCount() == nDataFrames,
			   "unrequested idle rendered a frame (live data)" );

		// An engine data change requests exactly one frame.
		pHydrogen->setSelectedPatternNumber( 1 );
		std::this_thread::sleep_for( std::chrono::milliseconds( 120 ) );
		viewWindow.idle();
		check( viewWindow.renderedFrameCount() == nDataFrames + 1,
			   "engine data change did not render a frame (live data)" );
		viewWindow.idle();
		check( viewWindow.renderedFrameCount() == nDataFrames + 1,
			   "unchanged engine data rendered a frame (live data)" );

		// A change while obscured renders nothing until the reveal —
		// the poll consumes the diff while hidden, the pending request
		// survives, and the reveal owes exactly one frame.
		sendViewVisibility( VisibilityFullyObscured );
		viewWindow.idle();
		check( viewWindow.renderedFrameCount() == nDataFrames + 1,
			   "obscured idle rendered a frame (live data)" );
		pHydrogen->setSelectedPatternNumber( 2 );
		std::this_thread::sleep_for( std::chrono::milliseconds( 120 ) );
		viewWindow.idle();
		check( viewWindow.renderedFrameCount() == nDataFrames + 1,
			   "obscured data change rendered a frame (live data)" );
		std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
		sendViewVisibility( VisibilityUnobscured );
		viewWindow.idle();
		check( viewWindow.renderedFrameCount() == nDataFrames + 2,
			   "reveal after a data change did not render one frame (live data)" );
		viewWindow.idle();
		check( viewWindow.renderedFrameCount() == nDataFrames + 2,
			   "unrequested idle after a reveal rendered a frame (live data)" );

		// Tab switching (the REAPER crash class): a tab branch that
		// forgets its EndTabItem leaves imgui's ID stack non-empty and
		// the debug assert aborts the HOST process. No gate sees this
		// unless a frame is rendered with the tab selected — so click
		// through every tab of the window's tab bar; the survival of
		// these frames IS the check.
		//
		// The click points are measured, not hardcoded: a probe frame
		// in the window's own imgui context (same style, font, and
		// display size) replicates the window's tab structure and
		// reads the tab rects off the items — the points track any
		// scale factor, font, or translation. The probe runs twice: on
		// a tab bar's first frame imgui reports empty rects for
		// appearing tabs, so the second frame carries the layout.
		const auto pCommonStrings = plugin.commonStrings();
		const QByteArray sTabGeneral =
			pCommonStrings->getTabGeneralDialog().toUtf8();
		const QByteArray sTabMapping = pCommonStrings->getMappingTab().toUtf8();
		const QByteArray sTabMixer = pCommonStrings->getMixerTab().toUtf8();
		const auto itemCenter = [] {
			const ImVec2 mn = ImGui::GetItemRectMin();
			const ImVec2 mx = ImGui::GetItemRectMax();
			return ImVec2( ( mn.x + mx.x ) * 0.5f, ( mn.y + mx.y ) * 0.5f );
		};
		ImVec2 tabCenters[ 3 ] = {}; // general, mapping, mixer
		for ( int nProbeFrame = 0; nProbeFrame < 2; ++nProbeFrame ) {
			ImGui::NewFrame();
			ImGui::SetNextWindowPos( ImVec2( 0.0f, 0.0f ) );
			ImGui::SetNextWindowSize( ImGui::GetIO().DisplaySize );
			ImGui::Begin( "##tabProbe", nullptr,
						  ImGuiWindowFlags_NoTitleBar |
							  ImGuiWindowFlags_NoResize |
							  ImGuiWindowFlags_NoMove |
							  ImGuiWindowFlags_NoCollapse |
							  ImGuiWindowFlags_NoSavedSettings |
							  // The probe window outlives its frames in
							  // imgui's window list; without this it
							  // would shadow the real window in the
							  // mouse hit-test of the next frame and
							  // swallow the first tab click's hover
							  // lead-in.
							  ImGuiWindowFlags_NoMouseInputs );
			if ( ImGui::BeginTabBar( "##tabs" ) ) {
				const bool bGeneral =
					ImGui::BeginTabItem( sTabGeneral.constData() );
				tabCenters[ 0 ] = itemCenter();
				if ( bGeneral ) {
					ImGui::EndTabItem();
				}
				const bool bMapping =
					ImGui::BeginTabItem( sTabMapping.constData() );
				tabCenters[ 1 ] = itemCenter();
				if ( bMapping ) {
					ImGui::EndTabItem();
				}
				const bool bMixer =
					ImGui::BeginTabItem( sTabMixer.constData() );
				tabCenters[ 2 ] = itemCenter();
				if ( bMixer ) {
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			ImGui::End();
			ImGui::EndFrame();
		}
		const ImVec2 probeDisplaySize = ImGui::GetIO().DisplaySize;
		for ( const auto& center : tabCenters ) {
			check( center.x > 0.0f && center.x < probeDisplaySize.x &&
					   center.y > 0.0f && center.y < probeDisplaySize.y,
				   "tab probe measured a degenerate rect (tab switch)" );
		}
		// The probe's frames are the only ones where the real window is
		// not submitted, and imgui's mouse hit-test only considers
		// windows of the previous frame — one plain frame restores the
		// real window's hoverability before the first click's lead-in.
		std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
		viewWindow.requestFrame();
		viewWindow.idle();
		auto sendViewButton = [ pDisplay, viewChild ]( int type, int x, int y ) {
			if ( viewChild == None ) {
				return;
			}
			XEvent ev = {};
			ev.xbutton.type = type;
			ev.xbutton.window = viewChild;
			ev.xbutton.x = x;
			ev.xbutton.y = y;
			ev.xbutton.button = 1;
			// The smoke's connection did not create the child, so the
			// event lands on the backend's connection; sync so it is
			// queued there before idle() pumps.
			XSendEvent( pDisplay, viewChild, False, 0, &ev );
			XSync( pDisplay, False );
		};
		auto clickViewTab = [ & ]( const ImVec2& center ) {
			const int x = static_cast<int>( center.x );
			const int y = static_cast<int>( center.y );
			// Tab items carry ImGuiButtonFlags_AllowOverlap, and imgui
			// defers the hover of such items for one frame after the
			// mouse arrives (ItemHoverable requires the item to have
			// been the hovered id of the previous frame). The motion
			// therefore needs its own rendered frame before the press
			// lands, or the tab is not hoverable yet and the click is
			// ignored. The backend only tracks the position from
			// motion events, and a click is only processed by a
			// rendered frame — the sleep + request pair guarantees one
			// past the 30 Hz gate, for the hover lead-in, the press,
			// and the release each.
			XEvent ev = {};
			ev.xmotion.type = MotionNotify;
			ev.xmotion.window = viewChild;
			ev.xmotion.x = x;
			ev.xmotion.y = y;
			XSendEvent( pDisplay, viewChild, False, 0, &ev );
			XSync( pDisplay, False );
			std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
			viewWindow.requestFrame();
			viewWindow.idle();
			sendViewButton( ButtonPress, x, y );
			std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
			viewWindow.requestFrame();
			viewWindow.idle();
			sendViewButton( ButtonRelease, x, y );
			std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
			viewWindow.requestFrame();
			viewWindow.idle();
		};
		const unsigned nTabFrames = viewWindow.renderedFrameCount();
		// The window tabs: the Mapping and Mixer renderers have never
		// been drawn by a gate before this; then back to General.
		clickViewTab( tabCenters[ 1 ] );
		clickViewTab( tabCenters[ 2 ] );
		clickViewTab( tabCenters[ 0 ] );
		check( viewWindow.renderedFrameCount() > nTabFrames,
			   "tab clicks did not render frames (tab switch)" );
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

	// The fallback top-level window (UI-5, TU5.2): parentless
	// construction — the self-managed window for hosts without
	// ui:parent (showInterface). The WM close button arrives as a
	// WM_DELETE_WINDOW client message (the backend registers the
	// protocol) and must surface as shouldClose() so the host's idle
	// loop learns to tear the UI down.
	unsigned nRootChildrenBefore = 0;
	{
		Window root = None;
		Window parentOfRoot = None;
		Window* pChildren = nullptr;
		XQueryTree( pDisplay, DefaultRootWindow( pDisplay ), &root,
					&parentOfRoot, &pChildren, &nRootChildrenBefore );
		if ( pChildren != nullptr ) {
			XFree( pChildren );
		}
	}
	{
		H2Core::PluginUiWindow topLevel( 0, 200, 150 );
		check( topLevel.isValid(), "top-level window invalid (TU5.2)" );
		const std::uintptr_t nTopLevel = topLevel.nativeHandle();
		check( nTopLevel != 0, "null top-level native handle (TU5.2)" );
		for ( int i = 0; i < 5; ++i ) {
			topLevel.idle();
		}
		check( ! topLevel.shouldClose(),
			   "top-level window close before any close event (TU5.2)" );
		if ( nTopLevel != 0 ) {
			XEvent ev = {};
			ev.xclient.type = ClientMessage;
			ev.xclient.window = static_cast<Window>( nTopLevel );
			ev.xclient.message_type =
				XInternAtom( pDisplay, "WM_PROTOCOLS", False );
			ev.xclient.format = 32;
			ev.xclient.data.l[ 0 ] =
				XInternAtom( pDisplay, "WM_DELETE_WINDOW", False );
			XSendEvent( pDisplay, static_cast<Window>( nTopLevel ), False, 0,
						&ev );
			XSync( pDisplay, False );
			topLevel.idle();
			check( topLevel.shouldClose(),
				   "WM_DELETE_WINDOW not surfaced as shouldClose (TU5.2)" );
		}
	}
	// The top-level window must not outlive its object either. With a
	// window manager the WM owns the frame around our window, so the
	// root's child count only returns to the baseline once the WM
	// processed the destroy — poll briefly instead of asserting a race.
	{
		bool bGone = false;
		for ( int i = 0; i < 50 && ! bGone; ++i ) {
			XSync( pDisplay, false );
			unsigned nChildrenNow = 0;
			Window root = None;
			Window parentOfRoot = None;
			Window* pChildren = nullptr;
			XQueryTree( pDisplay, DefaultRootWindow( pDisplay ), &root,
						&parentOfRoot, &pChildren, &nChildrenNow );
			if ( pChildren != nullptr ) {
				XFree( pChildren );
			}
			bGone = nChildrenNow <= nRootChildrenBefore;
			if ( ! bGone ) {
				std::this_thread::sleep_for(
					std::chrono::milliseconds( 20 ) );
			}
		}
		check( bGone,
			   "top-level window outlived the window object (TU5.2)" );
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
