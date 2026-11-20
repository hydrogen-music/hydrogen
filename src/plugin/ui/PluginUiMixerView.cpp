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
// The randomization wells reach this far beyond their content — inset
// within the strip panel's padding, so a rim of the panel stays
// visible around the well.
constexpr float s_nWellPadding = 2.0f;
// Extra lead before a randomization well: the gap sets the well apart
// from the mute controls above it.
constexpr float s_nRandomSectionGap = 4.0f;

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
// Rotary Type::Normal's value arc (Rotary.cpp): it starts at the
// lower left (210°) and sweeps clockwise around the top, 239° at the
// maximum value.
constexpr float s_fRotaryNormalArcStart = 210.0f;
constexpr float s_fRotaryNormalArcSweep = 239.0f;
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

bool PluginUiMixerView::drawRotary( const char* sId, float& fValue,
									float fMin, float fMax, bool bCenter,
									float fSize,
									const PluginUiPalette& palette )
{
	// imgui-knobs keeps the interaction (drag, wheel, item id); its own
	// face is pushed fully transparent — its secondary colors scale RGB
	// only, so zero alpha hides every part — and Rotary's face is drawn
	// over the item rect instead. No title (the L/R scale or the label
	// below replaces it) and no internal drag input — the knob's
	// DragScalar renders an empty frame below itself.
	const ImVec4 transparent( 0.0f, 0.0f, 0.0f, 0.0f );
	ImGui::PushStyleColor( ImGuiCol_Button, transparent );
	ImGui::PushStyleColor( ImGuiCol_ButtonHovered, transparent );
	ImGui::PushStyleColor( ImGuiCol_ButtonActive, transparent );
	const bool bMoved = ImGuiKnobs::Knob(
		sId, &fValue, fMin, fMax, 0, "%.2f", ImGuiKnobVariant_Tick,
		fSize, ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput );
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
	// Both Rotary types place the handle by the same normalized offset
	// from the range's center (Qt's Center and Normal formulas both
	// reduce to norm - 0.5).
	const float fNorm = std::clamp( ( fValue - fMin ) / ( fMax - fMin ),
									0.0f, 1.0f );
	const float fHandleOffset = fNorm - 0.5f;
	if ( bCenter ) {
		if ( fHandleOffset == 0.0f ) {
			arc( s_fRotaryCenterMarkStart, s_fRotaryCenterMarkSpan,
				 palette.knobCenterSet, fArcWidth * 1.25f );
		}
		else {
			arc( s_fRotaryCenterMarkStart, s_fRotaryCenterMarkSpan,
				 palette.knobCenterUnset, fArcWidth * 1.25f );
			// Right of center the arc starts 18° clockwise off the top
			// and sweeps clockwise; left of center 16° counter-clockwise
			// off it (Rotary's extra 2° to clear the marker) and sweeps
			// that way.
			const float fStart = fHandleOffset > 0.0f
				? 90.0f - s_fRotaryArcGap
				: 90.0f + s_fRotaryArcGap - 2.0f;
			arc( fStart, -s_fRotaryArcSweep * fHandleOffset, palette.knobArc,
				 fArcWidth );
		}
	}
	else if ( fNorm > 0.0f ) {
		// Type::Normal: the value arc starts at the lower left (210°)
		// and sweeps clockwise around the top, 239° at the maximum —
		// nothing at zero (Qt's zero-span drawArc paints nothing).
		arc( s_fRotaryNormalArcStart, -s_fRotaryNormalArcSweep * fNorm,
			 palette.knobArc, fArcWidth );
	}

	// The handle dot: 255° of travel over the range, straight up at the
	// range's center (screen angles here — y points down).
	const float fHandleAngle =
		( -90.0f + s_fRotaryHandleSweep * fHandleOffset ) *
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

	// The master section's strings, hoisted: the column width below
	// and the section itself (drawn after the strip lane) share them.
	const auto sMasterTitle = m_pCommonStrings->getMasterLabel().toUtf8();
	const auto sHumanize = m_pCommonStrings->getHumanizeLabel().toUtf8();
	const auto sVelocity = m_pCommonStrings->getNotePropertyVelocity()
							   .toUtf8();
	const auto sTiming = m_pCommonStrings->getTimingLabel().toUtf8();
	const auto sSwing = m_pCommonStrings->getSwingLabel().toUtf8();
	// The smaller companion font for the scale labels; null when the
	// atlas lacks it (the labels then render in the base font).
	ImFont* pSmallFont = PluginUiStyle::smallFont();
	// The header row's height: the master's "Humanize" line. The
	// strips reserve the same room above their pitch knobs, so the
	// wells — and their seams — align across the band.
	const float nHeaderHeight =
		ImGui::CalcTextSize( sHumanize.constData() ).y;
	// The knob labels render in the smaller font, so their width
	// decides the column at that size — measured so translations
	// cannot clip — plus the well's inset on both sides.
	if ( pSmallFont != nullptr ) {
		ImGui::PushFont( pSmallFont );
	}
	const float nKnobLabelWidth =
		std::max( { ImGui::CalcTextSize( sVelocity.constData() ).x,
					ImGui::CalcTextSize( sTiming.constData() ).x,
					ImGui::CalcTextSize( sSwing.constData() ).x } );
	if ( pSmallFont != nullptr ) {
		ImGui::PopFont();
	}
	const float nMasterWidth =
		std::max( { ImGui::CalcTextSize( sMasterTitle.constData() ).x,
					ImGui::CalcTextSize( sHumanize.constData() ).x,
					nKnobLabelWidth, s_nKnobSize } ) +
		2.0f * s_nWellPadding;
	// The gap between the master column and the strip lane: clear of
	// both panels' padding.
	const float nColumnGap = ImGui::GetStyle().ItemSpacing.x +
		s_nStripPanelPadding * 2.0f;

	// Centers the next item in its strip column: at a line start the
	// cursor sits at the column's left edge (a group indents its
	// following lines to its start).
	const auto centerInStrip = []( float nColumnWidth,
								   float nItemWidth ) {
		ImGui::SetCursorPosX( ImGui::GetCursorPosX() +
							  ( nColumnWidth - nItemWidth ) * 0.5f );
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
	// The randomization wells' fill and the 1px seam capping them.
	// The palette's separator color equals the well's fill (both the
	// theme's dark color), so the seam takes the meter well's black —
	// the palette's deepest recess tone.
	const ImU32 nWellColor = ImGui::GetColorU32(
		PluginUiColor::toImVec4( palette.stripLane ) );
	const ImU32 nSeamColor = ImGui::GetColorU32(
		PluginUiColor::toImVec4( palette.meterWell ) );
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
	// layout) — it never scrolls away while the strips pan. Its
	// randomization section is drawn after the lane (below), once the
	// strips' knob positions are known; the splitter stays open until
	// then.
	ImDrawList* pMasterDraw = ImGui::GetWindowDrawList();
	ImDrawListSplitter masterSplitter;
	masterSplitter.Split( pMasterDraw, 2 );
	masterSplitter.SetCurrentChannel( pMasterDraw, 1 );
	const float nMasterColumnLeft = ImGui::GetCursorScreenPos().x;
	ImGui::BeginGroup();
	ImGui::TextUnformatted( sMasterTitle.constData() );
	float fMasterVolume = m_model.masterVolume();
	centerInStrip( nMasterWidth, nFaderBlockWidth );
	if ( drawFaderWithMeter( "##masterVolume", fMasterVolume,
							 m_model.masterPeakL(),
							 m_model.masterPeakR(), palette ) ) {
		m_model.setMasterVolume( fMasterVolume );
	}
	const bool bMasterMuted = m_model.masterIsMuted();
	pushState( bMasterMuted, palette.mute, palette.muteText );
	// The full column width: the master mute shares the column's
	// horizontal boundaries.
	if ( ImGui::Button( sMute.constData(), ImVec2( nMasterWidth, 0 ) ) ) {
		m_model.setMasterIsMuted( ! bMasterMuted );
	}
	ImGui::PopStyleColor( nStateColors );
	m_layout.masterMuteMin = ImGui::GetItemRectMin();
	m_layout.masterMuteMax = ImGui::GetItemRectMax();
	ImGui::EndGroup();
	const ImVec2 masterGroupMin = ImGui::GetItemRectMin();
	// The panel, well and seam follow below, once the randomization
	// section — drawn after the strip lane, when the strips' knob
	// positions are known — has extended the column.
	// Clear of the master panel's padding on both sides of the gap.
	ImGui::SameLine( 0.0f, nColumnGap );

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
			centerInStrip( s_nStripWidth, nFaderBlockWidth );
			if ( drawFaderWithMeter( "##volume", fVolume,
									 m_model.instrumentPeakL( n ),
									 m_model.instrumentPeakR( n ),
									 palette ) ) {
				m_model.setStripVolume( n, fVolume );
			}
		float fPan = m_model.stripPan( n );
		centerInStrip( s_nStripWidth, s_nKnobSize );
		if ( drawRotary( "##pan", fPan, -1.0f, 1.0f, true, s_nKnobSize,
						 palette ) ) {
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
			// The scale labels render in the smaller font; the draw
			// list's AddText picks up the pushed font too.
			if ( pSmallFont != nullptr ) {
				ImGui::PushFont( pSmallFont );
			}
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
			if ( pSmallFont != nullptr ) {
				ImGui::PopFont();
			}
		}
			// The mute/solo pair on one line: equal-width buttons
			// centered as a pair around a one-pixel seam on the
			// strip's center axis.
			const bool bMuted = m_model.stripIsMuted( n );
			pushState( bMuted, palette.mute, palette.muteText );
			centerInStrip( s_nStripWidth,
						   nButtonWidth * 2.0f + s_nMuteSoloGap );
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

			// The strip's randomization section: the instrument's
			// random pitch (the InstrumentEditor's rotary) in the same
			// recessed well treatment as the master's humanize triple —
			// the shared look groups all randomization controls
			// together.
			ImGui::Dummy( ImVec2( 0.0f, s_nRandomSectionGap ) );
			// The column's left edge in screen space — the well rect
			// must live in the same space as the item rects it wraps
			// (GetCursorPosX() would be window-relative and misplace
			// the well inside the lane's child window).
			const float nColumnLeft = ImGui::GetCursorScreenPos().x;
			// The header row the master's "Humanize" header occupies:
			// reserving it above the pitch knob too aligns the wells'
			// tops — and their seams — across the band, with all knobs
			// on one line below.
			ImGui::Dummy( ImVec2( 0.0f, nHeaderHeight ) );
			ImVec2 randomMin{}, randomMax{};
			{
				// The spacer above is the well's content top.
				randomMin = ImGui::GetItemRectMin();
				float fRandomPitch = m_model.stripRandomPitch( n );
				centerInStrip( s_nStripWidth, s_nKnobSize );
				if ( drawRotary( "##randomPitch", fRandomPitch, 0.0f, 1.0f,
								 false, s_nKnobSize, palette ) ) {
					m_model.setStripRandomPitch( n, fRandomPitch );
				}
				if ( n == 0 ) {
					m_layout.randomKnobMin = ImGui::GetItemRectMin();
					m_layout.randomKnobMax = ImGui::GetItemRectMax();
					m_layout.fRandomPitchShown = fRandomPitch;
				}
				if ( ImGui::IsItemHovered() ) {
					const auto sTip =
						m_pCommonStrings->getActionSetRandomPitch()
							.toUtf8();
					ImGui::SetTooltip( "%s", sTip.constData() );
				}
				if ( pSmallFont != nullptr ) {
					ImGui::PushFont( pSmallFont );
				}
				const auto sPitch = m_pCommonStrings->getPitchLabel()
									 .toUtf8();
				centerInStrip(
					s_nStripWidth,
					ImGui::CalcTextSize( sPitch.constData() ).x );
				ImGui::TextUnformatted( sPitch.constData() );
				randomMax = ImGui::GetItemRectMax();
				if ( n == 0 ) {
					m_layout.pitchLabelMin = ImGui::GetItemRectMin();
					m_layout.pitchLabelMax = randomMax;
				}
				if ( pSmallFont != nullptr ) {
					ImGui::PopFont();
				}
			}

			ImGui::Dummy( ImVec2( s_nStripWidth, 0.0f ) );
			if ( n == 0 ) {
				m_layout.stripMin = ImGui::GetItemRectMin();
				m_layout.stripMax = ImGui::GetItemRectMax();
				m_layout.bValid = true;
			}
			ImGui::EndGroup();
			// The well's bottom ties to the group's rect, which is
			// queryable only while the group is the last item.
			const ImVec2 nGroupMax = ImGui::GetItemRectMax();
			drawPanel( pLaneDraw, laneSplitter );
			// The randomization well above the panel, capped by the
			// 1px seam: channel 0 draws in submission order, so the
			// panel (drawn first) stays below both. The well fills
			// the panel's lower section down to its bottom border,
			// framed by the same 1px panel rim as on its sides.
			const ImVec2 nWellMin( nColumnLeft - s_nWellPadding,
								   randomMin.y - s_nWellPadding );
			const ImVec2 nWellMax(
				nColumnLeft + s_nStripWidth + s_nWellPadding,
				nGroupMax.y + s_nWellPadding );
			laneSplitter.SetCurrentChannel( pLaneDraw, 0 );
			pLaneDraw->AddRectFilled( nWellMin, nWellMax, nWellColor,
									  ImGui::GetStyle().FrameRounding );
			pLaneDraw->AddRectFilled(
				nWellMin, ImVec2( nWellMax.x, nWellMin.y + 1.0f ),
				nSeamColor );
			laneSplitter.SetCurrentChannel( pLaneDraw, 1 );
			if ( n == 0 ) {
				m_layout.randomMin = nWellMin;
				m_layout.randomMax = nWellMax;
				m_layout.seamMin = nWellMin;
				m_layout.seamMax =
					ImVec2( nWellMax.x, nWellMin.y + 1.0f );
			}
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

	// The master's randomization section: the song-global humanize
	// triple (velocity, timing, swing) — the Qt mixer's MasterLine
	// rotaries — in a recessed well below the mute button. It is
	// placed after the strip lane, absolutely at the master's column,
	// so its knobs can sit at the strips' random-pitch height: the
	// "Humanize" header then reads as the heading of the whole
	// randomization band.
	{
		// The absolute placement below must not move the window's
		// flow cursor for whatever follows the mixer.
		const ImVec2 nAfterLane = ImGui::GetCursorPos();
		// The header line and its spacing sit between the section's
		// top and the first knob: land the knob at the strips'
		// random-pitch height. Without strips (an empty kit) the
			// section simply follows the mute button.
			const float nSectionTop =
			m_layout.randomKnobMin.y > 0.0f
				? m_layout.randomKnobMin.y -
					  ImGui::GetStyle().ItemSpacing.y - nHeaderHeight
				: m_layout.masterMuteMax.y +
					  ImGui::GetStyle().ItemSpacing.y +
					  s_nRandomSectionGap;
		ImGui::SetCursorScreenPos(
			ImVec2( nMasterColumnLeft, nSectionTop ) );
		ImGui::BeginGroup();
		ImVec2 masterRandomMin{}, masterRandomMax{};
		{
			centerInStrip( nMasterWidth,
						   ImGui::CalcTextSize( sHumanize.constData() )
							   .x );
			ImGui::TextUnformatted( sHumanize.constData() );
			masterRandomMin = ImGui::GetItemRectMin();
			m_layout.masterHeaderMin = masterRandomMin;
			m_layout.masterHeaderMax = ImGui::GetItemRectMax();

			float fHumanizeVelocity = m_model.humanizeVelocity();
			centerInStrip( nMasterWidth, s_nKnobSize );
			if ( drawRotary( "##humanizeVelocity", fHumanizeVelocity,
							 0.0f, 1.0f, false, s_nKnobSize,
							 palette ) ) {
				m_model.setHumanizeVelocity( fHumanizeVelocity );
			}
			m_layout.masterRandomKnobMin = ImGui::GetItemRectMin();
			m_layout.masterRandomKnobMax = ImGui::GetItemRectMax();
			m_layout.fHumanizeVelocityShown = fHumanizeVelocity;
			if ( ImGui::IsItemHovered() ) {
				const auto sTip = m_pCommonStrings
									   ->getActionSetHumanizeVelocity()
									   .toUtf8();
				ImGui::SetTooltip( "%s", sTip.constData() );
			}
			if ( pSmallFont != nullptr ) {
				ImGui::PushFont( pSmallFont );
			}
			centerInStrip( nMasterWidth,
						   ImGui::CalcTextSize( sVelocity.constData() )
							   .x );
			ImGui::TextUnformatted( sVelocity.constData() );
			m_layout.masterVelocityLabelMin = ImGui::GetItemRectMin();
			m_layout.masterVelocityLabelMax = ImGui::GetItemRectMax();
			if ( pSmallFont != nullptr ) {
				ImGui::PopFont();
			}

			float fHumanizeTime = m_model.humanizeTime();
			centerInStrip( nMasterWidth, s_nKnobSize );
			if ( drawRotary( "##humanizeTime", fHumanizeTime, 0.0f, 1.0f,
							 false, s_nKnobSize, palette ) ) {
				m_model.setHumanizeTime( fHumanizeTime );
			}
			if ( ImGui::IsItemHovered() ) {
				const auto sTip = m_pCommonStrings
									   ->getActionSetHumanizeTime()
									   .toUtf8();
				ImGui::SetTooltip( "%s", sTip.constData() );
			}
			if ( pSmallFont != nullptr ) {
				ImGui::PushFont( pSmallFont );
			}
			centerInStrip( nMasterWidth,
						   ImGui::CalcTextSize( sTiming.constData() ).x );
			ImGui::TextUnformatted( sTiming.constData() );
			if ( pSmallFont != nullptr ) {
				ImGui::PopFont();
			}

			float fSwing = m_model.swingFactor();
			centerInStrip( nMasterWidth, s_nKnobSize );
			if ( drawRotary( "##swing", fSwing, 0.0f, 1.0f, false,
							 s_nKnobSize, palette ) ) {
				m_model.setSwing( fSwing );
			}
			if ( ImGui::IsItemHovered() ) {
				const auto sTip = m_pCommonStrings->getActionSetSwing()
									   .toUtf8();
				ImGui::SetTooltip( "%s", sTip.constData() );
			}
			if ( pSmallFont != nullptr ) {
				ImGui::PushFont( pSmallFont );
			}
			centerInStrip( nMasterWidth,
						   ImGui::CalcTextSize( sSwing.constData() ).x );
			ImGui::TextUnformatted( sSwing.constData() );
			masterRandomMax = ImGui::GetItemRectMax();
			if ( pSmallFont != nullptr ) {
				ImGui::PopFont();
			}
		}
		ImGui::EndGroup();

		// One panel behind the whole master column: the group above
		// and the absolutely placed section share it, so the column
		// reads as one strip. Channel 0 renders below the widgets
		// whatever the submission order.
		masterSplitter.SetCurrentChannel( pMasterDraw, 0 );
		pMasterDraw->AddRectFilled(
			ImVec2( masterGroupMin.x - s_nStripPanelPadding,
					masterGroupMin.y - s_nStripPanelPadding ),
			ImVec2( nMasterColumnLeft + nMasterWidth +
						s_nStripPanelPadding,
					masterRandomMax.y + s_nStripPanelPadding ),
			nPanelColor, ImGui::GetStyle().FrameRounding );
		// The randomization well and its 1px seam, in the same
		// treatment as every strip's.
		const ImVec2 nMasterWellMin(
			nMasterColumnLeft - s_nWellPadding,
			masterRandomMin.y - s_nWellPadding );
		const ImVec2 nMasterWellMax(
			nMasterColumnLeft + nMasterWidth + s_nWellPadding,
			masterRandomMax.y + s_nWellPadding );
		pMasterDraw->AddRectFilled( nMasterWellMin, nMasterWellMax,
									nWellColor,
									ImGui::GetStyle().FrameRounding );
		pMasterDraw->AddRectFilled(
			nMasterWellMin,
			ImVec2( nMasterWellMax.x, nMasterWellMin.y + 1.0f ),
			nSeamColor );
		masterSplitter.SetCurrentChannel( pMasterDraw, 1 );
		m_layout.masterStripMin = masterGroupMin;
		m_layout.masterStripMax =
			ImVec2( nMasterColumnLeft + nMasterWidth,
					masterRandomMax.y );
		m_layout.masterRandomMin = nMasterWellMin;
		m_layout.masterRandomMax = nMasterWellMax;
		m_layout.masterSeamMin = nMasterWellMin;
		m_layout.masterSeamMax =
			ImVec2( nMasterWellMax.x, nMasterWellMin.y + 1.0f );
		masterSplitter.Merge( pMasterDraw );
		ImGui::SetCursorPos( nAfterLane );
	}
}

} // namespace H2Core
