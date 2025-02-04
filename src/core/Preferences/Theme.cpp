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
	, m_patternEditor_octaveRowColor( QColor( 180, 181, 173 ) )
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
	, m_patternEditor_instrumentRowColor( QColor( 128, 134, 152 ) )
	, m_patternEditor_instrumentRowTextColor( QColor( 240, 240, 240 ) )
	, m_patternEditor_instrumentAlternateRowColor( QColor( 106, 111, 126 ) )
	, m_patternEditor_instrumentSelectedRowColor( QColor( 149, 157, 178 ) )
	, m_patternEditor_instrumentSelectedRowTextColor( QColor( 0, 0, 0 ) )
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

void ColorTheme::saveTo( XMLNode& parent ) const {

	XMLNode colorThemeNode = parent.createNode( "colorTheme" );

	// SONG EDITOR
	XMLNode songEditorNode = colorThemeNode.createNode( "songEditor" );
	songEditorNode.write_color( "backgroundColor", m_songEditor_backgroundColor );
	songEditorNode.write_color( "alternateRowColor", m_songEditor_alternateRowColor );
	songEditorNode.write_color( "virtualRowColor", m_songEditor_virtualRowColor );
	songEditorNode.write_color( "selectedRowColor",
								 m_songEditor_selectedRowColor );
	songEditorNode.write_color( "selectedRowTextColor",
								 m_songEditor_selectedRowTextColor );
	songEditorNode.write_color( "lineColor", m_songEditor_lineColor );
	songEditorNode.write_color( "textColor", m_songEditor_textColor );
	songEditorNode.write_color( "automationBackgroundColor",
								 m_songEditor_automationBackgroundColor );
	songEditorNode.write_color( "automationLineColor",
								 m_songEditor_automationLineColor );
	songEditorNode.write_color( "automationNodeColor",
								 m_songEditor_automationNodeColor );
	songEditorNode.write_color( "stackedModeOnColor",
								 m_songEditor_stackedModeOnColor );
	songEditorNode.write_color( "stackedModeOnNextColor",
								 m_songEditor_stackedModeOnNextColor );
	songEditorNode.write_color( "stackedModeOffNextColor",
								 m_songEditor_stackedModeOffNextColor );

	// PATTERN EDITOR
	XMLNode patternEditorNode = colorThemeNode.createNode( "patternEditor" );
	patternEditorNode.write_color( "backgroundColor", m_patternEditor_backgroundColor );
	patternEditorNode.write_color( "alternateRowColor", m_patternEditor_alternateRowColor );
	patternEditorNode.write_color( "selectedRowColor",
								 m_patternEditor_selectedRowColor );
	patternEditorNode.write_color( "selectedRowTextColor",
								 m_patternEditor_selectedRowTextColor );
	patternEditorNode.write_color( "octaveRowColor",
								 m_patternEditor_octaveRowColor );
	patternEditorNode.write_color( "textColor", m_patternEditor_textColor );
	patternEditorNode.write_color( "noteVelocityFullColor",
								 m_patternEditor_noteVelocityFullColor );
	patternEditorNode.write_color( "noteVelocityDefaultColor",
								 m_patternEditor_noteVelocityDefaultColor );
	patternEditorNode.write_color( "noteVelocityHalfColor",
								 m_patternEditor_noteVelocityHalfColor );
	patternEditorNode.write_color( "noteVelocityZeroColor",
								 m_patternEditor_noteVelocityZeroColor );
	patternEditorNode.write_color( "noteOffColor", m_patternEditor_noteOffColor );

	patternEditorNode.write_color( "lineColor", m_patternEditor_lineColor );
	patternEditorNode.write_color( "line1Color", m_patternEditor_line1Color );
	patternEditorNode.write_color( "line2Color", m_patternEditor_line2Color );
	patternEditorNode.write_color( "line3Color", m_patternEditor_line3Color );
	patternEditorNode.write_color( "line4Color", m_patternEditor_line4Color );
	patternEditorNode.write_color( "line5Color", m_patternEditor_line5Color );
	patternEditorNode.write_color(
		"instrumentRowColor", m_patternEditor_instrumentRowColor );
	patternEditorNode.write_color(
		"instrumentRowTextColor", m_patternEditor_instrumentRowTextColor );
	patternEditorNode.write_color(
		"instrumentAlternateRowColor", m_patternEditor_instrumentAlternateRowColor );
	patternEditorNode.write_color(
		"instrumentSelectedRowColor", m_patternEditor_instrumentSelectedRowColor );
	patternEditorNode.write_color(
		"instrumentSelectedRowTextColor", m_patternEditor_instrumentSelectedRowTextColor );

	XMLNode selectionNode = colorThemeNode.createNode( "selection" );
	selectionNode.write_color( "highlightColor", m_selectionHighlightColor );
	selectionNode.write_color( "inactiveColor", m_selectionInactiveColor );

	XMLNode paletteNode = colorThemeNode.createNode( "palette" );
	paletteNode.write_color( "windowColor", m_windowColor );
	paletteNode.write_color( "windowTextColor", m_windowTextColor );
	paletteNode.write_color( "baseColor", m_baseColor );
	paletteNode.write_color( "alternateBaseColor", m_alternateBaseColor );
	paletteNode.write_color( "textColor", m_textColor );
	paletteNode.write_color( "buttonColor", m_buttonColor );
	paletteNode.write_color( "buttonTextColor", m_buttonTextColor );
	paletteNode.write_color( "lightColor", m_lightColor );
	paletteNode.write_color( "midLightColor", m_midLightColor );
	paletteNode.write_color( "midColor", m_midColor );
	paletteNode.write_color( "darkColor", m_darkColor );
	paletteNode.write_color( "shadowTextColor", m_shadowTextColor );
	paletteNode.write_color( "highlightColor", m_highlightColor );
	paletteNode.write_color( "highlightedTextColor", m_highlightedTextColor );
	paletteNode.write_color( "toolTipBaseColor", m_toolTipBaseColor );
	paletteNode.write_color( "toolTipTextColor", m_toolTipTextColor );

	XMLNode widgetNode = colorThemeNode.createNode( "widget" );
	widgetNode.write_color( "accentColor", m_accentColor );
	widgetNode.write_color( "accentTextColor", m_accentTextColor );
	widgetNode.write_color( "widgetColor", m_widgetColor );
	widgetNode.write_color( "widgetTextColor", m_widgetTextColor );
	widgetNode.write_color( "buttonRedColor", m_buttonRedColor );
	widgetNode.write_color( "buttonRedTextColor", m_buttonRedTextColor );
	widgetNode.write_color( "spinBoxColor", m_spinBoxColor );
	widgetNode.write_color( "spinBoxTextColor", m_spinBoxTextColor );
	widgetNode.write_color( "playheadColor", m_playheadColor );
	widgetNode.write_color( "cursorColor", m_cursorColor );
}

