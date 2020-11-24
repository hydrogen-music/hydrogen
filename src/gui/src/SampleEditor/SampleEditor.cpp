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
#include "../Widgets/Button.h"

#include "MainSampleWaveDisplay.h"
#include "DetailWaveDisplay.h"
#include "TargetWaveDisplay.h"

#include <core/H2Exception.h>
#include <core/Preferences.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Note.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Helpers/Filesystem.h>
#include <core/AudioEngine.h>
#include <core/Hydrogen.h>

#include <QModelIndex>
#include <QTreeWidget>
#include <QMessageBox>
#include <algorithm>
#include <memory>

using namespace H2Core;

const char* SampleEditor::__class_name = "SampleEditor";

SampleEditor::SampleEditor ( QWidget* pParent, int nSelectedComponent, int nSelectedLayer, QString sSampleFilename )
		: QDialog ( pParent )
		, Object ( __class_name )
{
	setupUi ( this );
	INFOLOG ( "INIT" );

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateMainsamplePositionRuler()));
	m_pTargetDisplayTimer = new QTimer(this);
	connect(m_pTargetDisplayTimer, SIGNAL(timeout()), this, SLOT(updateTargetsamplePositionRuler()));

	m_bSampleEditorStatus = true;
	m_nSelectedLayer = nSelectedLayer;
	m_nSelectedComponent = nSelectedComponent;
	m_sSampleName = sSampleFilename;
	m_fZoomfactor = 1;
	m_pDetailFrame = 0;
	m_pLineColor = "default";
	m_bOnewayStart = false;
	m_bOnewayLoop = false;
	m_bOnewayEnd = false;
	m_nSlframes = 0;
	m_pPositionsRulerPath = nullptr;
	m_bPlayButton = false;
	m_fRatio = 1.0f;
	__rubberband.c_settings = 4;

	QString newfilename = sSampleFilename.section( '/', -1 );

	//init Displays
	m_pMainSampleWaveDisplay = new MainSampleWaveDisplay( mainSampleview );
	m_pSampleAdjustView = new DetailWaveDisplay( mainSampleAdjustView );
	m_pTargetSampleView = new TargetWaveDisplay( targetSampleView );

	setWindowTitle ( QString( "SampleEditor " + newfilename) );
	setFixedSize ( width(), height() );
	setModal ( true );

	//this new sample give us the not changed real samplelength
	m_pSampleFromFile = Sample::load( sSampleFilename );
	if ( m_pSampleFromFile == nullptr ) {
		reject();
	}

	unsigned slframes = m_pSampleFromFile->get_frames();

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

#ifndef H2CORE_HAVE_RUBBERBAND
	if ( !Filesystem::file_executable( Preferences::get_instance()->m_rubberBandCLIexecutable , true /* silent */) ) {
		RubberbandCframe->setDisabled ( true );
		__rubberband.use = false;
		m_bSampleEditorStatus = true;
	}
#else
	RubberbandCframe->setDisabled ( false );
	m_bSampleEditorStatus = true;
#endif

	__rubberband.pitch = 0.0;

}



SampleEditor::~SampleEditor()
{
	m_pMainSampleWaveDisplay->close();
	delete m_pMainSampleWaveDisplay;
	m_pMainSampleWaveDisplay = nullptr;

	m_pSampleAdjustView->close();
	delete m_pSampleAdjustView;
	m_pSampleAdjustView = nullptr;

	m_pTargetSampleView->close();
	delete m_pTargetSampleView;
	m_pTargetSampleView = nullptr;

	INFOLOG ( "DESTROY" );
}


