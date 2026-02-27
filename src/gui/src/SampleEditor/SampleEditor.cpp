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
#include "../UndoActions.h"
#include "../Widgets/Button.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LCDSpinBox.h"
#include "DetailWaveDisplay.h"
#include "SampleWaveDisplay.h"
#include "TargetWaveDisplay.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
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

QString SampleEditor::EnvelopeTypeToQString( const EnvelopeType& envelopeType )
{
	switch ( envelopeType ) {
		case EnvelopeType::Velocity:
			return "Velocity";
		case EnvelopeType::Pan:
			return "Pan";
		default:
			return "Unknown";
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
	  m_playback( Playback::None ),
	  m_fZoomfactor( 1.0 ),
	  m_nPlayheadSample( 0 ),
	  m_nPlayheadTarget( 0 ),
	  m_nLoopFrames( 0 ),
	  m_nLastRealtimeFrame( 0 ),
	  m_looped( Looped::NotYet ),
	  m_hoveredSlider( Slider::None ),
	  m_selectedSlider( Slider::Start ),
	  m_bPlayButton( false ),
	  m_bSampleEditorClean( true ),
	  m_bRetriggerRequired( false ),
	  m_bLayerReloadRequired( false ),
	  m_fIncrementScaling( 1.0f ),
	  m_envelopeType( EnvelopeType::Velocity )
{
	if ( pInstrument == nullptr || pComponent == nullptr || pLayer == nullptr ||
		 pLayer->getSample() == nullptr ) {
		reject();
	}
	m_pSample = std::make_shared<Sample>( pLayer->getSample() );
	const auto nFrames = m_pSample->getFrames();

	m_pPreviewInstrument = std::make_shared<Instrument>( Instrument::EmptyId );
	auto pPreviewLayer = std::make_shared<InstrumentLayer>( m_pSample );
	m_pPreviewInstrument->setIsPreviewInstrument( true );
	m_pPreviewInstrument->addLayer(
		m_pPreviewInstrument->getComponents()->front(), pPreviewLayer, 0,
		Event::Trigger::Suppress
	);

	m_pSampleOriginal = Sample::load( m_pSample->getFilePath() );
	if ( m_pSampleOriginal == nullptr ) {
		ERRORLOG( QString( "Unable to load sample from [%1]" )
					  .arg( m_pSample->getFilePath() ) );
		reject();
	}
	m_pPreviewInstrumentOriginal =
		std::make_shared<Instrument>( Instrument::EmptyId );
	auto pPreviewLayerOriginal =
		std::make_shared<InstrumentLayer>( m_pSampleOriginal );
	m_pPreviewInstrumentOriginal->setIsPreviewInstrument( true );
	m_pPreviewInstrumentOriginal->addLayer(
		m_pPreviewInstrumentOriginal->getComponents()->front(),
		pPreviewLayerOriginal, 0, Event::Trigger::Suppress
	);

	setFixedSize( SampleEditor::nWidth, SampleEditor::nHeight );
	setModal( true );
	setWindowTitle( QString( tr( "SampleEditor " ) + m_pSample->getFilePath() )
	);

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

	auto pVBoxLayout = new QVBoxLayout();
	pVBoxLayout->setContentsMargins( 9, 9, 9, 9 );
	pVBoxLayout->setSpacing( SampleEditor::nSpacing );
	pScrollArea->setLayout( pVBoxLayout );

	auto createSeparator = [&]( QWidget* pParent ) {
		auto pSeparator = new QWidget( pParent );
		pSeparator->setFixedHeight( 1 );
		pSeparator->setStyleSheet( QString( "\
background-color: %1;" )
									   .arg( separatorColor.name() ) );

		return pSeparator;
	};

	auto addSpacer = [&]( QBoxLayout* pLayout ) {
		pLayout->addSpacerItem( new QSpacerItem(
			SampleEditor::nSpacerWidth, SampleEditor::nSpacerHeight
		) );
	};

	auto addHeading = [&]( const QString& sText, QBoxLayout* pLayout ) {
		auto pLabel = new QLabel( pScrollArea );
		pLabel->setStyleSheet(
			"\
font-size: 13px;                \
font-weight: bold; "
		);
		pLabel->setText( sText );
		pLayout->addWidget( pLabel );
	};

	////////////////////////////////////////////////////////////////////////////

	addHeading( tr( "Original Sample and Loop Settings" ), pVBoxLayout );

	////////////////////////////////////////////////////////////////////////////

	auto pWaveDisplayContainer = new QWidget( pScrollArea );
	pVBoxLayout->addWidget( pWaveDisplayContainer );

	auto pWaveDisplayContainerLayout = new QHBoxLayout();
	pWaveDisplayContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pWaveDisplayContainerLayout->setSpacing( SampleEditor::nSpacing );
	pWaveDisplayContainer->setLayout( pWaveDisplayContainerLayout );

	auto pMainSection = new QWidget( pWaveDisplayContainer );
	pWaveDisplayContainerLayout->addWidget( pMainSection );

	auto pMainSectionLayout = new QVBoxLayout();
	pMainSectionLayout->setContentsMargins( 0, 0, 0, 0 );
	pMainSectionLayout->setSpacing( 0 );
	pMainSection->setLayout( pMainSectionLayout );

	m_pSampleWaveDisplayL =
		new SampleWaveDisplay( this, WaveDisplay::Channel::Left );
	m_pSampleWaveDisplayL->setLayer( pPreviewLayerOriginal );
	pMainSectionLayout->addWidget( m_pSampleWaveDisplayL );
	m_pSampleWaveDisplayR =
		new SampleWaveDisplay( this, WaveDisplay::Channel::Right );
	m_pSampleWaveDisplayR->setLayer( pPreviewLayerOriginal );
	pMainSectionLayout->addWidget( m_pSampleWaveDisplayR );

	auto pDetailSection = new QWidget( pWaveDisplayContainer );
	pWaveDisplayContainerLayout->addWidget( pDetailSection );

	auto pDetailSectionLayout = new QVBoxLayout();
	pDetailSectionLayout->setContentsMargins( 0, 0, 0, 0 );
	pDetailSectionLayout->setSpacing( 0 );
	pDetailSection->setLayout( pDetailSectionLayout );

	m_pDetailWaveDisplayL =
		new DetailWaveDisplay( this, WaveDisplay::Channel::Left );
	m_pDetailWaveDisplayL->setLayer( pPreviewLayerOriginal );
	pDetailSectionLayout->addWidget( m_pDetailWaveDisplayL );
	m_pDetailWaveDisplayR =
		new DetailWaveDisplay( this, WaveDisplay::Channel::Right );
	m_pDetailWaveDisplayR->setLayer( pPreviewLayerOriginal );
	pDetailSectionLayout->addWidget( m_pDetailWaveDisplayR );

	auto pZoomSlider = new QSlider( pWaveDisplayContainer );
	pZoomSlider->setMaximumHeight( 265 );
	pZoomSlider->setValue( ( m_fZoomfactor - 1 ) * 10 );
	pZoomSlider->setOrientation( Qt::Vertical );
	connect( pZoomSlider, &QSlider::valueChanged, [=]() {
		m_fZoomfactor = pZoomSlider->value() / 10 + 1;
		updateSourceWaveDisplays();
	} );
	pWaveDisplayContainerLayout->addWidget( pZoomSlider );

	////////////////////////////////////////////////////////////////////////////

	auto pSpinBoxContainer = new QWidget( pScrollArea );
	pVBoxLayout->addWidget( pSpinBoxContainer );

	auto pSpinBoxContainerLayout = new QHBoxLayout();
	pSpinBoxContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pSpinBoxContainerLayout->setSpacing( SampleEditor::nSpacing );
	pSpinBoxContainer->setLayout( pSpinBoxContainerLayout );

	auto pStartFrameLabel = new QLabel( pSpinBoxContainer );
	pStartFrameLabel->setText( tr( "Start" ) );
	pSpinBoxContainerLayout->addWidget( pStartFrameLabel );

	m_pLoopStartFrameSpinBox = new LCDSpinBox( pSpinBoxContainer );
	m_pLoopStartFrameSpinBox->setMinimumWidth( 100 );
	m_pLoopStartFrameSpinBox->setToolTip( tr( "Adjust sample start frame" ) );
	m_pLoopStartFrameSpinBox->setRange( 0, nFrames );
	connect(
		m_pLoopStartFrameSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double ) {
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_changeSliderAction(
					SampleEditor::Slider::Start, m_loops.nStartFrame,
					m_pLoopStartFrameSpinBox->value()
				),
				QString( "SampleEditor::slider::%1" )
					.arg( SampleEditor::SliderToQString(
						SampleEditor::Slider::Start
					) )
			);
		}
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopStartFrameSpinBox );

	addSpacer( pSpinBoxContainerLayout );

	auto pLoopFrameLabel = new QLabel( pSpinBoxContainer );
	pLoopFrameLabel->setText( tr( "Loop" ) );
	pSpinBoxContainerLayout->addWidget( pLoopFrameLabel );

	m_pLoopLoopFrameSpinBox = new LCDSpinBox( pSpinBoxContainer );
	m_pLoopLoopFrameSpinBox->setMinimumWidth( 100 );
	m_pLoopLoopFrameSpinBox->setToolTip( tr( "Adjust sample loop begin frame" )
	);
	m_pLoopLoopFrameSpinBox->setRange( 0, nFrames );
	connect(
		m_pLoopLoopFrameSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double ) {
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_changeSliderAction(
					SampleEditor::Slider::Loop, m_loops.nLoopFrame,
					m_pLoopLoopFrameSpinBox->value()
				),
				QString( "SampleEditor::slider::%1" )
					.arg( SampleEditor::SliderToQString(
						SampleEditor::Slider::Loop
					) )
			);
		}
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopLoopFrameSpinBox );

	addSpacer( pSpinBoxContainerLayout );

	auto pLoopModeLabel = new QLabel( pSpinBoxContainer );
	pLoopModeLabel->setText( tr( "mode" ) );
	pSpinBoxContainerLayout->addWidget( pLoopModeLabel );

	m_pLoopModeComboBox = new LCDCombo( pSpinBoxContainer );
	m_pLoopModeComboBox->setMinimumWidth( 80 );
	m_pLoopModeComboBox->setToolTip( tr( "set processing" ) );
	m_pLoopModeComboBox->addItems(
		QStringList() << tr( "forward" ) << tr( "reverse" ) << tr( "pingpong" )
	);
	connect(
		m_pLoopModeComboBox, QOverload<int>::of( &QComboBox::activated ),
		[&]() {
			auto newLoops = m_loops;
			switch ( m_pLoopModeComboBox->currentIndex() ) {
				case 0:	 //
					newLoops.mode = Sample::Loops::Mode::Forward;
					break;
				case 1:	 //
					newLoops.mode = Sample::Loops::Mode::Reverse;
					break;
				case 2:	 //
					newLoops.mode = Sample::Loops::Mode::PingPong;
					break;
				default:
					newLoops.mode = Sample::Loops::Mode::Forward;
			}
			if ( newLoops.mode == m_loops.mode ) {
				return;
			}
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_changeLoopsAction( m_loops, newLoops ),
				"SampleEditor::loop::mode"
			);
		}
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopModeComboBox );

	addSpacer( pSpinBoxContainerLayout );

	auto pLoopCountLabel = new QLabel( pSpinBoxContainer );
	pLoopCountLabel->setText( tr( "count" ) );
	pSpinBoxContainerLayout->addWidget( pLoopCountLabel );

	m_pLoopCountSpinBox = new LCDSpinBox( pSpinBoxContainer );
	m_pLoopCountSpinBox->setMinimumWidth( 60 );
	m_pLoopCountSpinBox->setToolTip( tr( "loops" ) );
	m_pLoopCountSpinBox->setRange( 0, 20000 );
	connect(
		m_pLoopCountSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double ) {
			if ( m_loops.nCount == m_pLoopCountSpinBox->value() ) {
				return;
			}
			auto newLoops = m_loops;
			newLoops.nCount = m_pLoopCountSpinBox->value();
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_changeLoopsAction( m_loops, newLoops ),
				"SampleEditor::loop::count"
			);
		}
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopCountSpinBox );

	addSpacer( pSpinBoxContainerLayout );

	auto pEndFrameLabel = new QLabel( pSpinBoxContainer );
	pEndFrameLabel->setText( tr( "End" ) );
	pSpinBoxContainerLayout->addWidget( pEndFrameLabel );

	m_pLoopEndFrameSpinBox = new LCDSpinBox( pSpinBoxContainer );
	m_pLoopEndFrameSpinBox->setMinimumWidth( 100 );
	m_pLoopEndFrameSpinBox->setToolTip( tr( "Adjust sample and loop end frame" )
	);
	m_pLoopEndFrameSpinBox->setRange( 0, nFrames );
	connect(
		m_pLoopEndFrameSpinBox,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double ) {
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_changeSliderAction(
					SampleEditor::Slider::End, m_loops.nEndFrame,
					m_pLoopEndFrameSpinBox->value()
				),
				QString( "SampleEditor::slider::%1" )
					.arg( SampleEditor::SliderToQString(
						SampleEditor::Slider::End
					) )
			);
		}
	);
	pSpinBoxContainerLayout->addWidget( m_pLoopEndFrameSpinBox );

	pSpinBoxContainerLayout->addStretch();

	////////////////////////////////////////////////////////////////////////////

	auto pRubberBandContainer = new QWidget( pScrollArea );
	pVBoxLayout->addWidget( pRubberBandContainer );

	auto pRubberBandContainerLayout = new QVBoxLayout();
	pRubberBandContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pRubberBandContainerLayout->setSpacing( SampleEditor::nSpacing );
	pRubberBandContainer->setLayout( pRubberBandContainerLayout );

	auto pRubberBandSeparator = createSeparator( pRubberBandContainer );
	pRubberBandContainerLayout->addWidget( pRubberBandSeparator );

	auto pRubberBandHeaderContainer = new QWidget( pRubberBandContainer );
	pRubberBandContainerLayout->addWidget( pRubberBandHeaderContainer );

	auto pRubberBandHeaderContainerLayout = new QHBoxLayout();
	pRubberBandHeaderContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pRubberBandHeaderContainerLayout->setSpacing( SampleEditor::nSpacing );
	pRubberBandHeaderContainer->setLayout( pRubberBandHeaderContainerLayout );

	addHeading(
		tr( "Rubberband Audio Processor" ), pRubberBandHeaderContainerLayout
	);

	auto pRubberBandHeadingLabel = new QLabel( pRubberBandHeaderContainer );
	pRubberBandHeadingLabel->setText(
		tr( "Change the tempo (sample length) and "
			"pitch of audio." )
	);
	pRubberBandHeaderContainerLayout->addWidget( pRubberBandHeadingLabel );

	pRubberBandHeaderContainerLayout->addStretch();

	auto pRubberBandWidgetContainer = new QWidget( pRubberBandContainer );
	pRubberBandContainerLayout->addWidget( pRubberBandWidgetContainer );

	auto pRubberBandWidgetContainerLayout = new QHBoxLayout();
	pRubberBandWidgetContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pRubberBandWidgetContainerLayout->setSpacing( SampleEditor::nSpacing );
	pRubberBandWidgetContainer->setLayout( pRubberBandWidgetContainerLayout );

	auto pRubberBandLengthLabel = new QLabel( pRubberBandWidgetContainer );
	pRubberBandLengthLabel->setText( tr( "Sample length to beat:" ) );
	pRubberBandWidgetContainerLayout->addWidget( pRubberBandLengthLabel );

	m_pRubberBandLengthComboBox = new LCDCombo( pRubberBandWidgetContainer );
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
	connect(
		m_pRubberBandLengthComboBox,
		QOverload<int>::of( &QComboBox::activated ),
		[&]() {
			auto newRubberband = m_rubberband;
			newRubberband.bUse = true;
			switch ( m_pRubberBandLengthComboBox->currentIndex() ) {
				case 0:
					newRubberband.fLengthInBeats = 4.0;
					newRubberband.bUse = false;
					break;
				case 1:
					newRubberband.fLengthInBeats = 1.0 / 64.0;
					break;
				case 2:
					newRubberband.fLengthInBeats = 1.0 / 32.0;
					break;
				case 3:
					newRubberband.fLengthInBeats = 1.0 / 16.0;
					break;
				case 4:
					newRubberband.fLengthInBeats = 1.0 / 8.0;
					break;
				case 5:
					newRubberband.fLengthInBeats = 1.0 / 4.0;
					break;
				case 6:
					newRubberband.fLengthInBeats = 1.0 / 2.0;
					break;
				case 7:
					newRubberband.fLengthInBeats = 1.0;
					break;
				default:
					newRubberband.fLengthInBeats =
						(float) m_pRubberBandLengthComboBox->currentIndex() -
						6.0;
			}
			if ( m_rubberband.fLengthInBeats == newRubberband.fLengthInBeats ) {
				return;
			}
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_changeRubberbandAction( m_rubberband, newRubberband ),
				"SampleEditor::rubberband::length"
			);
		}
	);
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandLengthComboBox );

	addSpacer( pRubberBandWidgetContainerLayout );

	m_pRubberBandRatioLabel = new QLabel( pRubberBandWidgetContainer );
	m_pRubberBandRatioLabel->setMinimumWidth( 150 );
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandRatioLabel );

	pRubberBandWidgetContainerLayout->addStretch();

	auto pRubberBandPitchLabel = new QLabel( pRubberBandWidgetContainer );
	pRubberBandPitchLabel->setText( tr( "Pitch (Semitone,Cent)" ) );
	pRubberBandWidgetContainerLayout->addWidget( pRubberBandPitchLabel );

	m_pRubberBandPitchSpinBox = new LCDSpinBox(
		pRubberBandWidgetContainer, QSize(), LCDSpinBox::Type::Double, -36, 36
	);
	m_pRubberBandPitchSpinBox->setMaximumWidth( 74 );
	m_pRubberBandPitchSpinBox->setToolTip(
		tr( "Pitch the sample in semitones, cents" )
	);
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
			auto newRubberband = m_rubberband;
			newRubberband.fSemitonesToShift =
				m_pRubberBandPitchSpinBox->value();
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_changeRubberbandAction( m_rubberband, newRubberband ),
				"SampleEditor::rubberband::pitch"
			);
		}
	);
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandPitchSpinBox );

	addSpacer( pRubberBandWidgetContainerLayout );

	auto pRubberBandCrispnessLabel = new QLabel( pRubberBandWidgetContainer );
	pRubberBandCrispnessLabel->setText( tr( "Crispness: " ) );
	pRubberBandWidgetContainerLayout->addWidget( pRubberBandCrispnessLabel );

	m_pRubberBandCrispnessComboBox = new LCDCombo( pRubberBandWidgetContainer );
	m_pRubberBandCrispnessComboBox->setMaximumWidth( 45 );
	m_pRubberBandCrispnessComboBox->setToolTip(
		"http://www.breakfastquay.com/rubberband/"
	);
	QStringList crispnessOptions;
	for ( int ii = 1; ii <= 5; ++ii ) {
		crispnessOptions << QString::number( ii );
	}
	m_pRubberBandCrispnessComboBox->addItems( crispnessOptions );
	connect(
		m_pRubberBandLengthComboBox,
		QOverload<int>::of( &QComboBox::activated ),
		[&]() {
			const int nNewCrispness =
				m_pRubberBandCrispnessComboBox->currentIndex();
			if ( nNewCrispness == m_rubberband.nCrispness ) {
				return;
			}
			auto newRubberband = m_rubberband;
			newRubberband.nCrispness = nNewCrispness;
			HydrogenApp::get_instance()->pushUndoCommand(
				new SE_changeRubberbandAction( m_rubberband, newRubberband ),
				"SampleEditor::rubberband::crispness"
			);
		}
	);
	pRubberBandWidgetContainerLayout->addWidget( m_pRubberBandCrispnessComboBox
	);

	////////////////////////////////////////////////////////////////////////////

	auto pTargetSeparator = createSeparator( pScrollArea );
	pVBoxLayout->addWidget( pTargetSeparator );

	////////////////////////////////////////////////////////////////////////////

	auto pTargetContainer = new QWidget( pScrollArea );
	pVBoxLayout->addWidget( pTargetContainer );

	auto pTargetContainerLayout = new QHBoxLayout();
	pTargetContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pTargetContainerLayout->setSpacing( SampleEditor::nSpacing );
	pTargetContainer->setLayout( pTargetContainerLayout );

	addHeading(
		tr( "Resulting Sample and Envelopes" ), pTargetContainerLayout
	);

	pTargetContainerLayout->addStretch();

	auto pNewLengthLabel = new QLabel( pTargetContainer );
	pNewLengthLabel->setText( tr( "new sample length:" ) );
	pTargetContainerLayout->addWidget( pNewLengthLabel );

	m_pNewLengthDisplay =
		new LCDDisplay( pTargetContainer, QSize(), false, false );
	m_pNewLengthDisplay->setFixedWidth( 120 );
	pTargetContainerLayout->addWidget( m_pNewLengthDisplay );

	addSpacer( pTargetContainerLayout );

	m_pEnvelopeComboBox = new QComboBox( pTargetContainer );
	m_pEnvelopeComboBox->setMinimumWidth( 80 );
	m_pEnvelopeComboBox->setToolTip( tr( "fade-out type" ) );
	m_pEnvelopeComboBox->addItems(
		QStringList() << tr( "volume" ) << tr( "panorama" )
	);
	connect(
		m_pEnvelopeComboBox, QOverload<int>::of( &QComboBox::activated ),
		[&]() {
			if ( m_pEnvelopeComboBox->currentIndex() == 0 ) {
				m_envelopeType = EnvelopeType::Velocity;
			}
			else {
				m_envelopeType = EnvelopeType::Pan;
			}
		}
	);
	pTargetContainerLayout->addWidget( m_pEnvelopeComboBox );

	////////////////////////////////////////////////////////////////////////////

	m_pTargetSampleView = new TargetWaveDisplay( this );
	m_pTargetSampleView->setLayer( pPreviewLayer );
	m_pTargetSampleView->setMinimumHeight( 94 );
	pVBoxLayout->addWidget( m_pTargetSampleView );

	////////////////////////////////////////////////////////////////////////////

	const QSize buttonSize(
		SampleEditor::nButtonWidth, SampleEditor::nButtonHeight
	);

	auto pButtonContainer = new QWidget( pScrollArea );
	pVBoxLayout->addWidget( pButtonContainer );

	auto pButtonContainerLayout = new QHBoxLayout();
	pButtonContainerLayout->setContentsMargins( 0, 0, 0, 0 );
	pButtonContainerLayout->setSpacing( SampleEditor::nSpacing );
	pButtonContainer->setLayout( pButtonContainerLayout );

	pButtonContainerLayout->addStretch();

	m_pApplyButton =
		new Button( pButtonContainer, buttonSize, Button::Type::Push );
	m_pApplyButton->setText( pCommonStrings->getButtonApply() );
	connect( m_pApplyButton, &QPushButton::clicked, [&]() {
		QApplication::setOverrideCursor( Qt::WaitCursor );

		auto pHydrogenApp = HydrogenApp::get_instance();
		const auto pCommonStrings = pHydrogenApp->getCommonStrings();
		auto pHydrogen = H2Core::Hydrogen::get_instance();
		auto pAudioEngine = pHydrogen->getAudioEngine();

		auto pNewSample = std::make_shared<Sample>(
			m_pSample->getFilePath(), m_pSample->getLicense()
		);
		pNewSample->setLoops( m_loops );
		pNewSample->setRubberband( m_rubberband );
		pNewSample->setVelocityEnvelope( m_velocityEnvelope );
		pNewSample->setPanEnvelope( m_panEnvelope );

		if ( !pNewSample->load( pAudioEngine->getPlayhead()->getBpm() ) ) {
			ERRORLOG( "Unable to load modified sample" );
			return;
		}

		auto pNewInstrument = std::make_shared<Instrument>( m_pInstrument );
		auto pNewComponent =
			pNewInstrument->getComponent( m_pInstrument->index( m_pComponent )
			);
		auto pNewLayer =
			pNewComponent->getLayer( m_pComponent->index( m_pLayer ) );
		if ( pNewInstrument == nullptr || pNewComponent == nullptr ||
			 pNewLayer == nullptr ) {
			ERRORLOG( "Unable to apply changes" );
			return;
		}
		pNewInstrument->setSample(
			pNewComponent, pNewLayer, pNewSample, Event::Trigger::Suppress
		);

		pHydrogenApp->pushUndoCommand( new SE_replaceInstrumentAction(
			pNewInstrument, m_pInstrument,
			SE_replaceInstrumentAction::Type::EditLayer,
			pNewSample->getFileName()
		) );
		setClean();

		QApplication::restoreOverrideCursor();
	} );
	pButtonContainerLayout->addWidget( m_pApplyButton );

	m_pPlayButton =
		new Button( pButtonContainer, buttonSize, Button::Type::Push );
	m_pPlayButton->setText( pCommonStrings->getButtonPlay() );
	connect( m_pPlayButton, &QPushButton::clicked, [&]() {
		if ( m_playback == Playback::None ) {
			startPlayback( Playback::Target );
		}
		else {
			stopPlayback();
		}
	} );
	pButtonContainerLayout->addWidget( m_pPlayButton );

	m_pPlayOriginalButton =
		new Button( pButtonContainer, buttonSize, Button::Type::Push );
	m_pPlayOriginalButton->setText( pCommonStrings->getButtonPlayOriginalSample(
	) );
	connect( m_pPlayOriginalButton, &QPushButton::clicked, [&]() {
		if ( m_playback == Playback::None ) {
			startPlayback( Playback::Original );
		}
		else {
			stopPlayback();
		}
	} );
	pButtonContainerLayout->addWidget( m_pPlayOriginalButton );

	auto pCloseButton =
		new Button( pButtonContainer, buttonSize, Button::Type::Push );
	pCloseButton->setText( tr( "&Close" ) );
	connect( pCloseButton, &QPushButton::clicked, [=]() { close(); } );
	pButtonContainerLayout->addWidget( pCloseButton );

	pButtonContainerLayout->addStretch();

	////////////////////////////////////////////////////////////////////////////

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its
	// size beyond the minimum and make the scrollbars appear.
	setWindowFlags(
		windowFlags() | Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint
	);

	m_pWaveDisplayUpdateTimer = new QTimer( this );
	connect( m_pWaveDisplayUpdateTimer, &QTimer::timeout, [&]() {
		updateTransport();
	} );

	m_pSampleUpdateTimer = new QTimer( this );
	connect( m_pSampleUpdateTimer, &QTimer::timeout, [&]() {
		m_pSampleUpdateTimer->stop();
		updateSample();
	} );

	updateSourceWaveDisplays();