ColorTheme ColorTheme::loadFrom( const XMLNode& parent, const bool bSilent ) {
	auto colorTheme = ColorTheme();

	// SONG EDITOR
	const XMLNode songEditorNode = parent.firstChildElement( "songEditor" );
	if ( ! songEditorNode.isNull() ) {
		colorTheme.m_songEditor_backgroundColor = songEditorNode.read_color(
			"backgroundColor",
			colorTheme.m_songEditor_backgroundColor, false, false, bSilent );
		colorTheme.m_songEditor_alternateRowColor = songEditorNode.read_color(
			"alternateRowColor",
			colorTheme.m_songEditor_alternateRowColor, false, false, bSilent );
		colorTheme.m_songEditor_virtualRowColor = songEditorNode.read_color(
			"virtualRowColor",
			colorTheme.m_songEditor_virtualRowColor, false, false, bSilent );
		colorTheme.m_songEditor_selectedRowColor = songEditorNode.read_color(
			"selectedRowColor",
			colorTheme.m_songEditor_selectedRowColor, false, false, bSilent );
		colorTheme.m_songEditor_selectedRowTextColor = songEditorNode.read_color(
			"selectedRowTextColor",
			colorTheme.m_songEditor_selectedRowTextColor, false, false, bSilent );
		colorTheme.m_songEditor_lineColor = songEditorNode.read_color(
			"lineColor",
			colorTheme.m_songEditor_lineColor, false, false, bSilent );
		colorTheme.m_songEditor_textColor = songEditorNode.read_color(
			"textColor",
			colorTheme.m_songEditor_textColor, false, false, bSilent );
		colorTheme.m_songEditor_automationBackgroundColor =
			songEditorNode.read_color(
				"automationBackgroundColor",
				colorTheme.m_songEditor_automationBackgroundColor, false, false,
				bSilent );
		colorTheme.m_songEditor_automationLineColor =
			songEditorNode.read_color(
				"automationLineColor",
				colorTheme.m_songEditor_automationLineColor, false, false,
				bSilent );
		colorTheme.m_songEditor_automationNodeColor =
			songEditorNode.read_color(
				"automationNodeColor",
				colorTheme.m_songEditor_automationNodeColor, false, false,
				bSilent );
		colorTheme.m_songEditor_stackedModeOnColor =
			songEditorNode.read_color(
				"stackedModeOnColor",
				colorTheme.m_songEditor_stackedModeOnColor, false, false,
				bSilent );
		colorTheme.m_songEditor_stackedModeOnNextColor =
			songEditorNode.read_color(
				"stackedModeOnNextColor",
				colorTheme.m_songEditor_stackedModeOnNextColor, false, false,
				bSilent );
		colorTheme.m_songEditor_stackedModeOffNextColor =
			songEditorNode.read_color(
				"stackedModeOffNextColor",
				colorTheme.m_songEditor_stackedModeOffNextColor, false, false,
				bSilent );
	}
	else {
		WARNINGLOG( "<songEditor> node not found" );
	}

	// PATTERN EDITOR
	const XMLNode patternEditorNode = parent.firstChildElement( "patternEditor" );
	if ( ! patternEditorNode.isNull() ) {
		colorTheme.m_patternEditor_backgroundColor =
			patternEditorNode.read_color(
				"backgroundColor",
				colorTheme.m_patternEditor_backgroundColor, false, false, bSilent );
		colorTheme.m_patternEditor_alternateRowColor =
			patternEditorNode.read_color(
				"alternateRowColor",
				colorTheme.m_patternEditor_alternateRowColor, false, false, bSilent );
		colorTheme.m_patternEditor_selectedRowColor =
			patternEditorNode.read_color(
				"selectedRowColor",
				colorTheme.m_patternEditor_selectedRowColor, false, false, bSilent );
		colorTheme.m_patternEditor_selectedRowTextColor =
			patternEditorNode.read_color(
				"selectedRowTextColor",
				colorTheme.m_patternEditor_selectedRowTextColor, false, false, bSilent );
		colorTheme.m_patternEditor_octaveRowColor =
			patternEditorNode.read_color(
				"octaveRowColor",
				colorTheme.m_patternEditor_octaveRowColor, false, false, bSilent );
		colorTheme.m_patternEditor_textColor =
			patternEditorNode.read_color(
				"textColor",
				colorTheme.m_patternEditor_textColor, false, false, bSilent );
		colorTheme.m_patternEditor_noteVelocityFullColor =
			patternEditorNode.read_color(
				"noteVelocityFullColor",
				colorTheme.m_patternEditor_noteVelocityFullColor, false, false,
				bSilent );
		colorTheme.m_patternEditor_noteVelocityDefaultColor =
			patternEditorNode.read_color(
				"noteVelocityDefaultColor",
				colorTheme.m_patternEditor_noteVelocityDefaultColor, false,
				false, bSilent );
		colorTheme.m_patternEditor_noteVelocityHalfColor =
			patternEditorNode.read_color(
				"noteVelocityHalfColor",
				colorTheme.m_patternEditor_noteVelocityHalfColor, false, false,
				bSilent );
		colorTheme.m_patternEditor_noteVelocityZeroColor =
			patternEditorNode.read_color(
				"noteVelocityZeroColor",
				colorTheme.m_patternEditor_noteVelocityZeroColor, false, false,
				bSilent );
		colorTheme.m_patternEditor_noteOffColor =
			patternEditorNode.read_color(
				"noteOffColor",
				colorTheme.m_patternEditor_noteOffColor, false, false, bSilent );
		colorTheme.m_patternEditor_lineColor =
			patternEditorNode.read_color(
				"lineColor",
				colorTheme.m_patternEditor_lineColor, false, false, bSilent );
		colorTheme.m_patternEditor_line1Color =
			patternEditorNode.read_color(
				"line1Color",
				colorTheme.m_patternEditor_line1Color, false, false, bSilent );
		colorTheme.m_patternEditor_line2Color =
			patternEditorNode.read_color(
				"line2Color",
				colorTheme.m_patternEditor_line2Color, false, false, bSilent );
		colorTheme.m_patternEditor_line3Color =
			patternEditorNode.read_color(
				"line3Color",
				colorTheme.m_patternEditor_line3Color, false, false, bSilent );
		colorTheme.m_patternEditor_line4Color =
			patternEditorNode.read_color(
				"line4Color",
				colorTheme.m_patternEditor_line4Color, false, false, bSilent );
		colorTheme.m_patternEditor_line5Color = patternEditorNode.read_color(
			"line5Color", colorTheme.m_patternEditor_line5Color, false, false,
			bSilent );
		colorTheme.m_patternEditor_instrumentRowColor =
			patternEditorNode.read_color(
				"instrumentRowColor",
				colorTheme.m_patternEditor_instrumentRowColor, false, false,
				bSilent );
		colorTheme.m_patternEditor_instrumentRowTextColor =
			patternEditorNode.read_color(
				"instrumentRowTextColor",
				colorTheme.m_patternEditor_instrumentRowTextColor, false, false,
				bSilent );
		colorTheme.m_patternEditor_instrumentAlternateRowColor =
			patternEditorNode.read_color(
				"instrumentAlternateRowColor",
				colorTheme.m_patternEditor_instrumentAlternateRowColor, false,
				false, bSilent );
		colorTheme.m_patternEditor_instrumentSelectedRowColor =
			patternEditorNode.read_color(
				"instrumentSelectedRowColor",
				colorTheme.m_patternEditor_instrumentSelectedRowColor, false, false,
				bSilent );
		colorTheme.m_patternEditor_instrumentSelectedRowTextColor =
			patternEditorNode.read_color(
				"instrumentSelectedRowTextColor",
				colorTheme.m_patternEditor_instrumentSelectedRowTextColor, false, false,
				bSilent );
	}
	else {
		WARNINGLOG( "<patternEditor> node not found" );
	}

	const XMLNode selectionNode = parent.firstChildElement( "selection" );
	if ( ! selectionNode.isNull() ) {
		colorTheme.m_selectionHighlightColor =
			selectionNode.read_color(
				"highlightColor",
				colorTheme.m_selectionHighlightColor, false, false, bSilent );
		colorTheme.m_selectionInactiveColor =
			selectionNode.read_color(
				"inactiveColor",
				colorTheme.m_selectionInactiveColor, false, false, bSilent );
	}
	else {
		WARNINGLOG( "<selection> node not found" );
	}

	const XMLNode paletteNode = parent.firstChildElement( "palette" );
	if ( ! paletteNode.isNull() ) {
		colorTheme.m_windowColor =
			paletteNode.read_color(
				"windowColor",
				colorTheme.m_windowColor, false, false, bSilent );
		colorTheme.m_windowTextColor =
			paletteNode.read_color(
				"windowTextColor",
				colorTheme.m_windowTextColor, false, false, bSilent );
		colorTheme.m_baseColor =
			paletteNode.read_color(
				"baseColor",
				colorTheme.m_baseColor, false, false, bSilent );
		colorTheme.m_alternateBaseColor =
			paletteNode.read_color(
				"alternateBaseColor",
				colorTheme.m_alternateBaseColor, false, false, bSilent );
		colorTheme.m_textColor =
			paletteNode.read_color(
				"textColor",
				colorTheme.m_textColor, false, false, bSilent );
		colorTheme.m_buttonColor =
			paletteNode.read_color(
				"buttonColor",
				colorTheme.m_buttonColor, false, false, bSilent );
		colorTheme.m_buttonTextColor =
			paletteNode.read_color(
				"buttonTextColor",
				colorTheme.m_buttonTextColor, false, false, bSilent );
		colorTheme.m_lightColor =
			paletteNode.read_color(
				"lightColor",
				colorTheme.m_lightColor, false, false, bSilent );
		colorTheme.m_midLightColor =
			paletteNode.read_color(
				"midLightColor",
				colorTheme.m_midLightColor, false, false, bSilent );
		colorTheme.m_midColor =
			paletteNode.read_color(
				"midColor",
				colorTheme.m_midColor, false, false, bSilent );
		colorTheme.m_darkColor =
			paletteNode.read_color(
				"darkColor",
				colorTheme.m_darkColor, false, false, bSilent );
		colorTheme.m_shadowTextColor =
			paletteNode.read_color(
				"shadowTextColor",
				colorTheme.m_shadowTextColor, false, false, bSilent );
		colorTheme.m_highlightColor =
			paletteNode.read_color(
				"highlightColor",
				colorTheme.m_highlightColor, false, false, bSilent );
		colorTheme.m_highlightedTextColor =
			paletteNode.read_color(
				"highlightedTextColor",
				colorTheme.m_highlightedTextColor, false, false, bSilent );
		colorTheme.m_toolTipBaseColor =
			paletteNode.read_color(
				"toolTipBaseColor",
				colorTheme.m_toolTipBaseColor, false, false, bSilent );
		colorTheme.m_toolTipTextColor =
			paletteNode.read_color(
				"toolTipTextColor",
				colorTheme.m_toolTipTextColor, false, false, bSilent );
	}
	else {
		WARNINGLOG( "<palette> node not found" );
	}

	const XMLNode widgetNode = parent.firstChildElement( "widget" );
	if ( ! widgetNode.isNull() ) {
		colorTheme.m_accentColor =
			widgetNode.read_color(
				"accentColor",
				colorTheme.m_accentColor, false, false, bSilent );
		colorTheme.m_accentTextColor =
			widgetNode.read_color(
				"accentTextColor",
				colorTheme.m_accentTextColor, false, false, bSilent );
		colorTheme.m_widgetColor =
			widgetNode.read_color(
				"widgetColor",
				colorTheme.m_widgetColor, false, false, bSilent );
		colorTheme.m_widgetTextColor =
			widgetNode.read_color(
				"widgetTextColor",
				colorTheme.m_widgetTextColor, false, false, bSilent );
		colorTheme.m_buttonRedColor =
			widgetNode.read_color(
				"buttonRedColor",
				colorTheme.m_buttonRedColor, false, false, bSilent );
		colorTheme.m_buttonRedTextColor =
			widgetNode.read_color(
				"buttonRedTextColor",
				colorTheme.m_buttonRedTextColor, false, false, bSilent );
		colorTheme.m_spinBoxColor =
			widgetNode.read_color(
				"spinBoxColor",
				colorTheme.m_spinBoxColor, false, false, bSilent );
		colorTheme.m_spinBoxTextColor =
			widgetNode.read_color(
				"spinBoxTextColor",
				colorTheme.m_spinBoxTextColor, false, false, bSilent );
		colorTheme.m_playheadColor =
			widgetNode.read_color(
				"playheadColor",
				colorTheme.m_playheadColor, false, false, bSilent );
		colorTheme.m_cursorColor =
			widgetNode.read_color(
				"cursorColor",
				colorTheme.m_cursorColor, false, false, bSilent );
	}
	else {
		WARNINGLOG( "<widget> node not found" );
	}

	return colorTheme;
}

