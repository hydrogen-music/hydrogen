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
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "../widgets/Button.h"

#include "MainSampleWaveDisplay.h"
#include "DetailWaveDisplay.h"
#include "TargetWaveDisplay.h"

#include <hydrogen/data_path.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/sample.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/hydrogen.h>

#include <QModelIndex>
#include <QTreeWidget>
#include <QMessageBox>

using namespace H2Core;
using namespace std;

SampleEditor::SampleEditor ( QWidget* pParent, InstrumentLayer * mLayer )
		: QDialog ( pParent )
		, Object ( "SampleEditor" )
		, m_pLayer( mLayer )
		, m_pSampleEditorStatus( true )
		, m_pSample ( mLayer->get_sample() )
		, m_poldSample ( NULL )
		, m_proldSample ( NULL )
{
	setupUi ( this );
	INFOLOG ( "INIT" );
	setWindowTitle ( trUtf8 ( "SampleEditor" ) );
	setFixedSize ( width(), height() );
	installEventFilter( this );

	m_poldSample = Sample::load( m_pSample->get_filename() );//this is 
//get all sample modificationen 
	m_sample_is_modified = m_pSample->get_sample_is_modified();
	m_sample_mode = m_pSample->get_sample_mode();
	m_start_frame = m_pSample->get_start_frame();
	m_loop_frame = m_pSample->get_loop_frame();
	m_repeats = m_pSample->get_repeats();
	if (m_sample_is_modified) {
		m_end_frame = m_pSample->get_end_frame();
	}else
	{
		m_end_frame = m_poldSample->get_n_frames();
	}
		ERRORLOG( QString("endframe: %1").arg(m_end_frame) );
	m_fade_out_startframe = m_pSample->get_fade_out_startframe();
	m_fade_out_type = m_pSample->get_fade_out_type();

	m_ponewayStart = false;
	m_ponewayLoop = false;
	m_ponewayEnd = false;
	m_pslframes = 0;
	unsigned slframes = m_poldSample->get_n_frames();
	m_pzoomfactor = 1;
	m_pdetailframe = 0;
	m_plineColor = "default";


	LoopCountSpinBox->setRange(0, 20000 );
	StartFrameSpinBox->setRange(0, slframes );
	LoopFrameSpinBox->setRange(0, slframes );
	EndFrameSpinBox->setRange(0, slframes );
	if ( !m_pSample->get_sample_is_modified() ){
		EndFrameSpinBox->setValue( slframes ); 
	}else
	{
		EndFrameSpinBox->setValue( m_end_frame );
	}


//	m_pSample->set_end_frame( m_end_frame );

// mainSampleview = 624(575) x 265 :-)
// mainSampleAdjustView = 180 x 265 :-(
// targetSampleView = 451 x 91 :-( will removed
// StartFrameSpinBox :-)
// LoopFrameSpinBox :-)
// ProcessingTypeComboBox :forward, reverse, pingpong :-)
// LoopCountSpinBox :-(
// EndFrameSpinBox :-)
// FadeOutFrameSpinBox :-(
// FadeOutTypeComboBox: lin, log :-(
// ApplyChangesPushButton :-()
// PlayPushButton :-)
// RestoreSamplePushButton :-(
// ClosePushButton :-()
// verticalzoomSlider

	connect( StartFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedStartFrameSpinBox(int) ) );
	connect( LoopFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedLoopFrameSpinBox(int) ) );
	connect( EndFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedEndFrameSpinBox(int) ) );

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateMainsamplePostionRuler()));
}





SampleEditor::~SampleEditor()
{
	delete m_pMainSampleWaveDisplay;
	delete m_pSampleAdjustView;
	delete m_pTargetSampleView;
	delete m_poldSample;
	INFOLOG ( "DESTROY" );
}


void SampleEditor::intDisplays()
{
	H2Core::Instrument *m_pInstrument = NULL;
	Song *pSong = Hydrogen::get_instance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->get_instrument_list();
		int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		if ( nInstr >= (int)pInstrList->get_size() ) {
			nInstr = -1;
		}

		if (nInstr == -1) {
			m_pInstrument = NULL;
		}
		else {
			m_pInstrument = pInstrList->get( nInstr );
			//INFOLOG( "new instr: " + m_pInstrument->m_sName );
		}
	}
