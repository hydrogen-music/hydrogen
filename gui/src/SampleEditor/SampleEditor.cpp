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
#include <algorithm>

using namespace H2Core;
using namespace std;

SampleEditor::SampleEditor ( QWidget* pParent, int nSelectedLayer, QString mSamplefilename )
		: QDialog ( pParent )
		, Object ( "SampleEditor" )
		, m_pSampleEditorStatus( true )
		, m_pSamplefromFile ( NULL )
		, m_pSelectedLayer ( nSelectedLayer )
		, m_samplename ( mSamplefilename )
		, m_pzoomfactor ( 1 )
		, m_pdetailframe ( 0 )
		, m_plineColor ( "default" )
		, m_ponewayStart ( false )
		, m_ponewayLoop ( false )
		, m_ponewayEnd ( false )
		, m_pslframes ( 0 )
		, m_pPositionsRulerPath ( NULL )
		, m_pPlayButton ( false )
{
	setupUi ( this );
	INFOLOG ( "INIT" );

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateMainsamplePostionRuler()));

	QString newfilename = mSamplefilename.section( '/', -1 );

	//init Displays
	m_pMainSampleWaveDisplay = new MainSampleWaveDisplay( mainSampleview );
	m_pSampleAdjustView = new DetailWaveDisplay( mainSampleAdjustView );
	m_pTargetSampleView = new TargetWaveDisplay( targetSampleView );

	setWindowTitle ( QString( "SampleEditor " + newfilename) );
	setFixedSize ( width(), height() );
	installEventFilter( this );

//this new sample give us the not changed real samplelength 
	m_pSamplefromFile = Sample::load( mSamplefilename );
	if (!m_pSamplefromFile) reject();

	unsigned slframes = m_pSamplefromFile->get_n_frames();

	LoopCountSpinBox->setRange(0, 20000 );
	StartFrameSpinBox->setRange(0, slframes );
	LoopFrameSpinBox->setRange(0, slframes );
	EndFrameSpinBox->setRange(0, slframes );
	EndFrameSpinBox->setValue( slframes );

	openDisplays();
	getAllFrameInfos();

}



SampleEditor::~SampleEditor()
{
	delete m_pMainSampleWaveDisplay;
	delete m_pSampleAdjustView;
	delete m_pTargetSampleView;
	delete m_pSamplefromFile;
	INFOLOG ( "DESTROY" );
}


