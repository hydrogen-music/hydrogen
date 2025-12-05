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

#include "LayerPreview.h"
#include "WaveDisplay.h"
#include "../InstrumentEditor.h"
#include "../Rack.h"
#include "../../AudioFileBrowser/AudioFileBrowser.h"
#include "../../CommonStrings.h"
#include "../../Compatibility/MouseEvent.h"
#include "../../HydrogenApp.h"
#include "../../Skin.h"
#include "../../UndoActions.h"
#include "../../Widgets/Button.h"
#include "../../Widgets/ClickableLabel.h"
#include "../../Widgets/InlineEdit.h"
#include "../../Widgets/LCDCombo.h"
#include "../../Widgets/LCDDisplay.h"
#include "../../Widgets/Rotary.h"

using namespace H2Core;

ComponentView::ComponentView( QWidget* pParent,
							  std::shared_ptr<InstrumentComponent> pComponent )
	: QWidget( pParent )
	, m_pComponent( pComponent )
	, m_nSelectedLayer( 0 )
	, m_bIsExpanded( true )
{
	setMinimumHeight( ComponentView::nHeaderHeight );
	setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    auto pHydrogenApp = HydrogenApp::get_instance();
	auto pCommonStrings = pHydrogenApp->getCommonStrings();

	setObjectName( "ComponentProperties" );

	m_pVBoxMainLayout = new QVBoxLayout();
	m_pVBoxMainLayout->setSpacing( 0 );

	auto pHeaderWidget = new QWidget( this );
	pHeaderWidget->setFixedHeight( ComponentView::nHeaderHeight );
	pHeaderWidget->setObjectName( "HeaderWidget" );
	auto pHBoxHeaderLayout = new QHBoxLayout();
	pHBoxHeaderLayout->setSpacing( ComponentView::nHeaderSpacing );
	pHBoxHeaderLayout->setContentsMargins(
		ComponentView::nHeaderSpacing, ComponentView::nHeaderSpacing,
		ComponentView::nMargin, 0
	);
	pHeaderWidget->setLayout( pHBoxHeaderLayout );

	m_pShowLayersBtn = new QPushButton( pHeaderWidget );
	m_pShowLayersBtn->setObjectName( "ShowLayersBtn" );
	m_pShowLayersBtn->setFlat( true );
	m_pShowLayersBtn->setFixedSize(
		ComponentView::nHeaderHeight - 2, ComponentView::nHeaderHeight - 2
	);
	connect( m_pShowLayersBtn, &Button::clicked, [&]() {
		if ( m_bIsExpanded ) {
			collapse();
		}
		else {
			expand();
		}
		emit expandedOrCollapsed();
	} );
	pHBoxHeaderLayout->addWidget( m_pShowLayersBtn );

	m_pInlineEdit = new InlineEdit( pHeaderWidget );
	m_pInlineEdit->hide();
	m_pInlineEdit->setAlignment( Qt::AlignCenter );
	m_pInlineEdit->setTextMargins( 0, 0, 0, 0 );
	m_pInlineEdit->setContentsMargins( 0, 0, 0, 0 );

	m_pComponentNameLbl = new ClickableLabel(
		pHeaderWidget, QSize( 0, 0 ), "", ClickableLabel::DefaultColor::Bright, true
	);
	m_pComponentNameLbl->setFixedHeight( ComponentView::nHeaderHeight - 2 ),
		m_pComponentNameLbl->setObjectName( "ComponentName" );
	pHBoxHeaderLayout->addWidget( m_pComponentNameLbl );

	connect(
		m_pComponentNameLbl, &ClickableLabel::labelDoubleClicked, this,
		[=]() {
			m_pInlineEdit->startEditing(
				m_pComponentNameLbl->geometry(), m_pComponentNameLbl->text()
			);
			m_pComponentNameLbl->hide();
		}
	);

	connect( m_pInlineEdit, &InlineEdit::editAccepted, [=]() {
		if ( !m_pInlineEdit->isVisible() ) {
			// Already rejected
			return;
		}

		m_pComponentNameLbl->show();
		m_pInlineEdit->hide();

		auto pHydrogenApp = HydrogenApp::get_instance();
		const auto pCommonStrings = pHydrogenApp->getCommonStrings();

		auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
		if ( pInstrument == nullptr || m_pComponent == nullptr ) {
			return;
		}

		const QString sOldName = m_pComponent->getName();
		const QString sNewName = m_pInlineEdit->text();

		if ( sOldName != sNewName ) {
			pHydrogenApp->pushUndoCommand( new SE_renameComponentAction(
				sNewName, sOldName, pInstrument->index( m_pComponent )
			) );
			pHydrogenApp->showStatusBarMessage(
				QString( "%1: [%2] -> [%3]" )
					.arg( pCommonStrings->getActionRenameComponent() )
					.arg( sOldName )
					.arg( sNewName )
			);
		}
	} );

	connect( m_pInlineEdit, &InlineEdit::editRejected, [=]() {
		m_pComponentNameLbl->show();
		m_pInlineEdit->hide();
	} );

	// Expanded elements

	m_pSeparatorComponent = new QWidget( pHeaderWidget );
	m_pSeparatorComponent->setObjectName( "SeparatorComponent" );
	m_pSeparatorComponent->setFixedSize(
		Rack::nWidth, ComponentView::nSeparatorHeight
	);
	m_pSeparatorLayout = new QVBoxLayout();
	m_pSeparatorLayout->setAlignment( Qt::AlignCenter );
	m_pSeparatorLayout->setContentsMargins( 0, 0, 0, 0 );
	m_pSeparatorComponent->setLayout( m_pSeparatorLayout );

	auto pSeparatorWidget = new QWidget( m_pSeparatorComponent );
	pSeparatorWidget->setObjectName( "SeparatorToolBarComponent" );
	pSeparatorWidget->setFixedSize(
		Rack::nWidth / 6, ComponentView::nSeparatorHeight
	);
	m_pSeparatorLayout->addWidget( pSeparatorWidget );

	m_pToolBarComponent = new QToolBar( this );
	m_pToolBarComponent->setObjectName( "CVToolBarComponent" );
	m_pToolBarComponent->setFixedHeight( ComponentView::nToolBarHeight );
	m_pToolBarComponent->setFocusPolicy( Qt::NoFocus );

	auto createAction = [&]( const QString& sText, bool bCheckable ) {
		auto pAction = new QAction( m_pToolBarComponent );
		pAction->setCheckable( bCheckable );
		pAction->setIconText( sText );
		pAction->setToolTip( sText );

		return pAction;
	};

	m_pNewComponentAction =
		createAction( pCommonStrings->getActionAddComponent(), false );
	connect( m_pNewComponentAction, &QAction::triggered, [=]() {
		HydrogenApp::get_instance()->getComponentEditor()->addComponent();
	} );
	m_pToolBarComponent->addAction( m_pNewComponentAction );

	m_pToolBarComponent->addSeparator();

	m_pDuplicateComponentAction =
		createAction( pCommonStrings->getActionDuplicateComponent(), false );
	connect( m_pDuplicateComponentAction, &QAction::triggered, [=]() {
		const auto pInstrument =
			Hydrogen::get_instance()->getSelectedInstrument();
		if ( pInstrument == nullptr || m_pComponent == nullptr ) {
			return;
		}

		auto pHydrogenApp = HydrogenApp::get_instance();
		const auto pCommonStrings = pHydrogenApp->getCommonStrings();

		auto pNewInstrument = std::make_shared<Instrument>( pInstrument );

		const auto pNewComponent =
			std::make_shared<InstrumentComponent>( m_pComponent );
		pNewComponent->setName(
			Filesystem::appendNumberOrIncrement( m_pComponent->getName() )
		);
		pNewInstrument->addComponent( pNewComponent );

		pHydrogenApp->pushUndoCommand( new SE_replaceInstrumentAction(
			pNewInstrument, pInstrument,
			SE_replaceInstrumentAction::Type::DuplicateComponent,
			m_pComponent->getName()
		) );
		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2]" )
				.arg( pCommonStrings->getActionDuplicateComponent() )
				.arg( m_pComponent->getName() )
		);
	} );
	m_pToolBarComponent->addAction( m_pDuplicateComponentAction );

	m_pToolBarComponent->addSeparator();

	m_pDeleteComponentAction =
		createAction( pCommonStrings->getActionDeleteComponent(), false );
	connect( m_pDeleteComponentAction, &QAction::triggered, [=]() {
		deleteComponent();
	} );
	m_pToolBarComponent->addAction( m_pDeleteComponentAction );

	auto pStretch = new QWidget();
	pStretch->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
	m_pToolBarComponent->addWidget( pStretch );

	m_pComponentMuteBtn = new Button(
		m_pToolBarComponent,
		QSize( ComponentView::nButtonWidth, ComponentView::nButtonHeight ),
		Button::Type::Toggle, "",
		pCommonStrings->getSmallMuteButton(), QSize(), tr( "Mute component" ),
		true );
	m_pComponentMuteBtn->setChecked( pComponent->getIsMuted() );
	m_pComponentMuteBtn->setObjectName( "ComponentMuteButton" );
	connect( m_pComponentMuteBtn, &Button::clicked, [&](){
		if ( m_pComponent != nullptr ) {
			m_pComponent->setIsMuted( m_pComponentMuteBtn->isChecked() );
			// Repaint since we indicate mute for all layers.
			m_pLayerPreview->update();
		}
	});
	m_pToolBarComponent->addWidget( m_pComponentMuteBtn );

	m_pComponentSoloBtn = new Button(
		m_pToolBarComponent,
		QSize( ComponentView::nButtonWidth, ComponentView::nButtonHeight ),
		Button::Type::Toggle, "",
		pCommonStrings->getSmallSoloButton(), QSize(), tr( "Solo component" ),
		true );
	m_pComponentSoloBtn->setChecked( pComponent->getIsSoloed() );
	m_pComponentSoloBtn->setObjectName( "ComponentSoloButton" );
	connect( m_pComponentSoloBtn, &Button::clicked, [&](){
		if ( m_pComponent != nullptr ) {
			m_pComponent->setIsSoloed( m_pComponentSoloBtn->isChecked() );
		}
	});
	m_pToolBarComponent->addWidget( m_pComponentSoloBtn );

	m_pComponentGainRotary = new Rotary(
		m_pToolBarComponent, Rotary::Type::Normal, tr( "Component volume" ), false,
		0.0, 5.0 );
	m_pComponentGainRotary->setDefaultValue( 1.0 );
	connect( m_pComponentGainRotary, &Rotary::valueChanged, [&]() {
		if ( m_pComponent != nullptr ) {
			m_pComponent->setGain( m_pComponentGainRotary->getValue() );
		}
	});
	m_pToolBarComponent->addWidget( m_pComponentGainRotary );

	// Layer specific stuff

	m_pComponentWidget = new QWidget( this );
	m_pComponentWidget->setObjectName( "ComponentWidget" );
	auto pVBoxComponentLayout = new QVBoxLayout( this );
	pVBoxComponentLayout->setSpacing( 0 );
	pVBoxComponentLayout->setContentsMargins(
		ComponentView::nMargin, 0,
		ComponentView::nMargin, 0 );
	m_pComponentWidget->setLayout( pVBoxComponentLayout );

	// Layer preview
	m_pLayerPreview = new LayerPreview( this );

    // Toolbar with buttons
	m_pToolBarLayer = new QToolBar( m_pComponentWidget );
	m_pToolBarLayer->setObjectName( "CVToolBarLayer" );
	m_pToolBarLayer->setFixedHeight( ComponentView::nToolBarHeight );
	m_pToolBarLayer->setFocusPolicy( Qt::NoFocus );

	m_pNewLayerAction =
		createAction( pCommonStrings->getActionAddInstrumentLayer(), false );
	connect( m_pNewLayerAction, &QAction::triggered, [=]() {
		addNewLayer();
	} );
	m_pToolBarLayer->addAction( m_pNewLayerAction );

	m_pToolBarLayer->addSeparator();

	m_pReplaceLayerAction =
		createAction( pCommonStrings->getActionReplaceInstrumentLayer(), false );
	connect( m_pReplaceLayerAction, &QAction::triggered, [=]() {
		replaceLayer( m_nSelectedLayer );
	} );
	m_pToolBarLayer->addAction( m_pReplaceLayerAction );

	m_pDuplicateLayerAction = createAction(
		pCommonStrings->getActionDuplicateInstrumentLayer(), false
	);
	connect( m_pDuplicateLayerAction, &QAction::triggered, [=]() {
		auto pHydrogenApp = HydrogenApp::get_instance();
		auto pHydrogen = Hydrogen::get_instance();
		const auto pInstrument = pHydrogen->getSelectedInstrument();
		if ( m_pComponent == nullptr || pInstrument == nullptr ) {
			return;
		}
		auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
		if ( pLayer == nullptr ) {
			ERRORLOG( QString( "Unable to obtain selected layer [%1]" )
						  .arg( m_nSelectedLayer ) );
			return;
		}
		auto pNewInstrument = std::make_shared<Instrument>( pInstrument );
		auto pNewComponent =
			pNewInstrument->getComponent( pInstrument->index( m_pComponent ) );
		if ( pNewComponent == nullptr ) {
			ERRORLOG( "Hiccup while looking up component" );
			return;
		}

		++m_nSelectedLayer;

		auto pNewLayer = std::make_shared<InstrumentLayer>( pLayer );
		pNewInstrument->addLayer(
			pNewComponent, pNewLayer, m_nSelectedLayer, Event::Trigger::Suppress
		);

		pHydrogenApp->pushUndoCommand( new SE_replaceInstrumentAction(
			pNewInstrument, pInstrument,
			SE_replaceInstrumentAction::Type::DuplicateLayer, ""
		) );
	} );
	m_pToolBarLayer->addAction( m_pDuplicateLayerAction );

	m_pDeleteLayerAction =
		createAction( pCommonStrings->getActionDeleteInstrumentLayer(), false );
	connect( m_pDeleteLayerAction, &QAction::triggered, [=]() {
		removeLayerButtonClicked();
	} );
	m_pToolBarLayer->addAction( m_pDeleteLayerAction );

	m_pToolBarLayer->addSeparator();

	m_pEditLayerAction =
		createAction( pCommonStrings->getActionEditInstrumentLayer(), false );
	connect( m_pEditLayerAction, &QAction::triggered, [=]() {
		showSampleEditor();
	} );
	m_pToolBarLayer->addAction( m_pEditLayerAction );

	m_pToolBarLayer->addSeparator();

	auto pStretchLayer = new QWidget();
	pStretchLayer->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Preferred
	);
	m_pToolBarLayer->addWidget( pStretchLayer );

	m_pLayerMuteBtn = new Button(
		m_pToolBarLayer,
		QSize( ComponentView::nButtonWidth, ComponentView::nButtonHeight ),
		Button::Type::Toggle, "", pCommonStrings->getSmallMuteButton(), QSize(),
		tr( "Mute layer" ), true
	);
	m_pLayerMuteBtn->setObjectName( "LayerMuteButton" );
	connect( m_pLayerMuteBtn, &Button::clicked, [&]() {
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setIsMuted( m_pLayerMuteBtn->isChecked() );
				updateView();  // WaveDisplay update
			}
		}
	} );
	m_pToolBarLayer->addWidget( m_pLayerMuteBtn );

	m_pLayerSoloBtn = new Button(
		m_pToolBarLayer,
		QSize( ComponentView::nButtonWidth, ComponentView::nButtonHeight ),
		Button::Type::Toggle, "", pCommonStrings->getSmallSoloButton(), QSize(),
		tr( "Solo layer" ), true
	);
	m_pLayerSoloBtn->setObjectName( "LayerSoloButton" );
	connect( m_pLayerSoloBtn, &Button::clicked, [&]() {
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setIsSoloed( m_pLayerSoloBtn->isChecked() );
				updateView();  // WaveDisplay update
			}
		}
	} );
	m_pToolBarLayer->addWidget( m_pLayerSoloBtn );

	m_pLayerGainRotary = new Rotary(
		m_pToolBarLayer, Rotary::Type::Normal, tr( "Layer gain" ), false,
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
	m_pToolBarLayer->addWidget( m_pLayerGainRotary );

	// Sample selection

	auto pSampleSelectionWidget = new QWidget( m_pComponentWidget );
	pSampleSelectionWidget->setFixedHeight(
		ComponentView::nSampleSelectionHeight );
	pSampleSelectionWidget->setObjectName( "SampleSelectionWidget" );
	auto pHBoxSampleSelectionLayout = new QHBoxLayout();
	pHBoxSampleSelectionLayout->setSpacing( 0 );
	pHBoxSampleSelectionLayout->setContentsMargins( 0, 0, 0, 0 );
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
		Rotary::nHeight + ComponentView::nLabelHeight
	);
	pLayerPropWidget->setObjectName( "LayerPropWidget" );
	auto pLayerPropLayout = new QHBoxLayout();
	pLayerPropLayout->setAlignment( Qt::AlignLeft );
	pLayerPropLayout->setSpacing( 3 );
	pLayerPropLayout->setContentsMargins( 0, 0, 0, 0 );
	pLayerPropWidget->setLayout( pLayerPropLayout );

	const int nVerticalPropSpacing = 1;
	auto pLayerPropDisplayWidget = new QWidget( pLayerPropWidget );
	pLayerPropDisplayWidget->setFixedWidth( 56 );
	pLayerPropLayout->addWidget( pLayerPropDisplayWidget );
	auto pLayerPropDisplayLayout = new QVBoxLayout();
	pLayerPropDisplayLayout->setAlignment( Qt::AlignCenter );
	pLayerPropDisplayLayout->setSpacing( nVerticalPropSpacing );
	pLayerPropDisplayLayout->setContentsMargins( 0, 0, 0, 0 );
	pLayerPropDisplayWidget->setLayout( pLayerPropDisplayLayout );

	pLayerPropDisplayLayout->addStretch();

	m_pLayerPitchLCD = new LCDDisplay(
		pLayerPropDisplayWidget, QSize( 56, 20 ), false, false
	);
	pLayerPropDisplayLayout->addWidget( m_pLayerPitchLCD );
	m_pLayerPitchLbl = new ClickableLabel(
		pLayerPropDisplayWidget, QSize( 56, ComponentView::nLabelHeight ),
		pCommonStrings->getPitchLabel()
	);
	m_pLayerPitchLbl->setObjectName( "LayerPitchLabel" );
	pLayerPropDisplayLayout->addWidget( m_pLayerPitchLbl );

	auto pLayerPropCoarseWidget = new QWidget( pLayerPropWidget );
	pLayerPropLayout->addWidget( pLayerPropCoarseWidget );
	auto pLayerPropCoarseLayout = new QVBoxLayout();
	pLayerPropCoarseLayout->setSpacing( nVerticalPropSpacing );
	pLayerPropCoarseLayout->setContentsMargins( 0, 0, 0, 0 );
	pLayerPropCoarseWidget->setLayout( pLayerPropCoarseLayout );

	m_pLayerPitchCoarseRotary = new Rotary(
		pLayerPropCoarseWidget, Rotary::Type::Center,
		tr( "Layer pitch (Coarse)" ), true,
		Instrument::fPitchMin + InstrumentEditor::nPitchFineControl,
		Instrument::fPitchMax - InstrumentEditor::nPitchFineControl
	);
	connect( m_pLayerPitchCoarseRotary, &Rotary::valueChanged, [&]() {
		const float fNewPitch = round( m_pLayerPitchCoarseRotary->getValue() ) +
								m_pLayerPitchFineRotary->getValue() / 100.0;
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setPitch( fNewPitch );
				updatePitchDisplay();
			}
		}
	} );
	pLayerPropCoarseLayout->addWidget( m_pLayerPitchCoarseRotary );
	m_pLayerPitchCoarseLbl = new ClickableLabel(
		pLayerPropCoarseWidget, QSize( 44, ComponentView::nLabelHeight ),
		pCommonStrings->getPitchCoarseLabel()
	);
	m_pLayerPitchCoarseLbl->setObjectName( "LayerPitchCoarseLabel" );
	pLayerPropCoarseLayout->addWidget( m_pLayerPitchCoarseLbl );

	auto pLayerPropFineWidget = new QWidget( pLayerPropWidget );
	pLayerPropLayout->addWidget( pLayerPropFineWidget );
	auto pLayerPropFineLayout = new QVBoxLayout();
	pLayerPropFineLayout->setSpacing( nVerticalPropSpacing );
	pLayerPropFineLayout->setContentsMargins( 0, 0, 0, 0 );
	pLayerPropFineWidget->setLayout( pLayerPropFineLayout );

	m_pLayerPitchFineRotary = new Rotary(
		pLayerPropFineWidget, Rotary::Type::Center, tr( "Layer pitch (Fine)" ),
		true, -50.0, 50.0
	);
	connect( m_pLayerPitchFineRotary, &Rotary::valueChanged, [&]() {
		const float fNewPitch = round( m_pLayerPitchCoarseRotary->getValue() ) +
								m_pLayerPitchFineRotary->getValue() / 100.0;
		if ( m_pComponent != nullptr ) {
			auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setPitch( fNewPitch );
				updatePitchDisplay();
			}
		}
	} );
	pLayerPropFineLayout->addWidget( m_pLayerPitchFineRotary );
	m_pLayerPitchFineLbl = new ClickableLabel(
		pLayerPropFineWidget, QSize( 44, ComponentView::nLabelHeight ),
		pCommonStrings->getPitchFineLabel()
	);
	m_pLayerPitchFineLbl->setObjectName( "LayerPitchFineLabel" );
	pLayerPropFineLayout->addWidget( m_pLayerPitchFineLbl );

	pLayerPropLayout->addStretch();

	// Putting everything together.

	pVBoxComponentLayout->addSpacing( ComponentView::nVerticalSpacing );
	pVBoxComponentLayout->addWidget( m_pLayerPreview );
    pVBoxComponentLayout->addWidget( m_pToolBarLayer );
    pVBoxComponentLayout->addSpacing( ComponentView::nVerticalSpacing );
	pVBoxComponentLayout->addWidget( pSampleSelectionWidget );
    pVBoxComponentLayout->addSpacing( ComponentView::nVerticalSpacing );

	pVBoxLayerLayout->addWidget( m_pWaveDisplay );
	pVBoxLayerLayout->addWidget( pLayerPropWidget );

	m_pVBoxMainLayout->addWidget( pHeaderWidget );
	m_pVBoxMainLayout->addWidget( m_pSeparatorComponent );
    m_pVBoxMainLayout->addWidget( m_pToolBarComponent );
	m_pVBoxMainLayout->addWidget( m_pComponentWidget );
	m_pVBoxMainLayout->addWidget( m_pLayerWidget );
	setLayout( m_pVBoxMainLayout );

	updateColors();
    updateIcons();
	updateStyleSheet();
}

