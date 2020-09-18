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
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

#include "UndoActions.h"
#include "../HydrogenApp.h"
#include "SongEditorPanelBpmWidget.h"
#include "SongEditorPanel.h"
#include "SongEditor.h"
#include <hydrogen/hydrogen.h>
#include <hydrogen/timeline.h>

namespace H2Core
{

const char* SongEditorPanelBpmWidget::__class_name = "SongEditorPanelBpmWidget";

SongEditorPanelBpmWidget::SongEditorPanelBpmWidget( QWidget* pParent, int beat )
	: QDialog( pParent )
	, Object( __class_name )
	, m_stimelineposition ( beat )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( tr( "BPM" ) );	
	setFixedSize( width(), height() );

	lineEditBeat->setText(QString("%1").arg( m_stimelineposition + 1) );
	deleteBtn->setEnabled ( false );

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Timeline* pTimeline = pHydrogen->getTimeline();
	auto tempoMarkers = pTimeline->getAllTempoMarkers();

	//restore the bpm value
	if( tempoMarkers.size() > 0 ){
		for ( int t = 0; t < tempoMarkers.size(); t++ ){
			if ( tempoMarkers[t]->nBar == m_stimelineposition ) {
				lineEditBpm->setText( QString("%1").arg( tempoMarkers[t]->fBpm ) );
				deleteBtn->setEnabled ( true );
				return;
			}
			else
			{
				lineEditBpm->setText( QString("%1").arg( pHydrogen->getNewBpmJTM()) );
			}
		}
	}else
	{
		lineEditBpm->setText( QString("%1").arg( pHydrogen->getNewBpmJTM() ) );
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
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Timeline* pTimeline = pHydrogen->getTimeline();
	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();

	float fOldBpm = -1.0;
	//search for an old entry
	if( tempoMarkerVector.size() >= 1 ){
		for ( int t = 0; t < tempoMarkerVector.size(); t++){
			if ( tempoMarkerVector[t]->nBar == ( QString( lineEditBeat->text() ).toInt() ) -1 ) {
				fOldBpm = tempoMarkerVector[t]->fBpm;
			}
		}
	}


	SE_editTimeLineAction *action = new SE_editTimeLineAction( lineEditBeat->text().toInt(), fOldBpm, QString( lineEditBpm->text() ).toFloat() );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
	accept();
}


void SongEditorPanelBpmWidget::on_deleteBtn_clicked()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Timeline* pTimeline = pHydrogen->getTimeline();
	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();

	float fOldBpm = -1.0;
	//search for an old entry
	if( tempoMarkerVector.size() >= 1 ){
		for ( int t = 0; t < tempoMarkerVector.size(); t++){
			if ( tempoMarkerVector[t]->nBar == ( QString( lineEditBeat->text() ).toInt() ) -1 ) {
				fOldBpm = tempoMarkerVector[t]->fBpm;
			}
		}
	}

	SE_deleteTimeLineAction *action = new SE_deleteTimeLineAction( lineEditBeat->text().toInt(), fOldBpm );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
	accept();
}

}
