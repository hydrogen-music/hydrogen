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

#include "InstrumentEditorPanel.h"
#include "LayerPreview.h"
#include "WaveDisplay.h"
#include "../AudioFileBrowser/AudioFileBrowser.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "../Skin.h"
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
	setFixedWidth( InstrumentRack::nWidth );
	setMinimumHeight( ComponentView::nHeaderHeight );
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setObjectName( "ComponentProperties" );

	auto pVBoxMainLayout = new QVBoxLayout();
	pVBoxMainLayout->setContentsMargins( 0, 0, Skin::nScrollBarWidth, 0 );
;
	pVBoxMainLayout->setSpacing( 0 );

	auto pHeaderWidget = new QWidget( this );
	pHeaderWidget->setFixedHeight( ComponentView::nHeaderHeight );
	pHeaderWidget->setObjectName( "HeaderWidget" );
	auto pHBoxHeaderLayout = new QHBoxLayout();
	pHBoxHeaderLayout->setSpacing( 0 );
	pHBoxHeaderLayout->setContentsMargins(
		ComponentView::nMargin, 0, ComponentView::nMargin, 0 );
	pHeaderWidget->setLayout( pHBoxHeaderLayout );

	m_pShowLayersBtn = new Button(
		pHeaderWidget, QSize( ComponentView::nExpansionButtonWidth,
							  ComponentView::nExpansionButtonWidth ),
		Button::Type::Push, "minus.svg", "", false,
		QSize( ComponentView::nExpansionButtonWidth - 4,
			   ComponentView::nExpansionButtonWidth - 4 ) );
	connect( m_pShowLayersBtn, &Button::clicked, [&](){
		if ( m_bIsExpanded ) {
			collapse();
		} else {
			expand();
		}
		emit expandedOrCollapsed();
	});
	pHBoxHeaderLayout->addWidget( m_pShowLayersBtn );

	m_pComponentNameLbl = new ClickableLabel(
		pHeaderWidget, QSize( ComponentView::nWidth -
							  ComponentView::nButtonWidth * 2 -
							  ComponentView::nExpansionButtonWidth -
							  Rotary::nWidth - ComponentView::nMargin * 2,
							  ComponentView::nHeaderHeight - 2 ),
		"", ClickableLabel::Color::Bright, true );
	m_pComponentNameLbl->setObjectName( "ComponentName" );
	connect( m_pComponentNameLbl, SIGNAL( labelDoubleClicked(QMouseEvent*) ),
			 this, SLOT( renameComponentAction() ) );
	pHBoxHeaderLayout->addWidget( m_pComponentNameLbl );

	m_pComponentMuteBtn = new Button(
		pHeaderWidget,
		QSize( ComponentView::nButtonWidth, ComponentView::nButtonHeight ),
		Button::Type::Toggle, "",
		pCommonStrings->getSmallMuteButton(), true, QSize(), tr("Mute component"),
		false, true );
	m_pComponentMuteBtn->setChecked( pComponent->getIsMuted() );
	m_pComponentMuteBtn->setObjectName( "ComponentMuteButton" );
	connect( m_pComponentMuteBtn, &Button::clicked, [&](){
		if ( m_pComponent != nullptr ) {
			m_pComponent->setIsMuted( m_pComponentMuteBtn->isChecked() );
		}
	});
	pHBoxHeaderLayout->addWidget( m_pComponentMuteBtn );

	m_pComponentSoloBtn = new Button(
		pHeaderWidget,
		QSize( ComponentView::nButtonWidth, ComponentView::nButtonHeight ),
		Button::Type::Toggle, "",
		pCommonStrings->getSmallSoloButton(), false, QSize(), tr("Solo component"),
		false, true );
	m_pComponentSoloBtn->setChecked( pComponent->getIsSoloed() );
	m_pComponentSoloBtn->setObjectName( "ComponentSoloButton" );
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
		}
	});
	pHBoxHeaderLayout->addWidget( m_pComponentGainRotary );

	// Expanded elements

	m_pComponentWidget = new QWidget( this );
	m_pComponentWidget->setObjectName( "ComponentWidget" );
	auto pVBoxComponentLayout = new QVBoxLayout( this );
	pVBoxComponentLayout->setSpacing( ComponentView::nVerticalSpacing );
	pVBoxComponentLayout->setContentsMargins(
		ComponentView::nMargin, ComponentView::nMargin,
		ComponentView::nMargin, ComponentView::nMargin );
	m_pComponentWidget->setLayout( pVBoxComponentLayout );

	// Layer preview

	m_pLayerPreview = new LayerPreview( this );

	m_pLayerScrollArea = new QScrollArea( m_pComponentWidget );
	m_pLayerScrollArea->setFrameShape( QFrame::NoFrame );
	m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	if ( InstrumentComponent::getMaxLayers() > 16 ) {
		m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	}
	m_pLayerScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pLayerScrollArea->setWidget( m_pLayerPreview  );

	// Buttons to manipulate the current layer.

	auto pLayerButtonWidget = new QWidget( m_pComponentWidget );
	pLayerButtonWidget->setObjectName( "LayerButtonWidget" );
	pLayerButtonWidget->setFixedHeight( ComponentView::nLayerButtonsHeight );

	auto pHBoxLayerButtonLayout = new QHBoxLayout();
	pHBoxLayerButtonLayout->setSpacing( 0 );
	pHBoxLayerButtonLayout->setMargin( 0 );
	pLayerButtonWidget->setLayout( pHBoxLayerButtonLayout );

	const int nButtonWidth = static_cast<int>(
		std::floor( static_cast<float>(ComponentView::nWidth -
									   ComponentView::nMargin) / 3. ));

	m_pLoadLayerBtn = new Button(
		pLayerButtonWidget,
		QSize( nButtonWidth, ComponentView::nLayerButtonsHeight ),
		Button::Type::Push, "", pCommonStrings->getLoadLayerButton() );
	m_pLoadLayerBtn->setObjectName( "LoadLayerButton" );
	connect( m_pLoadLayerBtn, SIGNAL( clicked() ),
			 this, SLOT( loadLayerBtnClicked() ) );
	pHBoxLayerButtonLayout->addWidget( m_pLoadLayerBtn );

	// This button might be slightly bigger (in order to ensure we fill the
	// available space and since its English string is the longest).
	m_pRemoveLayerBtn = new Button(
		pLayerButtonWidget, QSize(
			ComponentView::nWidth - 2 * ComponentView::nMargin -
			2 * nButtonWidth, ComponentView::nLayerButtonsHeight ),
		Button::Type::Push, "", pCommonStrings->getDeleteLayerButton() );
	m_pRemoveLayerBtn->setObjectName( "RemoveLayerButton" );
	connect( m_pRemoveLayerBtn, SIGNAL( clicked() ),
			 this, SLOT( removeLayerButtonClicked() ) );
	pHBoxLayerButtonLayout->addWidget( m_pRemoveLayerBtn );

	m_pSampleEditorBtn = new Button(
		pLayerButtonWidget,
		QSize( nButtonWidth, ComponentView::nLayerButtonsHeight ),
		Button::Type::Push, "", pCommonStrings->getEditLayerButton() );
	m_pSampleEditorBtn->setObjectName( "SampleEditorButton" );
	connect( m_pSampleEditorBtn, SIGNAL( clicked() ),
			 this, SLOT( showSampleEditor() ) );
	pHBoxLayerButtonLayout->addWidget( m_pSampleEditorBtn );

	// Sample selection

	auto pSampleSelectionWidget = new QWidget( m_pComponentWidget );
	pSampleSelectionWidget->setFixedHeight(
		ComponentView::nSampleSelectionHeight );
	pSampleSelectionWidget->setObjectName( "SampleSelectionWidget" );
	auto pHBoxSampleSelectionLayout = new QHBoxLayout();
	pHBoxSampleSelectionLayout->setSpacing( 0 );
	pHBoxSampleSelectionLayout->setMargin( 0 );
	pHBoxSampleSelectionLayout->setAlignment( Qt::AlignVCenter );
	pSampleSelectionWidget->setLayout( pHBoxSampleSelectionLayout );

	m_pSampleSelectionLbl = new ClickableLabel(
		pSampleSelectionWidget, QSize( 70, 10 ),
		pCommonStrings->getSampleSelectionLabel() );
	m_pSampleSelectionLbl->setObjectName( "SampleSelectionLabel" );
	pHBoxSampleSelectionLayout->addWidget( m_pSampleSelectionLbl );

	m_pSampleSelectionCombo = new LCDCombo(
		pSampleSelectionWidget, QSize( 0, 0 ), true );
	m_pSampleSelectionCombo->setFixedHeight(
		ComponentView::nSampleSelectionHeight );
	m_pSampleSelectionCombo->setToolTip( tr( "Select selection algorithm" ) );
	setupSampleSelectionCombo();
	connect( m_pSampleSelectionCombo, SIGNAL( activated( int ) ),
			 this, SLOT( sampleSelectionChanged( int ) ) );
	pHBoxSampleSelectionLayout->addWidget( m_pSampleSelectionCombo );

	// Layer-specific widgets

	m_pLayerWidget = new QWidget( this );
	m_pLayerWidget->setObjectName( "LayerWidget" );
	auto pVBoxLayerLayout = new QVBoxLayout( this );
	pVBoxLayerLayout->setSpacing( ComponentView::nVerticalSpacing );
	pVBoxLayerLayout->setContentsMargins(
		ComponentView::nMargin, ComponentView::nMargin,
		ComponentView::nMargin, ComponentView::nMargin );
	m_pLayerWidget->setLayout( pVBoxLayerLayout );

	// Waveform display

	m_pWaveDisplay = new WaveDisplay( m_pLayerWidget );
	m_pWaveDisplay->setMinimumSize(
		ComponentView::nWidth - ComponentView::nMargin * 2,
		ComponentView::nWaveDisplayHeight );
	m_pWaveDisplay->updateDisplay( nullptr );
	connect( m_pWaveDisplay, SIGNAL( doubleClicked(QWidget*) ),
			 this, SLOT( waveDisplayDoubleClicked(QWidget*) ) );

	// Layer properties

	auto pLayerPropWidget = new QWidget( m_pLayerWidget );
	pLayerPropWidget->setFixedHeight(
		Rotary::nHeight + ComponentView::nLabelHeight );
	pLayerPropWidget->setObjectName( "LayerPropWidget" );
	auto pGridLayerPropLayout = new QGridLayout();
	pGridLayerPropLayout->setSpacing( 0 );
	pGridLayerPropLayout->setMargin( 0 );
	pLayerPropWidget->setLayout( pGridLayerPropLayout );

	m_pLayerPitchLCD = new LCDDisplay(
		pLayerPropWidget, QSize( 56, 20 ), false, false );
	pGridLayerPropLayout->addWidget( m_pLayerPitchLCD, 0, 0 );
	m_pLayerPitchLbl = new ClickableLabel(
		pLayerPropWidget, QSize( 45, ComponentView::nLabelHeight ),
		pCommonStrings->getPitchLabel() );
	m_pLayerPitchLbl->setObjectName( "LayerPitchLabel" );
	pGridLayerPropLayout->addWidget( m_pLayerPitchLbl, 1, 0 );

	m_pLayerPitchCoarseRotary = new Rotary(
		pLayerPropWidget, Rotary::Type::Center, tr( "Layer pitch (Coarse)" ), true,
		Instrument::fPitchMin + InstrumentEditorPanel::nPitchFineControl,
		Instrument::fPitchMax - InstrumentEditorPanel::nPitchFineControl );
	connect( m_pLayerPitchCoarseRotary, &Rotary::valueChanged, [&]() {
		const float fNewPitch = round( m_pLayerPitchCoarseRotary->getValue() ) +
			m_pLayerPitchFineRotary->getValue() / 100.0;
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setPitch( fNewPitch );
			}
		}
	});
	pGridLayerPropLayout->addWidget( m_pLayerPitchCoarseRotary, 0, 1 );
	m_pLayerPitchCoarseLbl = new ClickableLabel(
		pLayerPropWidget, QSize( 44, ComponentView::nLabelHeight ),
		pCommonStrings->getPitchCoarseLabel() );
	m_pLayerPitchCoarseLbl->setObjectName( "LayerPitchCoarseLabel" );
	pGridLayerPropLayout->addWidget( m_pLayerPitchCoarseLbl, 1, 1 );

	m_pLayerPitchFineRotary = new Rotary(
		pLayerPropWidget, Rotary::Type::Center, tr( "Layer pitch (Fine)" ), true,
		-50.0, 50.0 );
	connect( m_pLayerPitchFineRotary, &Rotary::valueChanged, [&]() {
		const float fNewPitch = round( m_pLayerPitchCoarseRotary->getValue() ) +
			m_pLayerPitchFineRotary->getValue() / 100.0;
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setPitch( fNewPitch );
			}
		}
	});
	pGridLayerPropLayout->addWidget( m_pLayerPitchFineRotary, 0, 2 );
	m_pLayerPitchFineLbl = new ClickableLabel(
		pLayerPropWidget, QSize( 44, ComponentView::nLabelHeight ),
		pCommonStrings->getPitchFineLabel() );
	m_pLayerPitchFineLbl->setObjectName( "LayerPitchFineLabel" );
	pGridLayerPropLayout->addWidget( m_pLayerPitchFineLbl, 1, 2 );

	m_pLayerMuteBtn = new Button(
		pLayerPropWidget,
		QSize( ComponentView::nButtonWidth, ComponentView::nButtonHeight ),
		Button::Type::Toggle, "",
		pCommonStrings->getSmallMuteButton(), true, QSize(), tr("Mute layer"),
		false, true );
	m_pLayerMuteBtn->setChecked( pComponent->getIsMuted() );
	m_pLayerMuteBtn->setObjectName( "LayerMuteButton" );
	connect( m_pLayerMuteBtn, &Button::clicked, [&](){
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setIsMuted( m_pLayerMuteBtn->isChecked() );
				updateView(); // WaveDisplay update
			}
		}
	});
	pGridLayerPropLayout->addWidget( m_pLayerMuteBtn, 0, 4 );

	m_pLayerSoloBtn = new Button(
		pLayerPropWidget,
		QSize( ComponentView::nButtonWidth, ComponentView::nButtonHeight ),
		Button::Type::Toggle, "",
		pCommonStrings->getSmallSoloButton(), false, QSize(), tr("Solo layer"),
		false, true );
	m_pLayerSoloBtn->setChecked( pComponent->getIsSoloed() );
	m_pLayerSoloBtn->setObjectName( "LayerSoloButton" );
	connect( m_pLayerSoloBtn, &Button::clicked, [&](){
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setIsSoloed( m_pLayerSoloBtn->isChecked() );
				updateView(); // WaveDisplay update
			}
		}
	});
	pGridLayerPropLayout->addWidget( m_pLayerSoloBtn, 0, 5 );

	m_pLayerGainRotary = new Rotary(
		pLayerPropWidget, Rotary::Type::Normal, tr( "Layer gain" ), false,
		0.0, 5.0 );
	m_pLayerGainRotary->setDefaultValue( 1.0 );
	connect( m_pLayerGainRotary, &Rotary::valueChanged, [&]() {
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setGain( m_pLayerGainRotary->getValue() );
				updateView(); // WaveDisplay update
			}
		}
	});
	pGridLayerPropLayout->addWidget( m_pLayerGainRotary, 0, 6 );
	m_pLayerGainLbl = new ClickableLabel(
		pLayerPropWidget, QSize( 44, ComponentView::nLabelHeight ),
		pCommonStrings->getLayerGainLabel() );
	m_pLayerGainLbl->setObjectName( "LayerGainLabel" );
	pGridLayerPropLayout->addWidget( m_pLayerGainLbl, 1, 6 );

	// Ensure the mute, solo, gain buttons are close to each other like the in
	// component header. In addition, we want a visual separation of pitch an
	// gain.
	pGridLayerPropLayout->setColumnStretch( 0, 1 );
	pGridLayerPropLayout->setColumnStretch( 1, 1 );
	pGridLayerPropLayout->setColumnStretch( 2, 1 );
	pGridLayerPropLayout->setColumnStretch( 3, 2 );
	pGridLayerPropLayout->setColumnStretch( 4, 0 );
	pGridLayerPropLayout->setColumnStretch( 5, 0 );
	pGridLayerPropLayout->setColumnStretch( 6, 0 );

	// Putting everything together.

	pVBoxComponentLayout->addWidget( m_pLayerScrollArea );
	pVBoxComponentLayout->addWidget( pLayerButtonWidget );
	pVBoxComponentLayout->addWidget( pSampleSelectionWidget );

	pVBoxLayerLayout->addWidget( m_pWaveDisplay );
	pVBoxLayerLayout->addWidget( pLayerPropWidget );

	pVBoxMainLayout->addWidget( pHeaderWidget );
	pVBoxMainLayout->addWidget( m_pComponentWidget );
	pVBoxMainLayout->addWidget( m_pLayerWidget );
	setLayout( pVBoxMainLayout );

	m_pPopup = new QMenu( this );
	m_pPopup->addAction( pCommonStrings->getMenuActionAdd(), pParent,
						 SLOT( addComponent() ) );
	m_pDeleteAction = m_pPopup->addAction(
		pCommonStrings->getMenuActionDelete(), this, SLOT( deleteComponent() ));
	updateStyleSheet();
}