#ifndef H2CORE_HAVE_RUBBERBAND
	if ( !Filesystem::file_executable(
			 pPref->m_sRubberBandCLIexecutable, true /* silent */
		 ) ) {
		pRubberBandContainer->hide();
	}
#else
	pRubberBandContainer->show();
#endif

	reloadLayer();

	updateStyleSheet();

	setClean();

	// Allow all shortcuts to take effect.
	installEventFilter( HydrogenApp::get_instance()->getMainForm() );

	// Listen to core events.
	HydrogenApp::get_instance()->addEventListener( this );
}

SampleEditor::~SampleEditor()
{
	HydrogenApp::get_instance()->removeEventListener( this );
}

void SampleEditor::setHoveredSlider( Slider slider )
{
	if ( m_hoveredSlider == slider ) {
		return;
	}
	m_hoveredSlider = slider;
	m_pSampleWaveDisplayL->update();
	m_pSampleWaveDisplayR->update();
}

void SampleEditor::setSelectedSlider( Slider slider )
{
	if ( m_selectedSlider == slider ) {
		return;
	}
	m_selectedSlider = slider;
	updateSourceWaveDisplays();
}

void SampleEditor::setLoopStartFrame( int nFrame )
{
	const int nFrameClamped =
		std::clamp( nFrame, 0, m_pSampleOriginal->getFrames() - 1 );
	if ( m_loops.nStartFrame == nFrameClamped &&
		 m_selectedSlider == Slider::Start ) {
		return;
	}

	m_loops.nStartFrame = nFrameClamped;

	if ( m_loops.nStartFrame > m_loops.nLoopFrame ) {
		m_loops.nLoopFrame = m_loops.nStartFrame;
	}
	if ( m_loops.nStartFrame > m_loops.nEndFrame ) {
		m_loops.nEndFrame = m_loops.nStartFrame;
	}

	m_pLoopStartFrameSpinBox->setValue(
		nFrameClamped, Event::Trigger::Suppress
	);

	m_selectedSlider = Slider::Start;

	setUnclean();
	triggerSampleUpdate();
	updateSourceWaveDisplays();
}

