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
#include "PluginUiStyle.h"

#include <imgui.h>
#include <imgui-knobs.h>

#include <algorithm>
#include <cmath>

#include <QString>

#include <core/CommonStrings.h>
#include <core/Preferences/Color.h>
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
// How far a strip's panel reaches beyond its widgets — the GUI's strips
// are framed panels, not bare widget columns.
constexpr float s_nStripPanelPadding = 3.0f;

// The GUI Rotary's geometry (src/gui/src/Widgets/Rotary.cpp), in its
// 26px-high widget's pixels; the plugin's knob scales it to its own size.
constexpr float s_fRotaryHeight = 26.0f;
constexpr float s_fRotaryArcWidth = 2.0f;
constexpr float s_fRotaryHandleDistance = 4.0f;
constexpr float s_fRotaryHandleRadius = 1.5f;
constexpr float s_fRotaryHighlightHeight = 2.0f;
// Rotary's angles, in Qt's degrees (counter-clockwise from 3 o'clock):
// the center marker spans 91°..88°, the offset arc starts 18° off the
// top and sweeps up to 100° per half range, and the handle travels 255°
// over the full range clockwise from 12 o'clock.
constexpr float s_fRotaryCenterMarkStart = 91.0f;
constexpr float s_fRotaryCenterMarkSpan = -3.0f;
constexpr float s_fRotaryArcGap = 18.0f;
constexpr float s_fRotaryArcSweep = 200.0f;
constexpr float s_fRotaryHandleSweep = 255.0f;
// M_PI is not standard C++ (MSVC hides it).
constexpr float s_fPi = 3.14159265358979f;
// Rotary dims its focus highlight while it lacks keyboard focus.
constexpr float s_fRotaryHighlightAlpha = 150.0f / 255.0f;

// Qt degrees (counter-clockwise, y up) to imgui radians (clockwise,
// y down).
float qtDegreesToImGui( float fDegrees ) {
	return -fDegrees * s_fPi / 180.0f;
}

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

} // namespace

namespace H2Core {

PluginUiMixerView::PluginUiMixerView( HydrogenPlugin* pPlugin )
	: m_model( pPlugin ), m_pCommonStrings( pPlugin->commonStrings() )
{
}

bool PluginUiMixerView::drawFaderWithMeter( const char* sId,
											 float& fVolume, float fPeakL,
											 float fPeakR,
											 const PluginUiPalette& palette )
{
	// The fader's track and the meters share the GUI's dark well; the
	// handle shares the unexpanded combo face (the frame color).
	const ImVec4 well = PluginUiColor::toImVec4( palette.meterWell );
	ImGui::PushStyleColor( ImGuiCol_FrameBg, well );
	ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, well );
	ImGui::PushStyleColor( ImGuiCol_FrameBgActive, well );
	const bool bMoved = ImGui::VSliderFloat(
		sId, ImVec2( s_nFaderWidth, s_nFaderHeight ), &fVolume, 0.0f, 1.0f );
	ImGui::PopStyleColor( 3 );
	ImGui::SameLine( 0.0f, s_nFaderMeterGap );

