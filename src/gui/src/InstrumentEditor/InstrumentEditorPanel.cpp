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

#include <QLabel>
#include <QPixmap>
#include <QGridLayout>

#include <core/Hydrogen.h>
#include <core/Basics/DrumkitComponent.h>

#include "InstrumentEditorPanel.h"
#include "../HydrogenApp.h"


InstrumentEditorPanel* InstrumentEditorPanel::m_pInstance = nullptr;

InstrumentEditorPanel* InstrumentEditorPanel::get_instance()
{
	if ( m_pInstance == nullptr  ) {
		m_pInstance = new InstrumentEditorPanel( nullptr );
	}
	return m_pInstance;
}



InstrumentEditorPanel::InstrumentEditorPanel( QWidget *pParent )
{
	UNUSED( pParent );

	

	m_pInstance = this;
	m_pInstrumentEditor = new InstrumentEditor( nullptr );

	// LAYOUT
	QGridLayout *vbox = new QGridLayout();
	vbox->setSpacing( 0 );
	vbox->setMargin( 0 );

	vbox->addWidget( m_pInstrumentEditor, 0, 0 );

	this->setLayout( vbox );
	m_nLayer = 0;

	HydrogenApp::get_instance()->addEventListener(this);
}



InstrumentEditorPanel::~InstrumentEditorPanel()
{
	INFOLOG( "DESTROY" );
}

void InstrumentEditorPanel::drumkitLoadedEvent() {
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	
	auto pComponentList = pSong->getComponents();
	if ( pComponentList != nullptr && pComponentList->size() > 0 ) {
		m_pInstrumentEditor->selectComponent( pComponentList->front()->get_id() );
	} else {
		m_pInstrumentEditor->selectComponent( -1 );
	}
	m_pInstrumentEditor->selectedInstrumentChangedEvent();
}

void InstrumentEditorPanel::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		drumkitLoadedEvent();
	}
}

void InstrumentEditorPanel::selectLayer( int nLayer )
{
	m_pInstrumentEditor->selectLayer( nLayer );
	m_nLayer = nLayer;
}

void InstrumentEditorPanel::updateWaveDisplay()
{
	selectLayer ( m_nLayer ); // trigger a redisplay of wave preview
}