ComponentView::~ComponentView() {
}

void ComponentView::updateColors() {
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	m_pComponentMuteBtn->setCheckedBackgroundColor( pColorTheme->m_muteColor );
	m_pComponentMuteBtn->setCheckedBackgroundTextColor(
		pColorTheme->m_muteTextColor );
	m_pComponentSoloBtn->setCheckedBackgroundColor( pColorTheme->m_soloColor );
	m_pComponentSoloBtn->setCheckedBackgroundTextColor(
		pColorTheme->m_soloTextColor );

	m_pLayerMuteBtn->setCheckedBackgroundColor( pColorTheme->m_muteColor );
	m_pLayerMuteBtn->setCheckedBackgroundTextColor(
		pColorTheme->m_muteTextColor );
	m_pLayerSoloBtn->setCheckedBackgroundColor( pColorTheme->m_soloColor );
	m_pLayerSoloBtn->setCheckedBackgroundTextColor(
		pColorTheme->m_soloTextColor );
}

void ComponentView::updateIcons() {
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	m_pShowLayersBtn->setIcon(
		QIcon( sIconPath + ( m_bIsExpanded ? "minus.svg" : "plus.svg" ) )
	);

	m_pNewComponentAction->setIcon( QIcon( sIconPath + "new.svg" ) );
	m_pDuplicateComponentAction->setIcon( QIcon( sIconPath + "duplicate.svg" )
	);
	m_pDeleteComponentAction->setIcon( QIcon( sIconPath + "bin.svg" ) );

	m_pNewLayerAction->setIcon( QIcon( sIconPath + "new.svg" ) );
	m_pReplaceLayerAction->setIcon( QIcon( sIconPath + "folder.svg" ) );
	m_pDuplicateLayerAction->setIcon( QIcon( sIconPath + "duplicate.svg" ) );
	m_pDeleteLayerAction->setIcon( QIcon( sIconPath + "bin.svg" ) );
	m_pEditLayerAction->setIcon( QIcon( sIconPath + "sample-editor.svg" ) );
}