void SampleEditor::closeEvent(QCloseEvent *event)
{
	if ( !m_pSampleEditorStatus ){
		int err = QMessageBox::information( this, "Hydrogen", tr( "Unsaved changes left. This changes will be lost. \nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
		if ( err == 0 ){
			m_pSampleEditorStatus = true;
			HydrogenApp::getInstance()->closeSampleEditor();;	
		}else
		{
			return;
		}
	}else
	{
		HydrogenApp::getInstance()->closeSampleEditor();
	}
}


void SampleEditor::getAllFrameInfos()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	H2Core::Instrument *m_pInstrument = NULL;
	Sample* pSample = NULL;
	Song *pSong = Hydrogen::get_instance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->get_instrument_list();
		int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		if ( nInstr >= static_cast<int>(pInstrList->get_size()) ) {
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
	H2Core::InstrumentLayer *pLayer = m_pInstrument->get_layer( m_pSelectedLayer );
	if ( pLayer ) {
		pSample = pLayer->get_sample();
	}

//this values are needed if we restore a sample from from disk if a new song with sample changes will load 
	m_sample_is_modified = pSample->get_sample_is_modified();
	m_sample_mode = pSample->get_sample_mode();
	m_start_frame = pSample->get_start_frame();
	m_loop_frame = pSample->get_loop_frame();
	m_repeats = pSample->get_repeats();
	m_end_frame = m_pSamplefromFile->get_end_frame();

	Hydrogen::HVeloVector velovector;
	//velovector
	if ( pSample->__velo_pan.m_Samplevolumen[0].m_SampleVeloframe == -1 ){
		pEngine->m_volumen.clear();
		velovector.m_hxframe = 0;
		velovector.m_hyvalue = 0;
		pEngine->m_volumen.push_back( velovector );
		velovector.m_hxframe = 841;
		velovector.m_hyvalue = 0;
		pEngine->m_volumen.push_back( velovector );
	}else
	{
		pEngine->m_volumen.clear();
		for( int i = 0 ; i < static_cast<int>(pSample->__velo_pan.m_Samplevolumen.size()); i++){
			velovector.m_hxframe = pSample->__velo_pan.m_Samplevolumen[i].m_SampleVeloframe;
			velovector.m_hyvalue = pSample->__velo_pan.m_Samplevolumen[i].m_SampleVelovalue;
			pEngine->m_volumen.push_back( velovector );	
		}
	}

	Hydrogen::HPanVector panvector;
	if ( pSample->__velo_pan.m_SamplePan[0].m_SamplePanframe == -1 ){
		pEngine->m_pan.clear();
		panvector.m_hxframe = 0;
		panvector.m_hyvalue = 45;
		pEngine->m_pan.push_back( panvector );
		panvector.m_hxframe = 841;
		panvector.m_hyvalue = 45;
		pEngine->m_pan.push_back( panvector );
	}else
	{
		pEngine->m_pan.clear();
		for( int i = 0 ; i < static_cast<int>(pSample->__velo_pan.m_SamplePan.size()); i++){
			panvector.m_hxframe = pSample->__velo_pan.m_SamplePan[i].m_SamplePanframe;
			panvector.m_hyvalue = pSample->__velo_pan.m_SamplePan[i].m_SamplePanvalue;
			pEngine->m_pan.push_back( panvector );
		}
	}

	if (m_sample_is_modified) {
		m_end_frame = pSample->get_end_frame();

		if ( m_sample_mode == "forward" ) 
			ProcessingTypeComboBox->setCurrentIndex ( 0 );
		if ( m_sample_mode == "reverse" ) 
			ProcessingTypeComboBox->setCurrentIndex ( 1 );
		if ( m_sample_mode == "pingpong" ) 
			ProcessingTypeComboBox->setCurrentIndex ( 2 );

		StartFrameSpinBox->setValue( m_start_frame );
		LoopFrameSpinBox->setValue( m_loop_frame );
		EndFrameSpinBox->setValue( m_end_frame );
		LoopCountSpinBox->setValue( m_repeats );

		m_pMainSampleWaveDisplay->m_pStartFramePosition = m_start_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_pLoopFramePosition =  m_loop_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_pEndFramePosition =  m_end_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();

	}
	m_pTargetSampleView->updateDisplay( pLayer );

	connect( StartFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedStartFrameSpinBox(int) ) );
	connect( LoopFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedLoopFrameSpinBox(int) ) );
	connect( EndFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedEndFrameSpinBox(int) ) );
	connect( LoopCountSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedLoopCountSpinBox( int ) ) );
	connect( ProcessingTypeComboBox, SIGNAL( currentIndexChanged ( const QString )  ), this, SLOT( valueChangedProcessingTypeComboBox( const QString ) ) );
}


void SampleEditor::getAllLocalFrameInfos()
{
	m_start_frame = StartFrameSpinBox->value();
	m_loop_frame = LoopFrameSpinBox->value();
	m_repeats = LoopCountSpinBox->value();
	m_end_frame = EndFrameSpinBox->value();
}



