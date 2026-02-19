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

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "DetailWaveDisplay.h"
#include "MainSampleWaveDisplay.h"
#include "TargetWaveDisplay.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Note.h>
#include <core/Basics/Sample.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#include <QMessageBox>
#include <QModelIndex>
#include <QTreeWidget>
#include <algorithm>

using namespace H2Core;

QString SampleEditor::SliderToQString( const Slider& slider )
{
	switch ( slider ) {
		case Slider::Start:
			return "Start";
		case Slider::Loop:
			return "Loop";
		case Slider::End:
			return "End";
		case Slider::None:
		default:
			return "None";
	}
}

SampleEditor::SampleEditor(
	QWidget* pParent,
	std::shared_ptr<H2Core::InstrumentLayer> pLayer,
	std::shared_ptr<H2Core::InstrumentComponent> pComponent,
	std::shared_ptr<H2Core::Instrument> pInstrument
)
	: QDialog( pParent ),
	  m_pLayer( pLayer ),
	  m_pComponent( pComponent ),
	  m_pInstrument( pInstrument ),
	  m_fZoomfactor( 1.0 ),
	  m_nFramePosition( 0 ),
	  m_selectedSlider( Slider::Start ),
	  m_bOnewayStart( false ),
	  m_bOnewayLoop( false ),
	  m_bOnewayEnd( false ),
	  m_bPlayButton( false ),
	  m_bAdjusting( false ),
	  m_bSampleEditorClean( true ),
	  m_nSlframes( 0 ),
	  m_pPositionsRulerPath( nullptr ),
	  m_fRatio( 1.0f )
{
	if ( pInstrument == nullptr || pComponent == nullptr || pLayer == nullptr ||
		 pLayer->getSample() == nullptr ) {
		reject();
	}
	m_pSample = pLayer->getSample();

	setFixedSize( SampleEditor::nWidth, SampleEditor::nHeight );
	setModal( true );
	setWindowTitle( QString( tr( "SampleEditor " ) + m_pSample->getFilePath() )
	);

	const auto nFrames = m_pSample->getFrames();

	m_fDivider = m_pSample->getFrames() / 574.0F;

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
    const auto pPref = Preferences::get_instance();
    const auto separatorColor = pPref->getColorTheme()->m_windowColor.darker( 135 );

	auto pMainLayout = new QVBoxLayout();
	pMainLayout->setContentsMargins( 0, 0, 0, 0 );
	pMainLayout->setSpacing( 0 );
	setLayout( pMainLayout );

	auto pScrollArea = new QScrollArea( this );
	pScrollArea->setWidgetResizable( true );
	pScrollArea->setMinimumSize(
		SampleEditor::nWidth - 2, SampleEditor::nHeight - 2
	);
	pScrollArea->resize( SampleEditor::nWidth - 2, SampleEditor::nHeight - 2 );
	pMainLayout->addWidget( pScrollArea );

	auto pGridLayout = new QGridLayout();
	pGridLayout->setContentsMargins( 9, 0, 9, 0 );
	pGridLayout->setSpacing( 6 );
    pScrollArea->setLayout( pGridLayout );

	auto createSeparator = [&]( QWidget* pParent ) {
		auto pSeparator = new QWidget( pParent );
        pSeparator->setFixedHeight( 1 );
		pSeparator->setStyleSheet( QString( "\
background-color: %1;" ).arg( separatorColor.name() ) );

		return pSeparator;
	};

	////////////////////////////////////////////////////////////////////////////

	auto pWaveDisplayContainer = new QWidget( pScrollArea );
	pGridLayout->addWidget( pWaveDisplayContainer, 0, 0, 1, 2 );

	auto pWaveDisplayContainerLayout = new QHBoxLayout();
	pWaveDisplayContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pWaveDisplayContainerLayout->setSpacing( 6 );
	pWaveDisplayContainer->setLayout( pWaveDisplayContainerLayout );

	m_pMainSampleWaveDisplay =
		new MainSampleWaveDisplay( pWaveDisplayContainer );
	m_pMainSampleWaveDisplay->updateDisplay( m_pSample );
	pWaveDisplayContainerLayout->addWidget( m_pMainSampleWaveDisplay );

	auto pDetailSection = new QWidget( pWaveDisplayContainer );
	pWaveDisplayContainerLayout->addWidget( pDetailSection );

	auto pDetailSectionLayout = new QVBoxLayout();
	pDetailSectionLayout->setContentsMargins( 0, 0, 0, 0 );
	pDetailSectionLayout->setSpacing( 0 );
	m_pDetailWaveDisplayL =
		new DetailWaveDisplay( this, DetailWaveDisplay::Channel::Left );
	m_pDetailWaveDisplayL->setLayer( m_pLayer );
	pDetailSectionLayout->addWidget( m_pDetailWaveDisplayL );
	m_pDetailWaveDisplayR =
		new DetailWaveDisplay( this, DetailWaveDisplay::Channel::Right );
	m_pDetailWaveDisplayR->setLayer( m_pLayer );
	pDetailSectionLayout->addWidget( m_pDetailWaveDisplayR );
	pDetailSection->setLayout( pDetailSectionLayout );

	auto pZoomSlider = new QSlider( pWaveDisplayContainer );
	pZoomSlider->setMaximumHeight( 265 );
	pZoomSlider->setValue( 1 );
	pZoomSlider->setOrientation( Qt::Vertical );
	connect( pZoomSlider, &QSlider::valueChanged, [=]() {
		m_fZoomfactor = pZoomSlider->value() / 10 + 1;
		m_pDetailWaveDisplayL->setZoomFactor( m_fZoomfactor );
		m_pDetailWaveDisplayR->setZoomFactor( m_fZoomfactor );
	} );
	pWaveDisplayContainerLayout->addWidget( pZoomSlider );

	////////////////////////////////////////////////////////////////////////////

	auto pSpinBoxContainer = new QWidget( pScrollArea );
	pGridLayout->addWidget( pSpinBoxContainer, 1, 0 );

	auto pSpinBoxContainerLayout = new QHBoxLayout();
    pSpinBoxContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pSpinBoxContainer->setLayout( pSpinBoxContainerLayout );

	auto pStartFrameLabel = new QLabel( pSpinBoxContainer );
	pStartFrameLabel->setText( tr( "Start" ) );
	pSpinBoxContainerLayout->addWidget( pStartFrameLabel );

	m_pStartFrameSpinBox = new QSpinBox( pSpinBoxContainer );
	m_pStartFrameSpinBox->setMinimumWidth( 100 );
	m_pStartFrameSpinBox->setToolTip( tr( "Adjust sample start frame" ) );
	m_pStartFrameSpinBox->setRange( 0, nFrames );
	connect(
		m_pStartFrameSpinBox, SIGNAL( valueChanged( int ) ), this,
		SLOT( valueChangedStartFrameSpinBox( int ) )
	);
	pSpinBoxContainerLayout->addWidget( m_pStartFrameSpinBox );

	pSpinBoxContainerLayout->addSpacerItem( new QSpacerItem( 40, 20 ) );

	auto pLoopFrameLabel = new QLabel( pSpinBoxContainer );
	pLoopFrameLabel->setText( tr( "Loop" ) );
	pSpinBoxContainerLayout->addWidget( pLoopFrameLabel );

	m_pLoopFrameSpinBox = new QSpinBox( pSpinBoxContainer );
	m_pLoopFrameSpinBox->setMinimumWidth( 100 );
	m_pLoopFrameSpinBox->setToolTip( tr( "Adjust sample loop begin frame" ) );
	m_pLoopFrameSpinBox->setRange( 0, nFrames );
	connect(
		m_pLoopFrameSpinBox, SIGNAL( valueChanged( int ) ), this,
		SLOT( valueChangedLoopFrameSpinBox( int ) )
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopFrameSpinBox );

	auto pLoopModeLabel = new QLabel( pSpinBoxContainer );
	pLoopModeLabel->setText( tr( "mode" ) );
	pSpinBoxContainerLayout->addWidget( pLoopModeLabel );

	m_pLoopModeComboBox = new QComboBox( pSpinBoxContainer );
	m_pLoopModeComboBox->setMinimumWidth( 80 );
	m_pLoopModeComboBox->setToolTip( tr( "set processing" ) );
	m_pLoopModeComboBox->addItems(
		QStringList() << tr( "forward" ) << tr( "reverse" ) << tr( "pingpong" )
	);
	connect(
		m_pLoopModeComboBox, SIGNAL( currentIndexChanged( int ) ), this,
		SLOT( valueChangedProcessingTypeComboBox( int ) )
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopModeComboBox );

	auto pLoopCountLabel = new QLabel( pSpinBoxContainer );
	pLoopCountLabel->setText( tr( "count" ) );
	pSpinBoxContainerLayout->addWidget( pLoopCountLabel );

	m_pLoopCountSpinBox = new QSpinBox( pSpinBoxContainer );
	m_pLoopCountSpinBox->setMinimumWidth( 60 );
	m_pLoopCountSpinBox->setToolTip( tr( "loops" ) );
	m_pLoopCountSpinBox->setRange( 0, 20000 );
	connect(
		m_pLoopCountSpinBox, SIGNAL( valueChanged( int ) ), this,
		SLOT( valueChangedLoopCountSpinBox( int ) )
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopCountSpinBox );

	pSpinBoxContainerLayout->addSpacerItem( new QSpacerItem( 40, 20 ) );

	auto pEndFrameLabel = new QLabel( pSpinBoxContainer );
	pEndFrameLabel->setText( tr( "End" ) );
	pSpinBoxContainerLayout->addWidget( pEndFrameLabel );

	m_pEndFrameSpinBox = new QSpinBox( pSpinBoxContainer );
	m_pEndFrameSpinBox->setMinimumWidth( 100 );
	m_pEndFrameSpinBox->setToolTip( tr( "Adjust sample and loop end frame" ) );
	m_pEndFrameSpinBox->setRange( 0, nFrames );
	m_pEndFrameSpinBox->setValue( nFrames );
	connect(
		m_pEndFrameSpinBox, SIGNAL( valueChanged( int ) ), this,
		SLOT( valueChangedEndFrameSpinBox( int ) )
	);
	pSpinBoxContainerLayout->addWidget( m_pEndFrameSpinBox );

	////////////////////////////////////////////////////////////////////////////

	auto pCloseButton = new QPushButton( pScrollArea );
	pCloseButton->setText( tr( "&Close" ) );
	connect( pCloseButton, &QPushButton::clicked, [=]() {
		if ( !m_bSampleEditorClean ) {
			auto pCommonStrings =
				HydrogenApp::get_instance()->getCommonStrings();
			if ( QMessageBox::information(
					 this, "Hydrogen", pCommonStrings->getUnsavedChanges(),
					 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
				 ) == QMessageBox::Ok ) {
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
	} );
	pGridLayout->addWidget( pCloseButton, 1, 1 );

	////////////////////////////////////////////////////////////////////////////

	auto pRubberBandContainer = new QWidget( pScrollArea );
	pGridLayout->addWidget( pRubberBandContainer, 2, 0, 1, 2 );

	auto pRubberBandContainerLayout = new QVBoxLayout();
	pRubberBandContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pRubberBandContainer->setLayout( pRubberBandContainerLayout );

    auto pRubberBandSeparator = createSeparator( pRubberBandContainer );
    pRubberBandContainerLayout->addWidget( pRubberBandSeparator );

	auto pRubberBandHeadingLabel = new QLabel( pRubberBandContainer );
	pRubberBandHeadingLabel->setText(
		tr( "Rubberband Audio Processor: Change the tempo (sample length) and "
			"pitch of audio." )
	);
	pRubberBandContainerLayout->addWidget( pRubberBandHeadingLabel );

	auto pRubberBandWidgetContainer = new QWidget( pRubberBandContainer );
	pRubberBandContainerLayout->addWidget( pRubberBandWidgetContainer );

	auto pRubberBandWidgetContainerLayout = new QHBoxLayout();
    pRubberBandWidgetContainer->setLayout( pRubberBandWidgetContainerLayout );

	auto pRubberBandLengthLabel = new QLabel( pRubberBandWidgetContainer );
	pRubberBandLengthLabel->setText( tr( "Sample length to beat:" ) );
	pRubberBandWidgetContainerLayout->addWidget( pRubberBandLengthLabel );

	m_pRubberBandLengthComboBox = new QComboBox( pRubberBandWidgetContainer );
	m_pRubberBandLengthComboBox->setMaximumWidth( 75 );
	QStringList rubberBandOptions;
	rubberBandOptions << pCommonStrings->getStatusOff() << "1/64"
					  << "1/32"
					  << "1/16"
					  << "1/8"
					  << "1/4"
					  << "1/2";
	for ( int ii = 1; ii <= 32; ++ii ) {
		rubberBandOptions << QString::number( ii );
	}
	m_pRubberBandLengthComboBox->addItems( rubberBandOptions );
	m_pRubberBandLengthComboBox->setCurrentIndex( 0 );
	connect(
		m_pRubberBandLengthComboBox, SIGNAL( currentIndexChanged( int ) ), this,
		SLOT( valueChangedrubberComboBox( int ) )
	);
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandLengthComboBox );

	m_pRubberBandRatioLabel = new QLabel( pRubberBandWidgetContainer );
	m_pRubberBandRatioLabel->setMinimumWidth( 150 );
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandRatioLabel );

	auto pRubberBandPitchLabel = new QLabel( pRubberBandWidgetContainer );
	pRubberBandPitchLabel->setMinimumWidth( 75 );
	pRubberBandPitchLabel->setText( tr( "Pitch (Semitone,Cent)" ) );
	pRubberBandWidgetContainerLayout->addWidget( pRubberBandPitchLabel );

	m_pRubberBandPitchSpinBox = new QDoubleSpinBox( pRubberBandWidgetContainer );
	m_pRubberBandPitchSpinBox->setMaximumWidth( 74 );
	m_pRubberBandPitchSpinBox->setToolTip(
		tr( "Pitch the sample in semitones, cents" )
	);
	m_pRubberBandPitchSpinBox->setMinimum( -36 );
	m_pRubberBandPitchSpinBox->setMaximum( 36 );
	m_pRubberBandPitchSpinBox->setSingleStep( 0.01 );
	// Make things consistent with the LCDDisplay and LCDSpinBox classes.
	m_pRubberBandPitchSpinBox->setLocale(
		QLocale( QLocale::C, QLocale::AnyCountry )
	);
	connect(
		m_pRubberBandPitchSpinBox, SIGNAL( valueChanged( double ) ), this,
		SLOT( valueChangedpitchdoubleSpinBox( double ) )
	);
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandPitchSpinBox );

	pRubberBandWidgetContainerLayout->addSpacerItem( new QSpacerItem( 40, 20 ) );

	auto pRubberBandCrispnessLabel = new QLabel( pRubberBandWidgetContainer );
	pRubberBandCrispnessLabel->setMinimumWidth( 100 );
	pRubberBandCrispnessLabel->setText( tr( "Crispness: " ) );
	pRubberBandWidgetContainerLayout->addWidget( pRubberBandCrispnessLabel );

	m_pRubberBandCrispnessComboBox = new QComboBox( pRubberBandWidgetContainer );
	m_pRubberBandCrispnessComboBox->setMaximumWidth( 45 );
	m_pRubberBandCrispnessComboBox->setToolTip(
		"http://www.breakfastquay.com/rubberband/"
	);
	QStringList crispnessOptions;
	for ( int ii = 1; ii <= 5; ++ii ) {
		crispnessOptions << QString::number( ii );
	}
	m_pRubberBandCrispnessComboBox->addItems( crispnessOptions );
	m_pRubberBandCrispnessComboBox->setCurrentIndex( 4 );
	connect(
		m_pRubberBandCrispnessComboBox, SIGNAL( currentIndexChanged( int ) ),
		this, SLOT( valueChangedrubberbandCsettingscomboBox( int ) )
	);
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandCrispnessComboBox );

	////////////////////////////////////////////////////////////////////////////

    auto pButtonSeparator = createSeparator( pScrollArea );
    pGridLayout->addWidget( pButtonSeparator, 3, 0, 1, 2 );

	////////////////////////////////////////////////////////////////////////////

	auto pButtonContainer = new QWidget( pScrollArea );
	pGridLayout->addWidget( pButtonContainer, 4, 0, 1, 2 );

	auto pButtonContainerLayout = new QHBoxLayout();
    pButtonContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pButtonContainer->setLayout( pButtonContainerLayout );

	m_pApplyButton = new QPushButton( pButtonContainer );
	m_pApplyButton->setText( pCommonStrings->getButtonApply() );
	connect( m_pApplyButton, &QPushButton::clicked, [&]() {
		QApplication::setOverrideCursor( Qt::WaitCursor );
		getAllLocalFrameInfos();
		createNewLayer();
		setClean();
		Hydrogen::get_instance()->setIsModified( true );
		QApplication::restoreOverrideCursor();
	} );
	pButtonContainerLayout->addWidget( m_pApplyButton );

	pButtonContainerLayout->addSpacerItem( new QSpacerItem( 13, 20 ) );

	m_pPlayButton = new QPushButton( pButtonContainer );
	m_pPlayButton->setText( pCommonStrings->getButtonPlay() );
	connect(
		m_pPlayButton, SIGNAL( clicked() ), this,
		SLOT( on_PlayPushButton_clicked() )
	);
	pButtonContainerLayout->addWidget( m_pPlayButton );

	pButtonContainerLayout->addSpacerItem( new QSpacerItem( 13, 20 ) );

	m_pPlayOriginalButton = new QPushButton( pButtonContainer );
	m_pPlayOriginalButton->setText( pCommonStrings->getButtonPlayOriginalSample(
	) );
	connect(
		m_pPlayOriginalButton, SIGNAL( clicked() ), this,
		SLOT( on_PlayOrigPushButton_clicked() )
	);
	pButtonContainerLayout->addWidget( m_pPlayOriginalButton );

	m_pNewLengthLabel = new QLabel( pButtonContainer );
	m_pNewLengthLabel->setMinimumWidth( 300 );
	m_pNewLengthLabel->setText( tr( "new sample length:" ) );
	pButtonContainerLayout->addWidget( m_pNewLengthLabel );

	pButtonContainerLayout->addSpacerItem( new QSpacerItem( 40, 20 ) );

	m_pEnvelopeComboBox = new QComboBox( pButtonContainer );
	m_pEnvelopeComboBox->setMinimumWidth( 80 );
	m_pEnvelopeComboBox->setToolTip( tr( "fade-out type" ) );
	m_pEnvelopeComboBox->addItems(
		QStringList() << tr( "volume" ) << tr( "panorama" )
	);
	pButtonContainerLayout->addWidget( m_pEnvelopeComboBox );

	////////////////////////////////////////////////////////////////////////////

	m_pTargetSampleView = new TargetWaveDisplay( pScrollArea );
	m_pTargetSampleView->setMinimumHeight( 94 );
	pGridLayout->addWidget( m_pTargetSampleView, 5, 0, 1, 2 );

	////////////////////////////////////////////////////////////////////////////

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags(
		windowFlags() | Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint
	);

	m_pTimer = new QTimer( this );
	connect(
		m_pTimer, SIGNAL( timeout() ), this,
		SLOT( updateMainsamplePositionRuler() )
	);
	m_pTargetDisplayTimer = new QTimer( this );
	connect(
		m_pTargetDisplayTimer, SIGNAL( timeout() ), this,
		SLOT( updateTargetsamplePositionRuler() )
	);

	setClean();

	__rubberband.nCrispness = 4;
	__rubberband.bUse = false;
	__rubberband.fLengthInBeats = 1.0;
	__rubberband.fSemitonesToShift = 0.0;

	getAllFrameInfos();