void ComponentView::updateStyleSheet() {
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	QColor iconColor;
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		iconColor = Qt::white;
	} else {
		iconColor = Qt::black;
	}

	const QColor headerColor = pColorTheme->m_componentEditor_componentColor;
	const QColor headerColorHover =
		headerColor.darker( Skin::nToolBarHoveredScaling );
	const QColor headerColorPressed =
		headerColor.darker( Skin::nToolBarCheckedScaling );
	const QColor headerSeparator = headerColor.lighter( 115 );
	const QColor headerTextColor =
		pColorTheme->m_componentEditor_componentTextColor;
	const QColor borderHeaderLightColor = headerColor.lighter(
		Skin::nListBackgroundLightBorderScaling );
	const QColor borderHeaderDarkColor = headerColor.darker(
		Skin::nListBackgroundDarkBorderScaling );

	const QColor layerColor = pColorTheme->m_componentEditor_layerColor;
	const QColor layerTextColor = pColorTheme->m_componentEditor_layerTextColor;
	const QColor borderLayerLightColor = layerColor.lighter(
		Skin::nListBackgroundLightBorderScaling );
	const QColor borderLayerDarkColor = layerColor.darker(
		Skin::nListBackgroundDarkBorderScaling );

	QString sStyleSheet;
	if ( m_bIsExpanded ) {
		sStyleSheet.append( QString(
			"\
QWidget#HeaderWidget { \
    background-color: %1; \
    border-top: 1px solid %2; \
    border-left: 1px solid %2; \
    border-right: 1px solid %3; \
    border-bottom: none; \
} \
QWidget#ComponentWidget, \
QWidget#LayerWidget { \
    background-color: %4; \
    border-left: 1px solid %5; \
    border-right: 1px solid %6; \
} \
QWidget#ComponentWidget { \
    border-bottom: 1px solid %6; \
} \
QWidget#LayerWidget { \
    border-top: 1px solid %5; \
    border-bottom: 2px solid %6; \
} \
QWidget#SeparatorComponent { \
    background-color: %1; \
    border-left: 1px solid %2; \
    border-right: 1px solid %3; \
} \
QWidget#SeparatorToolBarComponent { \
    border-top: 1px solid %7; \
} \
QToolBar {\
    spacing: 1px; \
} \
QToolBar#CVToolBarComponent {\
    background-color: %1; \
    color: %8; \
    border-left: 1px solid %2; \
    border-right: 1px solid %3; \
    border-bottom: 1px solid %3; \
} \
QToolBar#CVToolBarLayer {\
    background-color: %4; \
    color: %9; \
    border-top: 1px solid #000; \
    border-bottom: 1px solid %6; \
}" )
								.arg( headerColor.name() )
								.arg( borderHeaderLightColor.name() )
								.arg( borderHeaderDarkColor.name() )
								.arg( layerColor.name() )
								.arg( borderLayerLightColor.name() )
								.arg( borderLayerDarkColor.name() )
								.arg( headerSeparator.name() )
								.arg( headerTextColor.name() )
								.arg( layerTextColor.name() ) );
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
QPushButton#ShowLayersBtn {\
    background-color: %1; \
} \
QPushButton#ShowLayersBtn:hover {\
    background-color: %3; \
    border: 1px solid %5; \
} \
QPushButton#ShowLayersBtn:pressed {\
    background-color: %4; \
    border: 1px solid %5; \
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
" )
							.arg( headerColor.name() )
							.arg( layerColor.name() )
							.arg( headerColorHover.name() )
							.arg( headerColorPressed.name() )
							.arg( iconColor.name() ) );

	setStyleSheet( sStyleSheet );

	m_pComponentNameLbl->setColor( headerTextColor );
	m_pSampleSelectionLbl->setColor( layerTextColor );
	m_pLayerPitchLbl->setColor( layerTextColor );
	m_pLayerPitchCoarseLbl->setColor( layerTextColor );
	m_pLayerPitchFineLbl->setColor( layerTextColor );

}

