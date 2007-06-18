/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Fader.cpp,v 1.24 2005/07/11 10:29:55 comix Exp $
 *
 */


#include "../Skin.h"
#include "Fader.h"
#include "LCD.h"

#include <qcursor.h>
#include <qrect.h>
#include <qpainter.h>

Fader::Fader(QWidget * parent, bool bWithoutKnob)
 : QWidget( parent , "Fader", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "Fader" )
 , m_bWithoutKnob( bWithoutKnob )
 , peakValue_L( 0.0 )
 , peakValue_R( 0.0 )
 , m_nValue( 0 )
 , changed( true )
{
	setMinimumSize( 23, 116 );
	setMaximumSize( 23, 116);
	resize( 23, 116 );

	// Create temp image
	temp.resize(23, 116);

	// Background image
	string background_path = Skin::getImagePath() + string( "/mixerPanel/fader_background.png" );
	bool ok = back.load( background_path.c_str() );
	if( ok == false ) {
		errorLog("Fader: Error loading pixmap");
	}

	// Leds image
	string leds_path = Skin::getImagePath() + string( "/mixerPanel/fader_leds.png" );
	ok = leds.load( leds_path.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap" );
	}

	// Knob image
	string knob_path = Skin::getImagePath() + string( "/mixerPanel/fader_knob.png" );
	ok = knob.load( knob_path.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap" );
	}

	//
//	setErasePixmap( back );
}



Fader::~Fader() {
//	cout << "fader destroy" << endl;
}



void Fader::mouseMoveEvent( QMouseEvent *ev ) {
	int y = ev->y() - 6;
	const int max_y = 116 - 12;

	int value = max_y - y;

	if (value > max_y) {
		value = max_y;
	}
	else if (value < 0) {
		value = 0;
	}

	int perc = (int)(value * 100.0 / max_y);
	setValue(perc);
	emit valueChanged(this);
	update();
}



void Fader::mousePressEvent(QMouseEvent *ev) {
	int y = ev->y() - 6;
	const int max_y = 116 - 12;
	const int min_y = 0;
	const int delta = max_y - min_y;

	int value = delta - y;

	if (value > max_y) {
		value = max_y;
	}
	else if (value < min_y) {
		value = min_y;
	}

	int perc = (int)(value * 100.0 / 88.0);
	setValue(perc);
	emit valueChanged(this);
	update();
}


void Fader::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	if ( ev->delta() > 0 ) {
		setValue( m_nValue + 2 );
	}
	else {
		setValue( m_nValue - 2 );
	}
	update();
	emit valueChanged(this);
}



void Fader::setValue(int newValue) {
	if (newValue > 100) {
//		errorLog( "[setValue] Error: value > 100 (value = " + toString( newValue ) + ")" );
		newValue = 100;
	}
	else if (newValue < 0) {
//		errorLog( "[setValue] Error: value < 0 (value = " + toString( newValue ) + ")" );
		newValue = 0;
	}

	if ( m_nValue != newValue) {
		m_nValue = newValue;
		changed = true;
	}
}



uint Fader::getValue() {
	return m_nValue;
}



///
/// Set peak value (0.0 .. 1.0)
///
void Fader::setPeak_L(float peak) {
	if (peak < 0.01f) {
//		char tmp[200];
//		sprintf( tmp, "[setPeak_L] Error: peak < 0.01 (peak = %f)", peak );
//		errorLog( tmp );
		peak = 0.01f;
	}
//	else if (peak > 1.0f) {
//		peak = 1.0f;
//	}

	if (this->peakValue_L != peak) {
		this->peakValue_L = peak;
		changed = true;
	}
}




///
/// Set peak value (0.0 .. 1.0)
///
void Fader::setPeak_R(float peak) {
	if (peak < 0.01f) {
//		char tmp[200];
//		sprintf( tmp, "[setPeak_R] Error: peak < 0.01 (peak = %f)", peak );
//		errorLog( tmp );
		peak = 0.01f;
	}
//	else if (peak > 1.0f) {
//		peak = 1.0f;
//	}

	if (this->peakValue_R != peak) {
		this->peakValue_R = peak;
		changed = true;
	}
}



