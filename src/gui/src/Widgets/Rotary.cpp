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

#include <QFile>
#include <QSvgRenderer>

#include <core/Globals.h>

const char* Rotary::__class_name = "Rotary";

Rotary::Rotary( QWidget* parent, RotaryType type, QString sToolTip, bool bUseIntSteps, bool bUseValueTip, float fMin, float fMax, QColor color )
 : QWidget( parent )
 , Object( __class_name )
 , m_bUseIntSteps( bUseIntSteps )
 , m_type( type )
 , m_fMin( fMin )
 , m_fMax( fMax )
 , m_fMousePressValue( 0.0 )
 , m_fMousePressY( 0.0 )
 , m_bIgnoreMouseMove( false )
 , m_color( color )
 , m_sBaseTooltip( sToolTip )
{
	setToolTip( sToolTip );

	if ( type == TYPE_SMALL ) {
		m_nWidgetWidth = 18;
		m_nWidgetHeight = 18;
	} else {
		m_nWidgetWidth = 44;
		m_nWidgetHeight = 25;
	}

	if ( bUseIntSteps ) {
		m_fDefaultValue = static_cast<int>( type == TYPE_CENTER ? ( m_fMin + ( m_fMax - m_fMin ) / 2.0 ) : m_fMin );
	}
	else {
		m_fDefaultValue = ( type == TYPE_CENTER ? ( m_fMin + ( m_fMax - m_fMin ) / 2.0 ) : m_fMin );
	}

	m_fValue = m_fDefaultValue;

	m_nScrollSpeedSlow = 1;
	m_nScrollSpeedFast = 5;

	// Since the load function does not report success, we will check
	// for the existance of the background image separately.
	QString sPath;
	if ( type == TYPE_SMALL ) {
		sPath = Skin::getSvgImagePath() + "/rotary2.svg";
	} else {
		sPath = Skin::getSvgImagePath() + "/rotary.svg";
	}
	
	QFile backgroundFile( sPath );
	if ( backgroundFile.exists() ) {
		m_background = new QSvgRenderer( sPath, this );
	} else {
		m_background = nullptr;
		ERRORLOG( QString( "Unable to load background image [%1]" ).arg( sPath ) );
	}
	resize( m_nWidgetWidth, m_nWidgetHeight );
}

Rotary::~ Rotary() {
}

void Rotary::paintEvent( QPaintEvent* ev )
{
	ev->accept();
	QPainter painter( this );

	painter.setRenderHint( QPainter::Antialiasing, true );

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

	QRect source( 0, 0, m_nWidgetWidth, m_nWidgetHeight );

	if ( m_background != nullptr ) {
		m_background->render( &painter, source ); 
	}

	if ( m_type != TYPE_SMALL ) {
		int nStartAngle = 218 * 16; // given in 1/16 of a degree
		int nSpanAngle  = static_cast<int>( -255 * 16 * ( m_fValue - m_fMin ) / ( m_fMax - m_fMin ) );
		QRectF arcRect( 9.951, 2.2, 24.5, 24.5 );

		painter.setBrush( m_color );
		painter.setPen( QPen( m_color, 1.7 ) );
		painter.drawArc( arcRect, nStartAngle, nSpanAngle );
	}
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
	if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier ) {
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
	
	QToolTip::showText( ev->globalPos(), QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
}

void Rotary::mouseReleaseEvent( QMouseEvent *ev )
{
	UNUSED( ev );
	
	setCursor( QCursor( Qt::ArrowCursor ) );

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
	
	QToolTip::showText( ev->globalPos(), QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
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

	QToolTip::showText( ev->globalPos(), QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
}



void Rotary::setMin( float fMin )
{
	m_fMin = fMin;
	update();
}

void Rotary::setMax( float fMax )
{
	m_fMax = fMax;
	update();
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

void Rotary::resetValueToDefault()
{
	setValue(m_fDefaultValue);
}

void Rotary::setColor( QColor color ) {
	m_color = color;
	update();
}
