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

#include "SampleEditor.h"

#include "DetailWaveDisplay.h"
#include "MainSampleWaveDisplay.h"
#include "TargetWaveDisplay.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Note.h>
#include <core/Basics/Sample.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#include <algorithm>
#include <QMessageBox>
#include <QModelIndex>
#include <QTreeWidget>

using namespace H2Core;


SampleEditor::SampleEditor ( QWidget* pParent,
							std::shared_ptr< H2Core::InstrumentLayer > pLayer,
							std::shared_ptr< H2Core::InstrumentComponent > pComponent,
							std::shared_ptr< H2Core::Instrument > pInstrument )
		: QDialog ( pParent )
		, m_pLayer( pLayer )
		, m_pComponent( pComponent )
		, m_pInstrument( pInstrument )
{
	setupUi ( this );

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );
	if ( pInstrument == nullptr || pComponent == nullptr ||
		 pLayer == nullptr || pLayer->getSample() == nullptr ) {
		reject();
	}
	m_pSample = pLayer->getSample();

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateMainsamplePositionRuler()));
	m_pTargetDisplayTimer = new QTimer(this);
	connect(m_pTargetDisplayTimer, SIGNAL(timeout()), this, SLOT(updateTargetsamplePositionRuler()));

	setClean();
	m_fZoomfactor = 1;
	m_pDetailFrame = 0;
	m_sLineColor = "default";
	m_bOnewayStart = false;
	m_bOnewayLoop = false;
	m_bOnewayEnd = false;
	m_nSlframes = 0;
	m_pPositionsRulerPath = nullptr;
	m_bPlayButton = false;
	m_fRatio = 1.0f;
	__rubberband.c_settings = 4;

	//init Displays
	m_pMainSampleWaveDisplay = new MainSampleWaveDisplay( mainSampleview );
	m_pSampleAdjustView = new DetailWaveDisplay( mainSampleAdjustView );
	m_pTargetSampleView = new TargetWaveDisplay( targetSampleView );

	setWindowTitle( QString( tr( "SampleEditor " ) + m_pSample->getFilePath() )
	);

	setModal( true );

	const auto nFrames = m_pSample->getFrames();

	LoopCountSpinBox->setRange( 0, 20000 );
	StartFrameSpinBox->setRange( 0, nFrames );
	LoopFrameSpinBox->setRange( 0, nFrames );
	EndFrameSpinBox->setRange( 0, nFrames );
	EndFrameSpinBox->setValue( nFrames );
	rubberbandCsettingscomboBox->setCurrentIndex( 4 );
	rubberComboBox->setCurrentIndex( 0 );

	// Make things consistent with the LCDDisplay and LCDSpinBox classes.
	pitchdoubleSpinBox->setLocale( QLocale( QLocale::C, QLocale::AnyCountry ) );

	__rubberband.use = false;
	__rubberband.divider = 1.0;
	openDisplays();
	getAllFrameInfos();

#ifndef H2CORE_HAVE_RUBBERBAND
	if ( !Filesystem::file_executable( Preferences::get_instance()->m_sRubberBandCLIexecutable , true /* silent */) ) {
		RubberbandCframe->setDisabled ( true );
		setClean();
	}
#else
	RubberbandCframe->setDisabled ( false );
	setClean();
#endif

	__rubberband.pitch = 0.0;

	m_bAdjusting = false;
	m_bSampleEditorClean = true;
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

	INFOLOG( "DESTROY" );
}


void SampleEditor::closeEvent(QCloseEvent *event)
{
	if ( ! m_bSampleEditorClean ) {
		auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
		if ( QMessageBox::information(
				 this, "Hydrogen", pCommonStrings->getUnsavedChanges(),
				 QMessageBox::Ok | QMessageBox::Cancel,
				 QMessageBox::Cancel ) == QMessageBox::Ok ) {
			setClean();
			accept();
		}
		else {
			event->ignore();
			return;
		}
	} else {
		accept();
	}
}