void SampleEditor::setLoopLoopFrame( int nFrame )
{
	const int nFrameClamped =
		std::clamp( nFrame, 0, m_pSampleOriginal->getFrames() - 1 );
	if ( m_loops.nLoopFrame == nFrameClamped &&
		 m_selectedSlider == Slider::Loop ) {
		return;
	}

	m_loops.nLoopFrame = nFrameClamped;

	if ( m_loops.nLoopFrame < m_loops.nStartFrame ) {
		m_loops.nStartFrame = m_loops.nLoopFrame;
	}
	if ( m_loops.nLoopFrame > m_loops.nEndFrame ) {
		m_loops.nEndFrame = m_loops.nLoopFrame;
	}

	m_pLoopLoopFrameSpinBox->setValue(
		nFrameClamped, Event::Trigger::Suppress
	);

	m_selectedSlider = Slider::Loop;
	setUnclean();
	triggerSampleUpdate();
	updateSourceWaveDisplays();
}

void SampleEditor::setLoopEndFrame( int nFrame )
{
	const int nFrameClamped =
		std::clamp( nFrame, 0, m_pSampleOriginal->getFrames() - 1 );
	if ( m_loops.nEndFrame == nFrameClamped &&
		 m_selectedSlider == Slider::End ) {
		return;
	}

	m_loops.nEndFrame = nFrameClamped;

	if ( m_loops.nEndFrame < m_loops.nStartFrame ) {
		m_loops.nStartFrame = m_loops.nEndFrame;
	}
	if ( m_loops.nEndFrame < m_loops.nLoopFrame ) {
		m_loops.nLoopFrame = m_loops.nEndFrame;
	}

	m_pLoopEndFrameSpinBox->setValue( nFrameClamped, Event::Trigger::Suppress );

	m_selectedSlider = Slider::End;
	setUnclean();
	triggerSampleUpdate();
	updateSourceWaveDisplays();
}