#ifndef H2CORE_HAVE_RUBBERBAND
	if ( !Filesystem::file_executable(
			 pPref->m_sRubberBandCLIexecutable,
			 true /* silent */
		 ) ) {
		pRubberBandContainer->hide();
		setClean();
	}
#else
	pRubberBandContainer->show();
	setClean();
#endif
}

SampleEditor::~SampleEditor()
{
	m_pMainSampleWaveDisplay->close();
	delete m_pMainSampleWaveDisplay;
	m_pMainSampleWaveDisplay = nullptr;

	m_pTargetSampleView->close();
	delete m_pTargetSampleView;
	m_pTargetSampleView = nullptr;

	INFOLOG( "DESTROY" );
}

int SampleEditor::getEnvelopeIndex() const {
    return m_pEnvelopeComboBox->currentIndex();
}

void SampleEditor::closeEvent( QCloseEvent* event )
{
	if ( !m_bSampleEditorClean ) {
		auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
		if ( QMessageBox::information(
				 this, "Hydrogen", pCommonStrings->getUnsavedChanges(),
				 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
			 ) == QMessageBox::Ok ) {
			setClean();
			accept();
		}
		else {
			event->ignore();
			return;
		}
	}
	else {
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
	if ( __loops.start_frame == 0 && __loops.loop_frame == 0 &&
		 __loops.end_frame == 0 ) {
		__loops.end_frame = m_pSample->getFrames();
	}
	__rubberband = m_pSample->getRubberband();

	if ( m_pSample->getVelocityEnvelope().size() == 0 ) {
		m_pTargetSampleView->get_velocity()->clear();
		m_pTargetSampleView->get_velocity()->push_back( EnvelopePoint( 0, 0 ) );
		m_pTargetSampleView->get_velocity()->push_back(
			EnvelopePoint( m_pTargetSampleView->width(), 0 )
		);
	}
	else {
		m_pTargetSampleView->get_velocity()->clear();

		for ( const auto& pt : m_pSample->getVelocityEnvelope() ) {
			m_pTargetSampleView->get_velocity()->emplace_back( pt );
		}
	}

	if ( m_pSample->getPanEnvelope().size() == 0 ) {
		m_pTargetSampleView->get_pan()->clear();
		m_pTargetSampleView->get_pan()->push_back(
			EnvelopePoint( 0, m_pTargetSampleView->height() / 2 )
		);
		m_pTargetSampleView->get_pan()->push_back( EnvelopePoint(
			m_pTargetSampleView->width(), m_pTargetSampleView->height() / 2
		) );
	}
	else {
		m_pTargetSampleView->get_pan()->clear();
		for ( const auto& pt : m_pSample->getPanEnvelope() ) {
			m_pTargetSampleView->get_pan()->emplace_back( pt );
		}
	}

	if ( m_bSampleIsModified ) {
		__loops.end_frame = m_pSample->getLoops().end_frame;
		if ( __loops.mode == Sample::Loops::FORWARD ) {
			m_pLoopModeComboBox->setCurrentIndex( 0 );
		}
		if ( __loops.mode == Sample::Loops::REVERSE ) {
			m_pLoopModeComboBox->setCurrentIndex( 1 );
		}
		if ( __loops.mode == Sample::Loops::PINGPONG ) {
			m_pLoopModeComboBox->setCurrentIndex( 2 );
		}

		m_pStartFrameSpinBox->setValue( __loops.start_frame );
		m_pLoopFrameSpinBox->setValue( __loops.loop_frame );
		m_pEndFrameSpinBox->setValue( __loops.end_frame );
		m_pLoopCountSpinBox->setValue( __loops.count );

		m_pMainSampleWaveDisplay->m_nStartFramePosition =
			__loops.start_frame / m_fDivider + 25;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_nLoopFramePosition =
			__loops.loop_frame / m_fDivider + 25;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pMainSampleWaveDisplay->m_nEndFramePosition =
			__loops.end_frame / m_fDivider + 25;
		m_pMainSampleWaveDisplay->updateDisplayPointer();

		if ( !__rubberband.bUse ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 0 );
		}

		m_pRubberBandCrispnessComboBox->setCurrentIndex( __rubberband.nCrispness
		);
		if ( !__rubberband.bUse ) {
			m_pRubberBandCrispnessComboBox->setCurrentIndex( 4 );
		}
		m_pRubberBandPitchSpinBox->setValue( __rubberband.fSemitonesToShift );
		if ( !__rubberband.bUse ) {
			m_pRubberBandPitchSpinBox->setValue( 0.0 );
		}

		if ( __rubberband.fLengthInBeats == 1.0 / 64.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 1 );
		}
		else if ( __rubberband.fLengthInBeats == 1.0 / 32.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 2 );
		}
		else if ( __rubberband.fLengthInBeats == 1.0 / 16.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 3 );
		}
		else if ( __rubberband.fLengthInBeats == 1.0 / 8.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 4 );
		}
		else if ( __rubberband.fLengthInBeats == 1.0 / 4.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 5 );
		}
		else if ( __rubberband.fLengthInBeats == 1.0 / 2.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 6 );
		}
		else if ( __rubberband.bUse && ( __rubberband.fLengthInBeats >= 1.0 ) ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( (int
			) ( __rubberband.fLengthInBeats + 6 ) );
		}

		setSamplelengthFrames();
		checkRatioSettings();
	}
	m_pTargetSampleView->updateDisplay( m_pLayer );
}

