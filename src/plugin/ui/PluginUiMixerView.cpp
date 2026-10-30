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

// The mixer view's renderer (proposal 0006 TU4.3/TU4.4): master and
// instrument strips (fader, pan knob, mute/solo, meters). The bus
// routing (ADR 0019) lives in the mapping view's "Bus Output" section
// — its béziers overwhelmed the strips here. Thin draw calls only
// (TU1.3): every decision is the model's.

#include "PluginUiMixerView.h"

#include <imgui.h>
#include <imgui-knobs.h>

#include <algorithm>

#include <QString>

#include <core/CommonStrings.h>
#include <plugin/HydrogenPlugin.h>

namespace {

// The strips' shared geometry — every strip is a fixed-width column,
// so the strips align and each name clips to its column.
constexpr float s_nFaderWidth = 28.0f;
constexpr float s_nFaderHeight = 120.0f;
constexpr float s_nMeterBarWidth = 5.0f;
constexpr float s_nMeterGap = 1.0f;
constexpr float s_nKnobSize = 34.0f;
constexpr float s_nStripWidth = 80.0f;

// Elides @a sName to @a nMaxWidth pixels with a trailing "..." (the
// default bitmap font has no ellipsis glyph). The chop is
// character-wise, not byte-wise — it must not split a multi-byte
// UTF-8 sequence.
QString elided( const QString& sName, float nMaxWidth ) {
	if ( ImGui::CalcTextSize( sName.toUtf8().constData() ).x <=
		 nMaxWidth ) {
		return sName;
	}
	const QString sEllipsis = "...";
	const float nEllipsisWidth =
		ImGui::CalcTextSize( sEllipsis.toUtf8().constData() ).x;
	QString s = sName;
	while ( ! s.isEmpty() &&
			ImGui::CalcTextSize( s.toUtf8().constData() ).x +
					nEllipsisWidth > nMaxWidth ) {
		s.chop( 1 );
	}
	return s + sEllipsis;
}

} // namespace

