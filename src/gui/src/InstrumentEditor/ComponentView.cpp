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

#include "ComponentView.h"

#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Sample.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>

#include "ComponentsEditor.h"
#include "InstrumentEditorPanel.h"
#include "LayerPreview.h"
#include "WaveDisplay.h"
#include "../AudioFileBrowser/AudioFileBrowser.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "../UndoActions.h"
#include "../Widgets/Button.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/Rotary.h"

using namespace H2Core;

ComponentView::ComponentView( QWidget* pParent,
							  std::shared_ptr<InstrumentComponent> pComponent )
	: QWidget( pParent )
	, m_pComponent( pComponent )
	, m_nSelectedLayer( 0 )
	, m_bIsExpanded( true )
{
	setFixedSize( InstrumentEditorPanel::nWidth, ComponentView::nExpandedHeight );
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setObjectName( "ComponentProperties" );

	auto pHeaderWidget = new QWidget( this );
	pHeaderWidget->move( 5, 0 );
	pHeaderWidget->setFixedHeight( ComponentView::nHeaderHeight );
	auto pHBoxHeaderLayout = new QHBoxLayout();
	pHBoxHeaderLayout->setMargin( 0 );
	pHBoxHeaderLayout->setSpacing( 0 );
	pHeaderWidget->setLayout( pHBoxHeaderLayout );

	m_pShowLayersBtn = new Button(
		pHeaderWidget, QSize( 16, 16 ), Button::Type::Push, "minus.svg" );
	connect( m_pShowLayersBtn, &Button::clicked, [&](){
		if ( m_bIsExpanded ) {
			narrow();
		} else {
			expand();
		}
	});
	pHBoxHeaderLayout->addWidget( m_pShowLayersBtn );

	m_pComponentNameLbl = new ClickableLabel(
		pHeaderWidget, QSize( 279 - 27 -27 - 16 - 44, ComponentView::nHeaderHeight - 2 ),
		"", ClickableLabel::Color::Bright, true );
	connect( m_pComponentNameLbl, SIGNAL( labelClicked(ClickableLabel*) ),
			 this, SLOT( renameComponentAction() ) );
	pHBoxHeaderLayout->addWidget( m_pComponentNameLbl );

	m_pComponentMuteBtn = new Button(
		pHeaderWidget,
		QSize( ComponentView::nHeaderHeight - 2, ComponentView::nHeaderHeight - 2 ),
		Button::Type::Toggle, "",
		pCommonStrings->getSmallMuteButton(), true, QSize(), tr("Mute component"),
		false, true );
	m_pComponentMuteBtn->setChecked( pComponent->getIsMuted() );
	m_pComponentMuteBtn->setObjectName( "SidebarRowMuteButton" );
	connect( m_pComponentMuteBtn, &Button::clicked, [&](){
		if ( m_pComponent != nullptr ) {
			m_pComponent->setIsMuted( m_pComponentMuteBtn->isChecked() );
		}
	});
	pHBoxHeaderLayout->addWidget( m_pComponentMuteBtn );

	m_pComponentSoloBtn = new Button(
		pHeaderWidget,
		QSize( ComponentView::nHeaderHeight - 2, ComponentView::nHeaderHeight - 2 ),
		Button::Type::Toggle, "",
		pCommonStrings->getSmallSoloButton(), false, QSize(), tr("Solo component"),
		false, true );
	m_pComponentSoloBtn->setChecked( pComponent->getIsSoloed() );
	m_pComponentSoloBtn->setObjectName( "SidebarRowSoloButton" );
	connect( m_pComponentSoloBtn, &Button::clicked, [&](){
		if ( m_pComponent != nullptr ) {
			m_pComponent->setIsSoloed( m_pComponentSoloBtn->isChecked() );
		}
	});
	pHBoxHeaderLayout->addWidget( m_pComponentSoloBtn );

	m_pComponentGainRotary = new Rotary(
		pHeaderWidget, Rotary::Type::Normal, tr( "Component volume" ), false,
		0.0, 5.0 );
	m_pComponentGainRotary->setDefaultValue( 1.0 );
	connect( m_pComponentGainRotary, &Rotary::valueChanged, [&]() {
		if ( m_pComponent != nullptr ) {
			m_pComponent->setGain( m_pComponentGainRotary->getValue() );
			updateView(); // WaveDisplay update
		}
	});
	pHBoxHeaderLayout->addWidget( m_pComponentGainRotary );

	// Layer preview
	m_pLayerPreview = new LayerPreview( this );

	m_pLayerScrollArea = new QScrollArea( this );
	m_pLayerScrollArea->setFrameShape( QFrame::NoFrame );
	m_pLayerScrollArea->move( 6, 44 );
	m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	if ( InstrumentComponent::getMaxLayers() > 16 ) {
		m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	}
	m_pLayerScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pLayerScrollArea->setMaximumHeight( 182 );
	m_pLayerScrollArea->setWidget( m_pLayerPreview  );


	// Waveform display
	m_pWaveDisplay = new WaveDisplay( this );
	m_pWaveDisplay->resize( 277, 58 );
	m_pWaveDisplay->updateDisplay( nullptr );
	m_pWaveDisplay->move( 5, 241 );
	connect( m_pWaveDisplay, SIGNAL( doubleClicked(QWidget*) ),
			 this, SLOT( waveDisplayDoubleClicked(QWidget*) ) );

	m_pLoadLayerBtn = new Button(
		this, QSize( 92, 18 ), Button::Type::Push, "",
		pCommonStrings->getLoadLayerButton() );
	m_pLoadLayerBtn->setObjectName( "LoadLayerButton" );
	m_pLoadLayerBtn->move( 5, 304 );

	m_pRemoveLayerBtn = new Button(
		this, QSize( 94, 18 ), Button::Type::Push, "",
		pCommonStrings->getDeleteLayerButton() );
	m_pRemoveLayerBtn->setObjectName( "RemoveLayerButton" );
	m_pRemoveLayerBtn->move( 97, 304 );

	m_pSampleEditorBtn = new Button(
		this, QSize( 92, 18 ), Button::Type::Push, "",
		pCommonStrings->getEditLayerButton() );
	m_pSampleEditorBtn->setObjectName( "SampleEditorButton" );
	m_pSampleEditorBtn->move( 191, 304 );

	connect( m_pLoadLayerBtn, SIGNAL( clicked() ),
			 this, SLOT( loadLayerBtnClicked() ) );
	connect( m_pRemoveLayerBtn, SIGNAL( clicked() ),
			 this, SLOT( removeLayerButtonClicked() ) );
	connect( m_pSampleEditorBtn, SIGNAL( clicked() ),
			 this, SLOT( showSampleEditor() ) );
	// Layer gain
	m_pLayerGainLCD = new LCDDisplay( this, QSize( 36, 16 ), false, false );
	m_pLayerGainRotary = new Rotary(
		this, Rotary::Type::Normal, tr( "Layer gain" ), false , 0.0, 5.0);
	m_pLayerGainRotary->setDefaultValue( 1.0 );
	connect( m_pLayerGainRotary, &Rotary::valueChanged, [&]() {
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setGain( m_pLayerGainRotary->getValue() );
				updateView(); // LCD update
			}
		}
	});
	m_pLayerGainLbl = new ClickableLabel(
		this, QSize( 44, 10 ), pCommonStrings->getLayerGainLabel() );
	m_pLayerGainLbl->move( 50, 360 );

	m_pLayerPitchCoarseLCD = new LCDDisplay(
		this, QSize( 28, 16 ), false, false );
	m_pLayerPitchFineLCD = new LCDDisplay(
		this, QSize( 28, 16 ), false, false );
	m_pLayerPitchLbl = new ClickableLabel(
		this, QSize( 45, 10 ), pCommonStrings->getPitchLabel() );
	m_pLayerPitchLbl->move( 17, 412 );

	m_pLayerPitchCoarseRotary = new Rotary(
		this, Rotary::Type::Center, tr( "Layer pitch (Coarse)" ), true,
		Instrument::fPitchMin + InstrumentEditorPanel::nPitchFineControl,
		Instrument::fPitchMax - InstrumentEditorPanel::nPitchFineControl );
	connect( m_pLayerPitchCoarseRotary, &Rotary::valueChanged, [&]() {
		const float fNewPitch = round( m_pLayerPitchCoarseRotary->getValue() ) +
			m_pLayerPitchFineRotary->getValue() / 100.0;
		if ( m_pComponent != nullptr ) {
			auto pLayer = pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setPitch( fNewPitch );
				updateView(); // LCD update
			}
		}
	});
	m_pLayerPitchCoarseLbl = new ClickableLabel(
		this, QSize( 44, 10 ), pCommonStrings->getPitchCoarseLabel() );
	m_pLayerPitchCoarseLbl->move( 61, 412 );

	m_pLayerPitchFineRotary = new Rotary(
		this, Rotary::Type::Center, tr( "Layer pitch (Fine)" ), true,
		-50.0, 50.0 );
	connect( m_pLayerPitchFineRotary, &Rotary::valueChanged, [&]() {
		const float fNewPitch = round( m_pLayerPitchCoarseRotary->getValue() ) +
			m_pLayerPitchFineRotary->getValue() / 100.0;
		if ( m_pComponent != nullptr ) {
			auto pLayer = pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setPitch( fNewPitch );
				updateView(); // LCD update
			}
		}
	});
	m_pLayerPitchFineLbl = new ClickableLabel(
		this, QSize( 44, 10 ), pCommonStrings->getPitchFineLabel() );
	m_pLayerPitchFineLbl->move( 147, 412 );

	m_pLayerGainLCD->move( 53, 343 );
	m_pLayerGainRotary->move( 94, 341 );

	m_pLayerPitchCoarseLCD->move( 70, 393 );
	m_pLayerPitchCoarseRotary->move( 105, 391 );

	m_pLayerPitchFineLCD->move( 155, 393 );
	m_pLayerPitchFineRotary->move( 191, 391 );

	m_pSampleSelectionCombo = new LCDCombo(
		this, QSize( width() - 76 - 7, 18 ), true );
	m_pSampleSelectionCombo->move( 76, 432 );

	m_pSampleSelectionCombo->setToolTip( tr( "Select selection algorithm" ) );
	setupSampleSelectionCombo();
	connect( m_pSampleSelectionCombo, SIGNAL( activated( int ) ),
			 this, SLOT( sampleSelectionChanged( int ) ) );
	m_pSampleSelectionLbl = new ClickableLabel(
		this, QSize( 70, 10 ), pCommonStrings->getSampleSelectionLabel() );
	m_pSampleSelectionLbl->move( 7, 436 );

	updateView();
}

