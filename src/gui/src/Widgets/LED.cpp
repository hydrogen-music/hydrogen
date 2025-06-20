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
	if ( m_bActivated == bActivated ) {
		return;
	}
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