void ComponentView::updateView() {
	updateActivation();

	if ( m_pComponent != nullptr ) {
		m_pComponentNameLbl->setText( m_pComponent->getName() );
		m_pComponentMuteBtn->setChecked( m_pComponent->getIsMuted() );
		m_pComponentSoloBtn->setChecked( m_pComponent->getIsSoloed() );
		m_pComponentGainRotary->setValue(
			m_pComponent->getGain(), false, Event::Trigger::Suppress );

		switch( m_pComponent->getSelection() ) {
		case InstrumentComponent::Selection::RoundRobin:
			m_pSampleSelectionCombo->setCurrentIndex( 1 );
			break;
		case InstrumentComponent::Selection::Random:
			m_pSampleSelectionCombo->setCurrentIndex( 2 );
			break;
		case InstrumentComponent::Selection::Velocity:
		default:
			m_pSampleSelectionCombo->setCurrentIndex( 0 );
			break;
		}

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

				m_pWaveDisplay->updateDisplay( pLayer );
			}
		}
	}

	updatePitchDisplay();
	m_pLayerPreview->update();
}

void ComponentView::accountForScrollbar( bool bScrollBarVisible )
{
	m_pVBoxMainLayout->setContentsMargins(
		0, 0, bScrollBarVisible ? Skin::nScrollBarWidth : 0, 0
	);
	m_pSeparatorLayout->setContentsMargins(
		0, 0, bScrollBarVisible ? Skin::nScrollBarWidth : 0, 0
	);

	// We have to adjust those widgets manually which render their whole content
	// and do not have a Qt child widget that could claim the available space.
	const int nNewWidth = Rack::nWidth - ComponentView::nMargin * 2 -
						  ( bScrollBarVisible ? Skin::nScrollBarWidth : 0 );
	m_pLayerPreview->setFixedWidth( nNewWidth );
}

