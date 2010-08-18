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

#include "PixmapWidget.h"
#include "../Skin.h"
#include "MidiSenseWidget.h"

#include <qglobal.h>	// for QT_VERSION

#if QT_VERSION == 0x040100	// SVG renderer was introduced in QT4.1
	#include <QSvgRenderer>
#endif

#include <hydrogen/globals.h>


Button::Button( QWidget * pParent, const QString& sOnImage, const QString& sOffImage, const QString& sOverImage, QSize size, bool use_skin_style )
 : QWidget( pParent )
 , Object( "Button" )
 , m_bPressed( false )
 , m_onPixmap( size )
 , m_offPixmap( size )
 , m_overPixmap( size )
 , m_bMouseOver( false )
 , __use_skin_style(use_skin_style)
{
	// draw the background: slower but useful with transparent images!
	//setAttribute(Qt::WA_NoBackground);

	setMinimumSize( size );
	setMaximumSize( size );
	resize( size );

	if ( loadImage( sOnImage, m_onPixmap ) == false ) {
		m_onPixmap.fill( QColor( 0, 255, 0 ) );
	}

	if ( loadImage( sOffImage, m_offPixmap ) == false ) {
		m_offPixmap.fill( QColor( 0, 100, 0 ) );
	}

	if ( loadImage( sOverImage, m_overPixmap ) == false ) {
		m_overPixmap.fill( QColor( 0, 180, 0 ) );
	}

	// default text font
	m_textFont.setPointSize( 10 );
	m_textFont.setBold( true );
}



Button::~Button()
{
}



bool Button::loadImage( const QString& sFilename, QPixmap& pixmap )
{
#if QT_VERSION == 0x040100	// SVG renderer was introduced in QT4.1
  /*
	if ( sFilename.endsWith( ".svg" ) ) {
		ERRORLOG( "************* LOAD SVG!!" );
		if ( !QFile::exists( sFilename ) ) {
			return false;
		}
		QSvgRenderer doc( sFilename );
		if ( doc.isValid() == false ) {
			ERRORLOG( "error loading SVG image: '" + sFilename.toLocal8Bit().constData() + "'" );
			return false;
		}

		QPainter p;
		p.begin( &pixmap );
		p.setViewport( 0, 0, width(), height() );
		p.eraseRect( 0, 0, width(), height() );
		doc.render( &p );
		p.end();
		return true;
	}
  */
#endif
	// load an image
	if ( pixmap.load( Skin::getImagePath() + sFilename ) == false ) {
		if ( !sFilename.isEmpty() ) {
			ERRORLOG( QString( "Error loading image: '%1'" ).arg( sFilename ) );
		}
		return false;
	}
	return true;
}


void Button::mousePressEvent(QMouseEvent*ev) {

    /*
    *  Shift + Left-Click activate the midi learn widget
    */

    if ( ev->button() == Qt::LeftButton && (ev->modifiers() & Qt::ShiftModifier) ){
	    MidiSenseWidget midiSense( this, true, this->getAction() );
	    midiSense.exec();
	    return;
    }

    m_bPressed = true;
    update();
    emit mousePress(this);
}



void Button::mouseReleaseEvent(QMouseEvent* ev)
{
	setPressed( false );

	if (ev->button() == Qt::LeftButton) {
		emit clicked(this);
	}
	else if (ev->button() == Qt::RightButton) {
		emit rightClicked(this);
	}

}

void Button::setFontSize(int size)
{
	m_textFont.setPointSize(size);
}

void Button::setPressed(bool pressed)
{
	if (pressed != m_bPressed) {
		m_bPressed = pressed;
		update();
	}
}






void Button::enterEvent(QEvent *ev)
{
	UNUSED( ev );
	m_bMouseOver = true;
	update();
}



void Button::leaveEvent(QEvent *ev)
{
	UNUSED( ev );
	m_bMouseOver = false;
	update();
}



void Button::paintEvent( QPaintEvent* ev)
{
	QPainter painter(this);

	// background
	if (m_bPressed) {
		if (__use_skin_style) {
			static int w = 5;
			static int h = m_onPixmap.height();

			// central section, scaled
			painter.drawPixmap( QRect(w, 0, width() - w * 2, h), m_onPixmap, QRect(10, 0, w, h) );

			// left side
			painter.drawPixmap( QRect(0, 0, w, h), m_onPixmap, QRect(0, 0, w, h) );

			// right side
			painter.drawPixmap( QRect(width() - w, 0, w, h), m_onPixmap, QRect(m_onPixmap.width() - w, 0, w, h) );
		}
		else {
			painter.drawPixmap( ev->rect(), m_onPixmap, ev->rect() );
		}
	}
	else {
		if (m_bMouseOver) {
			if (__use_skin_style) {
				static int w = 5;
				static int h = m_overPixmap.height();

				// central section, scaled
				painter.drawPixmap( QRect(w, 0, width() - w * 2, h), m_overPixmap, QRect(10, 0, w, h) );

				// left side
				painter.drawPixmap( QRect(0, 0, w, h), m_overPixmap, QRect(0, 0, w, h) );

				// right side
				painter.drawPixmap( QRect(width() - w, 0, w, h), m_overPixmap, QRect(m_overPixmap.width() - w, 0, w, h) );
			}
			else {
				painter.drawPixmap( ev->rect(), m_overPixmap, ev->rect() );
			}
		}
		else {
			if (__use_skin_style) {
				static int w = 5;
				static int h = m_offPixmap.height();

				// central section, scaled
				painter.drawPixmap( QRect(w, 0, width() - w * 2, h), m_offPixmap, QRect(10, 0, w, h) );

				// left side
				painter.drawPixmap( QRect(0, 0, w, h), m_offPixmap, QRect(0, 0, w, h) );

				// right side
				painter.drawPixmap( QRect(width() - w, 0, w, h), m_offPixmap, QRect(m_offPixmap.width() - w, 0, w, h) );
			}
			else {
				painter.drawPixmap( ev->rect(), m_offPixmap, ev->rect() );
			}
		}
	}


	if ( !m_sText.isEmpty() ) {
		painter.setFont( m_textFont );

		QColor shadow(150, 150, 150, 100);
		QColor text(10, 10, 10);

		if (m_bMouseOver) {
			shadow = QColor(150, 250, 150, 100);
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



ToggleButton::ToggleButton( QWidget *pParent, const QString& sOnImg, const QString& sOffImg, const QString& sOverImg, QSize size, bool use_skin_style )
 : Button( pParent, sOnImg, sOffImg, sOverImg, size, use_skin_style )
{

}



ToggleButton::~ToggleButton() {
}



void ToggleButton::mousePressEvent(QMouseEvent *ev) {

	if ( ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ShiftModifier ){
	    MidiSenseWidget midiSense( this, true, this->getAction() );
	    midiSense.exec();
	    return;
	}

	if (ev->button() == Qt::RightButton) {
                emit rightClicked(this);
        }
	else {
		if (m_bPressed) {
			m_bPressed = false;
		}
		else {
			m_bPressed = true;
		}
		update();

		emit clicked(this);
	}
}



void ToggleButton::mouseReleaseEvent(QMouseEvent*) {
	// do nothing, this method MUST override Button's one
}
