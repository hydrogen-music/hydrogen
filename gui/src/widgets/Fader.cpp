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
#include <hydrogen/globals.h>

Fader::Fader( QWidget *pParent, bool bUseIntSteps, bool bWithoutKnob )
 : QWidget( pParent )
 , Object( "Fader" )
 , m_bWithoutKnob( bWithoutKnob )
 , m_bUseIntSteps( bUseIntSteps )
 , m_fPeakValue_L( 0.0 )
 , m_fPeakValue_R( 0.0 )
 , m_fMinPeak( 0.01f )
 , m_fMaxPeak( 1.0 )
 , m_fValue( 0.0 )
 , m_fMinValue( 0.0 )
 , m_fMaxValue( 1.0 )
{
	setAttribute( Qt::WA_NoBackground );
	setMinimumSize( 23, 116 );
	setMaximumSize( 23, 116);
	resize( 23, 116 );

	// Background image
	QString background_path = Skin::getImagePath() + "/mixerPanel/fader_background.png";
	bool ok = m_back.load( background_path );
	if( ok == false ) {
		ERRORLOG("Fader: Error loading pixmap");
	}

	// Leds image
	QString leds_path = Skin::getImagePath()  + "/mixerPanel/fader_leds.png";
	ok = m_leds.load( leds_path );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	// Knob image
	QString knob_path = Skin::getImagePath() + "/mixerPanel/fader_knob.png";
	ok = m_knob.load( knob_path );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}
}



Fader::~Fader()
{
	//infoLog( "[Destroy]" );
}



void Fader::mouseMoveEvent( QMouseEvent *ev )
{
	float fVal = (float)( height() - ev->y() ) / (float)height();
	fVal = fVal * ( m_fMaxValue - m_fMinValue );

	fVal = fVal + m_fMinValue;

	setValue( fVal );
	emit valueChanged(this);
}



void Fader::mousePressEvent(QMouseEvent *ev)
{
    if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ShiftModifier ){
	MidiSenseWidget midiSense( this, true, this->getAction() );
	midiSense.exec();
    }
}



void Fader::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	if ( m_bUseIntSteps ) {
		if ( ev->delta() > 0 ) {
			setValue( m_fValue + 1 );
		}
		else {
			setValue( m_fValue - 1 );
		}
	}
	else {
		float step = ( m_fMaxValue - m_fMinValue ) / 50.0;

		if ( ev->delta() > 0 ) {
			setValue( m_fValue + step );
		}
		else {
			setValue( m_fValue - step );
		}
	}

	emit valueChanged(this);
}



void Fader::setValue( float fVal )
{
	if ( fVal > m_fMaxValue ) {
		//WARNINGLOG( toString( fVal ) + " > " + toString( m_fMax ) );
		fVal = m_fMaxValue;
	}
	else if ( fVal < m_fMinValue ) {
		//WARNINGLOG( toString( fVal ) + " < " + toString( m_fMin ) );
		fVal = m_fMinValue;
	}

	if ( m_bUseIntSteps ) {
		fVal = (int)fVal;
	}

	if ( m_fValue != fVal ) {
		//INFOLOG( "new value: " + toString( fVal ) );
		m_fValue = fVal;
		update();
	}
}



float Fader::getValue()
{
	return m_fValue;
}



