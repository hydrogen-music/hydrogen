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

#include "Button.h"

#include "../Skin.h"
#include "MidiSenseWidget.h"

#include <qglobal.h>	// for QT_VERSION

#include <core/Globals.h>
#include <core/Preferences.h>

const char* Button::__class_name = "Button";

Button::Button( QWidget *pParent, QSize size, const QString& sIcon, const QString& sText, bool bUseRedBackground, QSize iconSize, bool bEnablePressHold )
 : QWidget( pParent )
 , Object( __class_name )
 , m_bIsPressed( false )
 , m_bMouseOver( false )
 , m_sText( sText )
 , m_bUseRedBackground( bUseRedBackground )
 , m_iconSize( iconSize )
 , m_bEnablePressHold( bEnablePressHold )
{
	setAttribute( Qt::WA_OpaquePaintEvent );
	setFixedSize( size );
	m_nWidth = size.width();
	m_nHeight = size.height();

	// Since the load function does not report success, we will check
	// for the existance of the background image separately.
	QString sPath;
	float fAspectRatio = static_cast<float>( m_nWidth ) / static_cast<float>( m_nHeight );
	if ( fAspectRatio < 0.6216216 ) {
		sPath = QString( Skin::getSvgImagePath() + "/button_9_37.svg" );
	} else if ( fAspectRatio > 0.6216216 && fAspectRatio < 1.2647059 ) {
		sPath = QString( Skin::getSvgImagePath() + "/button_17_17.svg" );
	} else if ( fAspectRatio > 1.2647059 && fAspectRatio < 1.7352941 ) {
		sPath = QString( Skin::getSvgImagePath() + "/button_26_17.svg" );
	} else if ( fAspectRatio > 1.7352941 && fAspectRatio < 3.264706 ) {
		sPath = QString( Skin::getSvgImagePath() + "/button_42_13.svg" );
	} else {
		sPath = QString( Skin::getSvgImagePath() + "/button_94_13.svg" );
	}

	QFile file( sPath );
	if ( file.exists() ) {
		m_background = new QSvgRenderer( sPath, this );
	} else {
		m_background = nullptr;
		ERRORLOG( QString( "Unable to load background image [%1]" ).arg( sPath ) );
	}
		
	if ( ! sIcon.isEmpty() ) {
		// Since the load function does not report success, we will check
		// for the existance of the background image separately.
		QString sPathIcon( Skin::getSvgImagePath() + "/icons/" + sIcon );

		QFile iconFile( sPathIcon );
		if ( iconFile.exists() ) {
			m_icon = new QSvgRenderer( sPathIcon, this );
		} else {
			m_icon = nullptr;
			ERRORLOG( QString( "Unable to load icon image [%1]" ).arg( sPathIcon ) );
		}
	} else {
		m_icon = nullptr;

		if ( sText.isEmpty() ) {
			ERRORLOG( "Neither an icon nor a text was provided. Button will be empty." );
		}
	}

	m_timerTimeout = 0;
	m_timer = new QTimer(this);
	connect(m_timer, SIGNAL(timeout()), this, SLOT(buttonPressed_timer_timeout()));
	
	resize( size );
}

Button::~Button() {
}

void Button::mousePressEvent(QMouseEvent*ev) {
	
	/*
	*  Shift + Left-Click activate the midi learn widget
	*/
	
	if ( ev->button() == Qt::LeftButton && ( ev->modifiers() & Qt::ShiftModifier ) ){
		MidiSenseWidget midiSense( this, true, this->getAction() );
		midiSense.exec();
		return;
	}
	
	m_bIsPressed = true;
	update();
	emit mousePress( this );

	if ( ev->button() == Qt::LeftButton && m_bEnablePressHold) {
		m_timerTimeout = 2000;
		buttonPressed_timer_timeout();
	}
}



void Button::mouseReleaseEvent( QMouseEvent* ev )
{
	setPressed( false );

	if ( ev->button() == Qt::LeftButton ) {
		if ( m_bEnablePressHold ) {
			m_timer->stop();
		} else {
			emit clicked( this );
		}
	}
	else if ( ev->button() == Qt::RightButton ) {
		emit rightClicked( this );
	}

}


void Button::buttonPressed_timer_timeout()
{
	emit clicked(this);

	if( m_timerTimeout > 100 ) {
		m_timerTimeout = m_timerTimeout / 2;
	}
	
	m_timer->start( m_timerTimeout );
}

void Button::setPressed( bool bIsPressed )
{
	if ( bIsPressed != m_bIsPressed ) {
		m_bIsPressed = bIsPressed;
		update();
	}
}

void Button::enterEvent( QEvent *ev )
{
	UNUSED( ev );
	m_bMouseOver = true;
	update();
}

void Button::leaveEvent( QEvent *ev )
{
	UNUSED( ev );
	m_bMouseOver = false;
	update();
}

