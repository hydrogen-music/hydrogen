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
	setFixedWidth( 290 );

	const auto pComponent =
		pPanel->getInstrument()->getComponent( m_nSelectedComponent );
	m_pComponentView = new ComponentView( this, pComponent );

	auto pVBoxMainLayout = new QVBoxLayout();
	pVBoxMainLayout->setSpacing( 0 );
	pVBoxMainLayout->setMargin( 0 );
	pVBoxMainLayout->addWidget( m_pComponentView );
	setLayout( pVBoxMainLayout );
}

ComponentsEditor::~ComponentsEditor() {
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

	updateActivation();

	if ( pInstrument != nullptr ) {
		pComponent = pInstrument->getComponent( m_nSelectedComponent );
	}
	m_pComponentView->setComponent( pComponent );
	m_pComponentView->updateView();
}

void ComponentsEditor::updateActivation() {
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

// void ComponentsEditor::populateComponentMenu() {
// 	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
// 	const int nSelectedComponent =
// 		m_pInstrumentEditorPanel->getSelectedComponent();
// 	if ( pInstrument == nullptr ) {
// 		return;
// 	}

// 	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

// 	m_pComponentMenu->clear();

// 	// Actions to switch between the drumkits
// 	for ( int ii = 0; ii < pInstrument->getComponents()->size(); ++ii ) {
// 		const auto ppComponent = pInstrument->getComponent( ii );
// 		if ( ppComponent != nullptr ) {
// 			auto pAction = m_pComponentMenu->addAction(
// 				ppComponent->getName(), this,
// 				[=](){ switchComponentAction( ii ); } );;
// 			if ( ii == nSelectedComponent ) {
// 				m_pComponentMenu->setDefaultAction( pAction );
// 			}
// 		}
// 	}
// 	m_pComponentMenu->addSeparator();
// 	m_pComponentMenu->addAction( pCommonStrings->getMenuActionAdd(), this,
// 								 SLOT( addComponentAction() ) );
// 	auto pDeleteAction = m_pComponentMenu->addAction(
// 		pCommonStrings->getMenuActionDelete(), this, SLOT( deleteComponentAction() ) );
// 	if ( pInstrument->getComponents()->size() < 2 ) {
// 		// If there is just a single component present, it must not be removed.
// 		pDeleteAction->setEnabled( false );
// 	}

// 	m_pComponentMenu->addAction( pCommonStrings->getMenuActionRename(), this,
// 								 SLOT( renameComponentAction() ) );
// }

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

void ComponentsEditor::switchComponentAction( int nId ) {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}

	const auto pComponent = pInstrument->getComponent( nId );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1]" )
				  .arg( nId ) );
		return;
	}

	setSelectedComponent( nId );
	updateEditor();
}