void Fader::paintEvent( QPaintEvent*) {
	if (!isVisible()) {
		return;
	}
	if (changed) {
		changed = false;
		// background
		bitBlt( &temp, 0, 0, &back, 0, 0, 23, 116, CopyROP );

		// leds
		float peak_L = peakValue_L * 116.0;
		if (peak_L > 116.0) {
			peak_L = 116.0;
		}
		uint offset_L = (uint)( 116.0 - peak_L );
		bitBlt( &temp, 0, offset_L, &leds, 0, offset_L, 11, 116, CopyROP );

		float peak_R = peakValue_R * 116.0;
		if (peak_R > 116.0) {
			peak_R = 116.0;
		}
		uint offset_R = (uint)( 116.0 - peak_R );
		bitBlt( &temp, 12, offset_R, &leds, 12, offset_R, 23, 116, CopyROP );

		if (m_bWithoutKnob == false) {
			// knob
			static const uint knob_height = 29;
			static const uint knob_width = 15;
			uint knob_y = (uint)( 116.0 - (88.0 * m_nValue / 100.0) );
			bitBlt( &temp, 4, knob_y - knob_height, &knob, 0, 0, knob_width, knob_height, CopyROP );
		}
//		bitBlt( this, 0, 0, &temp, 0, 0, width(), height(), CopyROP, true);
//		setErasePixmap( temp );
	}
	bitBlt( this, 0, 0, &temp, 0, 0, width(), height(), CopyROP, true);
}



void Fader::updateFader() {
	if (changed && isVisible()) {
		update();
	}
}



//////////////////////////////////



MasterFader::MasterFader(QWidget * parent, bool bWithoutKnob)
 : QWidget( parent , "MasterFader", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "MasterFader" )
 , m_bWithoutKnob( bWithoutKnob )
 , peakValue_L( 0.0 )
 , peakValue_R( 0.0 )
 , m_nValue( 0 )
 , changed( true )
{
	setMinimumSize( 34, 190 );
	setMaximumSize( 34, 190);
	resize( 34, 190 );

	// Create temp image
	temp.resize(34, 190);

	// Background image
	string background_path = Skin::getImagePath() + string( "/mixerPanel/masterMixer_background.png" );
	bool ok = back.load( background_path.c_str() );
	if( ok == false ) {
		errorLog("Fader: Error loading pixmap");
	}

	// Leds image
	string leds_path = Skin::getImagePath() + string( "/mixerPanel/masterMixer_leds.png" );
	ok = leds.load( leds_path.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap" );
	}

	// Knob image
	string knob_path = Skin::getImagePath() + string( "/mixerPanel/fader_knob.png" );
	ok = knob.load( knob_path.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap" );
	}
}



MasterFader::~MasterFader() {
//	cout << "fader destroy" << endl;
}



void MasterFader::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	if ( ev->delta() > 0 ) {
		setValue( m_nValue + 2 );
	}
	else {
		setValue( m_nValue - 2 );
	}
	update();
	emit valueChanged(this);
}


void MasterFader::mouseMoveEvent( QMouseEvent *ev ) {
	int y = ev->y() - 15;
	const int max_y = 190 - 31;

	int value = max_y - y;

	if (value > max_y) {
		value = max_y;
	}
	else if (value < 0) {
		value = 0;
	}

	int perc = (int)(value * 100.0 / max_y);
	setValue(perc);
	emit valueChanged(this);
	update();
}



void MasterFader::mousePressEvent(QMouseEvent *ev) {
	int y = ev->y() - 15;
	const int max_y = 190 - 31;
	const int min_y = 0;
	const int delta = max_y - min_y;

	int value = delta - y;

	if (value > max_y) {
		value = max_y;
	}
	else if (value < min_y) {
		value = min_y;
	}

	int perc = (int)(value * 100.0 / max_y);
	setValue(perc);
	emit valueChanged(this);
	update();
}