QString ColorTheme::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[ColorTheme]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_songEditor_backgroundColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_backgroundColor.name() ) )
			.append( QString( "%1%2m_songEditor_alternateRowColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_alternateRowColor.name() ) )
			.append( QString( "%1%2m_songEditor_virtualRowColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_virtualRowColor.name() ) )
			.append( QString( "%1%2m_songEditor_selectedRowColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_selectedRowColor.name() ) )
			.append( QString( "%1%2m_songEditor_selectedRowTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_selectedRowTextColor.name() ) )
			.append( QString( "%1%2m_songEditor_lineColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_lineColor.name() ) )
			.append( QString( "%1%2m_songEditor_textColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_textColor.name() ) )
			.append( QString( "%1%2m_songEditor_automationBackgroundColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_automationBackgroundColor.name() ) )
			.append( QString( "%1%2m_songEditor_automationLineColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_automationLineColor.name() ) )
			.append( QString( "%1%2m_songEditor_automationNodeColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_automationNodeColor.name() ) )
			.append( QString( "%1%2m_songEditor_stackedModeOnColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_stackedModeOnColor.name() ) )
			.append( QString( "%1%2m_songEditor_stackedModeOnNextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_stackedModeOnNextColor.name() ) )
			.append( QString( "%1%2m_songEditor_stackedModeOffNextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_songEditor_stackedModeOffNextColor.name() ) )
			.append( QString( "%1%2m_patternEditor_backgroundColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_backgroundColor.name() ) )
			.append( QString( "%1%2m_patternEditor_alternateRowColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_alternateRowColor.name() ) )
			.append( QString( "%1%2m_patternEditor_selectedRowColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_selectedRowColor.name() ) )
			.append( QString( "%1%2m_patternEditor_selectedRowTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_selectedRowTextColor.name() ) )
			.append( QString( "%1%2m_patternEditor_octaveRowColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_octaveRowColor.name() ) )
			.append( QString( "%1%2m_patternEditor_textColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_textColor.name() ) )
			.append( QString( "%1%2m_patternEditor_noteVelocityFullColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_noteVelocityFullColor.name() ) )
			.append( QString( "%1%2m_patternEditor_noteVelocityDefaultColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_noteVelocityDefaultColor.name() ) )
			.append( QString( "%1%2m_patternEditor_noteVelocityHalfColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_noteVelocityHalfColor.name() ) )
			.append( QString( "%1%2m_patternEditor_noteVelocityZeroColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_noteVelocityZeroColor.name() ) )
			.append( QString( "%1%2m_patternEditor_noteOffColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_noteOffColor.name() ) )
			.append( QString( "%1%2m_patternEditor_lineColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_lineColor.name() ) )
			.append( QString( "%1%2m_patternEditor_line1Color: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_line1Color.name() ) )
			.append( QString( "%1%2m_patternEditor_line2Color: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_line2Color.name() ) )
			.append( QString( "%1%2m_patternEditor_line3Color: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_line3Color.name() ) )
			.append( QString( "%1%2m_patternEditor_line4Color: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_line4Color.name() ) )
			.append( QString( "%1%2m_patternEditor_line5Color: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_patternEditor_line5Color.name() ) )
			.append( QString( "%1%2m_patternEditor_instrumentRowColor: %3\n" )
					 .arg( sPrefix ).arg( s )
					 .arg( m_patternEditor_instrumentRowColor.name() ) )
			.append( QString( "%1%2m_patternEditor_instrumentRowTextColor: %3\n" )
					 .arg( sPrefix ).arg( s )
					 .arg( m_patternEditor_instrumentRowTextColor.name() ) )
			.append( QString( "%1%2m_patternEditor_instrumentAlternateRowColor: %3\n" )
					 .arg( sPrefix ).arg( s )
					 .arg( m_patternEditor_instrumentAlternateRowColor.name() ) )
			.append( QString( "%1%2m_patternEditor_instrumentSelectedRowColor: %3\n" )
					 .arg( sPrefix ).arg( s )
					 .arg( m_patternEditor_instrumentSelectedRowColor.name() ) )
			.append( QString( "%1%2m_patternEditor_instrumentSelectedRowTextColor: %3\n" )
					 .arg( sPrefix ).arg( s )
					 .arg( m_patternEditor_instrumentSelectedRowTextColor.name() ) )
			.append( QString( "%1%2m_selectionHighlightColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_selectionHighlightColor.name() ) )
			.append( QString( "%1%2m_selectionInactiveColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_selectionInactiveColor.name() ) )
			.append( QString( "%1%2m_windowColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_windowColor.name() ) )
			.append( QString( "%1%2m_windowTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_windowTextColor.name() ) )
			.append( QString( "%1%2m_baseColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_baseColor.name() ) )
			.append( QString( "%1%2m_alternateBaseColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_alternateBaseColor.name() ) )
			.append( QString( "%1%2m_textColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_textColor.name() ) )
			.append( QString( "%1%2m_buttonColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_buttonColor.name() ) )
			.append( QString( "%1%2m_buttonTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_buttonTextColor.name() ) )
			.append( QString( "%1%2m_lightColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_lightColor.name() ) )
			.append( QString( "%1%2m_midLightColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_midLightColor.name() ) )
			.append( QString( "%1%2m_midColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_midColor.name() ) )
			.append( QString( "%1%2m_darkColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_darkColor.name() ) )
			.append( QString( "%1%2m_shadowTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_shadowTextColor.name() ) )
			.append( QString( "%1%2m_highlightColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_highlightColor.name() ) )
			.append( QString( "%1%2m_highlightedTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_highlightedTextColor.name() ) )
			.append( QString( "%1%2m_toolTipBaseColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_toolTipBaseColor.name() ) )
			.append( QString( "%1%2m_toolTipTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_toolTipTextColor.name() ) )
			.append( QString( "%1%2m_accentColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_accentColor.name() ) )
			.append( QString( "%1%2m_accentTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_accentTextColor.name() ) )
			.append( QString( "%1%2m_widgetColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_widgetColor.name() ) )
			.append( QString( "%1%2m_widgetTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_widgetTextColor.name() ) )
			.append( QString( "%1%2m_buttonRedColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_buttonRedColor.name() ) )
			.append( QString( "%1%2m_buttonRedTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_buttonRedTextColor.name() ) )
			.append( QString( "%1%2m_spinBoxColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_spinBoxColor.name() ) )
			.append( QString( "%1%2m_spinBoxTextColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_spinBoxTextColor.name() ) )
			.append( QString( "%1%2m_playheadColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_playheadColor.name() ) )
			.append( QString( "%1%2m_cursorColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_cursorColor.name() ) );
	}
	else {
		sOutput = QString( "[ColorTheme] " )
			.append( QString( "m_songEditor_backgroundColor: %1" )
					 .arg( m_songEditor_backgroundColor.name() ) )
			.append( QString( ", m_songEditor_alternateRowColor: %1" )
					 .arg( m_songEditor_alternateRowColor.name() ) )
			.append( QString( ", m_songEditor_virtualRowColor: %1" )
					 .arg( m_songEditor_virtualRowColor.name() ) )
			.append( QString( ", m_songEditor_selectedRowColor: %1" )
					 .arg( m_songEditor_selectedRowColor.name() ) )
			.append( QString( ", m_songEditor_selectedRowTextColor: %1" )
					 .arg( m_songEditor_selectedRowTextColor.name() ) )
			.append( QString( ", m_songEditor_lineColor: %1" )
					 .arg( m_songEditor_lineColor.name() ) )
			.append( QString( ", m_songEditor_textColor: %1" )
					 .arg( m_songEditor_textColor.name() ) )
			.append( QString( ", m_songEditor_automationBackgroundColor: %1" )
					 .arg( m_songEditor_automationBackgroundColor.name() ) )
			.append( QString( ", m_songEditor_automationLineColor: %1" )
					 .arg( m_songEditor_automationLineColor.name() ) )
			.append( QString( ", m_songEditor_automationNodeColor: %1" )
					 .arg( m_songEditor_automationNodeColor.name() ) )
			.append( QString( ", m_songEditor_stackedModeOnColor: %1" )
					 .arg( m_songEditor_stackedModeOnColor.name() ) )
			.append( QString( ", m_songEditor_stackedModeOnNextColor: %1" )
					 .arg( m_songEditor_stackedModeOnNextColor.name() ) )
			.append( QString( ", m_songEditor_stackedModeOffNextColor: %1" )
					 .arg( m_songEditor_stackedModeOffNextColor.name() ) )
			.append( QString( ", m_patternEditor_backgroundColor: %1" )
					 .arg( m_patternEditor_backgroundColor.name() ) )
			.append( QString( ", m_patternEditor_alternateRowColor: %1" )
					 .arg( m_patternEditor_alternateRowColor.name() ) )
			.append( QString( ", m_patternEditor_selectedRowColor: %1" )
					 .arg( m_patternEditor_selectedRowColor.name() ) )
			.append( QString( ", m_patternEditor_selectedRowTextColor: %1" )
					 .arg( m_patternEditor_selectedRowTextColor.name() ) )
			.append( QString( ", m_patternEditor_octaveRowColor: %1" )
					 .arg( m_patternEditor_octaveRowColor.name() ) )
			.append( QString( ", m_patternEditor_textColor: %1" )
					 .arg( m_patternEditor_textColor.name() ) )
			.append( QString( ", m_patternEditor_noteVelocityFullColor: %1" )
					 .arg( m_patternEditor_noteVelocityFullColor.name() ) )
			.append( QString( ", m_patternEditor_noteVelocityDefaultColor: %1" )
					 .arg( m_patternEditor_noteVelocityDefaultColor.name() ) )
			.append( QString( ", m_patternEditor_noteVelocityHalfColor: %1" )
					 .arg( m_patternEditor_noteVelocityHalfColor.name() ) )
			.append( QString( ", m_patternEditor_noteVelocityZeroColor: %1" )
					 .arg( m_patternEditor_noteVelocityZeroColor.name() ) )
			.append( QString( ", m_patternEditor_noteOffColor: %1" )
					 .arg( m_patternEditor_noteOffColor.name() ) )
			.append( QString( ", m_patternEditor_lineColor: %1" )
					 .arg( m_patternEditor_lineColor.name() ) )
			.append( QString( ", m_patternEditor_line1Color: %1" )
					 .arg( m_patternEditor_line1Color.name() ) )
			.append( QString( ", m_patternEditor_line2Color: %1" )
					 .arg( m_patternEditor_line2Color.name() ) )
			.append( QString( ", m_patternEditor_line3Color: %1" )
					 .arg( m_patternEditor_line3Color.name() ) )
			.append( QString( ", m_patternEditor_line4Color: %1" )
					 .arg( m_patternEditor_line4Color.name() ) )
			.append( QString( ", m_patternEditor_line5Color: %1" )
					 .arg( m_patternEditor_line5Color.name() ) )
			.append( QString( ", m_patternEditor_instrumentRowColor: %1" )
					 .arg( m_patternEditor_instrumentRowColor.name() ) )
			.append( QString( ", m_patternEditor_instrumentRowTextColor: %1" )
					 .arg( m_patternEditor_instrumentRowTextColor.name() ) )
			.append( QString( ", m_patternEditor_instrumentAlternateRowColor: %1" )
					 .arg( m_patternEditor_instrumentAlternateRowColor.name() ) )
			.append( QString( ", m_patternEditor_instrumentSelectedRowColor: %1" )
					 .arg( m_patternEditor_instrumentSelectedRowColor.name() ) )
			.append( QString( ", m_patternEditor_instrumentSelectedRowTextColor: %1" )
					 .arg( m_patternEditor_instrumentSelectedRowTextColor.name() ) )
			.append( QString( ", m_selectionHighlightColor: %1" )
					 .arg( m_selectionHighlightColor.name() ) )
			.append( QString( ", m_selectionInactiveColor: %1" )
					 .arg( m_selectionInactiveColor.name() ) )
			.append( QString( ", m_windowColor: %1" )
					 .arg( m_windowColor.name() ) )
			.append( QString( ", m_windowTextColor: %1" )
					 .arg( m_windowTextColor.name() ) )
			.append( QString( ", m_baseColor: %1" )
					 .arg( m_baseColor.name() ) )
			.append( QString( ", m_alternateBaseColor: %1" )
					 .arg( m_alternateBaseColor.name() ) )
			.append( QString( ", m_textColor: %1" )
					 .arg( m_textColor.name() ) )
			.append( QString( ", m_buttonColor: %1" )
					 .arg( m_buttonColor.name() ) )
			.append( QString( ", m_buttonTextColor: %1" )
					 .arg( m_buttonTextColor.name() ) )
			.append( QString( ", m_lightColor: %1" )
					 .arg( m_lightColor.name() ) )
			.append( QString( ", m_midLightColor: %1" )
					 .arg( m_midLightColor.name() ) )
			.append( QString( ", m_midColor: %1" )
					 .arg( m_midColor.name() ) )
			.append( QString( ", m_darkColor: %1" )
					 .arg( m_darkColor.name() ) )
			.append( QString( ", m_shadowTextColor: %1" )
					 .arg( m_shadowTextColor.name() ) )
			.append( QString( ", m_highlightColor: %1" )
					 .arg( m_highlightColor.name() ) )
			.append( QString( ", m_highlightedTextColor: %1" )
					 .arg( m_highlightedTextColor.name() ) )
			.append( QString( ", m_toolTipBaseColor: %1" )
					 .arg( m_toolTipBaseColor.name() ) )
			.append( QString( ", m_toolTipTextColor: %1" )
					 .arg( m_toolTipTextColor.name() ) )
			.append( QString( ", m_accentColor: %1" )
					 .arg( m_accentColor.name() ) )
			.append( QString( ", m_accentTextColor: %1" )
					 .arg( m_accentTextColor.name() ) )
			.append( QString( ", m_widgetColor: %1" )
					 .arg( m_widgetColor.name() ) )
			.append( QString( ", m_widgetTextColor: %1" )
					 .arg( m_widgetTextColor.name() ) )
			.append( QString( ", m_buttonRedColor: %1" )
					 .arg( m_buttonRedColor.name() ) )
			.append( QString( ", m_buttonRedTextColor: %1" )
					 .arg( m_buttonRedTextColor.name() ) )
			.append( QString( ", m_spinBoxColor: %1" )
					 .arg( m_spinBoxColor.name() ) )
			.append( QString( ", m_spinBoxTextColor: %1" )
					 .arg( m_spinBoxTextColor.name() ) )
			.append( QString( ", m_playheadColor: %1" )
					 .arg( m_playheadColor.name() ) )
			.append( QString( ", m_cursorColor: %1" )
					 .arg( m_cursorColor.name() ) );
	}

	return sOutput;
}

////////////////////////////////////////////////////////////////////////////////

float InterfaceTheme::FALLOFF_SLOW = 1.08f;
float InterfaceTheme::FALLOFF_NORMAL = 1.1f;
float InterfaceTheme::FALLOFF_FAST = 1.5f;

InterfaceTheme::InterfaceTheme()
	: m_sQTStyle( "Fusion" )
	, m_fMixerFalloffSpeed( InterfaceTheme::FALLOFF_NORMAL )
	, m_layout( InterfaceTheme::Layout::SinglePane )
	, m_uiScalingPolicy( InterfaceTheme::ScalingPolicy::Smaller )
	, m_iconColor( InterfaceTheme::IconColor::Black )
	, m_coloringMethod( InterfaceTheme::ColoringMethod::Custom )
	, m_nVisiblePatternColors( 18 )
	, m_bIndicateNotePlayback( true ) {
	m_patternColors.resize( nMaxPatternColors );

	std::vector<QColor> defaultColors {
		QColor( 67,96,131 ),
		QColor( 67,114,175 ),
		QColor( 105,165,189 ),
		QColor( 148,189,202 ),
		QColor( 193,211,215 ),
		QColor( 229,201,130 ),
		QColor( 234,160,90 ),
		QColor( 218,97,60 ),
		QColor( 186,55,50 ),
		QColor( 152,22,26 ),
		QColor( 186,55,50 ),
		QColor( 218,97,60 ),
		QColor( 234,160,90 ),
		QColor( 229,201,130 ),
		QColor( 193,211,215 ),
		QColor( 148,189,202 ),
		QColor( 105,165,189 ),
		QColor( 67,114,175 ),
		QColor( 67,96,131 )
	};

	for ( int ii = 0; ii < InterfaceTheme::nMaxPatternColors; ii++ ) {
		m_patternColors[ ii ] = defaultColors[
			std::min( ii, static_cast<int>(defaultColors.size() - 1 ) ) ];
	}
}

QString InterfaceTheme::LayoutToQString( const Layout& layout ) {
	switch( layout ) {
	case Layout::SinglePane:
		return "SinglePane";
	case Layout::Tabbed:
		return "Tabbed";
	default:
		return QString( "Unknown layout [%1]" ).arg( static_cast<int>(layout) );
	}
}

QString InterfaceTheme::ScalingPolicyToQString( const ScalingPolicy& scalingPolicy ) {
	switch( scalingPolicy ) {
	case ScalingPolicy::Smaller:
		return "Smaller";
	case ScalingPolicy::System:
		return "System";
	case ScalingPolicy::Larger:
		return "Larger";
	default:
		return QString( "Unknown scalingPolicy [%1]" )
			.arg( static_cast<int>(scalingPolicy) );
	}
}

QString InterfaceTheme::IconColorToQString( const IconColor& iconColor ) {
	switch( iconColor ) {
	case IconColor::Black:
		return "Black";
	case IconColor::White:
		return "White";
	default:
		return QString( "Unknown iconColor [%1]" )
			.arg( static_cast<int>(iconColor) );
	}
}

QString InterfaceTheme::ColoringMethodToQString( const ColoringMethod& coloringMethod ) {
	switch( coloringMethod ) {
	case ColoringMethod::Automatic:
		return "Automatic";
	case ColoringMethod::Custom:
		return "Custom";
	default:
		return QString( "Unknown coloringMethod [%1]" )
			.arg( static_cast<int>(coloringMethod) );
	}
}

QString InterfaceTheme::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[InterfaceTheme]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_sQTStyle: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sQTStyle ) )
			.append( QString( "%1%2m_fMixerFalloffSpeed: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_fMixerFalloffSpeed ) )
			.append( QString( "%1%2m_layout: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( LayoutToQString( m_layout ) ) )
			.append( QString( "%1%2m_uiScalingPolicy: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( ScalingPolicyToQString( m_uiScalingPolicy ) ) )
			.append( QString( "%1%2m_iconColor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( IconColorToQString( m_iconColor ) ) )
			.append( QString( "%1%2m_coloringMethod: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( ColoringMethodToQString( m_coloringMethod ) ) )
			.append( QString( "%1%2m_patternColors: [\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ccolor : m_patternColors ) {
			sOutput.append( QString( "%1%2%2%3\n" ).arg( sPrefix ).arg( s )
							.arg( ccolor.name() ) );
		}
		sOutput.append( QString( "%1%2]\n%1%2m_nVisiblePatternColors: %3\n" )
						.arg( sPrefix ) .arg( s ).arg( m_nVisiblePatternColors ) )
			.append( QString( "%1%2m_bIndicateNotePlayback: %3\n" )
					 .arg( sPrefix ) .arg( s ).arg( m_bIndicateNotePlayback ) );
	}
	else {
		sOutput = QString( "[InterfaceTheme] " )
			.append( QString( "m_sQTStyle: %1" ).arg( m_sQTStyle ) )
			.append( QString( ", m_fMixerFalloffSpeed: %1" )
					 .arg( m_fMixerFalloffSpeed ) )
			.append( QString( ", m_layout: %1" )
					 .arg( LayoutToQString( m_layout ) ) )
			.append( QString( ", m_uiScalingPolicy: %1" )
					 .arg( ScalingPolicyToQString( m_uiScalingPolicy ) ) )
			.append( QString( ", m_iconColor: %1" )
					 .arg( IconColorToQString( m_iconColor ) ) )
			.append( QString( ", m_coloringMethod: %1" )
					 .arg( ColoringMethodToQString( m_coloringMethod ) ) )
			.append( QString( ", m_patternColors: [" ) );
		for ( const auto& ccolor : m_patternColors ) {
			sOutput.append( QString( "%1, " ).arg( ccolor.name() ) );
		}
		sOutput.append( QString( "], m_nVisiblePatternColors: %1" )
						 .arg( m_nVisiblePatternColors ) )
			.append( QString( ", m_bIndicateNotePlayback: %1" )
					 .arg( m_bIndicateNotePlayback ) );
	}

	return sOutput;
}


////////////////////////////////////////////////////////////////////////////////

FontTheme::FontTheme()
	: m_sApplicationFontFamily( "Lucida Grande" )
	, m_sLevel2FontFamily( "Lucida Grande" )
	, m_sLevel3FontFamily( "Lucida Grande" )
	, m_fontSize( FontTheme::FontSize::Medium ) {
}

QString FontTheme::FontSizeToQString( const FontSize& fontSize ) {
	switch( fontSize ) {
	case FontSize::Medium:
		return "Medium";
	case FontSize::Small:
		return "Small";
	case FontSize::Large:
		return "Large";
	default:
		return QString( "Unknown fontSize [%1]" )
			.arg( static_cast<int>(fontSize) );
	}
}

QString FontTheme::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[FontTheme]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_sApplicationFontFamily: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sApplicationFontFamily ) )
			.append( QString( "%1%2m_sLevel2FontFamily: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sLevel2FontFamily ) )
			.append( QString( "%1%2m_sLevel3FontFamily: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sLevel3FontFamily ) )
			.append( QString( "%1%2m_fontSize: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( FontSizeToQString( m_fontSize ) ) );
	}
	else {
		sOutput = QString( "[FontTheme] " )
			.append( QString( "m_sApplicationFontFamily: %1" )
					 .arg( m_sApplicationFontFamily ) )
			.append( QString( ", m_sLevel2FontFamily: %1" )
					 .arg( m_sLevel2FontFamily ) )
			.append( QString( ", m_sLevel3FontFamily: %1" )
					 .arg( m_sLevel3FontFamily ) )
			.append( QString( ", m_fontSize: %1" )
					 .arg( FontSizeToQString( m_fontSize ) ) );
	}

	return sOutput;
}

////////////////////////////////////////////////////////////////////////////////

Theme::Theme( const ColorTheme& colorTheme,
			  const InterfaceTheme& interfaceTheme,
			  const FontTheme& fontTheme )
	: m_color{ colorTheme }
	, m_interface{ interfaceTheme }
	, m_font{ fontTheme } {
}

Theme::Theme( const Theme& other )
	: m_color{ other.m_color }
	, m_interface{ other.m_interface}
	, m_font{ other.m_font } {
}

Theme::Theme( Theme&& other )
	: m_color{ other.m_color }
	, m_interface{ other.m_interface }
	, m_font{ other.m_font }
{
}

Theme& Theme::operator=( const Theme& other ) {
	m_color = other.m_color;
	m_interface = other.m_interface;
	m_font = other.m_font;

	return *this;
}

Theme& Theme::operator=( Theme&& other ) {
	m_color = std::move( other.m_color );
	m_interface = std::move( other.m_interface );
	m_font = std::move( other.m_font );

	return *this;
}

std::unique_ptr<Theme> Theme::importFrom( const QString& sPath ) {
	if ( ! Filesystem::file_exists( sPath ) || ! Filesystem::file_readable( sPath ) ){
		return nullptr;
	}

	INFOLOG( QString( "Importing theme to %1" ).arg( sPath ) );

	XMLDoc doc;
	if ( ! doc.read( sPath, nullptr, true ) ) {
		ERRORLOG( "Unable to load theme." );
		return nullptr;
	}
	
	XMLNode rootNode = doc.firstChildElement( "hydrogen_theme" );
	if ( rootNode.isNull() ) {
		ERRORLOG( "'hydrogen_theme' node not found" );
		return nullptr;
	}

	XMLNode colorThemeNode = rootNode.firstChildElement( "colorTheme" );
	if ( colorThemeNode.isNull() ) {
		ERRORLOG( "'colorTheme' node not found" );
		return nullptr;
	}
	auto colorTheme = ColorTheme::loadFrom( colorThemeNode );
	
	XMLNode interfaceNode = rootNode.firstChildElement( "interfaceTheme" );
	if ( interfaceNode.isNull() ) {
		ERRORLOG( "'interfaceTheme' node not found" );
		return nullptr;
	}
	auto interfaceTheme = InterfaceTheme();
	interfaceTheme.m_layout =
		static_cast<InterfaceTheme::Layout>(
			interfaceNode.read_int( "defaultUILayout",
									static_cast<int>(InterfaceTheme::Layout::SinglePane),
									false, false ));
	interfaceTheme.m_uiScalingPolicy =
		static_cast<InterfaceTheme::ScalingPolicy>(
			interfaceNode.read_int( "uiScalingPolicy",
									static_cast<int>(InterfaceTheme::ScalingPolicy::Smaller),
									false, false ));
				
	// QT Style
	interfaceTheme.m_sQTStyle =
		interfaceNode.read_string( "QTStyle", "Fusion", false, false );

	if ( interfaceTheme.m_sQTStyle == "Plastique" ){
		interfaceTheme.m_sQTStyle = "Fusion";
	}
	interfaceTheme.m_iconColor =
		static_cast<InterfaceTheme::IconColor>(
			interfaceNode.read_int( "iconColor",
									static_cast<int>(InterfaceTheme::IconColor::Black),
									false, false));

	// Mixer falloff speed
	interfaceTheme.m_fMixerFalloffSpeed =
		interfaceNode.read_float( "mixer_falloff_speed",
								  InterfaceTheme::FALLOFF_NORMAL, false, false );

	//SongEditor coloring
	interfaceTheme.m_coloringMethod =
		static_cast<InterfaceTheme::ColoringMethod>(
			interfaceNode.read_int("SongEditor_ColoringMethod",
								   static_cast<int>(InterfaceTheme::ColoringMethod::Custom),
								   false, false ));
	std::vector<QColor> colors( interfaceTheme.nMaxPatternColors );
	for ( int ii = 0; ii < interfaceTheme.nMaxPatternColors; ii++ ) {
		colors[ ii ] = interfaceNode.read_color( QString( "SongEditor_pattern_color_%1" ).arg( ii ),
												 colorTheme.m_accentColor,
												 false, false );
	}
	interfaceTheme.m_patternColors = colors;
	interfaceTheme.m_nVisiblePatternColors =
		interfaceNode.read_int( "SongEditor_visible_pattern_colors", 1, false, false );
	if ( interfaceTheme.m_nVisiblePatternColors > 50 ) {
		interfaceTheme.m_nVisiblePatternColors = 50;
	} else if ( interfaceTheme.m_nVisiblePatternColors < 0 ) {
		interfaceTheme.m_nVisiblePatternColors = 0;
	}

	interfaceTheme.m_bIndicateNotePlayback = interfaceNode.read_bool(
		"indicate_note_playback", true, /* inexistent_ok */ true,
		/* empty_ok */ false );
			
	XMLNode fontNode = rootNode.firstChildElement( "fontTheme" );
	if ( fontNode.isNull() ) {
		ERRORLOG( "'fontTheme' node not found" );
		return nullptr;
	}

	auto fontTheme = FontTheme();
	// Font fun
	fontTheme.m_sApplicationFontFamily =
		fontNode.read_string( "application_font_family",
							  fontTheme.m_sApplicationFontFamily, false, false );
	// The value defaults to m_sApplicationFontFamily on
	// purpose to provide backward compatibility.
	fontTheme.m_sLevel2FontFamily =
		fontNode.read_string( "level2_font_family",
							  fontTheme.m_sLevel2FontFamily, false, false );
	fontTheme.m_sLevel3FontFamily =
		fontNode.read_string( "level3_font_family",
							  fontTheme.m_sLevel3FontFamily, false, false );
	fontTheme.m_fontSize =
		static_cast<FontTheme::FontSize>(
			fontNode.read_int( "font_size",
							   static_cast<int>(FontTheme::FontSize::Medium), false, false ) );

	return std::make_unique<Theme>(colorTheme, interfaceTheme, fontTheme );
}

bool Theme::exportTo( const QString& sPath ) const {

	INFOLOG( QString( "Exporting theme to %1" ).arg( sPath ) );

	XMLDoc doc;
	XMLNode rootNode = doc.set_root( "hydrogen_theme", "theme" );
	// hydrogen version
	rootNode.write_string( "version", QString( get_version().c_str() ) );
	
	m_color.saveTo( rootNode );

	XMLNode interfaceNode = rootNode.createNode( "interfaceTheme" );
	interfaceNode.write_int( "defaultUILayout",
							 static_cast<int>(m_interface.m_layout) );
	interfaceNode.write_int( "uiScalingPolicy",
							 static_cast<int>(m_interface.m_uiScalingPolicy) );
	interfaceNode.write_string( "QTStyle", m_interface.m_sQTStyle );
	interfaceNode.write_int( "iconColor",
							 static_cast<int>(m_interface.m_iconColor) );
	interfaceNode.write_float( "mixer_falloff_speed",
							   m_interface.m_fMixerFalloffSpeed );
	interfaceNode.write_int( "SongEditor_ColoringMethod",
							 static_cast<int>(m_interface.m_coloringMethod) );
	for ( int ii = 0; ii < m_interface.nMaxPatternColors; ii++ ) {
		interfaceNode.write_color( QString( "SongEditor_pattern_color_%1" ).arg( ii ),
								   m_interface.m_patternColors[ ii ] );
	}
	interfaceNode.write_int( "SongEditor_visible_pattern_colors",
							 m_interface.m_nVisiblePatternColors );
	interfaceNode.write_bool( "indicate_note_playback",
							  m_interface.m_bIndicateNotePlayback );

	XMLNode fontNode = rootNode.createNode( "fontTheme" );
	fontNode.write_string( "application_font_family",
						   m_font.m_sApplicationFontFamily );
	fontNode.write_string( "level2_font_family",
						   m_font.m_sLevel2FontFamily );
	fontNode.write_string( "level3_font_family",
						   m_font.m_sLevel3FontFamily );
	fontNode.write_int( "font_size",
						static_cast<int>(m_font.m_fontSize) );

	return doc.write( sPath );
}	

QString Theme::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Theme]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_color: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_color.toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_interface: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_interface.toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_font: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_font.toQString( sPrefix + s, bShort ) ) );
	}
	else {
		sOutput = QString( "[Theme] " )
			.append( QString( "m_color: %1" )
					 .arg( m_color.toQString( "", bShort ) ) )
			.append( QString( ", m_interface: %1" )
					 .arg( m_interface.toQString( "", bShort ) ) )
			.append( QString( ", m_font: %1" )
					 .arg( m_font.toQString( "", bShort ) ) );
	}

	return sOutput;
}


}
