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

#include "SampleEditor.h"
#include "../HydrogenApp.h"
#include "InstrumentEditor/InstrumentEditor.h"
#include "../widgets/Button.h"

#include <hydrogen/data_path.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/sample.h>
#include <hydrogen/audio_engine.h>

#include <QModelIndex>
#include <QTreeWidget>
#include <QMessageBox>

using namespace H2Core;
using namespace std;

SampleEditor::SampleEditor ( QWidget* pParent, Sample* Sample )
		: QDialog ( pParent )
		, Object ( "SampleEditor" )
{
	setupUi ( this );
	INFOLOG ( "INIT" );
	setWindowTitle ( trUtf8 ( "SampleEditor" ) );
	setFixedSize ( width(), height() );
	installEventFilter( this );
	m_pSampleEditorStatus = true; //set true if sample changes are save
	m_pSample = Sample;

//get all sample modificationen 
	m_sample_is_modified = m_pSample->get_sample_is_modified();
	m_sample_mode = m_pSample->get_sample_mode();
	m_start_frame = m_pSample->get_start_frame();
	m_loop_frame = m_pSample->get_loop_frame();
	m_repeats = m_pSample->get_repeats();
	m_end_frame = m_pSample->get_end_frame();
	m_fade_out_startframe = m_pSample->get_fade_out_startframe();
	m_fade_out_type = m_pSample->get_fade_out_type();

// mainSampleview = 624 x 265
// mainSampleAdjustView = 180 x 265
// targetSampleView = 451 x 91
// StartFrameSpinBox
// LoopFrameSpinBox
// ProcessingTypeComboBox :forward, reverse, pingpong
// LoopCountSpinBox
// EndFrameSpinBox
// FadeOutFrameSpinBox
// FadeOutTypeComboBox: lin, log
// ApplyChangesPushButton
// PlayPushButton
// RestoreSamplePushButton
// ClosePushButton

}



SampleEditor::~SampleEditor()
{
	INFOLOG ( "DESTROY" );
}



void SampleEditor::on_ClosePushButton_clicked()
{
	if ( !m_pSampleEditorStatus ){
		int err = QMessageBox::information( this, "Hydrogen", tr( "Unsaved changes left. This changes will be lost. \nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
		if ( err == 0 ){
			m_pSampleEditorStatus = true;
			accept();	
		}else
		{
			return;
		}
	}
	accept();
}



void SampleEditor::on_ApplyChangesPushButton_clicked()
{
	setAllSampleProps();	
	m_pSample->sampleEditProzess( m_pSample );
}



bool SampleEditor::getCloseQuestion()
{
	bool close = false;
	int err = QMessageBox::information( this, "Hydrogen", tr( "Close dialog! maybe there is some unsaved work on sample.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
	if ( err == 0 ) close = true;
	return close;
}



void SampleEditor::setSampleName( QString name )
{
	QString newfilename = name.section( '/', -1 );
//	newfilename.replace( "." + newfilename.section( '.', -1 ), "");

	QString windowname = "SampleEditor " + newfilename;
	m_samplename = name;
	setWindowTitle ( windowname );
}



void SampleEditor::setAllSampleProps()
{
	m_pSample->set_sample_is_modified( m_sample_is_modified );
	m_pSample->set_sample_mode( m_sample_mode );
	m_pSample->set_start_frame( m_start_frame );
	m_pSample->set_loop_frame( m_loop_frame );
	m_pSample->set_repeats( m_repeats );
	m_pSample->set_end_frame( m_end_frame );
	m_pSample->set_fade_out_startframe( m_fade_out_startframe );
	m_pSample->set_fade_out_type( m_fade_out_type );
}
