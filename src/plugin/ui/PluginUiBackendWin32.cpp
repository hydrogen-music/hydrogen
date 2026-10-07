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

// Win32 backend of PluginUiWindow (proposal 0006 TU2.2/TU2.3): a child HWND
// on the host-provided parent plus a WGL context, driven frame-by-frame
// from idle(). CI-gated: no Windows build host locally — the AppVeyor
// Windows job compiles and runs the smoke against this.

#include "PluginUiBackend.h"

#include <imgui_impl_opengl3.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <new>
#include <memory>

namespace H2Core {

namespace {

class PluginUiBackendWin32 : public PluginUiBackend {
public:
	PluginUiBackendWin32( std::uintptr_t parentNativeHandle, int width,
						  int height )
		: m_parent( reinterpret_cast<HWND>( parentNativeHandle ) ) {
		if ( m_parent == nullptr || ! IsWindow( m_parent ) ) {
			return;
		}

		// One window class per process, never unregistered: unregistering
		// at DLL unload would break other instances still alive in the
		// host; the OS reclaims the class at process exit anyway.
		static const wchar_t* kClassName = L"HydrogenPluginUiWindow";
		static const bool classRegistered = [] {
			WNDCLASSW wc = {};
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = DefWindowProcW;
			wc.hInstance = GetModuleHandleW( nullptr );
			wc.lpszClassName = kClassName;
			return RegisterClassW( &wc ) != 0;
		}();
		if ( ! classRegistered ) {
			return;
		}

		// WS_CHILD | WS_VISIBLE: embedded means mapped on the host parent
		// right away (the X11 backend's XMapWindow equivalent).
		m_window = CreateWindowExW( 0, kClassName, L"",
			WS_CHILD | WS_VISIBLE, 0, 0, width, height, m_parent, nullptr,
			GetModuleHandleW( nullptr ), nullptr );
		if ( m_window == nullptr ) {
			return;
		}

		m_dc = GetDC( m_window );
		if ( m_dc == nullptr ) {
			return;
		}
		PIXELFORMATDESCRIPTOR pfd = {};
		pfd.nSize = sizeof( pfd );
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL
			| PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 16;
		const int pixelFormat = ChoosePixelFormat( m_dc, &pfd );
		if ( pixelFormat == 0 || ! SetPixelFormat( m_dc, pixelFormat, &pfd ) ) {
			return;
		}

		m_context = wglCreateContext( m_dc );
		if ( m_context == nullptr ) {
			return;
		}

		m_valid = true;
	}

	~PluginUiBackendWin32() override {
		if ( m_context != nullptr ) {
			wglDeleteContext( m_context );
		}
		if ( m_dc != nullptr && m_window != nullptr ) {
			ReleaseDC( m_window, m_dc );
		}
		if ( m_window != nullptr ) {
			DestroyWindow( m_window );
		}
	}

	bool valid() const override {
		return m_valid;
	}

	std::uintptr_t nativeHandle() const override {
		return reinterpret_cast<std::uintptr_t>( m_window );
	}

	double scaleFactor() const override {
		if ( m_scaleFactor <= 0.0 && m_dc != nullptr ) {
			const int dpi = GetDeviceCaps( m_dc, LOGPIXELSX );
			m_scaleFactor = dpi > 0 ? dpi / 96.0 : 1.0;
		}
		return m_scaleFactor > 0.0 ? m_scaleFactor : 1.0;
	}

	void getSize( int& width, int& height ) const override {
		RECT rect = {};
		if ( m_window != nullptr && GetClientRect( m_window, &rect ) ) {
			m_width = rect.right - rect.left;
			m_height = rect.bottom - rect.top;
		}
		width = m_width;
		height = m_height;
	}

	unsigned inputEventCount() const override {
		// Input arrives through the host's message pump and our WndProc;
		// wiring the observation is UI-4 scope.
		return 0;
	}

	void pumpEvents() override {
		// The host's message pump delivers events; nothing to pump here.
	}

	bool initRenderBackend() override {
		if ( ! wglMakeCurrent( m_dc, m_context ) ) {
			return false;
		}
		const bool ok = ImGui_ImplOpenGL3_Init();
		wglMakeCurrent( nullptr, nullptr );
		return ok;
	}

	void shutdownRenderBackend() override {
		if ( wglMakeCurrent( m_dc, m_context ) ) {
			ImGui_ImplOpenGL3_Shutdown();
			wglMakeCurrent( nullptr, nullptr );
		}
	}

	void beginFrame() override {
		wglMakeCurrent( m_dc, m_context );
		ImGui_ImplOpenGL3_NewFrame();
	}

	void renderFrame( ImDrawData* data ) override {
		ImGui_ImplOpenGL3_RenderDrawData( data );
	}

	void endFrame() override {
		SwapBuffers( m_dc );
		wglMakeCurrent( nullptr, nullptr );
	}

private:
	HWND m_parent = nullptr;
	HWND m_window = nullptr;
	HDC m_dc = nullptr;
	HGLRC m_context = nullptr;
	mutable int m_width = 0;
	mutable int m_height = 0;
	mutable double m_scaleFactor = 0.0;
	bool m_valid = false;
};

} // namespace

std::unique_ptr<PluginUiBackend> createPluginUiBackend(
	std::uintptr_t parentNativeHandle, int width, int height ) {
	auto* backend = new ( std::nothrow ) PluginUiBackendWin32(
		parentNativeHandle, width, height );
	if ( backend == nullptr ) {
		return nullptr;
	}
	return std::unique_ptr<PluginUiBackend>( backend );
}

} // namespace H2Core
