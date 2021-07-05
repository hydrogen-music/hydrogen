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
#include <core/Preferences.h>

QString Skin::getGlobalStyleSheet() {
	auto pPref = H2Core::Preferences::get_instance();

	int nFactorGradient = 120;
	int nHover = 10;
	
	QColor buttonBackgroundLight = pPref->getDefaultUIStyle()->m_widgetColor.lighter( nFactorGradient );
	QColor buttonBackgroundDark = pPref->getDefaultUIStyle()->m_widgetColor.darker( nFactorGradient );
	QColor buttonBackgroundLightHover = pPref->getDefaultUIStyle()->m_widgetColor.lighter( nFactorGradient + nHover );
	QColor buttonBackgroundDarkHover = pPref->getDefaultUIStyle()->m_widgetColor.darker( nFactorGradient + nHover );

	QColor buttonBackgroundCheckedLight = pPref->getDefaultUIStyle()->m_accentColor.lighter( nFactorGradient );
	QColor buttonBackgroundCheckedDark = pPref->getDefaultUIStyle()->m_accentColor.darker( nFactorGradient );
	QColor buttonBackgroundCheckedLightHover = pPref->getDefaultUIStyle()->m_accentColor.lighter( nFactorGradient + nHover );
	QColor buttonBackgroundCheckedDarkHover = pPref->getDefaultUIStyle()->m_accentColor.darker( nFactorGradient + nHover );
	QColor buttonTextChecked = pPref->getDefaultUIStyle()->m_accentTextColor;
	
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
QLineEdit { \
    color: %14; \
    background-color: %15; \
} \
QDoubleSpinBox, QSpinBox { \
    color: %16; \
    background-color: %17; \
    selection-color: %18; \
    selection-background-color: %19; \
}"
					)
		.arg( pPref->getDefaultUIStyle()->m_toolTipTextColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_toolTipBaseColor.name() )
		.arg( buttonTextChecked.name() )
		.arg( buttonBackgroundLight.name() ).arg( buttonBackgroundDark.name() )
		.arg( buttonBackgroundLightHover.name() ).arg( buttonBackgroundDarkHover.name() )
		.arg( buttonBackgroundCheckedLight.name() ).arg( buttonBackgroundCheckedDark.name() )
		.arg( buttonBackgroundCheckedLightHover.name() ).arg( buttonBackgroundCheckedDarkHover.name() )
		.arg( pPref->getDefaultUIStyle()->m_widgetTextColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_widgetColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_windowTextColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_windowColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_accentTextColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_accentColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_spinBoxSelectionTextColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_spinBoxSelectionColor.name() );
}

void Skin::setPalette( QApplication *pQApp ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	// create the default palette
	QPalette defaultPalette;

	defaultPalette.setColor( QPalette::Window, pPref->getDefaultUIStyle()->m_windowColor );
	defaultPalette.setColor( QPalette::WindowText, pPref->getDefaultUIStyle()->m_windowTextColor );
	defaultPalette.setColor( QPalette::Base, pPref->getDefaultUIStyle()->m_baseColor );
	defaultPalette.setColor( QPalette::AlternateBase, pPref->getDefaultUIStyle()->m_alternateBaseColor );
	defaultPalette.setColor( QPalette::Text, pPref->getDefaultUIStyle()->m_textColor );
	defaultPalette.setColor( QPalette::Button, pPref->getDefaultUIStyle()->m_buttonColor );
	defaultPalette.setColor( QPalette::ButtonText, pPref->getDefaultUIStyle()->m_buttonTextColor );
	defaultPalette.setColor( QPalette::Light, pPref->getDefaultUIStyle()->m_lightColor );
	defaultPalette.setColor( QPalette::Midlight, pPref->getDefaultUIStyle()->m_midLightColor );
	defaultPalette.setColor( QPalette::Dark, pPref->getDefaultUIStyle()->m_darkColor );
	defaultPalette.setColor( QPalette::Mid, pPref->getDefaultUIStyle()->m_midColor );
	defaultPalette.setColor( QPalette::Shadow, pPref->getDefaultUIStyle()->m_shadowTextColor );
	defaultPalette.setColor( QPalette::Highlight, pPref->getDefaultUIStyle()->m_highlightColor );
	defaultPalette.setColor( QPalette::HighlightedText, pPref->getDefaultUIStyle()->m_highlightedTextColor );
	defaultPalette.setColor( QPalette::ToolTipBase, pPref->getDefaultUIStyle()->m_toolTipBaseColor );
	defaultPalette.setColor( QPalette::ToolTipText, pPref->getDefaultUIStyle()->m_toolTipTextColor );

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
		.arg( pPref->getDefaultUIStyle()->m_windowTextColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_windowColor.name() );
}
