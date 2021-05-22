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


#include "../Skin.h"
#include "Fader.h"
#include "LCD.h"
#include "MidiSenseWidget.h"

#include <QtGui>
#include <QtWidgets>

#include <core/Globals.h>

const char* Fader::__class_name = "Fader";

Fader::Fader( QWidget *pParent, Type type, QString sBaseTooltip, bool bUseIntSteps, bool bWithoutKnob, float fMin, float fMax )
	: WidgetWithInput( pParent,
					   bUseIntSteps,
					   sBaseTooltip,
					   1, // nScrollSpeed
					   5, // nScrollSpeedFast
					   fMin,
					   fMax  )
	, Object( __class_name )
	, m_type( type )
	, m_bWithoutKnob( bWithoutKnob )
	, m_fPeakValue_L( 0.01f )
	, m_fPeakValue_R( 0.01f )
	, m_fMinPeak( 0.01f )
	, m_fMaxPeak( 1.0 )
{
	m_fDefaultValue = m_fMax;

	if ( type == Type::Vertical ){ 
		m_nWidgetWidth = 116;
		m_nWidgetHeight = 23;
	} else if ( type == Type::Master ) {
		m_nWidgetWidth = 34;
		m_nWidgetHeight = 190;
	} else {
		m_nWidgetWidth = 23;
		m_nWidgetHeight = 116;
	}
	setFixedSize( m_nWidgetWidth, m_nWidgetHeight );

	// Background image
	QString sBackgroundPath;
	QString sKnobPath;
	if ( type == Type::Master ) {
		sBackgroundPath = Skin::getSvgImagePath() + "/fader_master.svg";
		sKnobPath = Skin::getSvgImagePath() + "/fader_knob.svg";
	} else if ( type == Type::Vertical ) {
		sBackgroundPath = Skin::getSvgImagePath() + "/fader_vertical.svg";
		sKnobPath = Skin::getSvgImagePath() + "/fader_knob_vertical.svg";
	} else {
		sBackgroundPath = Skin::getSvgImagePath() + "/fader.svg";
		sKnobPath = Skin::getSvgImagePath() + "/fader_knob.svg";
	}
		
	QFile fileBackground( sBackgroundPath );
	if ( fileBackground.exists() ) {
		m_pBackground = new QSvgRenderer( sBackgroundPath, this );
	} else {
		m_pBackground = nullptr;
		ERRORLOG( QString( "Unable to load background image [%1]" ).arg( sBackgroundPath ) );
	}

	QFile fileKnob( sKnobPath );
	if ( fileKnob.exists() ) {
		m_pKnob = new QSvgRenderer( sKnobPath, this );
	} else {
		m_pKnob = nullptr;
		ERRORLOG( QString( "Unable to load knob image [%1]" ).arg( sKnobPath ) );
	}
	
	resize( m_nWidgetWidth, m_nWidgetHeight );
	if ( type == Type::Vertical ) {
		QTransform transform;
		transform.rotate(90);
	}
}

Fader::~Fader() {
}

