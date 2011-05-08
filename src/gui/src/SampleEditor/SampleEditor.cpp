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
#include "../InstrumentEditor/InstrumentEditor.h"
#include "../InstrumentEditor/InstrumentEditorPanel.h"
#include "../widgets/Button.h"

#include "MainSampleWaveDisplay.h"
#include "DetailWaveDisplay.h"
#include "TargetWaveDisplay.h"

#include <hydrogen/data_path.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/hydrogen.h>

#include <QModelIndex>
#include <QTreeWidget>
#include <QMessageBox>
#include <algorithm>

using namespace H2Core;
using namespace std;

const char* SampleEditor::__class_name = "SampleEditor";

SampleEditor::SampleEditor ( QWidget* pParent, int nSelectedLayer, QString mSamplefilename )
		: QDialog ( pParent )
		, Object ( __class_name )
{
	setupUi ( this );
	INFOLOG ( "INIT" );

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateMainsamplePostionRuler()));
	m_pTargetDisplayTimer = new QTimer(this);
	connect(m_pTargetDisplayTimer, SIGNAL(timeout()), this, SLOT(updateTargetsamplePostionRuler()));

	m_pSampleEditorStatus = true;
	m_pSamplefromFile = NULL;
	m_pSelectedLayer = nSelectedLayer;
	m_samplename = mSamplefilename;
	m_pzoomfactor = 1;
	m_pdetailframe = 0;
	m_plineColor = "default";
	m_ponewayStart = false;
	m_ponewayLoop = false;
	m_ponewayEnd = false;
	m_pslframes = 0;
	m_pPositionsRulerPath = NULL;
	m_pPlayButton = false;
        m_pratio = 1.0;
	__rubberband.c_settings = 4;

	QString newfilename = mSamplefilename.section( '/', -1 );

	//init Displays
	m_pMainSampleWaveDisplay = new MainSampleWaveDisplay( mainSampleview );
	m_pSampleAdjustView = new DetailWaveDisplay( mainSampleAdjustView );
	m_pTargetSampleView = new TargetWaveDisplay( targetSampleView );

	setWindowTitle ( QString( "SampleEditor " + newfilename) );
	setFixedSize ( width(), height() );

//this new sample give us the not changed real samplelength 
	m_pSamplefromFile = Sample::load( mSamplefilename );
	if (!m_pSamplefromFile) reject();

	unsigned slframes = m_pSamplefromFile->get_frames();

	LoopCountSpinBox->setRange(0, 20000 );
	StartFrameSpinBox->setRange(0, slframes );
	LoopFrameSpinBox->setRange(0, slframes );
	EndFrameSpinBox->setRange(0, slframes );
	EndFrameSpinBox->setValue( slframes );
	rubberbandCsettingscomboBox->setCurrentIndex( 4 );
	rubberComboBox->setCurrentIndex( 0 );

	__rubberband.use = false;
	__rubberband.divider = 1.0;
	openDisplays();
	getAllFrameInfos();

	if ( QFile( Preferences::get_instance()->m_rubberBandCLIexecutable ).exists() == false ){
		RubberbandCframe->setDisabled ( true );
//pitchdoubleSpinBox
		__rubberband.use = false;
		m_pSampleEditorStatus = true;
	}
        __rubberband.pitch = 0.0;

}



SampleEditor::~SampleEditor()
{
	m_pMainSampleWaveDisplay->close();
	delete m_pMainSampleWaveDisplay;
	m_pMainSampleWaveDisplay = NULL;

	m_pSampleAdjustView->close();
	delete m_pSampleAdjustView;
	m_pSampleAdjustView = NULL;

	m_pTargetSampleView->close();
	delete m_pTargetSampleView;
	m_pTargetSampleView = NULL;

	delete m_pSamplefromFile;
	m_pSamplefromFile = NULL;

	INFOLOG ( "DESTROY" );
}


void SampleEditor::closeEvent(QCloseEvent *event)
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
	}else
	{
		accept();
	}
}


