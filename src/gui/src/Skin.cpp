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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Skin.h"
#include <core/Preferences/Preferences.h>

QString Skin::getGlobalStyleSheet() {
	auto pPref = H2Core::Preferences::get_instance();

	int nFactorGradient = 120;
	int nHover = 10;
	
	QColor buttonBackgroundLight = pPref->getColorTheme()->m_widgetColor.lighter( nFactorGradient );
	QColor buttonBackgroundDark = pPref->getColorTheme()->m_widgetColor.darker( nFactorGradient );
	QColor buttonBackgroundLightHover = pPref->getColorTheme()->m_widgetColor.lighter( nFactorGradient + nHover );
	QColor buttonBackgroundDarkHover = pPref->getColorTheme()->m_widgetColor.darker( nFactorGradient + nHover );

	QColor buttonBackgroundCheckedLight = pPref->getColorTheme()->m_accentColor.lighter( nFactorGradient );
	QColor buttonBackgroundCheckedDark = pPref->getColorTheme()->m_accentColor.darker( nFactorGradient );
	QColor buttonBackgroundCheckedLightHover = pPref->getColorTheme()->m_accentColor.lighter( nFactorGradient + nHover );
	QColor buttonBackgroundCheckedDarkHover = pPref->getColorTheme()->m_accentColor.darker( nFactorGradient + nHover );
	QColor buttonTextChecked = pPref->getColorTheme()->m_accentTextColor;
	QColor spinBoxSelection = pPref->getColorTheme()->m_spinBoxColor.darker( 120 );
	
	return QString( "\
QToolTip { \
    padding: 1px; \
    border: 1px solid %1; \
    background-color: %2; \
    color: %1; \
} \
QPushButton { \
    color: %12; \
    border-radius: 2px; \
    padding: 5px; \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %4, stop: 1 %5); \
} \
QPushButton:hover { \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %6, stop: 1 %7); \
} \
QPushButton:checked { \
    color: %3; \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %8, stop: 1 %9); \
} \
QPushButton:checked:hover { \
    color: %3; \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %10, stop: 1 %11); \
} \
QComboBox { \
    color: %12; \
    background-color: %13; \
} \
QComboBox QAbstractItemView { \
    background-color: #babfcf; \
} \
QLineEdit, QTextEdit { \
    color: %12; \
    background-color: %13; \
} \
QDoubleSpinBox, QSpinBox { \
    color: %14; \
    background-color: %15; \
    selection-color: %14; \
    selection-background-color: %16; \
}"
					)
		.arg( pPref->getColorTheme()->m_toolTipTextColor.name() )
		.arg( pPref->getColorTheme()->m_toolTipBaseColor.name() )
		.arg( buttonTextChecked.name() )
		.arg( buttonBackgroundLight.name() ).arg( buttonBackgroundDark.name() )
		.arg( buttonBackgroundLightHover.name() ).arg( buttonBackgroundDarkHover.name() )
		.arg( buttonBackgroundCheckedLight.name() ).arg( buttonBackgroundCheckedDark.name() )
		.arg( buttonBackgroundCheckedLightHover.name() ).arg( buttonBackgroundCheckedDarkHover.name() )
		.arg( pPref->getColorTheme()->m_widgetTextColor.name() )
		.arg( pPref->getColorTheme()->m_widgetColor.name() )
		.arg( pPref->getColorTheme()->m_spinBoxTextColor.name() )
		.arg( pPref->getColorTheme()->m_spinBoxColor.name() )
		.arg( spinBoxSelection.name() );
}

void Skin::setPalette( QApplication *pQApp ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	// create the default palette
	QPalette defaultPalette;

	defaultPalette.setColor( QPalette::Window, pPref->getColorTheme()->m_windowColor );
	defaultPalette.setColor( QPalette::WindowText, pPref->getColorTheme()->m_windowTextColor );
	defaultPalette.setColor( QPalette::Base, pPref->getColorTheme()->m_baseColor );
	defaultPalette.setColor( QPalette::AlternateBase, pPref->getColorTheme()->m_alternateBaseColor );
	defaultPalette.setColor( QPalette::Text, pPref->getColorTheme()->m_textColor );
	defaultPalette.setColor( QPalette::Button, pPref->getColorTheme()->m_buttonColor );
	defaultPalette.setColor( QPalette::ButtonText, pPref->getColorTheme()->m_buttonTextColor );
	defaultPalette.setColor( QPalette::Light, pPref->getColorTheme()->m_lightColor );
	defaultPalette.setColor( QPalette::Midlight, pPref->getColorTheme()->m_midLightColor );
	defaultPalette.setColor( QPalette::Dark, pPref->getColorTheme()->m_darkColor );
	defaultPalette.setColor( QPalette::Mid, pPref->getColorTheme()->m_midColor );
	defaultPalette.setColor( QPalette::Shadow, pPref->getColorTheme()->m_shadowTextColor );
	defaultPalette.setColor( QPalette::Highlight, pPref->getColorTheme()->m_highlightColor );
	defaultPalette.setColor( QPalette::HighlightedText, pPref->getColorTheme()->m_highlightedTextColor );
	defaultPalette.setColor( QPalette::ToolTipBase, pPref->getColorTheme()->m_toolTipBaseColor );
	defaultPalette.setColor( QPalette::ToolTipText, pPref->getColorTheme()->m_toolTipTextColor );

	// Desaturate disabled widgets by blending with the alternate colour
	for ( QPalette::ColorRole role : { QPalette::Window, QPalette::Base, QPalette::AlternateBase, QPalette::Dark,
									  QPalette::Light, QPalette::Midlight, QPalette::Mid, QPalette::Shadow,
									  QPalette::Text } ) {
		QColor normalColor = defaultPalette.color( QPalette::Normal, role );
		QColor disabledColor = QColor( ( normalColor.red() + 138 ) / 2,
									   ( normalColor.green() + 144 ) / 2,
									   ( normalColor.blue() + 162 ) / 2);
		defaultPalette.setColor( QPalette::Disabled, role, disabledColor );
	}

	pQApp->setPalette( defaultPalette );
	pQApp->setStyleSheet( getGlobalStyleSheet() );
}

QString Skin::getWarningButtonStyleSheet( int nSize) {
	auto pPref = H2Core::Preferences::get_instance();
	
	return QString( "\
width: %1px; \
height: %1px; \
color: %2; \
background-color: %3; \
border-color: %3;" )
		.arg( nSize )
		.arg( pPref->getColorTheme()->m_windowTextColor.name() )
		.arg( pPref->getColorTheme()->m_windowColor.name() );
}

QColor Skin::makeWidgetColorInactive( QColor color ){
	int nHue, nSaturation, nValue;
	color.getHsv( &nHue, &nSaturation, &nValue );
	nValue = std::max( 0, nValue - 40 );
	color.setHsv( nHue, nSaturation, nValue );

	return color;
}

QColor Skin::makeTextColorInactive( QColor color ) {
	int nHue, nSaturation, nValue;
	color.getHsv( &nHue, &nSaturation, &nValue );
	if ( nValue >= 130 ) {
		// TextInactive color is more white than black. Make it darker.
		nValue -= 55;
	} else {
		nValue += 55;
	}
	color.setHsv( nHue, nSaturation, nValue );

	return color;
}