	// The L/R meter pair beside the fader: each bar fills from the
	// bottom with its peak.
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	ImDrawList* pDraw = ImGui::GetWindowDrawList();
	const ImU32 nFrameColor = ImGui::GetColorU32( well );
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

bool PluginUiMixerView::drawPanKnob( const char* sId, float& fPan,
									 const PluginUiPalette& palette )
{
	// imgui-knobs keeps the interaction (drag, wheel, item id); its own
	// face is pushed fully transparent — its secondary colors scale RGB
	// only, so zero alpha hides every part — and Rotary's face is drawn
	// over the item rect instead. No title (the L/R scale replaces it)
	// and no internal drag input — the knob's DragScalar renders an
	// empty frame below itself.
	const ImVec4 transparent( 0.0f, 0.0f, 0.0f, 0.0f );
	ImGui::PushStyleColor( ImGuiCol_Button, transparent );
	ImGui::PushStyleColor( ImGuiCol_ButtonHovered, transparent );
	ImGui::PushStyleColor( ImGuiCol_ButtonActive, transparent );
	const bool bMoved = ImGuiKnobs::Knob(
		sId, &fPan, -1.0f, 1.0f, 0, "%.2f", ImGuiKnobVariant_Tick,
		s_nKnobSize, ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput );
	ImGui::PopStyleColor( 3 );
	const bool bHot = ImGui::IsItemHovered() || ImGui::IsItemActive();

	const ImVec2 itemMin = ImGui::GetItemRectMin();
	const ImVec2 itemMax = ImGui::GetItemRectMax();
	const float fSide =
		std::min( itemMax.x - itemMin.x, itemMax.y - itemMin.y );
	const float fScale = fSide / s_fRotaryHeight;
	const ImVec2 center( ( itemMin.x + itemMax.x ) * 0.5f,
						 itemMin.y + fSide * 0.5f );
	const float fArcWidth = s_fRotaryArcWidth * fScale;
	// Rotary's arc rect is its height minus the arc's width; the face
	// sits inside the arc's stroke.
	const float fArcRadius = ( fSide - fArcWidth ) * 0.5f;
	const float fFaceRadius = fArcRadius - fArcWidth;

	ImDrawList* pDraw = ImGui::GetWindowDrawList();
	pDraw->AddCircleFilled( center, fFaceRadius,
							ImGui::GetColorU32( PluginUiColor::toImVec4(
								palette.knobBody ) ) );
	pDraw->AddCircle( center, fFaceRadius,
					  ImGui::GetColorU32(
						  PluginUiColor::toImVec4( palette.knobRim ) ),
					  0, std::max( 1.0f, fScale ) );

	const auto arc = [ & ]( float fStartDegrees, float fSpanDegrees,
							const H2Core::Color& color, float fWidth ) {
		pDraw->PathClear();
		pDraw->PathArcTo( center, fArcRadius,
						  qtDegreesToImGui( fStartDegrees ),
						  qtDegreesToImGui( fStartDegrees + fSpanDegrees ) );
		pDraw->PathStroke(
			ImGui::GetColorU32( PluginUiColor::toImVec4( color ) ),
			ImDrawFlags_None, fWidth );
	};
	// Pan is centered at 0 over [-1, 1]: Rotary's Type::Center.
	const float fOffset = fPan * 0.5f; // ( value - mid ) / range
	if ( fPan == 0.0f ) {
		arc( s_fRotaryCenterMarkStart, s_fRotaryCenterMarkSpan,
			 palette.knobCenterSet, fArcWidth * 1.25f );
	}
	else {
		arc( s_fRotaryCenterMarkStart, s_fRotaryCenterMarkSpan,
			 palette.knobCenterUnset, fArcWidth * 1.25f );
		// Right of center the arc starts 18° clockwise off the top and
		// sweeps clockwise; left of center 16° counter-clockwise off it
		// (Rotary's extra 2° to clear the marker) and sweeps that way.
		const float fStart = fOffset > 0.0f
			? 90.0f - s_fRotaryArcGap
			: 90.0f + s_fRotaryArcGap - 2.0f;
		arc( fStart, -s_fRotaryArcSweep * fOffset, palette.knobArc,
			 fArcWidth );
	}

	// The handle dot: 255° of travel over the range, straight up at the
	// center (screen angles here — y points down).
	const float fHandleAngle =
		( -90.0f + s_fRotaryHandleSweep * fOffset ) *
		s_fPi / 180.0f;
	const float fHandleDistance = s_fRotaryHandleDistance * fScale;
	pDraw->AddCircleFilled(
		ImVec2( center.x + std::cos( fHandleAngle ) * fHandleDistance,
				center.y + std::sin( fHandleAngle ) * fHandleDistance ),
		s_fRotaryHandleRadius * fScale,
		ImGui::GetColorU32( PluginUiColor::toImVec4( palette.knobHandle ) ) );

	// Rotary's hover/focus bar along the widget's bottom edge.
	if ( bHot ) {
		pDraw->AddRectFilled(
			ImVec2( itemMin.x, itemMax.y - s_fRotaryHighlightHeight * fScale ),
			itemMax,
			ImGui::GetColorU32( PluginUiColor::toImVec4(
				palette.highlight, s_fRotaryHighlightAlpha ) ) );
	}
	return bMoved;
}

void PluginUiMixerView::draw( const PluginUiPalette& palette )
{
	m_layout = PluginUiMixerLayout{};
	const int nStrips = m_model.stripCount();

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
	// A mute/solo button's full palette: engaged — the state color as
	// the button face (hover and active included, the engaged state
	// must not wash out under the mouse) with its paired text color;
	// unchecked — MixerLine's darkened widget face with the text in the
	// state color (the GUI's MuteButton convention). Always four
	// colors, popped by the caller.
	const auto pushState = [ &palette ]( bool bEngaged, const Color& state,
										 const Color& engagedText ) {
		using PluginUiColor::toImVec4;
		if ( bEngaged ) {
			const ImVec4 faceColor = toImVec4( state );
			ImGui::PushStyleColor( ImGuiCol_Button, faceColor );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, faceColor );
			ImGui::PushStyleColor( ImGuiCol_ButtonActive, faceColor );
			ImGui::PushStyleColor( ImGuiCol_Text,
								   toImVec4( engagedText ) );
			return;
		}
		ImGui::PushStyleColor( ImGuiCol_Button,
							   toImVec4( palette.stripButton ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered,
							   toImVec4( palette.stripButtonHovered ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive,
							   toImVec4( state ) );
		ImGui::PushStyleColor( ImGuiCol_Text, toImVec4( state ) );
	};
	constexpr int nStateColors = 4;
	// The strips' framed panels: drawn behind each strip once its
	// extent is known — the splitter's background channel takes the
	// panel, the foreground one the widgets.
	const ImU32 nPanelColor = ImGui::GetColorU32(
		PluginUiColor::toImVec4( palette.stripPanel ) );
	const auto drawPanel = [ nPanelColor ]( ImDrawList* pDraw,
											ImDrawListSplitter& splitter ) {
		splitter.SetCurrentChannel( pDraw, 0 );
		pDraw->AddRectFilled(
			ImVec2( ImGui::GetItemRectMin().x - s_nStripPanelPadding,
					ImGui::GetItemRectMin().y - s_nStripPanelPadding ),
			ImVec2( ImGui::GetItemRectMax().x + s_nStripPanelPadding,
					ImGui::GetItemRectMax().y + s_nStripPanelPadding ),
			nPanelColor, ImGui::GetStyle().FrameRounding );
		splitter.SetCurrentChannel( pDraw, 1 );
	};

	// The master strip: fixed beside the strip lane (the Qt mixer's
	// layout) — it never scrolls away while the strips pan.
	ImDrawList* pMasterDraw = ImGui::GetWindowDrawList();
	ImDrawListSplitter masterSplitter;
	masterSplitter.Split( pMasterDraw, 2 );
	masterSplitter.SetCurrentChannel( pMasterDraw, 1 );
	ImGui::BeginGroup();
	ImGui::TextUnformatted( m_pCommonStrings->getMasterLabel().toUtf8()
								.constData() );
	float fMasterVolume = m_model.masterVolume();
	centerInStrip( nFaderBlockWidth );
	if ( drawFaderWithMeter( "##masterVolume", fMasterVolume,
							 m_model.masterPeakL(),
							 m_model.masterPeakR(), palette ) ) {
		m_model.setMasterVolume( fMasterVolume );
	}
	const bool bMasterMuted = m_model.masterIsMuted();
	pushState( bMasterMuted, palette.mute, palette.muteText );
	// The full column width: the master mute shares the strip's
	// horizontal boundaries.
	if ( ImGui::Button( sMute.constData(), ImVec2( s_nStripWidth, 0 ) ) ) {
		m_model.setMasterIsMuted( ! bMasterMuted );
	}
	ImGui::PopStyleColor( nStateColors );
	m_layout.masterMuteMin = ImGui::GetItemRectMin();
	m_layout.masterMuteMax = ImGui::GetItemRectMax();
	ImGui::Dummy( ImVec2( s_nStripWidth, 0.0f ) );
	m_layout.masterStripMin = ImGui::GetItemRectMin();
	m_layout.masterStripMax = ImGui::GetItemRectMax();
	ImGui::EndGroup();
	drawPanel( pMasterDraw, masterSplitter );
	masterSplitter.Merge( pMasterDraw );
	// Clear of the master panel's padding on both sides of the gap.
	ImGui::SameLine( 0.0f, ImGui::GetStyle().ItemSpacing.x +
						   s_nStripPanelPadding * 2.0f );

	// The strip lane: the strips scroll horizontally when the window
	// is too narrow (imgui hides the horizontal scrollbar unless the
	// window asks for one), and the lane hugs its strips vertically so
	// the scrollbar sits right below them.
	// The lane is the GUI mixer's dark backdrop the strip panels sit on;
	// its padding keeps the outermost panels' frames inside it.
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding,
						 ImVec2( s_nStripPanelPadding,
								 s_nStripPanelPadding ) );
	ImGui::PushStyleColor( ImGuiCol_ChildBg,
						   PluginUiColor::toImVec4( palette.stripLane ) );
	if ( ImGui::BeginChild( s_sStripLaneId, ImVec2( 0.0f, 0.0f ),
							ImGuiChildFlags_AutoResizeY |
								ImGuiChildFlags_AlwaysUseWindowPadding,
							ImGuiWindowFlags_HorizontalScrollbar ) ) {
		ImDrawList* pLaneDraw = ImGui::GetWindowDrawList();
		ImDrawListSplitter laneSplitter;
		laneSplitter.Split( pLaneDraw, 2 );
		laneSplitter.SetCurrentChannel( pLaneDraw, 1 );
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
			if ( n == 0 ) {
				m_layout.nameMin = ImGui::GetItemRectMin();
				m_layout.nameMax = ImGui::GetItemRectMax();
			}
			if ( ImGui::IsItemHovered() ) {
				const auto sFull = sFullName.toUtf8();
				ImGui::SetTooltip( "%s", sFull.constData() );
			}
			float fVolume = m_model.stripVolume( n );
			centerInStrip( nFaderBlockWidth );
			if ( drawFaderWithMeter( "##volume", fVolume,
									 m_model.instrumentPeakL( n ),
									 m_model.instrumentPeakR( n ),
									 palette ) ) {
				m_model.setStripVolume( n, fVolume );
			}
			float fPan = m_model.stripPan( n );
			centerInStrip( s_nKnobSize );
			if ( drawPanKnob( "##pan", fPan, palette ) ) {
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
			pushState( bMuted, palette.mute, palette.muteText );
			centerInStrip( nButtonWidth * 2.0f + s_nMuteSoloGap );
			if ( ImGui::Button( sMute.constData(),
								ImVec2( nButtonWidth, 0 ) ) ) {
				m_model.setStripIsMuted( n, ! bMuted );
			}
			ImGui::PopStyleColor( nStateColors );
			if ( n == 0 ) {
				m_layout.muteMin = ImGui::GetItemRectMin();
				m_layout.muteMax = ImGui::GetItemRectMax();
			}
			ImGui::SameLine( 0.0f, s_nMuteSoloGap );
			const bool bSoloed = m_model.stripIsSoloed( n );
			pushState( bSoloed, palette.solo, palette.soloText );
			if ( ImGui::Button( sSolo.constData(),
								ImVec2( nButtonWidth, 0 ) ) ) {
				m_model.setStripIsSoloed( n, ! bSoloed );
			}
			ImGui::PopStyleColor( nStateColors );
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
			drawPanel( pLaneDraw, laneSplitter );
			ImGui::PopID();
			if ( n + 1 < nStrips ) {
				// Clear of both neighbors' panel padding.
				ImGui::SameLine( 0.0f, ImGui::GetStyle().ItemSpacing.x +
									   s_nStripPanelPadding * 2.0f );
			}
		}
		laneSplitter.Merge( pLaneDraw );
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

} // namespace H2Core
