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

// macOS backend of PluginUiWindow (proposal 0006 TU2.2/TU2.3): an NSView
// child on the host-provided parent view — or, with a zero parent, an
// NSWindow around our view (the self-managed top-level fallback window,
// UI-5, TU5.2) — plus a CAMetalLayer-backed Metal context, driven
// frame-by-frame from idle(). CI-gated: no macOS build host locally — the
// AppVeyor macOS job compiles and runs the smoke against this (skipping
// when no window server/Metal device exists).
// Metal is the recorded de-risk item ("not well tested" upstream, proposal
// 0006 TU2.4) — the spike gate checks it on real hardware.
// Input wiring (TU6.2): H2PluginUiView (below) is the NSView this backend
// embeds — it overrides the NSResponder input methods and implements
// NSTextInputClient, so the macOS text-input manager delivers composed
// text through insertText: (the same route SDL2, GLFW and imgui's own
// OSX backend take). The responder runs on the host's AppKit pump — the
// same main thread as idle(), by AppKit's threading contract — but
// outside the per-instance ImGui context discipline, so it only records
// into a queue; pumpEvents() drains and translates under the
// caller-guaranteed current context (the Win32 backend's design).

#include "PluginUiBackend.h"

#include <imgui_impl_metal.h>

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h> // kVK_* virtual-key codes (imgui's OSX backend imports it too)
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstdint>
#include <new>
#include <memory>
#include <string>
#include <vector>

// The fallback window's close button (top-level only): windowShouldClose:
// records the request and refuses the close — the owner decides the
// teardown (hide()/destruction), the window stays until then.
// Objective-C class declarations are only legal at global scope (the
// ObjC runtime keeps classes in a flat global namespace), so this block
// must stay outside namespace H2Core.
@interface H2PluginUiWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) BOOL shouldClose;
@end
@implementation H2PluginUiWindowDelegate
- (BOOL)windowShouldClose: (NSWindow*)sender {
	_shouldClose = YES;
	return NO;
}
@end

namespace H2Core {
class PluginUiBackendMacOS;
}

