/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: SplashScreen.cpp,v 1.8 2005/05/01 19:50:59 comix Exp $
 *
 */


#include "SplashScreen.h"
#include "qpainter.h"
#include "Skin.h"


SplashScreen::SplashScreen()
 : QWidget( 0, "HydrogenSplashScreen", Qt::WStyle_Customize| Qt::WStyle_NoBorder | Qt::WStyle_StaysOnTop| Qt::WX11BypassWM )
 , Object("SplashScreen")
//SplashScreen::SplashScreen()
// : QWidget( 0, "HydrogenSplashScreen", Qt::WStyle_NoBorder | Qt::WStyle_StaysOnTop| Qt::WX11BypassWM )
// , Object("SplashScreen")
{
	setMinimumSize(width, height);
	setMaximumSize(width, height);
	resize(width, height);

	setCaption( trUtf8( "Hydrogen" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	string logo_path = Skin::getImagePath() + string( "/splash/splash.png" );
	QPixmap logo = QPixmap(logo_path.c_str());

	QFont font;
	font.setPointSize( 10 );
	font.setBold( true );

	QPainter p( &logo );
	p.setFont( font );
	p.setPen( QColor( 20, 20, 20 ) );

	string version = "v" + string(VERSION) + " (" + string(__DATE__) + ")";
	p.drawText( 5, 5, width - 10, 40, Qt::AlignRight | Qt::AlignTop, QString( version.c_str() ) );

	p.drawText( 5, height - 45, width - 10, 40, Qt::AlignHCenter | Qt::AlignBottom, trUtf8( QString("Modules: %1").arg( COMPILED_FEATURES ) ) );

	QLabel *logoLbl = new QLabel(this);
	logoLbl->move(0, 0);
	logoLbl->resize(400, 300);
	logoLbl->setPixmap(logo);

	QTimer::singleShot( 5000, this, SLOT( closeSplash() ) );

	// center...
	QRect rect = QApplication::desktop()->geometry();
	this->move( rect.center() - this->rect().center() );
}




/**
 * Destructor
 */
SplashScreen::~SplashScreen() {
//	cout << "destroy splashscreen" << endl;
}



/**
 * Close the splash screen
 */
void SplashScreen::closeSplash() {
//	infoLog( "[closeSplash]" );
	hide();
}