void SampleEditor::getAllLocalFrameInfos()
{
	__loops.start_frame = m_pStartFrameSpinBox->value();
	__loops.loop_frame = m_pLoopFrameSpinBox->value();
	__loops.count = m_pLoopCountSpinBox->value();
	__loops.end_frame = m_pEndFrameSpinBox->value();
}

bool SampleEditor::getCloseQuestion()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	return QMessageBox::information(
			   this, "Hydrogen", pCommonStrings->getUnsavedChanges(),
			   QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
		   ) == QMessageBox::Ok;
}

void SampleEditor::createNewLayer()
{
	if ( !m_bSampleEditorClean ) {
		auto pHydrogen = H2Core::Hydrogen::get_instance();
		auto pAudioEngine = pHydrogen->getAudioEngine();

		auto pEditSample = std::make_shared<Sample>(
			m_pSample->getFilePath(), m_pSample->getLicense()
		);
		pEditSample->setLoops( __loops );
		pEditSample->setRubberband( __rubberband );
		pEditSample->setVelocityEnvelope( *m_pTargetSampleView->get_velocity()
		);
		pEditSample->setPanEnvelope( *m_pTargetSampleView->get_pan() );

		if ( !pEditSample->load( pAudioEngine->getTransportPosition()->getBpm()
			 ) ) {
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

void SampleEditor::mouseReleaseEvent( QMouseEvent* ev )
{
}

bool SampleEditor::returnAllMainWaveDisplayValues()
{
	m_bAdjusting = true;

	testpTimer();
	m_bSampleIsModified = true;
	if ( m_pMainSampleWaveDisplay->m_bStartSliderIsMoved )
		__loops.start_frame =
			m_pMainSampleWaveDisplay->m_nStartFramePosition * m_fDivider -
			25 * m_fDivider;
	if ( m_pMainSampleWaveDisplay->m_bLoopSliderIsMoved )
		__loops.loop_frame =
			m_pMainSampleWaveDisplay->m_nLoopFramePosition * m_fDivider -
			25 * m_fDivider;
	if ( m_pMainSampleWaveDisplay->m_bEndSliderIsmoved )
		__loops.end_frame =
			m_pMainSampleWaveDisplay->m_nEndFramePosition * m_fDivider -
			25 * m_fDivider;
	m_pStartFrameSpinBox->setValue( __loops.start_frame );
	m_pLoopFrameSpinBox->setValue( __loops.loop_frame );
	m_pEndFrameSpinBox->setValue( __loops.end_frame );
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
	m_pApplyButton->setDisabled( false );
	m_pApplyButton->setFlat( false );
}

void SampleEditor::setClean()
{
	m_bSampleEditorClean = true;
	m_pApplyButton->setDisabled( true );
	m_pApplyButton->setFlat( true );
}

void SampleEditor::valueChangedStartFrameSpinBox( int )
{
	testpTimer();
	m_nFramePosition = m_pStartFrameSpinBox->value();
	if ( m_nFramePosition == __loops.start_frame ) {	// no actual change
		if ( !m_bAdjusting )
			on_PlayPushButton_clicked();
		return;
	}

	m_pDetailWaveDisplayL->setSlider( Slider::Start );
	m_pDetailWaveDisplayR->setSlider( Slider::Start );

	if ( !m_bOnewayStart ) {
		m_pMainSampleWaveDisplay->m_nStartFramePosition =
			m_pStartFrameSpinBox->value() / m_fDivider + 25;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pDetailWaveDisplayL->setPosition( m_nFramePosition );
		m_pDetailWaveDisplayR->setPosition( m_nFramePosition );
		__loops.start_frame = m_nFramePosition;
	}
	else {
		m_pDetailWaveDisplayL->setPosition( m_nFramePosition );
		m_pDetailWaveDisplayR->setPosition( m_nFramePosition );
		m_bOnewayStart = false;
	}
	testPositionsSpinBoxes();
	setUnclean();
	setSamplelengthFrames();
}

void SampleEditor::valueChangedLoopFrameSpinBox( int )
{
	testpTimer();
	m_nFramePosition = m_pLoopFrameSpinBox->value();
	if ( m_nFramePosition == __loops.loop_frame ) {
		if ( !m_bAdjusting )
			on_PlayPushButton_clicked();
		return;
	}

	m_pDetailWaveDisplayL->setSlider( Slider::Loop );
	m_pDetailWaveDisplayR->setSlider( Slider::Loop );

	if ( !m_bOnewayLoop ) {
		m_pMainSampleWaveDisplay->m_nLoopFramePosition =
			m_pLoopFrameSpinBox->value() / m_fDivider + 25;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pDetailWaveDisplayL->setPosition( m_nFramePosition );
		m_pDetailWaveDisplayR->setPosition( m_nFramePosition );
		__loops.loop_frame = m_nFramePosition;
	}
	else {
		m_pDetailWaveDisplayL->setPosition( m_nFramePosition );
		m_pDetailWaveDisplayR->setPosition( m_nFramePosition );
		m_bOnewayLoop = false;
	}
	testPositionsSpinBoxes();
	setUnclean();
	setSamplelengthFrames();
}

void SampleEditor::valueChangedEndFrameSpinBox( int )
{
	testpTimer();
	m_nFramePosition = m_pEndFrameSpinBox->value();
	if ( m_nFramePosition == __loops.end_frame ) {
		if ( !m_bAdjusting )
			on_PlayPushButton_clicked();
		return;
	}

	m_pDetailWaveDisplayL->setSlider( Slider::End );
	m_pDetailWaveDisplayR->setSlider( Slider::End );

	if ( !m_bOnewayEnd ) {
		m_pMainSampleWaveDisplay->m_nEndFramePosition =
			m_pEndFrameSpinBox->value() / m_fDivider + 25;
		m_pMainSampleWaveDisplay->updateDisplayPointer();
		m_pDetailWaveDisplayL->setPosition( m_nFramePosition );
		m_pDetailWaveDisplayR->setPosition( m_nFramePosition );
		__loops.end_frame = m_nFramePosition;
	}
	else {
		m_bOnewayEnd = false;
		m_pDetailWaveDisplayL->setPosition( m_nFramePosition );
		m_pDetailWaveDisplayR->setPosition( m_nFramePosition );
	}
	testPositionsSpinBoxes();
	setUnclean();
	setSamplelengthFrames();
}

void SampleEditor::on_PlayPushButton_clicked()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( m_pPlayButton->text() == "Stop" ) {
		testpTimer();
		return;
	}

	// Since we are in a separate dialog and working with a particular, we do
	// not want rendering to be affected by whether some instruments of the
	// current kit are soloed or muted.
	auto pPreviewInstr = std::make_shared<Instrument>( m_pInstrument );
	pPreviewInstr->setIsPreviewInstrument( true );

	auto pCompo =
		pPreviewInstr->getComponent( m_pInstrument->index( m_pComponent ) );
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
		pPreviewInstr, 0, pLayer->getEndVelocity() - 0.01
	);
	pNote->setSelectedLayerInfo( pSelectedLayerInfo, pCompo );

	pHydrogen->getAudioEngine()->getSampler()->noteOn( pNote );

	setSamplelengthFrames();
	createPositionsRulerPath();
	m_bPlayButton = true;
	m_pMainSampleWaveDisplay->paintLocatorEvent(
		m_pStartFrameSpinBox->value() / m_fDivider + 24, true
	);

	m_pDetailWaveDisplayL->setSlider( Slider::None );
	m_pDetailWaveDisplayR->setSlider( Slider::None );
	m_pDetailWaveDisplayL->setPosition( __loops.start_frame );
	m_pDetailWaveDisplayR->setPosition( __loops.start_frame );

	if ( __rubberband.bUse == false ) {
		m_pTimer->start( 40 );	// update ruler at 25 fps
	}

	m_nRealtimeFrameEnd = pAudioEngine->getRealtimeFrame() + m_nSlframes;

	// calculate the new rubberband sample length
	if ( __rubberband.bUse ) {
		m_nRealtimeFrameEndForTarget =
			pAudioEngine->getRealtimeFrame() + ( m_nSlframes * m_fRatio + 0.1 );
	}
	else {
		m_nRealtimeFrameEndForTarget = m_nRealtimeFrameEnd;
	}
	m_pTargetDisplayTimer->start( 40 );	 // update ruler at 25 fps
	m_pPlayButton->setText( QString( "Stop" ) );
}

void SampleEditor::on_PlayOrigPushButton_clicked()
{
	if ( m_pPlayOriginalButton->text() == "Stop" ) {
		testpTimer();
		return;
	}
	auto pHydrogen = Hydrogen::get_instance();
	auto tearDown = [&]() {
		m_pMainSampleWaveDisplay->paintLocatorEvent(
			m_pStartFrameSpinBox->value() / m_fDivider + 24, true
		);
		m_pDetailWaveDisplayL->setSlider( Slider::None );
		m_pDetailWaveDisplayR->setSlider( Slider::None );
		m_pDetailWaveDisplayL->setPosition( __loops.start_frame );
		m_pDetailWaveDisplayR->setPosition( __loops.start_frame );
		m_pTimer->start( 40 );	// update ruler at 25 fps
		m_nRealtimeFrameEnd =
			pHydrogen->getAudioEngine()->getRealtimeFrame() + m_nSlframes;
		m_pPlayOriginalButton->setText( QString( "Stop" ) );
	};

	// Construct a custom instrument containing the current settings -
	// instrument, component, and layer - but using the original sample.
	auto pPreviewInstr = std::make_shared<Instrument>( m_pInstrument );
	pPreviewInstr->setIsPreviewInstrument( true );

	auto pCompo =
		pPreviewInstr->getComponent( m_pInstrument->index( m_pComponent ) );
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
	const int nLength =
		( pNewSample->getFrames() / pNewSample->getSampleRate() + 1 ) * 100;
	auto pNote = std::make_shared<Note>(
		pPreviewInstr, 0, VELOCITY_MAX, PAN_DEFAULT, nLength
	);
	auto pSelectedLayerInfo = std::make_shared<SelectedLayerInfo>();
	pSelectedLayerInfo->pLayer = pLayer;
	pNote->setSelectedLayerInfo( pSelectedLayerInfo, pCompo );

	pHydrogen->getAudioEngine()->getSampler()->previewInstrument(
		pPreviewInstr, pNote
	);
	m_nSlframes = pNewSample->getFrames();

	tearDown();
}

void SampleEditor::updateMainsamplePositionRuler()
{
	unsigned long realpos =
		Hydrogen::get_instance()->getAudioEngine()->getRealtimeFrame();
	if ( realpos < m_nRealtimeFrameEnd ) {
		unsigned frame = m_nSlframes - ( m_nRealtimeFrameEnd - realpos );
		if ( m_bPlayButton == true ) {
			m_pMainSampleWaveDisplay->paintLocatorEvent(
				m_pPositionsRulerPath[frame] / m_fDivider + 25, true
			);
			m_pDetailWaveDisplayL->setPosition( m_pPositionsRulerPath[frame] );
			m_pDetailWaveDisplayR->setPosition( m_pPositionsRulerPath[frame] );
		}
		else {
			m_pMainSampleWaveDisplay->paintLocatorEvent(
				frame / m_fDivider + 25, true
			);
			m_pDetailWaveDisplayL->setPosition( frame );
			m_pDetailWaveDisplayR->setPosition( frame );
		}
	}
	else {
		auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1, false );
		m_pTimer->stop();
		m_pPlayButton->setText( pCommonString->getButtonPlay() );
		m_pPlayOriginalButton->setText(
			pCommonString->getButtonPlayOriginalSample()
		);
		m_bPlayButton = false;
	}
}