// Carbon virtual-key code → ImGuiKey for the subset the basic UI needs
// (mirrors the X11/Win32 maps): navigation, digits, letters, minus
// (negative channel entry), modifiers. Everything else maps to
// ImGuiKey_None and only its composed text (if any) reaches imgui.
// Modifiers map to the real L/R keys — imgui derives the mod state from
// them, and its docs prefer them over the ImGuiMod_* aliases.
// Global scope (like the ObjC classes): H2PluginUiView calls it, and it
// must not depend on the backend class below.
static ImGuiKey imguiKeyFromKeyCode( unsigned short keyCode ) {
	switch ( keyCode ) {
	case kVK_Space: return ImGuiKey_Space;
	case kVK_Tab: return ImGuiKey_Tab;
	case kVK_Return: return ImGuiKey_Enter;
	case kVK_ANSI_KeypadEnter: return ImGuiKey_KeypadEnter;
	case kVK_Escape: return ImGuiKey_Escape;
	case kVK_Delete: return ImGuiKey_Backspace; // "delete" key = backspace
	case kVK_ForwardDelete: return ImGuiKey_Delete;
	case kVK_Home: return ImGuiKey_Home;
	case kVK_End: return ImGuiKey_End;
	case kVK_PageUp: return ImGuiKey_PageUp;
	case kVK_PageDown: return ImGuiKey_PageDown;
	case kVK_LeftArrow: return ImGuiKey_LeftArrow;
	case kVK_RightArrow: return ImGuiKey_RightArrow;
	case kVK_UpArrow: return ImGuiKey_UpArrow;
	case kVK_DownArrow: return ImGuiKey_DownArrow;
	case kVK_Help: return ImGuiKey_Insert; // no Insert key on macOS
	case kVK_ANSI_Minus: return ImGuiKey_Minus;
	case kVK_ANSI_Equal: return ImGuiKey_Equal;
	case kVK_ANSI_KeypadMinus: return ImGuiKey_KeypadSubtract;
	case kVK_ANSI_KeypadEquals: return ImGuiKey_KeypadEqual;
	case kVK_Shift: return ImGuiKey_LeftShift;
	case kVK_RightShift: return ImGuiKey_RightShift;
	case kVK_Control: return ImGuiKey_LeftCtrl;
	case kVK_RightControl: return ImGuiKey_RightCtrl;
	case kVK_Option: return ImGuiKey_LeftAlt;
	case kVK_RightOption: return ImGuiKey_RightAlt;
	case kVK_Command: return ImGuiKey_LeftSuper;
	case kVK_RightCommand: return ImGuiKey_RightSuper;
	case kVK_ANSI_A: return ImGuiKey_A;
	case kVK_ANSI_S: return ImGuiKey_S;
	case kVK_ANSI_D: return ImGuiKey_D;
	case kVK_ANSI_F: return ImGuiKey_F;
	case kVK_ANSI_H: return ImGuiKey_H;
	case kVK_ANSI_G: return ImGuiKey_G;
	case kVK_ANSI_Z: return ImGuiKey_Z;
	case kVK_ANSI_X: return ImGuiKey_X;
	case kVK_ANSI_C: return ImGuiKey_C;
	case kVK_ANSI_V: return ImGuiKey_V;
	case kVK_ANSI_B: return ImGuiKey_B;
	case kVK_ANSI_Q: return ImGuiKey_Q;
	case kVK_ANSI_W: return ImGuiKey_W;
	case kVK_ANSI_E: return ImGuiKey_E;
	case kVK_ANSI_R: return ImGuiKey_R;
	case kVK_ANSI_Y: return ImGuiKey_Y;
	case kVK_ANSI_T: return ImGuiKey_T;
	case kVK_ANSI_1: return ImGuiKey_1;
	case kVK_ANSI_2: return ImGuiKey_2;
	case kVK_ANSI_3: return ImGuiKey_3;
	case kVK_ANSI_4: return ImGuiKey_4;
	case kVK_ANSI_6: return ImGuiKey_6;
	case kVK_ANSI_5: return ImGuiKey_5;
	case kVK_ANSI_9: return ImGuiKey_9;
	case kVK_ANSI_7: return ImGuiKey_7;
	case kVK_ANSI_8: return ImGuiKey_8;
	case kVK_ANSI_0: return ImGuiKey_0;
	case kVK_ANSI_O: return ImGuiKey_O;
	case kVK_ANSI_U: return ImGuiKey_U;
	case kVK_ANSI_LeftBracket: return ImGuiKey_LeftBracket;
	case kVK_ANSI_I: return ImGuiKey_I;
	case kVK_ANSI_P: return ImGuiKey_P;
	case kVK_ANSI_RightBracket: return ImGuiKey_RightBracket;
	case kVK_ANSI_L: return ImGuiKey_L;
	case kVK_ANSI_J: return ImGuiKey_J;
	case kVK_ANSI_Quote: return ImGuiKey_Apostrophe;
	case kVK_ANSI_K: return ImGuiKey_K;
	case kVK_ANSI_Semicolon: return ImGuiKey_Semicolon;
	case kVK_ANSI_Backslash: return ImGuiKey_Backslash;
	case kVK_ANSI_Comma: return ImGuiKey_Comma;
	case kVK_ANSI_Slash: return ImGuiKey_Slash;
	case kVK_ANSI_N: return ImGuiKey_N;
	case kVK_ANSI_M: return ImGuiKey_M;
	case kVK_ANSI_Period: return ImGuiKey_Period;
	case kVK_ANSI_Grave: return ImGuiKey_GraveAccent;
	default: break;
	}
	// The keypad digit row is a contiguous range.
	if ( keyCode >= kVK_ANSI_Keypad0 && keyCode <= kVK_ANSI_Keypad9 ) {
		return static_cast<ImGuiKey>(
			ImGuiKey_Keypad0 + ( keyCode - kVK_ANSI_Keypad0 ) );
	}
	return ImGuiKey_None;
}

