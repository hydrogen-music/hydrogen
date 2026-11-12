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

#include "PluginUiStyle.h"

#include <core/Preferences/Theme.h>

#include <algorithm>
#include <cstdint>

namespace {

using H2Core::Color;

// QColor keeps its components in 16 bits (0..USHRT_MAX) and only rounds to
// 8 bits on the way out. lighter()/darker() scale the 16-bit HSV value, so
// a port working on 8-bit HSV drifts by one step on many inputs — the
// whole round trip is mirrored instead (qcolor.cpp, Qt 6).
constexpr int s_nMax16 = 65535;
// QColor's marker for an achromatic hue.
constexpr int s_nHueAchromatic = 65535;

struct Hsv16
{
	int nHue;        // centi-degrees 0..35999, or s_nHueAchromatic
	int nSaturation; // 0..65535
	int nValue;      // 0..65535
};

// qRound(): half away from zero.
int roundHalfAway( float f ) {
	return f >= 0.0f ? static_cast<int>( f + 0.5f )
					 : static_cast<int>( f - 0.5f );
}

// qt_div_257(): the 16-to-8 bit reduction QColor::red() & co. apply.
int div257( int n ) {
	return ( n - ( ( n + 0x80 ) >> 8 ) + 0x80 ) >> 8;
}

// Integer division rounding half up — @a nDenominator > 0, @a nNumerator
// >= 0.
std::int64_t divRoundHalfUp( std::int64_t nNumerator,
							 std::int64_t nDenominator ) {
	return ( 2 * nNumerator + nDenominator ) / ( 2 * nDenominator );
}

// Exact rational arithmetic instead of QColor's float expressions: the
// 8-bit inputs scale by 257 into 16 bits, so saturation and hue land on
// exact .5 ties for many colors — which Qt rounds up, but a float port
// rounds either way depending on the last bit. Fuzzed against Qt 6 over
// 3M random lighter()/darker()/inactiveText() calls: 14 results differ by
// one step in a single channel, where float noise in Qt's own conversion
// tips a near-tie (no float formulation tried did better).
Hsv16 toHsv16( const Color& color ) {
	const int r = color.red();
	const int g = color.green();
	const int b = color.blue();
	const int nMax = std::max( { r, g, b } );
	const int nMin = std::min( { r, g, b } );
	const int nDelta = nMax - nMin;

	Hsv16 hsv{};
	hsv.nValue = nMax * 0x101;
	if ( nDelta == 0 ) {
		hsv.nHue = s_nHueAchromatic;
		hsv.nSaturation = 0;
		return hsv;
	}
	hsv.nSaturation = static_cast<int>(
		divRoundHalfUp( static_cast<std::int64_t>( nDelta ) * s_nMax16, nMax ) );

	// Hue in centi-degrees: 6000 per 60° sector, Qt's sector order
	// (red, green, blue maximum), wrapped into [0, 36000) before
	// rounding, as Qt adds 360° before its qRound().
	std::int64_t nSectorBase = 0;
	std::int64_t nSpan = 0;
	if ( r == nMax ) {
		nSectorBase = 0;
		nSpan = g - b;
	}
	else if ( g == nMax ) {
		nSectorBase = 12000;
		nSpan = b - r;
	}
	else {
		nSectorBase = 24000;
		nSpan = r - g;
	}
	std::int64_t nNumerator = nSectorBase * nDelta + 6000 * nSpan;
	if ( nNumerator < 0 ) {
		nNumerator += static_cast<std::int64_t>( 36000 ) * nDelta;
	}
	hsv.nHue = static_cast<int>( divRoundHalfUp( nNumerator, nDelta ) );
	return hsv;
}

Color fromHsv16( const Hsv16& hsv ) {
	if ( hsv.nSaturation == 0 || hsv.nHue == s_nHueAchromatic ) {
		const int n = div257( hsv.nValue );
		return Color( n, n, n );
	}
	const float h = hsv.nHue == 36000 ? 0.0f : hsv.nHue / 6000.0f;
	const float s = static_cast<float>( hsv.nSaturation ) / s_nMax16;
	const float v = static_cast<float>( hsv.nValue ) / s_nMax16;
	const int i = static_cast<int>( h );
	const float f = h - static_cast<float>( i );
	const float p = v * ( 1.0f - s );

	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	if ( ( i & 1 ) != 0 ) {
		const float q = v * ( 1.0f - ( s * f ) );
		switch ( i ) {
		case 1:
			r = q; g = v; b = p;
			break;
		case 3:
			r = p; g = q; b = v;
			break;
		case 5:
			r = v; g = p; b = q;
			break;
		default:
			break;
		}
	}
	else {
		const float t = v * ( 1.0f - ( s * ( 1.0f - f ) ) );
		switch ( i ) {
		case 0:
			r = v; g = t; b = p;
			break;
		case 2:
			r = p; g = v; b = t;
			break;
		case 4:
			r = t; g = p; b = v;
			break;
		default:
			break;
		}
	}
	return Color( div257( roundHalfAway( r * s_nMax16 ) ),
				  div257( roundHalfAway( g * s_nMax16 ) ),
				  div257( roundHalfAway( b * s_nMax16 ) ) );
}

// The fixed colors the GUI paints with outside the theme: Skin's
// QPushButton border (#000) and Rotary's arc/handle (Qt::red, Qt::green,
// Qt::gray, Qt::black). Mirrored, not themed, so both UIs stay identical.
const Color s_black( 0, 0, 0 );
const Color s_qtRed( 255, 0, 0 );
const Color s_qtGreen( 0, 255, 0 );
const Color s_qtGray( 160, 160, 164 );

// Skin's QPushButton / QToolButton scalings (src/gui/src/Skin.h).
constexpr int s_nHoveredScaling = 105;
constexpr int s_nCheckedScaling = 125;
// Skin's spin box selection: the spin box color darker(120).
constexpr int s_nSpinBoxSelectionScaling = 120;
// MixerLine's mute/solo button face: the widget color darker(130).
constexpr int s_nStripButtonScaling = 130;
// Shades measured off the macOS-styled Qt GUI, where the platform style
// derives them rather than the theme: the dialogs' tab pane (midlight
// darker(114)), the unselected tabs (window darker(141)), the mixer strip
// panel (window lighter(122)). Matching in Qt's own math keeps them
// following a customized theme.
constexpr int s_nPageScaling = 114;
constexpr int s_nTabScaling = 141;
constexpr int s_nTabHoveredScaling = 125;
constexpr int s_nStripPanelScaling = 122;
// The rim of the knob face — Rotary's SVG shades its edge; this keeps the
// flat imgui disk from blurring into a light strip panel.
constexpr int s_nKnobRimScaling = 150;

} // namespace

