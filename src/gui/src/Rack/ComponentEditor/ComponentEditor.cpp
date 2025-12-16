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

#include "ComponentEditor.h"

#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Hydrogen.h>

#include "ComponentView.h"
#include "../Rack.h"
#include "../../CommonStrings.h"
#include "../../Compatibility/MouseEvent.h"
#include "../../HydrogenApp.h"
#include "../../UndoActions.h"

using namespace H2Core;

ComponentEditor::ComponentEditor( QWidget* pParent )
	: QWidget( pParent )
{
	setMinimumSize( Rack::nWidth,
					ComponentView::nHeaderHeight );
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
	setObjectName( "ComponentEditor" );

    auto pHydrogenApp = HydrogenApp::get_instance();
 	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	m_pComponentsWidget = new QWidget( this );
	m_pComponentsWidget->setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Preferred );
	m_pComponentsWidget->setMinimumSize(
		Rack::nWidth, ComponentView::nHeaderHeight );
	m_pComponentsLayout = new QVBoxLayout();
	m_pComponentsLayout->setSpacing( 0 );
	m_pComponentsLayout->setContentsMargins( 0, 0, 0, 0 );
	m_pComponentsWidget->setLayout( m_pComponentsLayout );

	m_pScrollArea = new WidgetScrollArea( this );
	m_pScrollArea->setFocusPolicy( Qt::ClickFocus );
	m_pScrollArea->setFrameShape( QFrame::NoFrame );
	m_pScrollArea->setFixedWidth( Rack::nWidth );
	m_pScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_pScrollArea->setHorizontalScrollBarPolicy(
		Qt::ScrollBarAlwaysOff);
	m_pScrollArea->setWidget( m_pComponentsWidget );
	m_pComponentsWidget->show();

	auto pMainLayout = new QVBoxLayout();
	pMainLayout->setSpacing( 0 );
	pMainLayout->setContentsMargins( 0, 0, 0, 0 );
	pMainLayout->addWidget( m_pScrollArea );
	setLayout( pMainLayout );

	updateComponents();
	updateStyleSheet();

    pHydrogenApp->addEventListener( this );
	connect( pHydrogenApp, &HydrogenApp::preferencesChanged,
			 this, &ComponentEditor::onPreferencesChanged );
}

ComponentEditor::~ComponentEditor() {
}

void ComponentEditor::updateColors() {
	for ( auto& ppComponentView : m_componentViews ) {
		ppComponentView->updateColors();
	}
}

void ComponentEditor::updateComponents() {

	// We add a stretchable spacer item of zero height at the bottom. When
	// appending another widget we have to take remove it first and add another
	// one later on.
	bool bRequiresNewStretch = m_componentViews.size() == 0;
	auto handleStretch = [=]() {
		// In here we assume that there is just one stretch present.
		for ( int ii = 0; ii < m_pComponentsLayout->count(); ++ii ){
			if ( dynamic_cast<QSpacerItem*>(m_pComponentsLayout->itemAt( ii )) !=
				 nullptr ) {
				// Found the stretch
				m_pComponentsLayout->removeItem( m_pComponentsLayout->itemAt( ii ) );
				return true;
			}
		}
		return false;
	};

	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	if ( pInstrument == nullptr || pInstrument->getComponents()->size() == 0 ) {
		// No components at all
		for ( auto& ppComponentView : m_componentViews ) {
			delete ppComponentView;
		}
		m_componentViews.clear();
		return;
	}

	int nnCount = 0;
	for ( const auto& ppComponent : *pInstrument ) {
		if ( ppComponent == nullptr ) {
			continue;
		}
		if ( nnCount < m_componentViews.size() ) {
			// Ensure the correct component is assigned to the view.
			m_componentViews[nnCount]->setComponent( ppComponent );
		}
		else {
			// Create a new view
			auto pNewView = new ComponentView( this, ppComponent );
			connect( pNewView, &ComponentView::expandedOrCollapsed, [=]() {
				updateSize();
			} );
			m_pComponentsLayout->addWidget( pNewView );
			m_componentViews.push_back( pNewView );
			if ( !bRequiresNewStretch ) {
				bRequiresNewStretch = handleStretch();
			}
		}
		++nnCount;
	}

	// Remove superfluous views
	while ( nnCount < m_componentViews.size() ) {
		delete m_componentViews[ m_componentViews.size() - 1 ];
		m_componentViews.pop_back();
	}

	if ( bRequiresNewStretch ) {
		m_pComponentsLayout->addStretch();
	}

	updateSize();
}