// The input-recording view (TU6.2): overrides the NSResponder input
// methods and implements NSTextInputClient so the macOS text-input
// manager works (composed/IME text arrives via insertText:). Global
// scope like the delegate above — ObjC classes live in the flat global
// runtime namespace. The @implementation sits below the backend class:
// the responder calls its record*() methods, which need the complete
// type.
@interface H2PluginUiView : NSView <NSTextInputClient>
// The owning backend (assign: the backend owns and outlives the view —
// the destructor nils this before any teardown call into AppKit).
@property (nonatomic, assign) H2Core::PluginUiBackendMacOS* backend;
// Created once in updateTrackingAreas (assign: addTrackingArea
// retained it).
@property (nonatomic, assign) NSTrackingArea* trackingArea;
@end

namespace H2Core {

// Not in an anonymous namespace: H2PluginUiView (above) holds a
// PluginUiBackendMacOS* and calls its record*() methods, which needs
// the class name from outside this namespace.
class PluginUiBackendMacOS : public PluginUiBackend {
public:
	PluginUiBackendMacOS( std::uintptr_t parentNativeHandle, int width,
						  int height )
		: m_parent( reinterpret_cast<NSView*>( parentNativeHandle ) ) {

		// Headless CI (no window server) yields no device: the backend
		// reports invalid and the smoke skips with a message.
		m_device = MTLCreateSystemDefaultDevice();
		if ( m_device == nil ) {
			return;
		}

		m_view = [[H2PluginUiView alloc]
			initWithFrame: NSMakeRect( 0, 0, width, height )];
		if ( m_view == nil ) {
			return;
		}
		// Before the view joins any window: joining one makes it the
		// first responder (viewDidMoveToWindow), and from then on the
		// responder records through this pointer. The members are
		// default-initialized by now (the ctor body runs after the
		// member initializers), so recording during the remaining ctor
		// calls is safe.
		m_view.backend = this;
		m_view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

		// The view owns the layer after setLayer: (it retains it), so our
		// alloc reference is released right away; the layer is reached
		// through the view from here on.
		CAMetalLayer* layer = [[CAMetalLayer alloc] init];
		layer.device = m_device;
		layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
		layer.drawableSize = CGSizeMake( width, height );
		m_view.layer = layer;
		[layer release];
		m_view.wantsLayer = YES;

		if ( m_parent != nil ) {
			[m_parent addSubview: m_view];
		} else {
			// The self-managed top-level fallback window (UI-5, TU5.2):
			// decorated, with the close button the delegate surfaces as
			// shouldClose() ("Hydrogen" is the brand name, not a
			// translatable string). releasedWhenClosed NO — the owner
			// releases the window, never the close button.
			m_window = [[NSWindow alloc]
				initWithContentRect: NSMakeRect( 0, 0, width, height )
				styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
					| NSWindowStyleMaskMiniaturizable
					| NSWindowStyleMaskResizable
				backing: NSBackingStoreBuffered defer: NO];
			if ( m_window == nil ) {
				return;
			}
			m_window.releasedWhenClosed = NO;
			m_window.title = @"Hydrogen";
			m_delegate = [[H2PluginUiWindowDelegate alloc] init];
			m_window.delegate = m_delegate;
			// The window retains the content view; our own reference is
			// released in the destructor after the window's.
			m_window.contentView = m_view;
			[m_window makeKeyAndOrderFront: nil];
		}

		m_commandQueue = [m_device newCommandQueue];
		if ( m_commandQueue == nil ) {
			return;
		}

		// Losing the host app's active status stops keyboard delivery
		// without any responder callback (the view stays first
		// responder of a non-key window) — a key held across an app
		// switch would stay "down" in imgui. imgui's OSX backend
		// observes the same pair. Blocks over another ObjC class: the
		// token is retained here and removed first in the destructor,
		// so the captured pointer is always live.
		PluginUiBackendMacOS* backend = this;
		m_appActiveObserver = [[[NSNotificationCenter defaultCenter]
			addObserverForName: NSApplicationDidBecomeActiveNotification
			object: nil queue: nil
			usingBlock: ^(NSNotification*) {
				backend->recordFocus( true );
			}] retain];
		m_appInactiveObserver = [[[NSNotificationCenter defaultCenter]
			addObserverForName: NSApplicationDidResignActiveNotification
			object: nil queue: nil
			usingBlock: ^(NSNotification*) {
				backend->recordFocus( false );
			}] retain];

		m_valid = true;
	}