///
/// Set peak value (0.0 .. 1.0)
///
void Fader::setPeak_L( float fPeak )
{
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




///
/// Set peak value (0.0 .. 1.0)
///
void Fader::setPeak_R( float fPeak )
{
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



void Fader::paintEvent( QPaintEvent *ev)
{
	UNUSED( ev );

	QPainter painter(this);

	// background
//	painter.drawPixmap( rect(), m_back, QRect( 0, 0, 23, 116 ) );
	painter.drawPixmap( ev->rect(), m_back, ev->rect() );


	// peak leds
	//float fRange = abs( m_fMaxPeak ) + abs( m_fMinPeak );

	float realPeak_L = m_fPeakValue_L - m_fMinPeak;
	//int peak_L = 116 - ( realPeak_L / fRange ) * 116.0;
	int peak_L = 116 - ( realPeak_L / ( m_fMaxPeak - m_fMinPeak ) ) * 116.0;

	if ( peak_L > 116 ) {
		peak_L = 116;
	}
	painter.drawPixmap( QRect( 0, peak_L, 11, 116 - peak_L ), m_leds, QRect( 0, peak_L, 11, 116 - peak_L ) );


	float realPeak_R = m_fPeakValue_R - m_fMinPeak;
	int peak_R = 116 - ( realPeak_R / ( m_fMaxPeak - m_fMinPeak ) ) * 116.0;
	if ( peak_R > 116 ) {
		peak_R = 116;
	}
	painter.drawPixmap( QRect( 11, peak_R, 11, 116 - peak_R ), m_leds, QRect( 11, peak_R, 11, 116 - peak_R ) );

	if ( m_bWithoutKnob == false ) {
		// knob
		static const uint knob_height = 29;
		static const uint knob_width = 15;

		float fRange = m_fMaxValue - m_fMinValue;

		float realVal = m_fValue - m_fMinValue;

//		uint knob_y = (uint)( 116.0 - ( 86.0 * ( m_fValue / fRange ) ) );
		uint knob_y = (uint)( 116.0 - ( 86.0 * ( realVal / fRange ) ) );



		painter.drawPixmap( QRect( 4, knob_y - knob_height, knob_width, knob_height), m_knob, QRect( 0, 0, knob_width, knob_height ) );
	}
}



void Fader::setMinValue( float fMin )
{
	m_fMinValue = fMin;
}




void Fader::setMaxValue( float fMax )
{
	m_fMaxValue = fMax;
}



void Fader::setMaxPeak( float fMax )
{
	m_fMaxPeak = fMax;
}



void Fader::setMinPeak( float fMin )
{
	m_fMinPeak = fMin;
}




//////////////////////////////////



MasterFader::MasterFader(QWidget *pParent, bool bWithoutKnob)
 : QWidget( pParent )
 , Object( "MasterFader" )
 , m_bWithoutKnob( bWithoutKnob )
 , m_fPeakValue_L( 0.0 )
 , m_fPeakValue_R( 0.0 )
 , m_fValue( 0.0 )
 , m_fMin( 0.0 )
 , m_fMax( 1.0 )
{
	setAttribute(Qt::WA_NoBackground);

	setMinimumSize( 34, 190 );
	setMaximumSize( 34, 190);
	resize( 34, 190 );

	// Background image
	QString background_path = Skin::getImagePath() + "/mixerPanel/masterMixer_background.png";
	bool ok = m_back.load( background_path );
	if( ok == false ) {
		ERRORLOG("Fader: Error loading pixmap");
	}

	// Leds image
	QString leds_path = Skin::getImagePath() + "/mixerPanel/masterMixer_leds.png";
	ok = m_leds.load( leds_path );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	// Knob image
	QString knob_path = Skin::getImagePath() + "/mixerPanel/fader_knob.png";
	ok = m_knob.load( knob_path );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}
}



MasterFader::~MasterFader()
{
//	infoLog( "[~MasterFader]" );
}



void MasterFader::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	float step = ( m_fMax - m_fMin ) / 50.0;

	if ( ev->delta() > 0 ) {
		setValue( m_fValue + step );
	}
	else {
		setValue( m_fValue - step );
	}
	emit valueChanged(this);
}



void MasterFader::mouseMoveEvent( QMouseEvent *ev )
{
	float fVal = (float)( height() - ev->y() ) / (float)height();
	fVal = fVal * ( m_fMax - m_fMin );

	setValue( fVal );
	emit valueChanged(this);
}



void MasterFader::mousePressEvent(QMouseEvent *ev)
{
	if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ShiftModifier ){
	    MidiSenseWidget midiSense( this, true, this->getAction() );
	    midiSense.exec();
	}
}



void MasterFader::setValue( float newValue )
{
	if ( newValue > m_fMax ) {
		newValue = m_fMax;
	}
	else if ( newValue < m_fMin ) {
		newValue = m_fMin;
	}

	if ( m_fValue != newValue) {
		m_fValue = newValue;
		update();
	}
}



float MasterFader::getValue()
{
	return m_fValue;
}



///
/// Set peak value (0.0 .. 1.0)
///
void MasterFader::setPeak_L( float peak )
{
	if ( peak < 0.01f ) {
		peak = 0.01f;
	}
	else if (peak > 1.0f ) {
		peak = 1.0f;
	}

	if ( m_fPeakValue_L != peak ) {
		m_fPeakValue_L = peak;
		update();
	}
}




///
/// Set peak value (0.0 .. 1.0)
///
void MasterFader::setPeak_R( float peak )
{
	if ( peak < 0.01f ) {
		peak = 0.01f;
	}
	else if ( peak > 1.0f ) {
		peak = 1.0f;
	}

	if ( m_fPeakValue_R != peak ) {
		m_fPeakValue_R = peak;
		update();
	}
}



