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
// on the host-provided parent — or, with a zero parent, the self-managed
// top-level fallback window (UI-5, TU5.2) — plus a WGL context, driven
// frame-by-frame from idle(). Input translation (TU6.2): the WndProc only
// records native messages into a queue — it runs on the host's message
// pump, outside the per-instance ImGui context discipline — and
// pumpEvents() drains and translates them under the caller-guaranteed
// current context (the X11 backend's contract). CI-gated: no Windows build
// host locally — the AppVeyor Windows job compiles and runs the smoke
// against this.

#include "PluginUiBackend.h"

#include <imgui_impl_opengl3.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

#include <cstdint>
#include <new>
#include <memory>
#include <vector>

namespace H2Core {

namespace {

// Virtual key → ImGuiKey for the subset the basic UI needs (the X11
// backend's KeySym subset): navigation, digits, letters, minus, keypad
// minus, modifiers. The generic modifier VKs resolve to their L/R key
// through GetKeyState at message time — the message itself does not
// carry the side (queue-synchronized state, unlike GetAsyncKeyState's
// physical snapshot).
ImGuiKey imguiKeyFromVirtualKey( WPARAM wParam ) {
	const int vk = static_cast<int>( wParam );
	switch ( vk ) {
	case VK_BACK: return ImGuiKey_Backspace;
	case VK_TAB: return ImGuiKey_Tab;
	case VK_RETURN: return ImGuiKey_Enter;
	case VK_ESCAPE: return ImGuiKey_Escape;
	case VK_DELETE: return ImGuiKey_Delete;
	case VK_HOME: return ImGuiKey_Home;
	case VK_END: return ImGuiKey_End;
	case VK_PRIOR: return ImGuiKey_PageUp;
	case VK_NEXT: return ImGuiKey_PageDown;
	case VK_LEFT: return ImGuiKey_LeftArrow;
	case VK_RIGHT: return ImGuiKey_RightArrow;
	case VK_UP: return ImGuiKey_UpArrow;
	case VK_DOWN: return ImGuiKey_DownArrow;
	case VK_INSERT: return ImGuiKey_Insert;
	case VK_SPACE: return ImGuiKey_Space;
	case VK_OEM_MINUS: return ImGuiKey_Minus;
	case VK_OEM_PLUS: return ImGuiKey_Equal;
	case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
	case VK_SHIFT:
		return ( GetKeyState( VK_LSHIFT ) & 0x8000 ) != 0
			? ImGuiKey_LeftShift : ImGuiKey_RightShift;
	case VK_CONTROL:
		return ( GetKeyState( VK_LCONTROL ) & 0x8000 ) != 0
			? ImGuiKey_LeftCtrl : ImGuiKey_RightCtrl;
	case VK_MENU:
		return ( GetKeyState( VK_LMENU ) & 0x8000 ) != 0
			? ImGuiKey_LeftAlt : ImGuiKey_RightAlt;
	default: break;
	}
	if ( vk >= '0' && vk <= '9' ) {
		return static_cast<ImGuiKey>( ImGuiKey_0 + ( vk - '0' ) );
	}
	if ( vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9 ) {
		return static_cast<ImGuiKey>(
			ImGuiKey_Keypad0 + ( vk - VK_NUMPAD0 ) );
	}
	if ( vk >= 'A' && vk <= 'Z' ) {
		return static_cast<ImGuiKey>( ImGuiKey_A + ( vk - 'A' ) );
	}
	return ImGuiKey_None;
}

// Native input recorded by the WndProc (TU6.2): Windows delivers
// messages through the host's pump on the window's thread — the same
// thread that runs idle(), by the thread-affine message queue and the
// hosts' main-thread gui contracts — but not under our per-instance
// ImGui context discipline. The WndProc only records; pumpEvents()
// drains and translates with the caller-guaranteed current context.
struct InputEvent {
	enum class Kind {
		None, MouseMove, MouseLeave, MouseButton, Wheel, Key, Char, Focus
	};
	Kind kind = Kind::None;
	float x = 0.0f;               // MouseMove/MouseButton: client coords
	float y = 0.0f;
	int button = 0;               // MouseButton: 0 left, 1 right,
	                               // 2 middle (imgui order)
	bool pressed = false;         // MouseButton/Key
	float wheelX = 0.0f;          // Wheel: notches
	float wheelY = 0.0f;
	unsigned short char16 = 0;    // Char: one UTF-16 unit
	ImGuiKey key = ImGuiKey_None;
	bool focused = false;         // Focus
};

class PluginUiBackendWin32 : public PluginUiBackend {
public:
	PluginUiBackendWin32( std::uintptr_t parentNativeHandle, int width,
						  int height )
		: m_parent( reinterpret_cast<HWND>( parentNativeHandle ) ) {
		if ( m_parent != nullptr && ! IsWindow( m_parent ) ) {
			return;
		}

		// One window class per process, never unregistered: unregistering
		// at DLL unload would break other instances still alive in the
		// host; the OS reclaims the class at process exit anyway.
		static const wchar_t* kClassName = L"HydrogenPluginUiWindow";
		static const bool classRegistered = [] {
			WNDCLASSW wc = {};
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = wndProc;
			wc.hInstance = GetModuleHandleW( nullptr );
			wc.lpszClassName = kClassName;
			return RegisterClassW( &wc ) != 0;
		}();
		if ( ! classRegistered ) {
			return;
		}

		// WS_CHILD | WS_VISIBLE: embedded means mapped on the host parent
		// right away (the X11 backend's XMapWindow equivalent). A null
		// parent means the self-managed top-level fallback window (UI-5,
		// TU5.2): decorated, with the close button the WndProc surfaces
		// as shouldClose() ("Hydrogen" is the brand name, not a
		// translatable string).
		if ( m_parent != nullptr ) {
			m_window = CreateWindowExW( 0, kClassName, L"Hydrogen",
				WS_CHILD | WS_VISIBLE, 0, 0, width, height, m_parent,
				nullptr, GetModuleHandleW( nullptr ), nullptr );
		} else {
			m_window = CreateWindowExW( 0, kClassName, L"Hydrogen",
				WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
				CW_USEDEFAULT, width, height, nullptr, nullptr,
				GetModuleHandleW( nullptr ), nullptr );
		}
		if ( m_window == nullptr ) {
			return;
		}
		// The WndProc reaches this instance through the window's user
		// data — set after creation (no message can need it during
		// CreateWindowExW itself).
		SetWindowLongPtrW( m_window, GWLP_USERDATA,
						   reinterpret_cast<LONG_PTR>( this ) );

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
		if ( m_scaleFactor <= 0.0 && m_window != nullptr ) {
			// Per-window DPI (TU6.2): the host's process-wide DPI
			// awareness mode decides the window's DPI — the plugin must
			// not change that, it is the host's process. GetDpiForWindow
			// needs Win10 1607 and is resolved dynamically; the DC's
			// LOGPIXELSX is the pre-1607 fallback.
			using GetDpiForWindow_t = UINT ( WINAPI* )( HWND );
			static const GetDpiForWindow_t pGetDpiForWindow =
				reinterpret_cast<GetDpiForWindow_t>(
					reinterpret_cast<void*>( GetProcAddress(
						GetModuleHandleW( L"user32" ),
						"GetDpiForWindow" ) ) );
			UINT dpi = 0;
			if ( pGetDpiForWindow != nullptr ) {
				dpi = pGetDpiForWindow( m_window );
			}
			if ( dpi == 0 && m_dc != nullptr ) {
				dpi = static_cast<UINT>(
					GetDeviceCaps( m_dc, LOGPIXELSX ) );
			}
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

	void setSize( int width, int height ) override {
		if ( ! m_valid ) {
			return;
		}
		// The tracked size follows immediately; the next getSize() reads
		// the live client rect anyway.
		m_width = width;
		m_height = height;
		SetWindowPos( m_window, nullptr, 0, 0, width, height,
					  SWP_NOMOVE | SWP_NOZORDER );
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
		// Drain what the WndProc recorded on the host's pump (TU6.2).
		// The caller guarantees the per-instance ImGui context is
		// current, so events translate straight into its IO.
		for ( const InputEvent& ev : m_inputQueue ) {
			ImGuiIO& io = ImGui::GetIO();
			switch ( ev.kind ) {
			case InputEvent::Kind::MouseMove:
				io.AddMousePosEvent( ev.x, ev.y );
				m_fMouseX = ev.x;
				m_fMouseY = ev.y;
				break;
			case InputEvent::Kind::MouseLeave:
				// imgui's off-window sentinel: -FLT_MAX drops the hover
				// from hit-testing (its own backends do the same).
				io.AddMousePosEvent( -FLT_MAX, -FLT_MAX );
				m_fMouseX = -1.0f;
				m_fMouseY = -1.0f;
				break;
			case InputEvent::Kind::MouseButton:
				io.AddMouseButtonEvent( ev.button, ev.pressed );
				m_nMouseButtons = ev.pressed
					? ( m_nMouseButtons | ( 1 << ev.button ) )
					: ( m_nMouseButtons & ~( 1 << ev.button ) );
				break;
			case InputEvent::Kind::Wheel:
				io.AddMouseWheelEvent( ev.wheelX, ev.wheelY );
				break;
			case InputEvent::Kind::Key:
				if ( ev.key != ImGuiKey_None ) {
					io.AddKeyEvent( ev.key, ev.pressed );
				}
				break;
			case InputEvent::Kind::Char:
				io.AddInputCharacterUTF16( ev.char16 );
				break;
			case InputEvent::Kind::Focus:
				io.AddFocusEvent( ev.focused );
				break;
			}
			++m_inputEvents;
		}
		m_inputQueue.clear();
	}

	bool shouldClose() const override {
		return m_bShouldClose;
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
	// The fallback window's close button (top-level only): surfaced as
	// shouldClose() — the owner decides the teardown, so WM_CLOSE never
	// reaches DefWindowProcW (it would destroy the window under a live
	// WGL context). Input messages (TU6.2) only record into the queue;
	// the instance pointer is null during CreateWindowExW itself, where
	// no input can arrive — the guard keeps those safe anyway.
	static LRESULT CALLBACK wndProc( HWND hwnd, UINT msg, WPARAM wParam,
									  LPARAM lParam ) {
		auto* self = reinterpret_cast<PluginUiBackendWin32*>(
			GetWindowLongPtrW( hwnd, GWLP_USERDATA ) );
		if ( self != nullptr ) {
			InputEvent ev = {};
			switch ( msg ) {
			case WM_CLOSE:
				self->m_bShouldClose = true;
				return 0;
			case WM_MOUSEMOVE: {
				// TrackMouseEvent buys the WM_MOUSELEAVE that clears
				// imgui's hover when the pointer leaves the child.
				if ( ! self->m_bMouseTracked ) {
					TRACKMOUSEEVENT track = {};
					track.cbSize = sizeof( track );
					track.dwFlags = TME_LEAVE;
					track.hwndTrack = hwnd;
					TrackMouseEvent( &track );
					self->m_bMouseTracked = true;
				}
				ev.kind = InputEvent::Kind::MouseMove;
				ev.x = static_cast<float>( GET_X_LPARAM( lParam ) );
				ev.y = static_cast<float>( GET_Y_LPARAM( lParam ) );
				break;
			}
			case WM_MOUSELEAVE:
				self->m_bMouseTracked = false;
				ev.kind = InputEvent::Kind::MouseLeave;
				break;
			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONDBLCLK:
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP: {
				// Capture keeps WM_MOUSEMOVE arriving while a drag runs
				// outside the child (imgui's own backend does the same);
				// released with the last button. The held-mask lives
				// WndProc-side: the translated m_nMouseButtons only
				// updates when pumpEvents() drains, which is too late
				// for the capture decision.
				const bool bPressed = msg == WM_LBUTTONDOWN
					|| msg == WM_LBUTTONDBLCLK || msg == WM_MBUTTONDOWN
					|| msg == WM_MBUTTONDBLCLK || msg == WM_RBUTTONDOWN
					|| msg == WM_RBUTTONDBLCLK;
				HWND hwndCaptured = GetCapture();
				if ( self->m_nMouseButtonsDown != 0
					 && hwndCaptured != hwnd ) {
					// The host stole the capture mid-drag.
					self->m_nMouseButtonsDown = 0;
				}
				// imgui orders buttons 0 left, 1 right, 2 middle
				// (imgui.h: ImGuiMouseButton_*); the held-mask bits
				// follow the same order.
				int button = 0;
				if ( msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK
					 || msg == WM_LBUTTONUP ) {
					button = 0;
				}
				else if ( msg == WM_RBUTTONDOWN
						  || msg == WM_RBUTTONDBLCLK
						  || msg == WM_RBUTTONUP ) {
					button = 1;
				}
				else {
					button = 2;
				}
				if ( bPressed ) {
					if ( self->m_nMouseButtonsDown == 0
						 && hwndCaptured == nullptr ) {
						SetCapture( hwnd );
					}
					self->m_nMouseButtonsDown |= ( 1 << button );
				}
				else {
					self->m_nMouseButtonsDown &= ~( 1 << button );
					if ( self->m_nMouseButtonsDown == 0
						 && GetCapture() == hwnd ) {
						ReleaseCapture();
					}
				}
				ev.kind = InputEvent::Kind::MouseButton;
				ev.pressed = bPressed;
				ev.button = button;
				// Position rides along: hosts may deliver a click
				// without a preceding move.
				ev.x = static_cast<float>( GET_X_LPARAM( lParam ) );
				ev.y = static_cast<float>( GET_Y_LPARAM( lParam ) );
				break;
			}
			case WM_MOUSEWHEEL:
				ev.kind = InputEvent::Kind::Wheel;
				ev.wheelY = static_cast<float>(
					GET_WHEEL_DELTA_WPARAM( wParam ) ) / WHEEL_DELTA;
				break;
			case WM_MOUSEHWHEEL:
				ev.kind = InputEvent::Kind::Wheel;
				ev.wheelX = -( static_cast<float>(
					GET_WHEEL_DELTA_WPARAM( wParam ) ) / WHEEL_DELTA );
				break;
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
				ev.kind = InputEvent::Kind::Key;
				ev.pressed = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
				ev.key = imguiKeyFromVirtualKey( wParam );
				if ( msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP ) {
					// System keys reach DefWindowProc too: it turns
					// Alt+F4 into WM_CLOSE for the fallback window —
					// swallowing them would break its close path.
					self->m_inputQueue.push_back( ev );
					return DefWindowProcW( hwnd, msg, wParam, lParam );
				}
				break;
			case WM_CHAR:
				// The window is Unicode (CreateWindowExW), so WM_CHAR
				// carries UTF-16 — including the IME's composed result,
				// which DefWindowProcW below converts from the
				// composition string.
				if ( wParam > 0 && wParam < 0x10000 ) {
					ev.kind = InputEvent::Kind::Char;
					ev.char16 = static_cast<unsigned short>( wParam );
				}
				break;
			case WM_IME_COMPOSITION: {
				// imgui's own Win32 backend: let the default procedure
				// translate the composition result into WM_CHAR, then
				// swallow the message so it is not translated twice.
				const LRESULT result =
					DefWindowProcW( hwnd, msg, wParam, lParam );
				return ( lParam & GCS_RESULTSTR ) ? 1 : result;
			}
			case WM_SETFOCUS:
			case WM_KILLFOCUS:
				ev.kind = InputEvent::Kind::Focus;
				ev.focused = msg == WM_SETFOCUS;
				break;
			case WM_SIZE:
				// Bookkeeping parity with X11's ConfigureNotify;
				// getSize() reads the live client rect anyway.
				self->m_width = LOWORD( lParam );
				self->m_height = HIWORD( lParam );
				break;
			default:
				break;
			}
			if ( ev.kind != InputEvent::Kind::None ) {
				self->m_inputQueue.push_back( ev );
				return 0;
			}
		}
		return DefWindowProcW( hwnd, msg, wParam, lParam );
	}

	HWND m_parent = nullptr;
	HWND m_window = nullptr;
	HDC m_dc = nullptr;
	HGLRC m_context = nullptr;
	std::vector<InputEvent> m_inputQueue;
	unsigned m_inputEvents = 0;
	// The translated mouse state (mouseState()).
	float m_fMouseX = -1.0f;
	float m_fMouseY = -1.0f;
	int m_nMouseButtons = 0;
	// Held-button mask WndProc-side (capture decisions, TU6.2).
	int m_nMouseButtonsDown = 0;
	bool m_bMouseTracked = false;
	mutable int m_width = 0;
	mutable int m_height = 0;
	mutable double m_scaleFactor = 0.0;
	bool m_bShouldClose = false;
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