void SampleEditor::setLoops( Sample::Loops newLoops )
{
	m_loops = newLoops;

	m_pLoopStartFrameSpinBox->setValue(
		m_loops.nStartFrame, Event::Trigger::Suppress
	);
	m_pLoopLoopFrameSpinBox->setValue(
		m_loops.nLoopFrame, Event::Trigger::Suppress
	);
	m_pLoopEndFrameSpinBox->setValue(
		m_loops.nEndFrame, Event::Trigger::Suppress
	);
	m_pLoopCountSpinBox->setValue( m_loops.nCount, Event::Trigger::Suppress );
	if ( m_loops.mode == Sample::Loops::Mode::Forward ) {
		m_pLoopModeComboBox->setCurrentIndex( 0 );
	}
	if ( m_loops.mode == Sample::Loops::Mode::Reverse ) {
		m_pLoopModeComboBox->setCurrentIndex( 1 );
	}
	if ( m_loops.mode == Sample::Loops::Mode::PingPong ) {
		m_pLoopModeComboBox->setCurrentIndex( 2 );
	}

	setUnclean();
	triggerSampleUpdate();
	updateSourceWaveDisplays();
}

void SampleEditor::setRubberband( Sample::Rubberband newRubberband )
{
	m_rubberband = newRubberband;

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
	else {
		m_pRubberBandLengthComboBox->setCurrentIndex( 0 );
	}

	m_pRubberBandPitchSpinBox->setValue(
		m_rubberband.fSemitonesToShift, Event::Trigger::Suppress
	);

	m_pRubberBandCrispnessComboBox->setCurrentIndex( m_rubberband.nCrispness );

	triggerSampleUpdate();
	setUnclean();
}

