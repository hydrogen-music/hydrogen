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
#include <hydrogen/timeline.h>
#include <hydrogen/helpers/filesystem.h>

using namespace H2Core;
using namespace std;

Director::Director ( QWidget* pParent )
		: QDialog ( pParent )
		, Object ( "Director" )
{

	HydrogenApp::get_instance()->addEventListener( this );
	setupUi ( this );
	//INFOLOG ( "INIT" );
	setWindowTitle ( trUtf8 ( "Director" ) );

	m_nCounter = 1;	// to compute the right beat
	m_nFadeAlpha = 255;	//default alpha
	m_nBar = 1;	// default bar
	m_nFlashingArea = width() * 5/100;

	m_fBpm = Hydrogen::get_instance()->getSong()->__bpm;
	m_pTimeline = Hydrogen::get_instance()->getTimeline();
	m_pTimer = new QTimer( this );
	connect( m_pTimer, SIGNAL( timeout() ), this, SLOT( updateMetronomBackground() ) );
}


Director::~Director()
{
	//INFOLOG ( "DESTROY" );
}

void Director::keyPressEvent( QKeyEvent* ev )
{
	if(ev->key() == Qt::Key_Escape) {
		HydrogenApp::get_instance()->showDirector();
	}
}

void Director::closeEvent( QCloseEvent* ev )
{
	HydrogenApp::get_instance()->showDirector();
}

void Director::metronomeEvent( int nValue )
{

	//load a new song
	if( nValue == 3 ){

		//update songname
		QStringList list = Hydrogen::get_instance()->getSong()->get_filename().split("/");

		if ( !list.isEmpty() ){
			m_sSongName = list.last().replace( Filesystem::songs_ext, "" );

			// if songname is not set, default on an empty song, we call them "Untitled Song".
			if( m_sSongName.isEmpty() ){
				m_sSongName = QString("Untitled Song");
			}
		}

		update();
		return;
	}

	//bpm
	m_fBpm = Hydrogen::get_instance()->getSong()->__bpm;
	//bar
	m_nBar = Hydrogen::get_instance()->getPatternPos() + 1;

	if ( m_nBar <= 0 ){
		m_nBar = 1;
	}

	// 1000 ms / bpm / 60s
	m_pTimer->start( static_cast<int>( 1000 / ( m_fBpm / 60 )) / 2 );
	m_nFlashingArea = width() * 5/100;
	m_nFadeAlpha = 255;

	if ( nValue == 2 ){
		m_nFadeAlpha = 0;
		update();
		m_sTAG="";
		m_sTAG2="";
		return;
	}

	if ( nValue == 1 ) {	//foregroundcolor "rect" for first blink
		m_Color = QColor( 255, 50, 1 ,255 );
		m_nCounter = 1;
	}
	else {	//foregroundcolor "rect" for all other blinks
		m_nCounter++;
		if( m_nCounter %2 == 0 )
			m_nFlashingArea = width() * 52.5/100;

		m_Color = QColor( 24, 250, 31, 255 );
	}

	// get tags
	m_sTAG="";
	m_sTAG2="";
	for ( size_t t = 0; t < m_pTimeline->m_timelinetagvector.size(); t++){
		if(t+1<m_pTimeline->m_timelinetagvector.size() &&
				m_pTimeline->m_timelinetagvector[t+1].m_htimelinetagbeat == m_nBar ){
			m_sTAG2 =  m_pTimeline->m_timelinetagvector[t+1].m_htimelinetag ;
		}
		if ( m_pTimeline->m_timelinetagvector[t].m_htimelinetagbeat <= m_nBar-1){
			m_sTAG =  m_pTimeline->m_timelinetagvector[t].m_htimelinetag ;
		}
		if( m_pTimeline->m_timelinetagvector[t].m_htimelinetagbeat > m_nBar-1){
			break;
		}
	}
	update();
}


void Director::updateMetronomBackground()
{
	m_Color.setAlpha( 0 );
	m_pTimer->stop();
	update();
}


void Director::paintEvent( QPaintEvent* ev )
{
	QPainter painter(this);

	//draw the songname
	painter.setFont(QFont("Arial", height() * 14/100 ));
	QRect rect(QPoint( width() * 5/100 , height () * 2/100 ), QSize( width() * 90/100, height() * 21/100));
	painter.drawText( rect, Qt::AlignCenter,  QString( m_sSongName ) );


	//draw the metronome
	painter.setPen( QPen(QColor( 249, 235, 116, 200 ) ,1 , Qt::SolidLine ) );
	painter.setBrush( m_Color );
	painter.drawRect (  m_nFlashingArea, height() * 25/100, width() * 42.5/100, height() * 35/100);


	//draw bars
	painter.setPen(Qt::white);
	painter.setFont(QFont("Arial", height() * 25/100 ));
	QRect r1(QPoint( width() * 5/100 , height() * 25/100 ), QSize( width() * 42.5/100, height() * 35/100));
	painter.drawText( r1, Qt::AlignCenter, QString("%1").arg( m_nBar) );

	//draw beats
	QRect r2(QPoint( width() * 52.5/100 , height() * 25/100 ), QSize( width() * 42.5/100, height() * 35/100));
	painter.drawText( r2, Qt::AlignCenter, QString("%1").arg( m_nCounter) );

	if( m_sTAG == m_sTAG2 ){
		m_sTAG2 = "";
	}
	
	//draw current bar tag
	painter.setPen(Qt::white);
	painter.setFont(QFont("Arial", height() * 8/100 ));
	QRect r3(QPoint ( width() * 5/100 , height() * 65/100 ), QSize( width() * 90/100, height() * 14/100));
	painter.drawText( r3, Qt::AlignCenter, QString( (m_sTAG) ) );

	//draw next bar tag
	painter.setPen(Qt::gray);
	painter.setFont(QFont("Arial", height() * 6/100 ));
	QRect r4(QPoint ( width() * 5/100 , height() * 83/100 ), QSize( width() * 90/100, height() * 11/100));
	painter.drawText( r4, Qt::AlignCenter, QString( m_sTAG2 ) );
}
