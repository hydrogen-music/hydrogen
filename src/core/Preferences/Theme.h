/*
 * Hydrogen
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef THEME_H
#define THEME_H

#include <vector>
#include <memory>

#include <core/Object.h>
#include <core/Helpers/Xml.h>

#include <QString>
#include <QColor>

namespace H2Core
{

/**
\ingroup H2CORE
\brief	Colors for hydrogen
*/
/** \ingroup docCore docConfiguration*/
class ColorTheme : public H2Core::Object<ColorTheme>
{
	H2_OBJECT(ColorTheme)
public:
	ColorTheme();
	ColorTheme( const ColorTheme& other ) = default;
	ColorTheme& operator=( const ColorTheme& other ) = default;
	ColorTheme( ColorTheme&& other ) = default;
	ColorTheme& operator=( ColorTheme&& other ) = default;

	void saveTo( XMLNode& parent ) const;
	static ColorTheme loadFrom( const XMLNode& parent,
								const bool bSilent = false );

	QString toQString( const QString& sPrefix = "",
					   bool bShort = true ) const override;

	QColor m_songEditor_backgroundColor;
	QColor m_songEditor_alternateRowColor;
	QColor m_songEditor_virtualRowColor;
	QColor m_songEditor_selectedRowColor;
	QColor m_songEditor_selectedRowTextColor;
	QColor m_songEditor_lineColor;
	QColor m_songEditor_textColor;
	QColor m_songEditor_automationBackgroundColor;
	QColor m_songEditor_automationLineColor;
	QColor m_songEditor_automationNodeColor;
	QColor m_songEditor_stackedModeOnColor;
	QColor m_songEditor_stackedModeOnNextColor;
	QColor m_songEditor_stackedModeOffNextColor;

	QColor m_patternEditor_backgroundColor;
	QColor m_patternEditor_alternateRowColor;
	QColor m_patternEditor_selectedRowColor;
	QColor m_patternEditor_selectedRowTextColor;
	QColor m_patternEditor_octaveRowColor;
	QColor m_patternEditor_textColor;
	QColor m_patternEditor_noteVelocityFullColor;
	QColor m_patternEditor_noteVelocityDefaultColor;
	QColor m_patternEditor_noteVelocityHalfColor;
	QColor m_patternEditor_noteVelocityZeroColor;
	QColor m_patternEditor_noteOffColor;
	QColor m_patternEditor_lineColor;
	QColor m_patternEditor_line1Color;
	QColor m_patternEditor_line2Color;
	QColor m_patternEditor_line3Color;
	QColor m_patternEditor_line4Color;
	QColor m_patternEditor_line5Color;
	QColor m_patternEditor_instrumentRowColor;
	QColor m_patternEditor_instrumentRowTextColor;
	QColor m_patternEditor_instrumentAlternateRowColor;
	QColor m_patternEditor_instrumentSelectedRowColor;
	QColor m_patternEditor_instrumentSelectedRowTextColor;

	QColor m_selectionHighlightColor;
	QColor m_selectionInactiveColor;

	// QWidget palette stuff
	/** A general background color.*/
	QColor m_windowColor;
	/** A general foreground color.*/
	QColor m_windowTextColor;
	/** Used as the background color for text entry widgets; usually white or another light color.*/
	QColor m_baseColor;
	/** Used as the alternate background color in views with alternating row colors.*/
	QColor m_alternateBaseColor;
	/** The foreground color used with Base. This is usually the same as the Foreground, in which case it must provide good contrast with Background and Base.*/
	QColor m_textColor;
	/** The general button background color. This background can be different from Background as some styles require a different background color for buttons.*/
	QColor m_buttonColor;
	/** A foreground color used with the Button color.*/
	QColor m_buttonTextColor;
	/** Lighter than Button color.*/
	QColor m_lightColor;
	/** Between Button and Light.*/
	QColor m_midLightColor;
	/** Darker than Button.*/
	QColor m_midColor;
	/** Between Button and Dark.*/
	QColor m_darkColor;
	/** A very dark color. By default, the shadow color is Qt::black.*/
	QColor m_shadowTextColor;
	/** A color to indicate a selected item or the current item.*/
	QColor m_highlightColor;
	/** A text color that contrasts with Highlight.*/
	QColor m_highlightedTextColor;
	QColor m_toolTipBaseColor;
	QColor m_toolTipTextColor;