ComponentView::~ComponentView() {
}

void ComponentView::updateView() {
	updateActivation();

	if ( m_pComponent != nullptr ) {
		m_pComponentNameLbl->setText( m_pComponent->getName() );
		m_pComponentGainRotary->setValue(
			m_pComponent->getGain(), false, Event::Trigger::Suppress );

		if ( m_nSelectedLayer >= 0 ) {
			const auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );

			if ( pLayer != nullptr ) {
				// Layer GAIN
				m_pLayerGainRotary->setValue( pLayer->getGain(), false,
											  Event::Trigger::Suppress );
				m_pLayerGainLCD->setText(
					QString( "%1" ).arg( pLayer->getGain(), -2, 'f', 2, '0' ) );


				// Layer PITCH
				//
				// For most X.5 values we prefer to round the digit before point
				// up and set the fine value to -0.5. But this is not possible
				// for the maximum value and we have to ensure not to introduce
				// sudden jumps in the fine pitch rotary.
				float fCoarseLayerPitch;
				if ( ( m_pLayerPitchFineRotary->getValue() == 50 &&
					   pLayer->getPitch() -
					   trunc( pLayer->getPitch() ) == 0.5  ) ||
					 pLayer->getPitch() == Instrument::fPitchMax ) {
					fCoarseLayerPitch = trunc( pLayer->getPitch() );
				}
				else if ( m_pLayerPitchFineRotary->getValue() == -50 &&
						  trunc( pLayer->getPitch() ) -
						  pLayer->getPitch() == 0.5 ) {
					fCoarseLayerPitch = ceil( pLayer->getPitch() );
				}
				else {
					fCoarseLayerPitch = round( pLayer->getPitch() );
				}

				const float fFineLayerPitch =
					pLayer->getPitch() - fCoarseLayerPitch;
				m_pLayerPitchCoarseRotary->setValue( fCoarseLayerPitch, false,
													 Event::Trigger::Suppress );
				m_pLayerPitchFineRotary->setValue( fFineLayerPitch * 100, false,
												   Event::Trigger::Suppress );

				m_pLayerPitchCoarseLCD->setText(
					QString( "%1" ).arg( (int) fCoarseLayerPitch ) );
				m_pLayerPitchFineLCD->setText(
					QString( "%1" ).arg( fFineLayerPitch * 100, 0, 'f', 0 ) );

				m_pWaveDisplay->updateDisplay( pLayer );
			}
		}
	}

	m_pLayerPreview->update();
}

