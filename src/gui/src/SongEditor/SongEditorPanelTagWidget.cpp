/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <QtGui>
#include <QtWidgets>

#include "UndoActions.h"
#include "../HydrogenApp.h"
#include "SongEditorPanelTagWidget.h"
#include "SongEditorPanel.h"
#include "SongEditor.h"

#include <core/Hydrogen.h>
#include <core/Timeline.h>

namespace H2Core
{

SongEditorPanelTagWidget::SongEditorPanelTagWidget( QWidget* pParent, int nBeat )
	: QDialog( pParent )
	, m_nTimelinePosition( nBeat )
{
	m_nMaxRows = Preferences::get_instance()->getMaxBars();
	setupUi( this );
	
	setWindowTitle( tr( "Tag" ) );
	createTheTagTableWidget();
}

SongEditorPanelTagWidget::~SongEditorPanelTagWidget() {
}

void SongEditorPanelTagWidget::createTheTagTableWidget()
{
	auto pTimeline = Hydrogen::get_instance()->getTimeline();

	m_oldTags.resize( m_nMaxRows );
	
	for ( int ii = 0; ii < m_nMaxRows; ii++ ) {
		tagTableWidget->insertRow( ii );
		m_oldTags[ ii ] = "";
	}

	auto tagVector = pTimeline->getAllTags();

	//read the tag vector and fill all tags into items
	bool bExistingTagClicked = false;

	for ( const auto& ttag : tagVector ){
		if ( ttag->nColumn < m_nMaxRows ) {
			QTableWidgetItem *newTagItem = new QTableWidgetItem();
			newTagItem->setText( ttag->sTag );
			tagTableWidget->setItem( ttag->nColumn, 0, newTagItem );
			m_oldTags[ ttag->nColumn ] = ttag->sTag;

			if ( ttag->nColumn == m_nTimelinePosition ) {
				bExistingTagClicked = true;
				tagTableWidget->setCurrentItem( newTagItem );
				tagTableWidget->openPersistentEditor( newTagItem );
			}
		}
	}

	if ( ! bExistingTagClicked ) {
		// Open an editor on a blank line
		QTableWidgetItem *newBlankItem = new QTableWidgetItem();
		tagTableWidget->setItem( m_nTimelinePosition , 0, newBlankItem );
		tagTableWidget->setCurrentItem( newBlankItem );
		tagTableWidget->openPersistentEditor( newBlankItem );
	}
}


void SongEditorPanelTagWidget::on_CancelBtn_clicked()
{
	reject();
}


void SongEditorPanelTagWidget::on_okBtn_clicked()
{
	QTableWidgetItem* pCurrentItem = tagTableWidget->currentItem();
	if ( pCurrentItem != nullptr ) {
		tagTableWidget->closePersistentEditor( pCurrentItem );
	}
	
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pTimeline = pHydrogen->getTimeline();
	auto tagVector = pTimeline->getAllTags();

	// First, let's check whether there are any changes.
	struct tagChange {
		QString sNewText;
		QString sOldText;
		int nColumn;
	};
	std::vector<tagChange> changes;

	for ( int ii = 0; ii < m_nMaxRows; ii++ ){
		QTableWidgetItem* pTagItem = tagTableWidget->item( ii, 0 );
		if ( pTagItem != nullptr && m_oldTags[ ii ] != pTagItem->text() ) {
			changes.push_back( { pTagItem->text(), m_oldTags[ ii ], ii } );
		}
	}

	// If there are any changes, ensure they can be undone in a single action.
	auto pHydrogenApp = HydrogenApp::get_instance();
	if ( changes.size() == 1 ) {
		pHydrogenApp->pushUndoCommand( new SE_editTagAction(
			changes[0].sNewText, changes[0].sOldText, changes[0].nColumn ) );
	}
	else if ( changes.size() > 1 ){
		pHydrogenApp->beginUndoMacro( tr( "Edit tags" ) );
		for ( const auto& change : changes ) {
			pHydrogenApp->pushUndoCommand( new SE_editTagAction(
								  change.sNewText, change.sOldText, change.nColumn ) );
		}
		pHydrogenApp->endUndoMacro();
	}
	accept();
}


void SongEditorPanelTagWidget::on_tagTableWidget_currentItemChanged(QTableWidgetItem * current, QTableWidgetItem * previous )
{
	tagTableWidget->closePersistentEditor(previous);
}

}
