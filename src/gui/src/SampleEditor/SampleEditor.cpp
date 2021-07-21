/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

	m_nSelectedLayer = nSelectedLayer;
	m_nSelectedComponent = nSelectedComponent;
	m_sSampleName = sSampleFilename;
	m_fZoomfactor = 1;
	m_pDetailFrame = 0;
	m_sLineColor = "default";
	m_bOnewayStart = false;
	m_bOnewayLoop = false;
	m_bOnewayEnd = false;
	m_nSlframes = 0;
	m_pPositionsRulerPath = nullptr;
	m_bPlayButton = false;
	m_bPlayingOriginalSample = false;
	m_Ratio = 1.0;
	__rubberband.c_settings = 4;

	QString newfilename = sSampleFilename.section( '/', -1 );

	//init Displays
	m_pMainSampleWaveDisplay = new MainSampleWaveDisplay( mainSampleview );
	m_pSampleAdjustView = new DetailWaveDisplay( mainSampleAdjustView );
	m_pTargetSampleView = new TargetWaveDisplay( targetSampleView );

	setWindowTitle ( QString( tr( "SampleEditor " ) + newfilename) );
	setModal ( true );

	//this new sample give us the not changed real samplelength
	m_pSampleFromFile = Sample::load( sSampleFilename );
	if ( m_pSampleFromFile == nullptr ) {
		reject();
	}
	// m_pEditedSample = std::make_shared<H2Core::Sample>(m_pSampleFromFile);
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
	__rubberband.pitch = 0.0;

	openDisplays();
	getAllFrameInfos();
	returnAllMainWaveDisplayValues();
	doneEditing();
	setClean();

	m_bAdjusting = false;
	m_bSampleEditorClean = true;

	adjustSize();
	setFixedSize ( width(), height() );

	connect( StartFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedStartFrameSpinBox(int) ) );
	connect( LoopFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedLoopFrameSpinBox(int) ) );
	connect( EndFrameSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedEndFrameSpinBox(int) ) );
	connect( LoopCountSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( valueChangedLoopCountSpinBox( int ) ) );
	connect( ProcessingTypeComboBox, SIGNAL( currentIndexChanged ( const QString )  ), this, SLOT( valueChangedProcessingTypeComboBox( const QString ) ) );
	connect( rubberComboBox, SIGNAL( currentIndexChanged ( const QString )  ), this, SLOT( valueChangedrubberComboBox( const QString ) ) );
	connect( rubberbandCsettingscomboBox, SIGNAL( currentIndexChanged ( const QString )  ), this, SLOT( valueChangedrubberbandCsettingscomboBox( const QString ) ) );
	connect( pitchdoubleSpinBox, SIGNAL ( valueChanged( double )  ), this, SLOT( valueChangedpitchdoubleSpinBox( double ) ) );
	connect( EditTypeComboBox, SIGNAL ( currentIndexChanged ( int ) ), this, SLOT( valueChangedEditTypeComboBox( int ) ) );

	connect( m_pTargetSampleView, SIGNAL ( envelopeEdited ( SampleEditor::EnvelopeType ) ), this, SLOT( envelopeEdited ( SampleEditor::EnvelopeType ) ) );
	connect( m_pTargetSampleView, SIGNAL ( doneEditingEnvelope ( SampleEditor::EnvelopeType ) ), this, SLOT( envelopeEdited ( SampleEditor::EnvelopeType ) ) );

	connect( m_pMainSampleWaveDisplay, SIGNAL ( sliderEdited ( SampleEditor::Slider ) ), this, SLOT( sliderEdited ( SampleEditor::Slider ) ) );
	connect( m_pMainSampleWaveDisplay, SIGNAL ( doneEditingSlider ( SampleEditor::Slider ) ), this, SLOT( sliderEdited ( SampleEditor::Slider ) ) );


#ifndef H2CORE_HAVE_RUBBERBAND
	if ( !Filesystem::file_executable( Preferences::get_instance()->m_rubberBandCLIexecutable , true /* silent */) ) {
		RubberbandCframe->setDisabled ( true );
	}
#else
	RubberbandCframe->setDisabled ( false );
#endif
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

void SampleEditor::envelopeEdited( SampleEditor::EnvelopeType mode)
{
	returnAllTargetDisplayValues();
	setUnclean();
	doneEditing();
}

