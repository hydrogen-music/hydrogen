/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "LCDSpinBox.h"
#include "../HydrogenApp.h"
#include <core/Globals.h>
#include <core/Preferences.h>

const char* LCDSpinBox::__class_name = "LCDSpinBox";

// used in PlayerControl
LCDSpinBox::LCDSpinBox( QWidget *pParent, QSize size, Type type, double fMin, double fMax )
 : QDoubleSpinBox( pParent )
 , Object( __class_name )
 , m_size( size )
 , m_type( type )
 , m_bEntered( false )
 , m_kind( Kind::Default )
{
	setFocusPolicy( Qt::ClickFocus );
	
	if ( ! size.isNull() ) {
		adjustSize();
		setFixedSize( size );
	}

	auto pPref = H2Core::Preferences::get_instance();
	
	m_lastHighlightColor = pPref->getDefaultUIStyle()->m_highlightColor;
	m_lastAccentColor = pPref->getDefaultUIStyle()->m_accentColor;
	m_lastSpinBoxSelectionColor = pPref->getDefaultUIStyle()->m_spinBoxSelectionColor;
	m_lastSpinBoxSelectionTextColor = pPref->getDefaultUIStyle()->m_spinBoxSelectionTextColor;
	m_lastAccentTextColor = pPref->getDefaultUIStyle()->m_accentTextColor;

	updateStyleSheet();
		
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &LCDSpinBox::onPreferencesChanged );
		
	setMaximum( fMax );
	setMinimum( fMin );
	setValue( fMin );
}

LCDSpinBox::~LCDSpinBox() {
}

void LCDSpinBox::wheelEvent( QWheelEvent *ev ) {
	static float fCumulatedDelta;
	
	if ( m_kind == Kind::PatternSizeDenominator ) {

		// Cumulate scroll positions to provide a native feeling for
		// fine-grained mouse wheel and touch pads.
		fCumulatedDelta += ev->angleDelta().y();
		
		if ( std::fabs( fCumulatedDelta ) >= 120 ) {
			fCumulatedDelta = 0;
		 
			double fNextValue = nextValueInPatternSizeDenominator( ev->angleDelta().y() > 0, ev->modifiers() == Qt::ControlModifier );


			if ( fNextValue == 0 ) {
				ERRORLOG( QString( "Couldn't find next value for input: %1" ).arg( value() ) );
				return;
			}

			if ( ev->angleDelta().y() > 0 ) {
				setValue( fNextValue + 1 );
			} else {
				setValue( fNextValue - 1 );
			}
		}
	} else if (	m_kind == Kind::PatternSizeNumerator ) {
		QDoubleSpinBox::wheelEvent( ev );
		if ( value() < 1 ) {
			setValue( 1 );
		}
	} else {
	
		QDoubleSpinBox::wheelEvent( ev );

	}
}

void LCDSpinBox::keyPressEvent( QKeyEvent *ev ) {
	if ( m_kind == Kind::PatternSizeDenominator &&
		 ( ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down ||
		   ev->key() == Qt::Key_PageUp || ev->key() == Qt::Key_PageDown ) ) {
		 double fNextValue;

		 if ( ev->key() == Qt::Key_Up ) {
			 fNextValue = nextValueInPatternSizeDenominator( true, false );
		 } else if ( ev->key() == Qt::Key_Down ) {
			 fNextValue = nextValueInPatternSizeDenominator( false, false );
		 } else if ( ev->key() == Qt::Key_PageUp ) {
			 fNextValue = nextValueInPatternSizeDenominator( true, true );
		 } else if ( ev->key() == Qt::Key_PageDown ) {
			 fNextValue = nextValueInPatternSizeDenominator( false, true );
		 }

		 if ( fNextValue == 0 ) {
			 ERRORLOG( QString( "Couldn't find next value for input: %1" ).arg( value() ) );
			 return;
		 }

		 setValue( fNextValue );
	 
		 QDoubleSpinBox::keyPressEvent( ev );
		 
	 } else if ( m_kind == Kind::PatternSizeNumerator ) {
		
		QDoubleSpinBox::keyPressEvent( ev );
		
		if ( value() < 1 ) {
			setValue( 1 );
		}
		
	} else {
	 
		 QDoubleSpinBox::keyPressEvent( ev );
	}
}

