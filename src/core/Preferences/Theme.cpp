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
#include <fstream>

#include "Theme.h"

#include <core/LocalFileMng.h>
#include <core/Helpers/Filesystem.h>
#include "../Version.h"

namespace H2Core
{

ColorTheme::ColorTheme()
	: m_songEditor_backgroundColor( QColor( 128, 134, 152 ) )
	, m_songEditor_alternateRowColor( QColor( 112, 117, 133 ) )
	, m_songEditor_selectedRowColor( QColor( 149, 157, 178 ) )
	, m_songEditor_lineColor( QColor( 54, 57, 67 ) )
	, m_songEditor_textColor( QColor( 206, 211, 224 ) )
	, m_patternEditor_backgroundColor( QColor(167, 168, 163) )
	, m_patternEditor_alternateRowColor( QColor( 149, 150, 145 ) )
	, m_patternEditor_selectedRowColor( QColor( 207, 208, 200 ) )
	, m_patternEditor_textColor( QColor( 255, 255, 255 ) )
	, m_patternEditor_noteColor( QColor(40, 40, 40) )
	, m_patternEditor_lineColor( QColor(65, 65, 65) )
	, m_patternEditor_line1Color( QColor(75, 75, 75) )
	, m_patternEditor_line2Color( QColor(95, 95, 95) )
	, m_patternEditor_line3Color( QColor(115, 115, 115) )
	, m_patternEditor_line4Color( QColor(125, 125, 125) )
	, m_patternEditor_line5Color( QColor(135, 135, 135) )
	, m_selectionHighlightColor( QColor(255, 255, 255) )
	, m_selectionInactiveColor( QColor(73, 73, 73) )
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
	, m_automationColor( QColor( 67, 96, 131 ) )
	, m_automationCircleColor( QColor( 255, 255, 255 ) )
	, m_accentColor( QColor( 67, 96, 131 ) )
	, m_accentTextColor( QColor( 255, 255, 255 ) )
{
}

ColorTheme::ColorTheme( const std::shared_ptr<ColorTheme> pOther )
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
	, m_spinBoxColor( pOther->m_spinBoxColor )
	, m_spinBoxTextColor( pOther->m_spinBoxTextColor )
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
	, m_iconColor( InterfaceTheme::IconColor::Black )
	, m_coloringMethod( InterfaceTheme::ColoringMethod::Custom )
	, m_nVisiblePatternColors( 1 )
	, m_nMaxPatternColors( 50 ) {
	std::vector<QColor> m_patternColors( m_nMaxPatternColors );
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
	, m_fontSize( FontTheme::FontSize::Normal ) {
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
	DEBUGLOG("");
	m_pColorTheme->m_songEditor_backgroundColor = pOther->getColorTheme()->m_songEditor_backgroundColor;
	m_pColorTheme->m_songEditor_alternateRowColor = pOther->getColorTheme()->m_songEditor_alternateRowColor;
	m_pColorTheme->m_songEditor_selectedRowColor = pOther->getColorTheme()->m_songEditor_selectedRowColor;
	m_pColorTheme->m_songEditor_lineColor = pOther->getColorTheme()->m_songEditor_lineColor;
	m_pColorTheme->m_songEditor_textColor = pOther->getColorTheme()->m_songEditor_textColor;
	m_pColorTheme->m_patternEditor_backgroundColor = pOther->getColorTheme()->m_patternEditor_backgroundColor;
	m_pColorTheme->m_patternEditor_alternateRowColor = pOther->getColorTheme()->m_patternEditor_alternateRowColor;
	m_pColorTheme->m_patternEditor_selectedRowColor = pOther->getColorTheme()->m_patternEditor_selectedRowColor;
	m_pColorTheme->m_patternEditor_textColor = pOther->getColorTheme()->m_patternEditor_textColor;
	m_pColorTheme->m_patternEditor_noteColor = pOther->getColorTheme()->m_patternEditor_noteColor;
	m_pColorTheme->m_patternEditor_noteoffColor = pOther->getColorTheme()->m_patternEditor_noteoffColor;
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
	m_pColorTheme->m_automationColor = pOther->getColorTheme()->m_automationColor;
	m_pColorTheme->m_automationCircleColor = pOther->getColorTheme()->m_automationCircleColor;
		
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

void Theme::writeColorTheme( QDomNode* parent, std::shared_ptr<Theme> pTheme )
{
	QDomDocument doc;
	QDomNode node = doc.createElement( "colorTheme" );

	// SONG EDITOR
	QDomNode songEditorNode = doc.createElement( "songEditor" );
	LocalFileMng::writeXmlColor( songEditorNode, "backgroundColor", pTheme->getColorTheme()->m_songEditor_backgroundColor );
	LocalFileMng::writeXmlColor( songEditorNode, "alternateRowColor", pTheme->getColorTheme()->m_songEditor_alternateRowColor );
	LocalFileMng::writeXmlColor( songEditorNode, "selectedRowColor", pTheme->getColorTheme()->m_songEditor_selectedRowColor );
	LocalFileMng::writeXmlColor( songEditorNode, "lineColor", pTheme->getColorTheme()->m_songEditor_lineColor );
	LocalFileMng::writeXmlColor( songEditorNode, "textColor", pTheme->getColorTheme()->m_songEditor_textColor );
	node.appendChild( songEditorNode );

	// PATTERN EDITOR
	QDomNode patternEditorNode = doc.createElement( "patternEditor" );
	LocalFileMng::writeXmlColor( patternEditorNode, "backgroundColor", pTheme->getColorTheme()->m_patternEditor_backgroundColor );
	LocalFileMng::writeXmlColor( patternEditorNode, "alternateRowColor", pTheme->getColorTheme()->m_patternEditor_alternateRowColor );
	LocalFileMng::writeXmlColor( patternEditorNode, "selectedRowColor", pTheme->getColorTheme()->m_patternEditor_selectedRowColor );
	LocalFileMng::writeXmlColor( patternEditorNode, "textColor", pTheme->getColorTheme()->m_patternEditor_textColor );
	LocalFileMng::writeXmlColor( patternEditorNode, "noteColor", pTheme->getColorTheme()->m_patternEditor_noteColor );
	LocalFileMng::writeXmlColor( patternEditorNode, "noteoffColor", pTheme->getColorTheme()->m_patternEditor_noteoffColor );

	LocalFileMng::writeXmlColor( patternEditorNode, "lineColor", pTheme->getColorTheme()->m_patternEditor_lineColor );
	LocalFileMng::writeXmlColor( patternEditorNode, "line1Color", pTheme->getColorTheme()->m_patternEditor_line1Color );
	LocalFileMng::writeXmlColor( patternEditorNode, "line2Color", pTheme->getColorTheme()->m_patternEditor_line2Color );
	LocalFileMng::writeXmlColor( patternEditorNode, "line3Color", pTheme->getColorTheme()->m_patternEditor_line3Color );
	LocalFileMng::writeXmlColor( patternEditorNode, "line4Color", pTheme->getColorTheme()->m_patternEditor_line4Color );
	LocalFileMng::writeXmlColor( patternEditorNode, "line5Color", pTheme->getColorTheme()->m_patternEditor_line5Color );
	node.appendChild( patternEditorNode );

	QDomNode selectionNode = doc.createElement( "selection" );
	LocalFileMng::writeXmlColor( selectionNode, "highlightColor", pTheme->getColorTheme()->m_selectionHighlightColor );
	LocalFileMng::writeXmlColor( selectionNode, "inactiveColor", pTheme->getColorTheme()->m_selectionInactiveColor );
	node.appendChild( selectionNode );
	
	QDomNode paletteNode = doc.createElement( "palette" );
	LocalFileMng::writeXmlColor( paletteNode, "windowColor", pTheme->getColorTheme()->m_windowColor );
	LocalFileMng::writeXmlColor( paletteNode, "windowTextColor", pTheme->getColorTheme()->m_windowTextColor );
	LocalFileMng::writeXmlColor( paletteNode, "baseColor", pTheme->getColorTheme()->m_baseColor );
	LocalFileMng::writeXmlColor( paletteNode, "alternateBaseColor", pTheme->getColorTheme()->m_alternateBaseColor );
	LocalFileMng::writeXmlColor( paletteNode, "textColor", pTheme->getColorTheme()->m_textColor );
	LocalFileMng::writeXmlColor( paletteNode, "buttonColor", pTheme->getColorTheme()->m_buttonColor );
	LocalFileMng::writeXmlColor( paletteNode, "buttonTextColor", pTheme->getColorTheme()->m_buttonTextColor );
	LocalFileMng::writeXmlColor( paletteNode, "lightColor", pTheme->getColorTheme()->m_lightColor );
	LocalFileMng::writeXmlColor( paletteNode, "midLightColor", pTheme->getColorTheme()->m_midLightColor );
	LocalFileMng::writeXmlColor( paletteNode, "midColor", pTheme->getColorTheme()->m_midColor );
	LocalFileMng::writeXmlColor( paletteNode, "darkColor", pTheme->getColorTheme()->m_darkColor );
	LocalFileMng::writeXmlColor( paletteNode, "shadowTextColor", pTheme->getColorTheme()->m_shadowTextColor );
	LocalFileMng::writeXmlColor( paletteNode, "highlightColor", pTheme->getColorTheme()->m_highlightColor );
	LocalFileMng::writeXmlColor( paletteNode, "highlightedTextColor", pTheme->getColorTheme()->m_highlightedTextColor );
	LocalFileMng::writeXmlColor( paletteNode, "toolTipBaseColor", pTheme->getColorTheme()->m_toolTipBaseColor );
	LocalFileMng::writeXmlColor( paletteNode, "toolTipTextColor", pTheme->getColorTheme()->m_toolTipTextColor );
	node.appendChild( paletteNode );
	
	QDomNode widgetNode = doc.createElement( "widget" );
	LocalFileMng::writeXmlColor( widgetNode, "accentColor", pTheme->getColorTheme()->m_accentColor );
	LocalFileMng::writeXmlColor( widgetNode, "accentTextColor", pTheme->getColorTheme()->m_accentTextColor );
	LocalFileMng::writeXmlColor( widgetNode, "widgetColor", pTheme->getColorTheme()->m_widgetColor );
	LocalFileMng::writeXmlColor( widgetNode, "widgetTextColor", pTheme->getColorTheme()->m_widgetTextColor );
	LocalFileMng::writeXmlColor( widgetNode, "buttonRedColor", pTheme->getColorTheme()->m_buttonRedColor );
	LocalFileMng::writeXmlColor( widgetNode, "buttonRedTextColor", pTheme->getColorTheme()->m_buttonRedTextColor );
	LocalFileMng::writeXmlColor( widgetNode, "spinBoxColor", pTheme->getColorTheme()->m_spinBoxColor );
	LocalFileMng::writeXmlColor( widgetNode, "spinBoxTextColor", pTheme->getColorTheme()->m_spinBoxTextColor );
	LocalFileMng::writeXmlColor( widgetNode, "automationColor", pTheme->getColorTheme()->m_automationColor );
	LocalFileMng::writeXmlColor( widgetNode, "automationCircleColor", pTheme->getColorTheme()->m_automationCircleColor );
	node.appendChild( widgetNode );
	
	parent->appendChild( node );
}

void Theme::readColorTheme( QDomNode parent, std::shared_ptr<Theme> pTheme )
{
	// SONG EDITOR
	QDomNode pSongEditorNode = parent.firstChildElement( "songEditor" );
	if ( !pSongEditorNode.isNull() ) {
		pTheme->getColorTheme()->m_songEditor_backgroundColor = LocalFileMng::readXmlColor( pSongEditorNode, "backgroundColor", pTheme->getColorTheme()->m_songEditor_backgroundColor );
		pTheme->getColorTheme()->m_songEditor_alternateRowColor = LocalFileMng::readXmlColor( pSongEditorNode, "alternateRowColor", pTheme->getColorTheme()->m_songEditor_alternateRowColor );
		pTheme->getColorTheme()->m_songEditor_selectedRowColor = LocalFileMng::readXmlColor( pSongEditorNode, "selectedRowColor", pTheme->getColorTheme()->m_songEditor_selectedRowColor );
		pTheme->getColorTheme()->m_songEditor_lineColor = LocalFileMng::readXmlColor( pSongEditorNode, "lineColor", pTheme->getColorTheme()->m_songEditor_lineColor );
		pTheme->getColorTheme()->m_songEditor_textColor = LocalFileMng::readXmlColor( pSongEditorNode, "textColor", pTheme->getColorTheme()->m_songEditor_textColor );
	} else {
		WARNINGLOG( "songEditor node not found" );
	}

	// PATTERN EDITOR
	QDomNode pPatternEditorNode = parent.firstChildElement( "patternEditor" );
	if ( !pPatternEditorNode.isNull() ) {
		pTheme->getColorTheme()->m_patternEditor_backgroundColor = LocalFileMng::readXmlColor( pPatternEditorNode, "backgroundColor", pTheme->getColorTheme()->m_patternEditor_backgroundColor );
		pTheme->getColorTheme()->m_patternEditor_alternateRowColor = LocalFileMng::readXmlColor( pPatternEditorNode, "alternateRowColor", pTheme->getColorTheme()->m_patternEditor_alternateRowColor );
		pTheme->getColorTheme()->m_patternEditor_selectedRowColor = LocalFileMng::readXmlColor( pPatternEditorNode, "selectedRowColor", pTheme->getColorTheme()->m_patternEditor_selectedRowColor );
		pTheme->getColorTheme()->m_patternEditor_textColor = LocalFileMng::readXmlColor( pPatternEditorNode, "textColor", pTheme->getColorTheme()->m_patternEditor_textColor );
		pTheme->getColorTheme()->m_patternEditor_noteColor = LocalFileMng::readXmlColor( pPatternEditorNode, "noteColor", pTheme->getColorTheme()->m_patternEditor_noteColor );
		pTheme->getColorTheme()->m_patternEditor_noteoffColor = LocalFileMng::readXmlColor( pPatternEditorNode, "noteoffColor", pTheme->getColorTheme()->m_patternEditor_noteoffColor );
		pTheme->getColorTheme()->m_patternEditor_lineColor = LocalFileMng::readXmlColor( pPatternEditorNode, "lineColor", pTheme->getColorTheme()->m_patternEditor_lineColor );
		pTheme->getColorTheme()->m_patternEditor_line1Color = LocalFileMng::readXmlColor( pPatternEditorNode, "line1Color", pTheme->getColorTheme()->m_patternEditor_line1Color );
		pTheme->getColorTheme()->m_patternEditor_line2Color = LocalFileMng::readXmlColor( pPatternEditorNode, "line2Color", pTheme->getColorTheme()->m_patternEditor_line2Color );
		pTheme->getColorTheme()->m_patternEditor_line3Color = LocalFileMng::readXmlColor( pPatternEditorNode, "line3Color", pTheme->getColorTheme()->m_patternEditor_line3Color );
		pTheme->getColorTheme()->m_patternEditor_line4Color = LocalFileMng::readXmlColor( pPatternEditorNode, "line4Color", pTheme->getColorTheme()->m_patternEditor_line4Color );
		pTheme->getColorTheme()->m_patternEditor_line5Color = LocalFileMng::readXmlColor( pPatternEditorNode, "line5Color", pTheme->getColorTheme()->m_patternEditor_line5Color );
	} else {
		WARNINGLOG( "patternEditor node not found" );
	}

	QDomNode pSelectionNode = parent.firstChildElement( "selection" );
	if ( !pSelectionNode.isNull() ) {
		pTheme->getColorTheme()->m_selectionHighlightColor = LocalFileMng::readXmlColor( pSelectionNode, "highlightColor", pTheme->getColorTheme()->m_selectionHighlightColor );
		pTheme->getColorTheme()->m_selectionInactiveColor = LocalFileMng::readXmlColor( pSelectionNode, "inactiveColor", pTheme->getColorTheme()->m_selectionInactiveColor );
	} else {
		WARNINGLOG( "selection node not found" );
	}

	QDomNode pPaletteNode = parent.firstChildElement( "palette" );
	if ( !pPaletteNode.isNull() ) {
		pTheme->getColorTheme()->m_windowColor = LocalFileMng::readXmlColor( pPaletteNode, "windowColor", pTheme->getColorTheme()->m_windowColor );
		pTheme->getColorTheme()->m_windowTextColor = LocalFileMng::readXmlColor( pPaletteNode, "windowTextColor", pTheme->getColorTheme()->m_windowTextColor );
		pTheme->getColorTheme()->m_baseColor = LocalFileMng::readXmlColor( pPaletteNode, "baseColor", pTheme->getColorTheme()->m_baseColor );
		pTheme->getColorTheme()->m_alternateBaseColor = LocalFileMng::readXmlColor( pPaletteNode, "alternateBaseColor", pTheme->getColorTheme()->m_alternateBaseColor );
		pTheme->getColorTheme()->m_textColor = LocalFileMng::readXmlColor( pPaletteNode, "textColor", pTheme->getColorTheme()->m_textColor );
		pTheme->getColorTheme()->m_buttonColor = LocalFileMng::readXmlColor( pPaletteNode, "buttonColor", pTheme->getColorTheme()->m_buttonColor );
		pTheme->getColorTheme()->m_buttonTextColor = LocalFileMng::readXmlColor( pPaletteNode, "buttonTextColor", pTheme->getColorTheme()->m_buttonTextColor );
		pTheme->getColorTheme()->m_lightColor = LocalFileMng::readXmlColor( pPaletteNode, "lightColor", pTheme->getColorTheme()->m_lightColor );
		pTheme->getColorTheme()->m_midLightColor = LocalFileMng::readXmlColor( pPaletteNode, "midLightColor", pTheme->getColorTheme()->m_midLightColor );
		pTheme->getColorTheme()->m_midColor = LocalFileMng::readXmlColor( pPaletteNode, "midColor", pTheme->getColorTheme()->m_midColor );
		pTheme->getColorTheme()->m_darkColor = LocalFileMng::readXmlColor( pPaletteNode, "darkColor", pTheme->getColorTheme()->m_darkColor );
		pTheme->getColorTheme()->m_shadowTextColor = LocalFileMng::readXmlColor( pPaletteNode, "shadowTextColor", pTheme->getColorTheme()->m_shadowTextColor );
		pTheme->getColorTheme()->m_highlightColor = LocalFileMng::readXmlColor( pPaletteNode, "highlightColor", pTheme->getColorTheme()->m_highlightColor );
		pTheme->getColorTheme()->m_highlightedTextColor = LocalFileMng::readXmlColor( pPaletteNode, "highlightedTextColor", pTheme->getColorTheme()->m_highlightedTextColor );
		pTheme->getColorTheme()->m_toolTipBaseColor = LocalFileMng::readXmlColor( pPaletteNode, "toolTipBaseColor", pTheme->getColorTheme()->m_toolTipBaseColor );
		pTheme->getColorTheme()->m_toolTipTextColor = LocalFileMng::readXmlColor( pPaletteNode, "toolTipTextColor", pTheme->getColorTheme()->m_toolTipTextColor );
	} else {
		WARNINGLOG( "palette node not found" );
	}

	QDomNode pWidgetNode = parent.firstChildElement( "widget" );
	if ( !pWidgetNode.isNull() ) {
		pTheme->getColorTheme()->m_accentColor = LocalFileMng::readXmlColor( pWidgetNode, "accentColor", pTheme->getColorTheme()->m_accentColor );
		pTheme->getColorTheme()->m_accentTextColor = LocalFileMng::readXmlColor( pWidgetNode, "accentTextColor", pTheme->getColorTheme()->m_accentTextColor );
		pTheme->getColorTheme()->m_widgetColor = LocalFileMng::readXmlColor( pWidgetNode, "widgetColor", pTheme->getColorTheme()->m_widgetColor );
		pTheme->getColorTheme()->m_widgetTextColor = LocalFileMng::readXmlColor( pWidgetNode, "widgetTextColor", pTheme->getColorTheme()->m_widgetTextColor );
		pTheme->getColorTheme()->m_buttonRedColor = LocalFileMng::readXmlColor( pWidgetNode, "buttonRedColor", pTheme->getColorTheme()->m_buttonRedColor );
		pTheme->getColorTheme()->m_buttonRedTextColor = LocalFileMng::readXmlColor( pWidgetNode, "buttonRedTextColor", pTheme->getColorTheme()->m_buttonRedTextColor );
		pTheme->getColorTheme()->m_spinBoxColor = LocalFileMng::readXmlColor( pWidgetNode, "spinBoxColor", pTheme->getColorTheme()->m_spinBoxColor );
		pTheme->getColorTheme()->m_spinBoxTextColor = LocalFileMng::readXmlColor( pWidgetNode, "spinBoxTextColor", pTheme->getColorTheme()->m_spinBoxTextColor );
		pTheme->getColorTheme()->m_automationColor = LocalFileMng::readXmlColor( pWidgetNode, "automationColor", pTheme->getColorTheme()->m_automationColor );
		pTheme->getColorTheme()->m_automationCircleColor = LocalFileMng::readXmlColor( pWidgetNode, "automationCircleColor", pTheme->getColorTheme()->m_automationCircleColor );
	} else {
		WARNINGLOG( "widget node not found" );
	}
}

std::shared_ptr<Theme> Theme::importTheme( const QString& sPath ) {
	if ( ! Filesystem::file_exists( sPath ) || ! Filesystem::file_readable( sPath ) ){
		return nullptr;
	}

	std::shared_ptr<Theme> pTheme = std::make_shared<Theme>(); 

	INFOLOG( QString( "Importing theme to %1" ).arg( sPath ) );

	QDomDocument doc = LocalFileMng::openXmlDocument( sPath );
	QDomNode rootNode = doc.firstChildElement( "hydrogen_theme" );

	QDomNode colorThemeNode = rootNode.firstChildElement( "colorTheme" );
	if ( !colorThemeNode.isNull() ) {
		readColorTheme( colorThemeNode, pTheme );
	} else {
		ERRORLOG( "'colorTheme' node not found" );
		return nullptr;
	}

	if ( !rootNode.isNull() ) {
		QDomNode interfaceNode = rootNode.firstChildElement( "interfaceTheme" );
		if ( !interfaceNode.isNull() ) {
			pTheme->getInterfaceTheme()->m_layout = static_cast<InterfaceTheme::Layout>(LocalFileMng::readXmlInt( interfaceNode, "defaultUILayout",
																							  static_cast<int>(InterfaceTheme::Layout::SinglePane) ));
			pTheme->getInterfaceTheme()->m_scalingPolicy = static_cast<InterfaceTheme::ScalingPolicy>(LocalFileMng::readXmlInt( interfaceNode, "uiScalingPolicy", static_cast<int>(InterfaceTheme::ScalingPolicy::Smaller) ));
				
			// QT Style
			pTheme->getInterfaceTheme()->m_sQTStyle = LocalFileMng::readXmlString( interfaceNode, "QTStyle", "Fusion", true );

			if ( pTheme->getInterfaceTheme()->m_sQTStyle == "Plastique" ){
				pTheme->getInterfaceTheme()->m_sQTStyle = "Fusion";
			}
			pTheme->getInterfaceTheme()->m_iconColor = static_cast<InterfaceTheme::IconColor>(LocalFileMng::readXmlInt( interfaceNode, "iconColor", static_cast<int>(InterfaceTheme::IconColor::Black) ));

			// Mixer falloff speed
			pTheme->getInterfaceTheme()->m_fMixerFalloffSpeed = LocalFileMng::readXmlFloat( interfaceNode, "mixer_falloff_speed",
																							InterfaceTheme::FALLOFF_NORMAL );

			//SongEditor coloring
			pTheme->getInterfaceTheme()->m_coloringMethod =
				static_cast<InterfaceTheme::ColoringMethod>(LocalFileMng::readXmlInt( interfaceNode,
																					  "SongEditor_ColoringMethod",
																					  static_cast<int>(InterfaceTheme::ColoringMethod::Custom) ));
			std::vector<QColor> colors( pTheme->getInterfaceTheme()->m_nMaxPatternColors );
			for ( int ii = 0; ii < pTheme->getInterfaceTheme()->m_nMaxPatternColors; ii++ ) {
				colors[ ii ] = LocalFileMng::readXmlColor( interfaceNode, QString( "SongEditor_pattern_color_%1" ).arg( ii ),
														   pTheme->getColorTheme()->m_accentColor );
			}
			pTheme->getInterfaceTheme()->m_patternColors = colors;
			pTheme->getInterfaceTheme()->m_nVisiblePatternColors = LocalFileMng::readXmlInt( interfaceNode, "SongEditor_visible_pattern_colors", 1 );
			if ( pTheme->getInterfaceTheme()->m_nVisiblePatternColors > 50 ) {
				pTheme->getInterfaceTheme()->m_nVisiblePatternColors = 50;
			} else if ( pTheme->getInterfaceTheme()->m_nVisiblePatternColors < 0 ) {
				pTheme->getInterfaceTheme()->m_nVisiblePatternColors = 0;
			}									  
		} else {
			ERRORLOG( "'interfaceTheme' node not found" );
			return nullptr;
		}
			
		QDomNode fontNode = rootNode.firstChildElement( "fontTheme" );
		if ( !fontNode.isNull() ) {
			// Font fun
			pTheme->getFontTheme()->m_sApplicationFontFamily = LocalFileMng::readXmlString( fontNode, "application_font_family", pTheme->getFontTheme()->m_sApplicationFontFamily );
			// The value defaults to m_sApplicationFontFamily on
			// purpose to provide backward compatibility.
			pTheme->getFontTheme()->m_sLevel2FontFamily = LocalFileMng::readXmlString( fontNode, "level2_font_family", pTheme->getFontTheme()->m_sLevel2FontFamily );
			pTheme->getFontTheme()->m_sLevel3FontFamily = LocalFileMng::readXmlString( fontNode, "level3_font_family", pTheme->getFontTheme()->m_sLevel3FontFamily );
			pTheme->getFontTheme()->m_fontSize = static_cast<FontTheme::FontSize>( LocalFileMng::readXmlInt( fontNode, "font_size",
																					 static_cast<int>(FontTheme::FontSize::Normal) ) );

		} else {
			ERRORLOG( "'fontTheme' node not found" );
			return nullptr;
		}
	} else {
		ERRORLOG( "'hydrogen_theme' node not found" );
		return nullptr;
	}

	return pTheme;
}

void Theme::exportTheme( const QString& sPath, const std::shared_ptr<Theme> pTheme ) {

	INFOLOG( QString( "Exporting theme to %1" ).arg( sPath ) );

	QDomDocument doc;
	QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild( header );

	QDomNode rootNode = doc.createElement( "hydrogen_theme" );
	// hydrogen version
	LocalFileMng::writeXmlString( rootNode, "version", QString( get_version().c_str() ) );
	
	writeColorTheme( &rootNode, pTheme );
	
	QDomNode interfaceNode = doc.createElement( "interfaceTheme" );
	LocalFileMng::writeXmlString( interfaceNode, "defaultUILayout",
								  QString::number( static_cast<int>(pTheme->getInterfaceTheme()->m_layout) ) );
	LocalFileMng::writeXmlString( interfaceNode, "uiScalingPolicy",
								  QString::number( static_cast<int>(pTheme->getInterfaceTheme()->m_scalingPolicy) ) );
	LocalFileMng::writeXmlString( interfaceNode, "QTStyle",
								  pTheme->getInterfaceTheme()->m_sQTStyle );
	LocalFileMng::writeXmlString( interfaceNode, "iconColor",
								  QString::number( static_cast<int>(pTheme->getInterfaceTheme()->m_iconColor) ) );
	LocalFileMng::writeXmlString( interfaceNode, "mixer_falloff_speed",
								  QString("%1").arg( pTheme->getInterfaceTheme()->m_fMixerFalloffSpeed ) );
	LocalFileMng::writeXmlString( interfaceNode, "SongEditor_ColoringMethod",
								  QString::number( static_cast<int>(pTheme->getInterfaceTheme()->m_coloringMethod) ) );
	for ( int ii = 0; ii < pTheme->getInterfaceTheme()->m_nMaxPatternColors; ii++ ) {
		LocalFileMng::writeXmlColor( interfaceNode,
									 QString( "SongEditor_pattern_color_%1" ).arg( ii ),
									 pTheme->getInterfaceTheme()->m_patternColors[ ii ] );
	}
	LocalFileMng::writeXmlString( interfaceNode, "SongEditor_visible_pattern_colors",
								  QString::number( pTheme->getInterfaceTheme()->m_nVisiblePatternColors ) );
	rootNode.appendChild( interfaceNode );
	
	QDomNode fontNode = doc.createElement( "fontTheme" );
	LocalFileMng::writeXmlString( fontNode, "application_font_family",
								  pTheme->getFontTheme()->m_sApplicationFontFamily );
	LocalFileMng::writeXmlString( fontNode, "level2_font_family",
								  pTheme->getFontTheme()->m_sLevel2FontFamily );
	LocalFileMng::writeXmlString( fontNode, "level3_font_family",
								  pTheme->getFontTheme()->m_sLevel3FontFamily );
	LocalFileMng::writeXmlString( fontNode, "font_size",
								  QString::number( static_cast<int>(pTheme->getFontTheme()->m_fontSize) ) );
	rootNode.appendChild( fontNode );
	doc.appendChild( rootNode );

	QFile file( sPath );
	if ( !file.open(QIODevice::WriteOnly) ) {
		ERRORLOG( "Unable to export theme" );
		return;
	}

	QTextStream TextStream( &file );
	doc.save( TextStream, 1 );

	file.close();
}	

}