int ComponentView::getExpandedHeight() const
{
	const int nLayers =
		m_pComponent != nullptr ? m_pComponent->getLayers().size() : 1;
	return ComponentView::nVerticalSpacing * 4 + ComponentView::nHeaderHeight +
		   ComponentView::nToolBarHeight * 2 + LayerPreview::nHeader +
		   LayerPreview::nBorder + LayerPreview::nLayerHeight * nLayers +
		   ComponentView::nWaveDisplayHeight + Rotary::nHeight +
           ComponentView::nSampleSelectionHeight +
		   ComponentView::nLabelHeight + ComponentView::nMargin * 4;
}

void ComponentView::expand() {
	if ( m_bIsExpanded ) {
		return;
	}

	m_bIsExpanded = true;
	updateIcons();
	updateVisibility();
}

void ComponentView::collapse() {
	if ( ! m_bIsExpanded ) {
		return;
	}

	m_bIsExpanded = false;
    updateIcons();
	updateVisibility();
}

void ComponentView::deleteComponent() {
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();

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

void ComponentView::replaceLayer( int nLayer )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	const auto pInstrument = pHydrogen->getSelectedInstrument();
	if ( m_pComponent == nullptr || pInstrument == nullptr ) {
		return;
	}
	auto pLayer = m_pComponent->getLayer( nLayer );
	if ( pLayer == nullptr ) {
		ERRORLOG( QString( "Unable to obtain layer [%1]" ).arg( nLayer ) );
		return;
	}

	QString sSamplePath;
	auto pSample = pLayer->getSample();
	if ( pSample != nullptr && !pSample->getFilePath().isEmpty() ) {
		sSamplePath = pSample->getFilePath();
	}
	else {
		sSamplePath = pLayer->getFallbackSampleFileName();
	}

	QFileInfo fileInfo( pSample->getFilePath() );
	const auto sDir = fileInfo.absoluteDir().absolutePath();
	const auto sFileName = fileInfo.absoluteFilePath();

	auto pFileBrowser =
		new AudioFileBrowser( nullptr, false, true, sDir, sFileName );
	// The first two elements of this list will indicate whether the user has
	// checked the additional options.
	QStringList filename;
	filename << "false" << "false" << "";

	if ( pFileBrowser->exec() == QDialog::Accepted ) {
		filename = pFileBrowser->getSelectedFiles();

		if ( sDir != pFileBrowser->getSelectedDirectory() ) {
			Preferences::get_instance()->setLastOpenLayerDirectory(
				pFileBrowser->getSelectedDirectory()
			);
		}
	}

	delete pFileBrowser;

	if ( filename.size() < 3 || filename[2].isEmpty() ) {
		return;
	}

	const bool bRenameInstrument = filename[0] == "true";
	QString sNewInstrumentName;

	auto pNewInstrument = std::make_shared<Instrument>( pInstrument );
	auto pNewComponent =
		pNewInstrument->getComponent( pInstrument->index( m_pComponent ) );
	if ( pNewComponent == nullptr ) {
		ERRORLOG( "Hiccup while looking up component" );
		return;
	}

	const QString sNewFilePath = filename[2];
	auto pNewSample = Sample::load( sNewFilePath );
	auto pNewLayer = pNewComponent->getLayer( nLayer );
	if ( pNewLayer == nullptr ) {
		ERRORLOG( QString( "Unable to obtain new layer [%1]" ).arg( nLayer ) );
		return;
	}

	pNewInstrument->setSample(
		pNewComponent, pNewLayer, pNewSample, Event::Trigger::Default
	);

	if ( bRenameInstrument ) {
		sNewInstrumentName = sNewFilePath.section( '/', -1 );
		sNewInstrumentName.replace(
			"." + sNewInstrumentName.section( '.', -1 ), ""
		);
	}

	// set automatic velocity
	if ( filename[1] == "true" ) {
		setAutoVelocity();
	}

	pHydrogen->setIsModified( true );

	setSelectedLayer( nLayer );

	// The user choose to rename the instrument according to the (last) filename
	// of the selected sample.
	if ( bRenameInstrument && !sNewInstrumentName.isEmpty() ) {
		pNewInstrument->setName( sNewInstrumentName );

		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2] -> [%3]" )
				.arg( pHydrogenApp->getCommonStrings()
						  ->getActionRenameInstrument() )
				.arg( pInstrument->getName() )
				.arg( sNewInstrumentName )
		);
	}

	pHydrogenApp->pushUndoCommand( new SE_replaceInstrumentAction(
		pNewInstrument, pInstrument,
		SE_replaceInstrumentAction::Type::ReplaceLayer, sNewFilePath
	) );
}

