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

#include "InstrumentEditorPanel.h"

#include <QGridLayout>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Hydrogen.h>

#include "ComponentsEditor.h"
#include "InstrumentEditor.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Widgets/Button.h"

InstrumentEditorPanel::InstrumentEditorPanel( QWidget *pParent ) :
	PixmapWidget( pParent )
{
	setPixmap( "/instrumentEditor/instrumentTab_top.png" );

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pInstrument = H2Core::Hydrogen::get_instance()->getSelectedInstrument();

	auto pVBoxMainLayout = new QVBoxLayout();
	pVBoxMainLayout->setSpacing( 0 );
	pVBoxMainLayout->setMargin( 0 );

	// Editors

	auto pEditorWidget = new QWidget( this );
	auto pStackedEditorLayout = new QStackedLayout();
	pStackedEditorLayout->setMargin( 0 );
	pEditorWidget->setLayout( pStackedEditorLayout );
	m_pInstrumentEditor = new InstrumentEditor( this );
	m_pComponentsEditor = new ComponentsEditor( this );
	pStackedEditorLayout->addWidget( m_pComponentsEditor );
	pStackedEditorLayout->addWidget( m_pInstrumentEditor );

	// Buttons

	auto pHBoxButtonLayout = new QHBoxLayout();
	pHBoxButtonLayout->setSpacing( 0 );
	pHBoxButtonLayout->setMargin( 4 );
	auto pButtonWidget = new QWidget( this );
	pButtonWidget->setLayout( pHBoxButtonLayout );
	pButtonWidget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

	m_pShowInstrumentBtn = new Button(
		pButtonWidget, QSize( 141, 22 ), Button::Type::Toggle, "",
		pCommonStrings->getGeneralButton(), false, QSize(),
		tr( "Show instrument properties" ) );
	m_pShowInstrumentBtn->setChecked( true );
	connect( m_pShowInstrumentBtn, &Button::clicked, [=]() {
		pStackedEditorLayout->setCurrentIndex( 0 );
		m_pShowInstrumentBtn->setChecked( true );
		m_pShowComponentsBtn->setChecked( false );
	});
	pHBoxButtonLayout->addWidget( m_pShowInstrumentBtn );

	m_pShowComponentsBtn = new Button(
		pButtonWidget, QSize( 140, 22 ), Button::Type::Toggle, "",
		pCommonStrings->getLayersButton(), false, QSize(),
		tr( "Show components" ) );
	m_pShowComponentsBtn->setChecked( false );
	connect( m_pShowComponentsBtn, &Button::clicked, [=]() {
		pStackedEditorLayout->setCurrentIndex( 1 );
		m_pShowInstrumentBtn->setChecked( false );
		m_pShowComponentsBtn->setChecked( true );
	});
	pHBoxButtonLayout->addWidget( m_pShowComponentsBtn );

	pVBoxMainLayout->addWidget( pButtonWidget );
	pVBoxMainLayout->addWidget( pEditorWidget );
	setLayout( pVBoxMainLayout );

	HydrogenApp::get_instance()->addEventListener(this);

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &InstrumentEditorPanel::onPreferencesChanged );
}

InstrumentEditorPanel::~InstrumentEditorPanel() {
}

void InstrumentEditorPanel::updateEditors() {
	m_pInstrumentEditor->updateEditor();
	m_pComponentsEditor->updateEditor();
}

void InstrumentEditorPanel::drumkitLoadedEvent() {
	updateInstrument();
	m_pComponentsEditor->updateComponents();
	updateEditors();
}

void InstrumentEditorPanel::instrumentParametersChangedEvent( int nInstrumentNumber )
{
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();

	// Check if either this particular line or all lines should be updated.
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr &&
		 m_pInstrument != nullptr && nInstrumentNumber != -1 &&
		 m_pInstrument !=
		 pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber ) ) {
		// In case nInstrumentNumber does not belong to the currently
		// selected instrument we don't have to do anything.
	}
	else {
		updateEditors();
	}
}

void InstrumentEditorPanel::selectedInstrumentChangedEvent() {
	updateInstrument();
	m_pComponentsEditor->updateComponents();
	updateEditors();
}

void InstrumentEditorPanel::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		updateInstrument();
		m_pComponentsEditor->updateComponents();
		updateEditors();
	}
}

void InstrumentEditorPanel::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Colors ) {
		m_pInstrumentEditor->setStyleSheet(
			QString( "QLabel { background: %1 }" )
			.arg( pPref->getTheme().m_color.m_windowColor.name() ) );
	}
	if ( changes & ( H2Core::Preferences::Changes::Font |
					 H2Core::Preferences::Changes::Colors ) ) {
		m_pComponentsEditor->updateEditor();
	}
}

void InstrumentEditorPanel::updateInstrument() {
	m_pInstrument = H2Core::Hydrogen::get_instance()->getSelectedInstrument();
}
