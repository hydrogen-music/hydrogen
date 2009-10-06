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

#include <QtGui>


#include "../HydrogenApp.h"
#include "SongEditorPanelTagWidget.h"
#include "SongEditorPanel.h"
#include "SongEditor.h"
#include <hydrogen/hydrogen.h>

namespace H2Core
{

SongEditorPanelTagWidget::SongEditorPanelTagWidget( QWidget* pParent, int beat )
	: QDialog( pParent )
	, Object( "SongEditorPanelTagWidget" )
	, m_stimelineposition ( beat )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( trUtf8( "Tag" ) );	
	setFixedSize( width(), height() );

	lineEditBeat->setText(QString("%1").arg( m_stimelineposition + 1) );
	deleteTagBtn->setEnabled ( false );

	Hydrogen* engine = Hydrogen::get_instance();
	std::vector<Hydrogen::HTimelineTagVector> timelineTagVector = engine->m_timelinetagvector;

	//restore the bpm value
	if( timelineTagVector.size() > 0 ){
		for ( int t = 0; t < timelineTagVector.size(); t++ ){
//			ERRORLOG(QString("%1 %2").arg(Hydrogen::get_instance()->m_timelinevector[t].m_htimelinetagbeat).arg(m_stimelineposition));
			if ( timelineTagVector[t].m_htimelinetagbeat == m_stimelineposition ) {
				lineEditTag->setText( QString("%1").arg( timelineTagVector[t].m_htimelinetag ) );
				deleteTagBtn->setEnabled ( true );
				return;
			}
			else
			{
				lineEditTag->setText( QString( "Tag" ) );
			}
		}
	}else
	{
		lineEditTag->setText( QString( "Tag" ) );
	}
}




SongEditorPanelTagWidget::~SongEditorPanelTagWidget()
{
	INFOLOG( "DESTROY" );
}



void SongEditorPanelTagWidget::on_CancelBtn_clicked()
{
	reject();
}



void SongEditorPanelTagWidget::on_okBtn_clicked()
{
	Hydrogen* engine = Hydrogen::get_instance();
	
	//erase the value to set the new value
	if( engine->m_timelinetagvector.size() >= 1 ){
		for ( int t = 0; t < engine->m_timelinetagvector.size(); t++){
			if ( engine->m_timelinetagvector[t].m_htimelinetagbeat == ( QString( lineEditBeat->text() ).toInt() ) -1 ) {
				engine->m_timelinetagvector.erase( engine->m_timelinetagvector.begin() +  t);
			}
		}
	}

	Hydrogen::HTimelineTagVector tlvector;

	tlvector.m_htimelinetagbeat = ( QString( lineEditBeat->text() ).toInt() ) -1 ;
	QString tag;
	tag = QString( lineEditTag->text() );
	tlvector.m_htimelinetag = tag;
	engine->m_timelinetagvector.push_back( tlvector );
	engine->sortTimelineTagVector();
	accept();
}


void SongEditorPanelTagWidget::on_deleteTagBtn_clicked()
{
	Hydrogen* engine = Hydrogen::get_instance();
	std::vector<Hydrogen::HTimelineTagVector> timelineTagVector = engine->m_timelinetagvector;
	
	if( timelineTagVector.size() > 0 ){
		for ( int t = 0; t < timelineTagVector.size(); t++){
			if ( timelineTagVector[t].m_htimelinetagbeat == m_stimelineposition ) {
				timelineTagVector.erase( timelineTagVector.begin() +  t);
			}
		}
	}
	
	engine->m_timelinetagvector = timelineTagVector;
	accept();
}

}
