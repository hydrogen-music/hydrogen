/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "StatusMessageDisplay.h"

#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"

#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

StatusMessageDisplay::StatusMessageDisplay( QWidget * pParent, const QSize& size )
	: LCDDisplay( pParent, size, false, false )
	, m_bEntered( false )
	, m_nShowTimeout( 5500 )
	, m_nScrollTimeout( 150 )
	, m_nPreScrollTimeout( 1500 )
	, m_nHistorySize( 100 )
	, m_bPreScroll( true )
{
	setReadOnly( true );
	setEnabled( true );

	m_pStatusTimer = new QTimer( this );
	connect( m_pStatusTimer, SIGNAL( timeout() ), this, SLOT( onStatusTimerEvent() ) );

	m_pScrollTimer = new QTimer( this );
	connect( m_pScrollTimer, SIGNAL( timeout() ), this, SLOT( onScrollTimerEvent() ) );
	m_sScrollMessage = "";

	updateMaxLength();
	updateStyleSheet();

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &StatusMessageDisplay::onPreferencesChanged );
}

StatusMessageDisplay::~StatusMessageDisplay() {
}

void StatusMessageDisplay::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	LCDDisplay::onPreferencesChanged( changes );
	updateStyleSheet();
	
	if ( changes & H2Core::Preferences::Changes::Font ) {
		updateMaxLength();
	}
}

// We need the widget to be enabled in order to handle mouse
// clicks. But the line edit itself if read-only and does not nicely
// integrate into the current GUI design using the active LCDDisplay
// foreground and background colors.
void StatusMessageDisplay::updateStyleSheet() {
	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	QColor textColor = theme.m_color.m_windowTextColor;
	QColor backgroundColor = theme.m_color.m_windowColor;

	QString sStyleSheet = QString( "\
QLineEdit { \
    color: %1; \
    background-color: %2; \
}" )
		.arg( textColor.name() )
		.arg( backgroundColor.name() );

	setStyleSheet( sStyleSheet );
}

void StatusMessageDisplay::paintEvent( QPaintEvent *ev ) {
	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	LCDDisplay::paintEvent( ev );

	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);

		QColor colorHighlightActive = theme.m_color.m_highlightColor;

		// If the mouse is placed on the widget but the user hasn't
		// clicked it yet, the highlight will be done more transparent to
		// indicate that keyboard inputs are not accepted yet.
		if ( ! hasFocus() ) {
			colorHighlightActive.setAlpha( 150 );
		}

		QPen pen;
		pen.setColor( colorHighlightActive );
		pen.setWidth( 3 );
		painter.setPen( pen );
		painter.drawRoundedRect( QRect( 0, 0, m_size.width() - 1, m_size.height() - 1 ), 3, 3 );
	}
}

#ifdef H2CORE_HAVE_QT6
void StatusMessageDisplay::enterEvent( QEnterEvent *ev ) {
#else
void StatusMessageDisplay::enterEvent( QEvent *ev ) {
#endif
	LCDDisplay::enterEvent( ev );
	m_bEntered = true;
	update();
}

void StatusMessageDisplay::leaveEvent( QEvent* ev ) {
	LCDDisplay::leaveEvent( ev );
	m_bEntered = false;
	update();
}

void StatusMessageDisplay::mousePressEvent( QMouseEvent* ev ) {
	if ( m_statusMessages.size() == 0 ) {
		// No messages to display yet.
		return;
	}

	QMenu* messageMenu = new QMenu( this );

	for ( const auto& sMessage : m_statusMessages ) {
		messageMenu->addAction( sMessage );
	}

	auto pEv = static_cast<MouseEvent*>( ev );

	messageMenu->popup( pEv->globalPosition().toPoint() );
}

void StatusMessageDisplay::showMessage( const QString& sMessage, const QString& sCaller ) {

	// Make sure widgets like sliders or rotaries do not flood the
	// status message history.
	if ( ! sCaller.isEmpty() && sCaller == m_sLastCaller ) {
		m_statusMessages.removeLast();
	}
	m_sLastCaller = sCaller;

	m_statusMessages << sMessage;

	if ( m_statusMessages.size() > m_nHistorySize ) {
		m_statusMessages.removeFirst();
	}
	
	m_sScrollMessage = sMessage;
	m_bPreScroll = true;

	displayMessage( sMessage );
}

void StatusMessageDisplay::displayMessage( const QString& sMessage )
{
	if ( m_pScrollTimer->isActive() ) {
		m_pScrollTimer->stop(); 
	}
	if ( m_pStatusTimer->isActive() ) {
		m_pStatusTimer->stop(); 
	}
	
	setText( sMessage );

	if ( sMessage.length() >= maxLength() ) {
		// Text is too large to fit in the display. Use scrolled
		// message instead.
		if ( m_bPreScroll ) {
			m_pScrollTimer->start( m_nPreScrollTimeout );
			m_bPreScroll = false;
		}
		else {
			m_pScrollTimer->start( m_nScrollTimeout );
		}
	}
	else {
		m_pStatusTimer->start( m_nShowTimeout );
	}
}

void StatusMessageDisplay::onScrollTimerEvent()
{
	m_sScrollMessage.remove( 0, 1 );

	displayMessage( m_sScrollMessage );
}

void StatusMessageDisplay::onStatusTimerEvent()
{
	reset();
}

void StatusMessageDisplay::reset()
{
	m_pStatusTimer->stop();
	m_pScrollTimer->stop();
	setText( "" );
	m_sScrollMessage = "";
}

void StatusMessageDisplay::updateMaxLength()
{
	QString sLongString( "ThisIsALongOneThatShouldNotFitInTheLCDDisplayEvenWithVeryNarrowFonts" );
	setMaxLength( 120 );
	
	while ( fontMetrics().size( Qt::TextSingleLine, sLongString ).width() >
			width() && ! sLongString.isEmpty() ) {
		sLongString.chop( 1 );
	}

	setMaxLength( sLongString.length() );
}