void ComponentView::setComponent(
	std::shared_ptr<H2Core::InstrumentComponent> pComponent
)
{
	if ( m_pComponent != pComponent ) {
		m_pComponent = pComponent;
	}
	m_nSelectedLayer = std::clamp(
		m_nSelectedLayer, 0,
		static_cast<int>(
			m_pComponent != nullptr ? m_pComponent->getLayers().size() - 1 : 0
		)
	);

	m_pLayerPreview->updatePreview();

	// if there is just a single component left, we must not allow deleting it.
	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}
	m_pDeleteComponentAction->setEnabled(
		pInstrument->getComponents()->size() > 1
	);
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

		m_pReplaceLayerAction->setEnabled( true );
		m_pDuplicateLayerAction->setEnabled( true );
		m_pDeleteLayerAction->setEnabled( true );
		m_pEditLayerAction->setEnabled( pLayer->getSample() != nullptr );

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

		m_pReplaceLayerAction->setEnabled( false );
		m_pDuplicateLayerAction->setEnabled( false );
		m_pDeleteLayerAction->setEnabled( false );
		m_pEditLayerAction->setEnabled( false );

		m_pWaveDisplay->updateDisplay( nullptr );
	}
}

void ComponentView::updatePitchDisplay()
{
	if ( m_pComponent != nullptr && m_nSelectedLayer >= 0 &&
		 m_pComponent->getLayer( m_nSelectedLayer ) != nullptr ) {
		const QString sNewPitch = QString( "%1" ).arg(
			m_pComponent->getLayer( m_nSelectedLayer )->getPitch(), -2, 'f', 2,
			'0'
		);
		if ( m_pLayerPitchLCD->text() != sNewPitch ) {
			m_pLayerPitchLCD->setText( sNewPitch );
		}
	}
	else {
		m_pLayerPitchLCD->setText( "" );
	}
}