void ComponentEditor::updateIcons() {
	for ( auto& ppComponentView : m_componentViews ) {
		ppComponentView->updateIcons();
	}
}

void ComponentEditor::updateEditor() {
	for ( const auto& ppComponentView : m_componentViews ) {
		ppComponentView->updateView();
	}
}

void ComponentEditor::drumkitLoadedEvent() {
	updateComponents();
	updateEditor();
}

void ComponentEditor::instrumentLayerChangedEvent( int nId )
{
	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	if ( pInstrument != nullptr && pInstrument->getId() == nId ) {
		updateComponents();
	}
}

void ComponentEditor::instrumentParametersChangedEvent(
	int nInstrumentNumber
)
{
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();
	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();

	// Check if either this particular line or all lines should be updated.
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr &&
		 pInstrument != nullptr && nInstrumentNumber != -1 &&
		 pInstrument !=
		 pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber ) ) {
		// In case nInstrumentNumber does not belong to the currently
		// selected instrument we don't have to do anything.
	}
	else {
		updateEditor();
	}
}

void ComponentEditor::selectedInstrumentChangedEvent() {
	updateComponents();
	updateEditor();
}

void ComponentEditor::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		updateComponents();
		updateEditor();
	}
}

void ComponentEditor::updateStyleSheet() {
	setStyleSheet( QString( "QWidget#ComponentEditor {background-color: %1;}" )
				   .arg( H2Core::Preferences::get_instance()->
						 getColorTheme()->m_windowColor.name() ) );

	for ( auto& ppComponentView : m_componentViews ) {
		ppComponentView->updateStyleSheet();
	}
}

ComponentView* ComponentEditor::getCurrentView() const {
	for ( const auto& ppComponentView : m_componentViews ) {
		if ( ppComponentView != nullptr || ppComponentView->getIsExpanded() ) {
			return ppComponentView;
		}
	}

	return nullptr;
}

void ComponentEditor::setVisible( bool bVisible )
{
    QWidget::setVisible( bVisible );
	updateSize();
}

void ComponentEditor::addComponent() {
	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}

	/*: Default name for a newly created instrument component. */
	const QString sNewName = tr( "New Component" );

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	auto pNewInstrument = std::make_shared<Instrument>( pInstrument );

	const auto pNewComponent = std::make_shared<InstrumentComponent>( sNewName );
	pNewInstrument->addComponent( pNewComponent );

	pHydrogenApp->pushUndoCommand(
		new SE_replaceInstrumentAction(
			pNewInstrument, pInstrument,
			SE_replaceInstrumentAction::Type::AddComponent, sNewName ) );
	pHydrogenApp->showStatusBarMessage(
		QString( "%1 [%2]" )
			.arg( pCommonStrings->getActionAddComponent() )
			.arg( sNewName )
	);

	// Instant feedback
	updateEditor();
}

void ComponentEditor::onPreferencesChanged(
	const H2Core::Preferences::Changes& changes
)
{
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateColors();
		updateStyleSheet();
	}
	if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		updateIcons();
	}
	if ( changes & ( H2Core::Preferences::Changes::Font |
					 H2Core::Preferences::Changes::Colors ) ) {
		updateEditor();
	}
}

void ComponentEditor::updateSize() {
	int nNewHeight = 0;
	for ( const auto& ppView : m_componentViews ) {
		if ( ppView->getIsExpanded() ) {
			nNewHeight += ppView->getExpandedHeight();
		}
		else {
			nNewHeight += ComponentView::nHeaderHeight;
		}
	}

    m_pComponentsWidget->resize( width(), nNewHeight );

	int nMaxHeight;
	if ( Hydrogen::get_instance()->getGUIState() ==
		 Hydrogen::GUIState::ready ) {
        const auto pRack = HydrogenApp::get_instance()->getRack();
        nMaxHeight = pRack->height() - pRack->tabBar()->height();
	}
	else {
        nMaxHeight = Preferences::get_instance()->getRackProperties().height - 30;
	}

	resize( width(), std::min( nNewHeight, nMaxHeight ) );

    // Handle scroll bar visibility and width componensation programmatically
    // (in order to make the two show consistent behavior).
    const auto bShowScrollBar = nNewHeight > nMaxHeight;
	m_pScrollArea->setVerticalScrollBarPolicy(
		bShowScrollBar ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff
	);
	for ( auto& ppView : m_componentViews ) {
        ppView->accountForScrollbar( bShowScrollBar );
	}
}