void SampleEditor::getAllFrameInfos()
{
	H2Core::Instrument *pInstrument = NULL;
	Sample* pSample = NULL;
	Song *pSong = Hydrogen::get_instance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->get_instrument_list();
		int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		if ( nInstr >= static_cast<int>(pInstrList->size()) ) {
			nInstr = -1;
		}

		if (nInstr == -1) {
			pInstrument = NULL;
		}
		else {
			pInstrument = pInstrList->get( nInstr );
			//INFOLOG( "new instr: " + pInstrument->m_sName );
		}
	}
	H2Core::InstrumentLayer *pLayer = pInstrument->get_layer( m_pSelectedLayer );
	if ( pLayer ) {
		pSample = pLayer->get_sample();
	}

//this values are needed if we restore a sample from from disk if a new song with sample changes will load 
	m_sample_is_modified = pSample->get_is_modified();
	m_pSamplerate = pSample->get_sample_rate();
    __loops = pSample->get_loops();
    __rubberband = pSample->get_rubberband();

    if ( pSample->get_velocity_envelope()->size()==0 ) {
        m_pTargetSampleView->get_velocity()->clear();
        m_pTargetSampleView->get_velocity()->push_back( Sample::EnvelopePoint(   0, 0 ) );
        m_pTargetSampleView->get_velocity()->push_back( Sample::EnvelopePoint( m_pTargetSampleView->width(), 0 ) );
	} else {
        *m_pTargetSampleView->get_velocity() = *pSample->get_velocity_envelope();
	}

    if ( pSample->get_pan_envelope()->size()==0 ) {
        m_pTargetSampleView->get_pan()->clear();
        m_pTargetSampleView->get_pan()->push_back( Sample::EnvelopePoint(   0, m_pTargetSampleView->height()/2 ) );
        m_pTargetSampleView->get_pan()->push_back( Sample::EnvelopePoint( m_pTargetSampleView->width(), m_pTargetSampleView->height()/2 ) );
	} else {
        *m_pTargetSampleView->get_pan() = *pSample->get_pan_envelope();
	}

	if (m_sample_is_modified) {
		__loops.end_frame = pSample->get_loops().end_frame;
		if ( __loops.mode == Sample::Loops::FORWARD )
			ProcessingTypeComboBox->setCurrentIndex ( 0 );
		if ( __loops.mode == Sample::Loops::REVERSE )
			ProcessingTypeComboBox->setCurrentIndex ( 1 );
		if ( __loops.mode == Sample::Loops::PINGPONG )
			ProcessingTypeComboBox->setCurrentIndex ( 2 );

		StartFrameSpinBox->setValue( __loops.start_frame );
		LoopFrameSpinBox->setValue( __loops.loop_frame );
		EndFrameSpinBox->setValue( __loops.end_frame );
		LoopCountSpinBox->setValue( __loops.count );

		m_pMainSampleWaveDisplay->m_pStartFramePosition = __loops.start_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_pLoopFramePosition =  __loops.loop_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_pEndFramePosition =  __loops.end_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();

		if( !__rubberband.use )rubberComboBox->setCurrentIndex( 0 );
		rubberbandCsettingscomboBox->setCurrentIndex( __rubberband.c_settings );
		if( !__rubberband.use )rubberbandCsettingscomboBox->setCurrentIndex( 4 );
		pitchdoubleSpinBox->setValue( __rubberband.pitch );
		if( !__rubberband.use ) pitchdoubleSpinBox->setValue( 0.0 );

		if( __rubberband.divider == 1.0/64.0) rubberComboBox->setCurrentIndex( 1 );
		else if( __rubberband.divider == 1.0/32.0) rubberComboBox->setCurrentIndex( 2 );
		else if( __rubberband.divider == 1.0/16.0) rubberComboBox->setCurrentIndex( 3 );
		else if( __rubberband.divider == 1.0/8.0) rubberComboBox->setCurrentIndex( 4 );
		else if( __rubberband.divider == 1.0/4.0) rubberComboBox->setCurrentIndex( 5 );
		else if( __rubberband.divider == 1.0/2.0) rubberComboBox->setCurrentIndex( 6 );
		else if( __rubberband.use && ( __rubberband.divider >= 1.0 ) ) rubberComboBox->setCurrentIndex(  (int)(__rubberband.divider + 6) );
		setSamplelengthFrames();
		checkRatioSettings();

	}
	m_pTargetSampleView->updateDisplay( pLayer );

	connect( StartFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedStartFrameSpinBox(int) ) );
	connect( LoopFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedLoopFrameSpinBox(int) ) );
	connect( EndFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedEndFrameSpinBox(int) ) );
	connect( LoopCountSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedLoopCountSpinBox( int ) ) );
	connect( ProcessingTypeComboBox, SIGNAL( currentIndexChanged ( const QString )  ), this, SLOT( valueChangedProcessingTypeComboBox( const QString ) ) );
	connect( rubberComboBox, SIGNAL( currentIndexChanged ( const QString )  ), this, SLOT( valueChangedrubberComboBox( const QString ) ) );
	connect( rubberbandCsettingscomboBox, SIGNAL( currentIndexChanged ( const QString )  ), this, SLOT( valueChangedrubberbandCsettingscomboBox( const QString ) ) );
	connect( pitchdoubleSpinBox, SIGNAL ( valueChanged( double )  ), this, SLOT( valueChangedpitchdoubleSpinBox( double ) ) );
}

