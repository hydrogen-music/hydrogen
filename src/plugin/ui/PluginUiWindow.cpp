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

// PluginUiWindow's platform-independent half (proposal 0006 TU2.3): the
// per-instance ImGui context lifecycle and frame-governed per-idle rendering
// (ADR 0035's amendment). The native window and draw context live behind
// PluginUiBackend.

#include "PluginUiWindow.h"
#include "PluginUiBackend.h"
#include "PluginUiEngineSink.h"
#include "PluginUiFrameGate.h"
#include "PluginUiLandingView.h"
#include "PluginUiMappingView.h"
#include "PluginUiMixerView.h"
#include <core/CommonStrings.h>
#include <plugin/HydrogenPlugin.h>

#include <imgui.h>

#include <chrono>
#include <memory>
#include <new>

namespace H2Core {

class PluginUiWindow::Impl {
public:
	Impl( std::uintptr_t parentNativeHandle, int width, int height,
		  HydrogenPlugin* pPlugin )
		: m_pBackend( createPluginUiBackend( parentNativeHandle, width, height ) ) {
		if ( m_pBackend == nullptr || ! m_pBackend->valid() ) {
			m_pBackend.reset();
			return;
		}

		IMGUI_CHECKVERSION();
		m_pContext = ImGui::CreateContext();
		if ( m_pContext == nullptr ) {
			m_pBackend.reset();
			return;
		}
		ImGui::SetCurrentContext( m_pContext );

		ImGuiIO& io = ImGui::GetIO();
		// A plugin must not touch the host process's ini/log files.
		io.IniFilename = nullptr;
		io.LogFilename = nullptr;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes( m_pBackend->scaleFactor() );

		// The built-in bitmap font until the embedded TTF lands with the
		// real views (UI-4); it does not scale, so DPI is style-only for
		// now and the spike panel reports the detected factor.
		io.Fonts->AddFontDefault();

		if ( ! m_pBackend->initRenderBackend() ) {
			ImGui::DestroyContext( m_pContext );
			m_pContext = nullptr;
			m_pBackend.reset();
			return;
		}

		// The real views (UI-4) ride on a plugin engine; without one the
		// spike panel stays (the smoke's plugin-less path).
		if ( pPlugin != nullptr ) {
			m_pCommonStrings = pPlugin->commonStrings();
			m_pLandingView = std::make_unique<PluginUiLandingView>( pPlugin );
			m_pMappingView = std::make_unique<PluginUiMappingView>( pPlugin );
			m_pMixerView = std::make_unique<PluginUiMixerView>( pPlugin );
			// The engine-event-sink adapter (ADR 0035 UI-3): the
			// session's publish cycle feeds the models live data on the
			// bridge thread; the idle poll below diffs it into frame
			// requests. The host contract has the plugin (and its
			// session) outlive the window, so registering here is safe
			// for as long as ~Impl unregisters first.
			m_pPlugin = pPlugin;
			m_pEngineSink = std::make_unique<PluginUiEngineSink>(
				m_pLandingView->model(), m_pMixerView->model() );
			pPlugin->setEventSink( m_pEngineSink.get() );
		}
	}

	~Impl() {
		// Unregister the sink adapter before anything it forwards into
		// goes away: the conditional clear cannot kill a successor a
		// later window registered, and the session's in-flight drain
		// completes before this returns (EngineEventSink contract).
		if ( m_pPlugin != nullptr && m_pEngineSink != nullptr ) {
			m_pPlugin->clearEventSink( m_pEngineSink.get() );
		}
		if ( m_pContext != nullptr ) {
			ImGui::SetCurrentContext( m_pContext );
			m_pBackend->shutdownRenderBackend();
			ImGui::DestroyContext( m_pContext );
		}
	}

