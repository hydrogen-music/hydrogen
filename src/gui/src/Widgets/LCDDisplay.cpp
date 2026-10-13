/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "LCDDisplay.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

#include <core/Globals.h>
#include <core/Preferences/Theme.h>

#include <algorithm>

LCDDisplay::LCDDisplay( QWidget * pParent, const QSize& size, bool bFixedFont,
						bool bIsActive )
 : QLineEdit( pParent )
 , m_size( size )
 , m_bEntered( false )
 , m_bFixedFont( bFixedFont )
 , m_bUseRedFont( false )
 , m_bIsActive( bIsActive )
 , m_bTabularDigits( false )
 , m_nDigitCellWidth( 0 )
 , m_nTabularPixelSize( -1 )
{
	setReadOnly( ! bIsActive );
	setEnabled( bIsActive );
	if ( ! bIsActive ) {
		setFocusPolicy( Qt::NoFocus );
	}
	setAlignment( Qt::AlignCenter );
	setLocale( QLocale( QLocale::C, QLocale::AnyCountry ) );

	// Derive a set of scaling-dependent font sizes on the basis of
	// the default font size determined by Qt itself.
	QFont currentFont = font();
	int nStepSize = 2;

	m_fontPointSizes.resize( 3 );
	switch ( HydrogenApp::pPreferences()->getFontTheme()->m_fontSize ) {
	case H2Core::FontTheme::FontSize::Small:
		m_fontPointSizes[ 0 ] = currentFont.pointSize();
		break;
	case H2Core::FontTheme::FontSize::Large:
		m_fontPointSizes[ 0 ] = currentFont.pointSize() - 2 * nStepSize;
		break;
	default:
		m_fontPointSizes[ 0 ] = currentFont.pointSize() - nStepSize;
	}
	
	m_fontPointSizes[ 1 ] = m_fontPointSizes[ 0 ] + nStepSize;
	m_fontPointSizes[ 2 ] = m_fontPointSizes[ 0 ] + 2 * nStepSize;
	
	if ( ! size.isNull() && ! size.isEmpty() ) {
		adjustSize();
		setFixedSize( size );
	}

	updateFont();
	updateStyleSheet();

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &LCDDisplay::onPreferencesChanged );
}

LCDDisplay::~LCDDisplay() {
}

void LCDDisplay::setUseRedFont( bool bUseRedFont ) {
	if ( bUseRedFont != m_bUseRedFont ) {
		m_bUseRedFont = bUseRedFont;
		updateStyleSheet();
	}
}

void LCDDisplay::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;;

	setReadOnly( ! bIsActive );
	setEnabled( bIsActive );
	
	if ( ! bIsActive ) {
		setFocusPolicy( Qt::NoFocus );
	}
	else {
		setFocusPolicy( Qt::StrongFocus );
	}

	update();
}

void LCDDisplay::setTabularDigits( bool bTabular ) {
	if ( bTabular == m_bTabularDigits ) {
		return;
	}
	m_bTabularDigits = bTabular;

	// The mode takes ownership of the size set on the widget, so later
	// font updates can not let a foreign size trickle in.
	m_nTabularPixelSize = font().pixelSize();

	updateFont();
	updateStyleSheet();
	update();
}

void LCDDisplay::updateFont() {

	if ( m_bTabularDigits ) {
		// The "Item font" selected in the Preferences — the family all
		// other LCDDisplay widgets use. The font is rebuilt from just
		// the theme family and the captured size instead of copying
		// the current font, so neither an inherited nor a
		// stylesheet-applied font state can smuggle in a stale family
		// or size.
		const auto pFontTheme = HydrogenApp::pPreferences()->getFontTheme();
		QFont newFont;
		newFont.setFamily( pFontTheme->m_sLevel3FontFamily );
		if ( m_nTabularPixelSize > 0 ) {
			newFont.setPixelSize( m_nTabularPixelSize );
		}
		else {
			newFont = font();
			newFont.setFamily( pFontTheme->m_sLevel3FontFamily );
		}
		setFont( newFont );

		measureDigitCellWidth();
		return;
	}

	if ( m_bFixedFont ) {
		return;
	}

	const auto pFontTheme = HydrogenApp::pPreferences()->getFontTheme();

	int nIndex = 1;
	if ( pFontTheme->m_fontSize == H2Core::FontTheme::FontSize::Small ) {
		nIndex = 0;
	} else if ( pFontTheme->m_fontSize == H2Core::FontTheme::FontSize::Large ) {
		nIndex = 2;
	}

	QFont newFont = font();
	newFont.setFamily( pFontTheme->m_sLevel3FontFamily );
	newFont.setPointSize( m_fontPointSizes[ nIndex ] );
	setFont( newFont );
}

