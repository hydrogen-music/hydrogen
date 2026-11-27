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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2C_THEME_H
#define H2C_THEME_H

#include <vector>
#include <memory>

#include <core/Object.h>
#include <core/Helpers/Xml.h>

#include <QString>
#include <QStringList>
#include <core/Preferences/Color.h>

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
	ColorTheme( std::shared_ptr<ColorTheme> pOther );
	ColorTheme operator=( std::shared_ptr<ColorTheme> pOther ) = delete;

	void saveTo( XMLNode& parent ) const;
	static std::shared_ptr<ColorTheme> loadFrom( const XMLNode& parent,
												const bool bSilent = false );

	QString toQString( const QString& sPrefix = "",
					   bool bShort = true ) const override;

	Color m_songEditor_backgroundColor;
	Color m_songEditor_alternateRowColor;
	Color m_songEditor_virtualRowColor;
	Color m_songEditor_selectedRowColor;
	Color m_songEditor_selectedRowTextColor;
	Color m_songEditor_lineColor;
	Color m_songEditor_textColor;
	Color m_songEditor_automationBackgroundColor;
	Color m_songEditor_automationLineColor;
	Color m_songEditor_automationNodeColor;
	Color m_songEditor_stackedModeOnColor;
	Color m_songEditor_stackedModeOnNextColor;
	Color m_songEditor_stackedModeOffNextColor;

	Color m_patternEditor_backgroundColor;
	Color m_patternEditor_alternateRowColor;
	Color m_patternEditor_selectedRowColor;
	Color m_patternEditor_selectedRowTextColor;
	Color m_patternEditor_octaveRowColor;
	Color m_patternEditor_textColor;
	Color m_patternEditor_noteVelocityFullColor;
	Color m_patternEditor_noteVelocityDefaultColor;
	Color m_patternEditor_noteVelocityHalfColor;
	Color m_patternEditor_noteVelocityZeroColor;
	Color m_patternEditor_noteOffColor;
	Color m_patternEditor_lineColor;
	Color m_patternEditor_line1Color;
	Color m_patternEditor_line2Color;
	Color m_patternEditor_line3Color;
	Color m_patternEditor_line4Color;
	Color m_patternEditor_line5Color;
	Color m_patternEditor_instrumentRowColor;
	Color m_patternEditor_instrumentRowTextColor;
	Color m_patternEditor_instrumentAlternateRowColor;
	Color m_patternEditor_instrumentSelectedRowColor;
	Color m_patternEditor_instrumentSelectedRowTextColor;

	Color m_componentEditor_componentColor;
	Color m_componentEditor_componentTextColor;
	Color m_componentEditor_layerColor;
	Color m_componentEditor_layerTextColor;

	Color m_sampleEditor_backgroundColor;
	Color m_sampleEditor_textColor;
	Color m_sampleEditor_playheadColor;
	Color m_sampleEditor_startSliderColor;
	Color m_sampleEditor_loopSliderColor;
	Color m_sampleEditor_endSliderColor;
	Color m_sampleEditor_velocityEnvelopeColor;
	Color m_sampleEditor_panEnvelopeColor;

	Color m_selectionHighlightColor;
	Color m_selectionInactiveColor;

	// QWidget palette stuff
	/** A general background color.*/
	Color m_windowColor;
	/** A general foreground color.*/
	Color m_windowTextColor;
	/** Used as the background color for text entry widgets; usually white or
	 * another light color.*/
	Color m_baseColor;
	/** Used as the alternate background color in views with alternating row
	 * colors.*/
	Color m_alternateBaseColor;
	/** The foreground color used with Base. This is usually the same as the
	 * Foreground, in which case it must provide good contrast with Background
	 * and Base.*/
	Color m_textColor;
	/** The general button background color. This background can be different
	 * from Background as some styles require a different background color for
	 * buttons.*/
	Color m_buttonColor;
	/** A foreground color used with the Button color.*/
	Color m_buttonTextColor;
	/** Lighter than Button color.*/
	Color m_lightColor;
	/** Between Button and Light.*/
	Color m_midLightColor;
	/** Darker than Button.*/
	Color m_midColor;
	/** Between Button and Dark.*/
	Color m_darkColor;
	/** A very dark color. By delete, the shadow color is Qt::black.*/
	Color m_shadowTextColor;
	/** A color to indicate a selected item or the current item.*/
	Color m_highlightColor;
	/** A text color that contrasts with Highlight.*/
	Color m_highlightedTextColor;
	Color m_toolTipBaseColor;
	Color m_toolTipTextColor;

	// General widget stuff
	Color m_accentColor;
	Color m_accentTextColor;
	Color m_widgetColor;
	Color m_widgetTextColor;
	Color m_buttonRedColor;
	Color m_buttonRedTextColor;
	Color m_spinBoxColor;
	Color m_spinBoxTextColor;
	Color m_playheadColor;
	Color m_cursorColor;
	Color m_muteColor;
	Color m_muteTextColor;
	Color m_soloColor;
	Color m_soloTextColor;
	Color m_waveFormColor;
};

	
/** \ingroup docCore docConfiguration*/
class InterfaceTheme : public H2Core::Object<InterfaceTheme>
{
	H2_OBJECT(InterfaceTheme)
public:
	InterfaceTheme();
	InterfaceTheme( std::shared_ptr<InterfaceTheme> pOther );
	InterfaceTheme operator=( std::shared_ptr<InterfaceTheme> pOther ) = delete;

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
	std::vector<Color> m_patternColors;
	int	m_nVisiblePatternColors;
	/** Not read from/written to disk */
	static constexpr int nMaxPatternColors = 50;