	void idle() {
		if ( m_pBackend == nullptr ) {
			return;
		}

		// ADR 0035: hosts idle multiple UI instances sequentially on one
		// thread and ImGui contexts are thread-local — switch first, always.
		ImGui::SetCurrentContext( m_pContext );

		// Every idle pumps, whatever rendering decides — input
		// translation and resize bookkeeping must not wait for a frame
		// (the smoke's contract).
		m_pBackend->pumpEvents();

		// Frame governance (ADR 0035's amendment): native events that
		// asked for a frame, and a size change since the last rendered
		// frame (macOS has no resize event wired — the view just
		// autoresizes), request one; the gate coalesces both onto the
		// 30 Hz ceiling.
		if ( m_pBackend->takeFrameRequest() ) {
			m_frameGate.request();
		}
		int nWidth = 0;
		int nHeight = 0;
		m_pBackend->getSize( nWidth, nHeight );
		if ( nWidth != m_nRenderedWidth || nHeight != m_nRenderedHeight ) {
			m_frameGate.request();
		}

		// Input liveness vs. the trickle queue: imgui spreads a quick
		// tap's press+release over two frames
		// (io.ConfigInputTrickleEventQueue, default on), and the
		// deferred release only processes in a LATER frame. This gate's
		// frames are request-driven and the release's arrival already
		// spent its request — without this, the release strands and the
		// key sticks down, its auto-repeat (io.KeyRepeatDelay) running
		// away inside an active InputText. So: while imgui still holds a
		// key or button down, or a text field wants input (which also
		// drains trickle-deferred text events), keep frames coming at
		// the gate's ceiling until the state settles.
		{
			ImGuiIO& io = ImGui::GetIO();
			if ( io.WantTextInput ) {
				m_frameGate.request();
			}
			else {
				for ( ImGuiKey key = ImGuiKey_NamedKey_BEGIN;
					  key < ImGuiKey_NamedKey_END;
					  key = static_cast<ImGuiKey>( key + 1 ) ) {
					if ( io.KeysData[ key - ImGuiKey_NamedKey_BEGIN ]
							 .Down ) {
						m_frameGate.request();
						break;
					}
				}
			}
			for ( int button = 0; button < ImGuiMouseButton_COUNT;
				  ++button ) {
				if ( io.MouseDown[ button ] ) {
					m_frameGate.request();
					break;
				}
			}
		}

		// A frame is also owed when the active tab's displayed data went
		// stale: the engine keeps moving underneath (playback, editor
		// edits) with no input event to latch a request. Only the active
		// tab's model is polled — the frame it would request is the
		// frame that shows it. Polled before the presentability check:
		// while obscured the diff is consumed cheaply and the reveal
		// transition re-requests anyway, so a change that happened
		// hidden still shows with exactly one render once revealed.
		if ( m_pLandingView != nullptr ) {
			switch ( m_activeView ) {
			case ActiveView::Landing:
				if ( m_pLandingView->model().pollInfoChanged() ) {
					m_frameGate.request();
				}
				break;
			case ActiveView::Mixer:
				if ( m_pMixerView->model().pollMetersChanged() ) {
					m_frameGate.request();
				}
				break;
			case ActiveView::None:
			case ActiveView::Mapping:
				break;
			}
		}

		// A window that cannot present (unmapped, fully obscured, hidden
		// host tab) skips rendering — the pending request survives for
		// when it can present again.
		if ( ! m_pBackend->isPresentable() ) {
			m_bWasPresentable = false;
			return;
		}
		if ( ! m_bWasPresentable ) {
			// A reveal owes a frame: however long the window was hidden,
			// the last rendered frame may be stale — and macOS has no
			// paint/visibility event to latch a request with (the view
			// just un-hides), so the transition itself must.
			m_frameGate.request();
			m_bWasPresentable = true;
		}

		const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch() );
		if ( ! m_frameGate.shouldRender( now ) ) {
			return;
		}
		m_frameGate.markRendered( now );
		m_nRenderedWidth = nWidth;
		m_nRenderedHeight = nHeight;

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2( static_cast<float>( nWidth ),
								 static_cast<float>( nHeight ) );

		m_pBackend->beginFrame();
		ImGui::NewFrame();