void SampleEditor::getAllLocalFrameInfos()
{
	__loops.start_frame = StartFrameSpinBox->value();
	__loops.loop_frame = LoopFrameSpinBox->value();
	__loops.count = LoopCountSpinBox->value();
	__loops.end_frame = EndFrameSpinBox->value();
}



void SampleEditor::openDisplays()
{
	H2Core::Instrument *pInstrument = NULL;
	Song *pSong = Hydrogen::get_instance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->get_instrument_list();
		int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		if ( nInstr >= static_cast<int>(pInstrList->size()) ) {
			nInstr = -1;
		}

		if (nInstr == -1) {
			pInstrument = NULL;
		}
		else {
			pInstrument = pInstrList->get( nInstr );
			//INFOLOG( "new instr: " + pInstrument->m_sName );
		}
	}



// wavedisplays
	m_divider = m_pSamplefromFile->get_frames() / 574.0F;
	m_pMainSampleWaveDisplay->updateDisplay( m_samplename );
	m_pMainSampleWaveDisplay->move( 1, 1 );

	m_pSampleAdjustView->updateDisplay( m_samplename );
	m_pSampleAdjustView->move( 1, 1 );

	m_pTargetSampleView->move( 1, 1 );


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
	}else
	{
		accept();
	}
}



void SampleEditor::on_PrevChangesPushButton_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);	
	getAllLocalFrameInfos();	
	createNewLayer();
	m_pSampleEditorStatus = true;
        QApplication::restoreOverrideCursor();
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

        Sample *editSample = Sample::load( m_samplename, __loops, __rubberband, *m_pTargetSampleView->get_velocity(), *m_pTargetSampleView->get_pan() );

		if( editSample == NULL ){
			return;
		}

		AudioEngine::get_instance()->lock( RIGHT_HERE );

		H2Core::Instrument *pInstrument = NULL;
		Song *pSong = Hydrogen::get_instance()->getSong();
		if (pSong != NULL) {
			InstrumentList *pInstrList = pSong->get_instrument_list();
			int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
			if ( nInstr >= static_cast<int>(pInstrList->size()) ) {
				nInstr = -1;
			}
	
			if (nInstr == -1) {
				pInstrument = NULL;
			}
			else {
				pInstrument = pInstrList->get( nInstr );
			}
		}
	
		H2Core::InstrumentLayer *pLayer = pInstrument->get_layer( m_pSelectedLayer );

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