void SampleEditor::getAllFrameInfos()
{
	// this values are needed if we restore a sample from disk if a
	// new song with sample changes will load
	m_bSampleIsModified = m_pSample->getIsModified();
	m_nSamplerate = m_pSample->getSampleRate();
	__loops = m_pSample->getLoops();

	// Per default all loop frames will be set to zero by Hydrogen. But this is
	// dangerous since just altering start or loop might move them beyond the
	// end.
	if ( __loops.start_frame == 0 &&
		 __loops.loop_frame == 0 &&
		 __loops.end_frame == 0 ) {
		__loops.end_frame = m_pSample->getFrames();
	}
	__rubberband = m_pSample->getRubberband();

	if ( m_pSample->getVelocityEnvelope().size()==0 ) {
		m_pTargetSampleView->get_velocity()->clear();
		m_pTargetSampleView->get_velocity()->push_back( EnvelopePoint( 0, 0 ) );
		m_pTargetSampleView->get_velocity()->push_back( EnvelopePoint( m_pTargetSampleView->width(), 0 ) );
	} else {
		m_pTargetSampleView->get_velocity()->clear();

		for( const auto& pt : m_pSample->getVelocityEnvelope() ){
			m_pTargetSampleView->get_velocity()->emplace_back( pt );
		}
	}

	if ( m_pSample->getPanEnvelope().size()==0 ) {
		m_pTargetSampleView->get_pan()->clear();
		m_pTargetSampleView->get_pan()->push_back( EnvelopePoint( 0, m_pTargetSampleView->height()/2 ) );
		m_pTargetSampleView->get_pan()->push_back( EnvelopePoint( m_pTargetSampleView->width(), m_pTargetSampleView->height()/2 ) );
	}
	else {
		m_pTargetSampleView->get_pan()->clear();
		for ( const auto& pt : m_pSample->getPanEnvelope() ){
			m_pTargetSampleView->get_pan()->emplace_back( pt );
		}
	}

	if (m_bSampleIsModified) {
		__loops.end_frame = m_pSample->getLoops().end_frame;
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

		m_pMainSampleWaveDisplay->m_nStartFramePosition = __loops.start_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_nLoopFramePosition =  __loops.loop_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_nEndFramePosition =  __loops.end_frame / m_divider + 25 ;
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
	m_pTargetSampleView->updateDisplay( m_pLayer );

	connect( StartFrameSpinBox, SIGNAL( valueChanged( int ) ), this,
			 SLOT( valueChangedStartFrameSpinBox(int) ) );
	connect( LoopFrameSpinBox, SIGNAL( valueChanged( int ) ), this,
			 SLOT( valueChangedLoopFrameSpinBox(int) ) );
	connect( EndFrameSpinBox, SIGNAL( valueChanged( int ) ), this,
			 SLOT( valueChangedEndFrameSpinBox(int) ) );
	connect( LoopCountSpinBox, SIGNAL( valueChanged( int ) ), this,
			 SLOT( valueChangedLoopCountSpinBox( int ) ) );
	connect( ProcessingTypeComboBox, SIGNAL( currentIndexChanged( int )),
			 this, SLOT( valueChangedProcessingTypeComboBox( int ) ) );
	connect( rubberComboBox, SIGNAL(currentIndexChanged( int )), this,
			 SLOT( valueChangedrubberComboBox( int ) ) );
	connect( rubberbandCsettingscomboBox, SIGNAL(currentIndexChanged( int )),
			 this, SLOT( valueChangedrubberbandCsettingscomboBox( int ) ) );
	connect( pitchdoubleSpinBox, SIGNAL ( valueChanged( double )  ), this,
			 SLOT( valueChangedpitchdoubleSpinBox( double ) ) );
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
	m_divider = m_pSample->getFrames() / 574.0F;
	m_pMainSampleWaveDisplay->updateDisplay( m_pSample );
	m_pMainSampleWaveDisplay->move( 1, 1 );

	m_pSampleAdjustView->updateDisplay( m_pSample );
	m_pSampleAdjustView->move( 1, 1 );

	m_pTargetSampleView->move( 1, 1 );
}



void SampleEditor::on_ClosePushButton_clicked()
{
	if ( !m_bSampleEditorClean ){
		auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
		if ( QMessageBox::information(
				 this, "Hydrogen", pCommonStrings->getUnsavedChanges(),
				 QMessageBox::Ok | QMessageBox::Cancel,
				 QMessageBox::Cancel ) == QMessageBox::Ok ) {
			setClean();
			accept();
		}
		else {
			return;
		}
	}
	else {
		accept();
	}
}



void SampleEditor::on_PrevChangesPushButton_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	getAllLocalFrameInfos();
	createNewLayer();
	setClean();
	Hydrogen::get_instance()->setIsModified( true );
	QApplication::restoreOverrideCursor();
}



