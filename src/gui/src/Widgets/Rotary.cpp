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

#include <cmath>

#include <QFile>
#include <QSvgRenderer>

#include <core/Globals.h>
#include <core/Preferences.h>

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
 , m_sBaseTooltip( sToolTip )
 , m_bFocused( false )
 , m_bIsActive( true )
{
	setAttribute( Qt::WA_Hover );
	setToolTip( sToolTip );

	if ( type == TYPE_SMALL ) {
		m_nWidgetWidth = 18;
		m_nWidgetHeight = 18;
	} else {
		m_nWidgetWidth = 44;
		m_nWidgetHeight = 26;
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

	QColor colorHighlightActive;
	QColor colorArc;
	QColor colorArcCenterSet;
	QColor colorArcCenterUnset;
	if ( m_bIsActive ) {
		colorHighlightActive = QColor( 97, 167, 251);
		colorArc = Qt::red;
		colorArcCenterSet = Qt::green;
		colorArcCenterUnset = Qt::gray;
	} else {
		colorHighlightActive = Qt::lightGray;
		colorArc = Qt::darkGray;
		colorArcCenterSet = Qt::darkGray;
		colorArcCenterUnset = Qt::lightGray;
	}
	QColor colorHandle = Qt::black;
	QColor colorFont = Qt::white;

	
	QRect rectBackground( 0, 0, m_nWidgetWidth, m_nWidgetHeight );

	if ( m_background != nullptr ) {
		if ( m_type == TYPE_SMALL ) {
			m_background->render( &painter, rectBackground );
		} else {
			m_background->render( &painter, "layer3", rectBackground );

			if ( m_bFocused  ) {
				painter.fillRect( 0, 23, m_nWidgetWidth, 2, colorHighlightActive );
			}
			m_background->render( &painter, "layer2", QRectF( 9.91, 1.6, 23.5, 23.5 ) );
		}
	}

	if ( m_type != TYPE_SMALL ) {
		QRectF arcRect( 9.951, 2.2, 24.5, 24.5 );

		if ( m_type == TYPE_NORMAL ) {
			int nStartAngle = 210 * 16; // given in 1/16 of a degree
			int nSpanAngle  = static_cast<int>( -239 * 16 * ( m_fValue - m_fMin ) / ( m_fMax - m_fMin ) );

			painter.setPen( QPen( colorArc, 1.7 ) );
			painter.drawArc( arcRect, nStartAngle, nSpanAngle );
		} else {
			// TYPE_CENTER

			// There will be a special indication of the
			// center. Either as a gray dot or a bigger green one if
			// the value is smaller than +/-1% of the range around 0.
			if ( std::fabs( m_fValue - 0.5 * ( m_fMax + m_fMin ) ) < 0.01 * ( m_fMax - m_fMin ) ) {
				
				painter.setPen( QPen( colorArcCenterSet, 2.5 ) );
				painter.drawArc( arcRect, 90 * 16, -3 * 16 );
				
			} else {
				
				painter.setPen( QPen( colorArcCenterUnset, 2.5 ) );
				painter.drawArc( arcRect, 90 * 16, -3 * 16 );

				int nStartAngle = -18 * 16;
				int nSpanAngle  = static_cast<int>( -200* 16 * ( m_fValue - 0.5 * ( m_fMax + m_fMin ) ) / ( m_fMax - m_fMin ) );
				if ( m_fValue - 0.5 * ( m_fMax + m_fMin ) < 0 ) {
					nStartAngle *= -1;
					nStartAngle -= 2 * 16;
				}
				nStartAngle += 89 * 16;
				
				painter.setPen( QPen( colorArc, 1.7 ) );
				painter.drawArc( arcRect, nStartAngle, nSpanAngle ); 
			}
		}
	}
	
	float fPi = std::acos( -1 );
	float fCurrentAngle;
	float fStartAngle;
	if ( m_type == TYPE_CENTER ) {
		fStartAngle = -90 * fPi / 180;
		fCurrentAngle = fStartAngle + 255 * fPi / 180 * ( m_fValue - 0.5 * ( m_fMax + m_fMin ) ) / (  m_fMax - m_fMin );
	} else {
		fStartAngle = -90 * fPi / 180;
		fCurrentAngle = fStartAngle + 255 * fPi / 180 * ( m_fValue - m_fMin - 0.5 * ( m_fMax - m_fMin ) ) / ( m_fMax - m_fMin );
	}
	
	///////////////////////
	// Indicating the current position using a rotated line instead of
	// a dot
	///////////////////////
	//
	// float fLength, fWidth, fBaseX, fBaseY;
	// if ( m_type == TYPE_SMALL ) {
	// 	fBaseX = 9.0;
	// 	fBaseY = 9.0;
	// 	fLength = 4;
	// 	fWidth = 2;
	// } else {
	// 	fBaseX = 22.0;
	// 	fBaseY = 14.0;
	// 	fLength = 6;
	// 	fWidth = 3;
	// }
	//
	// QPointF p1( fBaseX + std::cos( fCurrentAngle + fPi / 2 ) * fWidth / 2,
	// 			fBaseY + std::sin( fCurrentAngle + fPi / 2 ) * fWidth / 2 );
	// QPointF p2( p1.x() + std::cos( fCurrentAngle ) * fLength,
	// 			p1.y() + std::sin( fCurrentAngle ) * fLength );
	// QPointF p3( p2.x() - std::cos( fCurrentAngle + fPi / 2 ) * fWidth / 2,
	// 			p2.y() - std::sin( fCurrentAngle + fPi / 2 ) * fWidth / 2 );
	// QPointF p4( p3.x() - std::cos( fCurrentAngle ) * fLength,
	// 			p3.y() - std::sin( fCurrentAngle ) * fLength );
	// QPainterPath path;
	// path.moveTo( p1 );
	// path.lineTo( p2 );
	// path.lineTo( p3 );
	// path.lineTo( p4 );
	//
	// path.setFillRule( Qt::WindingFill );
	// QPen pen( colorHandle );
	// pen.setJoinStyle( Qt::RoundJoin );
	// pen.setWidth( 1.7 );
	// painter.setPen( pen );
	// painter.setBrush( QBrush( colorHandle ) );
	// painter.drawPath( path );
	///////////////////////

	if ( m_bIsActive ) {
		float fDistance, fRadius, fBaseX, fBaseY;
		if ( m_type == TYPE_SMALL ) {
			fBaseX = 9.0;
			fBaseY = 9.0;
			fDistance = 3;
			fRadius = 1;
		} else {
			fBaseX = 22.0;
			fBaseY = 14.0;
			fDistance = 4;
			fRadius = 1.5;
		}

		QPointF p1( fBaseX + std::cos( fCurrentAngle ) * fDistance,
					fBaseY + std::sin( fCurrentAngle ) * fDistance );
		painter.setPen( QPen( colorHandle, 1 ) );
		painter.setBrush( QBrush( colorHandle ) );
		painter.drawEllipse( p1, fRadius, fRadius );
	}

	if ( m_type != TYPE_SMALL ) {
		QRectF leftTextRec( 2, 15, 7, 7 );
		QRectF rightTextRec( 34, 15, 9, 7 );

		QFont font( H2Core::Preferences::get_instance()->getApplicationFontFamily() );
		font.setPixelSize( 9 );
		painter.setFont( font );
		painter.setPen( QPen( colorFont, 3 ) );
		painter.drawText( leftTextRec, Qt::AlignCenter, "-" );
		painter.drawText( rightTextRec, Qt::AlignCenter, "+" );
	}
}

void Rotary::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;
	update();
}

void Rotary::setValue( float fValue )
{
	if ( ! m_bIsActive ) {
		return;
	}
	
	if ( m_bUseIntSteps ) {
		fValue = std::round( fValue );
	}
	
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

void Rotary::enterEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bFocused = true;
	update();
}

void Rotary::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bFocused = false;
	update();
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