void SampleEditor::editEnvelopePoint(
	H2Core::EnvelopePoint point,
	SampleEditor::EnvelopeType envelopeType,
	Editor::Action action
)
{
	auto envelope = envelopeType == EnvelopeType::Velocity ? &m_velocityEnvelope
														   : &m_panEnvelope;

	int nIndex = -1;
	for ( int ii = 0; ii < envelope->size(); ++ii ) {
		if ( envelope->at( ii ) == point ) {
			nIndex = ii;
			break;
		}
	}

	if ( action == Editor::Action::Add && nIndex != -1 ) {
		ERRORLOG( QString( "Unable to add point [%1]. There is already a point "
						   "present at frame [%1] in "
						   "[%2] envelope" )
					  .arg( point.toQString() )
					  .arg( EnvelopeTypeToQString( envelopeType ) ) );
		return;
	}
	else if ( action == Editor::Action::Delete && nIndex == -1 ) {
		ERRORLOG(
			QString(
				"Unable to delete ooint [%1] from [%2] envelope-> Not found."
			)
				.arg( point.toQString() )
				.arg( EnvelopeTypeToQString( envelopeType ) )
		);
		return;
	}

	if ( action == Editor::Action::Add ||
		 ( action == Editor::Action::Toggle && nIndex == -1 ) ) {
		envelope->push_back( point );
		sort( envelope->begin(), envelope->end(), EnvelopePoint::Comparator() );
	}
	else if ( action == Editor::Action::Delete || ( action == Editor::Action::Toggle && nIndex != 1 ) ) {
		envelope->erase( envelope->begin() + nIndex );
		sort( envelope->begin(), envelope->end(), EnvelopePoint::Comparator() );
	}
	else {
		ERRORLOG( "Nothing to do" );
		return;
	}

	triggerSampleUpdate();
	setUnclean();
}

