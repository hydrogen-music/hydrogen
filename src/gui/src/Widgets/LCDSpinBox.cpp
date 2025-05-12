/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "LCDSpinBox.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include <core/Globals.h>
#include <core/Preferences/Preferences.h>

// used in PlayerControl
LCDSpinBox::LCDSpinBox( QWidget *pParent, QSize size, Type type, double fMin, double fMax, bool bModifyOnChange, bool bMinusOneAsOff )
 : QDoubleSpinBox( pParent )
 , m_size( size )
 , m_bEntered( false )
 , m_kind( Kind::Default )
 , m_bIsActive( true )
 , m_bModifyOnChange( bModifyOnChange )
 , m_bMinusOneAsOff( bMinusOneAsOff )
{
	setFocusPolicy( Qt::ClickFocus );
	setLocale( QLocale( QLocale::C, QLocale::AnyCountry ) );
	
	if ( size.isNull() || size.isEmpty() ) {
		m_size = sizeHint();
	}
	adjustSize();
	setFixedSize( m_size );

	setType( type );
	
	updateStyleSheet();

	connect( this, SIGNAL(valueChanged(double)), this,
                        SLOT(valueChanged(double)));
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &LCDSpinBox::onPreferencesChanged );

	setMaximum( fMax );
	setMinimum( fMin );
	setValue( fMin );
}

LCDSpinBox::~LCDSpinBox() {
}

void LCDSpinBox::setType( Type type ) {
	m_type = type;

	if ( type == Type::Int ) {
		setDecimals( 0 );
	}
	else {
		setDecimals( std::numeric_limits<double>::max_exponent );
	}
}

void LCDSpinBox::setSize( QSize size ) {
	m_size = size;
	
	setFixedSize( size );
	adjustSize();
}

void LCDSpinBox::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;
	
	update();

	setEnabled( bIsActive );
	setReadOnly( ! bIsActive );
}

void LCDSpinBox::wheelEvent( QWheelEvent *ev ) {
	static float fCumulatedDelta;

	double fOldValue = value();

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

	if ( fOldValue != value() && m_bModifyOnChange ) {
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}
}

void LCDSpinBox::keyPressEvent( QKeyEvent *ev ) {
	double fOldValue = value();

	// Pass Undo/Redo commands up to the parent. In addition, pause and play
	// button will be passed to the parent too.
	if ( ev->matches( QKeySequence::StandardKey::Undo )
		 || ev->matches( QKeySequence::StandardKey::Redo )
		 || ev->key() == Qt::Key_Space ) {
		ev->ignore();
		return;
	}

	if ( m_kind == Kind::PatternSizeDenominator &&
		 ( ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down ||
		   ev->key() == Qt::Key_PageUp || ev->key() == Qt::Key_PageDown ) ) {
		 double fNextValue;

		 if ( ev->key() == Qt::Key_Up ) {
			 fNextValue = nextValueInPatternSizeDenominator( true, false );
		 }
		 else if ( ev->key() == Qt::Key_Down ) {
			 fNextValue = nextValueInPatternSizeDenominator( false, false );
		 }
		 else if ( ev->key() == Qt::Key_PageUp ) {
			 fNextValue = nextValueInPatternSizeDenominator( true, true );
		 }
		 else if ( ev->key() == Qt::Key_PageDown ) {
			 fNextValue = nextValueInPatternSizeDenominator( false, true );
		 }

		 if ( fNextValue == 0 ) {
			 ERRORLOG( QString( "Couldn't find next value for input: %1" ).arg( value() ) );
			 return;
		 }

		 setValue( fNextValue );
	 
		 QDoubleSpinBox::keyPressEvent( ev );
	}
	else if ( m_kind == Kind::PatternSizeNumerator ) {
		
		QDoubleSpinBox::keyPressEvent( ev );
		
		if ( value() < 1 ) {
			setValue( 1 );
		}
	}
	else {
		 QDoubleSpinBox::keyPressEvent( ev );
	}
	
	if ( fOldValue != value() && m_bModifyOnChange ) {
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}
}