void ComponentView::expand() {
	if ( m_bIsExpanded ) {
		return;
	}

	m_pShowLayersBtn->setIconFileName( "minus.svg" );

	setFixedHeight( ComponentView::nExpandedHeight );

	m_bIsExpanded = true;
	updateVisibility();
}

void ComponentView::narrow() {
	if ( ! m_bIsExpanded ) {
		return;
	}

	m_pShowLayersBtn->setIconFileName( "plus.svg" );

	setFixedHeight( ComponentView::nHeaderHeight );

	m_bIsExpanded = false;
	updateVisibility();
}

void ComponentView::setComponent(std::shared_ptr<H2Core::InstrumentComponent> pComponent) {
	if ( m_pComponent != pComponent ) {
		m_pComponent = pComponent;
		m_nSelectedLayer = std::clamp(
			m_nSelectedLayer, 0, H2Core::InstrumentComponent::getMaxLayers() );
	}
}

void ComponentView::renameComponentAction() {
	if ( m_pComponent == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	const QString sOldName = m_pComponent->getName();
	bool bIsOkPressed;
	const QString sNewName = QInputDialog::getText(
		this, "Hydrogen", tr( "New component name" ), QLineEdit::Normal,
		sOldName, &bIsOkPressed );

	if ( bIsOkPressed && sOldName != sNewName ) {
		 pHydrogenApp->pushUndoCommand(
			 new SE_renameComponentAction(
				 sNewName, sOldName,
				 HydrogenApp::get_instance()->getInstrumentRack()->
				 getInstrumentEditorPanel()->getComponentsEditor()->
				 getSelectedComponent() ) );
		 pHydrogenApp->showStatusBarMessage(
			 QString( "%1: [%2] -> [%3]" )
					 .arg( pCommonStrings->getActionRenameComponent() )
					 .arg( sOldName ).arg( sNewName ) );
	}
}

void ComponentView::updateActivation() {
	if ( m_pComponent != nullptr ) {
		m_pComponentGainRotary->setIsActive( true );
	}
	else {
		m_pComponentNameLbl->setText( "" );
		m_pComponentGainRotary->setIsActive( false );
	}

	std::shared_ptr<InstrumentLayer> pLayer = nullptr;
	if ( m_pComponent != nullptr && m_nSelectedLayer >= 0 ) {
		pLayer = m_pComponent->getLayer( m_nSelectedLayer );
	}

	if ( pLayer != nullptr ) {
		m_pLayerGainRotary->setIsActive( true );
		m_pLayerPitchCoarseRotary->setIsActive( true );
		m_pLayerPitchFineRotary->setIsActive( true );

		m_pRemoveLayerBtn->setIsActive( true );
		m_pSampleEditorBtn->setIsActive( true );

		m_pWaveDisplay->updateDisplay( nullptr );
	}
	else {
		m_pLayerGainRotary->setIsActive( false );
		m_pLayerGainLCD->setText( "" );

		// Layer PITCH
		m_pLayerPitchCoarseRotary->setIsActive( false );
		m_pLayerPitchFineRotary->setIsActive( false );
		m_pLayerPitchCoarseLCD->setText( "" );
		m_pLayerPitchFineLCD->setText( "" );

		m_pRemoveLayerBtn->setIsActive( false );
		m_pSampleEditorBtn->setIsActive( false );

		m_pWaveDisplay->updateDisplay( nullptr );
	}
}

void ComponentView::updateVisibility() {
	m_pLayerPreview->setVisible( m_bIsExpanded );
	m_pLayerGainRotary->setVisible( m_bIsExpanded );
	m_pLayerGainLbl->setVisible( m_bIsExpanded );
	m_pLayerGainLCD->setVisible( m_bIsExpanded );
	m_pLayerPitchCoarseRotary->setVisible( m_bIsExpanded );
	m_pLayerPitchCoarseLbl->setVisible( m_bIsExpanded );
	m_pLayerPitchCoarseLCD->setVisible( m_bIsExpanded );
	m_pLayerPitchFineRotary->setVisible( m_bIsExpanded );
	m_pLayerPitchFineLbl->setVisible( m_bIsExpanded );
	m_pLayerPitchFineLCD->setVisible( m_bIsExpanded );
	m_pLayerPitchLbl->setVisible( m_bIsExpanded );

	m_pLoadLayerBtn->setVisible( m_bIsExpanded );
	m_pRemoveLayerBtn->setVisible( m_bIsExpanded );
	m_pSampleEditorBtn->setVisible( m_bIsExpanded );
	m_pSampleSelectionCombo->setVisible( m_bIsExpanded );
	m_pSampleSelectionLbl->setVisible( m_bIsExpanded );

	m_pWaveDisplay->setVisible( m_bIsExpanded );
}

void ComponentView::waveDisplayDoubleClicked( QWidget* pRef ) {
	if ( m_pComponent == nullptr ) {
		return;
	}
			
	auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
	if ( pLayer != nullptr ) {
		showSampleEditor();
	}
	else {
		loadLayerBtnClicked();
	}
}

void ComponentView::showSampleEditor() {
	if ( m_pComponent == nullptr ) {
		return;
	}

	auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
	if ( pLayer == nullptr ) {
		return;
	}

	auto pSample = pLayer->getSample();
	if ( pSample != nullptr ) {
		auto pHydrogenApp = HydrogenApp::get_instance();
		pHydrogenApp->showSampleEditor(
			pSample->getFilepath(),
			pHydrogenApp->getInstrumentRack()->getInstrumentEditorPanel()->
			getComponentsEditor()->getSelectedComponent(), m_nSelectedLayer );
	}
}

void ComponentView::removeLayerButtonClicked() {
	if ( m_pComponent == nullptr ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	m_pComponent->setLayer( nullptr, m_nSelectedLayer );

	pHydrogen->getAudioEngine()->unlock();

	pHydrogen->setIsModified( true );

	// Select next loaded layer - if available - in order to allow for a quick
	// removal of all layers. In case the last layer was removed, the previous
	// one will be selected.
	int nNextLayerIndex = 0;
	int nCount = 0;
	for ( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
		auto pLayer = m_pComponent->getLayer( n );
		if ( pLayer != nullptr ){
			nCount++;

			if ( nNextLayerIndex <= m_nSelectedLayer &&
				 n != m_nSelectedLayer ) {
				nNextLayerIndex = n;
			}
		}
	}

	setSelectedLayer( nCount );
	updateView();
}

void ComponentView::loadLayerBtnClicked() {
	if ( m_pComponent == nullptr ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();

	QString sPath = Preferences::get_instance()->getLastOpenLayerDirectory();
	QString sFilename = "";
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = QDir::homePath();
	}

	// In case the button was pressed while a layer was selected, we
	// try to use path of the associated sample as default one.
	if ( m_nSelectedLayer > 0 ) {
		auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );

		if ( pLayer != nullptr ) {
			auto pSample = pLayer->getSample();

			if ( pSample != nullptr ) {
				if ( ! pSample->getFilepath().isEmpty() ) {
					QFileInfo fileInfo( pSample->getFilepath() );
					sPath = fileInfo.absoluteDir().absolutePath();
					sFilename = fileInfo.absoluteFilePath();
				}
			}
		}
	}

	AudioFileBrowser *pFileBrowser =
		new AudioFileBrowser( nullptr, true, true, sPath, sFilename );
	QStringList filename;
	filename << "false" << "false" << "";

	if (pFileBrowser->exec() == QDialog::Accepted) {
		filename = pFileBrowser->getSelectedFiles();
		
		// Only overwrite the default directory if we didn't start
		// from an existing file or the final directory differs from
		// the starting one.
		if ( sFilename.isEmpty() ||
			 sPath != pFileBrowser->getSelectedDirectory() ) {
			Preferences::get_instance()->setLastOpenLayerDirectory(
				pFileBrowser->getSelectedDirectory() );
		}
	}

	delete pFileBrowser;

	if ( filename[2].isEmpty() ) {
		return;
	}

	auto pInstrument = HydrogenApp::get_instance()->getInstrumentRack()->
		getInstrumentEditorPanel()->getInstrument();
	bool fnc = false;
	if ( filename[0] ==  "true" ){
		fnc = true;
	}

	int nLastInsertedLayer = m_nSelectedLayer;
	if ( filename.size() > 2 ) {
		for ( int ii = 2; ii < filename.size(); ++ii ) {
			int nnLayer = m_nSelectedLayer + ii - 2;
			if ( ( ii - 2 >= InstrumentComponent::getMaxLayers() ) ||
				 ( nnLayer + 1  > InstrumentComponent::getMaxLayers() ) ) {
				break;
			}

			auto pNewSample = Sample::load( filename[ii] );

			pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

			// If we're using multiple layers, we start inserting the first
			// layer at nSelectedLayer and the next layer at nSelectedLayer + 1.
			auto pLayer = m_pComponent->getLayer( nnLayer );
			if ( pLayer != nullptr ) {
				// insert new sample from newInstrument, old sample gets deleted
				// by setSample
				pLayer->setSample( pNewSample );
			}
			else {
				pLayer = std::make_shared<H2Core::InstrumentLayer>( pNewSample );
				m_pComponent->setLayer( pLayer, nnLayer );
			}
			nLastInsertedLayer = nnLayer;

			if ( fnc ){
				QString newFilename = filename[ii].section( '/', -1 );
				newFilename.replace( "." + newFilename.section( '.', -1 ), "");
				pInstrument->setName( newFilename );
			}

			//set automatic velocity
			if ( filename[1] ==  "true" ){
				setAutoVelocity();
			}

			pHydrogen->getAudioEngine()->unlock();
		}

		pHydrogen->setIsModified( true );
	}

	setSelectedLayer( nLastInsertedLayer );
	updateView();
}

void ComponentView::setAutoVelocity() {
	if ( m_pComponent == nullptr ) {
		return;
	}

	const int nLayers = m_pComponent->getLayers().size();
	if ( nLayers == 0 ) {
		ERRORLOG( "There are no layers in the selected component" );
		return;
	}

	const float fVelocityRange = 1.0 / nLayers;

	int nLayer = 0;
	for ( auto& ppLayer : m_pComponent->getLayers() ) {
		if ( ppLayer != nullptr ) {
			ppLayer->setStartVelocity( nLayer * fVelocityRange );
			ppLayer->setEndVelocity( nLayer * fVelocityRange + fVelocityRange );
			
			++nLayer;
		}
	}
}

void ComponentView::sampleSelectionChanged( int selected ) {
	auto pInstrument = HydrogenApp::get_instance()->getInstrumentRack()->
		getInstrumentEditorPanel()->getInstrument();
	 if ( pInstrument == nullptr ) {
		 return;
	 }

	 if ( m_pSampleSelectionCombo->count() < 3 ) {
		 // We flushed the widget while disabling the ComponentView and are
		 // in the process of refilling it.
		 return;
	 }

	if ( selected == 0 ){
		pInstrument->setSampleSelectionAlg( Instrument::VELOCITY );
	}
	else if ( selected == 1 ){
		pInstrument->setSampleSelectionAlg( Instrument::ROUND_ROBIN );
	}
	else if ( selected == 2){
		pInstrument->setSampleSelectionAlg( Instrument::RANDOM );
	}
}

void ComponentView::setupSampleSelectionCombo() {
	m_pSampleSelectionCombo->clear();
	/*: Sample selection algorithm available in the instrument editor */
	m_pSampleSelectionCombo->addItem( tr( "First in Velocity" ) );
	/*: Sample selection algorithm available in the instrument editor */
	m_pSampleSelectionCombo->addItem( tr( "Round Robin" ) );
	/*: Sample selection algorithm available in the instrument editor */
	m_pSampleSelectionCombo->addItem( tr( "Random" ) );
}