bool SampleEditor::getCloseQuestion()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	return QMessageBox::information(
		this, "Hydrogen", pCommonStrings->getUnsavedChanges(),
		QMessageBox::Ok | QMessageBox::Cancel,
		QMessageBox::Cancel ) == QMessageBox::Ok;
}



void SampleEditor::createNewLayer()
{
	if ( !m_bSampleEditorClean ){

		auto pHydrogen = H2Core::Hydrogen::get_instance();
		auto pAudioEngine = pHydrogen->getAudioEngine();

		auto pEditSample = std::make_shared<Sample>( m_pSample->getFilePath(),
													 m_pSample->getLicense() );
		pEditSample->setLoops( __loops );
		pEditSample->setRubberband( __rubberband );
		pEditSample->setVelocityEnvelope( *m_pTargetSampleView->get_velocity() );
		pEditSample->setPanEnvelope( *m_pTargetSampleView->get_pan() );

		if( ! pEditSample->load( pAudioEngine->getTransportPosition()->getBpm() ) ){
			ERRORLOG( "Unable to load modified sample" );
			return;
		}

		pAudioEngine->lock( RIGHT_HERE );

		m_pInstrument->setSample(
			m_pComponent, m_pLayer, pEditSample, Event::Trigger::Default
		);

		pAudioEngine->unlock();

		m_pTargetSampleView->updateDisplay( m_pLayer );
	}
}

void SampleEditor::mouseReleaseEvent(QMouseEvent *ev) {
}

bool SampleEditor::returnAllMainWaveDisplayValues()
{
	m_bAdjusting = true;

	testpTimer();
	m_bSampleIsModified = true;
	if( m_pMainSampleWaveDisplay->m_bStartSliderIsMoved ) __loops.start_frame = m_pMainSampleWaveDisplay->m_nStartFramePosition * m_divider - 25 * m_divider;
	if( m_pMainSampleWaveDisplay->m_bLoopSliderIsMoved ) __loops.loop_frame = m_pMainSampleWaveDisplay->m_nLoopFramePosition  * m_divider - 25 * m_divider;
	if( m_pMainSampleWaveDisplay->m_bEndSliderIsmoved ) __loops.end_frame = m_pMainSampleWaveDisplay->m_nEndFramePosition  * m_divider - 25 * m_divider ;
	StartFrameSpinBox->setValue( __loops.start_frame );
	LoopFrameSpinBox->setValue( __loops.loop_frame );
	EndFrameSpinBox->setValue( __loops.end_frame );
	m_bOnewayStart = true;
	m_bOnewayLoop = true;
	m_bOnewayEnd = true;
	setSamplelengthFrames();
	m_bAdjusting = false;
	setUnclean();
	return true;
}

void SampleEditor::returnAllTargetDisplayValues()
{
	setSamplelengthFrames();
	m_bSampleIsModified = true;
}

void SampleEditor::setUnclean()
{
	m_bSampleEditorClean = false;
	PrevChangesPushButton->setDisabled ( false );
	PrevChangesPushButton->setFlat ( false );
	// PrevChangesPushButton->show();
}

void SampleEditor::setClean()
{
	m_bSampleEditorClean = true;
	PrevChangesPushButton->setDisabled ( true );
	PrevChangesPushButton->setFlat ( true );
}

void SampleEditor::valueChangedStartFrameSpinBox( int )
{
	testpTimer();
	m_pDetailFrame = StartFrameSpinBox->value();
	if (m_pDetailFrame == __loops.start_frame) { // no actual change
		if (! m_bAdjusting ) on_PlayPushButton_clicked();
		return;
	}
	m_sLineColor = "Start";
	if ( !m_bOnewayStart ){
		m_pMainSampleWaveDisplay->m_nStartFramePosition = StartFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		__loops.start_frame = m_pDetailFrame;

	}else
	{
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		m_bOnewayStart = false;
	}
	testPositionsSpinBoxes();
	setUnclean();
	setSamplelengthFrames();
}

