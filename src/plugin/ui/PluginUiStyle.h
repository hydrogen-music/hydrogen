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

// The plugin UI's look (proposal 0006): the user's ColorTheme mapped onto
// imgui so the embedded UI reads like the Qt GUI. The GUI's own derivations
// (Skin's stylesheet, MixerLine, Rotary) live in src/gui, which the plugin
// UI must not include (tools/check_plugin_ui) — so the handful of derived
// shades they use is mirrored here, with QColor's lighter()/darker() math
// ported bit-for-bit (the core's Color carries no HSV on purpose).

#ifndef H2C_PLUGIN_UI_STYLE_H
#define H2C_PLUGIN_UI_STYLE_H

#include <imgui.h>

#include <core/Preferences/Color.h>

namespace H2Core {

class ColorTheme;

namespace PluginUiColor {

/** QColor::lighter() parity: scales the HSV value by @a nFactor percent,
 *  bleeding the overflow into the saturation. A factor below 100 darkens
 *  (Qt's delegation), a non-positive one returns @a color unchanged. An
 *  invalid color stays invalid. */
Color lighter( const Color& color, int nFactor );

/** QColor::darker() parity: divides the HSV value by @a nFactor percent.
 *  A factor below 100 lightens, a non-positive one returns @a color
 *  unchanged. An invalid color stays invalid. */
Color darker( const Color& color, int nFactor );

/** Skin::makeTextColorInactive() parity: the HSV value moved 55 towards
 *  the middle — the GUI's disabled-text shade. */
Color inactiveText( const Color& color );

/** The theme's colors are 8-bit RGB; imgui wants floats. */
ImVec4 toImVec4( const Color& color, float fAlpha = 1.0f );

} // namespace PluginUiColor

/** Every color the plugin UI paints with, resolved from a ColorTheme once
 *  per frame. A plain value: the views read it, never the theme, so the
 *  derivations live in exactly one place (fromTheme()) and a theme change
 *  shows on the next frame without a restart. */
struct PluginUiPalette
{
	/** Resolves the palette the Qt GUI would show for @a theme. */
	static PluginUiPalette fromTheme( const ColorTheme& theme );

	// The window: the tab bar sits on the window color, the active tab
	// merges into the lighter page below it (the Qt dialogs' tab pane).
	Color window;
	Color page;
	Color tab;
	Color tabHovered;
	Color text;
	Color textDisabled;
	Color separator;

	// Value fields — spin boxes, line edits, combo boxes (the plugin's
	// stand-in for the GUI's channel/note spin boxes).
	Color frame;
	Color frameHovered;
	Color frameActive;

	// Push buttons (Skin's QPushButton stylesheet): light face, dark
	// text, black 1px border.
	Color button;
	Color buttonHovered;
	Color buttonActive;
	Color buttonText;
	Color buttonBorder;

	// Tables, check boxes and popups sit on the palette's base.
	Color base;
	Color tableBorder;
	Color checkBox;
	Color checkBoxHovered;

	// Selection / attention.
	Color header;
	Color headerHovered;
	Color headerActive;
	Color highlight;
	Color error;

	// The mixer (MixerLine / Fader / Rotary).
	Color stripLane;
	Color stripPanel;
	Color stripButton;
	Color stripButtonHovered;
	Color meterWell;
 	Color faderHandle;
 	Color faderHandleActive;
	Color knobBody;
	Color knobRim;
	Color knobHandle;
	Color knobArc;
	Color knobCenterSet;
	Color knobCenterUnset;
	Color mute;
	Color muteText;
	Color solo;
	Color soloText;
};

namespace PluginUiStyle {

/** The size metrics of the Qt look (2px rounding, flat frames, 1px
 *  separators). Unscaled — apply before ImGuiStyle::ScaleAllSizes(). */
void applyMetrics( ImGuiStyle& style );

/** Maps @a palette onto every imgui color slot the plugin UI uses. */
void applyColors( const PluginUiPalette& palette, ImGuiStyle& style );

/** Scope guard for Qt-style push buttons: imgui draws button labels with
 *  the global text color, but the GUI's buttons carry dark text on a
 *  light face. Pushes the text color and the 1px border for the buttons
 *  drawn while it lives. */
class ScopedPushButton
{
public:
	explicit ScopedPushButton( const PluginUiPalette& palette );
	~ScopedPushButton();
	ScopedPushButton( const ScopedPushButton& ) = delete;
	ScopedPushButton& operator=( const ScopedPushButton& ) = delete;
};

/** Scope guard for Qt-style check boxes: imgui draws check boxes on the
 *  value-field frame (the spin box color), the GUI on the palette's
 *  base. */
class ScopedCheckBox
{
public:
	explicit ScopedCheckBox( const PluginUiPalette& palette );
	~ScopedCheckBox();
	ScopedCheckBox( const ScopedCheckBox& ) = delete;
	ScopedCheckBox& operator=( const ScopedCheckBox& ) = delete;
};

} // namespace PluginUiStyle

} // namespace H2Core

#endif // H2C_PLUGIN_UI_STYLE_H