void SampleEditor::moveEnvelopePoint(
	H2Core::EnvelopePoint oldPoint,
	H2Core::EnvelopePoint newPoint,
	SampleEditor::EnvelopeType envelopeType
)
{
	if ( oldPoint == newPoint ) {
		return;
	}

	auto envelope = envelopeType == EnvelopeType::Velocity ? &m_velocityEnvelope
														   : &m_panEnvelope;

	int nIndex = -1;
	for ( int ii = 0; ii < envelope->size(); ++ii ) {
		if ( envelope->at( ii ) == oldPoint ) {
			nIndex = ii;
			break;
		}
	}

	if ( nIndex == -1 ) {
		ERRORLOG( QString( "Point [%1] not found in [%2] envelope" )
					  .arg( oldPoint.toQString() )
					  .arg( EnvelopeTypeToQString( envelopeType ) ) );
		return;
	}

	envelope->erase( envelope->begin() + nIndex );
	envelope->push_back( newPoint );
	sort( envelope->begin(), envelope->end(), EnvelopePoint::Comparator() );

	// Ensure we only have a single point at a frame.
	for ( int i = 0; i < envelope->size() - 1; ++i ) {
		if ( envelope->at( i ).nFrame == envelope->at( i + 1 ).nFrame ) {
			envelope->erase( envelope->begin() + i );
		}
	}

	triggerSampleUpdate();
	setUnclean();
}

void SampleEditor::drumkitLoadedEvent()
{
	// Most likely the user has undone an "apply to sample" action. Pick our
	// sample from the new drumkit and register the new instrument (which should
	// have the same ID as the former one).
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	const auto pNewInstrument =
		pSong->getDrumkit()->getInstruments()->find( m_pInstrument->getId() );
	if ( pNewInstrument == nullptr ) {
		ERRORLOG( "Unable to find new instrument" );
		return;
	}
	const auto pNewComponent =
		pNewInstrument->getComponent( m_pInstrument->index( m_pComponent ) );
	if ( pNewComponent == nullptr ) {
		ERRORLOG( "Unable to find new component" );
		return;
	}
	const auto pNewLayer =
		pNewComponent->getLayer( m_pComponent->index( m_pLayer ) );
	if ( pNewLayer == nullptr || pNewLayer->getSample() == nullptr ) {
		ERRORLOG( "Invalid new layer" );
		return;
	}
	if ( pNewLayer->getSample()->getFilePath() !=
		 m_pSampleOriginal->getFilePath() ) {
		// That's not our sample. Could happen when the user switches drumkits
		// via keyboard shortcut, MIDI, or OSC command while SampleEditor is
		// still open.
		return;
	}

	m_pInstrument = pNewInstrument;
	m_pComponent = pNewComponent;
	m_pLayer = pNewLayer;

	if ( m_playback == Playback::None ) {
		reloadLayer();
	}
	else {
		// If a sample is playing, we delay wave form update to ensure both
		// audio and peaks are consistent.
		m_bLayerReloadRequired = true;
	}
}

void SampleEditor::closeEvent( QCloseEvent* event )
{
	if ( m_bSampleEditorClean ) {
		accept();
		return;
	}

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QMessageBox messageBox( this );
	messageBox.setWindowTitle( "Hydrogen" );
	messageBox.setText( pCommonStrings->getUnsavedChanges() );
	messageBox.setTextFormat( Qt::RichText );

	auto pDiscardButton = messageBox.addButton(
		pCommonStrings->getButtonDiscard(), QMessageBox::YesRole
	);
	auto pRejectButton = messageBox.addButton(
		pCommonStrings->getButtonCancel(), QMessageBox::RejectRole
	);
	messageBox.exec();

	if ( messageBox.clickedButton() == pDiscardButton ) {
		setClean();
		accept();
	}
	else if ( messageBox.clickedButton() == pRejectButton ) {
		event->ignore();
		return;
	}
}

void SampleEditor::keyPressEvent( QKeyEvent* pKeyEvent )
{
	if ( pKeyEvent->key() == Qt::Key_Escape ) {
		// Close window when hitting ESC.
		pKeyEvent->accept();
		close();
	}
}

void SampleEditor::updateSourceWaveDisplays()
{
	m_pDetailWaveDisplayL->update();
	m_pDetailWaveDisplayR->update();
	m_pSampleWaveDisplayL->update();
	m_pSampleWaveDisplayR->update();
}

void SampleEditor::lockWidgets( bool bLock )
{
	m_pSampleWaveDisplayL->setEnabled( ! bLock );
	m_pSampleWaveDisplayR->setEnabled( ! bLock );
	m_pTargetSampleView->setEnabled( !bLock );

	m_pLoopStartFrameSpinBox->setEnabled( ! bLock );
	m_pLoopLoopFrameSpinBox->setEnabled( ! bLock );
	m_pLoopCountSpinBox->setEnabled( ! bLock );
	m_pLoopModeComboBox->setEnabled( ! bLock );
	m_pLoopEndFrameSpinBox->setEnabled( ! bLock );

	m_pRubberBandLengthComboBox->setEnabled( ! bLock );
	m_pRubberBandRatioLabel->setEnabled( ! bLock );
	m_pRubberBandPitchSpinBox->setEnabled( ! bLock );
	m_pRubberBandCrispnessComboBox->setEnabled( ! bLock );

	m_pApplyButton->setEnabled( ! bLock );
	m_pNewLengthDisplay->setEnabled( ! bLock );
	m_pEnvelopeComboBox->setEnabled( ! bLock );
}

void SampleEditor::setUnclean()
{
	m_bSampleEditorClean = false;
	m_pApplyButton->setDisabled( false );
	setWindowTitle( QString( "%1%2*" )
						.arg( tr( "SampleEditor " ) )
						.arg( m_pSample->getFilePath() ) );
}

void SampleEditor::setClean()
{
	m_bSampleEditorClean = true;
	m_pApplyButton->setDisabled( true );
	setWindowTitle( QString( "%1%2" )
						.arg( tr( "SampleEditor " ) )
						.arg( m_pSample->getFilePath() ) );
}

void SampleEditor::startPlayback( Playback playback )
{
	if ( m_playback != Playback::None ) {
		stopPlayback();
	}
	m_playback = playback;
	lockWidgets( true );

	// Register the sample for playback
	std::shared_ptr<Note> pNote;
	long long nSampleLength;
	if ( m_playback == Playback::Target ) {
		pNote = std::make_shared<Note>(
			m_pPreviewInstrument, 0, VELOCITY_MAX, PAN_DEFAULT,
			LENGTH_ENTIRE_SAMPLE
		);
		nSampleLength = static_cast<long long>( m_pSample->getFrames() );
	}
	else {
		pNote = std::make_shared<Note>(
			m_pPreviewInstrumentOriginal, 0, VELOCITY_MAX, PAN_DEFAULT,
			LENGTH_ENTIRE_SAMPLE
		);
		nSampleLength =
			static_cast<long long>( m_pSampleOriginal->getFrames() );
	}

	// Reset playhead and other widgets and perform one UI refresh before
	// starting the actual rendering.
	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	m_playback = playback;
	m_previousState = pAudioEngine->getState();
	m_selectedSlider = Slider::None;
	m_pPlayButton->setText( tr( "&Stop" ) );
	m_pPlayOriginalButton->setText( tr( "&Stop" ) );

	if ( m_playback == Playback::Original ) {
		m_nPlayheadSample = 0;
		m_fIncrementScaling = 1.0;
	}
	else {
		m_nLoopFrames =
			static_cast<long long>( m_loops.nEndFrame - m_loops.nLoopFrame );
		m_looped = Looped::NotYet;
		m_nPlayheadSample = static_cast<long long>( m_loops.nStartFrame );
		m_nPlayheadTarget = 0;
		m_pTargetSampleView->update();
		m_fIncrementScaling = static_cast<double>(
								  m_loops.nEndFrame - m_loops.nStartFrame +
								  m_loops.nCount * m_nLoopFrames
							  ) /
							  static_cast<double>( m_pSample->getFrames() );
	}
	m_nTotalPlaybackFrames = nSampleLength;
	updateSourceWaveDisplays();

	// Render sample
	pAudioEngine->getSampler()->previewInstrument(
		pNote->getInstrument(), pNote
	);
	m_nLastRealtimeFrame = pAudioEngine->getRealtimeFrame();
	m_nRealtimeFrameEnd = m_nLastRealtimeFrame + nSampleLength;

	m_pWaveDisplayUpdateTimer->start( SampleEditor::nWaveDisplayUpdateInterval
	);
}

