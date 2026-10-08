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
// instrument strips (fader, pan knob, mute/solo, meters) and the bus
// row with drag&drop routing (ADR 0019): dragging a strip onto a bus
// slot (or Master) sets its explicit output bus, the béziers visualize
// the routings. Thin draw calls only (TU1.3): every decision is the
// model's.

#include "PluginUiMixerView.h"

#include <imgui.h>
#include <imgui-knobs.h>

#include <algorithm>
#include <vector>

#include <QString>

#include "PluginUiStrings.h"

namespace {

// Drag&drop payload type of a strip's bus routing (TU4.4).
const char* s_sStripBusPayload = "H2_STRIP_BUS";

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
	: m_model( pPlugin )
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
	const int nBusCount = m_model.busCount();

	// Shared button width so the strips' control columns align.
	const auto sMute = PluginUiStrings::muteButton().toUtf8();
	const auto sSolo = PluginUiStrings::soloButton().toUtf8();
	const float nButtonWidth =
		std::max( ImGui::CalcTextSize( sMute.constData() ).x,
				  ImGui::CalcTextSize( sSolo.constData() ).x ) +
		ImGui::GetStyle().FramePadding.x * 2.0f;

	// Strip routing state, recorded while drawing for the béziers: each
	// strip's bus-indicator rect and effective bus.
	std::vector<ImVec2> stripOrigins;
	std::vector<int> stripBuses;
	stripOrigins.reserve( nStrips );
	stripBuses.reserve( nStrips );

	// The master strip: the same column geometry, no pan/solo/routing.
	const auto highlight = []() {
		ImGui::PushStyleColor( ImGuiCol_Text,
							   ImGui::GetColorU32(
								   ImGuiCol_ButtonActive ) );
	};
	const auto sMasterOnly = PluginUiStrings::masterOnlyLabel().toUtf8();
	const auto sReset = PluginUiStrings::resetButton().toUtf8();
	ImGui::BeginGroup();
	ImGui::TextUnformatted( PluginUiStrings::masterLabel().toUtf8()
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
	// meters, pan, mute, solo, bus indicator, reset — so the strips
	// align and each name clips to its column.
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
		if ( ImGuiKnobs::Knob( PluginUiStrings::panLabel().toUtf8()
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
		// The strip's routing state — the indicator is the drag
		// source. Text() items carry no ID, so imgui requires an
		// explicit opt-in for them to act as drag sources.
		const int nBus = m_model.effectiveOutputBus( n, nBusCount );
		const auto sBus = nBus >= 0
			? ( PluginUiStrings::busLabel() + " " +
				QString::number( nBus + 1 ) ).toUtf8()
			: QByteArray( sMasterOnly );
		ImGui::TextUnformatted( sBus.constData(),
								sBus.constData() + sBus.size() );
		const ImVec2 stripOrigin = ImGui::GetItemRectMin();
		if ( ImGui::BeginDragDropSource(
				 ImGuiDragDropFlags_SourceAllowNullID ) ) {
			ImGui::SetDragDropPayload( s_sStripBusPayload, &n,
									   sizeof( int ) );
			ImGui::TextUnformatted( sBus.constData(),
									sBus.constData() + sBus.size() );
			ImGui::EndDragDropSource();
		}
		if ( ImGui::SmallButton( sReset.constData() ) ) {
			// Back to the implicit 1-to-1 default (ADR 0019).
			m_model.setInstrumentOutputBus( n, -1 );
		}
		ImGui::Dummy( ImVec2( s_nStripWidth, 0.0f ) );
		ImGui::EndGroup();
		ImGui::PopID();
		if ( n + 1 < nStrips ) {
			ImGui::SameLine();
		}
		stripOrigins.push_back( stripOrigin );
		stripBuses.push_back( nBus );
	}

	ImGui::Separator();

	// The bus row: one slot per host bus plus Master. Dropping a strip
	// sets its explicit output bus; Master resets it to the master
	// feed.
	std::vector<ImVec2> busOrigins;
	for ( int nBus = 0; nBus < nBusCount; ++nBus ) {
		const auto sBus = ( PluginUiStrings::busLabel() + " " +
							QString::number( nBus + 1 ) ).toUtf8();
		ImGui::Button( sBus.constData() );
		if ( ImGui::BeginDragDropTarget() ) {
			if ( const ImGuiPayload* pPayload =
					 ImGui::AcceptDragDropPayload( s_sStripBusPayload ) ) {
				const int nStrip =
					*static_cast<const int*>( pPayload->Data );
				m_model.setInstrumentOutputBus( nStrip, nBus );
			}
			ImGui::EndDragDropTarget();
		}
		busOrigins.push_back( ImGui::GetItemRectMin() );
		ImGui::SameLine();
	}
	ImGui::Button( PluginUiStrings::masterLabel().toUtf8().constData() );
	if ( ImGui::BeginDragDropTarget() ) {
		if ( const ImGuiPayload* pPayload =
				 ImGui::AcceptDragDropPayload( s_sStripBusPayload ) ) {
			const int nStrip = *static_cast<const int*>( pPayload->Data );
			m_model.setInstrumentOutputBus( nStrip, -1 );
		}
		ImGui::EndDragDropTarget();
	}
	busOrigins.push_back( ImGui::GetItemRectMin() );

	// The routing béziers (TU4.4): each strip to its effective bus —
	// the Master slot for the master-only strips. Full text alpha:
	// the routings are the point of the bus row, not a watermark.
	if ( ! busOrigins.empty() ) {
		ImDrawList* pDraw = ImGui::GetWindowDrawList();
		const ImU32 nCurveColor = ImGui::GetColorU32( ImGuiCol_Text );
		for ( int n = 0; n < nStrips; ++n ) {
			const int nBus = stripBuses[ n ];
			const ImVec2 from = stripOrigins[ n ];
			const ImVec2 to = busOrigins[ nBus >= 0 ? nBus : nBusCount ];
			const float nDip = std::max( ( to.y - from.y ) * 0.5f, 8.0f );
			pDraw->AddBezierCubic( from,
								  ImVec2( from.x, from.y + nDip ),
								  ImVec2( to.x, to.y - nDip ), to,
								  nCurveColor, 2.0f );
		}
	}
}

} // namespace H2Core
