/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include "Skin.h"
#include "Widgets/PixmapWidget.h"

#include <core/Preferences/Preferences.h>
#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Timeline.h>
#include <core/Helpers/Filesystem.h>

using namespace H2Core;

Director::Director ( QWidget* pParent )
		: QDialog ( pParent )
		, Object ()
{

	HydrogenApp::get_instance()->addEventListener( this );
	setupUi ( this );

	auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	setWindowTitle ( tr ( "Director" ) );

	m_fBpm = pAudioEngine->getTransportPosition()->getBpm();
	m_nBar = pAudioEngine->getTransportPosition()->getColumn() + 1;
	if ( m_nBar <= 0 ){
		m_nBar = 1;
	}
	
	m_nCounter = 1;	// to compute the right beat
	m_Color = pPref->getColorTheme()->m_accentColor;
	m_Color.setAlpha( 0 );
	m_nFlashingArea = width() * 5/100;

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

void Director::updateSongEvent( int nValue ) {

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	if ( nValue == 0 || // new song loaded
		 nValue == 1 ) { // current one saved

		// Update song name
		QStringList list = pSong->getFilename().split("/");

		if ( !list.isEmpty() ){
			m_sSongName = list.last().replace( Filesystem::songs_ext, "" );

			// if songname is not set, default on an empty song, we call them "Untitled Song".
			if( m_sSongName.isEmpty() ){
				m_sSongName = QString("Untitled Song");
			}
		}

		timelineUpdateEvent( 0 );

		update();
	}
}

void Director::timelineUpdateEvent( int nValue ) {

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	m_fBpm = pAudioEngine->getTransportPosition()->getBpm();
	m_nBar = pAudioEngine->getTransportPosition()->getColumn() + 1;

	if ( m_nBar <= 0 ){
		m_nBar = 1;
	}
	
	// get tags
	auto pTimeline = pHydrogen->getTimeline();

	if ( pTimeline->hasColumnTag( m_nBar ) ) {
		m_sTAG = pTimeline->getTagAtColumn( m_nBar );
	} else {
		m_sTAG = "";
	}
	m_sTAG2 = pTimeline->getTagAtColumn( m_nBar - 1 );
	update();
}

void Director::metronomeEvent( int nValue )
{

	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = H2Core::Preferences::get_instance();

	//bpm
	m_fBpm = pHydrogen->getSong()->getBpm();
	//bar
	m_nBar = pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() + 1;

	if ( m_nBar <= 0 ){
		m_nBar = 1;
	}

	// 1000 ms / bpm / 60s
	m_pTimer->start( static_cast<int>( 1000 / ( m_fBpm / 60 )) / 2 );
	m_nFlashingArea = width() * 5/100;

	if ( nValue == 1 ) {	//foregroundcolor "rect" for first blink
		m_Color = pPref->getColorTheme()->m_buttonRedColor;
		m_nCounter = 1;
	}
	else {	//foregroundcolor "rect" for all other blinks
		m_nCounter++;
		if( m_nCounter %2 == 0 ) {
			m_nFlashingArea = width() * 52.5/100;
		}

		m_Color = pPref->getColorTheme()->m_accentColor;
	}
	
	// get tags
	auto pTimeline = pHydrogen->getTimeline();

	if ( pTimeline->hasColumnTag( m_nBar ) ) {
		m_sTAG = pTimeline->getTagAtColumn( m_nBar );
	} else {
		m_sTAG = "";
	}
	m_sTAG2 = pTimeline->getTagAtColumn( m_nBar - 1 );
	
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

	auto pPref = H2Core::Preferences::get_instance();
	QString sFontFamily = pPref->getApplicationFontFamily();

	//draw the songname
	painter.setFont(QFont( sFontFamily, height() * 14/100 ));
	QRect rect(QPoint( width() * 5/100 , height () * 2/100 ), QSize( width() * 90/100, height() * 21/100));
	painter.drawText( rect, Qt::AlignCenter,  QString( m_sSongName ) );


	//draw the metronome
	
	painter.setPen( QPen( pPref->getColorTheme()->m_highlightColor, 1 , Qt::SolidLine ) );
	painter.setBrush( m_Color );
	painter.drawRect (  m_nFlashingArea, height() * 25/100, width() * 42.5/100, height() * 35/100);


	//draw bars
	QColor textColor = pPref->getColorTheme()->m_windowTextColor;
	painter.setPen( textColor );
	painter.setFont(QFont( sFontFamily, height() * 25/100 ));
	QRect r1(QPoint( width() * 5/100 , height() * 25/100 ), QSize( width() * 42.5/100, height() * 35/100));
	painter.drawText( r1, Qt::AlignCenter, QString("%1").arg( m_nBar) );

	//draw beats
	QRect r2(QPoint( width() * 52.5/100 , height() * 25/100 ), QSize( width() * 42.5/100, height() * 35/100));
	painter.drawText( r2, Qt::AlignCenter, QString("%1").arg( m_nCounter) );

	if( m_sTAG == m_sTAG2 ){
		m_sTAG2 = "";
	}
	
	//draw current bar tag
	painter.setFont(QFont( sFontFamily, height() * 8/100 ));
	QRect r3(QPoint ( width() * 5/100 , height() * 65/100 ), QSize( width() * 90/100, height() * 14/100));
	painter.drawText( r3, Qt::AlignCenter, QString( (m_sTAG) ) );

	//draw next bar tag
	painter.setPen( Skin::makeTextColorInactive( textColor ) );
	painter.setFont(QFont( sFontFamily, height() * 6/100 ));
	QRect r4(QPoint ( width() * 5/100 , height() * 83/100 ), QSize( width() * 90/100, height() * 11/100));
	painter.drawText( r4, Qt::AlignCenter, QString( m_sTAG2 ) );
}

void Director::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
			 
		update();
	}
}