bool SampleEditor::returnAllMainWaveDisplayValues()
{
	testpTimer();
//	QMessageBox::information ( this, "Hydrogen", trUtf8 ( "jep %1" ).arg(m_pSample->get_frames()));
	m_sample_is_modified = true;
	if( m_pMainSampleWaveDisplay->__startsliderismoved ) __loops.start_frame = m_pMainSampleWaveDisplay->m_pStartFramePosition * m_divider - 25 * m_divider;
	if( m_pMainSampleWaveDisplay->__loopsliderismoved ) __loops.loop_frame = m_pMainSampleWaveDisplay->m_pLoopFramePosition  * m_divider - 25 * m_divider;
	if( m_pMainSampleWaveDisplay->__endsliderismoved ) __loops.end_frame = m_pMainSampleWaveDisplay->m_pEndFramePosition  * m_divider - 25 * m_divider ;
	StartFrameSpinBox->setValue( __loops.start_frame );
	LoopFrameSpinBox->setValue( __loops.loop_frame );
	EndFrameSpinBox->setValue( __loops.end_frame );
	m_ponewayStart = true;	
	m_ponewayLoop = true;
	m_ponewayEnd = true;
	setSamplelengthFrames();

	return true;
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
		__loops.start_frame = StartFrameSpinBox->value();
				
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
		__loops.loop_frame = LoopFrameSpinBox->value();
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
		__loops.end_frame = EndFrameSpinBox->value();
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
	if (PlayPushButton->text() == "Stop" ){
		testpTimer();
		return;
	}
	const int selectedlayer = InstrumentEditorPanel::get_instance()->getselectedLayer();
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
	m_pSampleAdjustView->setDetailSamplePosition( __loops.start_frame, m_pzoomfactor , 0);

	if( __rubberband.use == false ){
		m_pTimer->start(40);	// update ruler at 25 fps
	}


	m_prealtimeframeend = Hydrogen::get_instance()->getRealtimeFrames() + m_pslframes;

	//calculate the new rubberband sample length
	if( __rubberband.use ){
		m_prealtimeframeendfortarget = Hydrogen::get_instance()->getRealtimeFrames() + (m_pslframes * m_pratio + 0.1);
	}else
	{
		m_prealtimeframeendfortarget = m_prealtimeframeend;
	}
	m_pTargetDisplayTimer->start(40);	// update ruler at 25 fps
	PlayPushButton->setText( QString( "Stop") ); 
	
}



void SampleEditor::on_PlayOrigPushButton_clicked()
{
	if (PlayOrigPushButton->text() == "Stop" ){
		testpTimer();
		return;
	}
	Sample *pNewSample = Sample::load( m_samplename );
	if ( pNewSample ){
		int length = ( ( pNewSample->get_frames() / pNewSample->get_sample_rate() + 1) * 100 );
		AudioEngine::get_instance()->get_sampler()->preview_sample( pNewSample, length );
	}

	m_pslframes = pNewSample->get_frames();
	m_pMainSampleWaveDisplay->paintLocatorEvent( StartFrameSpinBox->value() / m_divider + 24 , true);
	m_pSampleAdjustView->setDetailSamplePosition( __loops.start_frame, m_pzoomfactor , 0);
	m_pTimer->start(40);	// update ruler at 25 fps	
	m_prealtimeframeend = Hydrogen::get_instance()->getRealtimeFrames() + m_pslframes;
	PlayOrigPushButton->setText( QString( "Stop") ); 
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
		PlayPushButton->setText( QString("&Play") );
		PlayOrigPushButton->setText( QString( "P&lay original sample") ); 
		m_pPlayButton = false;
	}
}


void SampleEditor::updateTargetsamplePostionRuler()
{
	unsigned long realpos = Hydrogen::get_instance()->getRealtimeFrames();
	unsigned targetsamplelength;
	if( __rubberband.use ){
		targetsamplelength =  m_pslframes * m_pratio + 0.1;
	}else
	{
		targetsamplelength =  m_pslframes;
	}
	if ( realpos < m_prealtimeframeendfortarget ){
		unsigned pos = targetsamplelength - ( m_prealtimeframeendfortarget - realpos );
		m_pTargetSampleView->paintLocatorEventTargetDisplay( (m_pTargetSampleView->width() * pos /targetsamplelength), true);
//		ERRORLOG( QString("sampleval: %1").arg(frame) );
	}else
	{
		m_pTargetSampleView->paintLocatorEventTargetDisplay( -1 , false);
		m_pTargetDisplayTimer->stop();
		PlayPushButton->setText(QString( "&Play") );
		PlayOrigPushButton->setText( QString( "P&lay original sample") ); 
		m_pPlayButton = false;
	}
}



