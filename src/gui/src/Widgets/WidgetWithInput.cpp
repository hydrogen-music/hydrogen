/*
 * Hydrogen
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
#include "WidgetWithInput.h"

#include "../Compatibility/MouseEvent.h"
#include "../Compatibility/WheelEvent.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"
#include "MidiSenseWidget.h"

#include <core/Hydrogen.h>
#include <core/MidiMap.h>

#ifdef WIN32
#    include "core/Timehelper.h"
#else
#    include <unistd.h>
#    include <sys/time.h>
#endif

WidgetWithInput::WidgetWithInput( QWidget* parent, bool bUseIntSteps, QString sBaseTooltip, int nScrollSpeed, int nScrollSpeedFast, float fMin, float fMax, bool bModifyOnChange )
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
	, m_bEntered( false )
	, m_bIsActive( true )
	, m_nWidgetHeight( 20 )
	, m_nWidgetWidth( 20 )
	, m_sInputBuffer( "" )
	, m_inputBufferTimeout( 2.0 )
	, m_bModifyOnChange( bModifyOnChange ) {
	
	setAttribute( Qt::WA_Hover );
	setFocusPolicy( Qt::ClickFocus );
	updateTooltip();
	
	gettimeofday( &m_inputBufferTimeval, nullptr );
}

WidgetWithInput::~WidgetWithInput() {}

void WidgetWithInput::updateTooltip() {

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QString sTip = QString("%1: %2\n\n%3: [%4, %5]" ).arg( m_sBaseTooltip ).arg( m_fValue, 0, 'f', 2 )
		.arg( pCommonStrings->getRangeTooltip() )
		.arg( m_fMin, 0, 'f', 2 ).arg( m_fMax, 0, 'f', 2 );

	// Add the associated MIDI action.
	if ( m_pAction != nullptr ) {
		sTip.append( QString( "\n%1: %2 " ).arg( pCommonStrings->getMidiTooltipHeading() )
					 .arg( m_pAction->getType() ) );
		if ( m_registeredMidiEvents.size() > 0 ) {
			for ( const auto& [event, nnParam] : m_registeredMidiEvents ) {
				if ( event == H2Core::MidiMessage::Event::Note ||
					 event == H2Core::MidiMessage::Event::CC ) {
					sTip.append( QString( "\n%1 [%2 : %3]" )
								 .arg( pCommonStrings->getMidiTooltipBound() )
								 .arg( H2Core::MidiMessage::EventToQString( event ) )
								 .arg( nnParam ) );
				}
				else {
					// PC and MMC_x do not have a parameter.
					sTip.append( QString( "\n%1 [%2]" )
								 .arg( pCommonStrings->getMidiTooltipBound() )
								 .arg( H2Core::MidiMessage::EventToQString( event ) ) );
				}
			}

		}
		else {
			sTip.append( QString( "%1" ).arg( pCommonStrings->getMidiTooltipUnbound() ) );
		}
	}
			
	setToolTip( sTip );
}

void WidgetWithInput::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;
	update();
}

void WidgetWithInput::setValue( float fValue, bool bTriggeredByUserInteraction )
{
	if ( ! m_bIsActive ) {
		return;
	}
	
	if ( m_bUseIntSteps ) {
		fValue = std::round( fValue );
	} else {
		if ( std::abs( fValue ) < 1E-6 ) {
			// The calculation of the increment when altering the
			// value via drag or mouse wheel - (m_fMax - m_fMin)/100.0
			// - introduces rounding errors. These become dominant
			// when trying to reset the widget's value to zero and
			// cause a mismatch.
			fValue = 0.0;
		}
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
		updateTooltip();
		update();

		if ( m_bModifyOnChange && bTriggeredByUserInteraction ) {
			H2Core::Hydrogen::get_instance()->setIsModified( true );
		}
	}
}


void WidgetWithInput::mousePressEvent(QMouseEvent *ev)
{
	if ( ! m_bIsActive ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );
	
	if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier ) {
		resetValueToDefault();
		m_bIgnoreMouseMove = true;
	}
	else if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ShiftModifier ) {
		MidiSenseWidget midiSense( this, true, this->m_pAction );
		midiSense.exec();
	
		m_bIgnoreMouseMove = true;
	}
	else {
		setCursor( QCursor( Qt::SizeVerCursor ) );

		m_fMousePressValue = m_fValue;
		m_fMousePressY = pEv->position().y();
	}
	
	QToolTip::showText( pEv->globalPosition().toPoint(),
						QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
}

void WidgetWithInput::mouseReleaseEvent( QMouseEvent *ev )
{
	UNUSED( ev );
	
	if ( ! m_bIsActive ) {
		return;
	}
	
	setCursor( QCursor( Qt::ArrowCursor ) );

	m_bIgnoreMouseMove = false;
}

void WidgetWithInput::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();
	
	if ( ! m_bIsActive ) {
		return;
	}

	auto pEv = static_cast<WheelEvent*>( ev );

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

	setValue( getValue() + ( fDelta * fStepFactor ), true );
	
	QToolTip::showText( pEv->globalPosition().toPoint(),
						QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
}



void WidgetWithInput::mouseMoveEvent( QMouseEvent *ev )
{
	
	if ( ! m_bIsActive || m_bIgnoreMouseMove ) {
		return;
	}

	float fStepFactor;

	auto pEv = static_cast<MouseEvent*>( ev );

	if ( ev->modifiers() == Qt::ControlModifier ) {
		fStepFactor = m_nScrollSpeedFast;
	} else {
		fStepFactor = m_nScrollSpeed;
	}

	float fRange = m_fMax - m_fMin;

	float fDeltaY = pEv->position().y() - m_fMousePressY;
	float fNewValue = ( m_fMousePressValue - fStepFactor * ( fDeltaY / 100.0 * fRange ) );

	setValue( fNewValue, true );

	QToolTip::showText( pEv->globalPosition().toPoint(),
						QString( "%1" ).arg( m_fValue, 0, 'f', 2 ) , this );
}

#ifdef H2CORE_HAVE_QT6
void WidgetWithInput::enterEvent( QEnterEvent *ev ) {
#else
void WidgetWithInput::enterEvent( QEvent *ev ) {
#endif
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void WidgetWithInput::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void WidgetWithInput::keyPressEvent( QKeyEvent *ev ) {

	if ( ! m_bIsActive ) {
		return;
	}
	
	float fIncrement;
	if ( !m_bUseIntSteps ) {
		fIncrement = ( m_fMax - m_fMin ) / 100.0;
	} else {
		fIncrement = 1.0;
	}
	
	if ( ev->key() == Qt::Key_Right || ev->key() == Qt::Key_Up ) {
		if ( ev->modifiers() == Qt::ControlModifier ) {
			fIncrement *= m_nScrollSpeedFast;
		} else {
			fIncrement *= m_nScrollSpeed;
		}
		setValue( m_fValue + fIncrement, true );
	} else if ( ev->key() == Qt::Key_PageUp ) {
		setValue( m_fValue + fIncrement * m_nScrollSpeedFast, true );
	} else if ( ev->key() == Qt::Key_Home ) {
		setValue( m_fMax, true );
	} else if ( ev->key() == Qt::Key_Left || ev->key() == Qt::Key_Down ) {
		if ( ev->modifiers() == Qt::ControlModifier ) {
			fIncrement *= m_nScrollSpeedFast;
		} else {
			fIncrement *= m_nScrollSpeed;
		}
		setValue( m_fValue - fIncrement, true );
	} else if ( ev->key() == Qt::Key_PageDown ) {
		setValue( m_fValue - fIncrement * m_nScrollSpeedFast, true );
	} else if ( ev->key() == Qt::Key_Home ) {
		setValue( m_fMin, true );
	} else if ( ( ev->key() >= Qt::Key_0 && ev->key() <= Qt::Key_9 ) || ev->key() == Qt::Key_Minus || ev->key() == Qt::Key_Period ) {

		timeval now;
		gettimeofday( &now, nullptr );
		// Flush the input buffer if there was no user input for X
		// seconds
		if ( ( static_cast<double>( now.tv_sec ) +
			   static_cast<double>( now.tv_usec * US_DIVIDER ))  -
			 ( static_cast<double>( m_inputBufferTimeval.tv_sec ) +
			   static_cast<double>( m_inputBufferTimeval.tv_usec * US_DIVIDER ) ) >
			 m_inputBufferTimeout ) {
			m_sInputBuffer.clear();
		}
		m_inputBufferTimeval = now;

		if ( ev->key() == Qt::Key_Period  ) {
			m_sInputBuffer += ".";
		} else if ( ev->key() == Qt::Key_Minus ) {
			m_sInputBuffer += "-";
		} else {
			m_sInputBuffer += QString::number( ev->key() - 48 );
		}

		bool bOk;
		float fNewValue = m_sInputBuffer.toFloat( &bOk );
		if ( ! bOk ) {
			return;
		}
		setValue( fNewValue, true );
		update();
	} else if (  ev->key() == Qt::Key_Escape ) {
		// reset the input buffer
		m_sInputBuffer.clear();
		QToolTip::hideText();
		return;
	} else {
		// Return without showing a tooltop. Let MainForm handle the event for
		// virtual keyboard and shortcuts.
		HydrogenApp::get_instance()->getMainForm()->eventFilter( this, ev );
		return;
	}
	
	QPoint p( mapToGlobal( QPoint( 0,0 ) ) );
	QToolTip::showText( QPoint( p.x() + width(), p.y() ), QString( "%1" ).arg( m_fValue, 0, 'f', 2 ), this, geometry(), m_inputBufferTimeout * 1000 );
}

void WidgetWithInput::setMin( float fMin )
{
	if ( fMin == m_fMin ) {
		return;
	}
	if ( m_bUseIntSteps && std::fmod( fMin, 1.0 ) != 0.0 && ! std::isinf( fMin ) ) {
		___WARNINGLOG( QString( "As widget is set to use integer values only the supply minimal value [%1] will be rounded to [%2] " )
					.arg( fMin )
					.arg( std::round( fMin ) ) );
		fMin = std::round( fMin );
	}

	if ( fMin >= m_fMax ) {
		___ERRORLOG( QString( "Supplied value [%1] must be smaller than maximal one [%2]" )
					 .arg( fMin ).arg( m_fMax ) );
		return;
	}

	if ( fMin != m_fMin ) {
		m_fMin = fMin;

		if ( m_fValue < fMin ) {
			setValue( fMin, false );
		}
		update();
	}
}

void WidgetWithInput::setMax( float fMax )
{
	if ( fMax == m_fMax ) {
		return;
	}
	if ( m_bUseIntSteps && std::fmod( fMax, 1.0 ) != 0.0 && ! std::isinf( fMax ) ) {
		___WARNINGLOG( QString( "As widget is set to use integer values only the supply maximal value [%1] will be rounded to [%2] " )
					.arg( fMax )
					.arg( std::round( fMax ) ) );
		fMax = std::round( fMax );
	}

	if ( fMax >= m_fMin ) {
		___ERRORLOG( QString( "Supplied value [%1] must be bigger than the minimal one [%2]" )
					 .arg( fMax ).arg( m_fMin ) );
		return;
	}

	if ( fMax != m_fMax ) {
		m_fMax = fMax;

		if ( m_fValue > fMax ) {
			setValue( fMax, false );
		}
		update();
	}
}


void WidgetWithInput::setDefaultValue( float fDefaultValue )
{
	if ( fDefaultValue == m_fDefaultValue ) {
		return;
	}

	if ( m_bUseIntSteps && std::fmod( fDefaultValue, 1.0 ) != 0.0 && ! std::isinf( fDefaultValue ) ) {
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
	setValue( m_fDefaultValue, true );
}
