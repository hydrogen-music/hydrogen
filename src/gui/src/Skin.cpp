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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Skin.h"
#include <core/Preferences/Preferences.h>

#include <QApplication>

QString Skin::getGlobalStyleSheet() {
	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	const int nFactorGradient = 120;
	const int nHover = 10;
	
	const QColor buttonBackgroundLight =
		colorTheme.m_widgetColor.lighter( nFactorGradient );
	const QColor buttonBackgroundDark =
		colorTheme.m_widgetColor.darker( nFactorGradient );
	const QColor buttonBackgroundLightHover =
		colorTheme.m_widgetColor.lighter( nFactorGradient + nHover );
	const QColor buttonBackgroundDarkHover =
		colorTheme.m_widgetColor.darker( nFactorGradient + nHover );

	const QColor buttonBackgroundCheckedLight =
		colorTheme.m_accentColor.lighter( nFactorGradient );
	const QColor buttonBackgroundCheckedDark =
		colorTheme.m_accentColor.darker( nFactorGradient );
	const QColor buttonBackgroundCheckedLightHover =
		colorTheme.m_accentColor.lighter( nFactorGradient + nHover );
	const QColor buttonBackgroundCheckedDarkHover =
		colorTheme.m_accentColor.darker( nFactorGradient + nHover );
	const QColor buttonTextChecked = colorTheme.m_accentTextColor;
	const QColor spinBoxSelection = colorTheme.m_spinBoxColor.darker( 120 );
	
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
		.arg( colorTheme.m_toolTipTextColor.name() )
		.arg( colorTheme.m_toolTipBaseColor.name() )
		.arg( buttonTextChecked.name() )
		.arg( buttonBackgroundLight.name() ).arg( buttonBackgroundDark.name() )
		.arg( buttonBackgroundLightHover.name() )
		.arg( buttonBackgroundDarkHover.name() )
		.arg( buttonBackgroundCheckedLight.name() )
		.arg( buttonBackgroundCheckedDark.name() )
		.arg( buttonBackgroundCheckedLightHover.name() )
		.arg( buttonBackgroundCheckedDarkHover.name() )
		.arg( colorTheme.m_widgetTextColor.name() )
		.arg( colorTheme.m_widgetColor.name() )
		.arg( colorTheme.m_spinBoxTextColor.name() )
		.arg( colorTheme.m_spinBoxColor.name() )
		.arg( spinBoxSelection.name() );
}