void SampleEditor::updateTargetsamplePositionRuler()
{
	unsigned long realpos =
		Hydrogen::get_instance()->getAudioEngine()->getRealtimeFrame();
	unsigned targetSampleLength;
	if ( __rubberband.bUse ) {
		targetSampleLength = m_nSlframes * m_fRatio + 0.1;
	}
	else {
		targetSampleLength = m_nSlframes;
	}

	if ( realpos < m_nRealtimeFrameEndForTarget ) {
		unsigned pos =
			targetSampleLength - ( m_nRealtimeFrameEndForTarget - realpos );
		m_pTargetSampleView->paintLocatorEventTargetDisplay(
			( m_pTargetSampleView->width() * pos / targetSampleLength ), true
		);
		//		ERRORLOG( QString("sampleval: %1").arg(frame) );
	}
	else {
		auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();
		m_pTargetSampleView->paintLocatorEventTargetDisplay( -1, false );
		m_pTargetDisplayTimer->stop();
		m_pPlayButton->setText( pCommonString->getButtonPlay() );
		m_pPlayOriginalButton->setText(
			pCommonString->getButtonPlayOriginalSample()
		);
		m_bPlayButton = false;
	}
}

void SampleEditor::createPositionsRulerPath()
{
	setSamplelengthFrames();

	unsigned oneSampleLength = __loops.end_frame - __loops.start_frame;
	unsigned loopLength = __loops.end_frame - __loops.loop_frame;
	unsigned repeatsLength = loopLength * __loops.count;
	unsigned newLength = 0;
	if ( oneSampleLength == loopLength ) {
		newLength = oneSampleLength + oneSampleLength * __loops.count;
	}
	else {
		newLength = oneSampleLength + repeatsLength;
	}

	unsigned normalLength = m_pSample->getFrames();

	unsigned* normalFrames = new unsigned[normalLength];
	unsigned* tempFrames = new unsigned[newLength];
	unsigned* loopFrames = new unsigned[loopLength];

	for ( unsigned i = 0; i < normalLength; i++ ) {
		normalFrames[i] = i;
	}

	Sample::Loops::LoopMode loopmode = __loops.mode;
	long int z = __loops.loop_frame;
	long int y = __loops.start_frame;

	for ( unsigned i = 0; i < newLength; i++ ) {  // first vector
		tempFrames[i] = 0;
	}

	for ( unsigned i = 0; i < oneSampleLength; i++, y++ ) {	 // first vector

		tempFrames[i] = normalFrames[y];
	}

	for ( unsigned i = 0; i < loopLength; i++, z++ ) {	// loop vector

		loopFrames[i] = normalFrames[z];
	}

	if ( loopmode == Sample::Loops::REVERSE ) {
		std::reverse( loopFrames, loopFrames + loopLength );
	}

	if ( loopmode == Sample::Loops::REVERSE && __loops.count > 0 &&
		 __loops.start_frame == __loops.loop_frame ) {
		std::reverse( tempFrames, tempFrames + oneSampleLength );
	}

	if ( loopmode == Sample::Loops::PINGPONG &&
		 __loops.start_frame == __loops.loop_frame ) {
		std::reverse( loopFrames, loopFrames + loopLength );
	}

	for ( int i = 0; i < __loops.count; i++ ) {
		unsigned tempdataend = oneSampleLength + ( loopLength * i );
		if ( __loops.start_frame == __loops.loop_frame ) {
			std::copy(
				loopFrames, loopFrames + loopLength, tempFrames + tempdataend
			);
		}
		if ( loopmode == Sample::Loops::PINGPONG && __loops.count > 1 ) {
			std::reverse( loopFrames, loopFrames + loopLength );
		}
		if ( __loops.start_frame != __loops.loop_frame ) {
			std::copy(
				loopFrames, loopFrames + loopLength, tempFrames + tempdataend
			);
		}
	}

	if ( __loops.count == 0 && loopmode == Sample::Loops::REVERSE ) {
		std::reverse( tempFrames + __loops.loop_frame, tempFrames + newLength );
	}

	if ( m_pPositionsRulerPath ) {
		delete[] m_pPositionsRulerPath;
	}

	m_pPositionsRulerPath = tempFrames;

	delete[] loopFrames;
	delete[] normalFrames;
}

