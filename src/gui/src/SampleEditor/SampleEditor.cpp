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
#include "SampleWaveDisplay.h"
#include "TargetWaveDisplay.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LCDSpinBox.h"

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
	  m_bPlayButton( false ),
	  m_bSampleEditorClean( true ),
	  m_pPositionsRulerPath( nullptr ),
	  m_fRatio( 1.0f )
{
	if ( pInstrument == nullptr || pComponent == nullptr || pLayer == nullptr ||
		 pLayer->getSample() == nullptr ) {
		reject();
	}
	m_pSample = pLayer->getSample();
	m_nTargetFrames = static_cast<long long>( m_pSample->getFrames() );

	setFixedSize( SampleEditor::nWidth, SampleEditor::nHeight );
	setModal( true );
	setWindowTitle( QString( tr( "SampleEditor " ) + m_pSample->getFilePath() )
	);

	const auto nFrames = m_pSample->getFrames();
	m_loops = m_pSample->getLoops();
	// Per default all loop frames will be set to zero by Hydrogen. But this is
	// dangerous since just altering start or loop might move them beyond the
	// end.
	if ( m_loops.nStartFrame == 0 && m_loops.nLoopFrame == 0 &&
		 m_loops.nEndFrame == 0 ) {
		m_loops.nEndFrame = m_pSample->getFrames();
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pPref = Preferences::get_instance();
	const auto separatorColor =
		pPref->getColorTheme()->m_windowColor.darker( 135 );

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
background-color: %1;" )
									   .arg( separatorColor.name() ) );

		return pSeparator;
	};

	////////////////////////////////////////////////////////////////////////////

	auto pWaveDisplayContainer = new QWidget( pScrollArea );
	pGridLayout->addWidget( pWaveDisplayContainer, 0, 0, 1, 2 );

	auto pWaveDisplayContainerLayout = new QHBoxLayout();
	pWaveDisplayContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pWaveDisplayContainerLayout->setSpacing( 6 );
	pWaveDisplayContainer->setLayout( pWaveDisplayContainerLayout );

	auto pMainSection = new QWidget( pWaveDisplayContainer );
	pWaveDisplayContainerLayout->addWidget( pMainSection );

	auto pMainSectionLayout = new QVBoxLayout();
	pMainSectionLayout->setContentsMargins( 0, 0, 0, 0 );
	pMainSectionLayout->setSpacing( 0 );
	pMainSection->setLayout( pMainSectionLayout );

	m_pSampleWaveDisplayL =
		new SampleWaveDisplay( this, WaveDisplay::Channel::Left );
	m_pSampleWaveDisplayL->setLayer( pLayer );
	pMainSectionLayout->addWidget( m_pSampleWaveDisplayL );
	m_pSampleWaveDisplayR =
		new SampleWaveDisplay( this, WaveDisplay::Channel::Right );
	m_pSampleWaveDisplayR->setLayer( pLayer );
	pMainSectionLayout->addWidget( m_pSampleWaveDisplayR );

	auto pDetailSection = new QWidget( pWaveDisplayContainer );
	pWaveDisplayContainerLayout->addWidget( pDetailSection );

	auto pDetailSectionLayout = new QVBoxLayout();
	pDetailSectionLayout->setContentsMargins( 0, 0, 0, 0 );
	pDetailSectionLayout->setSpacing( 0 );
	pDetailSection->setLayout( pDetailSectionLayout );

	m_pDetailWaveDisplayL =
		new DetailWaveDisplay( this, WaveDisplay::Channel::Left );
	m_pDetailWaveDisplayL->setLayer( m_pLayer );
	pDetailSectionLayout->addWidget( m_pDetailWaveDisplayL );
	m_pDetailWaveDisplayR =
		new DetailWaveDisplay( this, WaveDisplay::Channel::Right );
	m_pDetailWaveDisplayR->setLayer( m_pLayer );
	pDetailSectionLayout->addWidget( m_pDetailWaveDisplayR );

	auto pZoomSlider = new QSlider( pWaveDisplayContainer );
	pZoomSlider->setMaximumHeight( 265 );
	pZoomSlider->setValue( 1 );
	pZoomSlider->setOrientation( Qt::Vertical );
	connect( pZoomSlider, &QSlider::valueChanged, [=]() {
		m_fZoomfactor = pZoomSlider->value() / 10 + 1;
		updateWaveDisplays();
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

	m_pLoopStartFrameSpinBox = new LCDSpinBox( pSpinBoxContainer );
	m_pLoopStartFrameSpinBox->setMinimumWidth( 100 );
	m_pLoopStartFrameSpinBox->setToolTip( tr( "Adjust sample start frame" ) );
	m_pLoopStartFrameSpinBox->setRange( 0, nFrames );
	m_pLoopStartFrameSpinBox->setValue( m_loops.nStartFrame );
	connect(
		m_pLoopStartFrameSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double ) {
			setLoopStartFrame( m_pLoopStartFrameSpinBox->value() );
		}
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopStartFrameSpinBox );

	pSpinBoxContainerLayout->addSpacerItem( new QSpacerItem( 40, 20 ) );

	auto pLoopFrameLabel = new QLabel( pSpinBoxContainer );
	pLoopFrameLabel->setText( tr( "Loop" ) );
	pSpinBoxContainerLayout->addWidget( pLoopFrameLabel );

	m_pLoopLoopFrameSpinBox = new LCDSpinBox( pSpinBoxContainer );
	m_pLoopLoopFrameSpinBox->setMinimumWidth( 100 );
	m_pLoopLoopFrameSpinBox->setToolTip( tr( "Adjust sample loop begin frame" )
	);
	m_pLoopLoopFrameSpinBox->setRange( 0, nFrames );
	m_pLoopLoopFrameSpinBox->setValue( m_loops.nLoopFrame );
	connect(
		m_pLoopLoopFrameSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double ) {
			setLoopLoopFrame( m_pLoopLoopFrameSpinBox->value() );
		}
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopLoopFrameSpinBox );

	auto pLoopModeLabel = new QLabel( pSpinBoxContainer );
	pLoopModeLabel->setText( tr( "mode" ) );
	pSpinBoxContainerLayout->addWidget( pLoopModeLabel );

	m_pLoopModeComboBox = new QComboBox( pSpinBoxContainer );
	m_pLoopModeComboBox->setMinimumWidth( 80 );
	m_pLoopModeComboBox->setToolTip( tr( "set processing" ) );
	m_pLoopModeComboBox->addItems(
		QStringList() << tr( "forward" ) << tr( "reverse" ) << tr( "pingpong" )
	);
	if ( m_loops.mode == Sample::Loops::Mode::Forward ) {
		m_pLoopModeComboBox->setCurrentIndex( 0 );
	}
	if ( m_loops.mode == Sample::Loops::Mode::Reverse ) {
		m_pLoopModeComboBox->setCurrentIndex( 1 );
	}
	if ( m_loops.mode == Sample::Loops::Mode::PingPong ) {
		m_pLoopModeComboBox->setCurrentIndex( 2 );
	}
	connect(
		m_pLoopModeComboBox, SIGNAL( currentIndexChanged( int ) ), this,
		SLOT( valueChangedProcessingTypeComboBox( int ) )
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopModeComboBox );

	auto pLoopCountLabel = new QLabel( pSpinBoxContainer );
	pLoopCountLabel->setText( tr( "count" ) );
	pSpinBoxContainerLayout->addWidget( pLoopCountLabel );

	m_pLoopCountSpinBox = new LCDSpinBox( pSpinBoxContainer );
	m_pLoopCountSpinBox->setMinimumWidth( 60 );
	m_pLoopCountSpinBox->setToolTip( tr( "loops" ) );
	m_pLoopCountSpinBox->setRange( 0, 20000 );
	m_pLoopCountSpinBox->setValue( m_loops.nCount );
	connect(
		m_pLoopCountSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double ) {
			if ( m_loops.nCount == m_pLoopCountSpinBox->value() ) {
				return;
			}

			m_loops.nCount = m_pLoopCountSpinBox->value();
			setUnclean();
			updateTargetFrames();
		}
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopCountSpinBox );

	pSpinBoxContainerLayout->addSpacerItem( new QSpacerItem( 40, 20 ) );

	auto pEndFrameLabel = new QLabel( pSpinBoxContainer );
	pEndFrameLabel->setText( tr( "End" ) );
	pSpinBoxContainerLayout->addWidget( pEndFrameLabel );

	m_pLoopEndFrameSpinBox = new LCDSpinBox( pSpinBoxContainer );
	m_pLoopEndFrameSpinBox->setMinimumWidth( 100 );
	m_pLoopEndFrameSpinBox->setToolTip( tr( "Adjust sample and loop end frame" )
	);
	m_pLoopEndFrameSpinBox->setRange( 0, nFrames );
	m_pLoopEndFrameSpinBox->setValue( m_loops.nEndFrame );
	connect(
		m_pLoopEndFrameSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double ) {
			setLoopEndFrame( m_pLoopEndFrameSpinBox->value() );
		}
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopEndFrameSpinBox );

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

	m_pRubberBandPitchSpinBox = new LCDSpinBox(
		pRubberBandWidgetContainer, QSize(), LCDSpinBox::Type::Double, -36, 36
	);
	m_pRubberBandPitchSpinBox->setMaximumWidth( 74 );
	m_pRubberBandPitchSpinBox->setToolTip(
		tr( "Pitch the sample in semitones, cents" )
	);
    m_pRubberBandPitchSpinBox->setValue( 0 );
	m_pRubberBandPitchSpinBox->setSingleStep( 0.01 );
	// Make things consistent with the LCDDisplay and LCDSpinBox classes.
	m_pRubberBandPitchSpinBox->setLocale(
		QLocale( QLocale::C, QLocale::AnyCountry )
	);
	connect(
		m_pRubberBandPitchSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double ) {
			if ( std::abs(
					 m_rubberband.fSemitonesToShift -
					 m_pRubberBandPitchSpinBox->value()
				 ) < 0.0001 ) {
				return;
			}

			m_rubberband.fSemitonesToShift = m_pRubberBandPitchSpinBox->value();
			setUnclean();
		}
	);
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandPitchSpinBox );

	pRubberBandWidgetContainerLayout->addSpacerItem( new QSpacerItem( 40, 20 )
	);

	auto pRubberBandCrispnessLabel = new QLabel( pRubberBandWidgetContainer );
	pRubberBandCrispnessLabel->setMinimumWidth( 100 );
	pRubberBandCrispnessLabel->setText( tr( "Crispness: " ) );
	pRubberBandWidgetContainerLayout->addWidget( pRubberBandCrispnessLabel );

	m_pRubberBandCrispnessComboBox =
		new QComboBox( pRubberBandWidgetContainer );
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
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandCrispnessComboBox
	);

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

	pButtonContainerLayout->addSpacerItem( new QSpacerItem( 40, 20 ) );

    // Ensure new length label and value are close of eachother.
    auto pNewLengthContainer = new QWidget( pButtonContainer );
    pNewLengthContainer->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred);
    pButtonContainerLayout->addWidget( pNewLengthContainer );

    auto pNewLengthContainerLayout = new QHBoxLayout();
    pNewLengthContainerLayout->setContentsMargins( 0, 0, 0, 0 );
    pNewLengthContainerLayout->setSpacing( 9 );
    pNewLengthContainer->setLayout( pNewLengthContainerLayout );

	auto pNewLengthLabel = new QLabel( pNewLengthContainer );
    pNewLengthLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
	pNewLengthLabel->setFixedWidth( 180 );
	pNewLengthLabel->setText( tr( "new sample length:" ) );
	pNewLengthContainerLayout->addWidget( pNewLengthLabel );

	m_pNewLengthDisplay =
		new LCDDisplay( pNewLengthContainer, QSize(), false, false );
	m_pNewLengthDisplay->setFixedWidth( 120 );
	pNewLengthContainerLayout->addWidget( m_pNewLengthDisplay );

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

	m_rubberband.nCrispness = 4;
	m_rubberband.bUse = false;
	m_rubberband.fLengthInBeats = 1.0;
	m_rubberband.fSemitonesToShift = 0.0;

	getAllFrameInfos();
    updateTargetFrames();

	updateWaveDisplays();