void ComponentView::updateVisibility()
{
	m_pComponentWidget->setVisible( m_bIsExpanded );
	m_pLayerWidget->setVisible( m_bIsExpanded );
	m_pToolBarComponent->setVisible( m_bIsExpanded );
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
		addNewLayer();
	}
}

void ComponentView::showSampleEditor() {
	if ( m_pComponent == nullptr ) {
		return;
	}
	auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}

	auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
	if ( pLayer == nullptr || pLayer->getSample() == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	pHydrogenApp->showSampleEditor( pLayer, m_pComponent, pInstrument );
}

void ComponentView::removeLayerButtonClicked() {
	if ( m_pComponent == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();

	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	auto pNewInstrument = std::make_shared<Instrument>( pInstrument );
	auto pNewComponent = pNewInstrument->getComponent(
		pInstrument->index( m_pComponent ) );
	if ( pNewComponent == nullptr ) {
		ERRORLOG( "Hiccup while looking up component" );
		return;
	}

	auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );
	if ( pLayer == nullptr ) {
		// Nothing to remove
		return;
	}
	const QString sLayerName = pLayer->getSample() != nullptr ?
		pLayer->getSample()->getFileName() : "nullptr";

	pNewInstrument->removeLayer(
		pNewComponent, m_nSelectedLayer, Event::Trigger::Default
	);

	pHydrogenApp->pushUndoCommand( new SE_replaceInstrumentAction(
		pNewInstrument, pInstrument,
		SE_replaceInstrumentAction::Type::DeleteLayer, sLayerName
	) );
}

