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

// The landing view's renderer (proposal 0006 TU4.1): the song/pattern load
// tabs with their keyed IGFD browsers, the sound-library drumkit picker,
// and the editor line. Thin draw calls only (TU1.3): every decision is the
// model's.

#include "PluginUiLandingView.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>

#include <core/CommonStrings.h>
#include <plugin/HydrogenPlugin.h>

namespace H2Core {

PluginUiLandingView::PluginUiLandingView( HydrogenPlugin* pPlugin )
	: m_model( pPlugin ), m_pCommonStrings( pPlugin->commonStrings() )
{
	// Prefill the editor line with the resolved default executable — the
	// line edit's content is what the button launches.
	const QByteArray sDefault = m_model.editorExecutable().toUtf8();
	const int nCopy = std::min( static_cast<int>( sDefault.size() ),
								static_cast<int>(
									m_sEditorExecutableBuffer.size() ) - 1 );
	std::memcpy( m_sEditorExecutableBuffer.data(), sDefault.constData(),
				 nCopy );
	m_sEditorExecutableBuffer[ nCopy ] = '\0';
}

void PluginUiLandingView::draw()
{
	// Song and pattern loading are mutually exclusive — a tab bar makes
	// that explicit. Each tab keeps its keyed IGFD browser.
	if ( ImGui::BeginTabBar( "##load" ) ) {
		if ( ImGui::BeginTabItem( m_pCommonStrings->getSong().toUtf8()
									  .constData() ) ) {
			if ( ImGui::Button( m_pCommonStrings->getLoadSongButton().toUtf8()
									.constData() ) ) {
				IGFD::FileDialogConfig config;
				config.path = ".";
				config.countSelectionMax = 1;
				m_fileDialog.OpenDialog(
					"song",
					m_pCommonStrings->getSongFileDialogTitle().toStdString(),
					Filesystem::sSongSuffix.toUtf8() + ", " +
						Filesystem::sProjectSuffix.toUtf8(),
					config );
			}
			ImGui::EndTabItem();
		}
		if ( ImGui::BeginTabItem( m_pCommonStrings->getPattern().toUtf8()
									  .constData() ) ) {
			if ( ImGui::Button(
					 m_pCommonStrings->getLoadPatternButton().toUtf8()
						 .constData() ) ) {
				IGFD::FileDialogConfig config;
				config.path = ".";
				config.countSelectionMax = 1;
				m_fileDialog.OpenDialog(
					"pattern",
					m_pCommonStrings->getPatternFileDialogTitle().toStdString(),
					Filesystem::sPatternSuffix.toUtf8(), config );
			}

			// Pattern-load target: the slot the next pattern load installs
			// into (0-based, like the GUI's pattern spinbox) and whether it
			// replaces the slot's current pattern.
			ImGui::SetNextItemWidth( 72.0f );
			ImGui::InputInt(
				m_pCommonStrings->getInputCapturePattern().toUtf8()
					.constData(),
				&m_nPatternNumber, 0, 0 );
			m_nPatternNumber = std::max( 0, m_nPatternNumber );
			ImGui::SameLine();
			ImGui::Checkbox(
				m_pCommonStrings->getReplaceLabel().toUtf8().constData(),
				&m_bReplacePattern );
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	// The info section: what the engine is doing right now — the loaded
	// song and drumkit, the mode, and the mode-specific playback state.
	// One model snapshot per frame; the frame gate keeps the cadence
	// request-driven.
	const auto info = m_model.infoSnapshot();

	ImGui::Separator();

	// Empty values (no song, nothing playing) show as a dash instead of
	// a blank.
	const auto row = []( const QString& sLabel, const QString& sValue ) {
		ImGui::TextUnformatted( sLabel.toUtf8().constData() );
		ImGui::SameLine();
		const QString sShown = sValue.isEmpty() ? QString( "-" ) : sValue;
		ImGui::TextUnformatted( sShown.toUtf8().constData() );
	};

	QString sMode;
	if ( info.mode == Song::Mode::Song ) {
		sMode = m_pCommonStrings->getSong();
	}
	else if ( info.mode == Song::Mode::Pattern ) {
		sMode = m_pCommonStrings->getPattern();
	}
	row( m_pCommonStrings->getModeLabel(), sMode );
	row( m_pCommonStrings->getSong(), info.sSongName );
	row( m_pCommonStrings->getDrumkitLabel(), info.sDrumkitName );

	if ( info.mode == Song::Mode::Song ) {
		row( m_pCommonStrings->getPatternsLabel(),
			 QString::number( info.nPatternCount ) );
		row( m_pCommonStrings->getColumnsLabel(),
			 QString::number( info.nColumnCount ) );
		row( m_pCommonStrings->getCurrentColumnLabel(),
			 info.nCurrentColumn >= 0
				 ? QString::number( info.nCurrentColumn )
				 : QString() );
		QString sPlaying;
		for ( const auto& sName : info.playingPatterns ) {
			if ( ! sPlaying.isEmpty() ) {
				sPlaying += ", ";
			}
			sPlaying += sName;
		}
		row( m_pCommonStrings->getPlayingLabel(), sPlaying );
	}
	else if ( info.mode == Song::Mode::Pattern ) {
		row( m_pCommonStrings->getPattern(), info.sPatternName );
		row( m_pCommonStrings->getNotePropertyLength(),
			 QString::number( info.nPatternLength ) );
		row( m_pCommonStrings->getNoteCountLabel(),
			 QString::number( info.nPatternNoteCount ) );
	}

	ImGui::Separator();

	// Drumkit loading is independent of the tabs — a kit can be swapped
	// no matter what is loaded next. The picker lists the sound library
	// database's kits instead of browsing the filesystem.
	if ( ImGui::Button( m_pCommonStrings->getActionSwitchDrumkit().toUtf8()
							.constData() ) ) {
		const QByteArray sTitle =
			m_pCommonStrings->getActionSwitchDrumkit().toUtf8();
		ImGui::OpenPopup( sTitle.constData() );
	}
	drawDrumkitPicker();

	ImGui::Separator();

	// Editor line: the executable the button launches — prefilled with
	// the resolved default, editable to point elsewhere.
	ImGui::SetNextItemWidth( 240.0f );
	ImGui::InputText( m_pCommonStrings->getEditorExecutableLabel().toUtf8()
						  .constData(),
					  m_sEditorExecutableBuffer.data(),
					  m_sEditorExecutableBuffer.size() );
	ImGui::SameLine();
	if ( ImGui::Button( m_pCommonStrings->getOpenEditorButton().toUtf8()
							.constData() ) ) {
		// The line edit's content is what launches; an empty field
		// restores the default resolution.
		m_model.setEditorExecutable(
			QString::fromUtf8( m_sEditorExecutableBuffer.data() ) );
		m_model.openEditor();
	}

	// The keyed browsers — one IGFD instance, at most one dialog open.
	// Drawn unconditionally so an open browser survives tab switches.
	//
	// The default size on Linux+wayland is far too tiny.
	const ImVec2 minSize =
		ImVec2( HydrogenPlugin::nMinWidth, HydrogenPlugin::nMinHeight );
	if ( m_fileDialog.Display( "song", ImGuiWindowFlags_NoCollapse, minSize ) ) {
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
}

void PluginUiLandingView::drawDrumkitPicker()
{
	// The picker reuses the load button's string as its title — no extra
	// translatable. Sized like the file browser it replaces.
	const QByteArray sTitle = m_pCommonStrings->getActionSwitchDrumkit().toUtf8();
	ImGui::SetNextWindowSize(
		ImVec2( HydrogenPlugin::nMinWidth, HydrogenPlugin::nMinHeight ),
		ImGuiCond_Appearing );
	if ( ! ImGui::BeginPopupModal( sTitle.constData(), nullptr,
								   ImGuiWindowFlags_NoCollapse ) ) {
		return;
	}

	// Re-queried every frame: the listing fills in as the database's
	// initial scan progresses.
	const auto kits = m_model.drumkits();

	if ( ImGui::BeginTable( "##kits", 4,
							ImGuiTableFlags_ScrollY |
								ImGuiTableFlags_RowBg |
								ImGuiTableFlags_Borders,
							ImVec2( 0.0f, 300.0f ) ) ) {
		ImGui::TableSetupScrollFreeze( 0, 1 );
		ImGui::TableSetupColumn(
			m_pCommonStrings->getNameDialog().toUtf8().constData() );
		ImGui::TableSetupColumn(
			m_pCommonStrings->getAuthorDialog().toUtf8().constData() );
		ImGui::TableSetupColumn(
			m_pCommonStrings->getLicenseDialog().toUtf8().constData() );
		ImGui::TableSetupColumn(
			m_pCommonStrings->getVersionDialog().toUtf8().constData() );
		ImGui::TableHeadersRow();

		for ( const auto& kit : kits ) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			// The "##" suffix keeps imgui IDs apart when two kits share a
			// name; the visible label stays the bare name.
			const QByteArray sLabel =
				kit.sName.toUtf8() + "##" + kit.sPath.toUtf8();
			if ( ImGui::Selectable( sLabel.constData(), false,
									ImGuiSelectableFlags_SpanAllColumns ) ) {
				m_model.loadDrumkit( kit.sPath );
				ImGui::CloseCurrentPopup();
			}
			if ( ! kit.sInfo.isEmpty() && ImGui::IsItemHovered() ) {
				const QByteArray sInfo = kit.sInfo.toUtf8();
				ImGui::SetTooltip( "%s", sInfo.constData() );
			}
			ImGui::TableNextColumn();
			const QByteArray sAuthor = kit.sAuthor.toUtf8();
			ImGui::TextUnformatted( sAuthor.constData() );
			ImGui::TableNextColumn();
			const QByteArray sLicense = kit.sLicense.toUtf8();
			ImGui::TextUnformatted( sLicense.constData() );
			ImGui::TableNextColumn();
			const QByteArray sVersion =
				QString::number( kit.nVersion ).toUtf8();
			ImGui::TextUnformatted( sVersion.constData() );
		}
		ImGui::EndTable();
	}

	if ( ImGui::Button( m_pCommonStrings->getButtonClose().toUtf8()
							.constData() ) ) {
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

} // namespace H2Core