void MasterFader::setValue(int newValue) {
	if (newValue > 100) {
//		char tmp[200];
//		sprintf( tmp, "[setValue] Error: value > 100 (value = %d)", newValue );
//		errorLog( tmp );
		newValue = 100;
	}
	else if (newValue < 0) {
//		char tmp[200];
//		sprintf( tmp, "[setValue] Error: value < 0 (value = %d)", newValue );
//		errorLog( tmp );
		newValue = 0;
	}

	if ( m_nValue != newValue) {
		m_nValue = newValue;
		changed = true;
	}
}



int MasterFader::getValue() {
	return m_nValue;
}



///
/// Set peak value (0.0 .. 1.0)
///
void MasterFader::setPeak_L(float peak) {
	if (peak < 0.01f) {
//		char tmp[200];
//		sprintf( tmp, "[setPeak_L] Error: peak < 0.01 (peak = %f)", peak );
//		errorLog( tmp );
		peak = 0.01f;
	}
	else if (peak > 1.0f) {
//		char tmp[200];
//		sprintf( tmp, "[setPeak_L] Error: peak > 1 (peak = %f)", peak );
//		errorLog( tmp );
		peak = 1.0f;
	}

	if (this->peakValue_L != peak) {
		this->peakValue_L = peak;
		changed = true;
	}
}




///
/// Set peak value (0.0 .. 1.0)
///
void MasterFader::setPeak_R(float peak) {
	if (peak < 0.01f) {
//		char tmp[200];
//		sprintf( tmp, "[setPeak_R] Error: peak < 0.01 (peak = %f)", peak );
//		errorLog( tmp );
		peak = 0.01f;
	}
	else if (peak > 1.0f) {
//		char tmp[200];
//		sprintf( tmp, "[setPeak_R] Error: peak > 1 (peak = %f)", peak );
//		errorLog( tmp );
		peak = 1.0f;
	}

	if (this->peakValue_R != peak) {
		this->peakValue_R = peak;
		changed = true;
	}
}



void MasterFader::paintEvent( QPaintEvent*) {
	if (!isVisible()) {
		return;
	}

	if (changed) {
		changed = false;

		// background
		bitBlt( &temp, 0, 0, &back, 0, 0, 34, 190, CopyROP );

		// leds
		float peak_L = peakValue_L * 190.0;
		uint offset_L = (uint)( 190.0 - peak_L );
		bitBlt( &temp, 0, offset_L, &leds, 0, offset_L, 9, 190, CopyROP );

		float peak_R = peakValue_R * 190.0;
		uint offset_R = (uint)( 190.0 - peak_R );
		bitBlt( &temp, 9, offset_R, &leds, 9, offset_R, 34, 190, CopyROP );

		if (m_bWithoutKnob == false) {
			// knob
			static const uint knob_height = 29;
			static const uint knob_width = 15;
			uint knob_y = (uint)( 190.0 - (159.0 * m_nValue / 100.0) );
			bitBlt( &temp, 19, knob_y - knob_height, &knob, 0, 0, knob_width, knob_height, CopyROP );
		}

//		setErasePixmap( temp );
	}
	bitBlt( this, 0, 0, &temp, 0, 0, width(), height(), CopyROP );
}



void MasterFader::updateFader() {
	if (changed && isVisible()) {
		update();
	}
}



//////////////////////////////////


