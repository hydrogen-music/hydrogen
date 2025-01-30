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

#include "LED.h"
#include "../Skin.h"

#include "../HydrogenApp.h"
#include <core/Globals.h>
#include <core/Preferences/Preferences.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>

LED::LED( QWidget *pParent, const QSize& size )
 : QWidget( pParent )
 , m_bActivated( false )
{
	setAttribute( Qt::WA_OpaquePaintEvent );
	setFixedSize( size );

	// Since the load function does not report success, we will check
	// for the existence of the background image separately.
	QString sPath;
	float fAspectRatio = static_cast<float>( size.width() ) / static_cast<float>( size.height() );
	if ( fAspectRatio < 1 ) {
		sPath = QString( Skin::getSvgImagePath() + "/led_5_13.svg" );
	} else {
		sPath = QString( Skin::getSvgImagePath() + "/led_11_9.svg" );
	}

	QFile file( sPath );
	if ( file.exists() ) {
		m_background = new QSvgRenderer( sPath, this );
	} else {
		m_background = nullptr;
		ERRORLOG( QString( "Unable to load background image [%1]" ).arg( sPath ) );
	}

	resize( size );
}

LED::~LED() {
}

void LED::setActivated( bool bActivated ) {
	m_bActivated = bActivated;
	update();
}

void LED::paintEvent( QPaintEvent* ev )
{
	QPainter painter( this );

	if ( m_background != nullptr ) {

		if ( m_bActivated ) {
			m_background->render( &painter, "layer2" );
		} else {
			m_background->render( &painter, "layer1" );
		}
	}
}

//////////////////////////////////////////////////////////////////////

MetronomeLED::MetronomeLED( QWidget *pParent, const QSize& size )
	: LED( pParent, size )
	, m_bFirstBar( false )
	, m_activityTimeout( 250 )
{
	HydrogenApp::get_instance()->addEventListener( this );
	
	// Since the load function does not report success, we will check
	// for the existence of the background image separately.
	QString sPath( Skin::getSvgImagePath() + "/led_22_7.svg" );
	QFile file( sPath );
	if ( file.exists() ) {
		m_background = new QSvgRenderer( sPath, this );
	} else {
		m_background = nullptr;
		ERRORLOG( QString( "Unable to load background image [%1]" ).arg( sPath ) );
	}

	m_pTimer = new QTimer( this );
	connect( m_pTimer, SIGNAL( timeout() ), this, SLOT( turnOff() ) );

	resize( size );
}

MetronomeLED::~MetronomeLED() {
}

void MetronomeLED::metronomeEvent( int nValue ) {

	// Only trigger LED if the metronome button was pressed or it was
	// activated via MIDI or OSC.
	if ( ! H2Core::Preferences::get_instance()->m_bUseMetronome ) {
		return;
	}
	
	m_bActivated = true;
	m_bFirstBar = nValue == 0;
	
	update();

	m_pTimer->start( std::chrono::duration_cast<std::chrono::milliseconds>( m_activityTimeout ).count() );
}

void MetronomeLED::turnOff() {
	m_pTimer->stop();
	m_bActivated = false;
	update();
}

void MetronomeLED::paintEvent( QPaintEvent* ev )
{
	QPainter painter( this );

	if ( m_background != nullptr ) {

		if ( m_bActivated ) {
			if ( m_bFirstBar ) {
				m_background->render( &painter, "layer3" );
			} else {
				m_background->render( &painter, "layer2" );
			}
		} else {
			m_background->render( &painter, "layer1" );
		}
	}
}
