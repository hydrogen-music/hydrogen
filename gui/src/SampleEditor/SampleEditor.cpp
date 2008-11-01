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

#include "MainSampleWaveDisplay.h"
#include "DetailWaveDisplay.h"
#include "TargetWaveDisplay.h"

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

	QApplication::setOverrideCursor(Qt::WaitCursor);
// wavedisplays
	m_divider = m_pSample->get_n_frames() / 574.0F;
	m_pMainSampleWaveDisplay = new MainSampleWaveDisplay( mainSampleview );
	m_pMainSampleWaveDisplay->updateDisplay( Sample->get_filename() );
	m_pMainSampleWaveDisplay->move( 1, 3 );

//	m_pTargetSampleView = new TargetWaveDisplay( targetSampleView );
//	m_pTargetSampleView->updateDisplay( Sample->get_filename() );
//	m_pTargetSampleView->move( 1, 1 );

//	m_pSampleAdjustView = new DetailWaveDisplay( mainSampleAdjustView );
//	m_pSampleAdjustView->updateDisplay( Sample->get_filename() );
//	m_pSampleAdjustView->move( 1, 1 );

	QApplication::restoreOverrideCursor();

	unsigned slframes = m_pSample->get_n_frames();
	StartFrameSpinBox->setMaximum( slframes );
	LoopCountSpinBox->setMaximum( slframes );
	EndFrameSpinBox->setMaximum( slframes );
	if ( !m_pSample->get_sample_is_modified() ){
		EndFrameSpinBox->setValue( slframes ); 
	}else
	{
		EndFrameSpinBox->setValue( m_end_frame );
	}

// mainSampleview = 624(575) x 265 
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
	connect( StartFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedStartFrameSpinBox(int) ) );
	connect( LoopFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedLoopFrameSpinBox(int) ) );
	connect( EndFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedEndFrameSpinBox(int) ) );
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
	m_pSampleEditorStatus = true;
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
	if ( !m_pSampleEditorStatus ){
		m_pSample->set_sample_is_modified( m_sample_is_modified );
		m_pSample->set_sample_mode( m_sample_mode );
		m_pSample->set_start_frame( m_start_frame );
		m_pSample->set_loop_frame( m_loop_frame );
		m_pSample->set_repeats( m_repeats );
		m_pSample->set_end_frame( m_end_frame );
		m_pSample->set_fade_out_startframe( m_fade_out_startframe );
		m_pSample->set_fade_out_type( m_fade_out_type );
	}
}


void SampleEditor::mouseReleaseEvent(QMouseEvent *ev)
{

}

void SampleEditor::returnAllMainWaveDisplayValues()
{
//	QMessageBox::information ( this, "Hydrogen", trUtf8 ( "jep %1" ).arg(m_pSample->get_n_frames()));
	m_sample_is_modified = true;
	m_start_frame = m_pMainSampleWaveDisplay->m_pStartFramePosition * m_divider - 25 * m_divider;
	m_loop_frame = m_pMainSampleWaveDisplay->m_pLoopFramePosition  * m_divider - 25 * m_divider;
	m_end_frame = m_pMainSampleWaveDisplay->m_pEndFramePosition  * m_divider - 25 * m_divider ;

	StartFrameSpinBox->setValue( m_start_frame );
	LoopFrameSpinBox->setValue( m_loop_frame );
	EndFrameSpinBox->setValue( m_end_frame );	
}


void SampleEditor::valueChangedStartFrameSpinBox( int )
{

	m_pMainSampleWaveDisplay->m_pStartFramePosition = StartFrameSpinBox->value() / m_divider + 25 ;
	m_pMainSampleWaveDisplay->updateDisplayPointer();
	//QMessageBox::information ( this, "Hydrogen", trUtf8 ( "jep %1" ).arg(StartFrameSpinBox->value() / m_divider + 25 ));
}



void SampleEditor::valueChangedLoopFrameSpinBox( int )
{

	m_pMainSampleWaveDisplay->m_pLoopFramePosition = LoopFrameSpinBox->value() / m_divider + 25 ;
	m_pMainSampleWaveDisplay->updateDisplayPointer();
}



void SampleEditor::valueChangedEndFrameSpinBox( int )
{

	m_pMainSampleWaveDisplay->m_pEndFramePosition = EndFrameSpinBox->value() / m_divider + 25 ;
	m_pMainSampleWaveDisplay->updateDisplayPointer();
}