void Button::paintEvent( QPaintEvent* ev )
{
	QPainter painter( this );

	if ( m_background != nullptr ) {

		if ( m_bIsPressed ) {
			if ( m_bUseRedBackground ) {
				m_background->render( &painter, "layer4" );
			} else {
				m_background->render( &painter, "layer3" );
			}
		} else if ( m_bMouseOver ) {
			m_background->render( &painter, "layer2" );
		} else {
			m_background->render( &painter, "layer1" );
		}
	}

	if ( m_icon != nullptr ) {
		QSize size;
		if ( m_iconSize.isEmpty() ) {
			size = QSize( m_icon->defaultSize() );
			if ( size.width() >= m_nWidth ) {
				size.setWidth( m_nWidth - 5 );
			}
			if ( size.height() >= m_nHeight ) {
				size.setHeight( m_nHeight - 5 );
			}
		} else {
			size = m_iconSize;
		}

		// Center icon in widget.
		QRect rect( 0.5 * ( m_nWidth - size.width() ), 0.5 * ( m_nHeight - size.height() ),
					size.width(), size.height() );
		m_icon->render( &painter, rect );
	}
	
	// // background
	// if ( m_bIsPressed ) {
	// 	if (__use_skin_style) {
	// 		static int w = 5;
	// 		static int h = m_onPixmap.height();

	// 		// central section, scaled
	// 		painter.drawPixmap( QRect(w, 0, width() - w * 2, h), m_onPixmap, QRect(10, 0, w, h) );

	// 		// left side
	// 		painter.drawPixmap( QRect(0, 0, w, h), m_onPixmap, QRect(0, 0, w, h) );

	// 		// right side
	// 		painter.drawPixmap( QRect(width() - w, 0, w, h), m_onPixmap, QRect(m_onPixmap.width() - w, 0, w, h) );
	// 	}
	// 	else {
	// 		painter.drawPixmap( ev->rect(), m_onPixmap, ev->rect() );
	// 	}
	// }
	// else {
	// 	if (m_bMouseOver) {
	// 		if (__use_skin_style) {
	// 			static int w = 5;
	// 			static int h = m_overPixmap.height();

	// 			// central section, scaled
	// 			painter.drawPixmap( QRect(w, 0, width() - w * 2, h), m_overPixmap, QRect(10, 0, w, h) );

	// 			// left side
	// 			painter.drawPixmap( QRect(0, 0, w, h), m_overPixmap, QRect(0, 0, w, h) );

	// 			// right side
	// 			painter.drawPixmap( QRect(width() - w, 0, w, h), m_overPixmap, QRect(m_overPixmap.width() - w, 0, w, h) );
	// 		}
	// 		else {
	// 			painter.drawPixmap( ev->rect(), m_overPixmap, ev->rect() );
	// 		}
	// 	}
	// 	else {
	// 		if (__use_skin_style) {
	// 			static int w = 5;
	// 			static int h = m_offPixmap.height();

	// 			// central section, scaled
	// 			painter.drawPixmap( QRect(w, 0, width() - w * 2, h), m_offPixmap, QRect(10, 0, w, h) );

	// 			// left side
	// 			painter.drawPixmap( QRect(0, 0, w, h), m_offPixmap, QRect(0, 0, w, h) );

	// 			// right side
	// 			painter.drawPixmap( QRect(width() - w, 0, w, h), m_offPixmap, QRect(m_offPixmap.width() - w, 0, w, h) );
	// 		}
	// 		else {
	// 			painter.drawPixmap( ev->rect(), m_offPixmap, ev->rect() );
	// 		}
	// 	}
	// }


	if ( ! m_sText.isEmpty() ) {
		painter.setFont( QFont( H2Core::Preferences::get_instance()->getApplicationFontFamily(), 6 ) );

		QColor shadow(150, 150, 150, 100);
		QColor text(10, 10, 10);

		if (m_bMouseOver) {
			shadow = QColor(220, 220, 220, 100);
		}

		// shadow
		painter.setPen( shadow );
		painter.drawText( 1, 1, width(), height(), Qt::AlignHCenter | Qt::AlignVCenter,  m_sText );

		// text
		painter.setPen( text );
		painter.drawText( 0, 0, width(), height(), Qt::AlignHCenter | Qt::AlignVCenter,  m_sText );

	}

}

void Button::setText( const QString& sText )
{
	m_sText = sText;
	update();
}



// :::::::::::::::::::::::::



ToggleButton::ToggleButton( QWidget *pParent, QSize size, const QString& sIcon, const QString& sText, bool bUseRedBackground, QSize iconSize )
	: Button( pParent, size, sIcon, sText, bUseRedBackground, iconSize, false ) {
}

ToggleButton::~ToggleButton() {
}

void ToggleButton::mousePressEvent( QMouseEvent *ev ) {
	
	if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ShiftModifier ){
		MidiSenseWidget midiSense( this, true, this->getAction() );
		midiSense.exec();
		return;
	}
	
	if (ev->button() == Qt::RightButton) {
		emit rightClicked( this );
	}
	else {
		if ( m_bIsPressed ) {
			m_bIsPressed = false;
		} else {
			m_bIsPressed = true;
		}
		update();
		
		emit clicked(this);
	}
}

void ToggleButton::mouseReleaseEvent(QMouseEvent*) {
	// do nothing, this method MUST override Button's one
}

LEDButton::LEDButton( QWidget *pParent, QSize size )
	: Button( pParent, size, "", "LED", false, QSize( 0, 0 ), false )
{
	setFixedSize( size );

	// Since the load function does not report success, we will check
	// for the existance of the background image separately.
	QString sPath( Skin::getSvgImagePath() + "/led.svg" );

	QFile file( sPath );
	if ( file.exists() ) {
		m_background = new QSvgRenderer( sPath, this );
	} else {
		m_background = nullptr;
		ERRORLOG( QString( "Unable to load background image [%1]" ).arg( sPath ) );
	}

	m_icon = nullptr;	
	resize( size );
}

LEDButton::~LEDButton(){
}

void LEDButton::paintEvent( QPaintEvent* ev )
{
	QPainter painter( this );

	if ( m_background != nullptr ) {

		if ( m_bIsPressed ) {
			m_background->render( &painter, "layer2" );
		} else {
			m_background->render( &painter, "layer1" );
		}
	}
}