void LCDDisplay::updateStyleSheet() {
	const auto pColorTheme = HydrogenApp::pPreferences()->getColorTheme();

	QColor textColor, textColorActive;
	if ( m_bUseRedFont ) {
		textColor = pColorTheme->m_buttonRedColor;
		textColorActive = pColorTheme->m_buttonRedColor;
	} else {
		textColor = pColorTheme->m_windowTextColor;
		textColorActive = pColorTheme->m_widgetTextColor;
	}
	QColor backgroundColor = pColorTheme->m_windowColor;

	QColor backgroundColorActive = pColorTheme->m_widgetColor;

	// In tabular mode the text is painted manually in paintEvent();
	// the base line edit must not render it a second time. Degenerate
	// metrics (no digit cells) keep the natural rendering.
	if ( m_bTabularDigits && m_nDigitCellWidth > 0 ) {
		textColor = QColor( Qt::transparent );
		textColorActive = QColor( Qt::transparent );
	}

	QString sStyleSheet = QString( "\
QLineEdit:enabled { \
    color: %1; \
    background-color: %2; \
} \
QLineEdit:disabled { \
    color: %3; \
    background-color: %4; \
}" )
		.arg( textColorActive.name( QColor::HexArgb ) )
		.arg( backgroundColorActive.name() )
		.arg( textColor.name( QColor::HexArgb ) )
		.arg( backgroundColor.name() );

	// For fixed font displays we have to add the current font
	// parameters as well to avoid any inherited changes. The family
	// has to be quoted — a name containing a space (like the default
	// "Lucida Grande") would fail the stylesheet parser.
	//
	// In tabular mode the family must NOT be baked: a stylesheet
	// family rule overrides the widget font in every later
	// updateFont() call, and the display would stop following the
	// "Item font" selected in the Preferences. The family lives on
	// the QFont alone; only the size is pinned here.
	if ( m_bFixedFont && font().pixelSize() > 0 ) {
		if ( m_bTabularDigits ) {
			sStyleSheet.append( QString( "\
QLineEdit { \
    font-size: %1px; \
}" )
									.arg( font().pixelSize() ) );
		}
		else {
			sStyleSheet.append( QString( "\
QLineEdit { \
    font-size: %1px; \
    font-family: \"%2\"; \
}" )
									.arg( font().pixelSize() )
									.arg( font().family() ) );
		}
	}

	setStyleSheet( sStyleSheet );
}

QSize LCDDisplay::sizeHint() const {
	const QSize base = QLineEdit::sizeHint();
	if ( ! m_bTabularDigits || m_nDigitCellWidth <= 0 ) {
		return base;
	}

	// The digit cells are wider than the glyphs shown at any given
	// moment: reserve the cell-based advance of the current text
	// instead of its natural one, so the text never clips.
	const QFontMetrics metrics( font() );
	const QString sText = text();
	int nCellWidth = 0;
	for ( int i = 0; i < sText.size(); ++i ) {
		const QChar ch = sText.at( i );
		nCellWidth += ch.isDigit() ? m_nDigitCellWidth
								   : metrics.horizontalAdvance( ch );
	}

	QSize hint = base;
	hint.setWidth( std::max( base.width(),
							base.width() -
								metrics.horizontalAdvance( sText ) +
								nCellWidth ) );
	return hint;
}

void LCDDisplay::measureDigitCellWidth() {
	const QFontMetrics metrics( font() );
	int nMax = 0;
	for ( char c = '0'; c <= '9'; ++c ) {
		nMax = std::max( nMax, metrics.horizontalAdvance( QChar( c ) ) );
	}
	// A font without usable digits would collapse the cells to zero
	// and swallow the text; the negative sentinel makes the callers
	// fall back to the natural rendering instead.
	m_nDigitCellWidth = nMax > 0 ? nMax : -1;
}

