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
// per-instance ImGui context lifecycle and the per-idle frame. The native
// window and draw context live behind PluginUiBackend.

#include "PluginUiWindow.h"
#include "PluginUiBackend.h"
#include "PluginUiLandingView.h"
#include "PluginUiMappingView.h"
#include "PluginUiMixerView.h"
#include "PluginUiStrings.h"

#include <imgui.h>

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
			m_pLandingView = std::make_unique<PluginUiLandingView>( pPlugin );
			m_pMappingView = std::make_unique<PluginUiMappingView>( pPlugin );
			m_pMixerView = std::make_unique<PluginUiMixerView>( pPlugin );
		}
	}

	~Impl() {
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

		m_pBackend->pumpEvents();

		ImGuiIO& io = ImGui::GetIO();
		int nWidth = 0;
		int nHeight = 0;
		m_pBackend->getSize( nWidth, nHeight );
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
			if ( ImGui::BeginTabBar( "##tabs" ) ) {
			if ( ImGui::BeginTabItem( PluginUiStrings::generalTab()
							  .toUtf8().constData() ) ) {
					m_pLandingView->draw();
					ImGui::EndTabItem();
				}
				if ( ImGui::BeginTabItem( PluginUiStrings::mappingTab()
											  .toUtf8().constData() ) ) {
					m_pMappingView->draw();
					ImGui::EndTabItem();
				}
				if ( ImGui::BeginTabItem( PluginUiStrings::mixerTab()
											  .toUtf8().constData() ) ) {
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

	std::unique_ptr<PluginUiBackend> m_pBackend = nullptr;
	ImGuiContext* m_pContext = nullptr;
	// The three views (UI-4); null without a plugin engine (spike path).
	std::unique_ptr<PluginUiLandingView> m_pLandingView;
	std::unique_ptr<PluginUiMappingView> m_pMappingView;
	std::unique_ptr<PluginUiMixerView> m_pMixerView;
	unsigned m_nFrames = 0;
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

void PluginUiWindow::idle() {
	if ( m_pImpl != nullptr ) {
		m_pImpl->idle();
	}
}

} // namespace H2Core