#ifndef H2CORE_HAVE_RUBBERBAND
	if ( !Filesystem::file_executable(
			 pPref->m_sRubberBandCLIexecutable, true /* silent */
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
	m_pTargetSampleView->close();
	delete m_pTargetSampleView;
	m_pTargetSampleView = nullptr;

	INFOLOG( "DESTROY" );
}

void SampleEditor::setSelectedSlider( Slider slider )
{
	if ( m_selectedSlider == slider ) {
		return;
	}
	m_selectedSlider = slider;
	updateWaveDisplays();
}

void SampleEditor::setLoopStartFrame( int nFrame )
{
	if ( m_loops.nStartFrame == nFrame && m_selectedSlider == Slider::Start ) {
		return;
	}

	m_loops.nStartFrame = nFrame;

	if ( m_loops.nStartFrame > m_loops.nLoopFrame ) {
		m_loops.nLoopFrame = m_loops.nStartFrame;
	}
	if ( m_loops.nStartFrame > m_loops.nEndFrame ) {
		m_loops.nEndFrame = m_loops.nStartFrame;
	}

	m_pLoopStartFrameSpinBox->setValue( nFrame, Event::Trigger::Suppress );

	m_selectedSlider = Slider::Start;

    setUnclean();
    updateTargetFrames();
	updateWaveDisplays();
}

void SampleEditor::setLoopLoopFrame( int nFrame )
{
	if ( m_loops.nLoopFrame == nFrame && m_selectedSlider == Slider::Loop ) {
		return;
	}

	m_loops.nLoopFrame = nFrame;

	if ( m_loops.nLoopFrame < m_loops.nStartFrame ) {
		m_loops.nStartFrame = m_loops.nLoopFrame;
	}
	if ( m_loops.nLoopFrame > m_loops.nEndFrame ) {
		m_loops.nEndFrame = m_loops.nLoopFrame;
	}

	m_pLoopLoopFrameSpinBox->setValue( nFrame, Event::Trigger::Suppress );

	m_selectedSlider = Slider::Loop;
    setUnclean();
    updateTargetFrames();
	updateWaveDisplays();
}

void SampleEditor::setLoopEndFrame( int nFrame )
{
	if ( m_loops.nEndFrame == nFrame && m_selectedSlider == Slider::End ) {
		return;
	}

	m_loops.nEndFrame = nFrame;

	if ( m_loops.nEndFrame < m_loops.nStartFrame ) {
		m_loops.nStartFrame = m_loops.nEndFrame;
	}
	if ( m_loops.nEndFrame < m_loops.nLoopFrame ) {
		m_loops.nLoopFrame = m_loops.nEndFrame;
	}

	m_pLoopEndFrameSpinBox->setValue( nFrame, Event::Trigger::Suppress );

	m_selectedSlider = Slider::End;
    setUnclean();
    updateTargetFrames();
	updateWaveDisplays();
}

int SampleEditor::getEnvelopeIndex() const
{
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

void SampleEditor::updateWaveDisplays()
{
	m_pDetailWaveDisplayL->update();
	m_pDetailWaveDisplayR->update();
	m_pSampleWaveDisplayL->update();
	m_pSampleWaveDisplayR->update();
}

void SampleEditor::getAllFrameInfos()
{
	// this values are needed if we restore a sample from disk if a
	// new song with sample changes will load
	m_bSampleIsModified = m_pSample->getIsModified();
	m_nSamplerate = m_pSample->getSampleRate();

	m_rubberband = m_pSample->getRubberband();

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
		if ( !m_rubberband.bUse ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 0 );
		}

		m_pRubberBandCrispnessComboBox->setCurrentIndex( m_rubberband.nCrispness
		);
		if ( !m_rubberband.bUse ) {
			m_pRubberBandCrispnessComboBox->setCurrentIndex( 4 );
		}
		m_pRubberBandPitchSpinBox->setValue( m_rubberband.fSemitonesToShift );
		if ( !m_rubberband.bUse ) {
			m_pRubberBandPitchSpinBox->setValue( 0.0 );
		}

		if ( m_rubberband.fLengthInBeats == 1.0 / 64.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 1 );
		}
		else if ( m_rubberband.fLengthInBeats == 1.0 / 32.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 2 );
		}
		else if ( m_rubberband.fLengthInBeats == 1.0 / 16.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 3 );
		}
		else if ( m_rubberband.fLengthInBeats == 1.0 / 8.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 4 );
		}
		else if ( m_rubberband.fLengthInBeats == 1.0 / 4.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 5 );
		}
		else if ( m_rubberband.fLengthInBeats == 1.0 / 2.0 ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( 6 );
		}
		else if ( m_rubberband.bUse && ( m_rubberband.fLengthInBeats >= 1.0 ) ) {
			m_pRubberBandLengthComboBox->setCurrentIndex( (int
			) ( m_rubberband.fLengthInBeats + 6 ) );
		}
	}
	m_pTargetSampleView->updateDisplay( m_pLayer );
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
		pEditSample->setLoops( m_loops );
		pEditSample->setRubberband( m_rubberband );
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

