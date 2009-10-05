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


#include "MetroBlinker.h"
#include "HydrogenApp.h"
#include "widgets/PixmapWidget.h"

#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>



using namespace H2Core;
using namespace std;

MetroBlinker::MetroBlinker ( QWidget* pParent )
		: QDialog ( pParent )
		, Object ( "MetroBlinker" )
{

	installEventFilter(this);
	HydrogenApp::get_instance()->addEventListener( this );
	setupUi ( this );
	INFOLOG ( "INIT" );
	setWindowTitle ( trUtf8 ( "Director" ) );

	p_counter = 1;	// to compute the right beat
	p_fadealpha = 255;	//default alpha
	p_bar = 1;	// default bar
	p_wechselblink = 0;

	beatblinkerLabel->setText(QString("1"));
	contentblinkerLabel->setText(QString("infos"));
	p_bpm = Hydrogen::get_instance()->getSong()->__bpm;
	timer = new QTimer( this );
	connect(timer, SIGNAL(timeout() ), this, SLOT( updateMetronomBackground() ) );
}


MetroBlinker::~MetroBlinker()
{
	INFOLOG ( "DESTROY" );
}


void MetroBlinker::metronomeEvent( int nValue )
{
	//bpm
	p_bpm = Hydrogen::get_instance()->getSong()->__bpm; 
	//bar
	p_bar = Hydrogen::get_instance()->getPatternPos() +1;
	// 1000 ms / bpm / 60s
	timer->start( static_cast<int>( 1000 / ( p_bpm / 60 )) / 2 );
	p_wechselblink = 0;
	p_fadealpha = 255;
	if (nValue == 2){
		p_fadealpha = 0;
		return;
	}
	if (nValue == 1) {	//foregroundcolor "rect" for first blink
		p_color = QColor( 255, 50, 1 ,255 );
		p_counter = 1;		
	}
	else {	//foregroundcolor "rect" for all other blinks
		p_counter++;
		if(p_counter %2 == 0) 
			p_wechselblink = width() / 2;

		p_color = QColor( 24, 250, 31, 255 );

	}
	update();
}


void MetroBlinker::updateMetronomBackground()
{
	p_color.setAlpha( 0 );
	timer->stop();
	update();
}


void MetroBlinker::paintEvent( QPaintEvent* ev)
{
	QPainter painter(this);
	painter.setPen( QPen(QColor( 249, 235, 116, 200 ) ,1 , Qt::SolidLine) );
	painter.setBrush( p_color );
	painter.drawRect ( 20.0 + p_wechselblink, 20.0, width() -40.0 - (width() / 2), height() - 40.0 );
	beatblinkerLabel->setText(QString("%1").arg(p_counter));
	barblinkerLabel->setText(QString("%1").arg(p_bar));

}