/*
PanFader::PanFader(QWidget * parent)
 : QWidget( parent , "PanFader", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "PanFader" )
{
	setMinimumSize( 38, 14 );
	setMaximumSize( 38, 14);
	resize(38, 14);
	changed = true;
	value = 0;

	// Create temp image
	temp.resize( width(), height() );

	// Background image
	string background_path = Skin::getImagePath() + string("/mixerPanel/panFader_background.png" );
	bool ok = back.load(background_path.c_str());
	if( ok == false ) {
		qWarning("Fader: Error loading pixmap");
	}

}



PanFader::~PanFader() {
}



void PanFader::updateFader() {
	if (changed && isVisible()) {
		update();
	}
}


void PanFader::paintEvent( QPaintEvent*) {
	if (!isVisible()) {
		return;
	}

	if (changed) {
		changed = false;

		// background
		bitBlt(&temp, 0, 0, &back, 0, 0, 38, 14, CopyROP);

		QColor black( 134, 182, 244 );
		QPainter p(&temp);
		p.setPen(black);

		uint x_pos = (uint)(value / 100.0 * 36.0) + 1;

		// draw the vert line
		p.drawLine(x_pos - 1, 3, x_pos - 1, 10);
		p.drawLine(x_pos, 3, x_pos, 10);

//		setErasePixmap(temp);
	}
	bitBlt(this, 0, 0, &temp, 0, 0, width(), height(), CopyROP);
}



 void PanFader::mouseMoveEvent( QMouseEvent *ev ) {
	if (ev->state() == (LeftButton | ShiftButton) ) {	// restore central position with shift + left click
		float newValue = 50;
		setValue(newValue);
		emit valueChanged(this);
		return;
	}
	if (ev->state() == LeftButton ) {
		int x = ev->x();
		float newValue = x * 100.0 / 38.0;
		setValue(newValue);
		emit valueChanged(this);
	}
}



void PanFader::mousePressEvent(QMouseEvent *ev) {
	if (ev->state() == (ShiftButton) ) {	// restore central position with shift + left click
		float newValue = 50;
		setValue(newValue);
		emit valueChanged(this);
		return;
	}

	int x = ev->x();
	float newValue = x * 100.0 / 38.0;
	setValue(newValue);
	emit valueChanged(this);
}



void PanFader::setValue(float val) {
	if (val == value) {
		return;
	}
	if (val < 0.0) {
		val = 0.0;
	}
	else if (val > 100.0) {
		val = 100.0;
	}
//	if (val != value) {
//		value = val;
//		changed = true;
//	}
	value = val;
	changed = true;
}
*/


///////////////////





QPixmap* Knob::m_background = NULL;


///
/// Constructor
///
Knob::Knob( QWidget* parent )
 : QWidget( parent , "Knob", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "Knob" )
{
//	infoLog( "INIT" );
	m_bChanged = true;
	m_nWidgetWidth = 18;
	m_nWidgetHeight = 18;
	m_fValue = 0.0;

	if ( m_background == NULL ) {
//		infoLog( "loading background pixmap" );
		string sBackground_path = Skin::getImagePath() + string( "/mixerPanel/knob_images.png" );
		m_background = new QPixmap();
		if ( m_background->load( sBackground_path.c_str() ) == false ){
			errorLog( string("Error loading pixmap ") + sBackground_path );
		}
	}

	resize( m_nWidgetWidth, m_nWidgetHeight );
	m_temp.resize( m_nWidgetWidth, m_nWidgetHeight );
}



///
/// Destructor
///
Knob::~ Knob()
{
//	infoLog( "DESTROY" );
//	delete m_background;
//	m_background = NULL;
}



void Knob::paintEvent( QPaintEvent* ev )
{
	if (!isVisible()) {
		return;
	}

	if (m_bChanged) {
		m_bChanged = false;

		int nFrame = (int)(31.0 * m_fValue);
		int xPos = m_nWidgetWidth * nFrame;
		bitBlt(&m_temp, 0, 0, m_background, xPos, 0, m_nWidgetWidth, m_nWidgetHeight, CopyROP);
//		setErasePixmap( m_temp );
	}
	bitBlt( this, 0, 0, &m_temp, 0, 0, width(), height(), CopyROP );
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
	m_fValue = fValue;
	m_bChanged = true;
}



void Knob::mousePressEvent(QMouseEvent *ev)
{
	setCursor( QCursor( Qt::SizeVerCursor ) );

	m_fMousePressValue = m_fValue;
	m_fMousePressY = ev->y();
}



void Knob::mouseReleaseEvent( QMouseEvent *ev )
{
	setCursor( QCursor( Qt::ArrowCursor ) );
}



 void Knob::mouseMoveEvent( QMouseEvent *ev ) {
	float y = ev->y() - m_fMousePressY;
	float fNewValue = m_fMousePressValue - ( y / 100.0 );
	setValue( fNewValue );
	update();
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
	update();
	emit valueChanged(this);
}



void Knob::updateKnob()
{
	if (m_bChanged && isVisible()) {
		update();
	}
}