/*
	QApplication::setOverrideCursor(Qt::WaitCursor);
// wavedisplays
	m_divider = m_poldSample->get_n_frames() / 574.0F;
	m_pMainSampleWaveDisplay = new MainSampleWaveDisplay( mainSampleview );
	m_pMainSampleWaveDisplay->updateDisplay( m_poldSample->get_filename() );
	m_pMainSampleWaveDisplay->move( 1, 1 );

	m_pSampleAdjustView = new DetailWaveDisplay( mainSampleAdjustView );
	m_pSampleAdjustView->updateDisplay( m_poldSample->get_filename() );
	m_pSampleAdjustView->move( 1, 1 );

	m_pTargetSampleView = new TargetWaveDisplay( targetSampleView );
	m_pTargetSampleView->updateDisplay( mLayer );
	m_pTargetSampleView->move( 1, 1 );
*/

	QApplication::restoreOverrideCursor();

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
//	setAllSampleProps();	
	createNewLayer();
	m_pSampleEditorStatus = true;
	m_pTargetSampleView->updateDisplay( m_pLayer );
	
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


/*
void SampleEditor::getAllSampleProps()
{

	m_pSample->set_sample_is_modified( m_sample_is_modified );
	m_pSample->set_sample_mode( m_sample_mode );
	m_pSample->set_start_frame( m_start_frame );
	m_pSample->set_loop_frame( m_loop_frame );
	m_pSample->set_repeats( m_repeats );
	m_pSample->set_end_frame( m_end_frame );
	ERRORLOG( QString("setAllSampleProps: %1").arg(m_end_frame) );
	m_pSample->set_fade_out_startframe( m_fade_out_startframe );
	m_pSample->set_fade_out_type( m_fade_out_type );

}
*/