void SampleEditor::valueChangedLoopFrameSpinBox( int )
{
	testpTimer();
	m_pDetailFrame = LoopFrameSpinBox->value();
	if (m_pDetailFrame == __loops.loop_frame) {
		if ( ! m_bAdjusting ) on_PlayPushButton_clicked();
		return;
	}
	m_sLineColor = "Loop";
	if ( !m_bOnewayLoop ){
		m_pMainSampleWaveDisplay->m_nLoopFramePosition = LoopFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		__loops.loop_frame = m_pDetailFrame;
	}else
	{
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		m_bOnewayLoop = false;
	}
	testPositionsSpinBoxes();
	setUnclean();
	setSamplelengthFrames();
}

void SampleEditor::valueChangedEndFrameSpinBox( int )
{
	testpTimer();
	m_pDetailFrame = EndFrameSpinBox->value();
	if ( m_pDetailFrame == __loops.end_frame) {
		if ( ! m_bAdjusting ) on_PlayPushButton_clicked();
		return;
	}
	m_sLineColor = "End";
	if ( !m_bOnewayEnd ){
		m_pMainSampleWaveDisplay->m_nEndFramePosition = EndFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		__loops.end_frame = m_pDetailFrame;
	}else
	{
		m_bOnewayEnd = false;
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
	}
	testPositionsSpinBoxes();
	setUnclean();
	setSamplelengthFrames();
}

void SampleEditor::on_PlayPushButton_clicked()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if (PlayPushButton->text() == "Stop" ){
		testpTimer();
		return;
	}

	// Since we are in a separate dialog and working with a particular, we do
	// not want rendering to be affected by whether some instruments of the
	// current kit are soloed or muted.
	auto pPreviewInstr = std::make_shared<Instrument>( m_pInstrument );
	pPreviewInstr->setIsPreviewInstrument( true );

	auto pCompo = pPreviewInstr->getComponent(
		m_pInstrument->index( m_pComponent ) );
	if ( pCompo == nullptr ) {
		return;
	}
	auto pLayer = pCompo->getLayer( m_pComponent->index( m_pLayer ) );
	if ( pLayer == nullptr ) {
		return;
	}

	// We register the current component to be rendered using a specific layer.
	// This will cause all other components _not_ to be rendered.
	auto pSelectedLayerInfo = std::make_shared<SelectedLayerInfo>();
	pSelectedLayerInfo->pLayer = pLayer;

	auto pNote = std::make_shared<Note>(
		pPreviewInstr, 0, pLayer->getEndVelocity() - 0.01 );
	pNote->setSelectedLayerInfo( pSelectedLayerInfo, pCompo );

	pHydrogen->getAudioEngine()->getSampler()->noteOn( pNote );

	setSamplelengthFrames();
	createPositionsRulerPath();
	m_bPlayButton = true;
	m_pMainSampleWaveDisplay->paintLocatorEvent( StartFrameSpinBox->value() / m_divider + 24 , true);
	m_pSampleAdjustView->setDetailSamplePosition( __loops.start_frame, m_fZoomfactor , nullptr);

	if( __rubberband.use == false ){
		m_pTimer->start(40);	// update ruler at 25 fps
	}


	m_nRealtimeFrameEnd = pAudioEngine->getRealtimeFrame() + m_nSlframes;

	//calculate the new rubberband sample length
	if( __rubberband.use ){
		m_nRealtimeFrameEndForTarget = pAudioEngine->getRealtimeFrame() + (m_nSlframes * m_fRatio + 0.1);
	}else
	{
		m_nRealtimeFrameEndForTarget = m_nRealtimeFrameEnd;
	}
	m_pTargetDisplayTimer->start(40);	// update ruler at 25 fps
	PlayPushButton->setText( QString( "Stop") );

}