void SampleEditor::createPositionsRulerPath()
{
	setSamplelengthFrames();

	unsigned onesamplelength =  __loops.end_frame - __loops.start_frame;
	unsigned looplength =  __loops.end_frame - __loops.loop_frame;
	unsigned repeatslength = looplength * __loops.count;
	unsigned newlength = 0;
	if (onesamplelength == looplength){	
		newlength = onesamplelength + onesamplelength * __loops.count ;
	}else
	{
		newlength =onesamplelength + repeatslength;
	}

	unsigned  normallength = m_pSamplefromFile->get_frames();

	unsigned *normalframes = new unsigned[ normallength ];


	for ( unsigned i = 0; i < normallength; i++ ) {
		normalframes[i] = i;
	}

	unsigned *tempframes = new unsigned[ newlength ];
	unsigned *loopframes = new unsigned[ looplength ];

    Sample::Loops::LoopMode loopmode = __loops.mode;
	long int z = __loops.loop_frame;
	long int y = __loops.start_frame;

	for ( unsigned i = 0; i < newlength; i++){ //first vector
		tempframes[i] = 0;
	}

	for ( unsigned i = 0; i < onesamplelength; i++, y++){ //first vector

		tempframes[i] = normalframes[y];
	}

	for ( unsigned i = 0; i < looplength; i++, z++){ //loop vector

		loopframes[i] = normalframes[z];
	}
	
	if ( loopmode == Sample::Loops::REVERSE ){
		reverse(loopframes, loopframes + looplength);
	}

	if ( loopmode == Sample::Loops::REVERSE && __loops.count > 0 && __loops.start_frame == __loops.loop_frame ){
		reverse( tempframes, tempframes + onesamplelength );
		}

	if ( loopmode == Sample::Loops::PINGPONG &&  __loops.start_frame == __loops.loop_frame){
		reverse(loopframes, loopframes + looplength);
	}
	
	for ( int i = 0; i< __loops.count ;i++){
		unsigned tempdataend = onesamplelength + ( looplength * i );
		if ( __loops.start_frame == __loops.loop_frame ){
			copy( loopframes, loopframes+looplength ,tempframes+ tempdataend );
		}
		if ( loopmode == Sample::Loops::PINGPONG && __loops.count > 1){
			reverse(loopframes, loopframes + looplength);
		}
		if ( __loops.start_frame != __loops.loop_frame ){
			copy( loopframes, loopframes+looplength ,tempframes+ tempdataend );
		}

	}

	
	if ( __loops.count == 0 && loopmode == Sample::Loops::REVERSE ){
		reverse( tempframes + __loops.loop_frame, tempframes + newlength);
	}

	m_pPositionsRulerPath = tempframes;
}



void SampleEditor::setSamplelengthFrames()
{
	getAllLocalFrameInfos();
	unsigned onesamplelength =  __loops.end_frame - __loops.start_frame;
	unsigned looplength =  __loops.end_frame - __loops.loop_frame ;
	unsigned repeatslength = looplength * __loops.count;
	unsigned newlength = 0;
	if (onesamplelength == looplength){	
		newlength = onesamplelength + onesamplelength * __loops.count ;
	}else
	{
		newlength =onesamplelength + repeatslength;
	}
	m_pslframes = newlength;
	newlengthLabel->setText(QString("new sample length: %1 frames").arg(newlength));
	checkRatioSettings();
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
	__loops.count = LoopCountSpinBox->value() ;
	m_pSampleEditorStatus = false;
	setSamplelengthFrames();
	if ( m_pslframes > Hydrogen::get_instance()->getAudioOutput()->getSampleRate() * 60 * 30){ // >30 min
		LoopCountSpinBox->setMaximum(LoopCountSpinBox->value() -1);	
	}
	
}



void SampleEditor::valueChangedrubberbandCsettingscomboBox( const QString  )
{
	__rubberband.c_settings = rubberbandCsettingscomboBox->currentIndex();
	m_pSampleEditorStatus = false;
}



void SampleEditor::valueChangedpitchdoubleSpinBox( double )
{
	__rubberband.pitch = pitchdoubleSpinBox->value();
	m_pSampleEditorStatus = false;
}