	~PluginUiBackendMacOS() override {
		// The notification blocks capture this: they go before
		// anything else is torn down.
		if ( m_appActiveObserver != nil ) {
			[[NSNotificationCenter defaultCenter]
				removeObserver: m_appActiveObserver];
			[m_appActiveObserver release];
		}
		if ( m_appInactiveObserver != nil ) {
			[[NSNotificationCenter defaultCenter]
				removeObserver: m_appInactiveObserver];
			[m_appInactiveObserver release];
		}
		// Cut the responder's back-pointer before any AppKit teardown
		// call. The destructor runs synchronously on the main thread —
		// AppKit delivers events only from the run loop, so none can
		// interleave — but a nil pointer keeps the invariant explicit
		// (and beats a dangling one) if a responder ever fires during
		// window/view teardown anyway.
		if ( m_view != nil ) {
			m_view.backend = nullptr;
		}
		// endFrame() releases the frame-scoped objects; this guards
		// destruction between the frame calls. The window goes before
		// the view — it retains the view as its content view.
		if ( m_drawable != nil ) {
			[m_drawable release];
		}
		if ( m_renderPassDescriptor != nil ) {
			[m_renderPassDescriptor release];
		}
		if ( m_window != nil ) {
			m_window.delegate = nil;
			[m_window release];
		}
		if ( m_delegate != nil ) {
			[m_delegate release];
		}
		if ( m_view != nil ) {
			[m_view removeFromSuperview];
			[m_view release];
		}
		if ( m_commandQueue != nil ) {
			[m_commandQueue release];
		}
		if ( m_device != nil ) {
			[m_device release];
		}
	}

	bool valid() const override {
		return m_valid;
	}

	std::uintptr_t nativeHandle() const override {
		return reinterpret_cast<std::uintptr_t>( m_view );
	}

	double scaleFactor() const override {
		// The parent view is the one living in a host window; our own view
		// may not be in one yet at smoke time. The fallback window is our
		// own, so the view reaches it either way.
		if ( m_scaleFactor <= 0.0 ) {
			NSWindow* window = [m_view window];
			if ( window == nil && m_parent != nil ) {
				window = [m_parent window];
			}
			if ( window != nil ) {
				m_scaleFactor = [window backingScaleFactor];
			}
		}
		return m_scaleFactor > 0.0 ? m_scaleFactor : 1.0;
	}

	void getSize( int& width, int& height ) const override {
		if ( m_view != nil ) {
			const NSSize size = [m_view bounds].size;
			m_width = static_cast<int>( size.width );
			m_height = static_cast<int>( size.height );
		}
		width = m_width;
		height = m_height;
	}

	void setSize( int width, int height ) override {
		if ( ! m_valid ) {
			return;
		}
		m_width = width;
		m_height = height;
		// The fallback window sizes through its content view; an embedded
		// view is sized directly (the parent resizes it otherwise).
		if ( m_window != nil ) {
			[m_window setContentSize: NSMakeSize( width, height )];
		}
		else {
			[m_view setFrameSize: NSMakeSize( width, height )];
		}
	}

	unsigned inputEventCount() const override {
		return m_inputEvents;
	}

	void mouseState( float& x, float& y, int& nButtons ) const override {
		x = m_fMouseX;
		y = m_fMouseY;
		nButtons = m_nMouseButtons;
	}