void SampleEditor::openDisplays()
{
	H2Core::Instrument *m_pInstrument = NULL;
	Song *pSong = Hydrogen::get_instance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->get_instrument_list();
		int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		if ( nInstr >= static_cast<int>(pInstrList->get_size()) ) {
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


	QApplication::setOverrideCursor(Qt::WaitCursor);

// wavedisplays
//	AudioEngine::get_instance()->get_sampler()->stop_playing_notes();
	m_divider = m_pSamplefromFile->get_n_frames() / 574.0F;
//	m_pMainSampleWaveDisplay = new MainSampleWaveDisplay( mainSampleview );
	m_pMainSampleWaveDisplay->updateDisplay( m_samplename );
	m_pMainSampleWaveDisplay->move( 1, 1 );

//	m_pSampleAdjustView = new DetailWaveDisplay( mainSampleAdjustView );
	m_pSampleAdjustView->updateDisplay( m_samplename );
	m_pSampleAdjustView->move( 1, 1 );

//	m_pTargetSampleView = new TargetWaveDisplay( targetSampleView );
//	m_pTargetSampleView->updateDisplay( pLayer );
	m_pTargetSampleView->move( 1, 1 );

	QApplication::restoreOverrideCursor();

}



void SampleEditor::on_ClosePushButton_clicked()
{
	if ( !m_pSampleEditorStatus ){
		int err = QMessageBox::information( this, "Hydrogen", tr( "Unsaved changes left. This changes will be lost. \nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
		if ( err == 0 ){
			m_pSampleEditorStatus = true;
			HydrogenApp::getInstance()->closeSampleEditor();	
		}else
		{
			return;
		}
	}else
	{
		HydrogenApp::getInstance()->closeSampleEditor();
//		accept();
	}
}



void SampleEditor::on_PrevChangesPushButton_clicked()
{
	getAllLocalFrameInfos();	
	createNewLayer();
	m_pSampleEditorStatus = true;
	
}



bool SampleEditor::getCloseQuestion()
{
	bool close = false;
	int err = QMessageBox::information( this, "Hydrogen", tr( "Close dialog! maybe there is some unsaved work on sample.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
	if ( err == 0 ) close = true;
	
	return close;
}



void SampleEditor::createNewLayer()
{

	if ( !m_pSampleEditorStatus ){

		Sample *editSample = Sample::load_edit_wave( m_samplename,
							    m_start_frame,
							    m_loop_frame,
							    m_end_frame,
							    m_repeats,
							    m_sample_mode);

		AudioEngine::get_instance()->lock( "SampeEditor::insert new sample" );

		H2Core::Instrument *m_pInstrument = NULL;
		Song *pSong = Hydrogen::get_instance()->getSong();
		if (pSong != NULL) {
			InstrumentList *pInstrList = pSong->get_instrument_list();
			int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
			if ( nInstr >= static_cast<int>(pInstrList->get_size()) ) {
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
	
		H2Core::InstrumentLayer *pLayer = m_pInstrument->get_layer( m_pSelectedLayer );

		Sample *oldSample = pLayer->get_sample();
		delete oldSample;
	
		// insert new sample from newInstrument
		pLayer->set_sample( editSample );

		AudioEngine::get_instance()->unlock();
		m_pTargetSampleView->updateDisplay( pLayer );
		}
		
}



void SampleEditor::mouseReleaseEvent(QMouseEvent *ev)
{

}



void SampleEditor::returnAllMainWaveDisplayValues()
{
	testpTimer();
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
	setSamplelengthFrames();
}


void SampleEditor::returnAllTargetDisplayValues()
{
	setSamplelengthFrames();
	m_sample_is_modified = true;

}



void SampleEditor::setTrue()
{
	m_pSampleEditorStatus = false;
}



void SampleEditor::valueChangedStartFrameSpinBox( int )
{
	testpTimer();
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
	setSamplelengthFrames();
}



void SampleEditor::valueChangedLoopFrameSpinBox( int )
{
	testpTimer();	
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
	setSamplelengthFrames();
}



void SampleEditor::valueChangedEndFrameSpinBox( int )
{
	testpTimer();
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
	setSamplelengthFrames();
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

	setSamplelengthFrames();
	createPositionsRulerPath();
	m_pPlayButton = true;
	m_pMainSampleWaveDisplay->paintLocatorEvent( StartFrameSpinBox->value() / m_divider + 24 , true);
	m_pSampleAdjustView->setDetailSamplePosition( m_start_frame, m_pzoomfactor , 0);
	m_pTimer->start(40);	// update ruler at 25 fps	
	m_prealtimeframeend = Hydrogen::get_instance()->getRealtimeFrames() + m_pslframes;
	
}



void SampleEditor::on_PlayOrigPushButton_clicked()
{
	Sample *pNewSample = Sample::load( m_samplename );
	if ( pNewSample ){
		int length = ( ( pNewSample->get_n_frames() / pNewSample->get_sample_rate() + 1) * 100 );
		AudioEngine::get_instance()->get_sampler()->preview_sample( pNewSample, length );
	}

	m_pslframes = pNewSample->get_n_frames();
	m_pMainSampleWaveDisplay->paintLocatorEvent( StartFrameSpinBox->value() / m_divider + 24 , true);
	m_pSampleAdjustView->setDetailSamplePosition( m_start_frame, m_pzoomfactor , 0);
	m_pTimer->start(40);	// update ruler at 25 fps	
	m_prealtimeframeend = Hydrogen::get_instance()->getRealtimeFrames() + m_pslframes;
}



void SampleEditor::updateMainsamplePostionRuler()
{
	unsigned long realpos = Hydrogen::get_instance()->getRealtimeFrames();
	if ( realpos < m_prealtimeframeend ){
		unsigned frame = m_pslframes - ( m_prealtimeframeend  - realpos );
		if ( m_pPlayButton == true ){
			m_pMainSampleWaveDisplay->paintLocatorEvent( m_pPositionsRulerPath[frame] / m_divider + 25 , true);
			m_pSampleAdjustView->setDetailSamplePosition( m_pPositionsRulerPath[frame], m_pzoomfactor , 0);
		}else{
			m_pMainSampleWaveDisplay->paintLocatorEvent( frame / m_divider + 25 , true);
			m_pSampleAdjustView->setDetailSamplePosition( frame, m_pzoomfactor , 0);
		}
//		ERRORLOG( QString("sampleval: %1").arg(frame) );
	}else
	{
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		m_pPlayButton = false;
	}
}



void SampleEditor::createPositionsRulerPath()
{
	setSamplelengthFrames();

	unsigned onesamplelength =  m_end_frame - m_start_frame;
	unsigned looplength =  m_end_frame - m_loop_frame;
	unsigned repeatslength = looplength * m_repeats;
	unsigned newlength = 0;
	if (onesamplelength == looplength){	
		newlength = onesamplelength + onesamplelength * m_repeats ;
	}else
	{
		newlength =onesamplelength + repeatslength;
	}

	unsigned  normallength = m_pSamplefromFile->get_n_frames();

	unsigned *normalframes = new unsigned[ normallength ];


	for ( unsigned i = 0; i < normallength; i++ ) {
		normalframes[i] = i;
	}

	unsigned *tempframes = new unsigned[ newlength ];
	unsigned *loopframes = new unsigned[ looplength ];

	QString loopmode = m_sample_mode;
	long int z = m_loop_frame;
	long int y = m_start_frame;

	for ( unsigned i = 0; i < newlength; i++){ //first vector
		tempframes[i] = 0;
	}

	for ( unsigned i = 0; i < onesamplelength; i++, y++){ //first vector

		tempframes[i] = normalframes[y];
	}

	for ( unsigned i = 0; i < looplength; i++, z++){ //loop vector

		loopframes[i] = normalframes[z];
	}
	
	if ( loopmode == "reverse" ){
		reverse(loopframes, loopframes + looplength);
	}

	if ( loopmode == "reverse" && m_repeats > 0 && m_start_frame == m_loop_frame ){
		reverse( tempframes, tempframes + onesamplelength );		
		}

	if ( loopmode == "pingpong" &&  m_start_frame == m_loop_frame){
		reverse(loopframes, loopframes + looplength);
	}
	
	for ( int i = 0; i< m_repeats ;i++){			
		unsigned tempdataend = onesamplelength + ( looplength * i );
		copy( loopframes, loopframes+looplength ,tempframes+ tempdataend );
		if ( loopmode == "pingpong" && m_repeats > 1){
			reverse(loopframes, loopframes + looplength);
		}

	}

	
	if ( m_repeats == 0 && loopmode == "reverse" ){
		reverse( tempframes + m_loop_frame, tempframes + newlength);		
		}

	m_pPositionsRulerPath = tempframes;
}



void SampleEditor::setSamplelengthFrames()
{
	getAllLocalFrameInfos();
//	getAllFrameInfos();

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
	m_pslframes = newlength;
	newlengthLabel->setText(QString("new sample length: %1 frames").arg(newlength));
}



void SampleEditor::valueChangedLoopCountSpinBox( int )
{
	testpTimer();
	if ( m_pslframes > Hydrogen::get_instance()->getAudioOutput()->getSampleRate() * 60 ){
		AudioEngine::get_instance()->get_sampler()->stop_playing_notes();
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		m_pPlayButton = false;
	}
	m_repeats = LoopCountSpinBox->value() ;
	m_pSampleEditorStatus = false;
	setSamplelengthFrames();
	if ( m_pslframes > Hydrogen::get_instance()->getAudioOutput()->getSampleRate() * 60 * 30){ // >30 min
		LoopCountSpinBox->setMaximum(LoopCountSpinBox->value() -1);	
	}
	
}



void SampleEditor::valueChangedProcessingTypeComboBox( const QString unused )
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
	if (  m_start_frame > m_loop_frame ) m_loop_frame = m_start_frame;
	if (  m_start_frame > m_end_frame ) m_end_frame = m_start_frame;
	if (  m_loop_frame > m_end_frame ) m_end_frame = m_loop_frame;
	if (  m_end_frame < m_loop_frame ) m_loop_frame = m_end_frame;
	if (  m_end_frame < m_start_frame ) m_start_frame = m_end_frame;
	StartFrameSpinBox->setValue( m_start_frame );
	LoopFrameSpinBox->setValue( m_loop_frame );
	EndFrameSpinBox->setValue( m_end_frame );
}



void SampleEditor::testpTimer()
{
	if ( m_pTimer->isActive() ){
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		AudioEngine::get_instance()->get_sampler()->stop_playing_notes();
		m_pPlayButton = false;
	}
}