void SampleEditor::on_PlayOrigPushButton_clicked()
{
	if ( PlayOrigPushButton->text() == "Stop" ){
		testpTimer();
		return;
	}
	auto pHydrogen = Hydrogen::get_instance();
	auto tearDown = [&]() {
		m_pMainSampleWaveDisplay->paintLocatorEvent(
			StartFrameSpinBox->value() / m_divider + 24 , true);
		m_pSampleAdjustView->setDetailSamplePosition(
			__loops.start_frame, m_fZoomfactor , nullptr);
		m_pTimer->start(40);	// update ruler at 25 fps
		m_nRealtimeFrameEnd =
			pHydrogen->getAudioEngine()->getRealtimeFrame() + m_nSlframes;
		PlayOrigPushButton->setText( QString( "Stop") );
	};

	// Construct a custom instrument containing the current settings -
	// instrument, component, and layer - but using the original sample.
	auto pPreviewInstr = std::make_shared<Instrument>( m_pInstrument );
	pPreviewInstr->setIsPreviewInstrument( true );

	auto pCompo = pPreviewInstr->getComponent(
		m_pInstrument->index( m_pComponent ) );
	if ( pCompo == nullptr ) {
		tearDown();
		return;
	}
	auto pLayer = pCompo->getLayer( m_pComponent->index( m_pLayer ) );
	if ( pLayer == nullptr ) {
		tearDown();
		return;
	}
	auto pNewSample = Sample::load( m_pSample->getFilePath() );
	if ( pNewSample == nullptr ) {
		ERRORLOG( QString( "Unable to load sample from [%1]" )
				  .arg( m_pSample->getFilePath() ) );
		tearDown();
		return;
	}

	pPreviewInstr->setSample(
		pCompo, pLayer, pNewSample, Event::Trigger::Default
	);

	// Construct a note rendering just our new sample.
	const int nLength = ( pNewSample->getFrames() /
						  pNewSample->getSampleRate() + 1 ) * 100;
	auto pNote = std::make_shared<Note>(
		pPreviewInstr, 0, VELOCITY_MAX, PAN_DEFAULT, nLength );
	auto pSelectedLayerInfo = std::make_shared<SelectedLayerInfo>();
	pSelectedLayerInfo->pLayer = pLayer;
	pNote->setSelectedLayerInfo( pSelectedLayerInfo, pCompo );

	pHydrogen->getAudioEngine()->getSampler()->previewInstrument(
		pPreviewInstr, pNote );
	m_nSlframes = pNewSample->getFrames();

	tearDown();
}

void SampleEditor::updateMainsamplePositionRuler()
{
	unsigned long realpos = Hydrogen::get_instance()->getAudioEngine()->getRealtimeFrame();
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
	} else {
		auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		PlayPushButton->setText( pCommonString->getButtonPlay() );
		PlayOrigPushButton->setText( pCommonString->getButtonPlayOriginalSample() );
		m_bPlayButton = false;
	}
}

void SampleEditor::updateTargetsamplePositionRuler()
{
	unsigned long realpos = Hydrogen::get_instance()->getAudioEngine()->getRealtimeFrame();
	unsigned targetSampleLength;
	if ( __rubberband.use ){
		targetSampleLength =  m_nSlframes * m_fRatio + 0.1;
	} else {
		targetSampleLength =  m_nSlframes;
	}
	
	if ( realpos < m_nRealtimeFrameEndForTarget ){
		unsigned pos = targetSampleLength - ( m_nRealtimeFrameEndForTarget - realpos );
		m_pTargetSampleView->paintLocatorEventTargetDisplay( (m_pTargetSampleView->width() * pos /targetSampleLength), true);
//		ERRORLOG( QString("sampleval: %1").arg(frame) );
	} else {
		auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();
		m_pTargetSampleView->paintLocatorEventTargetDisplay( -1 , false);
		m_pTargetDisplayTimer->stop();
		PlayPushButton->setText( pCommonString->getButtonPlay() );
		PlayOrigPushButton->setText( pCommonString->getButtonPlayOriginalSample() );
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

	unsigned  normalLength = m_pSample->getFrames();

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

	if ( oneSampleLength == loopLength ){
		newLength = oneSampleLength + oneSampleLength * __loops.count ;
	} else {
		newLength =oneSampleLength + repeatsLength;
	}

	m_nSlframes = newLength;
	newlengthLabel->setText(QString( tr( "new sample length" ) )
							.append( QString( ": %1 " ).arg(newLength) )
							.append( tr( "frames" )));
	checkRatioSettings();
}



void SampleEditor::valueChangedLoopCountSpinBox( int )
{
	testpTimer();
	int count = LoopCountSpinBox->value();

	if (count == __loops.count) {
		if ( ! m_bAdjusting ) on_PlayOrigPushButton_clicked();
		return;
	}

	const auto pHydrogen = Hydrogen::get_instance();
	const auto pAudioDriver = pHydrogen->getAudioOutput();
	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return;
	}

	if ( m_nSlframes > pAudioDriver->getSampleRate() * 60 ){
		pHydrogen->getAudioEngine()->getSampler()->stopPlayingNotes();
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		m_bPlayButton = false;
	}
	__loops.count = count; 
	setUnclean();
	setSamplelengthFrames();
	if ( m_nSlframes > pAudioDriver->getSampleRate() * 60 * 30){ // >30 min
		LoopCountSpinBox->setMaximum(LoopCountSpinBox->value() -1);
	}

}