	void pumpEvents() override {
		// Drain what the view responder recorded on the host's AppKit
		// pump (TU6.2). The caller guarantees the per-instance ImGui
		// context is current, so events translate straight into its
		// IO.
		for ( const MacInputEvent& ev : m_inputQueue ) {
			ImGuiIO& io = ImGui::GetIO();
			switch ( ev.kind ) {
			case MacInputEvent::Kind::MouseMove:
				io.AddMousePosEvent( ev.x, ev.y );
				m_fMouseX = ev.x;
				m_fMouseY = ev.y;
				break;
			case MacInputEvent::Kind::MouseLeave:
				// imgui's off-window sentinel: -FLT_MAX drops the
				// hover from hit-testing (its own backends do the
				// same).
				io.AddMousePosEvent( -FLT_MAX, -FLT_MAX );
				m_fMouseX = -1.0f;
				m_fMouseY = -1.0f;
				break;
			case MacInputEvent::Kind::MouseButton:
				io.AddMouseButtonEvent( ev.button, ev.pressed );
				m_nMouseButtons = ev.pressed
					? ( m_nMouseButtons | ( 1 << ev.button ) )
					: ( m_nMouseButtons & ~( 1 << ev.button ) );
				break;
			case MacInputEvent::Kind::Wheel:
				io.AddMouseWheelEvent( ev.wheelX, ev.wheelY );
				break;
			case MacInputEvent::Kind::Key:
				if ( ev.key != ImGuiKey_None ) {
					io.AddKeyEvent( ev.key, ev.pressed );
				}
				break;
			case MacInputEvent::Kind::Char:
				io.AddInputCharactersUTF8( ev.text.c_str() );
				break;
			case MacInputEvent::Kind::Focus:
				io.AddFocusEvent( ev.focused );
				break;
			}
			++m_inputEvents;
		}
		m_inputQueue.clear();
	}

	// TU6.2: H2PluginUiView's responder overrides record native input
	// here. AppKit delivers on the main thread — the same thread
	// idle() runs on, by AppKit's contract — but outside the
	// per-instance ImGui context discipline, so recording is all that
	// happens here; pumpEvents() drains under the caller-guaranteed
	// context.
	void recordMouseMove( float x, float y ) {
		MacInputEvent ev = {};
		ev.kind = MacInputEvent::Kind::MouseMove;
		ev.x = x;
		ev.y = y;
		m_inputQueue.push_back( ev );
	}
	void recordMouseLeave() {
		MacInputEvent ev = {};
		ev.kind = MacInputEvent::Kind::MouseLeave;
		m_inputQueue.push_back( ev );
	}
	void recordMouseButton( int button, bool pressed, float x, float y ) {
		MacInputEvent ev = {};
		ev.kind = MacInputEvent::Kind::MouseButton;
		ev.button = button;
		ev.pressed = pressed;
		ev.x = x;
		ev.y = y;
		m_inputQueue.push_back( ev );
	}
	void recordWheel( float x, float y ) {
		MacInputEvent ev = {};
		ev.kind = MacInputEvent::Kind::Wheel;
		ev.wheelX = x;
		ev.wheelY = y;
		m_inputQueue.push_back( ev );
	}
	void recordKey( ImGuiKey key, bool pressed ) {
		MacInputEvent ev = {};
		ev.kind = MacInputEvent::Kind::Key;
		ev.key = key;
		ev.pressed = pressed;
		m_inputQueue.push_back( ev );
	}
	void recordText( const char* utf8 ) {
		// UTF8String returns NULL for unpaired-surrogate strings.
		if ( utf8 == nullptr ) {
			return;
		}
		MacInputEvent ev = {};
		ev.kind = MacInputEvent::Kind::Char;
		ev.text = utf8;
		m_inputQueue.push_back( ev );
	}
	void recordFocus( bool focused ) {
		MacInputEvent ev = {};
		ev.kind = MacInputEvent::Kind::Focus;
		ev.focused = focused;
		m_inputQueue.push_back( ev );
	}

	bool shouldClose() const override {
		return m_delegate != nil && m_delegate.shouldClose;
	}

	bool initRenderBackend() override {
		return ImGui_ImplMetal_Init( m_device );
	}

	void shutdownRenderBackend() override {
		ImGui_ImplMetal_Shutdown();
	}