void SampleEditor::setSamplelengthFrames()
{
	getAllLocalFrameInfos();
	unsigned oneSampleLength = __loops.end_frame - __loops.start_frame;
	unsigned loopLength = __loops.end_frame - __loops.loop_frame;
	unsigned repeatsLength = loopLength * __loops.count;
	unsigned newLength = 0;

	if ( oneSampleLength == loopLength ) {
		newLength = oneSampleLength + oneSampleLength * __loops.count;
	}
	else {
		newLength = oneSampleLength + repeatsLength;
	}

	m_nSlframes = newLength;
	m_pNewLengthLabel->setText( QString( tr( "new sample length" ) )
									.append( QString( ": %1 " ).arg( newLength )
									)
									.append( tr( "frames" ) ) );
	checkRatioSettings();
}

void SampleEditor::valueChangedLoopCountSpinBox( int )
{
	testpTimer();
	int count = m_pLoopCountSpinBox->value();

	if ( count == __loops.count ) {
		if ( !m_bAdjusting )
			on_PlayOrigPushButton_clicked();
		return;
	}

	const auto pHydrogen = Hydrogen::get_instance();
	const auto pAudioDriver = pHydrogen->getAudioDriver();
	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return;
	}

	if ( m_nSlframes > pAudioDriver->getSampleRate() * 60 ) {
		pHydrogen->getAudioEngine()->getSampler()->stopPlayingNotes();
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1, false );
		m_pTimer->stop();
		m_bPlayButton = false;
	}
	__loops.count = count;
	setUnclean();
	setSamplelengthFrames();
	if ( m_nSlframes > pAudioDriver->getSampleRate() * 60 * 30 ) {	// >30 min
		m_pLoopCountSpinBox->setMaximum( m_pLoopCountSpinBox->value() - 1 );
	}
}