double LCDSpinBox::nextValueInPatternSizeDenominator( bool bUp, bool bAccelerated ) {

	// Determine the next value.
	std::vector vChoices{ 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 192 };

	double fNextValue = 1.0;
	double fOffset = 0.0;
	
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
	if ( m_type == Type::Int && m_bMinusOneAsOff &&
		 fValue == -1.0 ) {
		result = "off";
	} else {
		if ( m_type == Type::Int ) {
			result = QString( "%1" ).arg( fValue, 0, 'f', 0 );
		}
		else {
			result = QString( "%1" ).arg( fValue ) ;
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

void LCDSpinBox::setValue( double fValue, H2Core::Event::Trigger trigger ) {

	if ( value() == fValue && ! cleanText().isEmpty() ) {
		return;
	}

	if ( trigger == H2Core::Event::Trigger::Suppress ) {
		blockSignals( true );
	}

	QDoubleSpinBox::setValue( fValue );

	if ( trigger == H2Core::Event::Trigger::Suppress ) {
		blockSignals( false );
	}
}

bool LCDSpinBox::event( QEvent* ev ) {

	if ( ev->type() == QEvent::KeyPress && dynamic_cast<QKeyEvent*>( ev)->key() == Qt::Key_Slash ) {
		emit slashKeyPressed();
		return 0;
	}

	return QDoubleSpinBox::event( ev );
}

void LCDSpinBox::mousePressEvent( QMouseEvent* ev ) {
	double fOldValue = value();

	QDoubleSpinBox::mousePressEvent( ev );
	
	if ( fOldValue != value() && m_bModifyOnChange ) {
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}
}

void LCDSpinBox::mouseMoveEvent( QMouseEvent* ev ) {
	double fOldValue = value();

	QDoubleSpinBox::mouseMoveEvent( ev );
	
	if ( fOldValue != value() && m_bModifyOnChange ) {
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}
}

void LCDSpinBox::mouseReleaseEvent( QMouseEvent* ev ) {
	double fOldValue = value();

	QDoubleSpinBox::mouseReleaseEvent( ev );
	
	if ( fOldValue != value() && m_bModifyOnChange ) {
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}
}

void LCDSpinBox::paintEvent( QPaintEvent *ev ) {
	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	QDoubleSpinBox::paintEvent( ev );

	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);
	

		QColor colorHighlightActive;
		if ( m_bIsActive ) {
			colorHighlightActive = theme.m_color.m_highlightColor;
		} else {
			colorHighlightActive = theme.m_color.m_lightColor;
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
		painter.drawRoundedRect( QRect( 0, 0, width(), height() ), 3, 3 );
	}
}

void LCDSpinBox::enterEvent( QEnterEvent* ev ) {
	QDoubleSpinBox::enterEvent( ev );
	m_bEntered = true;
}

void LCDSpinBox::leaveEvent( QEvent* ev ) {
	QDoubleSpinBox::leaveEvent( ev );
	m_bEntered = false;
}

void LCDSpinBox::updateStyleSheet() {
	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	QColor spinBoxColor = theme.m_color.m_spinBoxColor;
	QColor spinBoxTextColor = theme.m_color.m_spinBoxTextColor;
	QColor selectionColor = spinBoxColor.darker( 120 );

	QColor spinBoxInactiveColor =
		Skin::makeWidgetColorInactive( spinBoxColor );
	QColor spinBoxTextInactiveColor =
		Skin::makeTextColorInactive( spinBoxTextColor );
	QColor selectionInactiveColor =
		Skin::makeWidgetColorInactive( selectionColor );

	
	setStyleSheet( QString( "\
QAbstractSpinBox:enabled { \
    color: %1; \
    background-color: %2; \
    selection-color: %1; \
    selection-background-color: %3; \
} \
QAbstractSpinBox:disabled { \
    color: %4; \
    background-color: %5; \
    selection-color: %4; \
    selection-background-color: %6; \
}" )
				   .arg( spinBoxTextColor.name() )
				   .arg( spinBoxColor.name() )
				   .arg( selectionColor.name() )
				   .arg( spinBoxTextInactiveColor.name() )
				   .arg( spinBoxInactiveColor.name() )
				   .arg( selectionInactiveColor.name() ) );
}

void LCDSpinBox::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateStyleSheet();
	}
}

void LCDSpinBox::valueChanged( double fNewValue ) {
	if ( m_type == Type::Int ) {
		emit valueChanged( static_cast<int>(fNewValue) );
		emit valueAdjusted();
	}
}
