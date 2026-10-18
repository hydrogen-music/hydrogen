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

// The landing view's renderer (proposal 0006 TU4.1): the load buttons
// with their keyed IGFD browsers and the pattern-load target row. Thin
// draw calls only (TU1.3): every decision is the model's.

#include "PluginUiLandingView.h"

#include <imgui.h>

#include <algorithm>

#include <core/CommonStrings.h>
#include <plugin/HydrogenPlugin.h>

namespace H2Core {

PluginUiLandingView::PluginUiLandingView( HydrogenPlugin* pPlugin )
	: m_model( pPlugin ), m_pCommonStrings( pPlugin->commonStrings() )
{
}

void PluginUiLandingView::draw()
{
	// Load row: each button opens its keyed browser; the drumkit
	// browser lists directories only (a drumkit is a folder).
	if ( ImGui::Button( m_pCommonStrings->getLoadSongButton().toUtf8()
							.constData() ) ) {
		IGFD::FileDialogConfig config;
		config.path = ".";
		config.countSelectionMax = 1;
		m_fileDialog.OpenDialog(
			"song", m_pCommonStrings->getSongFileDialogTitle().toStdString(),
			Filesystem::sSongSuffix.toUtf8() + ", " +
				Filesystem::sProjectSuffix.toUtf8(),
			config
		);
	}
	ImGui::SameLine();
	if ( ImGui::Button( m_pCommonStrings->getLoadPatternButton().toUtf8()
							.constData() ) ) {
		IGFD::FileDialogConfig config;
		config.path = ".";
		config.countSelectionMax = 1;
		m_fileDialog.OpenDialog(
			"pattern",
			m_pCommonStrings->getPatternFileDialogTitle().toStdString(),
			Filesystem::sPatternSuffix.toUtf8(), config );
	}
	ImGui::SameLine();
	if ( ImGui::Button( m_pCommonStrings->getActionLoadDrumkit().toUtf8()
							.constData() ) ) {
		IGFD::FileDialogConfig config;
		config.path = ".";
		config.countSelectionMax = 1;
		m_fileDialog.OpenDialog(
			"drumkit",
			m_pCommonStrings->getDrumkitFileDialogTitle().toStdString(),
			nullptr, config );
	}
	ImGui::SameLine();
	if ( ImGui::Button( m_pCommonStrings->getOpenEditorButton().toUtf8()
							.constData() ) ) {
		m_model.openEditor();
	}

	// Pattern-load target: the slot the next pattern load installs into
	// (0-based, like the GUI's pattern spinbox) and whether it replaces
	// the slot's current pattern.
	ImGui::SetNextItemWidth( 72.0f );
	ImGui::InputInt( m_pCommonStrings->getInputCapturePattern().toUtf8()
						.constData(),
					 &m_nPatternNumber, 0, 0 );
	m_nPatternNumber = std::max( 0, m_nPatternNumber );
	ImGui::SameLine();
	ImGui::Checkbox( m_pCommonStrings->getReplaceLabel().toUtf8().constData(),
					 &m_bReplacePattern );

	// The keyed browsers — one IGFD instance, at most one dialog open.
	//
	// The default size on Linux+wayland is far too tiny.
	const ImVec2 minSize =
		ImVec2( HydrogenPlugin::nMinWidth, HydrogenPlugin::nMinHeight );
	if ( m_fileDialog.Display(
			 "song", ImGuiWindowFlags_NoCollapse, minSize
		 ) ) {
		if ( m_fileDialog.IsOk() ) {
			m_model.loadSong( QString::fromStdString(
				m_fileDialog.GetFilePathName() ) );
		}
		m_fileDialog.Close();
	}
	if ( m_fileDialog.Display( "pattern", ImGuiWindowFlags_NoCollapse, minSize ) ) {
		if ( m_fileDialog.IsOk() ) {
			m_model.loadPattern(
				QString::fromStdString( m_fileDialog.GetFilePathName() ),
				m_nPatternNumber, m_bReplacePattern );
		}
		m_fileDialog.Close();
	}
	if ( m_fileDialog.Display( "drumkit", ImGuiWindowFlags_NoCollapse, minSize ) ) {
		if ( m_fileDialog.IsOk() ) {
			m_model.loadDrumkit( QString::fromStdString(
				m_fileDialog.GetFilePathName() ) );
		}
		m_fileDialog.Close();
	}
}

} // namespace H2Core