void SampleEditor::stopPlayback()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pWaveDisplayUpdateTimer->stop();

	m_pPlayButton->setText( pCommonStrings->getButtonPlay() );
	m_pPlayOriginalButton->setText( pCommonStrings->getButtonPlayOriginalSample(
	) );

	if ( m_playback == Playback::Original ) {
		m_nPlayheadSample = 0;
	}
	else {
		m_nPlayheadSample = static_cast<long long>( m_loops.nStartFrame );
		m_nPlayheadTarget = 0;
		m_pTargetSampleView->update();
	}

	m_playback = Playback::None;
	lockWidgets( false );

	if ( m_bLayerReloadRequired ) {
		m_bLayerReloadRequired = false;
		reloadLayer();
	}

	if ( m_bRetriggerRequired ) {
		m_bRetriggerRequired = false;
		triggerSampleUpdate();
	}

	updateSourceWaveDisplays();
}

void SampleEditor::updateTransport()
{
	if ( m_playback == Playback::None ) {
		stopPlayback();
		return;
	}
	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();

	if ( m_previousState != pAudioEngine->getState() ) {
		WARNINGLOG(
			QString( "Starting/stopping transport during sample rendering is "
					 "not supported." )
		);
		stopPlayback();
		return;
	}

	const auto nRealtimeFrame = pAudioEngine->getRealtimeFrame();
	if ( nRealtimeFrame == m_nLastRealtimeFrame ) {
		return;
	}
	else if ( nRealtimeFrame >= m_nRealtimeFrameEnd ) {
		stopPlayback();
		return;
	}

	const long long nIncrementScaled = static_cast<long long>(
		static_cast<float>( nRealtimeFrame - m_nLastRealtimeFrame ) *
		m_fIncrementScaling
	);
	if ( m_playback == Playback::Original ) {
		m_nPlayheadSample += nRealtimeFrame - m_nLastRealtimeFrame;
	}
	else if ( m_looped == Looped::NotYet &&
              m_nPlayheadSample + nRealtimeFrame -
              m_nLastRealtimeFrame <= m_loops.nEndFrame ) {
		// Still in the initial pass over the sample
		m_nPlayheadSample += nIncrementScaled;
	}
	else {
		// Transport already reached the end of the original sample.
		if ( m_looped == Looped::NotYet ) {
			if ( m_loops.mode == Sample::Loops::Mode::Forward ) {
				m_looped = Looped::Forward;
			}
			else {
				m_looped = Looped::Reverse;
				// Reflect playhead at end marker for a smooth reversing of
				// playback direction.
				m_nPlayheadSample +=
					2 * ( m_loops.nEndFrame - m_nPlayheadSample );
			}
		}

		bool bRequiresLooping = false;
		if ( m_loops.mode == Sample::Loops::Mode::Forward ) {
			if ( m_nPlayheadSample + nIncrementScaled > m_loops.nEndFrame ) {
				const long long nFramesSinceLoopPoint =
					m_nPlayheadSample + nIncrementScaled - m_loops.nLoopFrame;
				m_nPlayheadSample = m_loops.nLoopFrame +
									( nFramesSinceLoopPoint ) % m_nLoopFrames;
			}
			else {
				m_nPlayheadSample += nIncrementScaled;
			}
		}
		else if ( m_loops.mode == Sample::Loops::Mode::Reverse ) {
			if ( m_nPlayheadSample - nIncrementScaled < m_loops.nLoopFrame ) {
				const long long nFramesSinceLoopPoint =
					m_loops.nEndFrame - m_nPlayheadSample + nIncrementScaled;
				m_nPlayheadSample = m_loops.nEndFrame -
									( nFramesSinceLoopPoint ) % m_nLoopFrames;
			}
			else {
				m_nPlayheadSample -= nIncrementScaled;
			}
		}
		else {
			// Ping pong
			if ( m_looped == Looped::Reverse ) {
				// playhead moves backward
				if ( m_nPlayheadSample - nIncrementScaled <
					 m_loops.nLoopFrame ) {
					const long long nRemainingFrames = m_loops.nLoopFrame -
													   m_nPlayheadSample +
													   nIncrementScaled;
					const int nTotalLoops =
						std::floor( nRemainingFrames / m_nLoopFrames );
					if ( nTotalLoops == 0 || nTotalLoops % 2 == 0 ) {
						m_looped = Looped::Forward;
						m_nPlayheadSample = m_loops.nLoopFrame +
											nRemainingFrames % m_nLoopFrames;
					}
					else {
						m_nPlayheadSample = m_loops.nEndFrame -
											nRemainingFrames % m_nLoopFrames;
					}
				}
				else {
					m_nPlayheadSample -= nIncrementScaled;
				}
			}
			else {
				// playhead moves forward
				if ( m_nPlayheadSample + nIncrementScaled >
					 m_loops.nEndFrame ) {
					const long long nRemainingFrames = m_nPlayheadSample +
													   nIncrementScaled -
													   m_loops.nEndFrame;
					const int nTotalLoops =
						std::floor( nRemainingFrames / m_nLoopFrames );
					if ( nTotalLoops == 0 || nTotalLoops % 2 == 0 ) {
						m_looped = Looped::Reverse;
						m_nPlayheadSample = m_loops.nEndFrame -
											nRemainingFrames % m_nLoopFrames;
					}
					else {
						m_nPlayheadSample = m_loops.nLoopFrame +
											nRemainingFrames % m_nLoopFrames;
					}
				}
				else {
					m_nPlayheadSample += nIncrementScaled;
				}
			}
		}
	}

	updateSourceWaveDisplays();
	if ( m_playback == Playback::Target ) {
		m_nPlayheadTarget += nRealtimeFrame - m_nLastRealtimeFrame;
		m_pTargetSampleView->update();
	}

	m_nLastRealtimeFrame = nRealtimeFrame;
}

void SampleEditor::triggerSampleUpdate()
{
	if ( m_pSampleUpdateTimer->isActive() ) {
		m_pSampleUpdateTimer->stop();
	}
	m_pSampleUpdateTimer->start( SampleEditor::nSampleUpdateTimeout );
}