void SampleEditor::valueChangedrubberComboBox( const QString  )
{

	if( rubberComboBox->currentText() != "off" ){
		__rubberband.use = true;
	}else
	{
		__rubberband.use = false;
		__rubberband.divider = 1.0;
	}


	switch ( rubberComboBox->currentIndex() ){
	case 0 :// 
		__rubberband.divider = 4.0;
		break;
	case 1 :// 
		__rubberband.divider = 1.0/64.0;
		break;
	case 2 :// 
		__rubberband.divider = 1.0/32.0;
		break;
	case 3 :// 
		__rubberband.divider = 1.0/16.0;
		break;
	case 4 :// 
		__rubberband.divider = 1.0/8.0;
		break;
	case 5 :// 
		__rubberband.divider = 1.0/4.0;
		break;
	case 6 :// 
		__rubberband.divider = 1.0/2.0;
		break;
	case 7 :// 
		__rubberband.divider = 1.0;
		break;
	default:
		__rubberband.divider = (float)rubberComboBox->currentIndex() - 6.0;
	}
//	QMessageBox::information ( this, "Hydrogen", trUtf8 ( "divider %1" ).arg( __rubberband.divider ));
//	float __rubberband.divider;
	setSamplelengthFrames();


	m_pSampleEditorStatus = false;
}

void SampleEditor::checkRatioSettings()
{
	//my personal ratio quality settings
	//ratios < 0.1 || > 3.0 are bad (red) or experimental sounds
	//ratios > 0.1 - 0.5 || > 2.0 are middle (yellow)
	//ratios < 0.5 || < 2.0 are good (green)

	bool is_green = false;
	//green ratio
	if( (m_pratio >= 0.5) && (m_pratio <= 2.0) ){
		rubberComboBox->setStyleSheet("QComboBox { background-color: green; }");
		is_green = true;
	}
	//yellow ratio
	if( ( (m_pratio > 0.1) || ( m_pratio <=  3.0 ) )&& (!is_green)){
		rubberComboBox->setStyleSheet("QComboBox { background-color: yellow; }");
	}
	//red ratio
	if( ( m_pratio <= 0.1 ) || ( m_pratio > 3.0 ) && (!is_green) ){
		rubberComboBox->setStyleSheet("QComboBox { background-color: red; }");
	}
	QString text = QString( " RB-Ratio = %1").arg(m_pratio);
	ratiolabel->setText( text );

	//no rubberband = default
	if( !__rubberband.use ){
		rubberComboBox->setStyleSheet("QComboBox { background-color: 58, 62, 72; }");
		ratiolabel->setText( "" );
	}
}


void SampleEditor::valueChangedProcessingTypeComboBox( const QString unused )
{
	switch ( ProcessingTypeComboBox->currentIndex() ){
		case 0 :// 
			__loops.mode = Sample::Loops::FORWARD;
			break;
		case 1 :// 
			__loops.mode = Sample::Loops::REVERSE;
			break;
		case 2 :// 
			__loops.mode = Sample::Loops::PINGPONG;
			break;
		default:
			__loops.mode = Sample::Loops::FORWARD;
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
	if (  __loops.start_frame > __loops.loop_frame ) __loops.loop_frame = __loops.start_frame;
	if (  __loops.start_frame > __loops.end_frame ) __loops.end_frame = __loops.start_frame;
	if (  __loops.loop_frame > __loops.end_frame ) __loops.end_frame = __loops.loop_frame;
	if (  __loops.end_frame < __loops.loop_frame ) __loops.loop_frame = __loops.end_frame;
	if (  __loops.end_frame < __loops.start_frame ) __loops.start_frame = __loops.end_frame;
	StartFrameSpinBox->setValue( __loops.start_frame );
	LoopFrameSpinBox->setValue( __loops.loop_frame );
	EndFrameSpinBox->setValue( __loops.end_frame );
}



void SampleEditor::testpTimer()
{
	if ( m_pTimer->isActive() || m_pTargetDisplayTimer->isActive() ){
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		m_pTargetDisplayTimer->stop();
		PlayPushButton->setText( QString( "&Play" ) );
		PlayOrigPushButton->setText( QString( "P&lay original sample") ); 
		AudioEngine::get_instance()->get_sampler()->stop_playing_notes();
		m_pPlayButton = false;
	}
}
