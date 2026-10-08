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

// X11 backend of PluginUiWindow (proposal 0006 TU2.3): a child window on the
// host-provided parent plus a GLX context, driven frame-by-frame from idle().
// The window/GLX mechanism is adapted from DISTRHO/Pugl (src/x11.c,
// src/x11_gl.c, ISC license) and the per-instance ImGui context discipline
// from DISTRHO/DPF-Widgets (opengl/DearImGui.cpp, ISC license).

#include "PluginUiBackend.h"

#include <imgui_impl_opengl3.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>
#include <GL/glx.h>

#include <cfloat>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <memory>

namespace H2Core {

namespace {

// X11 KeySym → ImGuiKey for the subset the basic UI needs: navigation,
// digits, letters, minus (negative channel entry), modifiers. Everything
// else maps to ImGuiKey_None and only its composed text (if any) reaches
// imgui. Modifiers map to the real L/R keys — imgui derives the mod state
// from them, and its docs prefer them over the ImGuiMod_* aliases.
ImGuiKey imguiKeyFromKeySym( KeySym keySym ) {
	switch ( keySym ) {
	case XK_BackSpace: return ImGuiKey_Backspace;
	case XK_Tab: return ImGuiKey_Tab;
	case XK_Return: return ImGuiKey_Enter;
	case XK_KP_Enter: return ImGuiKey_KeypadEnter;
	case XK_Escape: return ImGuiKey_Escape;
	case XK_Delete: return ImGuiKey_Delete;
	case XK_Home: return ImGuiKey_Home;
	case XK_End: return ImGuiKey_End;
	case XK_Page_Up: return ImGuiKey_PageUp;
	case XK_Page_Down: return ImGuiKey_PageDown;
	case XK_Left: return ImGuiKey_LeftArrow;
	case XK_Right: return ImGuiKey_RightArrow;
	case XK_Up: return ImGuiKey_UpArrow;
	case XK_Down: return ImGuiKey_DownArrow;
	case XK_Insert: return ImGuiKey_Insert;
	case XK_space: return ImGuiKey_Space;
	case XK_minus: return ImGuiKey_Minus;
	case XK_equal: return ImGuiKey_Equal;
	case XK_KP_Subtract: return ImGuiKey_KeypadSubtract;
	case XK_KP_Equal: return ImGuiKey_KeypadEqual;
	case XK_Shift_L: return ImGuiKey_LeftShift;
	case XK_Shift_R: return ImGuiKey_RightShift;
	case XK_Control_L: return ImGuiKey_LeftCtrl;
	case XK_Control_R: return ImGuiKey_RightCtrl;
	case XK_Alt_L: return ImGuiKey_LeftAlt;
	case XK_Alt_R: return ImGuiKey_RightAlt;
	case XK_Super_L: return ImGuiKey_LeftSuper;
	case XK_Super_R: return ImGuiKey_RightSuper;
	default: break;
	}
	if ( keySym >= XK_0 && keySym <= XK_9 ) {
		return static_cast<ImGuiKey>( ImGuiKey_0 + ( keySym - XK_0 ) );
	}
	if ( keySym >= XK_KP_0 && keySym <= XK_KP_9 ) {
		return static_cast<ImGuiKey>(
			ImGuiKey_Keypad0 + ( keySym - XK_KP_0 ) );
	}
	if ( keySym >= XK_a && keySym <= XK_z ) {
		return static_cast<ImGuiKey>( ImGuiKey_A + ( keySym - XK_a ) );
	}
	if ( keySym >= XK_A && keySym <= XK_Z ) {
		return static_cast<ImGuiKey>( ImGuiKey_A + ( keySym - XK_A ) );
	}
	return ImGuiKey_None;
}

class PluginUiBackendX11 : public PluginUiBackend {
public:
	PluginUiBackendX11( std::uintptr_t parentNativeHandle, int width, int height )
		: m_parent( static_cast<Window>( parentNativeHandle ) )
		, m_width( width )
		, m_height( height ) {
		m_display = XOpenDisplay( nullptr );
		if ( m_display == nullptr ) {
			return;
		}

		// GL visual selection (Pugl x11_gl.c): the first FBConfig matching
		// the RGBA double-buffered window-bit attributes.
		static const int fbAttribs[] = {
			GLX_X_RENDERABLE, True,
			GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
			GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
			GLX_RENDER_TYPE, GLX_RGBA_BIT,
			GLX_DOUBLEBUFFER, True,
			None
		};
		int fbCount = 0;
		GLXFBConfig* fbConfigs = glXChooseFBConfig(
			m_display, DefaultScreen( m_display ), fbAttribs, &fbCount );
		if ( fbConfigs == nullptr || fbCount < 1 ) {
			if ( fbConfigs != nullptr ) {
				XFree( fbConfigs );
			}
			return;
		}
		m_fbConfig = fbConfigs[ 0 ];
		XFree( fbConfigs );

		XVisualInfo* visualInfo =
			glXGetVisualFromFBConfig( m_display, m_fbConfig );
		if ( visualInfo == nullptr ) {
			return;
		}
		m_visualInfo = visualInfo;

		// Child window on the host parent (Pugl x11.c puglRealize): the
		// colormap comes from the GL visual; the event mask is the input
		// surface pumpEvents translates (UI-4).
		m_colormap = XCreateColormap(
			m_display, m_parent, visualInfo->visual, AllocNone );
		XSetWindowAttributes attr = {};
		attr.colormap = m_colormap;
		attr.event_mask = ButtonPressMask | ButtonReleaseMask
			| EnterWindowMask | LeaveWindowMask
			| ExposureMask
			| FocusChangeMask
			| KeyPressMask | KeyReleaseMask
			| PointerMotionMask
			| StructureNotifyMask
			| VisibilityChangeMask;
		m_window = XCreateWindow(
			m_display, m_parent, 0, 0, m_width, m_height, 0,
			visualInfo->depth, InputOutput, visualInfo->visual,
			CWColormap | CWEventMask, &attr );
		if ( m_window == None ) {
			return;
		}

		// Legacy context (Pugl's fallback path): the imgui GL3 backend
		// targets compatibility-profile shaders, so CreateContextAttribsARB
		// buys nothing here.
		m_context = glXCreateNewContext(
			m_display, m_fbConfig, GLX_RGBA_TYPE, nullptr, True );
		if ( m_context == nullptr ) {
			return;
		}

		// Embedded = visible immediately (DPF WindowPrivateData).
		XMapWindow( m_display, m_window );
		XFlush( m_display );

		m_valid = true;
	}