bool SampleEditor::rubberbandIsOff()
{
	return rubberComboBox->currentIndex() == 0;
}

void SampleEditor::doneEditing() {
	auto edited = std::make_shared<H2Core::Sample>(m_pSampleFromFile);
	edited->apply(__loops,
				__rubberband,
				*m_pTargetSampleView->get_velocity(),
				*m_pTargetSampleView->get_pan());
	m_pTargetSampleView->updateDisplay( edited, 1.0 );
	m_pEditedSample = edited;
}

void SampleEditor::sliderEdited( SampleEditor::Slider slider )
{
	returnAllMainWaveDisplayValues();
	if ( rubberbandIsOff() &&
		(slider == SampleEditor::Slider::StartSlider ||
		slider == SampleEditor::Slider::EndSlider ||
		slider == SampleEditor::Slider::LoopSlider && __loops.count)
	) {
		__rubberband.divider = computeNoopRubberbandDivider();
	}
	setUnclean();
	doneEditing();
}


void SampleEditor::closeEvent(QCloseEvent *event)
{
	if ( !m_bSampleEditorClean ) {
		int err = QMessageBox::information( this, "Hydrogen", tr( "Unsaved changes left. These changes will be lost. \nAre you sure?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 );
		if ( err == 0 ) {
			setClean();
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
	H2Core::Hydrogen *hydrogen = Hydrogen::get_instance();
	if ( hydrogen == nullptr ) {
		ERRORLOG("no hydrogen");
		return;
	}

	std::shared_ptr<Song> pSong = hydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG("no song");
		return;
	}

	InstrumentList *pInstrList = pSong->getInstrumentList();
	if ( pInstrList == nullptr ) {
		ERRORLOG("no instrument list");
		return;
	}
	int nInstr = hydrogen->getSelectedInstrumentNumber();
	if ( nInstr < 0 || nInstr >= static_cast<int>(pInstrList->size()) ) {
		ERRORLOG("out of range instrument number");
		return;
	}

	std::shared_ptr<H2Core::Instrument> pInstrument = pInstrList->get( nInstr );
	if ( pInstrument == nullptr ) {
		ERRORLOG("no instrument");
		return;
	};

	auto pCompo = pInstrument->get_component( m_nSelectedComponent );
	if ( pCompo == nullptr ) {
		ERRORLOG("no instrument component");
		return;
	}

	auto pLayer = pCompo->get_layer( m_nSelectedLayer );
	if ( pLayer == nullptr ) {
		ERRORLOG("invalid layer selection");
		return;
	}

	std::shared_ptr<Sample> pSample = pLayer->get_sample();
	if ( pSample == nullptr ) {
		ERRORLOG("no sample");
	}

	// this values are needed if we restore a sample from disk if a new song with sample changes will load
	m_bSampleIsModified = pSample->get_is_modified();
	m_nSamplerate = pSample->get_sample_rate();
	__loops = pSample->get_loops();
	__rubberband = pSample->get_rubberband();

	if ( pSample->get_velocity_envelope()->size()==0 ) {
		m_pTargetSampleView->get_velocity()->clear();
		m_pTargetSampleView->get_velocity()->push_back( EnvelopePoint( 0, 0 ) );
		m_pTargetSampleView->get_velocity()->push_back( EnvelopePoint( m_pTargetSampleView->width(), 0 ) );
	} else {
		m_pTargetSampleView->get_velocity()->clear();
		for(auto& pt : *pSample->get_velocity_envelope() ){
			m_pTargetSampleView->get_velocity()->emplace_back( pt );
		}
	}

	if ( pSample->get_pan_envelope()->size()==0 ) {
		m_pTargetSampleView->get_pan()->clear();
		m_pTargetSampleView->get_pan()->push_back( EnvelopePoint( 0, m_pTargetSampleView->height()/2 ) );
		m_pTargetSampleView->get_pan()->push_back( EnvelopePoint( m_pTargetSampleView->width(), m_pTargetSampleView->height()/2 ) );
	} else {
		for(auto& pt : *pSample->get_pan_envelope() ){
			m_pTargetSampleView->get_pan()->emplace_back( pt );
		}
	}

	if (m_bSampleIsModified) {
		// update loop and rubberband settings view
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

		m_pMainSampleWaveDisplay->m_nStartFramePosition = __loops.start_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_nLoopFramePosition =  __loops.loop_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_nEndFramePosition =  __loops.end_frame / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();

		rubberbandCsettingscomboBox->setCurrentIndex( __rubberband.c_settings );

		pitchdoubleSpinBox->setValue( __rubberband.pitch );
		// if( !__rubberband.use ) {
		//	pitchdoubleSpinBox->setValue( 0.0 );
		//}

		setSamplelengthFrames();

		if ( !__rubberband.use || (std::abs(computeCurrentRatio() - 1.0) < 0.0001) ) {
			rubberComboBox->setCurrentIndex( 0 );
		} else if( __rubberband.divider == 1.0/64.0) {
			rubberComboBox->setCurrentIndex( 1 );
		} else if( __rubberband.divider == 1.0/32.0) {
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

		checkRatioSettings();
	}

	m_pTargetSampleView->updateDisplay( pLayer );
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
	auto sample = m_pMainSampleWaveDisplay->loadSampleAndUpdateDisplay( m_sSampleName );

	m_pMainSampleWaveDisplay->move( 1, 1 );

	m_pSampleAdjustView->updateDisplay( m_sSampleName );
	m_pSampleAdjustView->move( 1, 1 );
	m_pTargetSampleView->move( 1, 1 );
	m_pTargetSampleView->updateDisplay( sample, 1.0 );
}



void SampleEditor::on_ClosePushButton_clicked()
{
	if ( !m_bSampleEditorClean ){
		int err = QMessageBox::information( this, "Hydrogen", tr( "Unsaved changes left. These changes will be lost. \nAre you sure?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 );
		if ( err == 0 ){
			setClean();
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
	setClean();
	QApplication::restoreOverrideCursor();
	InstrumentEditorPanel::get_instance()->updateWaveDisplay();
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
	if ( !m_bSampleEditorClean ){

		auto pEditSample = Sample::load( m_sSampleName, __loops, __rubberband, *m_pTargetSampleView->get_velocity(), *m_pTargetSampleView->get_pan() );

		if( pEditSample == nullptr ){
			return;
		}

		Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );

		std::shared_ptr<H2Core::Instrument> pInstrument = nullptr;
		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
		if (pSong != nullptr) {
			InstrumentList *pInstrList = pSong->getInstrumentList();
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

		std::shared_ptr<H2Core::InstrumentLayer> pLayer = nullptr;
		if( pInstrument ) {
			pLayer = pInstrument->get_component( m_nSelectedComponent )->get_layer( m_nSelectedLayer );

			// insert new sample from newInstrument
			pLayer->set_sample( pEditSample );
		}

		Hydrogen::get_instance()->getAudioEngine()->unlock();

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
	m_bAdjusting = true;

	testpTimer();
//	QMessageBox::information ( this, "Hydrogen", tr ( "jep %1" ).arg(m_pSample->get_frames()));
	if (m_pMainSampleWaveDisplay->m_SelectedSlider != SampleEditor::NoSlider) {
		m_bSampleIsModified = true;
		__loops.start_frame = m_pMainSampleWaveDisplay->m_nStartFramePosition * m_divider - 25 * m_divider;
		__loops.loop_frame = m_pMainSampleWaveDisplay->m_nLoopFramePosition  * m_divider - 25 * m_divider;
		__loops.end_frame = m_pMainSampleWaveDisplay->m_nEndFramePosition  * m_divider - 25 * m_divider ;
		StartFrameSpinBox->setValue( __loops.start_frame );
		LoopFrameSpinBox->setValue( __loops.loop_frame );
		EndFrameSpinBox->setValue( __loops.end_frame );
	}
	m_bOnewayStart = true;
	m_bOnewayLoop = true;
	m_bOnewayEnd = true;
	setSamplelengthFrames();
	resetPositionsRulerPath();
	checkRatioSettings();
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

/////////////////////////////////////////////////////////////////
// Loop Controls
/////////////////////////////////////////////////////////////////
void SampleEditor::valueChangedStartFrameSpinBox( int value)
{
	bool change = (value != __loops.start_frame);
	if ( change && ! m_bAdjusting ) testpTimer();
	m_pDetailFrame = value;
	m_sLineColor = "Start";
	if ( !m_bOnewayStart ){
		m_pMainSampleWaveDisplay->m_nStartFramePosition = StartFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		__loops.start_frame = value;

	}else
	{
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		m_bOnewayStart = false;
	}
	if ( change ) {
		testPositionsSpinBoxes();
		setSamplelengthFrames();
		resetPositionsRulerPath();
		if ( rubberbandIsOff() ) {
			__rubberband.divider = computeNoopRubberbandDivider();
		}
		checkRatioSettings();
		setUnclean();
	}
	doneEditing();
}

void SampleEditor::valueChangedLoopFrameSpinBox( int value )
{
	bool change = (value != __loops.loop_frame);
	if ( change && ! m_bAdjusting ) testpTimer();
	m_pDetailFrame = value;
	m_sLineColor = "Loop";
	if ( !m_bOnewayLoop ){
		m_pMainSampleWaveDisplay->m_nLoopFramePosition = LoopFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		__loops.loop_frame = value;
	}else
	{
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		m_bOnewayLoop = false;
	}
	if ( change ) {
		testPositionsSpinBoxes();
		setSamplelengthFrames();
		resetPositionsRulerPath();
		if ( rubberbandIsOff() ) {
			__rubberband.divider = computeNoopRubberbandDivider();
		}
		checkRatioSettings();
		setUnclean();
	}
	doneEditing();
}

void SampleEditor::valueChangedEndFrameSpinBox( int value)
{
	bool change = (value != __loops.end_frame);
	if ( change && ! m_bAdjusting ) testpTimer();
	m_pDetailFrame = value;
	m_sLineColor = "End";
	if ( !m_bOnewayEnd ){
		m_pMainSampleWaveDisplay->m_nEndFramePosition = EndFrameSpinBox->value() / m_divider + 25 ;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
		__loops.end_frame = value;
	}else
	{
		m_bOnewayEnd = false;
		m_pSampleAdjustView->setDetailSamplePosition( m_pDetailFrame, m_fZoomfactor , m_sLineColor);
	}
	if ( change ) {
		testPositionsSpinBoxes();
		setSamplelengthFrames();
		resetPositionsRulerPath();
		if ( rubberbandIsOff() ) {
			__rubberband.divider = computeNoopRubberbandDivider();
		}
		checkRatioSettings();
		setUnclean();
	}
	doneEditing();
}

void SampleEditor::valueChangedLoopCountSpinBox( int value )
{
	Hydrogen *hydrogen = Hydrogen::get_instance();
	bool change = (value != __loops.count);

	if ( change && ! m_bAdjusting ) testpTimer();

	if ( hydrogen && m_nSlframes > hydrogen->getAudioOutput()->getSampleRate() * 60 ){
		hydrogen->getAudioEngine()->getSampler()->stopPlayingNotes();
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		m_bPlayButton = false;
	}
	__loops.count = value;
	if ( hydrogen && m_nSlframes > hydrogen->getAudioOutput()->getSampleRate() * 60 * 30){ // >30 min
		LoopCountSpinBox->setMaximum(LoopCountSpinBox->value() -1);
	}
	if ( change ) {
		testPositionsSpinBoxes();
		setSamplelengthFrames();
		resetPositionsRulerPath();
		if ( rubberbandIsOff() ) {
			__rubberband.divider = computeNoopRubberbandDivider();
		}
		checkRatioSettings();
		setUnclean();
	}
	doneEditing();
}


void SampleEditor::valueChangedProcessingTypeComboBox( const QString unused )
{
	Sample::Loops::LoopMode previous = __loops.mode;

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
	bool change = ( previous != __loops.mode );
	if ( change ) {
		resetPositionsRulerPath();
		setUnclean();
	}
	doneEditing();
}



/////////////////////////////////////////////////////////////////
// Original / Edited Sample Playing
/////////////////////////////////////////////////////////////////


void SampleEditor::on_PlayPushButton_clicked()
{
	if (PlayPushButton->text() == "Stop" ){ // this should be a better way if we want to translate this label
		testpTimer();
		return;
	}
	m_bPlayButton = true;
	playSample(m_pEditedSample, false);

	PlayPushButton->setText( QString( "Stop") );
}

void SampleEditor::on_PlayOrigPushButton_clicked()
{
	if (PlayOrigPushButton->text() == "Stop" ){
		testpTimer();
		return;
	}
	m_bPlayButton = true;
	playSample(m_pSampleFromFile, true);

	PlayOrigPushButton->setText( QString( "Stop") );
}

void SampleEditor::playSample(const std::shared_ptr<H2Core::Sample> sample, bool original)
{
	const int selectedlayer = InstrumentEditorPanel::get_instance()->getSelectedLayer();
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	auto pInstr = pSong->getInstrumentList()->get( Hydrogen::get_instance()->getSelectedInstrumentNumber() );

	/*
	 *preview_instrument deletes the last used preview instrument, therefore we have to construct a temporary
	 *instrument. Otherwise pInstr would be deleted if consumed by preview_instrument.
	*/
	auto pTmpInstrument = Instrument::load_instrument( pInstr->get_drumkit_name(), pInstr->get_name() );
	auto pNewSample = std::make_shared<H2Core::Sample>(sample);

	if ( pNewSample == nullptr ) {
		ERRORLOG("no sample to play");
		return;
	}
	if ( ! m_pPositionsRulerPath || m_bPlayingOriginalSample != original ) {
		createPositionsRulerPath(pNewSample, original);
	}
	// m_nSlframes = pNewSample->get_frames();
	int length = ( ( pNewSample->get_frames() / pNewSample->get_sample_rate() + 1) * 100 );
	Hydrogen::get_instance()->getAudioEngine()->getSampler()->preview_instrument( pTmpInstrument );
	Hydrogen::get_instance()->getAudioEngine()->getSampler()->preview_sample( pNewSample, length );
	unsigned long realtime_play_start = Hydrogen::get_instance()->getRealtimeFrames();;

	m_pMainSampleWaveDisplay->paintLocatorEvent( original ? 24 : StartFrameSpinBox->value() / m_divider + 24 , true);
	m_pSampleAdjustView->setDetailSamplePosition( original ? 0 : __loops.start_frame, m_fZoomfactor , nullptr);
	m_nRealtimeFrameEnd = realtime_play_start + m_nSlframes;

	if ( original ) {
		m_nRealtimeFrameEnd = realtime_play_start + pNewSample->get_frames();
		m_nOriginalFrames = pNewSample->get_frames();
	}
   //calculate the new rubberband sample length
	if( __rubberband.use ){
		m_nRealtimeFrameEndForTarget = realtime_play_start + (m_nSlframes * m_Ratio + 0.1);
	}else
	{
		m_nRealtimeFrameEndForTarget = m_nRealtimeFrameEnd;
    }
	if ( original || ! __rubberband.use || std::abs(m_Ratio - 1.0 ) < 0.000001) {
		m_pTimer->start(40);	// update ruler at 25 fps
	}
    if ( ! original ) m_pTargetDisplayTimer->start(40);       // update ruler at 25 fps
}

void SampleEditor::updateMainsamplePositionRuler()
{
	unsigned long realpos = Hydrogen::get_instance()->getRealtimeFrames();
	if ( realpos < m_nRealtimeFrameEnd ){
		unsigned total_frames = m_bPlayingOriginalSample ? m_nOriginalFrames : m_nSlframes;

		unsigned frame = total_frames - ( m_nRealtimeFrameEnd  - realpos );

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
		m_pTargetDisplayTimer->stop();
		PlayPushButton->setText( QString( tr( "&Play" )) );
		PlayOrigPushButton->setText( QString( tr( "P&lay original sample" ) ) );
		m_bPlayButton = false;
	}
}

void SampleEditor::updateTargetsamplePositionRuler()
{
	unsigned long realpos = Hydrogen::get_instance()->getRealtimeFrames();
	unsigned targetSampleLength;
	if( __rubberband.use ){
		targetSampleLength =  m_nSlframes * m_Ratio + 0.1;
	}else
	{
		targetSampleLength =  m_nSlframes;
	}
	if ( realpos < m_nRealtimeFrameEndForTarget ){
		unsigned pos = targetSampleLength - ( m_nRealtimeFrameEndForTarget - realpos );
		m_pTargetSampleView->paintLocatorEventTargetDisplay( (m_pTargetSampleView->width() * pos /targetSampleLength), true);
		// ERRORLOG( QString("sampleval: %1").arg(frame) );
	}else
	{
		m_pTargetSampleView->paintLocatorEventTargetDisplay( -1 , false);
		m_pTargetDisplayTimer->stop();
		PlayPushButton->setText(QString( tr( "&Play" )) );
		PlayOrigPushButton->setText( QString( tr( "P&lay original sample" ) ) );
		m_bPlayButton = false;
	}
}


void SampleEditor::resetPositionsRulerPath()
{
	if ( !m_bPlayingOriginalSample && m_pPositionsRulerPath ) {
		delete[] m_pPositionsRulerPath;
		m_pPositionsRulerPath = nullptr;
	}
}
void SampleEditor::createPositionsRulerPath(const std::shared_ptr<Sample> sample, bool original)
{
	qWarning()<< "createPositionsRulerPath: " << original;
	setSamplelengthFrames();
	unsigned  normalLength = m_pSampleFromFile->get_frames();

	unsigned oneSampleLength =  original ? normalLength : __loops.end_frame - __loops.start_frame;
	unsigned loopLength =  __loops.end_frame - __loops.loop_frame;
	unsigned repeatsLength = loopLength * __loops.count;
	unsigned newLength = oneSampleLength + repeatsLength;


	// if (original) {
	// 	oneSampleLength = newLength = normalLength;
	// 	loopLength = repeatsLength = 0;
	// }
	unsigned *	normalFrames = new unsigned[ normalLength ];
	unsigned *	tempFrames = new unsigned[ newLength ];
	unsigned *	loopFrames = new unsigned[ loopLength ];

	for ( unsigned i = 0; i < normalLength; i++ ) {
		normalFrames[i] = i;
	}
	assert (original || m_nSlframes == newLength);

	Sample::Loops::LoopMode loopmode = __loops.mode;

	for ( unsigned i = 0; i < newLength; i++){ //first vector
		tempFrames[i] = 0;
	}

	long int y = original ? 0 : __loops.start_frame;
	for ( unsigned i = 0; i < oneSampleLength; i++, y++){ //first vector

		tempFrames[i] = normalFrames[y];
	}
	if (! original ) {
		long int z = __loops.loop_frame;
		for ( unsigned i = 0; i < loopLength; i++, z++){ //loop vector

			loopFrames[i] = normalFrames[z];
		}

		if ( loopmode == Sample::Loops::REVERSE ) {

			std::reverse(loopFrames, loopFrames + loopLength);

			if ( __loops.count > 0 && __loops.start_frame == __loops.loop_frame ){
				std::reverse( tempFrames, tempFrames + oneSampleLength );
			}

		} else if ( loopmode == Sample::Loops::PINGPONG &&  __loops.start_frame == __loops.loop_frame){
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
	}

	if(m_pPositionsRulerPath)
	{
		delete[] m_pPositionsRulerPath;
	}

	m_pPositionsRulerPath = tempFrames;
	m_bPlayingOriginalSample = original;

	delete[] loopFrames;
	delete[] normalFrames;
}



void SampleEditor::setSamplelengthFrames()
{
	getAllLocalFrameInfos();
	unsigned oneSampleLength =  __loops.end_frame - __loops.start_frame;
	unsigned loopLength =  __loops.end_frame - __loops.loop_frame ;
	unsigned repeatsLength = loopLength * __loops.count;
	unsigned newLength = oneSampleLength + repeatsLength;

	m_nSlframes = newLength;
	newlengthLabel->setText(QString( tr( "new sample length" ) )
							.append( QString( ": %1 " ).arg(newLength) )
							.append( tr( "frames" )));
	if ( rubberbandIsOff() ) {
		__rubberband.divider == computeNoopRubberbandDivider();
	}
	checkRatioSettings();
}

/////////////////////////////////////////////////////////////////
// Rubberband Controls
/////////////////////////////////////////////////////////////////

void SampleEditor::valueChangedrubberbandCsettingscomboBox( const QString  )
{
	int new_settings = rubberbandCsettingscomboBox->currentIndex();
	bool change = (new_settings != __rubberband.c_settings);

	if ( ! change && ! m_bAdjusting ) testpTimer();
	__rubberband.c_settings = new_settings;
	if ( change ) setUnclean();
	doneEditing();
}



void SampleEditor::valueChangedpitchdoubleSpinBox( double )
{
	double new_value = pitchdoubleSpinBox->value();
	double old_value = __rubberband.pitch;
	bool change = (std::abs(new_value - old_value) >= 0.0001);
	if ( change ) {
		bool used = __rubberband.use;
		__rubberband.pitch = new_value;
		__rubberband.use = !rubberbandIsOff() || std::abs(__rubberband.pitch) >= 0.0001;
		if (!used && __rubberband.use) {
			valueChangedrubberComboBox("");
		}
		if (__rubberband.use && rubberbandIsOff()) {
			__rubberband.divider == computeNoopRubberbandDivider();
		}
		setUnclean();
	}
	doneEditing();
}


void SampleEditor::valueChangedrubberComboBox( const QString  )
{
	bool current_usage = __rubberband.use;
	float current_divider = __rubberband.divider;
	if( rubberComboBox->currentText() != "off" ){
		__rubberband.use = true;
	}else
	{
		__rubberband.use = std::abs(__rubberband.pitch) >= 0.0001;
	}


	switch ( rubberComboBox->currentIndex() ){
	case 0 ://
		__rubberband.divider = computeNoopRubberbandDivider();
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
	bool change = (current_usage != __rubberband.use) || (std::abs(current_divider - __rubberband.divider) >= 0.0001);
	if ( change ) setUnclean();
	doneEditing();
}

double SampleEditor::computeNoopRubberbandDivider()
{
	double bpm = Hydrogen::get_instance()->getNewBpmJTM();
	double duration = (double) m_nSlframes / (double) m_nSamplerate;
	return duration * bpm / 60;
}

double SampleEditor::computeCurrentRatio()
{
	double durationtime = 60.0 / Hydrogen::get_instance()->getNewBpmJTM() * __rubberband.divider;
	double induration = (double) m_nSlframes / (double) m_nSamplerate;
	if (induration == 0.0)
		return -1;
	double ratio = durationtime / induration;
	return ratio;
}

void SampleEditor::checkRatioSettings()
{
	//calculate ratio
	m_Ratio = computeCurrentRatio();

	//my personal ratio quality settings
	//ratios < 0.1 || > 3.0 are bad (red) or experimental sounds
	//ratios > 0.1 && < 0.5 || > 2.0 && < 3.0 are mediocre (yellow)
	//ratios > 0.5 && < 2.0 are good (green)
	//
	//         0.1        0.5               2.0            3.0
	//<---red---[--yellow--[------green------]----yellow----]---red--->

	//green ratio
	if( ( m_Ratio >= 0.5 ) && ( m_Ratio <= 2.0 ) ){
		rubberComboBox->setStyleSheet("QComboBox { background-color: green; }");
	}
	//yellow ratio
	else if( ( m_Ratio >= 0.1 ) && ( m_Ratio <=  3.0 ) ){
		rubberComboBox->setStyleSheet("QComboBox { background-color: yellow; }");
	}
	//red ratio
	else{
		rubberComboBox->setStyleSheet("QComboBox { background-color: red; }");
	}
	QString text = QString( tr(" RB-Ratio" ) )
		.append( QString( " %1" ).arg( m_Ratio ) );
	ratiolabel->setText( text );

	//no rubberband = default
	if( !__rubberband.use ){
		rubberComboBox->setStyleSheet("QComboBox { background-color: 58, 62, 72; }");
		ratiolabel->setText( "" );
	}
}


void SampleEditor::valueChangedEditTypeComboBox( int index )
{
	if ( ! m_pTargetSampleView ) {
		// qWarning() << "no TargetSampleView!";
		return;
	}
	switch ( index ){
		case 0 ://
			m_pTargetSampleView->setEditMode( SampleEditor::EnvelopeType::VelocityEnvelope );
			break;
		case 1 ://
			if ( m_pTargetSampleView ) {
				m_pTargetSampleView->setEditMode( SampleEditor::EnvelopeType::PanEnvelope );
			}
			break;
		default:
			if ( m_pTargetSampleView ) {
				m_pTargetSampleView->setEditMode( SampleEditor::EnvelopeType::NoEnvelope );
			}
	}
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
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1 , false);
		m_pTimer->stop();
		m_pTargetDisplayTimer->stop();
		PlayPushButton->setText( QString( tr( "&Play" ) ) );
		PlayOrigPushButton->setText( QString( tr( "P&lay original sample" ) ) );
		Hydrogen::get_instance()->getAudioEngine()->getSampler()->stopPlayingNotes();
		m_bPlayButton = false;
	}
}
