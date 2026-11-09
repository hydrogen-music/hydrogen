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
// instrument strips (fader, pan knob, mute/solo, meters). The master
// strip sits fixed beside the strips' scroll lane — narrow windows
// scroll the strips, never the master. The bus routing (ADR 0019)
// lives in the mapping view's "Bus Output" section — its béziers
// overwhelmed the strips here. Thin draw calls only (TU1.3): every
// decision is the model's.

#include "PluginUiMixerView.h"

#include <imgui.h>
#include <imgui-knobs.h>

#include <algorithm>

#include <QString>

#include <core/CommonStrings.h>
#include <core/Preferences/Color.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>
#include <plugin/HydrogenPlugin.h>

namespace {

// The strips' shared geometry — every strip is a fixed-width column,
// so the strips align and each name clips to its column.
constexpr float s_nFaderWidth = 28.0f;
constexpr float s_nFaderHeight = 120.0f;
constexpr float s_nMeterBarWidth = 5.0f;
constexpr float s_nMeterGap = 1.0f;
constexpr float s_nKnobSize = 34.0f;
constexpr float s_nStripWidth = 48.0f;
// The meters hug the fader so the pair reads as one unit.
constexpr float s_nFaderMeterGap = 2.0f;
// The L/R scale labels tuck this far under the pan knob's sides: at
// the knob rect's edges they read as a detached row, this close they
// read as the knob's scale.
constexpr float s_nPanLabelInset = 5.0f;
// The L/R scale labels ride this far up into the pan knob's row: the
// knob rect's bottom corners are empty beside the circle, so the
// scale sits inside the widget. The label row reserves only its part
// below the knob's row, so the following widgets move up with it.
constexpr float s_nPanLabelTuck = 7.0f;
// The mute/solo pair's seam: borderless imgui buttons fuse into one
// block when they touch — one pixel keeps the two readable.
constexpr float s_nMuteSoloGap = 1.0f;
// The strip lane's id — the smoke test finds the lane window by this
// name to assert its scrollbar.
constexpr const char* s_sStripLaneId = "##strips";

// Truncates @a sName to @a nMaxWidth pixels — a plain cut, no
// ellipsis: the "..." ate too much of the narrow column, and the
// hover tooltip carries the full name anyway. The chop is
// character-wise, not byte-wise — it must not split a multi-byte
// UTF-8 sequence.
QString truncated( const QString& sName, float nMaxWidth ) {
	QString s = sName;
	while ( ! s.isEmpty() &&
			ImGui::CalcTextSize( s.toUtf8().constData() ).x > nMaxWidth ) {
		s.chop( 1 );
	}
	return s;
}

// The theme's colors are 8-bit RGB; imgui wants floats.
ImVec4 toImVec4( const H2Core::Color& color ) {
	return ImVec4( color.red() / 255.0f, color.green() / 255.0f,
				   color.blue() / 255.0f, 1.0f );
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
	ImGui::SameLine( 0.0f, s_nFaderMeterGap );

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
	m_layout = PluginUiMixerLayout{};
	const int nStrips = m_model.stripCount();

	// The theme's engaged mute/solo palette — the same colors the GUI
	// mixer's checked buttons wear. Read per frame: cheap, and a theme
	// change shows without a restart. Null when the engine is
	// unavailable — then nothing is muted or soloed either, so no
	// strip ever misses its palette.
	const auto pPreferences = m_model.preferences();
	const auto pColorTheme = pPreferences != nullptr
		? pPreferences->getColorTheme() : nullptr;

	const auto sMute = m_pCommonStrings->getSmallMuteButton().toUtf8();
	const auto sSolo = m_pCommonStrings->getSmallSoloButton().toUtf8();
	const float nButtonWidth =
		std::max( ImGui::CalcTextSize( sMute.constData() ).x,
				  ImGui::CalcTextSize( sSolo.constData() ).x ) +
		ImGui::GetStyle().FramePadding.x * 2.0f;
	const float nFaderBlockWidth = s_nFaderWidth + s_nFaderMeterGap +
		s_nMeterBarWidth * 2.0f + s_nMeterGap;

	// Centers the next item in the strip column: at a line start the
	// cursor sits at the column's left edge (a group indents its
	// following lines to its start).
	const auto centerInStrip = []( float nItemWidth ) {
		ImGui::SetCursorPosX( ImGui::GetCursorPosX() +
							  ( s_nStripWidth - nItemWidth ) * 0.5f );
	};
	// A mute/solo button's full palette: engaged — the theme color as
	// the button face (hover and active included, the engaged state
	// must not wash out under the mouse) with its paired text color;
	// unchecked — a neutral face with the text in the state color,
	// the GUI mixer's MuteButton convention (its unchecked text
	// wears the state color too). Returns the pushed color count for
	// the matching pop.
	const auto pushState = []( bool bEngaged, const Color& state,
							   const Color& engagedText ) {
		if ( bEngaged ) {
			const ImVec4 faceColor = toImVec4( state );
			ImGui::PushStyleColor( ImGuiCol_Button, faceColor );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, faceColor );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, faceColor );
			ImGui::PushStyleColor( ImGuiCol_Text,
								   toImVec4( engagedText ) );
			return 4;
		}
		ImGui::PushStyleColor( ImGuiCol_Text, toImVec4( state ) );
		return 1;
	};

	// The master strip: fixed beside the strip lane (the Qt mixer's
	// layout) — it never scrolls away while the strips pan.
	ImGui::BeginGroup();
	ImGui::TextUnformatted( m_pCommonStrings->getMasterLabel().toUtf8()
								.constData() );
	float fMasterVolume = m_model.masterVolume();
	centerInStrip( nFaderBlockWidth );
	if ( drawFaderWithMeter( "##masterVolume", fMasterVolume,
							 m_model.masterPeakL(),
							 m_model.masterPeakR() ) ) {
		m_model.setMasterVolume( fMasterVolume );
	}
	const bool bMasterMuted = m_model.masterIsMuted();
	const int nMasterPop = pColorTheme != nullptr
		? pushState( bMasterMuted, pColorTheme->m_muteColor,
					 pColorTheme->m_muteTextColor )
		: 0;
	// The full column width: the master mute shares the strip's
	// horizontal boundaries.
	if ( ImGui::Button( sMute.constData(), ImVec2( s_nStripWidth, 0 ) ) ) {
		m_model.setMasterIsMuted( ! bMasterMuted );
	}
	if ( nMasterPop > 0 ) {
		ImGui::PopStyleColor( nMasterPop );
	}
	m_layout.masterMuteMin = ImGui::GetItemRectMin();
	m_layout.masterMuteMax = ImGui::GetItemRectMax();
	ImGui::Dummy( ImVec2( s_nStripWidth, 0.0f ) );
	m_layout.masterStripMin = ImGui::GetItemRectMin();
	m_layout.masterStripMax = ImGui::GetItemRectMax();
	ImGui::EndGroup();
	ImGui::SameLine();

	// The strip lane: the strips scroll horizontally when the window
	// is too narrow (imgui hides the horizontal scrollbar unless the
	// window asks for one), and the lane hugs its strips vertically so
	// the scrollbar sits right below them.
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
	if ( ImGui::BeginChild( s_sStripLaneId, ImVec2( 0.0f, 0.0f ),
							ImGuiChildFlags_AutoResizeY,
							ImGuiWindowFlags_HorizontalScrollbar ) ) {
		// One group per strip: a fixed-width column — name, fader with
		// meters, pan, mute, solo — so the strips align and each name
		// clips to its column.
		for ( int n = 0; n < nStrips; ++n ) {
			ImGui::PushID( n );
			ImGui::BeginGroup();
			// The truncated name; the full name rides in a hover
			// tooltip — the cut hides it otherwise.
			const QString sFullName = m_model.stripName( n );
			const auto sName =
				truncated( sFullName, s_nStripWidth ).toUtf8();
			if ( m_layout.sNameFull.isEmpty() &&
				 ImGui::CalcTextSize( sFullName.toUtf8().constData() ).x >
					 s_nStripWidth ) {
				m_layout.sNameFull = sFullName;
				m_layout.sNameShown = QString::fromUtf8( sName );
			}
			ImGui::TextUnformatted( sName.constData(),
									sName.constData() + sName.size() );
			if ( ImGui::IsItemHovered() ) {
				const auto sFull = sFullName.toUtf8();
				ImGui::SetTooltip( "%s", sFull.constData() );
			}
			float fVolume = m_model.stripVolume( n );
			centerInStrip( nFaderBlockWidth );
			if ( drawFaderWithMeter( "##volume", fVolume,
									 m_model.instrumentPeakL( n ),
									 m_model.instrumentPeakR( n ) ) ) {
				m_model.setStripVolume( n, fVolume );
			}
			// The pan knob: no title (the L/R scale below replaces it)
			// and no internal drag input — the knob's DragScalar
			// renders an empty frame below itself. The mouse drag
			// stays: the knob runs its own drag behavior.
			float fPan = m_model.stripPan( n );
			centerInStrip( s_nKnobSize );
			if ( ImGuiKnobs::Knob( "##pan", &fPan, -1.0f, 1.0f, 0, "%.2f",
								   ImGuiKnobVariant_Tick, s_nKnobSize,
								   ImGuiKnobFlags_NoTitle |
									   ImGuiKnobFlags_NoInput ) ) {
				m_model.setStripPan( n, fPan );
			}
			// The pan scale: "L" and "R" tucked into the knob's bottom
			// corners — draw-list text (nothing to hover); the row
			// reserves only the label part below the knob's row.
			{
				const ImVec2 knobMin = ImGui::GetItemRectMin();
				const ImVec2 knobMax = ImGui::GetItemRectMax();
				if ( n == 0 ) {
					m_layout.knobMin = knobMin;
					m_layout.knobMax = knobMax;
				}
				const auto sL = m_pCommonStrings->getPanScaleLeft()
									.toUtf8();
				const auto sR = m_pCommonStrings->getPanScaleRight()
									.toUtf8();
				const ImVec2 nLSize =
					ImGui::CalcTextSize( sL.constData() );
				const ImVec2 nRSize =
					ImGui::CalcTextSize( sR.constData() );
				const float nLabelHeight = std::max( nLSize.y, nRSize.y );
				ImGui::Dummy( ImVec2( s_nStripWidth,
									  std::max( nLabelHeight -
													s_nPanLabelTuck,
												0.0f ) ) );
				const ImVec2 rowMin = ImGui::GetItemRectMin();
				ImDrawList* pDraw = ImGui::GetWindowDrawList();
				const ImU32 nTextColor =
					ImGui::GetColorU32( ImGuiCol_Text );
				const ImVec2 nLPos( knobMin.x + s_nPanLabelInset,
									 rowMin.y - s_nPanLabelTuck );
				const ImVec2 nRPos( knobMax.x - nRSize.x - s_nPanLabelInset,
									 rowMin.y - s_nPanLabelTuck );
				if ( n == 0 ) {
					m_layout.labelLMin = nLPos;
					m_layout.labelLMax =
						ImVec2( nLPos.x + nLSize.x, nLPos.y + nLSize.y );
					m_layout.labelRMin = nRPos;
					m_layout.labelRMax =
						ImVec2( nRPos.x + nRSize.x, nRPos.y + nRSize.y );
				}
				pDraw->AddText( nLPos, nTextColor, sL.constData() );
				pDraw->AddText( nRPos, nTextColor, sR.constData() );
			}
			// The mute/solo pair on one line: equal-width buttons
			// centered as a pair around a one-pixel seam on the
			// strip's center axis.
			const bool bMuted = m_model.stripIsMuted( n );
			const int nMutePop = pColorTheme != nullptr
				? pushState( bMuted, pColorTheme->m_muteColor,
							 pColorTheme->m_muteTextColor )
				: 0;
			centerInStrip( nButtonWidth * 2.0f + s_nMuteSoloGap );
			if ( ImGui::Button( sMute.constData(),
								ImVec2( nButtonWidth, 0 ) ) ) {
				m_model.setStripIsMuted( n, ! bMuted );
			}
			if ( nMutePop > 0 ) {
				ImGui::PopStyleColor( nMutePop );
			}
			if ( n == 0 ) {
				m_layout.muteMin = ImGui::GetItemRectMin();
				m_layout.muteMax = ImGui::GetItemRectMax();
			}
			ImGui::SameLine( 0.0f, s_nMuteSoloGap );
			const bool bSoloed = m_model.stripIsSoloed( n );
			const int nSoloPop = pColorTheme != nullptr
				? pushState( bSoloed, pColorTheme->m_soloColor,
							 pColorTheme->m_soloTextColor )
				: 0;
			if ( ImGui::Button( sSolo.constData(),
								ImVec2( nButtonWidth, 0 ) ) ) {
				m_model.setStripIsSoloed( n, ! bSoloed );
			}
			if ( nSoloPop > 0 ) {
				ImGui::PopStyleColor( nSoloPop );
			}
			if ( n == 0 ) {
				m_layout.soloMin = ImGui::GetItemRectMin();
				m_layout.soloMax = ImGui::GetItemRectMax();
			}
			ImGui::Dummy( ImVec2( s_nStripWidth, 0.0f ) );
			if ( n == 0 ) {
				m_layout.stripMin = ImGui::GetItemRectMin();
				m_layout.stripMax = ImGui::GetItemRectMax();
				m_layout.bValid = true;
			}
			ImGui::EndGroup();
			ImGui::PopID();
			if ( n + 1 < nStrips ) {
				ImGui::SameLine();
			}
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

} // namespace H2Core