void SampleEditor::valueChangedrubberbandCsettingscomboBox( int )
{
	const int nNewCrispness = m_pRubberBandCrispnessComboBox->currentIndex();
	if ( nNewCrispness == __rubberband.nCrispness ) {
		if ( !m_bAdjusting ) {
			on_PlayPushButton_clicked();
		}
		return;
	}
	__rubberband.nCrispness = nNewCrispness;
	setUnclean();
}

void SampleEditor::valueChangedpitchdoubleSpinBox( double )
{
	const double fNewValue = m_pRubberBandPitchSpinBox->value();
	if ( std::abs( fNewValue - __rubberband.fSemitonesToShift ) < 0.0001 ) {
		if ( !m_bAdjusting ) {
			on_PlayPushButton_clicked();
		}
		return;
	}
	__rubberband.fSemitonesToShift = fNewValue;
	setUnclean();
}

void SampleEditor::valueChangedrubberComboBox( int )
{
	if ( m_pRubberBandLengthComboBox->currentText() != "off" ) {
		__rubberband.bUse = true;
	}
	else {
		__rubberband.bUse = false;
		__rubberband.fLengthInBeats = 1.0;
	}

	switch ( m_pRubberBandLengthComboBox->currentIndex() ) {
		case 0:	 //
			__rubberband.fLengthInBeats = 4.0;
			break;
		case 1:	 //
			__rubberband.fLengthInBeats = 1.0 / 64.0;
			break;
		case 2:	 //
			__rubberband.fLengthInBeats = 1.0 / 32.0;
			break;
		case 3:	 //
			__rubberband.fLengthInBeats = 1.0 / 16.0;
			break;
		case 4:	 //
			__rubberband.fLengthInBeats = 1.0 / 8.0;
			break;
		case 5:	 //
			__rubberband.fLengthInBeats = 1.0 / 4.0;
			break;
		case 6:	 //
			__rubberband.fLengthInBeats = 1.0 / 2.0;
			break;
		case 7:	 //
			__rubberband.fLengthInBeats = 1.0;
			break;
		default:
			__rubberband.fLengthInBeats =
				(float) m_pRubberBandLengthComboBox->currentIndex() - 6.0;
	}
	//	QMessageBox::information ( this, "Hydrogen", tr ( "divider %1" ).arg(
	//__rubberband.divider )); 	float __rubberband.divider;
	setSamplelengthFrames();

	setUnclean();
}