void SampleEditor::returnAllTargetDisplayValues()
{
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

void SampleEditor::on_PlayPushButton_clicked()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( m_pPlayButton->text() == "Stop" ) {
		testpTimer();
		return;
	}

	// Since we are in a separate dialog and working with a particular sample,
	// we do not want rendering to be affected by whether some instruments of
	// the current kit are soloed or muted.
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

	createPositionsRulerPath();
	m_bPlayButton = true;

	m_selectedSlider = Slider::None;
	m_nFramePosition = static_cast<long long>(m_loops.nStartFrame);

	updateWaveDisplays();

	if ( m_rubberband.bUse == false ) {
		m_pTimer->start( 40 );	// update ruler at 25 fps
	}

	m_nRealtimeFrameEnd = pAudioEngine->getRealtimeFrame() + m_nTargetFrames;

	// calculate the new rubberband sample length
	if ( m_rubberband.bUse ) {
		m_nRealtimeFrameEndForTarget =
			pAudioEngine->getRealtimeFrame() + ( m_nTargetFrames * m_fRatio + 0.1 );
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
		m_selectedSlider = Slider::None;
		m_nFramePosition = static_cast<long long>(m_loops.nStartFrame);

		updateWaveDisplays();

		m_pTimer->start( 40 );	// update ruler at 25 fps
		m_nRealtimeFrameEnd =
			pHydrogen->getAudioEngine()->getRealtimeFrame() + m_nTargetFrames;
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

	tearDown();
}

void SampleEditor::updateMainsamplePositionRuler()
{
	const auto nRealtimeFrame =
		Hydrogen::get_instance()->getAudioEngine()->getRealtimeFrame();
	if ( nRealtimeFrame < m_nRealtimeFrameEnd ) {
		const long long nFrame = m_nTargetFrames - ( m_nRealtimeFrameEnd - nRealtimeFrame );
		if ( m_bPlayButton == true ) {
			m_nFramePosition = m_pPositionsRulerPath[nFrame];
		}
		else {
			m_nFramePosition = nFrame;
		}

		updateWaveDisplays();
	}
	else {
		auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();
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
	const auto nRealtimeFrame =
		Hydrogen::get_instance()->getAudioEngine()->getRealtimeFrame();
	long long nTargetSampleLength;
	if ( m_rubberband.bUse ) {
		nTargetSampleLength = m_nTargetFrames * m_fRatio + 0.1;
	}
	else {
		nTargetSampleLength = m_nTargetFrames;
	}

	if ( nRealtimeFrame < m_nRealtimeFrameEndForTarget ) {
		const long long nPos =
			nTargetSampleLength - ( m_nRealtimeFrameEndForTarget - nRealtimeFrame );
		m_pTargetSampleView->paintLocatorEventTargetDisplay(
			( m_pTargetSampleView->width() * nPos / nTargetSampleLength ), true
		);
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
	unsigned oneSampleLength = m_loops.nEndFrame - m_loops.nStartFrame;
	unsigned loopLength = m_loops.nEndFrame - m_loops.nLoopFrame;
	unsigned repeatsLength = loopLength * m_loops.nCount;
	unsigned newLength = 0;
	if ( oneSampleLength == loopLength ) {
		newLength = oneSampleLength + oneSampleLength * m_loops.nCount;
	}
	else {
		newLength = oneSampleLength + repeatsLength;
	}

	unsigned normalLength = m_pSample->getFrames();

	long long* normalFrames = new long long[normalLength];
	long long* tempFrames = new long long[newLength];
	long long* loopFrames = new long long[loopLength];

	for ( unsigned i = 0; i < normalLength; i++ ) {
		normalFrames[i] = i;
	}

	Sample::Loops::Mode mode = m_loops.mode;
	long int z = m_loops.nLoopFrame;
	long int y = m_loops.nStartFrame;

	for ( unsigned i = 0; i < newLength; i++ ) {  // first vector
		tempFrames[i] = 0;
	}

	for ( unsigned i = 0; i < oneSampleLength; i++, y++ ) {	 // first vector

		tempFrames[i] = normalFrames[y];
	}

	for ( unsigned i = 0; i < loopLength; i++, z++ ) {	// loop vector

		loopFrames[i] = normalFrames[z];
	}

	if ( mode == Sample::Loops::Mode::Reverse ) {
		std::reverse( loopFrames, loopFrames + loopLength );
	}

	if ( mode == Sample::Loops::Mode::Reverse && m_loops.nCount > 0 &&
		 m_loops.nStartFrame == m_loops.nLoopFrame ) {
		std::reverse( tempFrames, tempFrames + oneSampleLength );
	}

	if ( mode == Sample::Loops::Mode::PingPong &&
		 m_loops.nStartFrame == m_loops.nLoopFrame ) {
		std::reverse( loopFrames, loopFrames + loopLength );
	}

	for ( int i = 0; i < m_loops.nCount; i++ ) {
		unsigned tempdataend = oneSampleLength + ( loopLength * i );
		if ( m_loops.nStartFrame == m_loops.nLoopFrame ) {
			std::copy(
				loopFrames, loopFrames + loopLength, tempFrames + tempdataend
			);
		}
		if ( mode == Sample::Loops::Mode::PingPong && m_loops.nCount > 1 ) {
			std::reverse( loopFrames, loopFrames + loopLength );
		}
		if ( m_loops.nStartFrame != m_loops.nLoopFrame ) {
			std::copy(
				loopFrames, loopFrames + loopLength, tempFrames + tempdataend
			);
		}
	}

	if ( m_loops.nCount == 0 && mode == Sample::Loops::Mode::Reverse ) {
		std::reverse( tempFrames + m_loops.nLoopFrame, tempFrames + newLength );
	}

	if ( m_pPositionsRulerPath ) {
		delete[] m_pPositionsRulerPath;
	}

	m_pPositionsRulerPath = tempFrames;

	delete[] loopFrames;
	delete[] normalFrames;
}

void SampleEditor::updateTargetFrames()
{
	const int nPreLoopFrames = m_loops.nLoopFrame - m_loops.nStartFrame;
	const int nLoopFrames = m_loops.nEndFrame - m_loops.nLoopFrame;
	m_nTargetFrames = static_cast<long long>( nPreLoopFrames ) +
					  static_cast<long long>( nLoopFrames ) *
						  static_cast<long long>( m_loops.nCount + 1 );

	m_pNewLengthDisplay->setText( QString::number( m_nTargetFrames ) );
	checkRatioSettings();
}

void SampleEditor::valueChangedrubberbandCsettingscomboBox( int )
{
	const int nNewCrispness = m_pRubberBandCrispnessComboBox->currentIndex();
	if ( nNewCrispness == m_rubberband.nCrispness ) {
		return;
	}
	m_rubberband.nCrispness = nNewCrispness;
	setUnclean();
}

void SampleEditor::valueChangedrubberComboBox( int )
{
	if ( m_pRubberBandLengthComboBox->currentText() != "off" ) {
		m_rubberband.bUse = true;
	}
	else {
		m_rubberband.bUse = false;
		m_rubberband.fLengthInBeats = 1.0;
	}

	switch ( m_pRubberBandLengthComboBox->currentIndex() ) {
		case 0:	 //
			m_rubberband.fLengthInBeats = 4.0;
			break;
		case 1:	 //
			m_rubberband.fLengthInBeats = 1.0 / 64.0;
			break;
		case 2:	 //
			m_rubberband.fLengthInBeats = 1.0 / 32.0;
			break;
		case 3:	 //
			m_rubberband.fLengthInBeats = 1.0 / 16.0;
			break;
		case 4:	 //
			m_rubberband.fLengthInBeats = 1.0 / 8.0;
			break;
		case 5:	 //
			m_rubberband.fLengthInBeats = 1.0 / 4.0;
			break;
		case 6:	 //
			m_rubberband.fLengthInBeats = 1.0 / 2.0;
			break;
		case 7:	 //
			m_rubberband.fLengthInBeats = 1.0;
			break;
		default:
			m_rubberband.fLengthInBeats =
				(float) m_pRubberBandLengthComboBox->currentIndex() - 6.0;
	}
	//	QMessageBox::information ( this, "Hydrogen", tr ( "divider %1" ).arg(
	// m_rubberband.divider )); 	float m_rubberband.divider;
	checkRatioSettings();

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
						  m_rubberband.fLengthInBeats;
	double induration = (double) m_nTargetFrames / (double) m_nSamplerate;
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
	if ( !m_rubberband.bUse ) {
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
			m_loops.mode = Sample::Loops::Mode::Forward;
			break;
		case 1:	 //
			m_loops.mode = Sample::Loops::Mode::Reverse;
			break;
		case 2:	 //
			m_loops.mode = Sample::Loops::Mode::PingPong;
			break;
		default:
			m_loops.mode = Sample::Loops::Mode::Forward;
	}
	setUnclean();
}

void SampleEditor::testpTimer()
{
	if ( m_pTimer->isActive() || m_pTargetDisplayTimer->isActive() ) {
		auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();
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