	~PluginUiBackendX11() override {
		// Pugl teardown order: GL context, window, colormap, display.
		if ( m_display != nullptr ) {
			if ( m_context != nullptr ) {
				glXDestroyContext( m_display, m_context );
			}
			if ( m_window != None ) {
				XDestroyWindow( m_display, m_window );
			}
			if ( m_colormap != None ) {
				XFreeColormap( m_display, m_colormap );
			}
			XFlush( m_display );
			XCloseDisplay( m_display );
		}
		if ( m_visualInfo != nullptr ) {
			XFree( m_visualInfo );
		}
	}

	bool valid() const override {
		return m_valid;
	}

	std::uintptr_t nativeHandle() const override {
		return static_cast<std::uintptr_t>( m_window );
	}

	double scaleFactor() const override {
		// Host DPI (Pugl x11.c): Xft.dpi from the resource database / 96.
		if ( m_scaleFactor <= 0.0 && m_display != nullptr ) {
			char* resourceString = XResourceManagerString( m_display );
			if ( resourceString != nullptr ) {
				XrmDatabase database = XrmGetStringDatabase( resourceString );
				if ( database != nullptr ) {
					char* type = nullptr;
					XrmValue value = {};
					if ( XrmGetResource( database, "Xft.dpi", "Xft.dpi",
										  &type, &value )
						 && value.addr != nullptr ) {
						const double dpi = std::atof( value.addr );
						if ( dpi > 0.0 ) {
							m_scaleFactor = dpi / 96.0;
						}
					}
					XrmDestroyDatabase( database );
				}
			}
			if ( m_scaleFactor <= 0.0 ) {
				m_scaleFactor = 1.0;
			}
		}
		return m_scaleFactor;
	}

	void getSize( int& width, int& height ) const override {
		width = m_width;
		height = m_height;
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
		// Pugl dispatchX11Events: flush, then drain the queue. The caller
		// guarantees the per-instance ImGui context is current, so events
		// translate straight into its IO (UI-4).
		XFlush( m_display );
		while ( XEventsQueued( m_display, QueuedAfterReading ) > 0 ) {
			XEvent event = {};
			XNextEvent( m_display, &event );
			if ( event.xany.window != m_window ) {
				continue;
			}
			switch ( event.type ) {
			case ConfigureNotify:
				m_width = event.xconfigure.width;
				m_height = event.xconfigure.height;
				break;
			case MotionNotify:
				translateMousePosition( event.xmotion.x, event.xmotion.y );
				++m_inputEvents;
				break;
			case EnterNotify:
				translateMousePosition( event.xcrossing.x, event.xcrossing.y );
				++m_inputEvents;
				break;
			case LeaveNotify:
				// imgui's off-window sentinel: -FLT_MAX drops the hover
				// from hit-testing (its own backends do the same).
				ImGui::GetIO().AddMousePosEvent( -FLT_MAX, -FLT_MAX );
				m_fMouseX = -1.0f;
				m_fMouseY = -1.0f;
				++m_inputEvents;
				break;
			case ButtonPress:
			case ButtonRelease:
				translateButton( event.xbutton );
				++m_inputEvents;
				break;
			case KeyPress:
			case KeyRelease:
				translateKey( event.xkey );
				++m_inputEvents;
				break;
			case FocusIn:
				ImGui::GetIO().AddFocusEvent( true );
				++m_inputEvents;
				break;
			case FocusOut:
				ImGui::GetIO().AddFocusEvent( false );
				++m_inputEvents;
				break;
			default:
				break;
			}
		}
	}