	void beginFrame() override {
		@autoreleasepool {
			CAMetalLayer* layer = (CAMetalLayer*)m_view.layer;
			// drawableSize is in pixels; bounds are in points.
			const NSSize bounds = [m_view bounds].size;
			const double scale = scaleFactor();
			layer.drawableSize = CGSizeMake(
				bounds.width * scale, bounds.height * scale );

			// imgui 1.92 moved the render pass descriptor into
			// ImGui_ImplMetal_NewFrame: the backend reads its attachment
			// textures there to configure the frame's pipelines, so the
			// drawable must be acquired before ImGui::NewFrame(), not at
			// draw-data time. The frame spans the beginFrame()/
			// renderFrame() virtual calls, so no autorelease pool can
			// cover it: the drawable and descriptor are retained here
			// and released in endFrame().
			m_drawable = [[layer nextDrawable] retain];
			m_renderPassDescriptor = [[MTLRenderPassDescriptor
				renderPassDescriptor] retain];
			if ( m_drawable != nil ) {
				m_renderPassDescriptor.colorAttachments[ 0 ].texture =
					[m_drawable texture];
				m_renderPassDescriptor.colorAttachments[ 0 ].loadAction =
					MTLLoadActionClear;
				m_renderPassDescriptor.colorAttachments[ 0 ].storeAction =
					MTLStoreActionStore;
				m_renderPassDescriptor.colorAttachments[ 0 ].clearColor =
					MTLClearColorMake( 0.1, 0.1, 0.12, 1.0 );
			}
			ImGui_ImplMetal_NewFrame( m_renderPassDescriptor );
		}
	}

	void renderFrame( ImDrawData* data ) override {
		@autoreleasepool {
			if ( m_drawable == nil ) {
				return; // no drawable this frame (e.g. occluded) — skip
			}
			id<MTLCommandBuffer> commandBuffer = [m_commandQueue commandBuffer];
			id<MTLRenderCommandEncoder> encoder =
				[commandBuffer renderCommandEncoderWithDescriptor:
					m_renderPassDescriptor];
			ImGui_ImplMetal_RenderDrawData( data, commandBuffer, encoder );
			[encoder endEncoding];
			[commandBuffer presentDrawable: m_drawable];
			[commandBuffer commit];
		}
	}

	void endFrame() override {
		// Metal presented inside renderFrame (the command buffer is
		// autoreleased); the frame-scoped drawable and render pass
		// descriptor retained in beginFrame() are released here.
		if ( m_drawable != nil ) {
			[m_drawable release];
			m_drawable = nil;
		}
		if ( m_renderPassDescriptor != nil ) {
			[m_renderPassDescriptor release];
			m_renderPassDescriptor = nil;
		}
	}

private:
	// Native input recorded by the view responder (TU6.2) — the macOS
	// twin of the Win32 backend's InputEvent. insertText: can deliver
	// multi-unit UTF-8, so text is a string, not one code unit.
	struct MacInputEvent {
		enum class Kind {
			None, MouseMove, MouseLeave, MouseButton, Wheel, Key, Char,
			Focus
		};
		Kind kind = Kind::None;
		float x = 0.0f;           // MouseMove/MouseButton: view coords
		float y = 0.0f;
		int button = 0;           // MouseButton: 0 left, 1 right,
		                          // 2 middle (imgui order)
		bool pressed = false;     // MouseButton/Key
		float wheelX = 0.0f;      // Wheel: notches
		float wheelY = 0.0f;
		ImGuiKey key = ImGuiKey_None;
		std::string text;         // Char: UTF-8 (composed, multi-unit)
		bool focused = false;     // Focus
	};

	NSView* m_parent = nil;
	H2PluginUiView* m_view = nil;
	NSWindow* m_window = nil;
	H2PluginUiWindowDelegate* m_delegate = nil;
	id m_appActiveObserver = nil;
	id m_appInactiveObserver = nil;
	id<MTLDevice> m_device = nil;
	id<MTLCommandQueue> m_commandQueue = nil;
	id<CAMetalDrawable> m_drawable = nil;
	MTLRenderPassDescriptor* m_renderPassDescriptor = nil;
	mutable int m_width = 0;
	mutable int m_height = 0;
	mutable double m_scaleFactor = 0.0;
	std::vector<MacInputEvent> m_inputQueue;
	unsigned m_inputEvents = 0;
	float m_fMouseX = -1.0f;
	float m_fMouseY = -1.0f;
	int m_nMouseButtons = 0;
	bool m_valid = false;
};

} // namespace H2Core

@implementation H2PluginUiView

- (BOOL)acceptsFirstResponder {
	return YES;
}

// Focus rides the responder chain (TU6.2): the view records focus when
// it becomes / resigns the window's first responder — the same
// FocusIn/FocusOut semantics the X11 backend translates.
- (BOOL)becomeFirstResponder {
	self.backend->recordFocus( true );
	return [super becomeFirstResponder];
}