double LCDSpinBox::nextValueInPatternSizeDenominator( bool bUp, bool bAccelerated ) {

	// Determine the next value.
	std::vector vChoices{ 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 192 };
	bool bContained;

	double fNextValue;
	double fOffset;
	if ( bAccelerated ) {
		fOffset = 10;
	} else {
		fOffset = 1;
	}
	for ( int ii = 0; ii < vChoices.size(); ii++ ) {
		if ( vChoices[ ii ] == value() ) {

			if ( bUp ) {
				if ( ii < vChoices.size() - 1 ) {
					fNextValue = vChoices[ ii + 1 ] - fOffset;
				} else {
					fNextValue = vChoices[ ii ] - fOffset;
				}
				if ( fNextValue < 0 ) {
					fNextValue = 2;
				}
			} else {
				if ( ii > 0 ) {
					fNextValue = vChoices[ ii - 1 ] + fOffset;
				} else {
					fNextValue = vChoices[ ii ] + fOffset;
				}
			}
		}
	}

	return fNextValue;
}

QString LCDSpinBox::textFromValue( double fValue ) const {
	QString result;
	if ( fValue == -1.0 ) {
		result = "off";
	} else {
		if ( m_type == Type::Int ) {
			result = QString( "%1" ).arg( fValue, 0, 'f', 0 );
		} else {
			result = QString( "%1" ).arg( fValue, 0, 'f', 2 );
		}
	}

	return result;
}

QValidator::State LCDSpinBox::validate( QString &text, int &pos ) const {
	if ( m_kind == Kind::PatternSizeDenominator ) {
		std::vector vChoices{ "1", "2", "3", "4", "6", "8", "12", "16", "24", "32", "48", "64", "96", "192" };
		std::vector vCandidates1{ "1", "2", "3", "4", "6", "9" };
		QString sCandidate2( "19" );
		bool bContained = false;
		bool bIsCandidate = false;
		for ( const auto& ii : vChoices ) {
			if ( ii == text ) {
				bContained = true;
			}
		}
		for ( const auto& ii : vCandidates1 ) {
			if ( ii == text.left( 1 ) ) {
				bIsCandidate = true;
			}
		}
		if ( sCandidate2 == text.left( 2 ) ) {
			bIsCandidate = true;
		}
		
		if ( bContained ) {
			return QValidator::Acceptable;
		} else if ( bIsCandidate ) {
			return QValidator::Intermediate;
		} else {
			return QValidator::Invalid;
		}
	}
	return QDoubleSpinBox::validate( text, pos );
}

double LCDSpinBox::valueFromText( const QString& sText ) const {

	double fResult;
	
	if ( sText == "off" ){
		fResult = -1.0;
	} else {
		fResult = QDoubleSpinBox::valueFromText( sText );
	}

	return fResult;
}

void LCDSpinBox::paintEvent( QPaintEvent *ev ) {

	QDoubleSpinBox::paintEvent( ev );

	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);
	
		QColor colorHighlightActive = m_lastHighlightColor;

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
		painter.drawRoundedRect( QRect( 0, 0, m_size.width(), m_size.height() ), 3, 3 );
	}
}

void LCDSpinBox::enterEvent( QEvent* ev ) {
	QDoubleSpinBox::enterEvent( ev );
	m_bEntered = true;
}

void LCDSpinBox::leaveEvent( QEvent* ev ) {
	QDoubleSpinBox::leaveEvent( ev );
	m_bEntered = false;
}

void LCDSpinBox::updateStyleSheet() {
	setStyleSheet( QString( "\
QDoubleSpinBox, QSpinBox { \
    color: %1; \
    background-color: %2; \
    selection-color: %3; \
    selection-background-color: %4; \
}" )
				   .arg( m_lastAccentTextColor.name() )
				   .arg( m_lastAccentColor.name() )
				   .arg( m_lastSpinBoxSelectionTextColor.name() )
				   .arg( m_lastSpinBoxSelectionColor.name() ) );
}

void LCDSpinBox::onPreferencesChanged( bool bAppearanceOnly ) {
	
	auto pPref = H2Core::Preferences::get_instance();

	if ( m_lastHighlightColor != pPref->getDefaultUIStyle()->m_highlightColor ||
		 m_lastAccentColor != pPref->getDefaultUIStyle()->m_accentColor ||
		 m_lastSpinBoxSelectionColor != pPref->getDefaultUIStyle()->m_spinBoxSelectionColor ||
		 m_lastSpinBoxSelectionTextColor != pPref->getDefaultUIStyle()->m_spinBoxSelectionTextColor ||
		 m_lastAccentTextColor != pPref->getDefaultUIStyle()->m_accentTextColor ) {
		
		m_lastHighlightColor = pPref->getDefaultUIStyle()->m_highlightColor;
		m_lastAccentColor = pPref->getDefaultUIStyle()->m_accentColor;
		m_lastSpinBoxSelectionColor = pPref->getDefaultUIStyle()->m_spinBoxSelectionColor;
		m_lastSpinBoxSelectionTextColor = pPref->getDefaultUIStyle()->m_spinBoxSelectionTextColor;
		m_lastAccentTextColor = pPref->getDefaultUIStyle()->m_accentTextColor;

		updateStyleSheet();
		update();
	}
}
