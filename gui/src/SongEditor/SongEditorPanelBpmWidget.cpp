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
	setWindowTitle( trUtf8( "TimeLine Edit" ) );	
	setFixedSize( width(), height() );

	lineEditBEAT->setText(QString("%1").arg(m_stimelineposition + 1));
	deleteBtn->setEnabled ( false );
//	okBtn->setEnabled ( false );

	//restore the bpm value
	if(Hydrogen::get_instance()->m_timelinevector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(Hydrogen::get_instance()->m_timelinevector.size()); t++){
//			ERRORLOG(QString("%1 %2").arg(Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebeat).arg(m_stimelineposition));
			if ( Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebeat == m_stimelineposition ) {
				lineEditBPM->setText(QString("%1").arg(Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebpm));
				deleteBtn->setEnabled ( true );
				return;
			}
			else
			{
				lineEditBPM->setText(QString("%1").arg(Hydrogen::get_instance()->getNewBpmJTM()));
			}
		}
	}else
	{
		lineEditBPM->setText(QString("%1").arg(Hydrogen::get_instance()->getNewBpmJTM()));	
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
	//erase the value to set the new value
	if(Hydrogen::get_instance()->m_timelinevector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(Hydrogen::get_instance()->m_timelinevector.size()); t++){
			if ( Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebeat == (QString(lineEditBEAT->text()).toInt()) -1 ) {
				Hydrogen::get_instance()->m_timelinevector.erase( Hydrogen::get_instance()->m_timelinevector.begin() +  t);
			}
		}
	}

	Hydrogen::HTimelineVector tlvector;

	tlvector.m_htimelinebeat = (QString(lineEditBEAT->text()).toInt()) -1 ;
	float bpm;
	bpm = QString(lineEditBPM->text()).toFloat();
	if( bpm < 30.0 ) bpm = 30.0;
	if( bpm > 500.0 ) bpm = 500.0;	
	tlvector.m_htimelinebpm = bpm;
	Hydrogen::get_instance()->m_timelinevector.push_back( tlvector );
	Hydrogen::get_instance()->sortTimelineVector();
	accept();
}


void SongEditorPanelBpmWidget::on_deleteBtn_clicked()
{

	if(Hydrogen::get_instance()->m_timelinevector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(Hydrogen::get_instance()->m_timelinevector.size()); t++){
			if ( Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebeat == m_stimelineposition ) {
				Hydrogen::get_instance()->m_timelinevector.erase( Hydrogen::get_instance()->m_timelinevector.begin() +  t);
			}
		}
	}
	accept();
}

}