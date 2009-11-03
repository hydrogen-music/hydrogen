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
#include "SongEditorPanelBpmWidget.h"
#include "SongEditorPanel.h"
#include "SongEditor.h"
#include <hydrogen/hydrogen.h>

namespace H2Core
{

SongEditorPanelBpmWidget::SongEditorPanelBpmWidget( QWidget* pParent, int beat )
	: QDialog( pParent )
	, Object( "SongEditorPanelBpmWidget" )
	, m_stimelineposition ( beat )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( trUtf8( "BPM" ) );	
	setFixedSize( width(), height() );

	lineEditBeat->setText(QString("%1").arg( m_stimelineposition + 1) );
	deleteBtn->setEnabled ( false );

	Hydrogen* engine = Hydrogen::get_instance();
	std::vector<Hydrogen::HTimelineVector> timelineVector = engine->m_timelinevector;

	//restore the bpm value
	if( timelineVector.size() > 0 ){
		for ( int t = 0; t < timelineVector.size(); t++ ){
//			ERRORLOG(QString("%1 %2").arg(Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebeat).arg(m_stimelineposition));
			if ( timelineVector[t].m_htimelinebeat == m_stimelineposition ) {
				lineEditBpm->setText( QString("%1").arg( timelineVector[t].m_htimelinebpm ) );
				deleteBtn->setEnabled ( true );
				return;
			}
			else
			{
				lineEditBpm->setText( QString("%1").arg( engine->getNewBpmJTM()) );
			}
		}
	}else
	{
		lineEditBpm->setText( QString("%1").arg( engine->getNewBpmJTM() ) );
	}
}




SongEditorPanelBpmWidget::~SongEditorPanelBpmWidget()
{
	INFOLOG( "DESTROY" );
}



void SongEditorPanelBpmWidget::on_CancelBtn_clicked()
{
	reject();
}



void SongEditorPanelBpmWidget::on_okBtn_clicked()
{
	Hydrogen* engine = Hydrogen::get_instance();
	
	//erase the value to set the new value
	if( engine->m_timelinevector.size() >= 1 ){
		for ( int t = 0; t < engine->m_timelinevector.size(); t++){
			if ( engine->m_timelinevector[t].m_htimelinebeat == ( QString( lineEditBeat->text() ).toInt() ) -1 ) {
				engine->m_timelinevector.erase( engine->m_timelinevector.begin() +  t);
			}
		}
	}

	Hydrogen::HTimelineVector tlvector;

	tlvector.m_htimelinebeat = ( QString( lineEditBeat->text() ).toInt() ) -1 ;
	float bpm;
	bpm = QString( lineEditBpm->text() ).toFloat();
	if( bpm < 30.0 ) bpm = 30.0;
	if( bpm > 500.0 ) bpm = 500.0;	
	tlvector.m_htimelinebpm = bpm;
	engine->m_timelinevector.push_back( tlvector );
	engine->sortTimelineVector();
	accept();
}


void SongEditorPanelBpmWidget::on_deleteBtn_clicked()
{
	Hydrogen* engine = Hydrogen::get_instance();
	std::vector<Hydrogen::HTimelineVector> timelineVector = engine->m_timelinevector;
	
	if( timelineVector.size() > 0 ){
		for ( int t = 0; t < timelineVector.size(); t++){
			if ( timelineVector[t].m_htimelinebeat == m_stimelineposition ) {
				timelineVector.erase( timelineVector.begin() +  t);
			}
		}
	}
	
	engine->m_timelinevector = timelineVector;
	accept();
}

}
