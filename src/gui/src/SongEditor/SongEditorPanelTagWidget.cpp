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
#include "SongEditorPanelTagWidget.h"
#include "SongEditorPanel.h"
#include "SongEditor.h"

#include <hydrogen/hydrogen.h>
#include <hydrogen/timeline.h>

namespace H2Core
{

const char* SongEditorPanelTagWidget::__class_name = "SongEditorPanelTagWidget";

SongEditorPanelTagWidget::SongEditorPanelTagWidget( QWidget* pParent, int beat )
	: QDialog( pParent )
	, Object( __class_name )
	, m_stimelineposition ( beat )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( tr( "Tag" ) );
	createTheTagTableWidget();

	connect( tagTableWidget, SIGNAL( itemChanged ( QTableWidgetItem *  ) ), this, SLOT( a_itemIsChanged( QTableWidgetItem * ) ) );
}



SongEditorPanelTagWidget::~SongEditorPanelTagWidget()
{
	INFOLOG( "DESTROY" );
}

void SongEditorPanelTagWidget::a_itemIsChanged(QTableWidgetItem *item)
{
	__theChangedItems << QString( "%1" ).arg( item->row() );
}

void SongEditorPanelTagWidget::createTheTagTableWidget()
{
	Hydrogen* engine = Hydrogen::get_instance();
	Timeline* pTimeline = engine->getTimeline();
	int patterngroupvectorsize;
	patterngroupvectorsize = engine->getSong()->get_pattern_group_vector()->size();
	
	for( int i = 0; i < patterngroupvectorsize; i++ )
	{
		tagTableWidget->insertRow( i );
	}

	std::vector<Timeline::HTimelineTagVector> timelineTagVector = pTimeline->m_timelinetagvector;

	//read the tag vector and fill all tags into items
	if( timelineTagVector.size() > 0 ){
		for ( unsigned int t = 0; t < timelineTagVector.size(); t++ ){
			QTableWidgetItem *newTagItem = new QTableWidgetItem();
			newTagItem->setText( QString( "%1" ).arg( timelineTagVector[t].m_htimelinetag ) );
			tagTableWidget->setItem( timelineTagVector[t].m_htimelinetagbeat, 0, newTagItem );
			tagTableWidget->setCurrentItem( newTagItem );
			tagTableWidget->openPersistentEditor( newTagItem );
		}
	}

	//activate the clicked item and
	//if you click on an existing tag
	//fill in the old contend
	if( timelineTagVector.size() > 0 ){
		int vpos = -1;
		QTableWidgetItem *newTagItem2 = new QTableWidgetItem();
		newTagItem2->setText( QString( "" ) );
		for ( unsigned int t = 0; t < timelineTagVector.size(); t++ ){
			if( timelineTagVector[t].m_htimelinetagbeat == m_stimelineposition){
				vpos = t;
			}
		}

		if( vpos >-1 ){
			newTagItem2->setText( QString( "%1" ).arg( timelineTagVector[vpos].m_htimelinetag ) );
		}
		tagTableWidget->setItem( m_stimelineposition , 0, newTagItem2 );
		tagTableWidget->setCurrentItem( newTagItem2 );
		tagTableWidget->openPersistentEditor( newTagItem2 );
	}

	//add first tag
	if( timelineTagVector.size() == 0 ){
		QTableWidgetItem *newTagItem3 = new QTableWidgetItem();
		tagTableWidget->setItem( m_stimelineposition , 0, newTagItem3 );
		tagTableWidget->setCurrentItem( newTagItem3 );
		tagTableWidget->openPersistentEditor( newTagItem3 );		
	}

}


void SongEditorPanelTagWidget::on_CancelBtn_clicked()
{
	reject();
}


void SongEditorPanelTagWidget::on_okBtn_clicked()
{
	Hydrogen* engine = Hydrogen::get_instance();
	Timeline* pTimeline = engine->getTimeline();

	int patterngroupvectorsize;
	patterngroupvectorsize = engine->getSong()->get_pattern_group_vector()->size();

	//oldText list contains all old item values. we need them for undo an item
	QStringList oldText;

	if(pTimeline->m_timelinetagvector.size() > 0){
		for (int i = 0; i < patterngroupvectorsize; i++){
			oldText << "";
		}
		for(int i = 0; i < pTimeline->m_timelinetagvector.size(); ++i){
			oldText.replace(pTimeline->m_timelinetagvector[i].m_htimelinetagbeat , pTimeline->m_timelinetagvector[i].m_htimelinetag);
		}
	}

	for( int i = 0; i < __theChangedItems.size() ; i++ )
	{
		int songPosition = __theChangedItems.value( i ).toInt();
		QTableWidgetItem * pNewTagItem = tagTableWidget->item( songPosition, 0 );
		if ( pNewTagItem ) {
			SE_editTagAction *action = new SE_editTagAction(  pNewTagItem->text() ,oldText.value( songPosition ), songPosition );
			HydrogenApp::get_instance()->m_pUndoStack->push( action );
		}
	}
	accept();
}


void SongEditorPanelTagWidget::on_tagTableWidget_currentItemChanged(QTableWidgetItem * current, QTableWidgetItem * previous )
{
	tagTableWidget->closePersistentEditor(previous);
}

}