- (BOOL)resignFirstResponder {
	self.backend->recordFocus( false );
	return [super resignFirstResponder];
}

- (void)viewDidMoveToWindow {
	// First-responder status is what routes keyboard input to the view
	// (imgui's OSX backend does the same from its responder).
	[super viewDidMoveToWindow];
	if ( self.window != nil ) {
		[self.window makeFirstResponder: self];
	}
}

- (void)updateTrackingAreas {
	// One bounds-following tracking area: mouseMoved/mouseEntered/
	// mouseExited only reach a responder through a tracking area.
	// NSTrackingInVisibleRect glues it to the visible rect, so it
	// never needs rebuilding on resize.
	[super updateTrackingAreas];
	if ( self.trackingArea == nil ) {
		self.trackingArea = [[NSTrackingArea alloc]
			initWithRect: NSZeroRect
			options: NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved
				| NSTrackingActiveAlways | NSTrackingInVisibleRect
			owner: self
			userInfo: nil];
		[self addTrackingArea: self.trackingArea];
		// addTrackingArea retained it; the property is assign.
		[self.trackingArea release];
	}
}

// View-local coordinates with a top-left origin (imgui's convention):
// NSView is not flipped by default.
- (void)recordMove: (NSEvent*)event {
	NSPoint point =
		[self convertPoint: event.locationInWindow fromView: nil];
	self.backend->recordMouseMove( (float)point.x,
		(float)( self.bounds.size.height - point.y ) );
}

- (void)recordButton: (NSEvent*)event pressed: (BOOL)pressed {
	NSPoint point =
		[self convertPoint: event.locationInWindow fromView: nil];
	// Cocoa numbers buttons 0 left, 1 right, 2 middle — imgui's order.
	self.backend->recordMouseButton( (int)event.buttonNumber, pressed,
		(float)point.x,
		(float)( self.bounds.size.height - point.y ) );
}

- (void)mouseMoved: (NSEvent*)event {
	[self recordMove: event];
}
- (void)mouseDragged: (NSEvent*)event {
	[self recordMove: event];
}
- (void)rightMouseDragged: (NSEvent*)event {
	[self recordMove: event];
}
- (void)otherMouseDragged: (NSEvent*)event {
	[self recordMove: event];
}
- (void)mouseEntered: (NSEvent*)event {
	// The enter position seeds the hover like X11's EnterNotify.
	[self recordMove: event];
}
- (void)mouseExited: (NSEvent*)event {
	self.backend->recordMouseLeave();
}

- (void)mouseDown: (NSEvent*)event {
	[self recordButton: event pressed: YES];
}
- (void)mouseUp: (NSEvent*)event {
	[self recordButton: event pressed: NO];
}
- (void)rightMouseDown: (NSEvent*)event {
	[self recordButton: event pressed: YES];
}
- (void)rightMouseUp: (NSEvent*)event {
	[self recordButton: event pressed: NO];
}
- (void)otherMouseDown: (NSEvent*)event {
	[self recordButton: event pressed: YES];
}
- (void)otherMouseUp: (NSEvent*)event {
	[self recordButton: event pressed: NO];
}

- (void)scrollWheel: (NSEvent*)event {
	// Momentum-phase "cancelled" events carry huge deltas (the
	// two-finger deceleration tap, macOS 12.1+) — imgui's backend
	// drops them; so do we.
	if ( event.phase == NSEventPhaseCancelled ) {
		return;
	}
	double wheelX = event.scrollingDeltaX;
	double wheelY = event.scrollingDeltaY;
	if ( event.hasPreciseScrollingDeltas ) {
		// Pixel deltas → notches (imgui's own scaling).
		wheelX *= 0.01;
		wheelY *= 0.01;
	}
	if ( wheelX != 0.0 || wheelY != 0.0 ) {
		self.backend->recordWheel( (float)wheelX, (float)wheelY );
	}
}

- (void)keyDown: (NSEvent*)event {
	// imgui deduplicates repeats itself; only state changes are
	// recorded (imgui's OSX backend skips repeats the same way).
	if ( ! event.isARepeat ) {
		self.backend->recordKey(
			imguiKeyFromKeyCode( event.keyCode ), true );
	}
	// The macOS text-input manager: composed/IME text comes back
	// through insertText: (the SDL2/GLFW/imgui route).
	[self interpretKeyEvents: @[ event ]];
}

