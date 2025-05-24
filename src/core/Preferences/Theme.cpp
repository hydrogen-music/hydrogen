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
#include <fstream>

#include "Theme.h"

#include <core/Helpers/Filesystem.h>
#include "../Version.h"

namespace H2Core
{

ColorTheme::ColorTheme()
	: m_songEditor_backgroundColor( QColor( 128, 134, 152 ) )
	, m_songEditor_alternateRowColor( QColor( 106, 111, 126 ) )
	, m_songEditor_virtualRowColor( QColor( 120, 112, 97 ) )
	, m_songEditor_selectedRowColor( QColor( 149, 157, 178 ) )
	, m_songEditor_selectedRowTextColor( QColor( 0, 0, 0 ) )
	, m_songEditor_lineColor( QColor( 54, 57, 67 ) )
	, m_songEditor_textColor( QColor( 206, 211, 224 ) )
	, m_songEditor_automationBackgroundColor( QColor( 83, 89, 103 ) )
	, m_songEditor_automationLineColor( QColor( 45, 66, 89 ) )
	, m_songEditor_automationNodeColor( QColor( 255, 255, 255 ) )
	, m_songEditor_stackedModeOnColor( QColor( 127, 159, 127 ) )
	, m_songEditor_stackedModeOnNextColor( QColor( 240, 223, 175 ) )
	, m_songEditor_stackedModeOffNextColor( QColor( 247, 100, 100 ) )
	, m_patternEditor_backgroundColor( QColor( 165, 166, 160 ) )
	, m_patternEditor_alternateRowColor( QColor( 133, 134, 129 ) )
	, m_patternEditor_selectedRowColor( QColor( 194, 195, 187 ) )
	, m_patternEditor_selectedRowTextColor( QColor( 0, 0, 0 ) )
	, m_patternEditor_octaveRowColor( QColor( 193, 194, 186 ) )
	, m_patternEditor_textColor( QColor( 240, 240, 240 ) )
	, m_patternEditor_noteVelocityFullColor( QColor( 247, 100, 100 ) )
	, m_patternEditor_noteVelocityDefaultColor( QColor( 40, 40, 40 ) )
	, m_patternEditor_noteVelocityHalfColor( QColor( 89, 131, 175 ) )
	, m_patternEditor_noteVelocityZeroColor( QColor( 255, 255, 255 ) )
	, m_patternEditor_noteOffColor( QColor( 71, 79, 191 ) )
	, m_patternEditor_lineColor( QColor(45, 45, 45) )
	, m_patternEditor_line1Color( QColor(55, 55, 55) )
	, m_patternEditor_line2Color( QColor(75, 75, 75) )
	, m_patternEditor_line3Color( QColor(95, 95, 95) )
	, m_patternEditor_line4Color( QColor(105, 105, 105) )
	, m_patternEditor_line5Color( QColor(115, 115, 115) )
	, m_selectionHighlightColor( QColor( 255, 255, 255 ) )
	, m_selectionInactiveColor( QColor( 199, 199, 199 ) )
	, m_windowColor( QColor( 58, 62, 72 ) )
	, m_windowTextColor( QColor( 255, 255, 255 ) )
	, m_baseColor( QColor( 88, 94, 112 ) )
	, m_alternateBaseColor( QColor( 138, 144, 162 ) )
	, m_textColor( QColor( 255, 255, 255 ) )
	, m_buttonColor( QColor( 88, 94, 112 ) )
	, m_buttonTextColor( QColor( 255, 255, 255 ) )
	, m_lightColor( QColor( 138, 144, 162 ) )
	, m_midLightColor( QColor( 128, 134, 152 ) )
	, m_midColor( QColor( 58, 62, 72 ) )
	, m_darkColor( QColor( 81, 86, 99 ) )
	, m_shadowTextColor( QColor( 255, 255, 255 ) )
	, m_highlightColor( QColor( 206, 150, 30 ) )
	, m_highlightedTextColor( QColor( 255, 255, 255 ) )
	, m_toolTipBaseColor( QColor( 227, 243, 252 ) )
	, m_toolTipTextColor( QColor( 64, 64, 66 ) )
	, m_widgetColor( QColor( 164, 170, 190 ) )
	, m_widgetTextColor( QColor( 10, 10, 10 ) )
	, m_buttonRedColor( QColor( 247, 100, 100 ) )
	, m_buttonRedTextColor( QColor( 10, 10, 10 ) )
	, m_spinBoxColor( QColor( 51, 74 , 100 ) )
	, m_spinBoxTextColor( QColor( 240, 240, 240 ) )
	, m_accentColor( QColor( 67, 96, 131 ) )
	, m_accentTextColor( QColor( 255, 255, 255 ) )
	, m_playheadColor( QColor( 0, 0, 0 ) )
	, m_cursorColor( QColor( 38, 39, 44 ) )
{
}

ColorTheme::ColorTheme( const std::shared_ptr<ColorTheme> pOther )
	: m_songEditor_backgroundColor( pOther->m_songEditor_backgroundColor )
	, m_songEditor_alternateRowColor( pOther->m_songEditor_alternateRowColor )
	, m_songEditor_virtualRowColor( pOther->m_songEditor_virtualRowColor )
	, m_songEditor_selectedRowColor( pOther->m_songEditor_selectedRowColor )
	, m_songEditor_selectedRowTextColor( pOther->m_songEditor_selectedRowTextColor )
	, m_songEditor_lineColor( pOther->m_songEditor_lineColor )
	, m_songEditor_textColor( pOther->m_songEditor_textColor )
	, m_songEditor_automationBackgroundColor( pOther->m_songEditor_automationBackgroundColor )
	, m_songEditor_automationLineColor( pOther->m_songEditor_automationLineColor )
	, m_songEditor_automationNodeColor( pOther->m_songEditor_automationNodeColor )
	, m_songEditor_stackedModeOnColor( pOther->m_songEditor_stackedModeOnColor )
	, m_songEditor_stackedModeOnNextColor( pOther->m_songEditor_stackedModeOnNextColor )
	, m_songEditor_stackedModeOffNextColor( pOther->m_songEditor_stackedModeOffNextColor )
	, m_patternEditor_backgroundColor( pOther->m_patternEditor_backgroundColor )
	, m_patternEditor_alternateRowColor( pOther->m_patternEditor_alternateRowColor )
	, m_patternEditor_selectedRowColor( pOther->m_patternEditor_selectedRowColor )
	, m_patternEditor_selectedRowTextColor( pOther->m_patternEditor_selectedRowTextColor )
	, m_patternEditor_octaveRowColor( pOther->m_patternEditor_octaveRowColor )
	, m_patternEditor_textColor( pOther->m_patternEditor_textColor )
	, m_patternEditor_noteVelocityFullColor( pOther->m_patternEditor_noteVelocityFullColor )
	, m_patternEditor_noteVelocityDefaultColor( pOther->m_patternEditor_noteVelocityDefaultColor )
	, m_patternEditor_noteVelocityHalfColor( pOther->m_patternEditor_noteVelocityHalfColor )
	, m_patternEditor_noteVelocityZeroColor( pOther->m_patternEditor_noteVelocityZeroColor )
	, m_patternEditor_noteOffColor( pOther->m_patternEditor_noteOffColor )
	, m_patternEditor_lineColor( pOther->m_patternEditor_lineColor )
	, m_patternEditor_line1Color( pOther->m_patternEditor_line1Color )
	, m_patternEditor_line2Color( pOther->m_patternEditor_line2Color )
	, m_patternEditor_line3Color( pOther->m_patternEditor_line3Color )
	, m_patternEditor_line4Color( pOther->m_patternEditor_line4Color )
	, m_patternEditor_line5Color( pOther->m_patternEditor_line5Color )
	, m_selectionHighlightColor( pOther->m_selectionHighlightColor )
	, m_selectionInactiveColor( pOther->m_selectionInactiveColor )
	, m_windowColor( pOther->m_windowColor )
	, m_windowTextColor( pOther->m_windowTextColor )
	, m_baseColor( pOther->m_baseColor )
	, m_alternateBaseColor( pOther->m_alternateBaseColor )
	, m_textColor( pOther->m_textColor )
	, m_buttonColor( pOther->m_buttonColor )
	, m_buttonTextColor( pOther->m_buttonTextColor )
	, m_lightColor( pOther->m_lightColor )
	, m_midLightColor( pOther->m_midLightColor )
	, m_midColor( pOther->m_midColor )
	, m_darkColor( pOther->m_darkColor )
	, m_shadowTextColor( pOther->m_shadowTextColor )
	, m_highlightColor( pOther->m_highlightColor )
	, m_highlightedTextColor( pOther->m_highlightedTextColor )
	, m_toolTipBaseColor( pOther->m_toolTipBaseColor )
	, m_toolTipTextColor( pOther->m_toolTipTextColor )
	, m_accentColor( pOther->m_accentColor )
	, m_accentTextColor( pOther->m_accentTextColor )
	, m_widgetColor( pOther->m_widgetColor )
	, m_widgetTextColor( pOther->m_widgetTextColor )
	, m_buttonRedColor( pOther->m_buttonRedColor )
	, m_buttonRedTextColor( pOther->m_buttonRedTextColor )
	, m_spinBoxColor( pOther->m_spinBoxColor )
	, m_spinBoxTextColor( pOther->m_spinBoxTextColor )
	, m_playheadColor( pOther->m_playheadColor )
	, m_cursorColor( pOther->m_cursorColor )
{
}

float InterfaceTheme::FALLOFF_SLOW = 1.08f;
float InterfaceTheme::FALLOFF_NORMAL = 1.1f;
float InterfaceTheme::FALLOFF_FAST = 1.5f;

InterfaceTheme::InterfaceTheme()
	: m_sQTStyle( "Fusion" )
	, m_fMixerFalloffSpeed( InterfaceTheme::FALLOFF_NORMAL )
	, m_layout( InterfaceTheme::Layout::SinglePane )
	, m_scalingPolicy( InterfaceTheme::ScalingPolicy::Smaller )
	, m_iconColor( InterfaceTheme::IconColor::Black )
	, m_coloringMethod( InterfaceTheme::ColoringMethod::Custom )
	, m_nVisiblePatternColors( 18 )
	, m_nMaxPatternColors( 50 ) {
	m_patternColors.resize( m_nMaxPatternColors );
	for ( int ii = 0; ii < m_nMaxPatternColors; ii++ ) {
		m_patternColors[ ii ] = QColor( 67, 96, 131 );
	}
}

InterfaceTheme::InterfaceTheme( const std::shared_ptr<InterfaceTheme> pOther )
	: m_sQTStyle( pOther->m_sQTStyle )
	, m_fMixerFalloffSpeed( pOther->m_fMixerFalloffSpeed )
	, m_layout( pOther->m_layout )
	, m_scalingPolicy( pOther->m_scalingPolicy )
	, m_iconColor( pOther->m_iconColor )
	, m_coloringMethod( pOther->m_coloringMethod )
	, m_nVisiblePatternColors( pOther->m_nVisiblePatternColors )
	, m_nMaxPatternColors( pOther->m_nMaxPatternColors ){
	m_patternColors.resize( pOther->m_nMaxPatternColors );
	for ( int ii = 0; ii < pOther->m_nMaxPatternColors; ii++ ) {
		m_patternColors[ ii ] = pOther->m_patternColors[ ii ];
	}
}

FontTheme::FontTheme()
	: m_sApplicationFontFamily( "Lucida Grande" )
	, m_sLevel2FontFamily( "Lucida Grande" )
	, m_sLevel3FontFamily( "Lucida Grande" )
	, m_fontSize( FontTheme::FontSize::Medium ) {
}

FontTheme::FontTheme( const std::shared_ptr<FontTheme> pOther )
	: m_sApplicationFontFamily( pOther->m_sApplicationFontFamily )
	, m_sLevel2FontFamily( pOther->m_sLevel2FontFamily )
	, m_sLevel3FontFamily( pOther->m_sLevel3FontFamily )
	, m_fontSize( pOther->m_fontSize ) {
}

Theme::Theme() {
	m_pColorTheme = std::make_shared<ColorTheme>();
	m_pInterfaceTheme = std::make_shared<InterfaceTheme>();
	m_pFontTheme = std::make_shared<FontTheme>();
}

Theme::Theme( const std::shared_ptr<Theme> pOther ) {
	m_pColorTheme = std::make_shared<ColorTheme>( pOther->getColorTheme() );
	m_pInterfaceTheme = std::make_shared<InterfaceTheme>( pOther->getInterfaceTheme() );
	m_pFontTheme = std::make_shared<FontTheme>( pOther->getFontTheme() );
}

void Theme::setTheme( const std::shared_ptr<Theme> pOther ) {
	m_pColorTheme->m_songEditor_backgroundColor = pOther->getColorTheme()->m_songEditor_backgroundColor;
	m_pColorTheme->m_songEditor_alternateRowColor = pOther->getColorTheme()->m_songEditor_alternateRowColor;
	m_pColorTheme->m_songEditor_virtualRowColor = pOther->getColorTheme()->m_songEditor_virtualRowColor;
	m_pColorTheme->m_songEditor_selectedRowColor =
		pOther->getColorTheme()->m_songEditor_selectedRowColor;
	m_pColorTheme->m_songEditor_selectedRowTextColor =
		pOther->getColorTheme()->m_songEditor_selectedRowTextColor;
	m_pColorTheme->m_songEditor_lineColor = pOther->getColorTheme()->m_songEditor_lineColor;
	m_pColorTheme->m_songEditor_textColor = pOther->getColorTheme()->m_songEditor_textColor;
	m_pColorTheme->m_songEditor_automationBackgroundColor =
		pOther->getColorTheme()->m_songEditor_automationBackgroundColor;
	m_pColorTheme->m_songEditor_automationLineColor =
		pOther->getColorTheme()->m_songEditor_automationLineColor;
	m_pColorTheme->m_songEditor_automationNodeColor =
		pOther->getColorTheme()->m_songEditor_automationNodeColor;
	m_pColorTheme->m_songEditor_stackedModeOnColor =
		pOther->getColorTheme()->m_songEditor_stackedModeOnColor;
	m_pColorTheme->m_songEditor_stackedModeOnNextColor =
		pOther->getColorTheme()->m_songEditor_stackedModeOnNextColor;
	m_pColorTheme->m_songEditor_stackedModeOffNextColor =
		pOther->getColorTheme()->m_songEditor_stackedModeOffNextColor;
	m_pColorTheme->m_patternEditor_backgroundColor = pOther->getColorTheme()->m_patternEditor_backgroundColor;
	m_pColorTheme->m_patternEditor_alternateRowColor = pOther->getColorTheme()->m_patternEditor_alternateRowColor;
	m_pColorTheme->m_patternEditor_selectedRowColor =
		pOther->getColorTheme()->m_patternEditor_selectedRowColor;
	m_pColorTheme->m_patternEditor_selectedRowTextColor =
		pOther->getColorTheme()->m_patternEditor_selectedRowTextColor;
	m_pColorTheme->m_patternEditor_octaveRowColor =
		pOther->getColorTheme()->m_patternEditor_octaveRowColor;
	m_pColorTheme->m_patternEditor_textColor = pOther->getColorTheme()->m_patternEditor_textColor;
	m_pColorTheme->m_patternEditor_noteVelocityFullColor =
		pOther->getColorTheme()->m_patternEditor_noteVelocityFullColor;
	m_pColorTheme->m_patternEditor_noteVelocityDefaultColor =
		pOther->getColorTheme()->m_patternEditor_noteVelocityDefaultColor;
	m_pColorTheme->m_patternEditor_noteVelocityHalfColor =
		pOther->getColorTheme()->m_patternEditor_noteVelocityHalfColor;
	m_pColorTheme->m_patternEditor_noteVelocityZeroColor =
		pOther->getColorTheme()->m_patternEditor_noteVelocityZeroColor;
	m_pColorTheme->m_patternEditor_noteOffColor =
		pOther->getColorTheme()->m_patternEditor_noteOffColor;
	m_pColorTheme->m_patternEditor_lineColor = pOther->getColorTheme()->m_patternEditor_lineColor;
	m_pColorTheme->m_patternEditor_line1Color = pOther->getColorTheme()->m_patternEditor_line1Color;
	m_pColorTheme->m_patternEditor_line2Color = pOther->getColorTheme()->m_patternEditor_line2Color;
	m_pColorTheme->m_patternEditor_line3Color = pOther->getColorTheme()->m_patternEditor_line3Color;
	m_pColorTheme->m_patternEditor_line4Color = pOther->getColorTheme()->m_patternEditor_line4Color;
	m_pColorTheme->m_patternEditor_line5Color = pOther->getColorTheme()->m_patternEditor_line5Color;
	m_pColorTheme->m_selectionHighlightColor = pOther->getColorTheme()->m_selectionHighlightColor;
	m_pColorTheme->m_selectionInactiveColor = pOther->getColorTheme()->m_selectionInactiveColor;
	m_pColorTheme->m_windowColor = pOther->getColorTheme()->m_windowColor;
	m_pColorTheme->m_windowTextColor = pOther->getColorTheme()->m_windowTextColor;
	m_pColorTheme->m_baseColor = pOther->getColorTheme()->m_baseColor;
	m_pColorTheme->m_alternateBaseColor = pOther->getColorTheme()->m_alternateBaseColor;
	m_pColorTheme->m_textColor = pOther->getColorTheme()->m_textColor;
	m_pColorTheme->m_buttonColor = pOther->getColorTheme()->m_buttonColor;
	m_pColorTheme->m_buttonTextColor = pOther->getColorTheme()->m_buttonTextColor;
	m_pColorTheme->m_lightColor = pOther->getColorTheme()->m_lightColor;
	m_pColorTheme->m_midLightColor = pOther->getColorTheme()->m_midLightColor;
	m_pColorTheme->m_midColor = pOther->getColorTheme()->m_midColor;
	m_pColorTheme->m_darkColor = pOther->getColorTheme()->m_darkColor;
	m_pColorTheme->m_shadowTextColor = pOther->getColorTheme()->m_shadowTextColor;
	m_pColorTheme->m_highlightColor = pOther->getColorTheme()->m_highlightColor;
	m_pColorTheme->m_highlightedTextColor = pOther->getColorTheme()->m_highlightedTextColor;
	m_pColorTheme->m_toolTipBaseColor = pOther->getColorTheme()->m_toolTipBaseColor;
	m_pColorTheme->m_toolTipTextColor = pOther->getColorTheme()->m_toolTipTextColor;
	m_pColorTheme->m_accentColor = pOther->getColorTheme()->m_accentColor;
	m_pColorTheme->m_accentTextColor = pOther->getColorTheme()->m_accentTextColor;
	m_pColorTheme->m_widgetColor = pOther->getColorTheme()->m_widgetColor;
	m_pColorTheme->m_widgetTextColor = pOther->getColorTheme()->m_widgetTextColor;
	m_pColorTheme->m_buttonRedColor = pOther->getColorTheme()->m_buttonRedColor;
	m_pColorTheme->m_buttonRedTextColor = pOther->getColorTheme()->m_buttonRedTextColor;
	m_pColorTheme->m_spinBoxColor = pOther->getColorTheme()->m_spinBoxColor;
	m_pColorTheme->m_spinBoxTextColor = pOther->getColorTheme()->m_spinBoxTextColor;
	m_pColorTheme->m_playheadColor = pOther->getColorTheme()->m_playheadColor;
	m_pColorTheme->m_cursorColor = pOther->getColorTheme()->m_cursorColor;
		
	m_pInterfaceTheme->m_sQTStyle = pOther->getInterfaceTheme()->m_sQTStyle;
	m_pInterfaceTheme->m_fMixerFalloffSpeed = pOther->getInterfaceTheme()->m_fMixerFalloffSpeed;
	m_pInterfaceTheme->m_layout = pOther->getInterfaceTheme()->m_layout;
	m_pInterfaceTheme->m_scalingPolicy = pOther->getInterfaceTheme()->m_scalingPolicy;
	m_pInterfaceTheme->m_iconColor = pOther->getInterfaceTheme()->m_iconColor;
	m_pInterfaceTheme->m_coloringMethod = pOther->getInterfaceTheme()->m_coloringMethod;
	m_pInterfaceTheme->m_nVisiblePatternColors = pOther->getInterfaceTheme()->m_nVisiblePatternColors;
	m_pInterfaceTheme->m_nMaxPatternColors = pOther->getInterfaceTheme()->m_nMaxPatternColors;
	std::vector<QColor> patternColors( pOther->getInterfaceTheme()->m_nMaxPatternColors );
	for ( int ii = 0; ii < pOther->getInterfaceTheme()->m_nMaxPatternColors; ii++ ) {
		patternColors[ ii ] = pOther->getInterfaceTheme()->m_patternColors[ ii ];
	}

	m_pInterfaceTheme->m_patternColors = patternColors;
	
	m_pFontTheme->m_sApplicationFontFamily = pOther->getFontTheme()->m_sApplicationFontFamily;
	m_pFontTheme->m_sLevel2FontFamily = pOther->getFontTheme()->m_sLevel2FontFamily;
	m_pFontTheme->m_sLevel3FontFamily = pOther->getFontTheme()->m_sLevel3FontFamily;
	m_pFontTheme->m_fontSize = pOther->getFontTheme()->m_fontSize;
}

void Theme::writeColorTheme( XMLNode* pParent, std::shared_ptr<Theme> pTheme )
{
	auto pColorTheme = pTheme->getColorTheme();
	
	XMLNode colorThemeNode = pParent->createNode( "colorTheme" );

	// SONG EDITOR
	XMLNode songEditorNode = colorThemeNode.createNode( "songEditor" );
	songEditorNode.write_color( "backgroundColor", pColorTheme->m_songEditor_backgroundColor );
	songEditorNode.write_color( "alternateRowColor", pColorTheme->m_songEditor_alternateRowColor );
	songEditorNode.write_color( "virtualRowColor", pColorTheme->m_songEditor_virtualRowColor );
	songEditorNode.write_color( "selectedRowColor",
								 pColorTheme->m_songEditor_selectedRowColor );
	songEditorNode.write_color( "selectedRowTextColor",
								 pColorTheme->m_songEditor_selectedRowTextColor );
	songEditorNode.write_color( "lineColor", pColorTheme->m_songEditor_lineColor );
	songEditorNode.write_color( "textColor", pColorTheme->m_songEditor_textColor );
	songEditorNode.write_color( "automationBackgroundColor",
								 pColorTheme->m_songEditor_automationBackgroundColor );
	songEditorNode.write_color( "automationLineColor",
								 pColorTheme->m_songEditor_automationLineColor );
	songEditorNode.write_color( "automationNodeColor",
								 pColorTheme->m_songEditor_automationNodeColor );
	songEditorNode.write_color( "stackedModeOnColor",
								 pColorTheme->m_songEditor_stackedModeOnColor );
	songEditorNode.write_color( "stackedModeOnNextColor",
								 pColorTheme->m_songEditor_stackedModeOnNextColor );
	songEditorNode.write_color( "stackedModeOffNextColor",
								 pColorTheme->m_songEditor_stackedModeOffNextColor );

	// PATTERN EDITOR
	XMLNode patternEditorNode = colorThemeNode.createNode( "patternEditor" );
	patternEditorNode.write_color( "backgroundColor", pColorTheme->m_patternEditor_backgroundColor );
	patternEditorNode.write_color( "alternateRowColor", pColorTheme->m_patternEditor_alternateRowColor );
	patternEditorNode.write_color( "selectedRowColor",
								 pColorTheme->m_patternEditor_selectedRowColor );
	patternEditorNode.write_color( "selectedRowTextColor",
								 pColorTheme->m_patternEditor_selectedRowTextColor );
	patternEditorNode.write_color( "octaveRowColor",
								 pColorTheme->m_patternEditor_octaveRowColor );
	patternEditorNode.write_color( "textColor", pColorTheme->m_patternEditor_textColor );
	patternEditorNode.write_color( "noteVelocityFullColor",
								 pColorTheme->m_patternEditor_noteVelocityFullColor );
	patternEditorNode.write_color( "noteVelocityDefaultColor",
								 pColorTheme->m_patternEditor_noteVelocityDefaultColor );
	patternEditorNode.write_color( "noteVelocityHalfColor",
								 pColorTheme->m_patternEditor_noteVelocityHalfColor );
	patternEditorNode.write_color( "noteVelocityZeroColor",
								 pColorTheme->m_patternEditor_noteVelocityZeroColor );
	patternEditorNode.write_color( "noteOffColor", pColorTheme->m_patternEditor_noteOffColor );

	patternEditorNode.write_color( "lineColor", pColorTheme->m_patternEditor_lineColor );
	patternEditorNode.write_color( "line1Color", pColorTheme->m_patternEditor_line1Color );
	patternEditorNode.write_color( "line2Color", pColorTheme->m_patternEditor_line2Color );
	patternEditorNode.write_color( "line3Color", pColorTheme->m_patternEditor_line3Color );
	patternEditorNode.write_color( "line4Color", pColorTheme->m_patternEditor_line4Color );
	patternEditorNode.write_color( "line5Color", pColorTheme->m_patternEditor_line5Color );

	XMLNode selectionNode = colorThemeNode.createNode( "selection" );
	selectionNode.write_color( "highlightColor", pColorTheme->m_selectionHighlightColor );
	selectionNode.write_color( "inactiveColor", pColorTheme->m_selectionInactiveColor );
	
	XMLNode paletteNode = colorThemeNode.createNode( "palette" );
	paletteNode.write_color( "windowColor", pColorTheme->m_windowColor );
	paletteNode.write_color( "windowTextColor", pColorTheme->m_windowTextColor );
	paletteNode.write_color( "baseColor", pColorTheme->m_baseColor );
	paletteNode.write_color( "alternateBaseColor", pColorTheme->m_alternateBaseColor );
	paletteNode.write_color( "textColor", pColorTheme->m_textColor );
	paletteNode.write_color( "buttonColor", pColorTheme->m_buttonColor );
	paletteNode.write_color( "buttonTextColor", pColorTheme->m_buttonTextColor );
	paletteNode.write_color( "lightColor", pColorTheme->m_lightColor );
	paletteNode.write_color( "midLightColor", pColorTheme->m_midLightColor );
	paletteNode.write_color( "midColor", pColorTheme->m_midColor );
	paletteNode.write_color( "darkColor", pColorTheme->m_darkColor );
	paletteNode.write_color( "shadowTextColor", pColorTheme->m_shadowTextColor );
	paletteNode.write_color( "highlightColor", pColorTheme->m_highlightColor );
	paletteNode.write_color( "highlightedTextColor", pColorTheme->m_highlightedTextColor );
	paletteNode.write_color( "toolTipBaseColor", pColorTheme->m_toolTipBaseColor );
	paletteNode.write_color( "toolTipTextColor", pColorTheme->m_toolTipTextColor );
	
	XMLNode widgetNode = colorThemeNode.createNode( "widget" );
	widgetNode.write_color( "accentColor", pColorTheme->m_accentColor );
	widgetNode.write_color( "accentTextColor", pColorTheme->m_accentTextColor );
	widgetNode.write_color( "widgetColor", pColorTheme->m_widgetColor );
	widgetNode.write_color( "widgetTextColor", pColorTheme->m_widgetTextColor );
	widgetNode.write_color( "buttonRedColor", pColorTheme->m_buttonRedColor );
	widgetNode.write_color( "buttonRedTextColor", pColorTheme->m_buttonRedTextColor );
	widgetNode.write_color( "spinBoxColor", pColorTheme->m_spinBoxColor );
	widgetNode.write_color( "spinBoxTextColor", pColorTheme->m_spinBoxTextColor );
	widgetNode.write_color( "playheadColor", pColorTheme->m_playheadColor );
	widgetNode.write_color( "cursorColor", pColorTheme->m_cursorColor );
}

void Theme::readColorTheme( XMLNode parent, std::shared_ptr<Theme> pTheme )
{
	auto pColorTheme = pTheme->getColorTheme();
	
	// SONG EDITOR
	XMLNode songEditorNode = parent.firstChildElement( "songEditor" );
	if ( ! songEditorNode.isNull() ) {
		pColorTheme->m_songEditor_backgroundColor =
			songEditorNode.read_color( "backgroundColor",
									   pColorTheme->m_songEditor_backgroundColor, false, false );
		pColorTheme->m_songEditor_alternateRowColor =
			songEditorNode.read_color( "alternateRowColor",
									   pColorTheme->m_songEditor_alternateRowColor, false, false );
		pColorTheme->m_songEditor_virtualRowColor =
			songEditorNode.read_color( "virtualRowColor",
									   pColorTheme->m_songEditor_virtualRowColor, false, false );
		pColorTheme->m_songEditor_selectedRowColor =
			songEditorNode.read_color( "selectedRowColor",
									   pColorTheme->m_songEditor_selectedRowColor, false, false );
		pColorTheme->m_songEditor_selectedRowTextColor =
			songEditorNode.read_color( "selectedRowTextColor",
									   pColorTheme->m_songEditor_selectedRowTextColor, false, false );
		pColorTheme->m_songEditor_lineColor =
			songEditorNode.read_color( "lineColor",
									   pColorTheme->m_songEditor_lineColor, false, false );
		pColorTheme->m_songEditor_textColor =
			songEditorNode.read_color( "textColor",
									   pColorTheme->m_songEditor_textColor, false, false );
		pColorTheme->m_songEditor_automationBackgroundColor =
			songEditorNode.read_color( "automationBackgroundColor",
									   pColorTheme->m_songEditor_automationBackgroundColor, false, false );
		pColorTheme->m_songEditor_automationLineColor =
			songEditorNode.read_color( "automationLineColor",
									   pColorTheme->m_songEditor_automationLineColor, false, false );
		pColorTheme->m_songEditor_automationNodeColor =
			songEditorNode.read_color( "automationNodeColor",
									   pColorTheme->m_songEditor_automationNodeColor, false, false );
		pColorTheme->m_songEditor_stackedModeOnColor =
			songEditorNode.read_color( "stackedModeOnColor",
									   pColorTheme->m_songEditor_stackedModeOnColor, false, false );
		pColorTheme->m_songEditor_stackedModeOnNextColor =
			songEditorNode.read_color( "stackedModeOnNextColor",
									   pColorTheme->m_songEditor_stackedModeOnNextColor, false, false );
		pColorTheme->m_songEditor_stackedModeOffNextColor =
			songEditorNode.read_color( "stackedModeOffNextColor",
									   pColorTheme->m_songEditor_stackedModeOffNextColor, false, false );
	} else {
		WARNINGLOG( "'songEditor' node not found" );
	}

	// PATTERN EDITOR
	XMLNode patternEditorNode = parent.firstChildElement( "patternEditor" );
	if ( ! patternEditorNode.isNull() ) {
		pColorTheme->m_patternEditor_backgroundColor =
			patternEditorNode.read_color( "backgroundColor",
										  pColorTheme->m_patternEditor_backgroundColor, false, false );
		pColorTheme->m_patternEditor_alternateRowColor =
			patternEditorNode.read_color( "alternateRowColor",
										  pColorTheme->m_patternEditor_alternateRowColor, false, false );
		pColorTheme->m_patternEditor_selectedRowColor =
			patternEditorNode.read_color( "selectedRowColor",
										  pColorTheme->m_patternEditor_selectedRowColor, false, false );
		pColorTheme->m_patternEditor_selectedRowTextColor =
			patternEditorNode.read_color( "selectedRowTextColor",
										  pColorTheme->m_patternEditor_selectedRowTextColor, false, false );
		pColorTheme->m_patternEditor_octaveRowColor =
			patternEditorNode.read_color( "octaveRowColor",
										  pColorTheme->m_patternEditor_octaveRowColor, false, false );
		pColorTheme->m_patternEditor_textColor =
			patternEditorNode.read_color( "textColor",
										  pColorTheme->m_patternEditor_textColor, false, false );
		pColorTheme->m_patternEditor_noteVelocityFullColor =
			patternEditorNode.read_color( "noteVelocityFullColor",
										  pColorTheme->m_patternEditor_noteVelocityFullColor, false, false );
		pColorTheme->m_patternEditor_noteVelocityDefaultColor =
			patternEditorNode.read_color( "noteVelocityDefaultColor",
										  pColorTheme->m_patternEditor_noteVelocityDefaultColor, false, false );
		pColorTheme->m_patternEditor_noteVelocityHalfColor =
			patternEditorNode.read_color( "noteVelocityHalfColor",
										  pColorTheme->m_patternEditor_noteVelocityHalfColor, false, false );
		pColorTheme->m_patternEditor_noteVelocityZeroColor =
			patternEditorNode.read_color( "noteVelocityZeroColor",
										  pColorTheme->m_patternEditor_noteVelocityZeroColor, false, false );
		pColorTheme->m_patternEditor_noteOffColor =
			patternEditorNode.read_color( "noteOffColor",
										  pColorTheme->m_patternEditor_noteOffColor, false, false );
		pColorTheme->m_patternEditor_lineColor =
			patternEditorNode.read_color( "lineColor",
										  pColorTheme->m_patternEditor_lineColor, false, false );
		pColorTheme->m_patternEditor_line1Color =
			patternEditorNode.read_color( "line1Color",
										  pColorTheme->m_patternEditor_line1Color, false, false );
		pColorTheme->m_patternEditor_line2Color =
			patternEditorNode.read_color( "line2Color",
										  pColorTheme->m_patternEditor_line2Color, false, false );
		pColorTheme->m_patternEditor_line3Color =
			patternEditorNode.read_color( "line3Color",
										  pColorTheme->m_patternEditor_line3Color, false, false );
		pColorTheme->m_patternEditor_line4Color =
			patternEditorNode.read_color( "line4Color",
										  pColorTheme->m_patternEditor_line4Color, false, false );
		pColorTheme->m_patternEditor_line5Color =
			patternEditorNode.read_color( "line5Color",
										  pColorTheme->m_patternEditor_line5Color, false, false );
	} else {
		WARNINGLOG( "'patternEditor' node not found" );
	}

	XMLNode selectionNode = parent.firstChildElement( "selection" );
	if ( ! selectionNode.isNull() ) {
		pColorTheme->m_selectionHighlightColor =
			selectionNode.read_color( "highlightColor",
									  pColorTheme->m_selectionHighlightColor, false, false );
		pColorTheme->m_selectionInactiveColor =
			selectionNode.read_color( "inactiveColor",
									  pColorTheme->m_selectionInactiveColor, false, false );
	} else {
		WARNINGLOG( "'selection' node not found" );
	}

	XMLNode paletteNode = parent.firstChildElement( "palette" );
	if ( ! paletteNode.isNull() ) {
		pColorTheme->m_windowColor =
			paletteNode.read_color( "windowColor",
									pColorTheme->m_windowColor, false, false );
		pColorTheme->m_windowTextColor =
			paletteNode.read_color( "windowTextColor",
									pColorTheme->m_windowTextColor, false, false );
		pColorTheme->m_baseColor =
			paletteNode.read_color( "baseColor",
									pColorTheme->m_baseColor, false, false );
		pColorTheme->m_alternateBaseColor =
			paletteNode.read_color( "alternateBaseColor",
									pColorTheme->m_alternateBaseColor, false, false );
		pColorTheme->m_textColor =
			paletteNode.read_color( "textColor",
									pColorTheme->m_textColor, false, false );
		pColorTheme->m_buttonColor =
			paletteNode.read_color( "buttonColor",
									pColorTheme->m_buttonColor, false, false );
		pColorTheme->m_buttonTextColor =
			paletteNode.read_color( "buttonTextColor",
									pColorTheme->m_buttonTextColor, false, false );
		pColorTheme->m_lightColor =
			paletteNode.read_color( "lightColor",
									pColorTheme->m_lightColor, false, false );
		pColorTheme->m_midLightColor =
			paletteNode.read_color( "midLightColor",
									pColorTheme->m_midLightColor, false, false );
		pColorTheme->m_midColor =
			paletteNode.read_color( "midColor",
									pColorTheme->m_midColor, false, false );
		pColorTheme->m_darkColor =
			paletteNode.read_color( "darkColor",
									pColorTheme->m_darkColor, false, false );
		pColorTheme->m_shadowTextColor =
			paletteNode.read_color( "shadowTextColor",
									pColorTheme->m_shadowTextColor, false, false );
		pColorTheme->m_highlightColor =
			paletteNode.read_color( "highlightColor",
									pColorTheme->m_highlightColor, false, false );
		pColorTheme->m_highlightedTextColor =
			paletteNode.read_color( "highlightedTextColor",
									pColorTheme->m_highlightedTextColor, false, false );
		pColorTheme->m_toolTipBaseColor =
			paletteNode.read_color( "toolTipBaseColor",
									pColorTheme->m_toolTipBaseColor, false, false );
		pColorTheme->m_toolTipTextColor =
			paletteNode.read_color( "toolTipTextColor",
									pColorTheme->m_toolTipTextColor, false, false );
	} else {
		WARNINGLOG( "'palette' node not found" );
	}

	XMLNode widgetNode = parent.firstChildElement( "widget" );
	if ( ! widgetNode.isNull() ) {
		pColorTheme->m_accentColor =
			widgetNode.read_color( "accentColor",
								   pColorTheme->m_accentColor, false, false );
		pColorTheme->m_accentTextColor =
			widgetNode.read_color( "accentTextColor",
								   pColorTheme->m_accentTextColor, false, false );
		pColorTheme->m_widgetColor =
			widgetNode.read_color( "widgetColor",
								   pColorTheme->m_widgetColor, false, false );
		pColorTheme->m_widgetTextColor =
			widgetNode.read_color( "widgetTextColor",
								   pColorTheme->m_widgetTextColor, false, false );
		pColorTheme->m_buttonRedColor =
			widgetNode.read_color( "buttonRedColor",
								   pColorTheme->m_buttonRedColor, false, false );
		pColorTheme->m_buttonRedTextColor =
			widgetNode.read_color( "buttonRedTextColor",
								   pColorTheme->m_buttonRedTextColor, false, false );
		pColorTheme->m_spinBoxColor =
			widgetNode.read_color( "spinBoxColor",
								   pColorTheme->m_spinBoxColor, false, false );
		pColorTheme->m_spinBoxTextColor =
			widgetNode.read_color( "spinBoxTextColor",
								   pColorTheme->m_spinBoxTextColor, false, false );
		pColorTheme->m_playheadColor =
			widgetNode.read_color( "playheadColor",
								   pColorTheme->m_playheadColor, false, false );
		pColorTheme->m_cursorColor =
			widgetNode.read_color( "cursorColor",
								   pColorTheme->m_cursorColor, false, false );
	} else {
		WARNINGLOG( "'widget' node not found" );
	}
}

std::shared_ptr<Theme> Theme::importTheme( const QString& sPath ) {
	if ( ! Filesystem::file_exists( sPath ) || ! Filesystem::file_readable( sPath ) ){
		return nullptr;
	}

	std::shared_ptr<Theme> pTheme = std::make_shared<Theme>(); 

	INFOLOG( QString( "Importing theme to %1" ).arg( sPath ) );

	XMLDoc doc;
	if ( ! doc.read( sPath, true ) ) {
		ERRORLOG( "Unable to load theme." );
		return nullptr;
	}
	
	XMLNode rootNode = doc.firstChildElement( "hydrogen_theme" );
	if ( rootNode.isNull() ) {
		ERRORLOG( "'hydrogen_theme' node not found" );
		return nullptr;
	}

	XMLNode colorThemeNode = rootNode.firstChildElement( "colorTheme" );
	if ( ! colorThemeNode.isNull() ) {
		readColorTheme( colorThemeNode, pTheme );
	} else {
		ERRORLOG( "'colorTheme' node not found" );
		return nullptr;
	}
	
	XMLNode interfaceNode = rootNode.firstChildElement( "interfaceTheme" );
	if ( interfaceNode.isNull() ) {
		ERRORLOG( "'interfaceTheme' node not found" );
		return nullptr;
	}
	pTheme->getInterfaceTheme()->m_layout =
		static_cast<InterfaceTheme::Layout>(
			interfaceNode.read_int( "defaultUILayout",
									static_cast<int>(InterfaceTheme::Layout::SinglePane),
									false, false ));
	pTheme->getInterfaceTheme()->m_scalingPolicy =
		static_cast<InterfaceTheme::ScalingPolicy>(
			interfaceNode.read_int( "uiScalingPolicy",
									static_cast<int>(InterfaceTheme::ScalingPolicy::Smaller),
									false, false ));
				
	// QT Style
	pTheme->getInterfaceTheme()->m_sQTStyle =
		interfaceNode.read_string( "QTStyle", "Fusion", false, false );

	if ( pTheme->getInterfaceTheme()->m_sQTStyle == "Plastique" ){
		pTheme->getInterfaceTheme()->m_sQTStyle = "Fusion";
	}
	pTheme->getInterfaceTheme()->m_iconColor =
		static_cast<InterfaceTheme::IconColor>(
			interfaceNode.read_int( "iconColor",
									static_cast<int>(InterfaceTheme::IconColor::Black),
									false, false));

	// Mixer falloff speed
	pTheme->getInterfaceTheme()->m_fMixerFalloffSpeed =
		interfaceNode.read_float( "mixer_falloff_speed",
								  InterfaceTheme::FALLOFF_NORMAL, false, false );

	//SongEditor coloring
	pTheme->getInterfaceTheme()->m_coloringMethod =
		static_cast<InterfaceTheme::ColoringMethod>(
			interfaceNode.read_int("SongEditor_ColoringMethod",
								   static_cast<int>(InterfaceTheme::ColoringMethod::Custom),
								   false, false ));
	std::vector<QColor> colors( pTheme->getInterfaceTheme()->m_nMaxPatternColors );
	for ( int ii = 0; ii < pTheme->getInterfaceTheme()->m_nMaxPatternColors; ii++ ) {
		colors[ ii ] = interfaceNode.read_color( QString( "SongEditor_pattern_color_%1" ).arg( ii ),
												 pTheme->getColorTheme()->m_accentColor,
												 false, false );
	}
	pTheme->getInterfaceTheme()->m_patternColors = colors;
	pTheme->getInterfaceTheme()->m_nVisiblePatternColors =
		interfaceNode.read_int( "SongEditor_visible_pattern_colors", 1, false, false );
	if ( pTheme->getInterfaceTheme()->m_nVisiblePatternColors > 50 ) {
		pTheme->getInterfaceTheme()->m_nVisiblePatternColors = 50;
	} else if ( pTheme->getInterfaceTheme()->m_nVisiblePatternColors < 0 ) {
		pTheme->getInterfaceTheme()->m_nVisiblePatternColors = 0;
	}									  
			
	XMLNode fontNode = rootNode.firstChildElement( "fontTheme" );
	if ( fontNode.isNull() ) {
		ERRORLOG( "'fontTheme' node not found" );
		return nullptr;
	}
	// Font fun
	pTheme->getFontTheme()->m_sApplicationFontFamily =
		fontNode.read_string( "application_font_family",
							  pTheme->getFontTheme()->m_sApplicationFontFamily, false, false );
	// The value defaults to m_sApplicationFontFamily on
	// purpose to provide backward compatibility.
	pTheme->getFontTheme()->m_sLevel2FontFamily =
		fontNode.read_string( "level2_font_family",
							  pTheme->getFontTheme()->m_sLevel2FontFamily, false, false );
	pTheme->getFontTheme()->m_sLevel3FontFamily =
		fontNode.read_string( "level3_font_family",
							  pTheme->getFontTheme()->m_sLevel3FontFamily, false, false );
	pTheme->getFontTheme()->m_fontSize =
		static_cast<FontTheme::FontSize>(
			fontNode.read_int( "font_size",
							   static_cast<int>(FontTheme::FontSize::Medium), false, false ) );

	return pTheme;
}

bool Theme::exportTheme( const QString& sPath, const std::shared_ptr<Theme> pTheme ) {

	INFOLOG( QString( "Exporting theme to %1" ).arg( sPath ) );

	XMLDoc doc;
	XMLNode rootNode = doc.set_root( "hydrogen_theme", "theme" );
	// hydrogen version
	rootNode.write_string( "version", QString( get_version().c_str() ) );
	
	writeColorTheme( &rootNode, pTheme );

	auto pInterfaceTheme = pTheme->getInterfaceTheme();
	XMLNode interfaceNode = rootNode.createNode( "interfaceTheme" );
	interfaceNode.write_int( "defaultUILayout",
							 static_cast<int>(pInterfaceTheme->m_layout) );
	interfaceNode.write_int( "uiScalingPolicy",
							 static_cast<int>(pInterfaceTheme->m_scalingPolicy) );
	interfaceNode.write_string( "QTStyle", pInterfaceTheme->m_sQTStyle );
	interfaceNode.write_int( "iconColor",
							 static_cast<int>(pInterfaceTheme->m_iconColor) );
	interfaceNode.write_float( "mixer_falloff_speed",
							   pInterfaceTheme->m_fMixerFalloffSpeed );
	interfaceNode.write_int( "SongEditor_ColoringMethod",
							 static_cast<int>(pInterfaceTheme->m_coloringMethod) );
	for ( int ii = 0; ii < pInterfaceTheme->m_nMaxPatternColors; ii++ ) {
		interfaceNode.write_color( QString( "SongEditor_pattern_color_%1" ).arg( ii ),
								   pInterfaceTheme->m_patternColors[ ii ] );
	}
	interfaceNode.write_int( "SongEditor_visible_pattern_colors",
							 pInterfaceTheme->m_nVisiblePatternColors );
	
	XMLNode fontNode = rootNode.createNode( "fontTheme" );
	fontNode.write_string( "application_font_family",
						   pTheme->getFontTheme()->m_sApplicationFontFamily );
	fontNode.write_string( "level2_font_family",
						   pTheme->getFontTheme()->m_sLevel2FontFamily );
	fontNode.write_string( "level3_font_family",
						   pTheme->getFontTheme()->m_sLevel3FontFamily );
	fontNode.write_int( "font_size",
						static_cast<int>(pTheme->getFontTheme()->m_fontSize) );

	return doc.write( sPath );
}	

}