namespace H2Core {

namespace PluginUiColor {

Color lighter( const Color& color, int nFactor ) {
	if ( ! color.isValid() || nFactor <= 0 ) {
		return color;
	}
	if ( nFactor < 100 ) {
		return darker( color, 10000 / nFactor );
	}
	Hsv16 hsv = toHsv16( color );
	// Unsigned 32-bit arithmetic, as QColor's: factor * value fits.
	std::uint32_t nValue = static_cast<std::uint32_t>( nFactor ) *
		static_cast<std::uint32_t>( hsv.nValue ) / 100u;
	if ( nValue > static_cast<std::uint32_t>( s_nMax16 ) ) {
		hsv.nSaturation -= static_cast<int>( nValue - s_nMax16 );
		if ( hsv.nSaturation < 0 ) {
			hsv.nSaturation = 0;
		}
		nValue = s_nMax16;
	}
	hsv.nValue = static_cast<int>( nValue );
	return fromHsv16( hsv );
}

Color darker( const Color& color, int nFactor ) {
	if ( ! color.isValid() || nFactor <= 0 ) {
		return color;
	}
	if ( nFactor < 100 ) {
		return lighter( color, 10000 / nFactor );
	}
	Hsv16 hsv = toHsv16( color );
	hsv.nValue = static_cast<int>(
		static_cast<std::uint32_t>( hsv.nValue ) * 100u /
		static_cast<std::uint32_t>( nFactor ) );
	return fromHsv16( hsv );
}

Color inactiveText( const Color& color ) {
	if ( ! color.isValid() ) {
		return color;
	}
	// Skin::makeTextColorInactive() works on QColor::getHsv()'s 8-bit
	// components and writes back through setHsv() — both reductions
	// mirrored.
	const Hsv16 hsv = toHsv16( color );
	int nValue = div257( hsv.nValue );
	nValue = nValue >= 130 ? nValue - 55 : nValue + 55;
	Hsv16 result{};
	result.nHue = hsv.nHue == s_nHueAchromatic
		? s_nHueAchromatic
		: ( hsv.nHue / 100 ) * 100;
	result.nSaturation = div257( hsv.nSaturation ) * 0x101;
	result.nValue = nValue * 0x101;
	return fromHsv16( result );
}

ImVec4 toImVec4( const Color& color, float fAlpha ) {
	return ImVec4( color.red() / 255.0f, color.green() / 255.0f,
				   color.blue() / 255.0f, fAlpha );
}

} // namespace PluginUiColor

PluginUiPalette PluginUiPalette::fromTheme( const ColorTheme& theme ) {
	using PluginUiColor::darker;
	using PluginUiColor::lighter;

	PluginUiPalette palette;

	palette.window = theme.m_windowColor;
	palette.page = darker( theme.m_midLightColor, s_nPageScaling );
	palette.tab = darker( theme.m_windowColor, s_nTabScaling );
	palette.tabHovered = lighter( theme.m_windowColor, s_nTabHoveredScaling );
	palette.text = theme.m_textColor;
	palette.textDisabled = PluginUiColor::inactiveText( theme.m_textColor );
	palette.separator = theme.m_darkColor;

	palette.frame = theme.m_spinBoxColor;
	palette.frameHovered = lighter( theme.m_spinBoxColor, s_nHoveredScaling );
	palette.frameActive =
		darker( theme.m_spinBoxColor, s_nSpinBoxSelectionScaling );

	palette.button = theme.m_widgetColor;
	palette.buttonHovered = lighter( theme.m_widgetColor, s_nHoveredScaling );
	palette.buttonActive = lighter( theme.m_accentColor, s_nCheckedScaling );
	palette.buttonText = theme.m_widgetTextColor;
	palette.buttonBorder = s_black;

	palette.base = theme.m_baseColor;
	palette.tableBorder = palette.page;
	palette.checkBox = theme.m_baseColor;
	palette.checkBoxHovered = lighter( theme.m_baseColor, s_nHoveredScaling );

	palette.header = theme.m_accentColor;
	palette.headerHovered = lighter( theme.m_accentColor, s_nHoveredScaling );
	palette.headerActive = lighter( theme.m_accentColor, s_nCheckedScaling );
	palette.highlight = theme.m_highlightColor;
	palette.error = theme.m_buttonRedColor;

	palette.stripLane = theme.m_darkColor;
	palette.stripPanel = lighter( theme.m_windowColor, s_nStripPanelScaling );
	palette.stripButton = darker( theme.m_widgetColor, s_nStripButtonScaling );
	palette.stripButtonHovered =
		lighter( palette.stripButton, s_nHoveredScaling );
	palette.meterWell = s_black;
	// The handle shares the unexpanded combo face (the frame color) so
	// both read as the same dark interactive well.
	palette.faderHandle = theme.m_spinBoxColor;
	palette.faderHandleActive =
		darker( theme.m_spinBoxColor, s_nSpinBoxSelectionScaling );
	palette.knobBody = theme.m_widgetColor;
	palette.knobRim = darker( theme.m_widgetColor, s_nKnobRimScaling );
	palette.knobHandle = s_black;
	palette.knobArc = s_qtRed;
	palette.knobCenterSet = s_qtGreen;
	palette.knobCenterUnset = s_qtGray;
	palette.mute = theme.m_muteColor;
	palette.muteText = theme.m_muteTextColor;
	palette.solo = theme.m_soloColor;
	palette.soloText = theme.m_soloTextColor;

	return palette;
}

namespace PluginUiStyle {

void applyMetrics( ImGuiStyle& style ) {
	// Skin's QPushButton radius; frames are flat like the GUI's spin
	// boxes — only push buttons carry a border (ScopedPushButton).
	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 2.0f;
	style.PopupRounding = 2.0f;
	style.GrabRounding = 2.0f;
	style.TabRounding = 2.0f;
	style.ScrollbarRounding = 2.0f;
	style.WindowBorderSize = 0.0f;
	style.ChildBorderSize = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	// The selected tab merges into the page below it: no outline, no
	// rule under the bar, no overline on top.
	style.TabBorderSize = 0.0f;
	style.TabBarBorderSize = 0.0f;
	style.TabBarOverlineSize = 0.0f;
}

void applyColors( const PluginUiPalette& palette, ImGuiStyle& style ) {
	using PluginUiColor::toImVec4;
	ImVec4* colors = style.Colors;

	colors[ ImGuiCol_Text ] = toImVec4( palette.text );
	colors[ ImGuiCol_TextDisabled ] = toImVec4( palette.textDisabled );
	colors[ ImGuiCol_WindowBg ] = toImVec4( palette.window );
	// Tab content is drawn into child regions on the page.
	colors[ ImGuiCol_ChildBg ] = toImVec4( palette.page );
	colors[ ImGuiCol_PopupBg ] = toImVec4( palette.base );
	colors[ ImGuiCol_Border ] = toImVec4( palette.separator );
	colors[ ImGuiCol_BorderShadow ] = ImVec4( 0.0f, 0.0f, 0.0f, 0.0f );

	colors[ ImGuiCol_FrameBg ] = toImVec4( palette.frame );
	colors[ ImGuiCol_FrameBgHovered ] = toImVec4( palette.frameHovered );
	colors[ ImGuiCol_FrameBgActive ] = toImVec4( palette.frameActive );
	colors[ ImGuiCol_TextSelectedBg ] = toImVec4( palette.frameActive );
	colors[ ImGuiCol_InputTextCursor ] = toImVec4( palette.text );

	colors[ ImGuiCol_TitleBg ] = toImVec4( palette.header );
	colors[ ImGuiCol_TitleBgActive ] = toImVec4( palette.header );
	colors[ ImGuiCol_TitleBgCollapsed ] = toImVec4( palette.header );
	colors[ ImGuiCol_MenuBarBg ] = toImVec4( palette.window );

	colors[ ImGuiCol_ScrollbarBg ] = toImVec4( palette.window );
	colors[ ImGuiCol_ScrollbarGrab ] = toImVec4( palette.stripButton );
	colors[ ImGuiCol_ScrollbarGrabHovered ] = toImVec4( palette.button );
	colors[ ImGuiCol_ScrollbarGrabActive ] =
		toImVec4( palette.buttonHovered );

	colors[ ImGuiCol_CheckMark ] = toImVec4( palette.text );
	colors[ ImGuiCol_SliderGrab ] = toImVec4( palette.faderHandle );
	colors[ ImGuiCol_SliderGrabActive ] =
		toImVec4( palette.faderHandleActive );

	colors[ ImGuiCol_Button ] = toImVec4( palette.button );
	colors[ ImGuiCol_ButtonHovered ] = toImVec4( palette.buttonHovered );
	colors[ ImGuiCol_ButtonActive ] = toImVec4( palette.buttonActive );

	colors[ ImGuiCol_Header ] = toImVec4( palette.header );
	colors[ ImGuiCol_HeaderHovered ] = toImVec4( palette.headerHovered );
	colors[ ImGuiCol_HeaderActive ] = toImVec4( palette.headerActive );

	colors[ ImGuiCol_Separator ] = toImVec4( palette.separator );
	colors[ ImGuiCol_SeparatorHovered ] = toImVec4( palette.highlight );
	colors[ ImGuiCol_SeparatorActive ] = toImVec4( palette.highlight );
	colors[ ImGuiCol_ResizeGrip ] = ImVec4( 0.0f, 0.0f, 0.0f, 0.0f );
	colors[ ImGuiCol_ResizeGripHovered ] = toImVec4( palette.highlight );
	colors[ ImGuiCol_ResizeGripActive ] = toImVec4( palette.highlight );

	colors[ ImGuiCol_Tab ] = toImVec4( palette.tab );
	colors[ ImGuiCol_TabHovered ] = toImVec4( palette.tabHovered );
	colors[ ImGuiCol_TabSelected ] = toImVec4( palette.page );
	colors[ ImGuiCol_TabSelectedOverline ] = toImVec4( palette.page );
	// A plugin window loses focus to the host all the time — the tabs
	// must not change color with it.
	colors[ ImGuiCol_TabDimmed ] = toImVec4( palette.tab );
	colors[ ImGuiCol_TabDimmedSelected ] = toImVec4( palette.page );
	colors[ ImGuiCol_TabDimmedSelectedOverline ] = toImVec4( palette.page );

	colors[ ImGuiCol_PlotLines ] = toImVec4( palette.text );
	colors[ ImGuiCol_PlotLinesHovered ] = toImVec4( palette.highlight );
	colors[ ImGuiCol_PlotHistogram ] = toImVec4( palette.highlight );
	colors[ ImGuiCol_PlotHistogramHovered ] = toImVec4( palette.highlight );

	colors[ ImGuiCol_TableHeaderBg ] = toImVec4( palette.base );
	colors[ ImGuiCol_TableBorderStrong ] = toImVec4( palette.tableBorder );
	colors[ ImGuiCol_TableBorderLight ] = toImVec4( palette.tableBorder );
	// The GUI's tables do not alternate their rows.
	colors[ ImGuiCol_TableRowBg ] = toImVec4( palette.base );
	colors[ ImGuiCol_TableRowBgAlt ] = toImVec4( palette.base );

	colors[ ImGuiCol_TextLink ] = toImVec4( palette.highlight );
	colors[ ImGuiCol_DragDropTarget ] = toImVec4( palette.highlight );
	colors[ ImGuiCol_NavCursor ] = toImVec4( palette.highlight );
}

ScopedPushButton::ScopedPushButton( const PluginUiPalette& palette ) {
	ImGui::PushStyleColor( ImGuiCol_Text,
						   PluginUiColor::toImVec4( palette.buttonText ) );
	ImGui::PushStyleColor( ImGuiCol_Border,
						   PluginUiColor::toImVec4( palette.buttonBorder ) );
	ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize, 1.0f );
}

ScopedPushButton::~ScopedPushButton() {
	ImGui::PopStyleVar();
	ImGui::PopStyleColor( 2 );
}

ScopedCheckBox::ScopedCheckBox( const PluginUiPalette& palette ) {
	ImGui::PushStyleColor( ImGuiCol_FrameBg,
						   PluginUiColor::toImVec4( palette.checkBox ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgHovered,
						   PluginUiColor::toImVec4( palette.checkBoxHovered ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgActive,
						   PluginUiColor::toImVec4( palette.checkBox ) );
}

ScopedCheckBox::~ScopedCheckBox() {
	ImGui::PopStyleColor( 3 );
}

} // namespace PluginUiStyle

} // namespace H2Core
