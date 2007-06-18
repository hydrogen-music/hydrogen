/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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


#include "SplashScreen.h"

#include <QPainter>
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>

#include "Skin.h"


SplashScreen::SplashScreen()
// : QWidget( NULL, Qt::SplashScreen )
 : QSplashScreen( NULL )
 , Object( "SplashScreen" )
{
	//INFOLOG( "SplashScreen" );

	resize(width, height);
	setMinimumSize(width, height);
	setMaximumSize(width, height);

	m_pBackground = new QPixmap( Skin::getImagePath() + "/splash/splash.png" );

	QFont font;
	font.setPointSize( 10 );
	font.setBold( true );

	QPainter p;
	p.begin( m_pBackground );
	p.setFont( font );
	p.setPen( QColor( 20, 20, 20 ) );

	string version = "v" + string(VERSION) + " (" + string(__DATE__) + ")";
	p.drawText( 5, 5, width - 10, 40, Qt::AlignRight | Qt::AlignTop, QString( version.c_str() ) );

//	p.drawText( 5, height - 45, width - 10, 40, Qt::AlignHCenter | Qt::AlignBottom, QString( trUtf8( "Modules: %1" ) ).arg( COMPILED_FEATURES ) );
	p.end();

	setPixmap( *m_pBackground );


//	QPixmap logo = QPixmap( Skin::getImagePath() + "/splash/splash.png" );
//	setPixmap( logo );


/*
vecchio
	QLabel *logoLbl = new QLabel(this);
	logoLbl->move(0, 0);
	logoLbl->resize(400, 300);
	logoLbl->setPixmap( logo );
*/
//	setPixmap( *pLogo );

//	repaint();
//	show();
//
//
	// Center on screeen
	QRect rect( QApplication::desktop()->screenGeometry() );
	move( rect.center() - this->rect().center() );


	//connect( &m_closeTimer, SIGNAL(timeout()), this, SLOT(onCloseTimer()));
	//m_closeTimer.start( 5000 );

	QTimer::singleShot( 5000, this, SLOT( onCloseTimer() ) );
}




/**
 * Destructor
 */
SplashScreen::~SplashScreen()
{
	//INFOLOG( "~SplashScreen" );
}



void SplashScreen::onCloseTimer()
{
	hide();
}


/*void SplashScreen::drawContents ( QPainter *p )
{
	ERRORLOG( "drawContents" );
	p->drawPixmap( rect(), *m_pBackground, rect() );
*/
	/*
	QPixmap logo = QPixmap( Skin::getImagePath() + "/splash/splash.png" );

	p->drawPixmap( rect(), logo, rect() );


	QFont font;
	font.setPointSize( 10 );
	font.setBold( true );

	p->setFont( font );
	p->setPen( QColor( 20, 20, 20 ) );

	string version = "v" + string(VERSION) + " (" + string(__DATE__) + ")";
	p->drawText( 5, 5, width - 10, 40, Qt::AlignRight | Qt::AlignTop, QString( version.c_str() ) );

	p->drawText( 5, height - 45, width - 10, 40, Qt::AlignHCenter | Qt::AlignBottom, QString( trUtf8( "Modules: %1" ) ).arg( COMPILED_FEATURES ) );


	QLabel *logoLbl = new QLabel(this);
	logoLbl->move(0, 0);
	logoLbl->resize(400, 300);
	logoLbl->setPixmap(logo);

	*/
//}