void SampleEditor::checkRatioSettings()
{
	// calculate ratio
	double durationtime = 60.0 /
						  Hydrogen::get_instance()
							  ->getAudioEngine()
							  ->getTransportPosition()
							  ->getBpm() *
						  __rubberband.fLengthInBeats;
	double induration = (double) m_nSlframes / (double) m_nSamplerate;
	if ( induration != 0.0 )
		m_fRatio = durationtime / induration;

	// my personal ratio quality settings
	// ratios < 0.1 || > 3.0 are bad (red) or experimental sounds
	// ratios > 0.1 && < 0.5 || > 2.0 && < 3.0 are mediocre (yellow)
	// ratios > 0.5 && < 2.0 are good (green)
	//
	//          0.1        0.5               2.0            3.0
	//<---red---[--yellow--[------green------]----yellow----]---red--->

	// green ratio
	if ( ( m_fRatio >= 0.5 ) && ( m_fRatio <= 2.0 ) ) {
		m_pRubberBandLengthComboBox->setStyleSheet(
			"QComboBox { background-color: green; }"
		);
	}
	// yellow ratio
	else if ( ( m_fRatio >= 0.1 ) && ( m_fRatio <= 3.0 ) ) {
		m_pRubberBandLengthComboBox->setStyleSheet(
			"QComboBox { background-color: yellow; }"
		);
	}
	// red ratio
	else {
		m_pRubberBandLengthComboBox->setStyleSheet(
			"QComboBox { background-color: red; }"
		);
	}
	QString text =
		QString( tr( " RB-Ratio" ) ).append( QString( " %1" ).arg( m_fRatio ) );
	m_pRubberBandRatioLabel->setText( text );

	// no rubberband = default
	if ( !__rubberband.bUse ) {
		m_pRubberBandLengthComboBox->setStyleSheet(
			"QComboBox { background-color: 58, 62, 72; }"
		);
		m_pRubberBandRatioLabel->setText( "" );
	}
}

