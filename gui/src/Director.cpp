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


 /**
 **
 ** this dialog is used to use show a director.
 ** for example to play live without a click in your ears.
 ** here you get :
 ** 	- song name
 ** 	- a visual metronome
 ** 	- bar position info
 ** 	- beat position info
 **	- bar position tags (current and next)
 **
 **	-------------------------------------------
 **	|                                           |
 **	|                 song name                 |
 **	|                                           |	
 **	-------------------------------------------
 **	|                     |                     |
 **	|        Bar          |       Beat          |	
 **	|                     |                     |	
 **	-------------------------------------------
 **	|                                           |
 **	|            current bar tag                |
 **	|                                           |	
 **	-------------------------------------------
 **	|                                           |
 **	|              next bar tag                 |
 **	|                                           |
 **	-------------------------------------------
 **/


#include "Director.h"
#include "HydrogenApp.h"
#include "widgets/PixmapWidget.h"

#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <QRect>


using namespace H2Core;
using namespace std;
Director::Director ( QWidget* pParent )
		: QDialog ( pParent )
		, Object ( "Director" )
{

	HydrogenApp::get_instance()->addEventListener( this );
	setupUi ( this );
	INFOLOG ( "INIT" );
	setWindowTitle ( trUtf8 ( "Director" ) );

	p_counter = 1;	// to compute the right beat
	p_fadealpha = 255;	//default alpha
	p_bar = 1;	// default bar
	p_wechselblink = width() * 5/100;

	p_bpm = Hydrogen::get_instance()->getSong()->__bpm;
	timer = new QTimer( this );
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateMetronomBackground() ) );
}


Director::~Director()
{
	INFOLOG ( "DESTROY" );
}


void Director::metronomeEvent( int nValue )
{

	//load a new song
	if( nValue == 3 ){
		
		//update songname
		QStringList list = Hydrogen::get_instance()->getSong()->get_filename().split("/");

		if ( !list.isEmpty() ){
			songName = list.last().replace( ".h2song", "" );
		}
		
		update();
		return;
	}


	//bpm
	p_bpm = Hydrogen::get_instance()->getSong()->__bpm; 
	//bar
	p_bar = Hydrogen::get_instance()->getPatternPos() +1;
	if ( p_bar <= 0 )
		p_bar = 1;
	// 1000 ms / bpm / 60s
	timer->start( static_cast<int>( 1000 / ( p_bpm / 60 )) / 2 );
	p_wechselblink = width() * 5/100;
	p_fadealpha = 255;
	if ( nValue == 2 ){
		p_fadealpha = 0;
		update();
		TAG="";
		TAG2="";
		return;
	}
	if ( nValue == 1 ) {	//foregroundcolor "rect" for first blink
		p_color = QColor( 255, 50, 1 ,255 );
		p_counter = 1;		
	}
	else {	//foregroundcolor "rect" for all other blinks
		p_counter++;
		if( p_counter %2 == 0 ) 
			p_wechselblink = width() * 52.5/100;

		p_color = QColor( 24, 250, 31, 255 );

	}

	//get tags

	if(Hydrogen::get_instance()->m_timelinetagvector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(Hydrogen::get_instance()->m_timelinetagvector.size()); t++){
			if ( Hydrogen::get_instance()->m_timelinetagvector[t].m_htimelinetagbeat == p_bar ){
				TAG2 =  Hydrogen::get_instance()->m_timelinetagvector[t].m_htimelinetag ;
			}
			else if ( Hydrogen::get_instance()->m_timelinetagvector[t].m_htimelinetagbeat == p_bar - 1){
				TAG =  Hydrogen::get_instance()->m_timelinetagvector[t].m_htimelinetag ;
			}
		}
	}else
	{
		TAG="";
		TAG2="";
	}

	update();
}


void Director::updateMetronomBackground()
{
	p_color.setAlpha( 0 );
	timer->stop();
	update();
}


void Director::paintEvent( QPaintEvent* ev )
{
	QPainter painter(this);

	//draw the songname
	painter.setFont(QFont("Arial", height() * 14/100 ));
	QRect rect(QPoint( width() * 5/100 , height () * 2/100 ), QSize( width() * 90/100, height() * 21/100));
	painter.drawText( rect, Qt::AlignCenter,  QString( songName ) );


	//draw the metronome
	painter.setPen( QPen(QColor( 249, 235, 116, 200 ) ,1 , Qt::SolidLine ) );
	painter.setBrush( p_color );
	painter.drawRect (  p_wechselblink, height() * 25/100, width() * 42.5/100, height() * 35/100);


	//draw bars
	painter.setPen(Qt::white);
	painter.setFont(QFont("Arial", height() * 25/100 ));
	QRect r1(QPoint( width() * 5/100 , height() * 25/100 ), QSize( width() * 42.5/100, height() * 35/100));
	painter.drawText( r1, Qt::AlignCenter, QString("%1").arg( p_bar) );

	//draw beats
	QRect r2(QPoint( width() * 52.5/100 , height() * 25/100 ), QSize( width() * 42.5/100, height() * 35/100));
	painter.drawText( r2, Qt::AlignCenter, QString("%1").arg( p_counter) );

	if( TAG == TAG2 )
		 TAG2 = "";
	//draw current bar tag
	painter.setPen(Qt::white);
	painter.setFont(QFont("Arial", height() * 8/100 ));
	QRect r3(QPoint ( width() * 5/100 , height() * 65/100 ), QSize( width() * 90/100, height() * 14/100));
	painter.drawText( r3, Qt::AlignCenter, QString( (TAG) ) );

	//draw next bar tag
	painter.setPen(Qt::gray);
	painter.setFont(QFont("Arial", height() * 6/100 ));
	QRect r4(QPoint ( width() * 5/100 , height() * 83/100 ), QSize( width() * 90/100, height() * 11/100));
	painter.drawText( r4, Qt::AlignCenter, QString( TAG2 ) );
}
