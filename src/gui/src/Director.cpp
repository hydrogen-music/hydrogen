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
	
	setWindowTitle ( tr ( "Director" ) );

	const auto theme = H2Core::Preferences::get_instance()->getTheme();
	auto pPos = H2Core::Hydrogen::get_instance()->getAudioEngine()
		->getTransportPosition();
	
	m_nBar = pPos->getBar();
	m_nBeat = pPos->getBeat();
	m_Color = theme.m_color.m_accentColor;
	m_Color.setAlpha( 0 );
	m_nFlashingArea = width() * 5/100;

	m_pTimer = new QTimer( this );
	connect( m_pTimer, SIGNAL( timeout() ), this, SLOT( updateMetronomBackground() ) );

	updateLabelContainers();
}


Director::~Director()
{
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

void Director::tempoChangedEvent( int ) {
	bbtChangedEvent();
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

		updateTags();
		updateFontSize( FontUpdate::SongName );
		update();
	}
}

void Director::timelineUpdateEvent( int nValue ) {
	if ( updateTags() ) {
		update();
	}
}

bool Director::updateTags() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pTimeline = pHydrogen->getTimeline();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		return false;
	}
	
	const int nColumns = pSong->getPatternGroupVector()->size();

	bool bRequiresUpdate = false;
	// Note that bar = column + 1
	if ( m_nBar == nColumns ) {
		if ( pSong->getLoopMode() == Song::LoopMode::Enabled &&
			 m_sTagNext != pTimeline->getTagAtColumn( 0 ) ) {
			// We are in the last column and transport will be looped back
			// to the beginning. Show the tag in the first column as the
			// next one.
			m_sTagNext = pTimeline->getTagAtColumn( 0 );
			updateFontSize( FontUpdate::TagNext );
			bRequiresUpdate = true;
		}
	}
	else if ( m_sTagNext != pTimeline->getTagAtColumn( m_nBar ) ) {
		m_sTagNext = pTimeline->getTagAtColumn( m_nBar );
		updateFontSize( FontUpdate::TagNext );
		bRequiresUpdate = true;
	}
	if ( m_sTagCurrent != pTimeline->getTagAtColumn( m_nBar - 1 ) ) {
		m_sTagCurrent = pTimeline->getTagAtColumn( m_nBar - 1 );
		updateFontSize( FontUpdate::TagCurrent );
		bRequiresUpdate = true;
	}

	return bRequiresUpdate;
}

void Director::bbtChangedEvent()
{
	const auto theme = H2Core::Preferences::get_instance()->getTheme();
	auto pPos = Hydrogen::get_instance()->getAudioEngine()->getTransportPosition();

	// 1000 ms / bpm / 60s
	m_pTimer->start( static_cast<int>( 1000 / ( pPos->getBpm() / 60 )) / 2 );
	m_nFlashingArea = width() * 5/100;

	m_nBar = pPos->getBar();
	m_nBeat = pPos->getBeat();

	if ( m_nBeat == 1 ) {	//foregroundcolor "rect" for first blink
		m_Color = theme.m_color.m_buttonRedColor;
	}
	else {	//foregroundcolor "rect" for all other blinks
		if ( m_nBeat %2 == 0 ) {
			m_nFlashingArea = width() * 52.5/100;
		}
		m_Color = theme.m_color.m_accentColor;
	}
	
	updateTags();
	update();
}


void Director::updateMetronomBackground()
{
	m_Color.setAlpha( 0 );
	m_pTimer->stop();
	update();
}