void SampleEditor::updateSample()
{
	if ( m_playback != Playback::None ) {
		// We delay any updates till after playback has finished.
		m_bRetriggerRequired = true;
		return;
	}

	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	auto pEditSample = std::make_shared<Sample>(
		m_pSample->getFilePath(), m_pSample->getLicense()
	);
	pEditSample->setLoops( m_loops );
	pEditSample->setRubberband( m_rubberband );
	pEditSample->setVelocityEnvelope( m_velocityEnvelope );
	pEditSample->setPanEnvelope( m_panEnvelope );

	if ( !pEditSample->load( pAudioEngine->getPlayhead()->getBpm() ) ) {
		ERRORLOG( "Unable to load modified sample" );
		return;
	}

	// Required to ensure we don't mess with the preview instrument while the
	// sampler is still rendering its notes.
	pAudioEngine->lock( RIGHT_HERE );

	m_pPreviewInstrument->setSample(
		m_pPreviewInstrument->getComponents()->front(),
		m_pPreviewInstrument->getComponents()->front()->getLayer( 0 ),
		pEditSample, Event::Trigger::Suppress
	);
	m_pSample = pEditSample;

	pAudioEngine->unlock();

	m_pNewLengthDisplay->setText( QString::number( m_pSample->getFrames() ) );
	checkRubberbandSettings();
	updateSourceWaveDisplays();
	m_pTargetSampleView->setLayer(
		m_pPreviewInstrument->getComponents()->front()->getLayer( 0 )
	);
}

void SampleEditor::reloadLayer()
{
	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );

	m_pSample = std::make_shared<Sample>( m_pLayer->getSample() );
	const auto nFrames = m_pSample->getFrames();

	m_pPreviewInstrument->setSample(
		m_pPreviewInstrument->getComponents()->front(),
		m_pPreviewInstrument->getComponents()->front()->getLayer( 0 ),
		m_pSample, Event::Trigger::Suppress
	);

	pAudioEngine->unlock();

	m_loops = m_pSample->getLoops();
	// Per default all loop frames will be set to zero by Hydrogen. But this is
	// dangerous since just altering start or loop might move them beyond the
	// end.
	if ( m_loops.nStartFrame == 0 && m_loops.nLoopFrame == 0 &&
		 m_loops.nEndFrame == 0 ) {
		m_loops.nEndFrame = m_pSample->getFrames() - 1;
	}
	m_rubberband = m_pSample->getRubberband();

	m_velocityEnvelope.clear();
	if ( m_pSample->getVelocityEnvelope().size() == 0 ) {
		m_velocityEnvelope.push_back( EnvelopePoint( 0, 0 ) );
		m_velocityEnvelope.push_back(
			EnvelopePoint( TargetWaveDisplay::nWidth, 0 )
		);
	}
	else {
		for ( const auto& pt : m_pSample->getVelocityEnvelope() ) {
			m_velocityEnvelope.emplace_back( pt );
		}
	}

	m_panEnvelope.clear();
	if ( m_pSample->getPanEnvelope().size() == 0 ) {
		m_panEnvelope.push_back(
			EnvelopePoint( 0, TargetWaveDisplay::nHeight / 2 )
		);
		m_panEnvelope.push_back( EnvelopePoint(
			TargetWaveDisplay::nWidth, TargetWaveDisplay::nHeight / 2
		) );
	}
	else {
		for ( const auto& pt : m_pSample->getPanEnvelope() ) {
			m_panEnvelope.emplace_back( pt );
		}
	}

	m_pLoopStartFrameSpinBox->setValue(
		m_loops.nStartFrame, Event::Trigger::Suppress
	);
	m_pLoopLoopFrameSpinBox->setValue(
		m_loops.nLoopFrame, Event::Trigger::Suppress
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
	m_pLoopCountSpinBox->setValue( m_loops.nCount, Event::Trigger::Suppress );
	m_pLoopEndFrameSpinBox->setValue(
		m_loops.nEndFrame, Event::Trigger::Suppress
	);

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
	else {
		m_pRubberBandLengthComboBox->setCurrentIndex( 0 );
	}
	m_pRubberBandPitchSpinBox->setValue(
		m_rubberband.fSemitonesToShift, Event::Trigger::Suppress
	);
	m_pRubberBandPitchSpinBox->setEnabled( m_rubberband.bUse );
	m_pRubberBandCrispnessComboBox->setCurrentIndex( m_rubberband.nCrispness );
	m_pRubberBandCrispnessComboBox->setEnabled( m_rubberband.bUse );

	setClean();

	checkRubberbandSettings();
	m_pNewLengthDisplay->setText( QString::number( m_pSample->getFrames() ) );
	updateSourceWaveDisplays();
	m_pTargetSampleView->update();
}

void SampleEditor::checkRubberbandSettings()
{
	const double fSampleRate = static_cast<double>( Hydrogen::get_instance()
														->getAudioEngine()
														->getAudioDriver()
														->getSampleRate() );
	// calculate ratio
	double fRatio;
	if ( m_rubberband.bUse ) {
		fRatio = static_cast<double>( m_pSample->getFrames() ) /
				 static_cast<double>( m_pSampleOriginal->getFrames() );
	}
	else {
		fRatio = 1;
	}

	// my personal ratio quality settings
	// ratios < 0.1 || > 3.0 are bad (red) or experimental sounds
	// ratios > 0.1 && < 0.5 || > 2.0 && < 3.0 are mediocre (yellow)
	// ratios > 0.5 && < 2.0 are good (green)
	//
	//          0.1        0.5               2.0            3.0
	//<---red---[--yellow--[------green------]----yellow----]---red--->

	if ( ( fRatio >= 0.5 ) && ( fRatio <= 2.0 ) ) {
		m_pRubberBandLengthComboBox->setStyleSheet(
			"QComboBox { background-color: green; }"
		);
	}
	else if ( ( fRatio >= 0.1 ) && ( fRatio <= 3.0 ) ) {
		m_pRubberBandLengthComboBox->setStyleSheet(
			"QComboBox { background-color: yellow; }"
		);
	}
	else {
		m_pRubberBandLengthComboBox->setStyleSheet(
			"QComboBox { background-color: red; }"
		);
	}

	if ( !m_rubberband.bUse ) {
		m_pRubberBandLengthComboBox->setStyleSheet( "" );
		m_pRubberBandRatioLabel->setText( "" );
	}
	else {
		m_pRubberBandRatioLabel->setText(
			QString( tr( " RB-Ratio" ) )
				.append( QString( ": %1" ).arg( fRatio ) )
		);
	}

	m_pRubberBandPitchSpinBox->setEnabled( m_rubberband.bUse );
	m_pRubberBandCrispnessComboBox->setEnabled( m_rubberband.bUse );
}

void SampleEditor::updateStyleSheet()
{
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();
	const QColor colorBackground = pColorTheme->m_sampleEditor_backgroundColor;
	const QColor colorFont = pColorTheme->m_sampleEditor_textColor;

	setStyleSheet( QString( "\
QWidget {\
     background-color: %1;                      \
     color: %2;                                 \
}\
" )
					   .arg( colorBackground.name() )
					   .arg( colorFont.name() ) );
}