void MasterFader::paintEvent( QPaintEvent* ev )
{
	QPainter painter(this);

	// background
	painter.drawPixmap( ev->rect(), m_back, ev->rect() );

	// leds
	float peak_L = m_fPeakValue_L * 190.0;
	uint offset_L = (uint)( 190.0 - peak_L );
	painter.drawPixmap( QRect( 0, offset_L, 9, 190 - offset_L), m_leds, QRect( 0, offset_L, 9, 190 - offset_L) );

	float peak_R = m_fPeakValue_R * 190.0;
	uint offset_R = (uint)( 190.0 - peak_R );
	painter.drawPixmap( QRect( 9, offset_R, 9, 190 - offset_R), m_leds, QRect( 9, offset_R, 9, 190 - offset_R) );

	if (m_bWithoutKnob == false) {
		// knob
		static const uint knob_height = 29;
		static const uint knob_width = 15;
		uint knob_y = (uint)( 190.0 - ( 159.0 * ( m_fValue / ( m_fMax - m_fMin ) ) ) );
		painter.drawPixmap( QRect( 19, knob_y - knob_height, knob_width, knob_height), m_knob, QRect( 0, 0, knob_width, knob_height ) );
	}


/*
	if ( m_bWithoutKnob == false ) {
		// knob
		static const uint knob_height = 29;
		static const uint knob_width = 15;
		uint knob_y = (uint)( 116.0 - ( 86.0 * ( m_fValue / ( m_fMax - m_fMin ) ) ) );
		painter.drawPixmap( QRect( 4, knob_y - knob_height, knob_width, knob_height), knob, QRect( 0, 0, knob_width, knob_height ) );
	}
*/
}



void MasterFader::setMin( float fMin )
{
	m_fMin = fMin;
}




void MasterFader::setMax( float fMax )
{
	m_fMax = fMax;
}





///////////////////





QPixmap* Knob::m_background = NULL;


///
/// Constructor
///
Knob::Knob( QWidget* pParent )
 : QWidget( pParent )
 , Object( "Knob" )
{
	setAttribute(Qt::WA_NoBackground);

	m_nWidgetWidth = 18;
	m_nWidgetHeight = 18;
	m_fValue = 0.0;

	if ( m_background == NULL ) {
		QString sBackground_path = Skin::getImagePath() + "/mixerPanel/knob_images.png";
		m_background = new QPixmap();
		if ( m_background->load( sBackground_path ) == false ){
			ERRORLOG( "Error loading pixmap" );
		}
	}

	resize( m_nWidgetWidth, m_nWidgetHeight );
}



///
/// Destructor
///
Knob::~ Knob()
{
}



void Knob::paintEvent( QPaintEvent* ev )
{
	UNUSED( ev );

	QPainter painter(this);

	int nFrame = (int)(31.0 * m_fValue);
	int xPos = m_nWidgetWidth * nFrame;
//	bitBlt(&m_temp, 0, 0, m_background, xPos, 0, m_nWidgetWidth, m_nWidgetHeight, CopyROP);
	painter.drawPixmap( rect(), *m_background, QRect( xPos, 0, m_nWidgetWidth, m_nWidgetHeight ) );
}



void Knob::setValue( float fValue )
{
	if ( fValue == m_fValue ) {
		return;
	}

	if ( fValue < 0.0 ) {
		fValue = 0.0;
//		warningLog( "[setValue] fValue < 0" );
	}
	else if ( fValue > 1.0 ) {
		fValue = 1.0;
//		warningLog( "[setValue] fValue > 1" );
	}

	if ( fValue != m_fValue ) {
		m_fValue = fValue;
		update();
	}
}



void Knob::mousePressEvent(QMouseEvent *ev)
{
    if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ShiftModifier ){
	MidiSenseWidget midiSense( this, true, this->getAction() );
	midiSense.exec();
    }

    setCursor( QCursor( Qt::SizeVerCursor ) );

	m_fMousePressValue = m_fValue;
	m_fMousePressY = ev->y();
}



void Knob::mouseReleaseEvent( QMouseEvent *ev )
{
	UNUSED( ev );
	setCursor( QCursor( Qt::ArrowCursor ) );
}



 void Knob::mouseMoveEvent( QMouseEvent *ev ) {
	float y = ev->y() - m_fMousePressY;
	float fNewValue = m_fMousePressValue - ( y / 100.0 );
	setValue( fNewValue );
	emit valueChanged(this);
}


void Knob::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	if ( ev->delta() > 0 ) {
		setValue( m_fValue + 0.025 );
	}
	else {
		setValue( m_fValue - 0.025 );
	}
	emit valueChanged(this);
}



