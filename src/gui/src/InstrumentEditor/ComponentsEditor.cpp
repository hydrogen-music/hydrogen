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
#include "../HydrogenApp.h"
#include "../UndoActions.h"

using namespace H2Core;

ComponentsEditor::ComponentsEditor( InstrumentEditorPanel* pPanel )
	: QWidget( pPanel )
	, m_pInstrumentEditorPanel( pPanel )
	, m_nSelectedComponent( 0 )
{
	setFixedWidth( InstrumentEditorPanel::nWidth );
	setMinimumSize( InstrumentEditorPanel::nWidth,
					ComponentView::nExpandedHeight );
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );

	m_pComponentsWidget = new QWidget( this );
	m_pComponentsWidget->setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Expanding );
	m_pComponentsWidget->setMinimumSize(
		InstrumentEditorPanel::nWidth, ComponentView::nExpandedHeight );
	m_pComponentsWidget->setMaximumSize(
		InstrumentEditorPanel::nWidth, 16777215 );
	m_pComponentsLayout = new QVBoxLayout();
	m_pComponentsLayout->setSpacing( 0 );
	m_pComponentsLayout->setMargin( 0 );
	m_pComponentsWidget->setLayout( m_pComponentsLayout );

	m_pScrollArea = new WidgetScrollArea( this );
	m_pScrollArea->setFrameShape( QFrame::NoFrame );
	m_pScrollArea->setFixedWidth( InstrumentEditorPanel::nWidth );
	m_pScrollArea->setMinimumHeight( ComponentView::nExpandedHeight );
	m_pScrollArea->setVerticalScrollBarPolicy(
		Qt::ScrollBarAsNeeded );
	m_pScrollArea->setHorizontalScrollBarPolicy(
		Qt::ScrollBarAlwaysOff);
	m_pScrollArea->setFocusPolicy( Qt::ClickFocus );
	m_pScrollArea->setWidget( m_pComponentsWidget );
	m_pComponentsWidget->show();

	auto pMainLayout = new QVBoxLayout();
	pMainLayout->setSpacing( 0 );
	pMainLayout->setMargin( 0 );
	pMainLayout->addWidget( m_pScrollArea );
	setLayout( pMainLayout );

	updateComponents();
}

ComponentsEditor::~ComponentsEditor() {
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
}

void ComponentsEditor::updateEditor()
{
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	std::shared_ptr<InstrumentComponent> pComponent = nullptr;
	if ( pInstrument != nullptr ) {
		// As each instrument can have an arbitrary compoments, we have to
		// ensure to select a valid one.
		m_nSelectedComponent = std::clamp(
			m_nSelectedComponent, 0,
			static_cast<int>(pInstrument->getComponents()->size()) - 1 );
	}
	else {
		m_nSelectedComponent = 0;
	}

	for ( const auto& ppComponentView : m_componentViews ) {
		ppComponentView->updateView();
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

void ComponentsEditor::addComponentAction() {
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

	// New components will be appended and should be selected.
	setSelectedComponent( pInstrument->getComponents()->size() );
	updateEditor();
}

void ComponentsEditor::deleteComponentAction() {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}

	if ( pInstrument->getComponents()->size() <= 1 ) {
		ERRORLOG( "There is just a single component remaining. This one can not be deleted." );
		return;
	}

	auto pComponent = pInstrument->getComponent( m_nSelectedComponent );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to find selected component [%1]" )
				  .arg( m_nSelectedComponent ) );
		return;
	}
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	const auto sName = pComponent->getName();

	auto pNewInstrument = std::make_shared<Instrument>( pInstrument );
	pNewInstrument->removeComponent( m_nSelectedComponent );

	pHydrogenApp->pushUndoCommand(
		new SE_replaceInstrumentAction(
			pNewInstrument, pInstrument,
			SE_replaceInstrumentAction::Type::DeleteComponent, sName ) );
	pHydrogenApp->showStatusBarMessage(
		QString( "%1 [%2]" ).arg( pCommonStrings->getActionDeleteComponent() )
		.arg( sName ) );

	setSelectedComponent(
		std::clamp( m_nSelectedComponent, 0,
					static_cast<int>(pInstrument->getComponents()->size()) - 2 ) );
	updateEditor();
}

	}
}