	// General widget stuff
	QColor m_accentColor;
	QColor m_accentTextColor;
	QColor m_widgetColor;
	QColor m_widgetTextColor;
	QColor m_buttonRedColor;
	QColor m_buttonRedTextColor;
	QColor m_spinBoxColor;
	QColor m_spinBoxTextColor;
	QColor m_playheadColor;
	QColor m_cursorColor;
};

	
/** \ingroup docCore docConfiguration*/
class InterfaceTheme : public H2Core::Object<InterfaceTheme>
{
	H2_OBJECT(InterfaceTheme)
public:
	InterfaceTheme();
	InterfaceTheme( const InterfaceTheme& other ) = default;
	InterfaceTheme& operator=( const InterfaceTheme& other ) = default;
	InterfaceTheme( InterfaceTheme&& other ) = default;
	InterfaceTheme& operator=( InterfaceTheme&& other ) = default;

	static float FALLOFF_SLOW;
	static float FALLOFF_NORMAL;
	static float FALLOFF_FAST;

	enum class Layout {
		SinglePane = 0,
		Tabbed = 1
	};
		static QString LayoutToQString( const Layout& layout );

	enum class ScalingPolicy {
		Smaller = 0,
		System = 1,
		Larger = 2
	};
		static QString ScalingPolicyToQString( const ScalingPolicy& policy );

	enum class IconColor {
		Black = 0,
		White = 1
	};
		static QString IconColorToQString( const IconColor& color );

	enum class ColoringMethod {
		Automatic = 0,
		Custom = 1
	};
		static QString ColoringMethodToQString( const ColoringMethod& method );

	QString toQString( const QString& sPrefix = "",
					   bool bShort = true ) const override;

	QString m_sQTStyle;
	float m_fMixerFalloffSpeed;
	Layout m_layout;
	ScalingPolicy m_uiScalingPolicy;
	IconColor m_iconColor;
	ColoringMethod m_coloringMethod;
	std::vector<QColor> m_patternColors;
	int	m_nVisiblePatternColors;
	/** Not read from/written to disk */
	static constexpr int nMaxPatternColors = 50;
};
	
/** \ingroup docCore docConfiguration*/
class FontTheme : public H2Core::Object<FontTheme>
{
	H2_OBJECT(FontTheme)
public:
	FontTheme();
	FontTheme( const FontTheme& other ) = default;
	FontTheme& operator=( const FontTheme& other ) = default;
	FontTheme( FontTheme&& other ) = default;
	FontTheme& operator=( FontTheme&& other ) = default;

	/** Enables custom scaling of the font size in the GUI.*/
	enum class FontSize {
		Medium = 0,
		Small = 1,
		Large = 2
	};
		static QString FontSizeToQString( const FontSize& fontSize );

	QString toQString( const QString& sPrefix = "",
					   bool bShort = true ) const override;

	QString	m_sApplicationFontFamily;
	QString	m_sLevel2FontFamily;
	QString m_sLevel3FontFamily;
	FontSize m_fontSize;
};
	
/** \ingroup docCore docConfiguration*/
class Theme : public H2Core::Object<Theme> {
	H2_OBJECT(Theme)
public:
	Theme( const ColorTheme& colorTheme = ColorTheme(),
		   const InterfaceTheme& interfaceTheme = InterfaceTheme(),
		   const FontTheme& fontTheme = FontTheme() );

	Theme( const Theme& other );
	Theme& operator=( const Theme& other );

	Theme( Theme&& other );
	Theme& operator=( Theme&& other );

	static std::unique_ptr<Theme> importFrom( const QString& sPath );
	bool exportTo( const QString& sPath ) const;

	QString toQString( const QString& sPrefix = "",
					   bool bShort = true ) const override;

	ColorTheme m_color;
	InterfaceTheme m_interface;
	FontTheme m_font;
};

};

#endif

