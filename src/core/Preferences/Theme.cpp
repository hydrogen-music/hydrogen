/*
 * Hydrogen
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "Theme.h"

namespace H2Core
{

ColorTheme::ColorTheme()
	: m_songEditor_backgroundColor( QColor(95, 101, 117) )
	, m_songEditor_alternateRowColor( QColor(128, 134, 152) )
	, m_songEditor_selectedRowColor( QColor(128, 134, 152) )
	, m_songEditor_lineColor( QColor(72, 76, 88) )
	, m_songEditor_textColor( QColor(196, 201, 214) )
	, m_patternEditor_backgroundColor( QColor(167, 168, 163) )
	, m_patternEditor_alternateRowColor( QColor(167, 168, 163) )
	, m_patternEditor_selectedRowColor( QColor(207, 208, 200) )
	, m_patternEditor_textColor( QColor(40, 40, 40) )
	, m_patternEditor_noteColor( QColor(40, 40, 40) )
	, m_patternEditor_lineColor( QColor(65, 65, 65) )
	, m_patternEditor_line1Color( QColor(75, 75, 75) )
	, m_patternEditor_line2Color( QColor(95, 95, 95) )
	, m_patternEditor_line3Color( QColor(115, 115, 115) )
	, m_patternEditor_line4Color( QColor(125, 125, 125) )
	, m_patternEditor_line5Color( QColor(135, 135, 135) )
	, m_selectionHighlightColor( QColor(0, 0, 255) )
	, m_selectionInactiveColor( QColor(85, 85, 85) )
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
	, m_spinBoxSelectionColor( QColor( 51, 74 , 100 ) )
	, m_spinBoxSelectionTextColor( QColor( 240, 240, 240 ) )
	, m_automationColor( QColor( 67, 96, 131 ) )
	, m_automationCircleColor( QColor( 255, 255, 255 ) )
	, m_accentColor( QColor( 67, 96, 131 ) )
	, m_accentTextColor( QColor( 255, 255, 255 ) )
{
}

ColorTheme::ColorTheme( const ColorTheme* pOther )
	: m_songEditor_backgroundColor( pOther->m_songEditor_backgroundColor )
	, m_songEditor_alternateRowColor( pOther->m_songEditor_alternateRowColor )
	, m_songEditor_selectedRowColor( pOther->m_songEditor_selectedRowColor )
	, m_songEditor_lineColor( pOther->m_songEditor_lineColor )
	, m_songEditor_textColor( pOther->m_songEditor_textColor )
	, m_patternEditor_backgroundColor( pOther->m_patternEditor_backgroundColor )
	, m_patternEditor_alternateRowColor( pOther->m_patternEditor_alternateRowColor )
	, m_patternEditor_selectedRowColor( pOther->m_patternEditor_selectedRowColor )
	, m_patternEditor_textColor( pOther->m_patternEditor_textColor )
	, m_patternEditor_noteColor( pOther->m_patternEditor_noteColor )
	, m_patternEditor_noteoffColor( pOther->m_patternEditor_noteoffColor )
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
	, m_spinBoxSelectionColor( pOther->m_spinBoxSelectionColor )
	, m_spinBoxSelectionTextColor( pOther->m_spinBoxSelectionTextColor )
	, m_automationColor( pOther->m_automationColor )
	, m_automationCircleColor( pOther->m_automationCircleColor )
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
	, m_nColoringMethod( 2 )
	, m_nVisiblePatternColors( 1 )
	, m_nMaxPatternColors( 50 ) {
	std::vector<QColor> m_patternColors( m_nMaxPatternColors );
	for ( int ii = 0; ii < m_nMaxPatternColors; ii++ ) {
		m_patternColors[ ii ] = QColor( 67, 96, 131 );
	}
}

InterfaceTheme::InterfaceTheme( const InterfaceTheme* pOther )
	: m_sQTStyle( pOther->m_sQTStyle )
	, m_fMixerFalloffSpeed( pOther->m_fMixerFalloffSpeed )
	, m_layout( pOther->m_layout )
	, m_scalingPolicy( pOther->m_scalingPolicy )
	, m_nColoringMethod( pOther->m_nColoringMethod )
	, m_nVisiblePatternColors( pOther->m_nVisiblePatternColors )
	, m_nMaxPatternColors( pOther->m_nMaxPatternColors ){
	std::vector<QColor> m_patternColors( m_nMaxPatternColors );
	for ( int ii = 0; ii < m_nMaxPatternColors; ii++ ) {
		m_patternColors[ ii ] = QColor( pOther->m_patternColors[ ii ] );
	}
}

FontTheme::FontTheme()
	: m_sApplicationFontFamily( "Lucida Grande" )
	, m_sLevel2FontFamily( "Lucida Grande" )
	, m_sLevel3FontFamily( "Lucida Grande" )
	, m_fontSize( FontTheme::FontSize::Normal ) {
}

FontTheme::FontTheme( const FontTheme* pOther )
	: m_sApplicationFontFamily( pOther->m_sApplicationFontFamily )
	, m_sLevel2FontFamily( pOther->m_sLevel2FontFamily )
	, m_sLevel3FontFamily( pOther->m_sLevel3FontFamily )
	, m_fontSize( pOther->m_fontSize ) {
}

Theme::Theme() {
	m_pColorTheme = new ColorTheme;
	m_pInterfaceTheme = new InterfaceTheme;
	m_pFontTheme = new FontTheme;
}

Theme::Theme( const Theme* pOther ) {
	m_pColorTheme = new ColorTheme( pOther->getColorTheme() );
	m_pInterfaceTheme = new InterfaceTheme( pOther->getInterfaceTheme() );
	m_pFontTheme = new FontTheme( pOther->getFontTheme() );
}

Theme::~Theme() {
	delete m_pColorTheme;
	delete m_pInterfaceTheme;
	delete m_pFontTheme;
}

}