void Skin::setPalette( QApplication *pQApp ) {
	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	// create the default palette
	QPalette defaultPalette;

	defaultPalette.setColor( QPalette::Window, colorTheme.m_windowColor );
	defaultPalette.setColor( QPalette::WindowText, colorTheme.m_windowTextColor );
	defaultPalette.setColor( QPalette::Base, colorTheme.m_baseColor );
	defaultPalette.setColor( QPalette::AlternateBase, colorTheme.m_alternateBaseColor );
	defaultPalette.setColor( QPalette::Text, colorTheme.m_textColor );
	defaultPalette.setColor( QPalette::Button, colorTheme.m_buttonColor );
	defaultPalette.setColor( QPalette::ButtonText, colorTheme.m_buttonTextColor );
	defaultPalette.setColor( QPalette::Light, colorTheme.m_lightColor );
	defaultPalette.setColor( QPalette::Midlight, colorTheme.m_midLightColor );
	defaultPalette.setColor( QPalette::Dark, colorTheme.m_darkColor );
	defaultPalette.setColor( QPalette::Mid, colorTheme.m_midColor );
	defaultPalette.setColor( QPalette::Shadow, colorTheme.m_shadowTextColor );
	defaultPalette.setColor( QPalette::Highlight, colorTheme.m_highlightColor );
	defaultPalette.setColor( QPalette::HighlightedText, colorTheme.m_highlightedTextColor );
	defaultPalette.setColor( QPalette::ToolTipBase, colorTheme.m_toolTipBaseColor );
	defaultPalette.setColor( QPalette::ToolTipText, colorTheme.m_toolTipTextColor );

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

void Skin::drawListBackground( QPainter* p, const QRect& rect,
							   QColor background, bool bHovered ) {

	if ( bHovered ) {
		background = background.lighter( 110 );
	}

	QColor borderLight = background.lighter(
		Skin::nListBackgroundLightBorderScaling );
	QColor borderDark = background.darker(
		Skin::nListBackgroundDarkBorderScaling );

	p->fillRect( QRect( rect.x() + 1, rect.y() + 1,
						rect.width() - 2, rect.height() - 2 ), background );
	p->fillRect( QRect( rect.x(), rect.y(), rect.width(), 1 ), borderLight );
	p->fillRect( QRect( rect.x(), rect.y(), 1, rect.height() ), borderLight );
	p->fillRect( QRect( rect.x(), rect.y() + rect.height() - 1, rect.width(), 1 ),
				 borderDark );
	p->fillRect( QRect( rect.x() + rect.width() - 1, rect.y(), 1, rect.height() ),
				 borderDark );
}

QColor Skin::makeWidgetColorInactive( const QColor& color ){
	int nHue, nSaturation, nValue;
	color.getHsv( &nHue, &nSaturation, &nValue );
	nValue = std::max( 0, nValue - 40 );

	auto newColor = QColor( color );
	newColor.setHsv( nHue, nSaturation, nValue );
	return newColor;
}

QColor Skin::makeTextColorInactive( const QColor& color ) {
	int nHue, nSaturation, nValue;
	color.getHsv( &nHue, &nSaturation, &nValue );
	if ( nValue >= 130 ) {
		// TextInactive color is more white than black. Make it darker.
		nValue -= 55;
	} else {
		nValue += 55;
	}

	auto newColor = QColor( color );
	newColor.setHsv( nHue, nSaturation, nValue );
	return newColor;
}

void Skin::setPlayheadPen( QPainter* p, bool bHovered ) {

	QColor playheadColor( H2Core::Preferences::get_instance()->getTheme().m_color.m_playheadColor );
	if ( bHovered ) {
		playheadColor = Skin::makeTextColorInactive( playheadColor );
	}
	QPen pen ( playheadColor );
	pen.setWidth( 2 );
	
	p->setPen( pen );
	p->setRenderHint( QPainter::Antialiasing );
}

void Skin::drawPlayhead( QPainter* p, int x, int y, bool bHovered ) {

	const QPointF points[3] = {
		QPointF( x, y ),
		QPointF( x + Skin::nPlayheadWidth - 1, y ),
		QPointF( x + Skin::getPlayheadShaftOffset(),
				 y + Skin::nPlayheadHeight ),
	};

	QColor playheadColor( H2Core::Preferences::get_instance()->getTheme().m_color.m_playheadColor );
	if ( bHovered ) {
		playheadColor = Skin::makeTextColorInactive( playheadColor );
	}

	Skin::setPlayheadPen( p, bHovered );
	p->setBrush( playheadColor );
	p->drawPolygon( points, 3 );
	p->setBrush( Qt::NoBrush );
}

void Skin::drawStackedIndicator( QPainter* p, int x, int y,
								 const Skin::Stacked& stacked ) {

	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	const QPointF points[3] = {
		QPointF( x, y ),
		QPointF( x + 8, y + 6 ),
		QPointF( x, y + 12 )
	};

	QPen pen( Qt::black );
	pen.setWidth( 1 );
	
	QColor fillColor;
	switch ( stacked ) {
	case Skin::Stacked::None:
	case Skin::Stacked::Off:
		fillColor = QColor( 0, 0, 0 );
		fillColor.setAlpha( 0 );
		break;
	case Skin::Stacked::OffNext:
		fillColor = colorTheme.m_songEditor_stackedModeOffNextColor;
		break;
	case Skin::Stacked::On:
		fillColor = colorTheme.m_songEditor_stackedModeOnColor;
		break;
	case Skin::Stacked::OnNext:
		fillColor = colorTheme.m_songEditor_stackedModeOnNextColor;
		break;
	}

	p->setPen( pen );
	p->setBrush( fillColor );
	p->setRenderHint( QPainter::Antialiasing );
	p->drawPolygon( points, 3 );
	p->setBrush( Qt::NoBrush );
}

bool Skin::moreBlackThanWhite( const QColor& color ) {
	const int nThreshold = 128;
	int nHue, nSaturation, nValue;

	color.getHsv( &nHue, &nSaturation, &nValue );

	return nValue < nThreshold;
}