void LCDDisplay::paintTabularText() {
	if ( m_nDigitCellWidth <= 0 ) {
		return;
	}

	const QString sText = text();
	if ( sText.isEmpty() ) {
		return;
	}

	const auto pColorTheme = HydrogenApp::pPreferences()->getColorTheme();
	QColor color;
	if ( m_bUseRedFont ) {
		color = pColorTheme->m_buttonRedColor;
	}
	else if ( m_bIsActive ) {
		color = pColorTheme->m_widgetTextColor;
	}
	else {
		color = pColorTheme->m_windowTextColor;
	}

	const QFontMetrics metrics( font() );

	// The base line edit's own text is hidden in this mode, so the
	// cells are laid out within the widget rect reduced by the text
	// margins — a deterministic inset, independent of the style's
	// frame metrics.
	const QMargins margins = textMargins();
	const QRect textRect = rect().adjusted(
		margins.left(), margins.top(), -margins.right(), -margins.bottom() );

	QPainter painter( this );
	painter.setFont( font() );
	painter.setPen( color );

	// Centered like the base line edit (the widget enforces
	// Qt::AlignCenter in its constructor).
	int nTotalWidth = 0;
	for ( int i = 0; i < sText.size(); ++i ) {
		const QChar ch = sText.at( i );
		nTotalWidth += ch.isDigit() ? m_nDigitCellWidth
									: metrics.horizontalAdvance( ch );
	}
	int nX = textRect.left() + ( textRect.width() - nTotalWidth ) / 2;
	const int nBaseline = textRect.top() +
		( textRect.height() + metrics.ascent() - metrics.descent() ) / 2;

	for ( int i = 0; i < sText.size(); ++i ) {
		const QChar ch = sText.at( i );
		if ( ch.isDigit() ) {
			// Each glyph centered in its cell — the position of every
			// digit is independent of the digits shown around it.
			const int nAdvance = metrics.horizontalAdvance( ch );
			painter.drawText(
				QPoint( nX + ( m_nDigitCellWidth - nAdvance ) / 2, nBaseline ),
				QString( ch ) );
			nX += m_nDigitCellWidth;
		}
		else {
			painter.drawText( QPoint( nX, nBaseline ), QString( ch ) );
			nX += metrics.horizontalAdvance( ch );
		}
	}
}

void LCDDisplay::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		updateFont();
		updateStyleSheet();
	}
}

void LCDDisplay::paintEvent( QPaintEvent *ev ) {
	const auto pColorTheme = HydrogenApp::pPreferences()->getColorTheme();

	QLineEdit::paintEvent( ev );
	updateFont();

	if ( m_bTabularDigits ) {
		paintTabularText();
	}

	// Hovering highlights
	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);

		QColor colorHighlightActive;
		if ( m_bIsActive ) {
			colorHighlightActive = pColorTheme->m_highlightColor;
		} else {
			colorHighlightActive = pColorTheme->m_lightColor;
		}

		// If the mouse is placed on the widget but the user hasn't
		// clicked it yet, the highlight will be done more transparent to
		// indicate that keyboard inputs are not accepted yet.
		if ( ! hasFocus() ) {
			colorHighlightActive.setAlpha( 150 );
		}

		QPen pen;
		pen.setColor( colorHighlightActive );
		pen.setWidth( 3 );
		painter.setPen( pen );
		painter.drawRoundedRect( QRect( 0, 0, width() - 1, height() - 1 ), 3, 3 );
	}
}

#ifdef H2CORE_HAVE_QT6
void LCDDisplay::enterEvent( QEnterEvent *ev ) {
#else
void LCDDisplay::enterEvent( QEvent *ev ) {
#endif
	QLineEdit::enterEvent( ev );
	m_bEntered = true;
	update();
}

void LCDDisplay::leaveEvent( QEvent* ev ) {
	QLineEdit::leaveEvent( ev );
	m_bEntered = false;
	update();
}
