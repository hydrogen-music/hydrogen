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

#include "ComponentsEditor.h"

#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Hydrogen.h>

#include "ComponentView.h"
#include "InstrumentEditorPanel.h"
#include "../CommonStrings.h"
#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "../UndoActions.h"

using namespace H2Core;

ComponentsEditor::ComponentsEditor( InstrumentEditorPanel* pPanel )
	: QWidget( pPanel )
	, m_pInstrumentEditorPanel( pPanel )
{
	setMinimumSize( InstrumentRack::nWidth,
					ComponentView::nExpandedHeight );
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
	setObjectName( "ComponentsEditor" );

 	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pComponentsWidget = new QWidget( this );
	m_pComponentsWidget->setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Expanding );
	m_pComponentsWidget->setMinimumSize(
		InstrumentRack::nWidth, ComponentView::nExpandedHeight );
	m_pComponentsLayout = new QVBoxLayout();
	m_pComponentsLayout->setSpacing( 0 );
	m_pComponentsLayout->setContentsMargins( 0, 0, 0, 0 );
	m_pComponentsWidget->setLayout( m_pComponentsLayout );

	m_pScrollArea = new WidgetScrollArea( this );
	m_pScrollArea->setFocusPolicy( Qt::ClickFocus );
	m_pScrollArea->setFrameShape( QFrame::NoFrame );
	m_pScrollArea->setFixedWidth( InstrumentRack::nWidth );
	m_pScrollArea->setMinimumHeight( ComponentView::nExpandedHeight );
	m_pScrollArea->setVerticalScrollBarPolicy(
		Qt::ScrollBarAlwaysOn );
	m_pScrollArea->setHorizontalScrollBarPolicy(
		Qt::ScrollBarAlwaysOff);
	m_pScrollArea->setFocusPolicy( Qt::ClickFocus );
	m_pScrollArea->setWidget( m_pComponentsWidget );
	m_pComponentsWidget->show();

	auto pMainLayout = new QVBoxLayout();
	pMainLayout->setSpacing( 0 );
	pMainLayout->setContentsMargins( 0, 0, 0, 0 );
	pMainLayout->addWidget( m_pScrollArea );
	setLayout( pMainLayout );

	// Component popup menu
	m_pPopup = new QMenu( this );
	m_pPopup->addAction( pCommonStrings->getMenuActionAdd(), this,
								 SLOT( addComponent() ) );
	auto pDeleteAction = m_pPopup->addAction(
		pCommonStrings->getMenuActionDelete() );
	if ( pPanel->getInstrument()->getComponents()->size() < 2 ) {
		// If there is just a single component present, it must not be removed.
		pDeleteAction->setEnabled( false );
	}

	updateComponents();
	updateStyleSheet();
}

ComponentsEditor::~ComponentsEditor() {
}

void ComponentsEditor::updateColors() {
	for ( auto& ppComponentView : m_componentViews ) {
		ppComponentView->updateColors();
	}
}

void ComponentsEditor::updateComponents() {

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

	auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	if ( pInstrument == nullptr || pInstrument->getComponents()->size() == 0 ) {
		// No components at all
		for ( auto& ppComponentView : m_componentViews ) {
			delete ppComponentView;
		}
		m_componentViews.clear();
		return;
	}

	int nnCount = 0;
	for ( const auto& ppComponent : *pInstrument->getComponents() ) {
		if ( nnCount < m_componentViews.size() ) {
			// Ensure the correct component is assigned to the view.
			m_componentViews[ nnCount ]->setComponent( ppComponent );
		}
		else {
			// Create a new view
			auto pNewView = new ComponentView( this, ppComponent );
			connect( pNewView, &ComponentView::expandedOrCollapsed, [=]() {
				updateSize();
			});
			m_pComponentsLayout->addWidget( pNewView );
			m_componentViews.push_back( pNewView );
			if ( ! bRequiresNewStretch ) {
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

void ComponentsEditor::updateEditor() {
	for ( const auto& ppComponentView : m_componentViews ) {
		ppComponentView->updateView();
	}
}

void ComponentsEditor::updateStyleSheet() {
	setStyleSheet( QString( "QWidget#ComponentsEditor {background-color: %1;}" )
				   .arg( H2Core::Preferences::get_instance()->
						 getColorTheme()->m_windowColor.name() ) );

	for ( auto& ppComponentView : m_componentViews ) {
		ppComponentView->updateStyleSheet();
	}
}

ComponentView* ComponentsEditor::getCurrentView() const {
	for ( const auto& ppComponentView : m_componentViews ) {
		if ( ppComponentView != nullptr || ppComponentView->getIsExpanded() ) {
			return ppComponentView;
		}
	}

	return nullptr;
}

void ComponentsEditor::renameComponent( int nComponentId, const QString& sNewName ) {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}

	auto pComponent = pInstrument->getComponent( nComponentId );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1]" )
				  .arg( nComponentId ) );
		return;
	}

	pComponent->setName( sNewName );

	Hydrogen::get_instance()->setIsModified( true );

	updateEditor();
}

void ComponentsEditor::addComponent() {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}

	bool bIsOkPressed;
	const QString sNewName = QInputDialog::getText(
		this, "Hydrogen", tr( "Component name" ), QLineEdit::Normal,
		"New Component", &bIsOkPressed );
	if ( ! bIsOkPressed ) {
		// Dialog closed using cancel
		return;
	}

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
		QString( "%1 [%2]" ).arg( pCommonStrings->getActionAddComponent() )
		.arg( sNewName ) );

	updateEditor();
}

void ComponentsEditor::mousePressEvent( QMouseEvent* pEvent ) {
	auto pEv = static_cast<MouseEvent*>( pEvent );
	if ( pEvent->button() == Qt::RightButton ) {
		m_pPopup->popup( pEv->globalPosition().toPoint() );
	}
}

void ComponentsEditor::updateSize() {
	int nNewHeight = 0;
	for ( const auto& ppView : m_componentViews ) {
		if ( ppView->getIsExpanded() ) {
			nNewHeight += ComponentView::nExpandedHeight;
		}
		else {
			nNewHeight += ComponentView::nHeaderHeight;
		}

	}

	 m_pComponentsWidget->setMinimumHeight(
	 	std::max( ComponentView::nExpandedHeight, nNewHeight ) );
	 m_pComponentsWidget->resize( width(), nNewHeight );
}
