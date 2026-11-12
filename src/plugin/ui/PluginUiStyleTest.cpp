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

// The plugin UI's theme mapping (PluginUiStyle): headless — no window, no
// GPU — so it runs everywhere the window smoke skips. The expected shades
// are QColor's own output (Qt 6 lighter()/darker() on the same inputs), so
// the port is pinned to Qt rather than to itself.

#include "PluginUiStyle.h"

#include <core/Preferences/Theme.h>

#include <imgui.h>

#include <cstdio>
#include <memory>

namespace {

bool g_bOk = true;

void check( bool bCondition, const char* sMessage ) {
	if ( ! bCondition ) {
		g_bOk = false;
		std::fprintf( stderr, "PLUGIN UI STYLE: %s\n", sMessage );
	}
}

void checkColor( const H2Core::Color& actual, int nRed, int nGreen,
				 int nBlue, const char* sWhat ) {
	if ( ! actual.isValid() || actual.red() != nRed ||
		 actual.green() != nGreen || actual.blue() != nBlue ) {
		g_bOk = false;
		std::fprintf( stderr,
					  "PLUGIN UI STYLE: %s: got %d,%d,%d (valid=%d), "
					  "expected %d,%d,%d\n",
					  sWhat, actual.red(), actual.green(), actual.blue(),
					  actual.isValid() ? 1 : 0, nRed, nGreen, nBlue );
	}
}

bool sameImVec4( const ImVec4& a, const ImVec4& b ) {
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

} // namespace

int main() {
	using H2Core::Color;
	namespace PluginUiColor = H2Core::PluginUiColor;

	// QColor parity — the reference values were produced by Qt 6.
	{
		checkColor( PluginUiColor::darker( Color( 128, 134, 152 ), 114 ),
					112, 118, 133, "darker(114)" );
		checkColor( PluginUiColor::darker( Color( 58, 62, 72 ), 141 ),
					41, 44, 51, "darker(141)" );
		checkColor( PluginUiColor::lighter( Color( 58, 62, 72 ), 125 ),
					72, 77, 90, "lighter(125)" );
		checkColor( PluginUiColor::lighter( Color( 58, 62, 72 ), 122 ),
					71, 76, 88, "lighter(122)" );
		checkColor( PluginUiColor::lighter( Color( 51, 74, 100 ), 105 ),
					54, 78, 105, "lighter(105)" );
		checkColor( PluginUiColor::darker( Color( 51, 74, 100 ), 120 ),
					42, 62, 83, "darker(120)" );
		checkColor( PluginUiColor::lighter( Color( 164, 170, 190 ), 105 ),
					172, 178, 199, "widget lighter(105)" );
		checkColor( PluginUiColor::darker( Color( 164, 170, 190 ), 130 ),
					126, 131, 146, "widget darker(130)" );
		checkColor( PluginUiColor::lighter( Color( 67, 96, 131 ), 105 ),
					70, 101, 138, "accent lighter(105)" );
		checkColor( PluginUiColor::lighter( Color( 67, 96, 131 ), 125 ),
					84, 120, 164, "accent lighter(125)" );
		// Saturating: the value overflow bleeds into the saturation.
		checkColor( PluginUiColor::lighter( Color( 250, 10, 10 ), 150 ),
					255, 130, 130, "saturating lighter" );
		checkColor( PluginUiColor::lighter( Color( 255, 0, 128 ), 110 ),
					255, 25, 141, "saturated lighter" );
		// Factors below 100 swap direction (Qt's delegation).
		checkColor( PluginUiColor::lighter( Color( 200, 100, 50 ), 50 ),
					100, 50, 25, "lighter(<100)" );
		checkColor( PluginUiColor::darker( Color( 200, 100, 50 ), 50 ),
					255, 224, 209, "darker(<100)" );
		// Achromatic and black.
		checkColor( PluginUiColor::lighter( Color( 128, 128, 128 ), 130 ),
					166, 166, 166, "gray lighter" );
		checkColor( PluginUiColor::lighter( Color( 0, 0, 0 ), 200 ),
					0, 0, 0, "black lighter" );
		checkColor( PluginUiColor::darker( Color( 10, 200, 90 ), 300 ),
					3, 67, 30, "darker(300)" );
		// Non-positive factors are a no-op.
		checkColor( PluginUiColor::lighter( Color( 10, 20, 30 ), 0 ),
					10, 20, 30, "lighter(0)" );
		checkColor( PluginUiColor::darker( Color( 10, 20, 30 ), -5 ),
					10, 20, 30, "darker(-5)" );
		// Invalid stays invalid.
		check( ! PluginUiColor::lighter( Color(), 150 ).isValid(),
			   "lighter() validated an invalid color" );
		check( ! PluginUiColor::darker( Color(), 150 ).isValid(),
			   "darker() validated an invalid color" );

		// Skin::makeTextColorInactive parity.
		checkColor( PluginUiColor::inactiveText( Color( 255, 255, 255 ) ),
					200, 200, 200, "inactiveText(white)" );
		checkColor( PluginUiColor::inactiveText( Color( 40, 40, 40 ) ),
					95, 95, 95, "inactiveText(dark)" );
	}

	// The default theme resolves to the shades the Qt GUI shows.
	const auto pDefaultTheme = std::make_shared<H2Core::ColorTheme>();
	const auto palette = H2Core::PluginUiPalette::fromTheme( *pDefaultTheme );
	{
		checkColor( palette.window, 58, 62, 72, "window" );
		checkColor( palette.page, 112, 118, 133, "page" );
		checkColor( palette.tab, 41, 44, 51, "tab" );
		checkColor( palette.tabHovered, 72, 77, 90, "tabHovered" );
		checkColor( palette.text, 255, 255, 255, "text" );
		checkColor( palette.textDisabled, 200, 200, 200, "textDisabled" );
		checkColor( palette.separator, 81, 86, 99, "separator" );
		checkColor( palette.frame, 51, 74, 100, "frame" );
		checkColor( palette.frameHovered, 54, 78, 105, "frameHovered" );
		checkColor( palette.frameActive, 42, 62, 83, "frameActive" );
		checkColor( palette.button, 164, 170, 190, "button" );
		checkColor( palette.buttonHovered, 172, 178, 199, "buttonHovered" );
		checkColor( palette.buttonActive, 84, 120, 164, "buttonActive" );
		checkColor( palette.buttonText, 10, 10, 10, "buttonText" );
		checkColor( palette.buttonBorder, 0, 0, 0, "buttonBorder" );
		checkColor( palette.base, 88, 94, 112, "base" );
		checkColor( palette.checkBox, 88, 94, 112, "checkBox" );
		checkColor( palette.checkBoxHovered, 92, 99, 118, "checkBoxHovered" );
		checkColor( palette.tableBorder, 112, 118, 133, "tableBorder" );
		checkColor( palette.header, 67, 96, 131, "header" );
		checkColor( palette.headerHovered, 70, 101, 138, "headerHovered" );
		checkColor( palette.headerActive, 84, 120, 164, "headerActive" );
		checkColor( palette.highlight, 206, 150, 30, "highlight" );
		checkColor( palette.error, 247, 100, 100, "error" );
		checkColor( palette.stripLane, 81, 86, 99, "stripLane" );
		checkColor( palette.stripPanel, 71, 76, 88, "stripPanel" );
		checkColor( palette.stripButton, 126, 131, 146, "stripButton" );
		checkColor( palette.stripButtonHovered, 132, 138, 153,
					"stripButtonHovered" );
		checkColor( palette.meterWell, 0, 0, 0, "meterWell" );
		checkColor( palette.faderHandle, 51, 74, 100, "faderHandle" );
		checkColor( palette.faderHandleActive, 42, 62, 83,
					"faderHandleActive" );
		checkColor( palette.knobBody, 164, 170, 190, "knobBody" );
		checkColor( palette.knobRim, 109, 113, 127, "knobRim" );
		checkColor( palette.knobHandle, 0, 0, 0, "knobHandle" );
		checkColor( palette.knobArc, 255, 0, 0, "knobArc" );
		checkColor( palette.knobCenterSet, 0, 255, 0, "knobCenterSet" );
		checkColor( palette.knobCenterUnset, 160, 160, 164,
					"knobCenterUnset" );
		checkColor( palette.mute, 170, 29, 29, "mute" );
		checkColor( palette.muteText, 255, 255, 255, "muteText" );
		checkColor( palette.solo, 219, 177, 50, "solo" );
		checkColor( palette.soloText, 255, 255, 255, "soloText" );
	}

	// A customized theme flows through — no shade is baked in.
	{
		auto pTheme = std::make_shared<H2Core::ColorTheme>();
		pTheme->m_spinBoxColor = Color( 1, 2, 3 );
		pTheme->m_widgetColor = Color( 200, 100, 50 );
		pTheme->m_windowColor = Color( 128, 128, 128 );
		pTheme->m_muteColor = Color( 9, 8, 7 );
		const auto custom = H2Core::PluginUiPalette::fromTheme( *pTheme );
		checkColor( custom.frame, 1, 2, 3, "custom frame" );
		checkColor( custom.button, 200, 100, 50, "custom button" );
		checkColor( custom.knobBody, 200, 100, 50, "custom knobBody" );
		checkColor( custom.faderHandle, 1, 2, 3, "custom faderHandle" );
		checkColor( custom.faderHandleActive,
					PluginUiColor::darker( Color( 1, 2, 3 ), 120 ).red(),
					PluginUiColor::darker( Color( 1, 2, 3 ), 120 ).green(),
					PluginUiColor::darker( Color( 1, 2, 3 ), 120 ).blue(),
					"custom faderHandleActive" );
		checkColor( custom.stripButton,
					PluginUiColor::darker( Color( 200, 100, 50 ), 130 ).red(),
					PluginUiColor::darker( Color( 200, 100, 50 ), 130 ).green(),
					PluginUiColor::darker( Color( 200, 100, 50 ), 130 ).blue(),
					"custom stripButton" );
		checkColor( custom.window, 128, 128, 128, "custom window" );
		checkColor( custom.tabHovered, 160, 160, 160, "custom tabHovered" );
		checkColor( custom.mute, 9, 8, 7, "custom mute" );
	}

	// The palette lands in imgui's color slots.
	{
		using PluginUiColor::toImVec4;
		ImGuiStyle style;
		H2Core::PluginUiStyle::applyColors( palette, style );
		const struct {
			ImGuiCol nSlot;
			Color color;
			const char* sName;
		} colorSlots[] = {
			{ ImGuiCol_WindowBg, palette.window, "WindowBg" },
			{ ImGuiCol_ChildBg, palette.page, "ChildBg" },
			{ ImGuiCol_PopupBg, palette.base, "PopupBg" },
			{ ImGuiCol_Text, palette.text, "Text" },
			{ ImGuiCol_TextDisabled, palette.textDisabled, "TextDisabled" },
			{ ImGuiCol_FrameBg, palette.frame, "FrameBg" },
			{ ImGuiCol_FrameBgHovered, palette.frameHovered, "FrameBgHovered" },
			{ ImGuiCol_FrameBgActive, palette.frameActive, "FrameBgActive" },
			{ ImGuiCol_TextSelectedBg, palette.frameActive, "TextSelectedBg" },
			{ ImGuiCol_Button, palette.button, "Button" },
			{ ImGuiCol_ButtonHovered, palette.buttonHovered, "ButtonHovered" },
			{ ImGuiCol_ButtonActive, palette.buttonActive, "ButtonActive" },
			{ ImGuiCol_Header, palette.header, "Header" },
			{ ImGuiCol_HeaderHovered, palette.headerHovered, "HeaderHovered" },
			{ ImGuiCol_HeaderActive, palette.headerActive, "HeaderActive" },
			{ ImGuiCol_CheckMark, palette.text, "CheckMark" },
			{ ImGuiCol_SliderGrab, palette.faderHandle, "SliderGrab" },
			{ ImGuiCol_SliderGrabActive, palette.faderHandleActive,
			  "SliderGrabActive" },
			{ ImGuiCol_Separator, palette.separator, "Separator" },
			{ ImGuiCol_Tab, palette.tab, "Tab" },
			{ ImGuiCol_TabDimmed, palette.tab, "TabDimmed" },
			{ ImGuiCol_TabHovered, palette.tabHovered, "TabHovered" },
			{ ImGuiCol_TabSelected, palette.page, "TabSelected" },
			{ ImGuiCol_TabDimmedSelected, palette.page, "TabDimmedSelected" },
			{ ImGuiCol_TabSelectedOverline, palette.page,
			  "TabSelectedOverline" },
			{ ImGuiCol_TableHeaderBg, palette.base, "TableHeaderBg" },
			{ ImGuiCol_TableRowBg, palette.base, "TableRowBg" },
			{ ImGuiCol_TableRowBgAlt, palette.base, "TableRowBgAlt" },
			{ ImGuiCol_TableBorderStrong, palette.tableBorder,
			  "TableBorderStrong" },
			{ ImGuiCol_TableBorderLight, palette.tableBorder,
			  "TableBorderLight" },
			{ ImGuiCol_PlotHistogram, palette.highlight, "PlotHistogram" },
			{ ImGuiCol_DragDropTarget, palette.highlight, "DragDropTarget" },
			{ ImGuiCol_NavCursor, palette.highlight, "NavCursor" },
		};
		for ( const auto& slot : colorSlots ) {
			if ( ! sameImVec4( style.Colors[ slot.nSlot ],
							   toImVec4( slot.color ) ) ) {
				g_bOk = false;
				std::fprintf( stderr, "PLUGIN UI STYLE: slot %s not mapped\n",
							  slot.sName );
			}
		}
	}

	// The metrics: the GUI's 2px rounding, flat frames, a 1px tab border.
	{
		ImGuiStyle style;
		H2Core::PluginUiStyle::applyMetrics( style );
		check( style.FrameRounding == 2.0f, "FrameRounding" );
		check( style.TabRounding == 2.0f, "TabRounding" );
		check( style.GrabRounding == 2.0f, "GrabRounding" );
		check( style.WindowRounding == 0.0f, "WindowRounding" );
		check( style.ChildRounding == 0.0f, "ChildRounding" );
		check( style.FrameBorderSize == 0.0f, "FrameBorderSize" );
		check( style.WindowBorderSize == 0.0f, "WindowBorderSize" );
		check( style.TabBorderSize == 0.0f, "TabBorderSize" );
	}

	// The push-button scope swaps text and border, and restores both.
	{
		ImGui::CreateContext();
		ImGuiStyle& style = ImGui::GetStyle();
		H2Core::PluginUiStyle::applyMetrics( style );
		H2Core::PluginUiStyle::applyColors( palette, style );
		const ImVec4 textBefore = style.Colors[ ImGuiCol_Text ];
		const float fBorderBefore = style.FrameBorderSize;
		{
			H2Core::PluginUiStyle::ScopedPushButton scope( palette );
			check( sameImVec4( style.Colors[ ImGuiCol_Text ],
							   PluginUiColor::toImVec4( palette.buttonText ) ),
				   "push button text not applied" );
			check( sameImVec4( style.Colors[ ImGuiCol_Border ],
							   PluginUiColor::toImVec4( palette.buttonBorder ) ),
				   "push button border color not applied" );
			check( style.FrameBorderSize == 1.0f,
				   "push button border size not applied" );
		}
		check( sameImVec4( style.Colors[ ImGuiCol_Text ], textBefore ),
			   "push button text not restored" );
		check( style.FrameBorderSize == fBorderBefore,
			   "push button border size not restored" );

		// The check-box scope moves the frame onto the palette's base.
		const ImVec4 frameBefore = style.Colors[ ImGuiCol_FrameBg ];
		{
			H2Core::PluginUiStyle::ScopedCheckBox scope( palette );
			check( sameImVec4( style.Colors[ ImGuiCol_FrameBg ],
							   PluginUiColor::toImVec4( palette.checkBox ) ),
				   "check box frame not applied" );
			check( sameImVec4( style.Colors[ ImGuiCol_FrameBgHovered ],
							   PluginUiColor::toImVec4(
								   palette.checkBoxHovered ) ),
				   "check box hovered frame not applied" );
			check( sameImVec4( style.Colors[ ImGuiCol_FrameBgActive ],
							   PluginUiColor::toImVec4( palette.checkBox ) ),
				   "check box active frame not applied" );
		}
		check( sameImVec4( style.Colors[ ImGuiCol_FrameBg ], frameBefore ),
			   "check box frame not restored" );
		ImGui::DestroyContext();
	}

	std::printf( g_bOk ? "PLUGIN UI STYLE: PASSED\n"
					   : "PLUGIN UI STYLE: FAILED\n" );
	return g_bOk ? 0 : 1;
}