void SampleEditor::closeEvent(QCloseEvent *event)
{
	if ( !m_bSampleEditorStatus ) {
		int err = QMessageBox::information( this, "Hydrogen", tr( "Unsaved changes left. These changes will be lost. \nAre you sure?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 );
		if ( err == 0 ) {
			m_bSampleEditorStatus = true;
			accept();
		} else {
			event->ignore();
			return;
		}
	} else {
		accept();
	}
}


void SampleEditor::getAllFrameInfos()
{
	H2Core::Instrument *pInstrument = nullptr;
	std::shared_ptr<Sample> pSample;
	Song *pSong = Hydrogen::get_instance()->getSong();
	
	if (pSong != nullptr) {
		InstrumentList *pInstrList = pSong->get_instrument_list();
		int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		if ( nInstr >= static_cast<int>(pInstrList->size()) ) {
			nInstr = -1;
		}

		if (nInstr == -1) {
			pInstrument = nullptr;
		}
		else {
			pInstrument = pInstrList->get( nInstr );
			//INFOLOG( "new instr: " + pInstrument->m_sName );
		}
	}
	
	assert( pInstrument );
	
	H2Core::InstrumentLayer *pLayer = pInstrument->get_component(0)->get_layer( m_nSelectedLayer );
	if ( pLayer ) {
		pSample = pLayer->get_sample();
	}
	
	assert( pSample );

//this values are needed if we restore a sample from disk if a new song with sample changes will load
	m_bSampleIsModified = pSample->get_is_modified();
	m_nSamplerate = pSample->get_sample_rate();
	__loops = pSample->get_loops();
	__rubberband = pSample->get_rubberband();

	if ( pSample->get_velocity_envelope()->size()==0 ) {
		m_pTargetSampleView->get_velocity()->clear();
		m_pTargetSampleView->get_velocity()->push_back( std::make_unique<EnvelopePoint>( 0, 0 ) );
		m_pTargetSampleView->get_velocity()->push_back( std::make_unique<EnvelopePoint>( m_pTargetSampleView->width(), 0 ) );
	} else {
		m_pTargetSampleView->get_velocity()->clear();
		
		for(auto& pEnvPtr : *pSample->get_velocity_envelope() ){
			m_pTargetSampleView->get_velocity()->emplace_back( std::make_unique<EnvelopePoint>( pEnvPtr->value, pEnvPtr->frame ) );
		}
	}

	if ( pSample->get_pan_envelope()->size()==0 ) {
		m_pTargetSampleView->get_pan()->clear();
		m_pTargetSampleView->get_pan()->push_back( std::make_unique<EnvelopePoint>( 0, m_pTargetSampleView->height()/2 ) );
		m_pTargetSampleView->get_pan()->push_back( std::make_unique<EnvelopePoint>( m_pTargetSampleView->width(), m_pTargetSampleView->height()/2 ) );
	} else {
		for(auto& pEnvPtr : *pSample->get_pan_envelope() ){
			m_pTargetSampleView->get_pan()->emplace_back( std::make_unique<EnvelopePoint>( pEnvPtr.get() ) );
		}
	}

	if (m_bSampleIsModified) {
		__loops.end_frame = pSample->get_loops().end_frame;
		if ( __loops.mode == Sample::Loops::FORWARD ) {
			ProcessingTypeComboBox->setCurrentIndex ( 0 );
		}
		if ( __loops.mode == Sample::Loops::REVERSE ) {
			ProcessingTypeComboBox->setCurrentIndex ( 1 );
		}
		if ( __loops.mode == Sample::Loops::PINGPONG ) {
			ProcessingTypeComboBox->setCurrentIndex ( 2 );
		}
		
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

		if( !__rubberband.use ) { 
			rubberComboBox->setCurrentIndex( 0 );
		}
		
		rubberbandCsettingscomboBox->setCurrentIndex( __rubberband.c_settings );
		if( !__rubberband.use ) {
			rubberbandCsettingscomboBox->setCurrentIndex( 4 );
		}
		
		pitchdoubleSpinBox->setValue( __rubberband.pitch );
		if( !__rubberband.use ) { 
			pitchdoubleSpinBox->setValue( 0.0 );
		}

		if( __rubberband.divider == 1.0/64.0) {
			rubberComboBox->setCurrentIndex( 1 );
		}
		else if( __rubberband.divider == 1.0/32.0) {
			rubberComboBox->setCurrentIndex( 2 );
		} else if( __rubberband.divider == 1.0/16.0) {
			rubberComboBox->setCurrentIndex( 3 );
		} else if( __rubberband.divider == 1.0/8.0) {
			rubberComboBox->setCurrentIndex( 4 );
		} else if( __rubberband.divider == 1.0/4.0) {
			rubberComboBox->setCurrentIndex( 5 );
		} else if( __rubberband.divider == 1.0/2.0) {
			rubberComboBox->setCurrentIndex( 6 );
		} else if( __rubberband.use && ( __rubberband.divider >= 1.0 ) ) { 
			rubberComboBox->setCurrentIndex(  (int)(__rubberband.divider + 6) );
		}
		
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
	// wavedisplays
	m_divider = m_pSampleFromFile->get_frames() / 574.0F;
	m_pMainSampleWaveDisplay->updateDisplay( m_sSampleName );
	m_pMainSampleWaveDisplay->move( 1, 1 );

	m_pSampleAdjustView->updateDisplay( m_sSampleName );
	m_pSampleAdjustView->move( 1, 1 );

	m_pTargetSampleView->move( 1, 1 );
}



void SampleEditor::on_ClosePushButton_clicked()
{
	if ( !m_bSampleEditorStatus ){
		int err = QMessageBox::information( this, "Hydrogen", tr( "Unsaved changes left. These changes will be lost. \nAre you sure?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 );
		if ( err == 0 ){
			m_bSampleEditorStatus = true;
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
	m_bSampleEditorStatus = true;
	QApplication::restoreOverrideCursor();
}



bool SampleEditor::getCloseQuestion()
{
	bool close = false;
	int err = QMessageBox::information( this, "Hydrogen", tr( "Close dialog! maybe there is some unsaved work on sample.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 );
	if ( err == 0 ) close = true;

	return close;
}



void SampleEditor::createNewLayer()
{
	if ( !m_bSampleEditorStatus ){

		auto pEditSample = Sample::load( m_sSampleName, __loops, __rubberband, *m_pTargetSampleView->get_velocity(), *m_pTargetSampleView->get_pan() );

		if( pEditSample == nullptr ){
			return;
		}

		AudioEngine::get_instance()->lock( RIGHT_HERE );

		H2Core::Instrument *pInstrument = nullptr;
		Song *pSong = Hydrogen::get_instance()->getSong();
		if (pSong != nullptr) {
			InstrumentList *pInstrList = pSong->get_instrument_list();
			int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
			if ( nInstr >= static_cast<int>(pInstrList->size()) ) {
				nInstr = -1;
			}

			if (nInstr == -1) {
				pInstrument = nullptr;
			}
			else {
				pInstrument = pInstrList->get( nInstr );
			}
		}
		
		H2Core::InstrumentLayer *pLayer = nullptr;
		if( pInstrument ) {
			H2Core::InstrumentLayer *pLayer = pInstrument->get_component(0)->get_layer( m_nSelectedLayer );

			// insert new sample from newInstrument
			pLayer->set_sample( pEditSample );
		}

		AudioEngine::get_instance()->unlock();

		if( pLayer ) {
			m_pTargetSampleView->updateDisplay( pLayer );
		}
	}
}



void SampleEditor::mouseReleaseEvent(QMouseEvent *ev)
{

}



bool SampleEditor::returnAllMainWaveDisplayValues()
{
	testpTimer();
//	QMessageBox::information ( this, "Hydrogen", tr ( "jep %1" ).arg(m_pSample->get_frames()));
	m_bSampleIsModified = true;
	if( m_pMainSampleWaveDisplay->__startsliderismoved ) __loops.start_frame = m_pMainSampleWaveDisplay->m_pStartFramePosition * m_divider - 25 * m_divider;
	if( m_pMainSampleWaveDisplay->__loopsliderismoved ) __loops.loop_frame = m_pMainSampleWaveDisplay->m_pLoopFramePosition  * m_divider - 25 * m_divider;
	if( m_pMainSampleWaveDisplay->__endsliderismoved ) __loops.end_frame = m_pMainSampleWaveDisplay->m_pEndFramePosition  * m_divider - 25 * m_divider ;
	StartFrameSpinBox->setValue( __loops.start_frame );
	LoopFrameSpinBox->setValue( __loops.loop_frame );
	EndFrameSpinBox->setValue( __loops.end_frame );
	m_bOnewayStart = true;
	m_bOnewayLoop = true;
	m_bOnewayEnd = true;
	setSamplelengthFrames();

	return true;
}

void SampleEditor::returnAllTargetDisplayValues()
{
	setSamplelengthFrames();
	m_bSampleIsModified = true;

}

void SampleEditor::setTrue()
{
	m_bSampleEditorStatus = false;
}

void SampleEditor::valueChangedStartFrameSpinBox( int )
{
	testpTimer();
	m_pDetailFrame = StartFrameSpinBox->value();
	m_pLineColor = "Start";
	if ( !m_bOnewayStart ){
		m_pMainSampleWaveDisplay->m_pStartFramePosition = StartFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_pLineColor);
		__loops.start_frame = StartFrameSpinBox->value();

	}else
	{
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_pLineColor);
		m_bOnewayStart = false;
	}
	testPositionsSpinBoxes();
	m_bSampleEditorStatus = false;
	setSamplelengthFrames();
}

void SampleEditor::valueChangedLoopFrameSpinBox( int )
{
	testpTimer();
	m_pDetailFrame = LoopFrameSpinBox->value();
	m_pLineColor = "Loop";
	if ( !m_bOnewayLoop ){
		m_pMainSampleWaveDisplay->m_pLoopFramePosition = LoopFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_pLineColor);
		__loops.loop_frame = LoopFrameSpinBox->value();
	}else
	{
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_pLineColor);
		m_bOnewayLoop = false;
	}
	testPositionsSpinBoxes();
	m_bSampleEditorStatus = false;
	setSamplelengthFrames();
}

void SampleEditor::valueChangedEndFrameSpinBox( int )
{
	testpTimer();
	m_pDetailFrame = EndFrameSpinBox->value();
	m_pLineColor = "End";
	if ( !m_bOnewayEnd ){
		m_pMainSampleWaveDisplay->m_pEndFramePosition = EndFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_pLineColor);
		__loops.end_frame = EndFrameSpinBox->value();
	}else
	{
		m_bOnewayEnd = false;
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_pLineColor);
	}
	testPositionsSpinBoxes();
	m_bSampleEditorStatus = false;
	setSamplelengthFrames();
}

void SampleEditor::on_PlayPushButton_clicked()
{
	if (PlayPushButton->text() == "Stop" ){
		testpTimer();
		return;
	}

	const float pan_L = 0.5f;
	const float pan_R = 0.5f;
	const int nLength = -1;
	const float fPitch = 0.0f;
	const int selectedLayer = InstrumentEditorPanel::get_instance()->getSelectedLayer();

	Song *pSong = Hydrogen::get_instance()->getSong();
	Instrument *pInstr = pSong->get_instrument_list()->get( Hydrogen::get_instance()->getSelectedInstrumentNumber() );

	Note *pNote = new Note( pInstr, 0, pInstr->get_component( m_nSelectedComponent )->get_layer( selectedLayer )->get_end_velocity() - 0.01, pan_L, pan_R, nLength, fPitch);
	pNote->set_specific_compo_id( m_nSelectedComponent );
	AudioEngine::get_instance()->get_sampler()->noteOn(pNote);

	setSamplelengthFrames();
	createPositionsRulerPath();
	m_bPlayButton = true;
	m_pMainSampleWaveDisplay->paintLocatorEvent( StartFrameSpinBox->value() / m_divider + 24 , true);
	m_pSampleAdjustView->setDetailSamplePosition( __loops.start_frame, m_fZoomfactor , nullptr);

	if( __rubberband.use == false ){
		m_pTimer->start(40);	// update ruler at 25 fps
	}


	m_nRealtimeFrameEnd = Hydrogen::get_instance()->getRealtimeFrames() + m_nSlframes;

	//calculate the new rubberband sample length
	if( __rubberband.use ){
		m_nRealtimeFrameEndForTarget = Hydrogen::get_instance()->getRealtimeFrames() + (m_nSlframes * m_fRatio + 0.1);
	}else
	{
		m_nRealtimeFrameEndForTarget = m_nRealtimeFrameEnd;
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

	const int selectedlayer = InstrumentEditorPanel::get_instance()->getSelectedLayer();
	Song *pSong = Hydrogen::get_instance()->getSong();
	Instrument *pInstr = pSong->get_instrument_list()->get( Hydrogen::get_instance()->getSelectedInstrumentNumber() );

	/*
	 *preview_instrument deletes the last used preview instrument, therefore we have to construct a temporary
	 *instrument. Otherwise pInstr would be deleted if consumed by preview_instrument.
	*/
	Instrument *pTmpInstrument = Instrument::load_instrument( pInstr->get_drumkit_name(), pInstr->get_name() );
	auto pNewSample = Sample::load( pInstr->get_component(0)->get_layer( selectedlayer )->get_sample()->get_filepath() );

	if ( pNewSample != nullptr ){
		int length = ( ( pNewSample->get_frames() / pNewSample->get_sample_rate() + 1) * 100 );
		AudioEngine::get_instance()->get_sampler()->preview_instrument( pTmpInstrument );
		AudioEngine::get_instance()->get_sampler()->preview_sample( pNewSample, length );
		m_nSlframes = pNewSample->get_frames();
	}

	m_pMainSampleWaveDisplay->paintLocatorEvent( StartFrameSpinBox->value() / m_divider + 24 , true);
	m_pSampleAdjustView->setDetailSamplePosition( __loops.start_frame, m_fZoomfactor , nullptr);
	m_pTimer->start(40);	// update ruler at 25 fps
	m_nRealtimeFrameEnd = Hydrogen::get_instance()->getRealtimeFrames() + m_nSlframes;
	PlayOrigPushButton->setText( QString( "Stop") );
}

void SampleEditor::updateMainsamplePositionRuler()
{
	unsigned long realpos = Hydrogen::get_instance()->getRealtimeFrames();
	if ( realpos < m_nRealtimeFrameEnd ){
		unsigned frame = m_nSlframes - ( m_nRealtimeFrameEnd  - realpos );
		if ( m_bPlayButton == true ){
			m_pMainSampleWaveDisplay->paintLocatorEvent( m_pPositionsRulerPath[frame] / m_divider + 25 , true);
			m_pSampleAdjustView->setDetailSamplePosition( m_pPositionsRulerPath[frame], m_fZoomfactor , nullptr);
		}else{
			m_pMainSampleWaveDisplay->paintLocatorEvent( frame / m_divider + 25 , true);
			m_pSampleAdjustView->setDetailSamplePosition( frame, m_fZoomfactor , nullptr);
		}
//		ERRORLOG( QString("sampleval: %1").arg(frame) );
	}else
	{
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		PlayPushButton->setText( QString("&Play") );
		PlayOrigPushButton->setText( QString( "P&lay original sample") );
		m_bPlayButton = false;
	}
}

void SampleEditor::updateTargetsamplePositionRuler()
{
	unsigned long realpos = Hydrogen::get_instance()->getRealtimeFrames();
	unsigned targetSampleLength;
	if( __rubberband.use ){
		targetSampleLength =  m_nSlframes * m_fRatio + 0.1;
	}else
	{
		targetSampleLength =  m_nSlframes;
	}
	if ( realpos < m_nRealtimeFrameEndForTarget ){
		unsigned pos = targetSampleLength - ( m_nRealtimeFrameEndForTarget - realpos );
		m_pTargetSampleView->paintLocatorEventTargetDisplay( (m_pTargetSampleView->width() * pos /targetSampleLength), true);
//		ERRORLOG( QString("sampleval: %1").arg(frame) );
	}else
	{
		m_pTargetSampleView->paintLocatorEventTargetDisplay( -1 , false);
		m_pTargetDisplayTimer->stop();
		PlayPushButton->setText(QString( "&Play") );
		PlayOrigPushButton->setText( QString( "P&lay original sample") );
		m_bPlayButton = false;
	}
}



void SampleEditor::createPositionsRulerPath()
{
	setSamplelengthFrames();

	unsigned oneSampleLength =  __loops.end_frame - __loops.start_frame;
	unsigned loopLength =  __loops.end_frame - __loops.loop_frame;
	unsigned repeatsLength = loopLength * __loops.count;
	unsigned newLength = 0;
	if (oneSampleLength == loopLength){
		newLength = oneSampleLength + oneSampleLength * __loops.count ;
	}else
	{
		newLength =oneSampleLength + repeatsLength;
	}

	unsigned  normalLength = m_pSampleFromFile->get_frames();

	unsigned *	normalFrames = new unsigned[ normalLength ];
	unsigned *	tempFrames = new unsigned[ newLength ];
	unsigned *	loopFrames = new unsigned[ loopLength ];

	for ( unsigned i = 0; i < normalLength; i++ ) {
		normalFrames[i] = i;
	}

	Sample::Loops::LoopMode loopmode = __loops.mode;
	long int z = __loops.loop_frame;
	long int y = __loops.start_frame;

	for ( unsigned i = 0; i < newLength; i++){ //first vector
		tempFrames[i] = 0;
	}

	for ( unsigned i = 0; i < oneSampleLength; i++, y++){ //first vector

		tempFrames[i] = normalFrames[y];
	}

	for ( unsigned i = 0; i < loopLength; i++, z++){ //loop vector

		loopFrames[i] = normalFrames[z];
	}

	if ( loopmode == Sample::Loops::REVERSE ){
		std::reverse(loopFrames, loopFrames + loopLength);
	}

	if ( loopmode == Sample::Loops::REVERSE && __loops.count > 0 && __loops.start_frame == __loops.loop_frame ){
		std::reverse( tempFrames, tempFrames + oneSampleLength );
		}

	if ( loopmode == Sample::Loops::PINGPONG &&  __loops.start_frame == __loops.loop_frame){
		std::reverse(loopFrames, loopFrames + loopLength);
	}

	for ( int i = 0; i< __loops.count ;i++){
		unsigned tempdataend = oneSampleLength + ( loopLength * i );
		if ( __loops.start_frame == __loops.loop_frame ){
			std::copy( loopFrames, loopFrames+loopLength ,tempFrames+ tempdataend );
		}
		if ( loopmode == Sample::Loops::PINGPONG && __loops.count > 1){
			std::reverse(loopFrames, loopFrames + loopLength);
		}
		if ( __loops.start_frame != __loops.loop_frame ){
			std::copy( loopFrames, loopFrames+loopLength ,tempFrames+ tempdataend );
		}
	}


	if ( __loops.count == 0 && loopmode == Sample::Loops::REVERSE ){
		std::reverse( tempFrames + __loops.loop_frame, tempFrames + newLength);
	}

	if(m_pPositionsRulerPath)
	{
		delete[] m_pPositionsRulerPath;
	}

	m_pPositionsRulerPath = tempFrames;

	delete[] loopFrames;
	delete[] normalFrames;
}



void SampleEditor::setSamplelengthFrames()
{
	getAllLocalFrameInfos();
	unsigned oneSampleLength =  __loops.end_frame - __loops.start_frame;
	unsigned loopLength =  __loops.end_frame - __loops.loop_frame ;
	unsigned repeatsLength = loopLength * __loops.count;
	unsigned newLength = 0;

	if (oneSampleLength == loopLength){
		newLength = oneSampleLength + oneSampleLength * __loops.count ;
	}else
	{
		newLength =oneSampleLength + repeatsLength;
	}

	m_nSlframes = newLength;
	newlengthLabel->setText(QString("new sample length: %1 frames").arg(newLength));
	checkRatioSettings();
}



void SampleEditor::valueChangedLoopCountSpinBox( int )
{
	testpTimer();
	if ( m_nSlframes > Hydrogen::get_instance()->getAudioOutput()->getSampleRate() * 60 ){
		AudioEngine::get_instance()->get_sampler()->stopPlayingNotes();
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		m_bPlayButton = false;
	}
	__loops.count = LoopCountSpinBox->value() ;
	m_bSampleEditorStatus = false;
	setSamplelengthFrames();
	if ( m_nSlframes > Hydrogen::get_instance()->getAudioOutput()->getSampleRate() * 60 * 30){ // >30 min
		LoopCountSpinBox->setMaximum(LoopCountSpinBox->value() -1);
	}

}



void SampleEditor::valueChangedrubberbandCsettingscomboBox( const QString  )
{
	__rubberband.c_settings = rubberbandCsettingscomboBox->currentIndex();
	m_bSampleEditorStatus = false;
}



void SampleEditor::valueChangedpitchdoubleSpinBox( double )
{
	__rubberband.pitch = pitchdoubleSpinBox->value();
	m_bSampleEditorStatus = false;
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
//	QMessageBox::information ( this, "Hydrogen", tr ( "divider %1" ).arg( __rubberband.divider ));
//	float __rubberband.divider;
	setSamplelengthFrames();


	m_bSampleEditorStatus = false;
}

void SampleEditor::checkRatioSettings()
{
	//calculate ratio
	double durationtime = 60.0 / Hydrogen::get_instance()->getNewBpmJTM() * __rubberband.divider;
	double induration = (double) m_nSlframes / (double) m_nSamplerate;
	if (induration != 0.0) m_fRatio = durationtime / induration;

	//my personal ratio quality settings
	//ratios < 0.1 || > 3.0 are bad (red) or experimental sounds
	//ratios > 0.1 && < 0.5 || > 2.0 && < 3.0 are mediocre (yellow)
	//ratios > 0.5 && < 2.0 are good (green)
	//
	//         0.1        0.5               2.0            3.0
	//<---red---[--yellow--[------green------]----yellow----]---red--->

	//green ratio
	if( ( m_fRatio >= 0.5 ) && ( m_fRatio <= 2.0 ) ){
		rubberComboBox->setStyleSheet("QComboBox { background-color: green; }");
	}
	//yellow ratio
	else if( ( m_fRatio >= 0.1 ) && ( m_fRatio <=  3.0 ) ){
		rubberComboBox->setStyleSheet("QComboBox { background-color: yellow; }");
	}
	//red ratio
	else{
		rubberComboBox->setStyleSheet("QComboBox { background-color: red; }");
	}
	QString text = QString( " RB-Ratio = %1").arg(m_fRatio);
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
	m_bSampleEditorStatus = false;
}



void SampleEditor::on_verticalzoomSlider_valueChanged( int value )
{
	m_fZoomfactor = value / 10 +1;
	m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor, m_pLineColor );
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
		AudioEngine::get_instance()->get_sampler()->stopPlayingNotes();
		m_bPlayButton = false;
	}
}