ComponentView::~ComponentView() {
}

void ComponentView::updateStyleSheet() {
	const auto theme = Preferences::get_instance()->getTheme();

	const QColor headerColor = theme.m_color.m_windowColor;
	const QColor headerTextColor = theme.m_color.m_windowTextColor;
	const QColor borderHeaderLightColor = headerColor.lighter(
		Skin::nListBackgroundLightBorderScaling );
	const QColor borderHeaderDarkColor = headerColor.darker(
		Skin::nListBackgroundDarkBorderScaling );

	const QColor layerColor = theme.m_color.m_baseColor;
	const QColor borderLayerLightColor = layerColor.lighter(
		Skin::nListBackgroundLightBorderScaling );
	const QColor borderLayerDarkColor = layerColor.darker(
		Skin::nListBackgroundDarkBorderScaling );

	QString sStyleSheet;
	if ( m_bIsExpanded ) {
		sStyleSheet.append( QString( "\
QWidget#HeaderWidget { \
    background-color: %1; \
    border-top: 1px solid %2; \
    border-left: 1px solid %2; \
    border-right: 1px solid %3; \
} \
QWidget#ComponentWidget, \
QWidget#LayerWidget { \
    background-color: %4; \
    border-left: 2px solid %5; \
    border-right: 2px solid %6; \
} \
QWidget#ComponentWidget { \
    border-bottom: 1px solid %6; \
} \
QWidget#LayerWidget { \
    border-top: 1px solid %5; \
    border-bottom: 2px solid %6; \
}" )
							.arg( headerColor.name() )
							.arg( borderHeaderLightColor.name() )
							.arg( borderHeaderDarkColor.name() )
							.arg( layerColor.name() )
							.arg( borderLayerLightColor.name() )
							.arg( borderLayerDarkColor.name() ) );
}
	else {
		// LayerWidget won't be visible.
		sStyleSheet.append( QString( "\
QWidget#HeaderWidget { \
    background-color: %1; \
    border-top: 1px solid %2; \
    border-left: 1px solid %2; \
    border-right: 1px solid %3; \
    border-bottom: 1px solid %3; \
}" )
							.arg( headerColor.name() )
							.arg( borderHeaderLightColor.name() )
							.arg( borderHeaderDarkColor.name() ) );
	}

	sStyleSheet.append( QString( "\
ClickableLabel#ComponentName {	\
    background-color: %1; \
} \
QWidget#LayerButtonWidget, \
QWidget#LayerPropWidget, \
QWidget#SampleSelectionWidget { \
    background-color: %2; \
} \
ClickableLabel#LayerPitchLabel, \
ClickableLabel#LayerPitchCoarseLabel, \
ClickableLabel#LayerPitchFineLabel, \
ClickableLabel#LayerGainLabel, \
ClickableLabel#SampleSelectionLabel { \
    background-color: %2; \
} \
")
						.arg( headerColor.name() )
						.arg( layerColor.name() ) );

	setStyleSheet( sStyleSheet );
}