void SampleEditor::createNewLayer()
{
	if ( !m_pSampleEditorStatus ){
	
		//create new  sample length
		unsigned onesamplelength =  m_end_frame - m_start_frame;
		unsigned looplength =  m_end_frame - m_loop_frame ;
		unsigned repeatslength = looplength * m_repeats;
		unsigned newlength = 0;
		if (onesamplelength == looplength){	
			newlength = onesamplelength + onesamplelength * m_repeats ;
		}else
		{
			newlength =onesamplelength + repeatslength;
		}
	
		ERRORLOG( QString("startlang: %1").arg(onesamplelength) );
		ERRORLOG( QString("looplang: %1").arg(looplength) );	
		ERRORLOG( QString("newlength: %1").arg(newlength) );


		Sample *newSample = new Sample( newlength, m_poldSample->get_filename() );	

		//create temp data
		float *tempdata_l = new float[ newlength ];
		float *tempdata_r = new float[ newlength ];

		float *looptempdata_l = new float[ looplength ];
		float *looptempdata_r = new float[ looplength ];

		long int z = m_loop_frame;
		long int y = m_start_frame;

	        for ( unsigned i = 0; i < onesamplelength; i++, y++){ //first vector

	                tempdata_l[i] = m_poldSample->__data_l[y];
	                tempdata_r[i] = m_poldSample->__data_r[y];
	        }

		for ( unsigned i = 0; i < looplength; i++, z++){ //loop vector

			looptempdata_l[i] = m_poldSample->__data_l[z];
			looptempdata_r[i] = m_poldSample->__data_r[z];
		}
		
	        for ( int i = 0; i< m_repeats;i++){
	                unsigned tempdataend = onesamplelength + ( looplength * i );
	                copy( looptempdata_l, looptempdata_l+looplength ,tempdata_l+tempdataend );
			copy( looptempdata_r, looptempdata_r+looplength ,tempdata_r+tempdataend );

	        }

		newSample->__data_l = tempdata_l;
		newSample->__data_r = tempdata_r;

		newSample->__sample_rate = m_poldSample->get_sample_rate();

		AudioEngine::get_instance()->lock( "SampeEditor::insert new sample" );
		if (m_pLayer != NULL) {
			// delete old sample
			Sample *proldSample = m_pLayer->get_sample();
			m_pSample = NULL;
			delete proldSample;
			m_pSample = newSample;
//			m_psample = newSample;
//			proldSample = NULL;
			// insert new sample from newInstrument
			m_pLayer->set_sample( newSample );
		}
		else {	
		
			delete[] tempdata_l;
			tempdata_l = NULL;
			delete[] tempdata_r;
			tempdata_r = NULL;
			delete[] looptempdata_l;
			looptempdata_l = NULL;
			delete[] looptempdata_r;
			looptempdata_r = NULL;
			return;
		}
		AudioEngine::get_instance()->unlock();
	
		delete[] tempdata_l;
//		tempdata_l = NULL;
		delete[] tempdata_r;
//		tempdata_r = NULL;
		delete[] looptempdata_l;
//		looptempdata_l = NULL;
		delete[] looptempdata_r;
//		looptempdata_r = NULL;

	}


/*
Sample* Sample::load_wave( const QString& filename )
{
	// file exists?
	if ( QFile( filename ).exists() == false ) {
		_ERRORLOG( QString( "[Sample::load] Load sample: File %1 not found" ).arg( filename ) );
		return NULL;
	}


	SF_INFO soundInfo;
	SNDFILE* file = sf_open( filename.toAscii(), SFM_READ, &soundInfo );
	if ( !file ) {
		_ERRORLOG( QString( "[Sample::load] Error loading file %1" ).arg( filename ) );
	}


	float *pTmpBuffer = new float[ soundInfo.frames * soundInfo.channels ];

	//int res = sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_read_float( file, pTmpBuffer, soundInfo.frames * soundInfo.channels );
	sf_close( file );

	float *data_l = new float[ soundInfo.frames ];
	float *data_r = new float[ soundInfo.frames ];


	if ( soundInfo.channels == 1 ) {	// MONO sample
		for ( long int i = 0; i < soundInfo.frames; i++ ) {
			data_l[i] = pTmpBuffer[i];
			data_r[i] = pTmpBuffer[i];
		}
	} else if ( soundInfo.channels == 2 ) { // STEREO sample
		for ( long int i = 0; i < soundInfo.frames; i++ ) {
			data_l[i] = pTmpBuffer[i * 2];
			data_r[i] = pTmpBuffer[i * 2 + 1];
		}
	}
	delete[] pTmpBuffer;

	Sample *pSample = new Sample( soundInfo.frames, filename );
	pSample->__data_l = data_l;
	pSample->__data_r = data_r;
	pSample->__sample_rate = soundInfo.samplerate;
//	pSample->reverse_sample( pSample ); // test reverse
	return pSample;
}

//simple reverse example 
void Sample::sampleEditProzess( Sample* Sample )
{

	unsigned onesamplelength =  __end_frame - __start_frame;
	unsigned looplength = __end_frame - __loop_frame ;
	unsigned repeatslength = looplength * __repeats;
	unsigned newlength = 0;
	if (onesamplelength == looplength){	
		newlength = onesamplelength + onesamplelength * __repeats ;
	}else
	{
		newlength =onesamplelength + repeatslength;
	}

	ERRORLOG( QString("beginlang: %1").arg(onesamplelength) );
	ERRORLOG( QString("looplang: %1").arg(looplength) );	
	ERRORLOG( QString("newlength: %1").arg(newlength) );
//neuer weg :-) im sampleeditor wird ein komplett neues sample gebaut. 
//das neue sample wird hier zusammengesetz und an den editor zurückgegeben.dort wird es in dem aktuellen layer das alte sample ersetzen.
// das targetsamplewavedispay wird wie gwhabt durch den instrumentlayer das sample laden. so werden änderungen auch hier sichtbar sichtbar	

}
*/


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
	m_ponewayStart = true;	
	m_ponewayLoop = true;
	m_ponewayEnd = true;
}



void SampleEditor::valueChangedStartFrameSpinBox( int )
{
	m_pdetailframe = StartFrameSpinBox->value();
	m_plineColor = "Start";
	if ( !m_ponewayStart ){
		m_pMainSampleWaveDisplay->m_pStartFramePosition = StartFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pdetailframe, m_pzoomfactor , m_plineColor);
		m_start_frame = StartFrameSpinBox->value();
//		m_pMainSampleWaveDisplay->testPositionFromSampleeditor();
				
	}else
	{
		m_pSampleAdjustView->setDetailSamplePosition( m_pdetailframe, m_pzoomfactor , m_plineColor);
		m_ponewayStart = false;
	}
	testPositionsSpinBoxes();
	m_pSampleEditorStatus = false;
	//QMessageBox::information ( this, "Hydrogen", trUtf8 ( "jep %1" ).arg(StartFrameSpinBox->value() / m_divider + 25 ));
}



void SampleEditor::valueChangedLoopFrameSpinBox( int )
{	
	m_pdetailframe = LoopFrameSpinBox->value();
	m_plineColor = "Loop";
	if ( !m_ponewayLoop ){
		m_pMainSampleWaveDisplay->m_pLoopFramePosition = LoopFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pdetailframe, m_pzoomfactor , m_plineColor);
		m_loop_frame = LoopFrameSpinBox->value();
	}else
	{
		m_pSampleAdjustView->setDetailSamplePosition( m_pdetailframe, m_pzoomfactor , m_plineColor);
		m_ponewayLoop = false;
	}
	testPositionsSpinBoxes();
	m_pSampleEditorStatus = false;
}