		/** Whether notes which will not be rendered by the audio engine be
		 * highlighted in a distinct color. */
		bool m_bIndicateNotePlayback;
		/** Whether notes stopped prematurely by an adjecent note off or another
		 * note from the same mute group will be rendered with a tail. */
		bool m_bIndicateEffectiveNoteLength;
};
	
/** \ingroup docCore docConfiguration*/
class FontTheme : public H2Core::Object<FontTheme>
{
	H2_OBJECT(FontTheme)
public:
	FontTheme();
	FontTheme( std::shared_ptr<FontTheme> pOther );
	FontTheme operator=( std::shared_ptr<FontTheme> pOther ) = delete;

	/** Enables custom scaling of the font size in the GUI.*/
	enum class FontSize {
		Medium = 0,
		Small = 1,
		Large = 2
	};
		static QString FontSizeToQString( const FontSize& fontSize );

	/**
	 * Replaces any family which is empty or not contained in
	 * sAvailableFamilies with sFallbackFamily.
	 *
	 * Deliberately a pure string operation: the headless core (plugin
	 * host) must never enumerate fonts, and keeping QFont/QFontDatabase
	 * out makes this unit-testable without a QGuiApplication. The GUI
	 * passes the local QFontDatabase content once at its boundary.
	 */
	void sanitizeFamilies( const QStringList& sAvailableFamilies,
						   const QString& sFallbackFamily );

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
	Theme( std::shared_ptr<ColorTheme> pColorTheme,
		   std::shared_ptr<InterfaceTheme> pInterfaceTheme,
		   std::shared_ptr<FontTheme> pFontTheme );

	Theme( const std::shared_ptr<const Theme> pOther );
	Theme operator=( const std::shared_ptr<const Theme> pOther );

	static std::shared_ptr<Theme> importFrom( const QString& sPath );
	bool exportTo( const QString& sPath ) const;

	QString toQString( const QString& sPrefix = "",
					   bool bShort = true ) const override;

	std::shared_ptr<ColorTheme> m_pColor;
	std::shared_ptr<InterfaceTheme> m_pInterface;
	std::shared_ptr<FontTheme> m_pFont;
};

};

#endif

