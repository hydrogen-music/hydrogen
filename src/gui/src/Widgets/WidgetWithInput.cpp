/*
 * Hydrogen
 * Copyright (C) 2021 The hydrogen development team <hydrogen-devel@lists.sourceforge.net>
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
#include "WidgetWithInput.h"

WidgetWithInput::WidgetWithInput( QWidget* parent, bool bUseIntSteps, QString sBaseTooltip, int nScrollSpeed, int nScrollSpeedFast, float fMin, float fMax )
	: QWidget( parent )
	, m_bUseIntSteps( bUseIntSteps )
	, m_sBaseTooltip( sBaseTooltip )
	, m_nScrollSpeed( nScrollSpeed )
	, m_nScrollSpeedFast( nScrollSpeedFast )
	, m_fMin( fMin )
	, m_fMax( fMax )
	, m_fDefaultValue( fMin )
	, m_fValue( fMin )
	, m_fMousePressValue( 0.0 )
	, m_fMousePressY( 0.0 )
	, m_bIgnoreMouseMove( false )
	, m_bFocused( false )
	, m_bIsActive( true )
	, m_nWidgetHeight( 20 )
	, m_nWidgetWidth( 20 ){

	setAttribute( Qt::WA_Hover );
	setToolTip( sBaseTooltip );
}

WidgetWithInput::~WidgetWithInput() {}

void WidgetWithInput::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;
	update();
}

void WidgetWithInput::setValue( float fValue )
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
		emit valueChanged( this );
		update();
	}
}


void WidgetWithInput::mousePressEvent(QMouseEvent *ev)
{
	if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier ) {
		resetValueToDefault();
		m_bIgnoreMouseMove = true;
	}
	else if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ShiftModifier ) {
		MidiSenseWidget midiSense( this, true, this->getAction() );
		midiSense.exec();
		m_bIgnoreMouseMove = true;
	}
	else {
		setCursor( QCursor( Qt::SizeVerCursor ) );

		m_fMousePressValue = m_fValue;
		m_fMousePressY = ev->y();
	}
	
	QToolTip::showText( ev->globalPos(), QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
}

void WidgetWithInput::mouseReleaseEvent( QMouseEvent *ev )
{
	UNUSED( ev );
	
	setCursor( QCursor( Qt::ArrowCursor ) );

	m_bIgnoreMouseMove = false;
}

void WidgetWithInput::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	float fStepFactor;
	float fDelta = 1.0;

	if ( ev->modifiers() == Qt::ControlModifier ) {
		fStepFactor = m_nScrollSpeedFast;
	} else {
		fStepFactor = m_nScrollSpeed;
	}
	
	if ( !m_bUseIntSteps ) {
		float fRange = m_fMax - m_fMin;
		fDelta = fRange / 100.0;
	}
	if ( ev->angleDelta().y() < 0 ) {
		fDelta *= -1.;
	}
	setValue( getValue() + ( fDelta * fStepFactor ) );
	
	QToolTip::showText( ev->globalPos(), QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
}



void WidgetWithInput::mouseMoveEvent( QMouseEvent *ev )
{
	if ( m_bIgnoreMouseMove ) {
		return;
	}

	float fStepFactor;

	if ( ev->modifiers() == Qt::ControlModifier ) {
		fStepFactor = m_nScrollSpeedFast;
	} else {
		fStepFactor = m_nScrollSpeed;
	}

	float fRange = m_fMax - m_fMin;

	float fDeltaY = ev->y() - m_fMousePressY;
	float fNewValue = ( m_fMousePressValue - fStepFactor * ( fDeltaY / 100.0 * fRange ) );

	setValue( fNewValue );

	QToolTip::showText( ev->globalPos(), QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
}

void WidgetWithInput::enterEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bFocused = true;
	update();
}

void WidgetWithInput::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bFocused = false;
	update();
}

void WidgetWithInput::setMin( float fMin )
{
	if ( fMin == m_fMin ) {
		return;
	}
	if ( m_bUseIntSteps && std::fmod( fMin, 1.0 ) != 0.0 ) {
		___WARNINGLOG( QString( "As widget is set to use integer values only the supply minimal value [%1] will be rounded to [%2] " )
					.arg( fMin )
					.arg( std::round( fMin ) ) );
		fMin = std::round( fMin );
	}
	m_fMin = fMin;
	update();
}

void WidgetWithInput::setMax( float fMax )
{
	if ( fMax == m_fMax ) {
		return;
	}
	if ( m_bUseIntSteps && std::fmod( fMax, 1.0 ) != 0.0 ) {
		___WARNINGLOG( QString( "As widget is set to use integer values only the supply maximal value [%1] will be rounded to [%2] " )
					.arg( fMax )
					.arg( std::round( fMax ) ) );
		fMax = std::round( fMax );
	}
	
	m_fMax = fMax;
	update();
}


void WidgetWithInput::setDefaultValue( float fDefaultValue )
{
	if ( fDefaultValue == m_fDefaultValue ) {
		return;
	}

	if ( m_bUseIntSteps && std::fmod( fDefaultValue, 1.0 ) != 0.0 ) {
		___WARNINGLOG( QString( "As widget is set to use integer values only the supply default value [%1] will be rounded to [%2] " )
					.arg( fDefaultValue )
					.arg( std::round( fDefaultValue ) ) );
		fDefaultValue = std::round( fDefaultValue );
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

void WidgetWithInput::resetValueToDefault()
{
	setValue( m_fDefaultValue );
}