namespace H2Core {

PluginUiMixerView::PluginUiMixerView( HydrogenPlugin* pPlugin )
	: m_model( pPlugin ), m_pCommonStrings( pPlugin->commonStrings() )
{
}

bool PluginUiMixerView::drawFaderWithMeter( const char* sId,
											 float& fVolume, float fPeakL,
											 float fPeakR )
{
	const bool bMoved = ImGui::VSliderFloat(
		sId, ImVec2( s_nFaderWidth, s_nFaderHeight ), &fVolume, 0.0f, 1.0f );
	ImGui::SameLine();

	// The L/R meter pair beside the fader: each bar fills from the
	// bottom with its peak.
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	ImDrawList* pDraw = ImGui::GetWindowDrawList();
	const ImU32 nFrameColor = ImGui::GetColorU32( ImGuiCol_FrameBg );
	const ImU32 nPeakColor = ImGui::GetColorU32( ImGuiCol_PlotHistogram );
	for ( int nBar = 0; nBar < 2; ++nBar ) {
		const float fPeak = std::clamp( nBar == 0 ? fPeakL : fPeakR, 0.0f,
										1.0f );
		const float nBarX =
			origin.x + nBar * ( s_nMeterBarWidth + s_nMeterGap );
		pDraw->AddRectFilled( ImVec2( nBarX, origin.y ),
							  ImVec2( nBarX + s_nMeterBarWidth,
									  origin.y + s_nFaderHeight ),
							  nFrameColor );
		pDraw->AddRectFilled(
			ImVec2( nBarX, origin.y + s_nFaderHeight * ( 1.0f - fPeak ) ),
			ImVec2( nBarX + s_nMeterBarWidth, origin.y + s_nFaderHeight ),
			nPeakColor );
	}
	ImGui::Dummy( ImVec2( s_nMeterBarWidth * 2.0f + s_nMeterGap,
						  s_nFaderHeight ) );
	return bMoved;
}

void PluginUiMixerView::draw()
{
	const int nStrips = m_model.stripCount();

	// Shared button width so the strips' control columns align.
	const auto sMute = m_pCommonStrings->getBigMuteButton().toUtf8();
	const auto sSolo = m_pCommonStrings->getBigSoloButton().toUtf8();
	const float nButtonWidth =
		std::max( ImGui::CalcTextSize( sMute.constData() ).x,
				  ImGui::CalcTextSize( sSolo.constData() ).x ) +
		ImGui::GetStyle().FramePadding.x * 2.0f;

	// The master strip: the same column geometry, no pan/solo/routing.
	const auto highlight = []() {
		ImGui::PushStyleColor( ImGuiCol_Text,
							   ImGui::GetColorU32(
								   ImGuiCol_ButtonActive ) );
	};
	ImGui::BeginGroup();
	ImGui::TextUnformatted( m_pCommonStrings->getMasterLabel().toUtf8()
								.constData() );
	float fMasterVolume = m_model.masterVolume();
	if ( drawFaderWithMeter( "##masterVolume", fMasterVolume,
							 m_model.masterPeakL(),
							 m_model.masterPeakR() ) ) {
		m_model.setMasterVolume( fMasterVolume );
	}
	const bool bMasterMuted = m_model.masterIsMuted();
	if ( bMasterMuted ) {
		highlight();
	}
	if ( ImGui::Button( sMute.constData(), ImVec2( nButtonWidth, 0 ) ) ) {
		m_model.setMasterIsMuted( ! bMasterMuted );
	}
	if ( bMasterMuted ) {
		ImGui::PopStyleColor();
	}
	ImGui::Dummy( ImVec2( s_nStripWidth, 0.0f ) );
	ImGui::EndGroup();
	ImGui::SameLine();

	// One group per strip: a fixed-width column — name, fader with
	// meters, pan, mute, solo — so the strips align and each name clips
	// to its column.
	for ( int n = 0; n < nStrips; ++n ) {
		ImGui::PushID( n );
		ImGui::BeginGroup();
		const auto sName =
			elided( m_model.stripName( n ), s_nStripWidth ).toUtf8();
		ImGui::TextUnformatted( sName.constData(),
								sName.constData() + sName.size() );
		float fVolume = m_model.stripVolume( n );
		if ( drawFaderWithMeter( "##volume", fVolume,
								 m_model.instrumentPeakL( n ),
								 m_model.instrumentPeakR( n ) ) ) {
			m_model.setStripVolume( n, fVolume );
		}
		float fPan = m_model.stripPan( n );
		if ( ImGuiKnobs::Knob( m_pCommonStrings->getNotePropertyPan().toUtf8()
								   .constData(),
							   &fPan, -1.0f, 1.0f, 0, "%.2f",
							   ImGuiKnobVariant_Tick, s_nKnobSize ) ) {
			m_model.setStripPan( n, fPan );
		}
		const bool bMuted = m_model.stripIsMuted( n );
		if ( bMuted ) {
			highlight();
		}
		if ( ImGui::Button( sMute.constData(), ImVec2( nButtonWidth, 0 ) ) ) {
			m_model.setStripIsMuted( n, ! bMuted );
		}
		if ( bMuted ) {
			ImGui::PopStyleColor();
		}
		const bool bSoloed = m_model.stripIsSoloed( n );
		if ( bSoloed ) {
			highlight();
		}
		if ( ImGui::Button( sSolo.constData(), ImVec2( nButtonWidth, 0 ) ) ) {
			m_model.setStripIsSoloed( n, ! bSoloed );
		}
		if ( bSoloed ) {
			ImGui::PopStyleColor();
		}
		ImGui::Dummy( ImVec2( s_nStripWidth, 0.0f ) );
		ImGui::EndGroup();
		ImGui::PopID();
		if ( n + 1 < nStrips ) {
			ImGui::SameLine();
		}
	}
}

} // namespace H2Core
