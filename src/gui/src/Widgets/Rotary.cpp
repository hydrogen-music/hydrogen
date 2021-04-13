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
#include "Rotary.h"
#include "LCD.h"
#include "../Skin.h"

#include <core/Globals.h>

RotaryTooltip::RotaryTooltip( QPoint pos )
//  : QWidget( 0, "RotaryTooltip", Qt::WStyle_Customize| Qt::WStyle_NoBorder | Qt::WStyle_StaysOnTop| Qt::WX11BypassWM )
  : QWidget( nullptr, Qt::ToolTip )
{
	UNUSED( pos );

	m_pDisplay = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 4);
	m_pDisplay->move( 0, 0 );
	resize( m_pDisplay->size() );

	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Window, QColor( 49, 53, 61 ) );
	this->setPalette( defaultPalette );

}


void RotaryTooltip::showTip( QPoint pos, QString sText )
{
	move( pos );
	m_pDisplay->setText( sText );
	show();
}

RotaryTooltip::~RotaryTooltip()
{
//	delete m_pDisplay;
}





///////////////////

QPixmap* Rotary::m_background_normal = nullptr;
QPixmap* Rotary::m_background_center = nullptr;
QPixmap* Rotary::m_background_small = nullptr;

const char* Rotary::__class_name = "Rotary";

Rotary::Rotary( QWidget* parent, RotaryType type, QString sToolTip, bool bUseIntSteps, bool bUseValueTip, float fMin, float fMax )
 : QWidget( parent )
 , Object( __class_name )
 , m_bUseIntSteps( bUseIntSteps )
 , m_type( type )
 , m_fMin( fMin )
 , m_fMax( fMax )
 , m_fMousePressValue( 0.0 )
 , m_fMousePressY( 0.0 )
 , m_bShowValueToolTip( bUseValueTip )
 , m_bIgnoreMouseMove( false )
{
	setAttribute(Qt::WA_OpaquePaintEvent);
	setToolTip( sToolTip );

	m_pValueToolTip = new RotaryTooltip( mapToGlobal( QPoint( 0, 0 ) ) );
	
	if ( type == TYPE_SMALL ) {
		m_nWidgetWidth = 18;
		m_nWidgetHeight = 18;
	} else {
		m_nWidgetWidth = 28;
		m_nWidgetHeight = 26;
	}

	if (bUseIntSteps) {
		m_fDefaultValue = (int) ( type == TYPE_CENTER ? ( m_fMin + ( m_fMax - m_fMin ) / 2.0 ) : m_fMin );
	}
	else {
		m_fDefaultValue = ( type == TYPE_CENTER ? ( m_fMin + ( m_fMax - m_fMin ) / 2.0 ) : m_fMin );
	}

	m_fValue = m_fDefaultValue;

	if ( type == TYPE_NORMAL && m_background_normal == nullptr ) {
		m_background_normal = new QPixmap();
		if ( m_background_normal->load( Skin::getImagePath() + "/mixerPanel/rotary_images.png" ) == false ){
			ERRORLOG( "Error loading pixmap" );
		}
	} else if ( type == TYPE_CENTER && m_background_center == nullptr ) {
		m_background_center = new QPixmap();
		if ( m_background_center->load( Skin::getImagePath() + "/mixerPanel/rotary_center_images.png" ) == false ){
			ERRORLOG( "Error loading pixmap" );
		}
	} else if ( type == TYPE_SMALL && m_background_small == nullptr ) {
		m_background_small = new QPixmap();
		if ( m_background_small->load( Skin::getImagePath() + "/mixerPanel/knob_images.png" ) == false ){ //TODO rename png?
			ERRORLOG( "Error loading pixmap" );
		}
	}
	resize( m_nWidgetWidth, m_nWidgetHeight );
//	m_temp.resize( m_nWidgetWidth, m_nWidgetHeight );
}



Rotary::~ Rotary()
{
	delete m_pValueToolTip;
}



void Rotary::paintEvent( QPaintEvent* ev )
{
	UNUSED( ev );
	QPainter painter(this);

	float fRange = m_fMax - m_fMin;

	int nFrame, nTotFrames;
	if ( m_type == TYPE_NORMAL || m_type == TYPE_CENTER ) {
		nTotFrames = 63;
	} else if ( m_type == TYPE_SMALL ) {
		nTotFrames = 31;	
	} else { // undefined RotaryType
		ERRORLOG( "Error undefined RotaryType" );
	}
	
	if ( m_bUseIntSteps ) {
		nFrame = (int)( nTotFrames * floor( m_fValue - m_fMin ) / fRange );
	}
	else {
		nFrame = (int)( nTotFrames * ( m_fValue - m_fMin ) / fRange );
	}

//	INFOLOG( "\nrange: " + toString( fRange ) );
//	INFOLOG( "norm value: " + toString( fValue ) );
//	INFOLOG( "frame: " + toString( nFrame ) );

	/*if ( m_type == TYPE_NORMAL || 1) {
		int xPos = m_nWidgetWidth * nFrame;
		painter.drawPixmap( rect(), *m_background_normal, QRect( xPos, 0, m_nWidgetWidth, m_nWidgetHeight ) );
	}
	else {*/
	
		// the image is broken...
		/*if ( nFrame >= nTotFrames ) { // impossible since the boundary checks in setValue()
			nFrame = nTotFrames - 1;
		}*/
	int xPos = m_nWidgetWidth * nFrame;
	if ( m_type == TYPE_NORMAL ) {
		painter.drawPixmap( rect(), *m_background_normal, QRect( xPos, 0, m_nWidgetWidth, m_nWidgetHeight ) );
	} else if ( m_type == TYPE_CENTER ) {
		painter.drawPixmap( rect(), *m_background_center, QRect( xPos, 0, m_nWidgetWidth, m_nWidgetHeight ) );
	} else if ( m_type == TYPE_SMALL ) {
		painter.drawPixmap( rect(), *m_background_small, QRect( xPos, 0, m_nWidgetWidth, m_nWidgetHeight ) );
	}

	//}
}