void SampleEditor::valueChangedProcessingTypeComboBox( int nUnused )
{
	switch ( m_pLoopModeComboBox->currentIndex() ) {
		case 0:	 //
			__loops.mode = Sample::Loops::FORWARD;
			break;
		case 1:	 //
			__loops.mode = Sample::Loops::REVERSE;
			break;
		case 2:	 //
			__loops.mode = Sample::Loops::PINGPONG;
			break;
		default:
			__loops.mode = Sample::Loops::FORWARD;
	}
	setUnclean();
}

void SampleEditor::testPositionsSpinBoxes()
{
	m_bAdjusting = true;
	if ( __loops.start_frame > __loops.loop_frame )
		__loops.loop_frame = __loops.start_frame;
	if ( __loops.start_frame > __loops.end_frame )
		__loops.end_frame = __loops.start_frame;
	if ( __loops.loop_frame > __loops.end_frame )
		__loops.end_frame = __loops.loop_frame;
	if ( __loops.end_frame < __loops.loop_frame )
		__loops.loop_frame = __loops.end_frame;
	if ( __loops.end_frame < __loops.start_frame )
		__loops.start_frame = __loops.end_frame;
	m_pStartFrameSpinBox->setValue( __loops.start_frame );
	m_pLoopFrameSpinBox->setValue( __loops.loop_frame );
	m_pEndFrameSpinBox->setValue( __loops.end_frame );
	m_bAdjusting = false;
}

void SampleEditor::testpTimer()
{
	if ( m_pTimer->isActive() || m_pTargetDisplayTimer->isActive() ) {
		auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();
		m_pMainSampleWaveDisplay->paintLocatorEvent( -1, false );
		m_pTimer->stop();
		m_pTargetDisplayTimer->stop();
		m_pPlayButton->setText( pCommonString->getButtonPlay() );
		m_pPlayOriginalButton->setText(
			pCommonString->getButtonPlayOriginalSample()
		);
		Hydrogen::get_instance()
			->getAudioEngine()
			->getSampler()
			->stopPlayingNotes();
		m_bPlayButton = false;
	}
}
