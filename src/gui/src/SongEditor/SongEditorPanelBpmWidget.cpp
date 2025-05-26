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

#include <QtGui>
#include <QtWidgets>

#include "UndoActions.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "SongEditorPanelBpmWidget.h"
#include "SongEditorPanel.h"
#include "SongEditor.h"
#include "../Widgets/Button.h"

#include <core/Hydrogen.h>
#include <core/Timeline.h>

namespace H2Core
{

SongEditorPanelBpmWidget::SongEditorPanelBpmWidget( QWidget* pParent, int nColumn, bool bTempoMarkerPresent )
	: QDialog( pParent )
	, m_nColumn( nColumn )
	, m_bTempoMarkerPresent( bTempoMarkerPresent )
{
	setupUi( this );
	setWindowTitle( bTempoMarkerPresent ? tr( "Edit Tempo Marker" ) :
					tr( "Create New Tempo Marker" ) );
	adjustSize();
	setFixedSize( width(), height() );

	auto pHydrogen = Hydrogen::get_instance();

	bpmSpinBox->setType( LCDSpinBox::Type::Double );
	bpmSpinBox->setMinimum( MIN_BPM );
	bpmSpinBox->setMaximum( MAX_BPM );
	bpmSpinBox->setValue( pHydrogen->getTimeline()->getTempoAtColumn( m_nColumn ) );
	bpmSpinBox->setToolTip( bTempoMarkerPresent ?
								tr( "Alter tempo of selected tempo marker" ) :
								tr( "Set tempo of new tempo marker" ) );
	// Required for correct focus highlighting.
	bpmSpinBox->setSize( QSize( 146, 23 ) );
	bpmSpinBox->setFocus();

	columnSpinBox->setType( LCDSpinBox::Type::Int );
	columnSpinBox->setMinimum( 1 );
	columnSpinBox->setMaximum( Preferences::get_instance()->getMaxBars() );
	columnSpinBox->setValue( m_nColumn + 1 );
	columnSpinBox->setToolTip( bTempoMarkerPresent ?
								tr( "Move tempo marker to different column" ) :
								tr( "Set column of new tempo marker" ) );
	// Required for correct focus highlighting.
	columnSpinBox->setSize( QSize( 146, 23 ) );

	deleteBtn->setSize( QSize( 180, 23 ) );
	deleteBtn->setIsActive( bTempoMarkerPresent );
	deleteBtn->setFixedFontSize( 12 );
}




SongEditorPanelBpmWidget::~SongEditorPanelBpmWidget()
{
}



void SongEditorPanelBpmWidget::on_CancelBtn_clicked()
{
	reject();
}



void SongEditorPanelBpmWidget::on_okBtn_clicked()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	float fNewBpm = bpmSpinBox->text().toFloat( nullptr );

	// In case the input text can not be parsed by Qt `fNewBpm' is 0
	// and also covered by the warning below.
	if ( fNewBpm > MAX_BPM || fNewBpm < MIN_BPM ){
		QMessageBox::warning(
			this, "Hydrogen",
			QString( tr( "Please enter a number within the range of " )
					 .append( QString( "[%1,%2]" ).arg( MIN_BPM )
							  .arg( MAX_BPM ) ) ),
			QMessageBox::Cancel, QMessageBox::Cancel );
		return;
	}

	auto pTimeline = Hydrogen::get_instance()->getTimeline();
	int nNewColumn = columnSpinBox->text().toInt() - 1;
	if ( ! ( m_bTempoMarkerPresent && nNewColumn == m_nColumn ) &&
		 pTimeline->hasColumnTempoMarker( nNewColumn ) ) {
		QMessageBox::warning(
			this, "Hydrogen",
			QString( tr( "There is already a tempo marker present at this Column. Please use left-click to edit it instead." ) ),
			QMessageBox::Cancel, QMessageBox::Cancel );
		return;
	}
	
	float fOldBpm = pTimeline->getTempoAtColumn( m_nColumn );

	SE_editTimelineAction *action = new SE_editTimelineAction( m_nColumn, nNewColumn, fOldBpm, bpmSpinBox->value(), m_bTempoMarkerPresent );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
	accept();
}


void SongEditorPanelBpmWidget::on_deleteBtn_clicked()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pTimeline = pHydrogen->getTimeline();

	float fBpm = pTimeline->getTempoAtColumn( m_nColumn );

	SE_deleteTimelineAction *action = new SE_deleteTimelineAction( m_nColumn, fBpm );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
	accept();
}

}