void Fader::mouseMoveEvent( QMouseEvent *ev )
{
	if ( m_bIgnoreMouseMove ) {
		return;
	}

	float fValue;
	if ( m_type == Type::Vertical ) {
		fValue = static_cast<float>( ev->x() ) / static_cast<float>( width() );
	} else {
		fValue = static_cast<float>( height() - ev->y() ) / static_cast<float>( height() );
	}
	if ( fValue > 1. ) { // for QToolTip text validity
		fValue = 1.;
	} else if ( fValue < 0. ) {
		fValue = 0.;
	}	

	fValue = fValue * ( m_fMax - m_fMin ) + m_fMin;

	setValue( fValue );
	QToolTip::showText( ev->globalPos(), QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
}

void Fader::paintEvent( QPaintEvent *ev)
{

	QPainter painter(this);

	if ( m_bIsActive ) {
		float fFaderTopLeftX_L, fFaderTopLeftY_L, fFaderTopLeftX_R,
			fFaderTopLeftY_R, fFaderWidth, fFaderHeight, fPeak_L, fPeak_R;

		if ( m_type == Type::Master ) {
			fFaderTopLeftX_L = 1;
			fFaderTopLeftY_L = 2;
			fFaderTopLeftX_R = 12;
			fFaderTopLeftY_R = 2;
			fFaderWidth = 6,8;
			fFaderHeight = 186;
			fPeak_L = ( m_fPeakValue_L - m_fMinPeak ) / ( m_fMaxPeak - m_fMinPeak ) * fFaderHeight;
			fPeak_R = ( m_fPeakValue_R - m_fMinPeak ) / ( m_fMaxPeak - m_fMinPeak ) * fFaderHeight;
		} else if ( m_type == Type::Vertical ) {
			fFaderTopLeftX_L = 1.5;
			fFaderTopLeftY_L = 2;
			fFaderTopLeftX_R = 1.5;
			fFaderTopLeftY_R = 14.5;
			fFaderWidth = 114;
			fFaderHeight = 6.5;
			fPeak_L = ( m_fPeakValue_L - m_fMinPeak ) / ( m_fMaxPeak - m_fMinPeak ) * fFaderWidth;
			fPeak_R = ( m_fPeakValue_R - m_fMinPeak ) / ( m_fMaxPeak - m_fMinPeak ) * fFaderWidth;
		} else {
			fFaderTopLeftX_L = 1.5;
			fFaderTopLeftY_L = 1.7;
			fFaderTopLeftX_R = 15.5;
			fFaderTopLeftY_R = 1.7;
			fFaderWidth = 6.5;
			fFaderHeight = 114;
			fPeak_L = ( m_fPeakValue_L - m_fMinPeak ) / ( m_fMaxPeak - m_fMinPeak ) * fFaderHeight;
			fPeak_R = ( m_fPeakValue_R - m_fMinPeak ) / ( m_fMaxPeak - m_fMinPeak ) * fFaderHeight;
		}

		QLinearGradient gradient;
		if ( m_type == Type::Vertical ) {
			gradient = QLinearGradient( fFaderTopLeftX_L, fFaderTopLeftY_L, fFaderTopLeftX_L + fFaderWidth, fFaderTopLeftY_L );
		gradient.setColorAt( 0.0, Qt::green );
		gradient.setColorAt( 0.6, Qt::green );
		gradient.setColorAt( 0.65, Qt::yellow );
		gradient.setColorAt( 0.85, Qt::yellow );
		gradient.setColorAt( 0.9, Qt::red );
		gradient.setColorAt( 1.0, Qt::red );
		} else {
			gradient = QLinearGradient( fFaderTopLeftX_L, fFaderTopLeftY_L, fFaderTopLeftX_L, fFaderTopLeftY_L + fFaderHeight );	
		gradient.setColorAt( 1.0, Qt::green );
		gradient.setColorAt( 0.4, Qt::green );
		gradient.setColorAt( 0.35, Qt::yellow );
		gradient.setColorAt( 0.15, Qt::yellow );
		gradient.setColorAt( 0.1, Qt::red );
		gradient.setColorAt( 0.0, Qt::red );
		}

		if ( m_type == Type::Vertical ) {
			painter.fillRect( QRectF( fFaderTopLeftX_L, fFaderTopLeftY_L, fPeak_L, fFaderHeight ), QBrush( gradient ) );
			painter.fillRect( QRectF( fFaderTopLeftX_R, fFaderTopLeftY_R, fPeak_R, fFaderHeight ), QBrush( gradient ) );
		} else {
			painter.fillRect( QRectF( fFaderTopLeftX_L, fFaderTopLeftY_L + fFaderHeight - fPeak_L, fFaderWidth, fPeak_L ), QBrush( gradient ) );
			painter.fillRect( QRectF( fFaderTopLeftX_R, fFaderTopLeftY_R + fFaderHeight - fPeak_R, fFaderWidth, fPeak_R ), QBrush( gradient ) );
		}
	}
	// 	float peak_L = m_fPeakValue_L * 190.0;
	// 	uint offset_L = (uint)( 190.0 - peak_L );
	// 	painter.drawPixmap( QRect( 0, offset_L, 9, 190 - offset_L), m_leds, QRect( 0, offset_L, 9, 190 - offset_L) );

	// 	float peak_R = m_fPeakValue_R * 190.0;
	// 	uint offset_R = (uint)( 190.0 - peak_R );
	// 	painter.drawPixmap( QRect( 9, offset_R, 9, 190 - offset_R), m_leds, QRect( 9, offset_R, 9, 190 - offset_R) );
	// } else {
		
	// 	float realPeak_L = m_fPeakValue_L - m_fMinPeak;
	// 	int peak_L = 116 - ( realPeak_L / ( m_fMaxPeak - m_fMinPeak ) ) * 116.0;

	// 	if ( peak_L > 116 ) {
	// 		peak_L = 116;
	// 	}
	// 	if ( m_type == Type::Vertical ) {
	// 		painter.drawPixmap( QRect( 0, 0, 116 - peak_L, 11 ), m_leds, QRect( 0, 0, 116 - peak_L, 11 ) );
	// 	} else {
	// 		painter.drawPixmap( QRect( 0, peak_L, 11, 116 - peak_L ), m_leds, QRect( 0, peak_L, 11, 116 - peak_L ) );
	// 	}

	// 	float realPeak_R = m_fPeakValue_R - m_fMinPeak;
	// 	int peak_R = 116 - ( realPeak_R / ( m_fMaxPeak - m_fMinPeak ) ) * 116.0;
	// 	if ( peak_R > 116 ) {
	// 		peak_R = 116;
	// 	}

	// 	if ( m_type == Type::Vertical ) {
	// 		painter.drawPixmap( QRect( 0, 11, 116 - peak_R, 11 ), m_leds, QRect( 0, 11, 116 - peak_R, 11 ) );
	// 	} else {
	// 		painter.drawPixmap( QRect( 11, peak_R, 11, 116 - peak_R ), m_leds, QRect( 11, peak_R, 11, 116 - peak_R ) );
	// 	}
	// }
	
	// Draws the outline of the fader on top of the colors indicating
	// the peak value.
	if ( m_pBackground != nullptr ) {
		m_pBackground->render( &painter );
	}
	
	if ( m_bIsActive && m_bWithoutKnob == false ) {
		float fVal = ( m_fValue - m_fMin ) / ( m_fMax - m_fMin );
		float fKnobHeight, fKnobWidth, fKnobX, fKnobY;

		if ( m_type == Type::Vertical ) {
			fKnobHeight = 15;
			fKnobWidth = 29;
			fKnobX = 116.0 - ( 101 * ( 1 - fVal ) ) - fKnobHeight;
			fKnobY = 4;
		} else {
			fKnobHeight = 29;

			if ( m_type == Type::Master ) {
				fKnobWidth = 19;
				fKnobY = 190.0 - ( 159.0 * fVal ) - fKnobHeight;
				fKnobX = 21;
			} else {
				fKnobWidth = 15;
				fKnobY = 116.0 - ( 86.0 * fVal ) - fKnobHeight;
				fKnobX = 4;
			}
		}

		if ( m_pKnob != nullptr ) {
			m_pKnob->render( &painter, QRectF( fKnobX, fKnobY, fKnobWidth, fKnobHeight) );
		}
	}
}


void Fader::setPeak_L( float fPeak )
{
	if ( m_fPeakValue_L == fPeak ) {
		return;
	}

	if ( m_bUseIntSteps && std::fmod( fPeak, 1.0 ) != 0.0 ) {
		___WARNINGLOG( QString( "As widget is set to use integer values only the supply  value [%1] will be rounded to [%2] " )
					.arg( fPeak )
					.arg( std::round( fPeak ) ) );
		fPeak = std::round( fPeak );
	}
	if ( fPeak <  m_fMinPeak ) {
		fPeak = m_fMinPeak;
	}
	else if ( fPeak > m_fMaxPeak ) {
		fPeak = m_fMaxPeak;
	}

	if ( m_fPeakValue_L != fPeak) {
		m_fPeakValue_L = fPeak;
		update();
	}
}

void Fader::setPeak_R( float fPeak )
{
	if ( m_fPeakValue_R == fPeak ) {
		return;
	}

	if ( m_bUseIntSteps && std::fmod( fPeak, 1.0 ) != 0.0 ) {
		___WARNINGLOG( QString( "As widget is set to use integer values only the supply  value [%1] will be rounded to [%2] " )
					.arg( fPeak )
					.arg( std::round( fPeak ) ) );
		fPeak = std::round( fPeak );
	}
	if ( fPeak <  m_fMinPeak ) {
		fPeak = m_fMinPeak;
	}
	else if ( fPeak > m_fMaxPeak ) {
		fPeak = m_fMaxPeak;
	}

	if ( m_fPeakValue_R != fPeak ) {
		m_fPeakValue_R = fPeak;
		update();
	}
}

void Fader::setMaxPeak( float fMax )
{
	if ( m_fMaxPeak == fMax ) {
		return;
	}

	if ( m_bUseIntSteps && std::fmod( fMax, 1.0 ) != 0.0 ) {
		___WARNINGLOG( QString( "As widget is set to use integer values only the supply  value [%1] will be rounded to [%2] " )
					.arg( fMax )
					.arg( std::round( fMax ) ) );
		fMax = std::round( fMax );
	}

	if ( fMax <= m_fMinPeak ) {
		___ERRORLOG( QString( "Supplied value [%1] must be larger than minimal one [%2]" )
					 .arg( fMax ).arg( m_fMinPeak ) );
		return;
	}
	
	if ( m_fMaxPeak != fMax ) {
		m_fMaxPeak = fMax;

		if ( m_fPeakValue_L > fMax ) {
			setPeak_L( fMax );
		}
		if ( m_fPeakValue_R > fMax ) {
			setPeak_R( fMax );
		}
	}
}

void Fader::setMinPeak( float fMin )
{
	if ( m_fMinPeak == fMin ) {
		return;
	}

	if ( m_bUseIntSteps && std::fmod( fMin, 1.0 ) != 0.0 ) {
		___WARNINGLOG( QString( "As widget is set to use integer values only the supply  value [%1] will be rounded to [%2] " )
					.arg( fMin )
					.arg( std::round( fMin ) ) );
		fMin = std::round( fMin );
	}

	if ( fMin >= m_fMaxPeak ) {
		___ERRORLOG( QString( "Supplied value [%1] must be smaller than maximal one [%2]" )
					 .arg( fMin ).arg( m_fMaxPeak ) );
		return;
	}
	
	if ( m_fMinPeak != fMin ) {
		m_fMinPeak = fMin;

		if ( m_fPeakValue_L < fMin ) {
			setPeak_L( fMin );
		}
		if ( m_fPeakValue_R < fMin ) {
			setPeak_R( fMin );
		}
	}
}
