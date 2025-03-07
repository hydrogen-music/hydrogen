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

#include "InstrumentEditor.h"
#include "LayerPreview.h"
#include "../HydrogenApp.h"

InstrumentEditorPanel::InstrumentEditorPanel( QWidget *pParent )
	: m_nSelectedComponent( 0 )
	, m_nSelectedLayer( 0 )
{
	UNUSED( pParent );

	m_pInstrument = H2Core::Hydrogen::get_instance()->getSelectedInstrument();

	m_pInstrumentEditor = new InstrumentEditor( nullptr, this );

	// LAYOUT
	QGridLayout *vbox = new QGridLayout();
	vbox->setSpacing( 0 );
	vbox->setMargin( 0 );

	vbox->addWidget( m_pInstrumentEditor, 0, 0 );

	this->setLayout( vbox );

	HydrogenApp::get_instance()->addEventListener(this);

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &InstrumentEditorPanel::onPreferencesChanged );
}

InstrumentEditorPanel::~InstrumentEditorPanel() {
}

void InstrumentEditorPanel::updateEditors() {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();

	m_pInstrument = pHydrogen->getSelectedInstrument();

	// Check whether the current selection is still valid.
	if ( pSong != nullptr && m_pInstrument != nullptr ) {
		// As each instrument can have an arbitrary compoments, we have to
		// ensure to select a valid one.
		m_nSelectedComponent = std::clamp(
			m_nSelectedComponent, 0,
			static_cast<int>(m_pInstrument->getComponents()->size()) - 1 );

		m_nSelectedLayer = std::clamp(
			m_nSelectedLayer, 0, H2Core::InstrumentComponent::getMaxLayers() );
	}
	else {
		m_nSelectedLayer = -1;
		m_nSelectedComponent = 0;
	}

	m_pInstrumentEditor->updateEditor();
}

void InstrumentEditorPanel::drumkitLoadedEvent() {
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
	updateEditors();
}

void InstrumentEditorPanel::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		updateEditors();
	}
}

void InstrumentEditorPanel::setSelectedComponent( int nComponent ) {
	m_nSelectedComponent = nComponent;
}
void InstrumentEditorPanel::setSelectedLayer( int nLayer ) {
	m_nSelectedLayer = nLayer;
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
		m_pInstrumentEditor->getLayerPreview()->update();
	}
}