- (void)keyUp: (NSEvent*)event {
	if ( ! event.isARepeat ) {
		self.backend->recordKey(
			imguiKeyFromKeyCode( event.keyCode ), false );
	}
}

- (void)flagsChanged: (NSEvent*)event {
	// macOS delivers modifier changes as one "flags changed" event
	// whose keyCode says which key it was; the class-wide flag says
	// whether that modifier class is now held (there is no per-key
	// up/down for modifiers).
	ImGuiKey key = ImGuiKey_None;
	NSEventModifierFlags flag = 0;
	switch ( event.keyCode ) {
	case kVK_Shift: key = ImGuiKey_LeftShift;
		flag = NSEventModifierFlagShift; break;
	case kVK_RightShift: key = ImGuiKey_RightShift;
		flag = NSEventModifierFlagShift; break;
	case kVK_Control: key = ImGuiKey_LeftCtrl;
		flag = NSEventModifierFlagControl; break;
	case kVK_RightControl: key = ImGuiKey_RightCtrl;
		flag = NSEventModifierFlagControl; break;
	case kVK_Option: key = ImGuiKey_LeftAlt;
		flag = NSEventModifierFlagOption; break;
	case kVK_RightOption: key = ImGuiKey_RightAlt;
		flag = NSEventModifierFlagOption; break;
	case kVK_Command: key = ImGuiKey_LeftSuper;
		flag = NSEventModifierFlagCommand; break;
	case kVK_RightCommand: key = ImGuiKey_RightSuper;
		flag = NSEventModifierFlagCommand; break;
	default: break;
	}
	if ( key != ImGuiKey_None ) {
		self.backend->recordKey(
			key, ( event.modifierFlags & flag ) != 0 );
	}
}

#pragma mark - NSTextInputClient (composed/IME text input)

- (void)insertText: (id)aString
	replacementRange: (NSRange)replacementRange {
	NSString* characters;
	if ( [aString isKindOfClass: [NSAttributedString class]] ) {
		characters = [aString string];
	}
	else {
		characters = (NSString*)aString;
	}
	if ( characters != nil ) {
		self.backend->recordText( characters.UTF8String );
	}
}

// Unhandled key equivalents (layout-dependent commands) — swallowed to
// keep the system alert sound out of the plugin window.
- (void)doCommandBySelector: (SEL)selector {
}

// The rest of the protocol: no marked-text state (composition happens
// in the input manager's own window) and no selection — the same stubs
// imgui's OSX backend ships. The IME placement rect stays empty: like
// the X11 backend's PreeditNothing trade-off, composition UI placement
// is out of the basic UI's scope.
- (nullable NSAttributedString*)attributedSubstringForProposedRange:
		(NSRange)range
	actualRange: (nullable NSRangePointer)actualRange {
	return nil;
}
- (NSUInteger)characterIndexForPoint: (NSPoint)point {
	return 0;
}
- (NSRect)firstRectForCharacterRange: (NSRange)range
	actualRange: (nullable NSRangePointer)actualRange {
	return NSZeroRect;
}
- (BOOL)hasMarkedText {
	return NO;
}
- (NSRange)markedRange {
	return NSMakeRange( NSNotFound, 0 );
}
- (NSRange)selectedRange {
	return NSMakeRange( NSNotFound, 0 );
}
- (void)setMarkedText: (nonnull id)string
	selectedRange: (NSRange)selectedRange
	replacementRange: (NSRange)replacementRange {
}
- (void)unmarkText {
}
- (nonnull NSArray<NSAttributedStringKey>*)validAttributesForMarkedText {
	return @[];
}

@end

namespace H2Core {

std::unique_ptr<PluginUiBackend> createPluginUiBackend(
	std::uintptr_t parentNativeHandle, int width, int height ) {
	auto* backend = new ( std::nothrow ) PluginUiBackendMacOS(
		parentNativeHandle, width, height );
	if ( backend == nullptr ) {
		return nullptr;
	}
	return std::unique_ptr<PluginUiBackend>( backend );
}

} // namespace H2Core
