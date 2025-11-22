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

#include "../Rack/InstrumentEditor.h"
#include "LayerPreview.h"
#include "WaveDisplay.h"
#include "../AudioFileBrowser/AudioFileBrowser.h"
#include "../CommonStrings.h"
#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"
#include "../Rack/Rack.h"
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
	setFixedWidth( Rack::nWidth );
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
		Button::Type::Push, "minus.svg", "",
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
		pCommonStrings->getSmallMuteButton(), QSize(), tr( "Mute component" ),
		true );
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
		pCommonStrings->getSmallSoloButton(), QSize(), tr( "Solo component" ),
		true );
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
	pVBoxComponentLayout->setSpacing( 0 );
	pVBoxComponentLayout->setContentsMargins(
		ComponentView::nMargin, 0,
		ComponentView::nMargin, 0 );
	m_pComponentWidget->setLayout( pVBoxComponentLayout );

	// Layer preview

	m_pLayerPreview = new LayerPreview( this );

	m_pLayerScrollArea = new QScrollArea( m_pComponentWidget );
	m_pLayerScrollArea->setFocusPolicy( Qt::ClickFocus );
	m_pLayerScrollArea->setFrameShape( QFrame::NoFrame );
	m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	if ( InstrumentComponent::getMaxLayers() > 16 ) {
		m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	}
	m_pLayerScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pLayerScrollArea->setWidget( m_pLayerPreview  );

    // Toolbar with buttons

	m_pToolBar = new QToolBar( m_pComponentWidget );
	m_pToolBar->setFixedSize(
		ComponentView::nWidth - ComponentView::nMargin * 2,
		ComponentView::nToolBarHeight
	);
	m_pToolBar->setFocusPolicy( Qt::NoFocus );

	auto createAction = [&]( const QString& sText, bool bCheckable ) {
		auto pAction = new QAction( m_pToolBar );
		pAction->setCheckable( bCheckable );
		pAction->setIconText( sText );
		pAction->setToolTip( sText );

		return pAction;
	};

	m_pNewLayerAction =
		createAction( pCommonStrings->getNewLayerButton(), false );
	connect( m_pNewLayerAction, &QAction::triggered, [=]() {
		loadLayerBtnClicked();
	} );
	m_pToolBar->addAction( m_pNewLayerAction );

	m_pToolBar->addSeparator();

	m_pReplaceLayerAction =
		createAction( pCommonStrings->getLoadLayerButton(), false );
	connect( m_pReplaceLayerAction, &QAction::triggered, [=]() {
		loadLayerBtnClicked();
	} );
	m_pToolBar->addAction( m_pReplaceLayerAction );

	m_pDuplicateLayerAction =
		createAction( pCommonStrings->getDuplicateLayerButton(), false );
	connect( m_pDuplicateLayerAction, &QAction::triggered, [=]() {
		loadLayerBtnClicked();
	} );
	m_pToolBar->addAction( m_pDuplicateLayerAction );

	m_pDeleteLayerAction =
		createAction( pCommonStrings->getDeleteLayerButton(), false );
	connect( m_pDeleteLayerAction, &QAction::triggered, [=]() {
		removeLayerButtonClicked();
	} );
	m_pToolBar->addAction( m_pDeleteLayerAction );

	m_pToolBar->addSeparator();

	m_pEditLayerAction =
		createAction( pCommonStrings->getEditLayerButton(), false );
	connect( m_pEditLayerAction, &QAction::triggered, [=]() {
		showSampleEditor();
	} );
	m_pToolBar->addAction( m_pEditLayerAction );

	m_pToolBar->addSeparator();

	m_pLayerMuteBtn = new Button(
		m_pToolBar,
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
	m_pToolBar->addWidget( m_pLayerMuteBtn );

	m_pLayerSoloBtn = new Button(
		m_pToolBar,
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
	m_pToolBar->addWidget( m_pLayerSoloBtn );

	m_pLayerGainRotary = new Rotary(
		m_pToolBar, Rotary::Type::Normal, tr( "Layer gain" ), false,
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
	m_pToolBar->addWidget( m_pLayerGainRotary );

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
	pVBoxComponentLayout->addWidget( m_pLayerScrollArea );
    pVBoxComponentLayout->addWidget( m_pToolBar );
    pVBoxComponentLayout->addSpacing( ComponentView::nVerticalSpacing );
	pVBoxComponentLayout->addWidget( pSampleSelectionWidget );
    pVBoxComponentLayout->addSpacing( ComponentView::nVerticalSpacing );

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
	QColor color;
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
		color = Qt::white;
	} else {
		sIconPath.append( "/icons/black/" );
		color = Qt::black;
	}

	m_pNewLayerAction->setIcon( QIcon( sIconPath + "plus.svg" ) );
	m_pReplaceLayerAction->setIcon( QIcon( sIconPath + "folder.svg" ) );
	m_pDuplicateLayerAction->setIcon( QIcon( sIconPath + "duplicate.svg" ) );
	m_pDeleteLayerAction->setIcon( QIcon( sIconPath + "bin.svg" ) );
	m_pEditLayerAction->setIcon( QIcon( sIconPath + "sample-editor.svg" ) );
}

void ComponentView::updateStyleSheet() {
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	const QColor headerColor = pColorTheme->m_windowColor;
	const QColor headerTextColor = pColorTheme->m_windowTextColor;
	const QColor borderHeaderLightColor = headerColor.lighter(
		Skin::nListBackgroundLightBorderScaling );
	const QColor borderHeaderDarkColor = headerColor.darker(
		Skin::nListBackgroundDarkBorderScaling );

	const QColor layerColor = pColorTheme->m_baseColor;
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
} \
QToolBar {\
    spacing: 1px; \
    border-top: 1px solid #000; \
    border-bottom: 1px solid %6; \
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

	auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
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
	auto pEv = static_cast<MouseEvent*>( pEvent );
	if ( pEvent->button() == Qt::RightButton ) {
		m_pPopup->popup( pEv->globalPosition().toPoint() );
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

	// If there is only a single component left, we do not allow to remove it.
	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	if ( pInstrument != nullptr && pInstrument->getComponents()->size() <= 1 ) {
		m_pDeleteAction->setDisabled( true );
	}
	else {
		m_pDeleteAction->setDisabled( false );
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

	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	pNewInstrument->setLayer(
		pNewComponent, nullptr, m_nSelectedLayer, Event::Trigger::Default
	);

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

	pHydrogenApp->pushUndoCommand(
		new SE_replaceInstrumentAction(
			pNewInstrument, pInstrument,
			SE_replaceInstrumentAction::Type::DeleteLayer, sLayerName ) );
}

void ComponentView::loadLayerBtnClicked()
{
	if ( m_pComponent == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();

	QString sPath = Preferences::get_instance()->getLastOpenLayerDirectory();
	QString sFileName = "";
	if ( !Filesystem::dir_readable( sPath, false ) ) {
		sPath = QDir::homePath();
	}

	// In case the button was pressed while a layer was selected, we
	// try to use path of the associated sample as default one.
	if ( m_nSelectedLayer > 0 ) {
		auto pLayer = m_pComponent->getLayer( m_nSelectedLayer );

		if ( pLayer != nullptr ) {
			auto pSample = pLayer->getSample();

			if ( pSample != nullptr ) {
				if ( !pSample->getFilePath().isEmpty() ) {
					QFileInfo fileInfo( pSample->getFilePath() );
					sPath = fileInfo.absoluteDir().absolutePath();
					sFileName = fileInfo.absoluteFilePath();
				}
			}
		}
	}

	AudioFileBrowser* pFileBrowser =
		new AudioFileBrowser( nullptr, true, true, sPath, sFileName );
	// The first two elements of this list will indicate whether the user has
	// checked the additional options.
	QStringList filename;
	filename << "false" << "false" << "";

	if ( pFileBrowser->exec() == QDialog::Accepted ) {
		filename = pFileBrowser->getSelectedFiles();

		// Only overwrite the default directory if we didn't start
		// from an existing file or the final directory differs from
		// the starting one.
		if ( sFileName.isEmpty() ||
			 sPath != pFileBrowser->getSelectedDirectory() ) {
			Preferences::get_instance()->setLastOpenLayerDirectory(
				pFileBrowser->getSelectedDirectory()
			);
		}
	}

	delete pFileBrowser;

	if ( filename[2].isEmpty() ) {
		return;
	}

	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	bool bRenameInstrument = false;
	if ( filename[0] == "true" ) {
		bRenameInstrument = true;
	}
	QString sNewInstrumentName;

	auto pNewInstrument = std::make_shared<Instrument>( pInstrument );
	auto pNewComponent =
		pNewInstrument->getComponent( pInstrument->index( m_pComponent ) );
	if ( pNewComponent == nullptr ) {
		ERRORLOG( "Hiccup while looking up component" );
		return;
	}

	QStringList newLayersPaths;
	int nLastInsertedLayer = m_nSelectedLayer;
	if ( filename.size() > 2 ) {
		for ( int ii = 2; ii < filename.size(); ++ii ) {
			int nnLayer = m_nSelectedLayer + ii - 2;
			if ( ( ii - 2 >= InstrumentComponent::getMaxLayers() ) ||
				 ( nnLayer + 1 > InstrumentComponent::getMaxLayers() ) ) {
				break;
			}

			auto pNewSample = Sample::load( filename[ii] );
			newLayersPaths << filename[ii];

			pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

			// If we're using multiple layers, we start inserting the first
			// layer at nSelectedLayer and the next layer at nSelectedLayer + 1.
			auto pLayer = pNewComponent->getLayer( nnLayer );
			if ( pLayer != nullptr ) {
				pNewInstrument->setSample(
					pNewComponent, pLayer, pNewSample, Event::Trigger::Default
				);
			}
			else {
				pLayer =
					std::make_shared<H2Core::InstrumentLayer>( pNewSample );
				pNewInstrument->setLayer(
					pNewComponent, pLayer, nnLayer, Event::Trigger::Default
				);
			}
			nLastInsertedLayer = nnLayer;

			if ( bRenameInstrument ) {
				sNewInstrumentName = filename[ii].section( '/', -1 );
				sNewInstrumentName.replace(
					"." + sNewInstrumentName.section( '.', -1 ), ""
				);
			}

			// set automatic velocity
			if ( filename[1] == "true" ) {
				setAutoVelocity();
			}

			pHydrogen->getAudioEngine()->unlock();
		}

		pHydrogen->setIsModified( true );
	}

	setSelectedLayer( nLastInsertedLayer );
	updateView();

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