void SampleEditor::valueChangedEndFrameSpinBox( int )
{
	m_pdetailframe = EndFrameSpinBox->value();
	m_plineColor = "End";
	if ( !m_ponewayEnd ){
		m_pMainSampleWaveDisplay->m_pEndFramePosition = EndFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pdetailframe, m_pzoomfactor , m_plineColor);
		m_end_frame = EndFrameSpinBox->value();
	}else
	{
		m_ponewayEnd = false;
		m_pSampleAdjustView->setDetailSamplePosition( m_pdetailframe, m_pzoomfactor , m_plineColor);
	}
	testPositionsSpinBoxes();
	m_pSampleEditorStatus = false;
}


void SampleEditor::on_PlayPushButton_clicked()
{

	const int selectedlayer = InstrumentEditorPanel::getInstance()->getselectedLayer();
	const float pan_L = 0.5f;
	const float pan_R = 0.5f;
	const int nLength = -1;
	const float fPitch = 0.0f;
	Song *pSong = Hydrogen::get_instance()->getSong();
	
	Instrument *pInstr = pSong->get_instrument_list()->get( Hydrogen::get_instance()->getSelectedInstrumentNumber() );
	
	Note *pNote = new Note( pInstr, 0, pInstr->get_layer( selectedlayer )->get_end_velocity() - 0.01, pan_L, pan_R, nLength, fPitch);
	AudioEngine::get_instance()->get_sampler()->note_on(pNote);

	m_pslframes = m_pSample->get_n_frames();
	m_pMainSampleWaveDisplay->paintLocatorEvent( StartFrameSpinBox->value() / m_divider + 24 , true);
	m_pSampleAdjustView->setDetailSamplePosition( m_start_frame, m_pzoomfactor , 0);
	m_pTimer->start(40);	// update ruler at 25 fps	
	m_prealtimeframeend = Hydrogen::get_instance()->getRealtimeFrames() + m_end_frame - m_start_frame;
	
}

void SampleEditor::updateMainsamplePostionRuler()
{
	unsigned long realpos = Hydrogen::get_instance()->getRealtimeFrames();
	if ( realpos < m_prealtimeframeend ){
		unsigned frame = m_pslframes - ( m_prealtimeframeend  - realpos );
		m_pMainSampleWaveDisplay->paintLocatorEvent( frame / m_divider + 25 , true);
		m_pSampleAdjustView->setDetailSamplePosition( frame, m_pzoomfactor , 0);
//		ERRORLOG( QString("sampleval: %1").arg(frame) );
	}else
	{
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
//		m_pSampleAdjustView->setDetailSamplePosition( 0, m_pzoomfactor , 0);
		m_pTimer->stop();
	}
}


void SampleEditor::on_LoopCountSpinBox_valueChanged( int )
{
	m_repeats = LoopCountSpinBox->value() ;
	m_pSampleEditorStatus = false;
}


void SampleEditor::on_ProcessingTypeComboBox_currentIndexChanged( int )
{
	switch ( ProcessingTypeComboBox->currentIndex() ){
		case 0 :// 
			m_sample_mode = "forward";
			break;
		case 1 :// 
			m_sample_mode = "reverse";
			break;
		case 2 :// 
			m_sample_mode = "pingpong";
			break;
		default:
			m_sample_mode = "forward";
	}
	m_pSampleEditorStatus = false;
}




void SampleEditor::on_verticalzoomSlider_valueChanged( int value )
{
	m_pzoomfactor = value / 10 +1;
	m_pSampleAdjustView->setDetailSamplePosition( m_pdetailframe, m_pzoomfactor, m_plineColor );
}


void SampleEditor::testPositionsSpinBoxes()
{
//m_start_frame;
//m_loop_frame;
//m_end_frame;
	if (  m_start_frame > m_loop_frame ) m_loop_frame = m_start_frame;
	if (  m_start_frame > m_end_frame ) m_end_frame = m_start_frame;
	if (  m_loop_frame > m_end_frame ) m_end_frame = m_loop_frame;
	if (  m_end_frame < m_loop_frame ) m_loop_frame = m_end_frame;
	if (  m_end_frame < m_start_frame ) m_start_frame = m_end_frame;
	StartFrameSpinBox->setValue( m_start_frame );
	LoopFrameSpinBox->setValue( m_loop_frame );
	EndFrameSpinBox->setValue( m_end_frame );
}