		++m_nFrames;
		ImGui::SetNextWindowPos( ImVec2( 0.0f, 0.0f ) );
		ImGui::SetNextWindowSize( io.DisplaySize );
		ImGui::Begin( "##pluginUi", nullptr,
					  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
					  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
					  ImGuiWindowFlags_NoSavedSettings );
		if ( m_pLandingView != nullptr ) {
			// The real views (UI-4): the three tabs over their models.
			// The active tab is tracked for the idle poll above — reset
			// each frame, set by whichever tab item actually renders.
			m_activeView = ActiveView::None;
			if ( ImGui::BeginTabBar( "##tabs" ) ) {
			if ( ImGui::BeginTabItem( m_pCommonStrings->getTabGeneralDialog()
							  .toUtf8().constData() ) ) {
					m_activeView = ActiveView::Landing;
					m_pLandingView->draw();
					ImGui::EndTabItem();
				}
				if ( ImGui::BeginTabItem( m_pCommonStrings->getMappingTab()
											  .toUtf8().constData() ) ) {
					m_activeView = ActiveView::Mapping;
					m_pMappingView->draw();
					ImGui::EndTabItem();
				}
				if ( ImGui::BeginTabItem( m_pCommonStrings->getMixerTab()
											  .toUtf8().constData() ) ) {
					m_activeView = ActiveView::Mixer;
					m_pMixerView->draw();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		else {
			// The trivial spike panel (TU2.4): live evidence that the
			// embedded window renders and receives input in a real host.
			// Only the plugin-less path (the smoke) still draws it. The
			// strings are spike placeholders, not shipped UI — they never
			// pass through the translation stack.
			ImGui::Text( "Hydrogen plugin UI (spike)" );
			ImGui::Text( "frames: %u", m_nFrames );
			ImGui::Text( "input events: %u", m_pBackend->inputEventCount() );
			float fMouseX = -1.0f;
			float fMouseY = -1.0f;
			int nMouseButtons = 0;
			m_pBackend->mouseState( fMouseX, fMouseY, nMouseButtons );
			ImGui::Text( "mouse: %.0f, %.0f (%d)", fMouseX, fMouseY,
						  nMouseButtons );
			ImGui::Text( "scale: %.2f", m_pBackend->scaleFactor() );
		}
		ImGui::End();

		ImGui::Render();
		m_pBackend->renderFrame( ImGui::GetDrawData() );
		m_pBackend->endFrame();
	}

	void requestFrame() {
		if ( m_pBackend == nullptr ) {
			return;
		}
		m_frameGate.request();
	}

	unsigned renderedFrameCount() const {
		return m_nFrames;
	}

	std::unique_ptr<PluginUiBackend> m_pBackend = nullptr;
	ImGuiContext* m_pContext = nullptr;
	// The three views (UI-4); null without a plugin engine (spike path).
	std::unique_ptr<PluginUiLandingView> m_pLandingView;
	std::unique_ptr<PluginUiMappingView> m_pMappingView;
	std::unique_ptr<PluginUiMixerView> m_pMixerView;
	// The engine-event-sink adapter (ADR 0035 UI-3), declared AFTER the
	// views: reverse destruction order destroys it before the models it
	// forwards into — and ~Impl unregisters it before any of that.
	std::unique_ptr<PluginUiEngineSink> m_pEngineSink;
	// The plugin the views and sink adapter attach to; null on the spike
	// path. The host contract: the plugin (and its session) outlives
	// the window.
	HydrogenPlugin* m_pPlugin = nullptr;
	// The views' shared string source; null on the spike path, and the
	// tab labels below only draw under the landing-view guard.
	std::shared_ptr<CommonStrings> m_pCommonStrings;
	unsigned m_nFrames = 0;
	// The frame gate (ADR 0035's amendment): 27 ms is the 30 Hz target
	// (33 ms) minus slack — a 33 ms gate would beat against the hosts'
	// 30 ms idle timers down to ~17 Hz.
	PluginUiFrameGate m_frameGate{ std::chrono::milliseconds( 27 ) };
	// The size the last rendered frame was drawn at — a change requests
	// the next frame. -1: nothing rendered yet.
	int m_nRenderedWidth = -1;
	int m_nRenderedHeight = -1;
	// Presentability at the last idle — a false→true transition is a
	// reveal and owes a frame. Starts false so the first presentable
	// idle counts as a reveal.
	bool m_bWasPresentable = false;
	// Which tab the last rendered frame showed — only the active tab's
	// model is polled for staleness (None until the first render, and
	// whenever the tab bar itself did not render).
	enum class ActiveView { None, Landing, Mapping, Mixer };
	ActiveView m_activeView = ActiveView::None;
};

PluginUiWindow::PluginUiWindow( std::uintptr_t parentNativeHandle, int width,
								int height, HydrogenPlugin* pPlugin )
	: m_pImpl( new ( std::nothrow ) Impl( parentNativeHandle, width, height,
										  pPlugin ) ) {
}

PluginUiWindow::~PluginUiWindow() {
	delete m_pImpl;
}

bool PluginUiWindow::isValid() const {
	return m_pImpl != nullptr && m_pImpl->m_pBackend != nullptr;
}

std::uintptr_t PluginUiWindow::nativeHandle() const {
	return m_pImpl != nullptr && m_pImpl->m_pBackend != nullptr
		? m_pImpl->m_pBackend->nativeHandle() : 0;
}

bool PluginUiWindow::shouldClose() const {
	return m_pImpl != nullptr && m_pImpl->m_pBackend != nullptr
		&& m_pImpl->m_pBackend->shouldClose();
}

double PluginUiWindow::scaleFactor() const {
	return m_pImpl != nullptr && m_pImpl->m_pBackend != nullptr
		? m_pImpl->m_pBackend->scaleFactor() : 1.0;
}

unsigned PluginUiWindow::inputEventCount() const {
	return m_pImpl != nullptr && m_pImpl->m_pBackend != nullptr
		? m_pImpl->m_pBackend->inputEventCount() : 0;
}

void PluginUiWindow::mouseState( float& x, float& y, int& nButtons ) const {
	if ( m_pImpl != nullptr && m_pImpl->m_pBackend != nullptr ) {
		m_pImpl->m_pBackend->mouseState( x, y, nButtons );
	}
	else {
		x = -1.0f;
		y = -1.0f;
		nButtons = 0;
	}
}

void PluginUiWindow::getSize( int& width, int& height ) const {
	if ( m_pImpl != nullptr && m_pImpl->m_pBackend != nullptr ) {
		m_pImpl->m_pBackend->getSize( width, height );
	}
	else {
		width = 0;
		height = 0;
	}
}

void PluginUiWindow::setSize( int width, int height ) {
	if ( m_pImpl != nullptr && m_pImpl->m_pBackend != nullptr ) {
		m_pImpl->m_pBackend->setSize( width, height );
	}
}

unsigned PluginUiWindow::renderedFrameCount() const {
	return m_pImpl != nullptr ? m_pImpl->renderedFrameCount() : 0;
}

void PluginUiWindow::requestFrame() {
	if ( m_pImpl != nullptr ) {
		m_pImpl->requestFrame();
	}
}

void PluginUiWindow::idle() {
	if ( m_pImpl != nullptr ) {
		m_pImpl->idle();
	}
}

} // namespace H2Core
