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
#include <GL/glx.h>

#include <cstdint>
#include <cstdlib>
#include <new>
#include <memory>

namespace H2Core {

namespace {

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
		// surface UI-4 will translate.
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

	void pumpEvents() override {
		// Pugl dispatchX11Events: flush, then drain the queue. Only resize
		// bookkeeping and input observation live here; full input
		// translation is UI-4 scope.
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
			case ButtonPress:
			case ButtonRelease:
			case KeyPress:
			case KeyRelease:
			case MotionNotify:
			case EnterNotify:
			case LeaveNotify:
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