void Director::updateFontSize( FontUpdate update ) {
	const QString sFontFamily =
		H2Core::Preferences::get_instance()->getTheme().m_font.m_sApplicationFontFamily;

	// Reduce the pixelsize of the font till it fits the width of its
	// enclosing rectangle.
	auto shrinkTillItFits = [&]( QFont* pFont, const QRect& rect, const QString& sLabel ) {
		if ( sLabel.isEmpty() ) {
			return;
		}

		while ( ( QFontMetrics( *pFont ).size( Qt::TextSingleLine, sLabel ).width() >
				  rect.width() ) &&
				pFont->pointSize() > 1 ) {
			pFont->setPointSize( pFont->pointSize() - 1 );
		}
	};

	if ( update & FontUpdate::SongName ) {
		// Reset to default value
		m_fontSongName = QFont( sFontFamily, height() * 14/100 );
		shrinkTillItFits( &m_fontSongName, m_rectSongName, m_sSongName );
	}

	if ( update & FontUpdate::TagCurrent ) {
		m_fontTagCurrent = QFont( sFontFamily, height() * 8/100 );
		shrinkTillItFits( &m_fontTagCurrent, m_rectTagCurrent, m_sTagCurrent );
	}

	if ( update & FontUpdate::TagNext ) {
		m_fontTagNext = QFont( sFontFamily, height() * 6/100 );
		shrinkTillItFits( &m_fontTagNext, m_rectTagNext, m_sTagNext );
	}
}

void Director::updateLabelContainers() {
	m_rectSongName = QRect( QPoint( width() * 5/100 , height () * 2/100 ),
						    QSize( width() * 90/100, height() * 21/100) );
	m_rectTagCurrent = QRect( QPoint( width() * 5/100 , height() * 65/100 ),
							  QSize( width() * 90/100, height() * 14/100) );
	m_rectTagNext = QRect( QPoint( width() * 5/100 , height() * 83/100 ),
						   QSize( width() * 90/100, height() * 11/100) );

	updateFontSize( static_cast<FontUpdate>( FontUpdate::SongName |
											 FontUpdate::TagCurrent |
											 FontUpdate::TagNext ) );
}

void Director::resizeEvent( QResizeEvent* ev ) {
	updateLabelContainers();

	QDialog::resizeEvent( ev );
	update();
}

void Director::paintEvent( QPaintEvent* ev )
{
	QPainter painter(this);

	const auto theme = H2Core::Preferences::get_instance()->getTheme();
	const QString sFontFamily = theme.m_font.m_sApplicationFontFamily;

	//draw the songname
	painter.setFont( m_fontSongName );
	painter.drawText( m_rectSongName, Qt::AlignCenter, m_sSongName );


	//draw the metronome
	painter.setPen( QPen( theme.m_color.m_highlightColor, 1 , Qt::SolidLine ) );
	painter.setBrush( m_Color );
	painter.drawRect( m_nFlashingArea, height() * 25/100, width() * 42.5/100,
					  height() * 35/100);


	//draw bars
	QColor textColor = theme.m_color.m_windowTextColor;
	painter.setPen( textColor );
	painter.setFont(QFont( sFontFamily, height() * 25/100 ));
	QRect r1(QPoint( width() * 5/100 , height() * 25/100 ), QSize( width() * 42.5/100, height() * 35/100));
	painter.drawText( r1, Qt::AlignCenter, QString("%1").arg( m_nBar) );

	//draw beats
	QRect r2(QPoint( width() * 52.5/100 , height() * 25/100 ), QSize( width() * 42.5/100, height() * 35/100));
	painter.drawText( r2, Qt::AlignCenter, QString("%1").arg( m_nBeat) );

	if( m_sTagNext == m_sTagCurrent ){
		m_sTagNext = "";
	}
	
	//draw current bar tag
	painter.setFont( m_fontTagCurrent );
	painter.drawText( m_rectTagCurrent, Qt::AlignCenter, m_sTagCurrent );

	//draw next bar tag
	painter.setPen( Skin::makeTextColorInactive( textColor ) );
	painter.setFont( m_fontTagNext );
	painter.drawText( m_rectTagNext, Qt::AlignCenter, QString( m_sTagNext ) );
}

void Director::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
			 
		update();
	}
}
