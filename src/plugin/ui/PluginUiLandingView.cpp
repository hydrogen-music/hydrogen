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
// with their keyed IGFD browsers, the pattern-load target row, and the
// exposed MIDI configuration (feedback and transport stay engine-side —
// settable via the editor). Thin draw calls only (TU1.3): every decision
// is the model's.

#include "PluginUiLandingView.h"

#include <imgui.h>

#include <algorithm>

#include <core/CommonStrings.h>
#include <plugin/HydrogenPlugin.h>

namespace H2Core {

PluginUiLandingView::PluginUiLandingView( HydrogenPlugin* pPlugin )
	: m_model( pPlugin ), m_pCommonStrings( pPlugin->commonStrings() )
{
	refreshMidiConfigWidgets();
}

void PluginUiLandingView::refreshMidiConfigWidgets()
{
	m_bNoteOffIgnore = m_model.midiNoteOffIgnore();
	m_nActionChannel = static_cast<int>( m_model.midiActionChannel() );
	m_nSendNoteOff = static_cast<int>( m_model.midiSendNoteOff() );
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
			Filesystem::sSongSuffix.toUtf8(), config );
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

	// The exposed MIDI configuration.
	if ( ImGui::CollapsingHeader(
			 m_pCommonStrings->getMidiConfigurationHeader().toUtf8()
					.constData(),
			 ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::Checkbox(
			m_pCommonStrings->getIgnoreNoteOffLabel().toUtf8().constData(),
			&m_bNoteOffIgnore );

		// The action channel dropdown — the GUI spinbox's range and
		// texts (all channels, Off, 1..16), the same +1 index shift as
		// the mapping tab's channel dropdowns. The QByteArrays must
		// outlive the Combo call — held locals, not chained
		// temporaries.
		QByteArray sChannelEntries[ 18 ];
		const char* sChannelEntryPtrs[ 18 ];
		for ( int n = 0; n < 18; ++n ) {
			sChannelEntries[ n ] = m_pCommonStrings->getChannelDisplayName(
				Midi::channelFromInt( n - 1 ) ).toUtf8();
			sChannelEntryPtrs[ n ] = sChannelEntries[ n ].constData();
		}
		int nActionChannel = m_nActionChannel + 1;
		ImGui::SetNextItemWidth( 72.0f );
		if ( ImGui::Combo(
				 m_pCommonStrings->getActionChannelLabel().toUtf8()
					 .constData(),
				 &nActionChannel, sChannelEntryPtrs, 18 ) ) {
			m_nActionChannel = nActionChannel - 1;
		}

		// The combo entries must outlive the Combo call — held locals,
		// not chained temporaries.
		const auto sAlways = m_pCommonStrings->getOptionAlways().toUtf8();
		const auto sOnCustomNoteLengths =
			m_pCommonStrings->getOptionOnCustomNoteLengths().toUtf8();
		const auto sNever = m_pCommonStrings->getOptionNever().toUtf8();
		const char* sSendNoteOffEntries[ 3 ] = {
			sAlways.constData(), sOnCustomNoteLengths.constData(),
			sNever.constData()
		};
		ImGui::Combo( m_pCommonStrings->getSendNoteOffLabel().toUtf8()
							.constData(),
					 &m_nSendNoteOff, sSendNoteOffEntries, 3 );

		if ( ImGui::Button( m_pCommonStrings->getApplyButton().toUtf8()
								.constData() ) ) {
			if ( m_model.applyExposedMidiControlSettings(
					 m_bNoteOffIgnore,
					 Midi::channelFromInt( m_nActionChannel ),
					 static_cast<Preferences::MidiSendNoteOff>(
						 m_nSendNoteOff ) ) ) {
				// The engine may have normalized a value — resync the
				// widgets, but never per frame (snap-back).
				refreshMidiConfigWidgets();
			}
		}
	}

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