void ComponentView::updateView() {
	updateActivation();

	if ( m_pComponent != nullptr ) {
		m_pComponentNameLbl->setText( m_pComponent->getName() );
		m_pComponentMuteBtn->setChecked( m_pComponent->getIsMuted() );
		m_pComponentSoloBtn->setChecked( m_pComponent->getIsSoloed() );
		m_pComponentGainRotary->setValue(
			m_pComponent->getGain(), false, Event::Trigger::Suppress );

		if ( m_nSelectedLayer >= 0 ) {
			const auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );

			if ( pLayer != nullptr ) {
				m_pLayerMuteBtn->setChecked( pLayer->getIsMuted() );
				m_pLayerSoloBtn->setChecked( pLayer->getIsSoloed() );

				// Layer GAIN
				m_pLayerGainRotary->setValue( pLayer->getGain(), false,
											  Event::Trigger::Suppress );

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

				const QString sNewPitch = QString( "%1" )
					.arg( pLayer->getPitch(), -2, 'f', 2, '0' );
				if ( m_pLayerPitchLCD->text() != sNewPitch ) {
					m_pLayerPitchLCD->setText( sNewPitch );
				}

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
	m_bIsExpanded = true;
	updateVisibility();
}

void ComponentView::collapse() {
	if ( ! m_bIsExpanded ) {
		return;
	}

	m_pShowLayersBtn->setIconFileName( "plus.svg" );

	m_bIsExpanded = false;
	updateVisibility();
}

void ComponentView::deleteComponent() {
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pInstrument = pHydrogenApp->getInstrumentRack()->
		getInstrumentEditorPanel()->getInstrument();

	if ( pInstrument->getComponents()->size() <= 1 ) {
		ERRORLOG( "There is just a single component remaining. This one can not be deleted." );
		return;
	}

	if ( m_pComponent == nullptr ) {
		return;
	}
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	const auto sName = m_pComponent->getName();

	auto pNewInstrument = std::make_shared<Instrument>( pInstrument );
	pNewInstrument->removeComponent( pInstrument->index( m_pComponent ) );

	pHydrogenApp->pushUndoCommand(
		new SE_replaceInstrumentAction(
			pNewInstrument, pInstrument,
			SE_replaceInstrumentAction::Type::DeleteComponent, sName ) );
	pHydrogenApp->showStatusBarMessage(
		QString( "%1 [%2]" ).arg( pCommonStrings->getActionDeleteComponent() )
		.arg( sName ) );
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

	auto pInstrument = HydrogenApp::get_instance()->getInstrumentRack()->
		getInstrumentEditorPanel()->getInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}

	const QString sOldName = m_pComponent->getName();
	bool bIsOkPressed;
	const QString sNewName = QInputDialog::getText(
		this, "Hydrogen", tr( "New component name" ), QLineEdit::Normal,
		sOldName, &bIsOkPressed );

	if ( bIsOkPressed && sOldName != sNewName ) {
		 pHydrogenApp->pushUndoCommand(
			 new SE_renameComponentAction(
				 sNewName, sOldName, pInstrument->index( m_pComponent ) ) );
		 pHydrogenApp->showStatusBarMessage(
			 QString( "%1: [%2] -> [%3]" )
					 .arg( pCommonStrings->getActionRenameComponent() )
					 .arg( sOldName ).arg( sNewName ) );
	}
}

void ComponentView::mousePressEvent( QMouseEvent* pEvent ) {
	if ( pEvent->button() == Qt::RightButton ) {
		m_pPopup->popup( QPoint( pEvent->globalX(), pEvent->globalY() ) );
	}
}

void ComponentView::updateActivation() {
	if ( m_pComponent != nullptr ) {
		m_pComponentMuteBtn->setIsActive( true );
		m_pComponentSoloBtn->setIsActive( true );
		m_pComponentGainRotary->setIsActive( true );
	}
	else {
		m_pComponentNameLbl->setText( "" );
		m_pComponentMuteBtn->setIsActive( false );
		m_pComponentSoloBtn->setIsActive( false );
		m_pComponentGainRotary->setIsActive( false );
	}

	std::shared_ptr<InstrumentLayer> pLayer = nullptr;
	if ( m_pComponent != nullptr && m_nSelectedLayer >= 0 ) {
		pLayer = m_pComponent->getLayer( m_nSelectedLayer );
	}

	if ( pLayer != nullptr ) {
		m_pLayerMuteBtn->setIsActive( true );
		m_pLayerSoloBtn->setIsActive( true );
		m_pLayerGainRotary->setIsActive( true );
		m_pLayerPitchCoarseRotary->setIsActive( true );
		m_pLayerPitchFineRotary->setIsActive( true );

		m_pRemoveLayerBtn->setIsActive( true );
		m_pSampleEditorBtn->setIsActive( true );

		m_pWaveDisplay->updateDisplay( nullptr );
	}
	else {
		m_pLayerMuteBtn->setChecked( false );
		m_pLayerMuteBtn->setIsActive( false );
		m_pLayerSoloBtn->setChecked( false );
		m_pLayerSoloBtn->setIsActive( false );
		m_pLayerGainRotary->setIsActive( false );

		// Layer PITCH
		m_pLayerPitchCoarseRotary->setIsActive( false );
		m_pLayerPitchFineRotary->setIsActive( false );
		m_pLayerPitchLCD->setText( "" );

		m_pRemoveLayerBtn->setIsActive( false );
		m_pSampleEditorBtn->setIsActive( false );

		m_pWaveDisplay->updateDisplay( nullptr );
	}

	// If there is only a single component left, we do not allow to remove it.
	const auto pInstrument = HydrogenApp::get_instance()->getInstrumentRack()->
		getInstrumentEditorPanel()->getInstrument();
	if ( pInstrument->getComponents()->size() <= 1 ) {
		m_pDeleteAction->setDisabled( true );
	}
	else {
		m_pDeleteAction->setDisabled( false );
	}
}

void ComponentView::updateVisibility() {
	m_pComponentWidget->setVisible( m_bIsExpanded );
	m_pLayerWidget->setVisible( m_bIsExpanded );
	updateStyleSheet();
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
	auto pInstrument = HydrogenApp::get_instance()->getInstrumentRack()->
		getInstrumentEditorPanel()->getInstrument();
	if ( pInstrument == nullptr ) {
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
			pSample->getFilepath(), pInstrument->index( m_pComponent ),
			m_nSelectedLayer );
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
