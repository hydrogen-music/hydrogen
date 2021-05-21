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
	, m_fPeakValue_L( 0.0 )
	, m_fPeakValue_R( 0.0 )
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
	QString background_path;
	QString leds_path;
	QString knob_path;
	if ( type == Type::Master ) {
		background_path = Skin::getImagePath() + "/mixerPanel/masterMixer_background.png";
		leds_path = Skin::getImagePath() + "/mixerPanel/masterMixer_leds.png";
		knob_path = Skin::getImagePath() + "/mixerPanel/fader_knob.png";
	} else {
		background_path = Skin::getImagePath() + "/mixerPanel/fader_background.png";
		leds_path = Skin::getImagePath()  + "/mixerPanel/fader_leds.png";
		knob_path = Skin::getImagePath() + "/mixerPanel/fader_knob.png";
	}
	
	bool ok = m_back.load( background_path );
	if( ok == false ) {
		ERRORLOG("Fader: Error loading pixmap");
	}
	ok = m_leds.load( leds_path );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}
	ok = m_knob.load( knob_path );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
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
		fValue = static_cast<float>( ev->x() ) - static_cast<float>( width() );
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
	QToolTip::showText( ev->globalPos(), QString( "%1" ).arg( fValue, 0, 'f', 2 ) , this );
}

void Fader::paintEvent( QPaintEvent *ev)
{

	QPainter painter(this);

	// background
	painter.drawPixmap( ev->rect(), m_back, ev->rect() );

	if ( m_type == Type::Master ) {
		float peak_L = m_fPeakValue_L * 190.0;
		uint offset_L = (uint)( 190.0 - peak_L );
		painter.drawPixmap( QRect( 0, offset_L, 9, 190 - offset_L), m_leds, QRect( 0, offset_L, 9, 190 - offset_L) );

		float peak_R = m_fPeakValue_R * 190.0;
		uint offset_R = (uint)( 190.0 - peak_R );
		painter.drawPixmap( QRect( 9, offset_R, 9, 190 - offset_R), m_leds, QRect( 9, offset_R, 9, 190 - offset_R) );
	} else {
		float realPeak_L = m_fPeakValue_L - m_fMinPeak;
		int peak_L = 116 - ( realPeak_L / ( m_fMaxPeak - m_fMinPeak ) ) * 116.0;

		if ( peak_L > 116 ) {
			peak_L = 116;
		}
		if ( m_type == Type::Vertical ) {
			painter.drawPixmap( QRect( 0, 0, 116 - peak_L, 11 ), m_leds, QRect( 0, 0, 116 - peak_L, 11 ) );
		} else {
			painter.drawPixmap( QRect( 0, peak_L, 11, 116 - peak_L ), m_leds, QRect( 0, peak_L, 11, 116 - peak_L ) );
		}

		float realPeak_R = m_fPeakValue_R - m_fMinPeak;
		int peak_R = 116 - ( realPeak_R / ( m_fMaxPeak - m_fMinPeak ) ) * 116.0;
		if ( peak_R > 116 ) {
			peak_R = 116;
		}

		if ( m_type == Type::Vertical ) {
			painter.drawPixmap( QRect( 0, 11, 116 - peak_R, 11 ), m_leds, QRect( 0, 11, 116 - peak_R, 11 ) );
		} else {
			painter.drawPixmap( QRect( 11, peak_R, 11, 116 - peak_R ), m_leds, QRect( 11, peak_R, 11, 116 - peak_R ) );
		}
	}
	
	if ( m_bWithoutKnob == false ) {
		float fRange = m_fMax - m_fMin;
		float realVal = m_fValue - m_fMin;

		if ( m_type == Type::Vertical ) {
			uint knob_height = 15;
			uint knob_width = 29;
			uint knob_x = (uint)( 116.0 - ( 101 * ( 1-realVal / fRange ) ) );

			painter.drawPixmap( QRect(knob_x - knob_height, 4 , knob_width, knob_height), m_knob, QRect( 0, 0, knob_width, knob_height ) );
		} else {
			uint knob_height = 29;
			uint knob_width = 15;

			if ( m_type == Type::Master ) {
				uint knob_y = (uint)( 190.0 - ( 159.0 * ( m_fValue / ( m_fMax - m_fMin ) ) ) );
				painter.drawPixmap( QRect( 19, knob_y - knob_height, knob_width, knob_height), m_knob, QRect( 0, 0, knob_width, knob_height ) );
			} else {
				uint knob_y = (uint)( 116.0 - ( 86.0 * ( realVal / fRange ) ) );

				painter.drawPixmap( QRect( 4, knob_y - knob_height, knob_width, knob_height), m_knob, QRect( 0, 0, knob_width, knob_height ) );
			}
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