void SampleEditor::valueChangedrubberbandCsettingscomboBox( int )
{
	int new_settings = rubberbandCsettingscomboBox->currentIndex();
	if (new_settings == __rubberband.c_settings) {
		if (! m_bAdjusting ) on_PlayPushButton_clicked();
		return;
	}
	__rubberband.c_settings = new_settings;
	setUnclean();
}



void SampleEditor::valueChangedpitchdoubleSpinBox( double )
{
	double new_value = pitchdoubleSpinBox->value();
	if (std::abs(new_value - __rubberband.pitch) < 0.0001) {
		if (! m_bAdjusting ) on_PlayPushButton_clicked();
		return;
	}
	__rubberband.pitch = new_value;
	setUnclean();
}


void SampleEditor::valueChangedrubberComboBox( int )
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


	setUnclean();
}

void SampleEditor::checkRatioSettings()
{
	//calculate ratio
	double durationtime = 60.0 / Hydrogen::get_instance()->getAudioEngine()->getTransportPosition()->getBpm()
		* __rubberband.divider;
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
	QString text = QString( tr(" RB-Ratio" ) )
		.append( QString( " %1" ).arg( m_fRatio ) );
	ratiolabel->setText( text );

	//no rubberband = default
	if( !__rubberband.use ){
		rubberComboBox->setStyleSheet("QComboBox { background-color: 58, 62, 72; }");
		ratiolabel->setText( "" );
	}
}


void SampleEditor::valueChangedProcessingTypeComboBox( int nUnused )
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
	setUnclean();
}



void SampleEditor::on_verticalzoomSlider_valueChanged( int value )
{
	m_fZoomfactor = value / 10 +1;
	m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor, m_sLineColor );
}



void SampleEditor::testPositionsSpinBoxes()
{
	m_bAdjusting = true;
	if (  __loops.start_frame > __loops.loop_frame ) __loops.loop_frame = __loops.start_frame;
	if (  __loops.start_frame > __loops.end_frame ) __loops.end_frame = __loops.start_frame;
	if (  __loops.loop_frame > __loops.end_frame ) __loops.end_frame = __loops.loop_frame;
	if (  __loops.end_frame < __loops.loop_frame ) __loops.loop_frame = __loops.end_frame;
	if (  __loops.end_frame < __loops.start_frame ) __loops.start_frame = __loops.end_frame;
	StartFrameSpinBox->setValue( __loops.start_frame );
	LoopFrameSpinBox->setValue( __loops.loop_frame );
	EndFrameSpinBox->setValue( __loops.end_frame );
	m_bAdjusting = false;
}



void SampleEditor::testpTimer()
{
	if ( m_pTimer->isActive() || m_pTargetDisplayTimer->isActive() ){
		auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		m_pTargetDisplayTimer->stop();
		PlayPushButton->setText( pCommonString->getButtonPlay() );
		PlayOrigPushButton->setText( pCommonString->getButtonPlayOriginalSample() );
		Hydrogen::get_instance()->getAudioEngine()->getSampler()->stopPlayingNotes();
		m_bPlayButton = false;
	}
}
