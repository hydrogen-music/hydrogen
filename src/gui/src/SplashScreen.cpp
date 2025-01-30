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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <core/Version.h>
#include "SplashScreen.h"

#include <QPainter>
#include <QApplication>
#include <QScreen>

#include "Skin.h"

SplashScreen::SplashScreen()
 : QSplashScreen()
 , Object()
{
	//INFOLOG( "SplashScreen" );

	setFixedSize(width, height);

	m_pBackground = new QPixmap( Skin::getImagePath() + "/splash/splash.png" );

	QFont font;
	font.setPointSize( 10 );
	font.setBold( true );

	QPainter p;
	p.begin( m_pBackground );
	p.setFont( font );
	p.setPen( QColor( 20, 20, 20 ) );

	QString version = QString( "v%1 (%2)" ).arg( H2Core::get_version().c_str() ).arg( __DATE__ );
	p.drawText( 5, 5, width - 10, 40, Qt::AlignRight | Qt::AlignTop, version );

	p.end();

	setPixmap( *m_pBackground );

	// Center on screeen
	QRect rect( QGuiApplication::screens().first()->geometry() );
	move( rect.center() - this->rect().center() );

	QTimer::singleShot( 5000, this, SLOT( onCloseTimer() ) );
}




SplashScreen::~SplashScreen()
{
	//INFOLOG( "~SplashScreen" );
	delete m_pBackground;
}



void SplashScreen::onCloseTimer()
{
	hide();
}