	bool initRenderBackend() override {
		if ( ! glXMakeCurrent( m_display, m_window, m_context ) ) {
			return false;
		}
		const bool ok = ImGui_ImplOpenGL3_Init();
		glXMakeCurrent( m_display, None, nullptr );
		return ok;
	}

	void shutdownRenderBackend() override {
		if ( glXMakeCurrent( m_display, m_window, m_context ) ) {
			ImGui_ImplOpenGL3_Shutdown();
			glXMakeCurrent( m_display, None, nullptr );
		}
	}

	void beginFrame() override {
		glXMakeCurrent( m_display, m_window, m_context );
		ImGui_ImplOpenGL3_NewFrame();
	}

	void renderFrame( ImDrawData* data ) override {
		ImGui_ImplOpenGL3_RenderDrawData( data );
	}

	void endFrame() override {
		glXSwapBuffers( m_display, m_window );
		glXMakeCurrent( m_display, None, nullptr );
	}

private:
	// Input translation (UI-4): native events → the current ImGui IO.
	// The caller guarantees the per-instance context is current.

	void translateMousePosition( int x, int y ) {
		ImGui::GetIO().AddMousePosEvent( static_cast<float>( x ),
										static_cast<float>( y ) );
		m_fMouseX = static_cast<float>( x );
		m_fMouseY = static_cast<float>( y );
	}

	void translateButton( const XButtonEvent& button ) {
		ImGuiIO& io = ImGui::GetIO();
		const bool bPressed = ( button.type == ButtonPress );
		// Position rides along: hosts may deliver a click without a
		// preceding motion (pointer warps, synthetic events).
		translateMousePosition( button.x, button.y );
		switch ( button.button ) {
		case 1:
			io.AddMouseButtonEvent( 0, bPressed );
			m_nMouseButtons = bPressed ? ( m_nMouseButtons | 1 )
				: ( m_nMouseButtons & ~1 );
			break;
		case 2:
			io.AddMouseButtonEvent( 1, bPressed );
			m_nMouseButtons = bPressed ? ( m_nMouseButtons | 2 )
				: ( m_nMouseButtons & ~2 );
			break;
		case 3:
			io.AddMouseButtonEvent( 2, bPressed );
			m_nMouseButtons = bPressed ? ( m_nMouseButtons | 4 )
				: ( m_nMouseButtons & ~4 );
			break;
		case 4:
		case 5:
			// The wheel reports as a press/release pair; only the press
			// carries a notch.
			if ( bPressed ) {
				io.AddMouseWheelEvent( 0.0f,
									   button.button == 4 ? 1.0f : -1.0f );
			}
			break;
		case 6:
		case 7:
			if ( bPressed ) {
				io.AddMouseWheelEvent( button.button == 6 ? 1.0f : -1.0f,
									   0.0f );
			}
			break;
		default:
			break;
		}
	}

	void translateKey( XKeyEvent& key ) {
		ImGuiIO& io = ImGui::GetIO();
		const bool bPressed = ( key.type == KeyPress );
		KeySym keySym = NoSymbol;
		char sText[ 32 ] = {};
		// XLookupString (no XIM yet — IME is TU6.2): keysym and composed
		// text in one pass.
		const int nText = XLookupString( &key, sText, sizeof( sText ) - 1,
										  &keySym, nullptr );
		const ImGuiKey imguiKey = imguiKeyFromKeySym( keySym );
		if ( imguiKey != ImGuiKey_None ) {
			io.AddKeyEvent( imguiKey, bPressed );
		}
		if ( bPressed && nText > 0 ) {
			// The composed bytes are locale-encoded; the basic UI's text
			// entry is ASCII (digits, minus), where byte == codepoint.
			// Full UTF-8/IME rides the TU6.2 XIM wiring.
			for ( int ii = 0; ii < nText; ++ii ) {
				const unsigned char c =
					static_cast<unsigned char>( sText[ ii ] );
				if ( c < 0x80 ) {
					io.AddInputCharacter( c );
				}
			}
		}
	}

	Display* m_display = nullptr;
	Window m_parent = None;
	Window m_window = None;
	Colormap m_colormap = None;
	GLXFBConfig m_fbConfig = nullptr;
	XVisualInfo* m_visualInfo = nullptr;
	GLXContext m_context = nullptr;
	int m_width = 0;
	int m_height = 0;
	unsigned m_inputEvents = 0;
	// The translated mouse state (mouseState()).
	float m_fMouseX = -1.0f;
	float m_fMouseY = -1.0f;
	int m_nMouseButtons = 0;
	mutable double m_scaleFactor = 0.0;
	bool m_valid = false;
};

} // namespace

std::unique_ptr<PluginUiBackend> createPluginUiBackend(
	std::uintptr_t parentNativeHandle, int width, int height ) {
	auto* backend = new ( std::nothrow ) PluginUiBackendX11(
		parentNativeHandle, width, height );
	if ( backend == nullptr ) {
		return nullptr;
	}
	return std::unique_ptr<PluginUiBackend>( backend );
}

} // namespace H2Core