void ComponentView::addNewLayer()
{
	if ( m_pComponent == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();

	QString sPath =
		Preferences::get_instance()->getLastOpenLayerDirectory();
	const QString sFileName = "";
	if ( !Filesystem::dir_readable( sPath, false ) ) {
		sPath = QDir::homePath();
	}

	AudioFileBrowser* pFileBrowser =
		new AudioFileBrowser( nullptr, true, true, sPath, sFileName );
	// The first two elements of this list will indicate whether the user has
	// checked the additional options.
	QStringList selectedFiles;
	selectedFiles << "false" << "false" << "";

	if ( pFileBrowser->exec() == QDialog::Accepted ) {
		selectedFiles = pFileBrowser->getSelectedFiles();

		if ( sPath != pFileBrowser->getSelectedDirectory() ) {
			Preferences::get_instance()->setLastOpenLayerDirectory(
				pFileBrowser->getSelectedDirectory()
			);
		}
	}

	delete pFileBrowser;

	if ( selectedFiles[2].isEmpty() ) {
		return;
	}

	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	const bool bRenameInstrument = selectedFiles[0] == "true";
	QString sNewInstrumentName;

	auto pNewInstrument = std::make_shared<Instrument>( pInstrument );
	auto pNewComponent =
		pNewInstrument->getComponent( pInstrument->index( m_pComponent ) );
	if ( pNewComponent == nullptr ) {
		ERRORLOG( "Hiccup while looking up component" );
		return;
	}

	QStringList newLayersPaths;
	if ( selectedFiles.size() > 2 ) {
		for ( int ii = 2; ii < selectedFiles.size(); ++ii ) {
			auto pNewSample = Sample::load( selectedFiles[ii] );
			if ( pNewSample == nullptr ) {
				ERRORLOG(
					QString( "Failed to load [%1]" ).arg( selectedFiles[ii] )
				);
				continue;
			}
			newLayersPaths << selectedFiles[ii];

			++m_nSelectedLayer;

			const auto pNewLayer =
				std::make_shared<H2Core::InstrumentLayer>( pNewSample );

			pNewInstrument->addLayer(
				pNewComponent, pNewLayer, m_nSelectedLayer,
				Event::Trigger::Default
			);

			if ( bRenameInstrument ) {
				sNewInstrumentName = selectedFiles[ii].section( '/', -1 );
				sNewInstrumentName.replace(
					"." + sNewInstrumentName.section( '.', -1 ), ""
				);
			}

			// set automatic velocity
			if ( selectedFiles[1] == "true" ) {
				setAutoVelocity();
			}
		}
	}

	// The user choose to rename the instrument according to the (last) filename
	// of the selected sample.
	if ( bRenameInstrument && !sNewInstrumentName.isEmpty() ) {
		pNewInstrument->setName( sNewInstrumentName );

		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2] -> [%3]" )
				.arg( pHydrogenApp->getCommonStrings()
						  ->getActionRenameInstrument() )
				.arg( pInstrument->getName() )
				.arg( sNewInstrumentName )
		);
	}

	pHydrogenApp->pushUndoCommand( new SE_replaceInstrumentAction(
		pNewInstrument, pInstrument, SE_replaceInstrumentAction::Type::AddLayer,
		newLayersPaths.join( " " )
	) );
}

void ComponentView::setAutoVelocity()
{
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
	 if ( m_pComponent == nullptr ) {
		 return;
	 }

	 if ( m_pSampleSelectionCombo->count() < 3 ) {
		 // We flushed the widget while disabling the ComponentView and are
		 // in the process of refilling it.
		 return;
	 }

	if ( selected == 0 ){
		m_pComponent->setSelection( InstrumentComponent::Selection::Velocity );
	}
	else if ( selected == 1 ){
		m_pComponent->setSelection( InstrumentComponent::Selection::RoundRobin );
	}
	else if ( selected == 2){
		m_pComponent->setSelection( InstrumentComponent::Selection::Random );
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