void Rotary::setValue( float fValue )
{
	if ( fValue == m_fValue ) {
		return;
	}

	if ( fValue < m_fMin ) {
		fValue = m_fMin;
	}
	else if ( fValue > m_fMax ) {
		fValue = m_fMax;
	}

	if ( fValue != m_fValue ) {
		m_fValue = fValue;
		update();
	}
}



void Rotary::mousePressEvent(QMouseEvent *ev)
{
	if (ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier) {
		resetValueToDefault();
		m_bIgnoreMouseMove = true;
		emit valueChanged(this);
	}
	else if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ShiftModifier ) {
		MidiSenseWidget midiSense( this, true, this->getAction() );
		midiSense.exec();
	}
	else {
		setCursor( QCursor( Qt::SizeVerCursor ) );

		m_fMousePressValue = m_fValue;
		m_fMousePressY = ev->y();
	}
	
	if ( m_bShowValueToolTip ) {
		char tmp[20];
		sprintf( tmp, "%#.2f", m_fValue );
		m_pValueToolTip->showTip( mapToGlobal( QPoint( -38, 1 ) ), QString( tmp ) );
	}
}




void Rotary::mouseReleaseEvent( QMouseEvent *ev )
{
	UNUSED( ev );
	
	setCursor( QCursor( Qt::ArrowCursor ) );
	m_pValueToolTip->hide();

	m_bIgnoreMouseMove = false;
}




void Rotary::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	float stepfactor = 5.0; // coarse adjustment
	float delta = 1.0;

	// Control Modifier = fine adjustment
	if (ev->modifiers() == Qt::ControlModifier) {
		stepfactor = 1.0;
	}
	if ( !m_bUseIntSteps ) {
		float fRange = m_fMax - m_fMin;
		delta = fRange / 100.0;
	}
	if ( ev->angleDelta().y() < 0 ) {
		delta *= -1.;
	}
	setValue( getValue() + (delta * stepfactor) );
	emit valueChanged(this);
	
	if ( m_bShowValueToolTip ) {
		char tmp[20];
		sprintf( tmp, "%#.2f", m_fValue );
		
		// Problem: ValueToolTip remains visible after wheel event
		//m_pValueToolTip->showTip( mapToGlobal( QPoint( -38, 1 ) ), QString( tmp ) );
		
		// useful but graphically incoeherent
		QToolTip::showText( ev->globalPos(), QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
	}
}



void Rotary::mouseMoveEvent( QMouseEvent *ev )
{
	if ( m_bIgnoreMouseMove ) {
		return;
	}

	float fRange = m_fMax - m_fMin;

	float deltaY = ev->y() - m_fMousePressY;
	float fNewValue = ( m_fMousePressValue - ( deltaY / 100.0 * fRange ) );

	setValue( fNewValue );
	emit valueChanged(this);

	if ( m_bShowValueToolTip ) {
		char tmp[20];
		sprintf( tmp, "%#.2f", m_fValue );
		m_pValueToolTip->showTip( mapToGlobal( QPoint( -38, 1 ) ), QString( tmp ) );
	}
}



void Rotary::setMin( float fMin )
{
	m_fMin = fMin;
	update();
}

float Rotary::getMin()
{
	return m_fMin;
}



void Rotary::setMax( float fMax )
{
	m_fMax = fMax;
	update();
}


float Rotary::getMax()
{
	return m_fMax;
}


void Rotary::setDefaultValue( float fDefaultValue )
{
	if ( fDefaultValue == m_fDefaultValue ) {
		return;
	}

	if ( fDefaultValue < m_fMin ) {
		fDefaultValue = m_fMin;
	}
	else if ( fDefaultValue > m_fMax ) {
		fDefaultValue = m_fMax;
	}

	if ( fDefaultValue != m_fDefaultValue ) {
		m_fDefaultValue = fDefaultValue;
	}
}

float Rotary::getDefaultValue()
{
	return m_fDefaultValue;
}

void Rotary::resetValueToDefault()
{
	setValue(m_fDefaultValue);
}



////////////////////////////
/*
QPixmap* Knob::m_background = nullptr;

//const char* Knob::__class_name = "Knob";

///
/// Constructor
///

Knob::Knob( QWidget* pParent, QString sToolTip, bool bUseValueTip )
 : Rotary::Rotary( pParent, TYPE_NORMAL, sToolTip, false, bUseValueTip )
{
	setAttribute(Qt::WA_OpaquePaintEvent);

	m_nWidgetWidth = 18;
	m_nWidgetHeight = 18;

	if ( m_background == nullptr ) {
		QString sBackground_path = Skin::getImagePath() + "/mixerPanel/knob_images.png";
		m_background = new QPixmap();
		if ( m_background->load( sBackground_path ) == false ){
			ERRORLOG( "Error loading pixmap" );
		}
	}

	resize( m_nWidgetWidth, m_nWidgetHeight );
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

void Knob::mouseMoveEvent( QMouseEvent *ev )
{
	if ( m_bIgnoreMouseMove ) {
		return;
	}

	float y = ev->y() - m_fMousePressY;
	float fNewValue = m_fMousePressValue - ( y / 100.0 );
	setValue( fNewValue );
	emit valueChanged(this);
	
	if ( m_bShowValueToolTip ) {
		char tmp[20];
		sprintf( tmp, "%#.2f", m_fValue );
		m_pValueToolTip->showTip( mapToGlobal( QPoint( -38, 1 ) ), QString( tmp ) );
	}
}*/
