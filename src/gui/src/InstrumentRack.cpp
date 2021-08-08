/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "InstrumentRack.h"
#include "Skin.h"
#include "Widgets/Button.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "HydrogenApp.h"

#include <QGridLayout>

InstrumentRack::InstrumentRack( QWidget *pParent )
 : QWidget( pParent )
 , Object()
{
	INFOLOG( "INIT" );

	resize( 290, 405 );
	setMinimumSize( width(), height() );
	setFixedWidth( width() );

	m_lastUsedFontSize = H2Core::Preferences::get_instance()->getFontSize();
	QFont fontButtons( H2Core::Preferences::get_instance()->getApplicationFontFamily(), getPointSize( m_lastUsedFontSize ) );

// TAB buttons
	QWidget *pTabButtonsPanel = new QWidget( nullptr );
	pTabButtonsPanel->setFixedHeight( 24 );
	pTabButtonsPanel->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

	// instrument editor button
	m_pShowInstrumentEditorBtn = new ToggleButton(
			pTabButtonsPanel,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize( 130, 17 ), 
			true );

	m_pShowInstrumentEditorBtn->setToolTip( tr( "Show Instrument editor" ) );
	m_pShowInstrumentEditorBtn->setText( tr( "Instrument" ) );
	m_pShowInstrumentEditorBtn->setFont( fontButtons );
	connect( m_pShowInstrumentEditorBtn, SIGNAL( clicked( Button* ) ), this, SLOT( on_showInstrumentEditorBtnClicked() ) );

	// show sound library button
	m_pShowSoundLibraryBtn = new ToggleButton(
			pTabButtonsPanel,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize( 150, 17 ), 
			true );

	m_pShowSoundLibraryBtn->setToolTip( tr( "Show sound library" ) );
	m_pShowSoundLibraryBtn->setText( tr( "Sound library" ) );
	m_pShowSoundLibraryBtn->setFont( fontButtons );
	connect( m_pShowSoundLibraryBtn, SIGNAL( clicked( Button* ) ), this, SLOT( on_showSoundLibraryBtnClicked() ) );

	QHBoxLayout *pTabHBox = new QHBoxLayout();
	pTabHBox->setSpacing( 0 );
	pTabHBox->setMargin( 0 );
	pTabHBox->addWidget( m_pShowInstrumentEditorBtn );
	pTabHBox->addWidget( m_pShowSoundLibraryBtn );

	pTabButtonsPanel->setLayout( pTabHBox );

//~ TAB buttons


	InstrumentEditorPanel::get_instance()->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

	m_pSoundLibraryPanel = new SoundLibraryPanel( nullptr, false );

	// LAYOUT
	QGridLayout *pGrid = new QGridLayout();
	pGrid->setSpacing( 0 );
	pGrid->setMargin( 0 );

	pGrid->addWidget( pTabButtonsPanel, 0, 0, 1, 3 );
	pGrid->addWidget( InstrumentEditorPanel::get_instance(), 2, 1 );
	pGrid->addWidget( m_pSoundLibraryPanel, 2, 1 );

	this->setLayout( pGrid );
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &InstrumentRack::onPreferencesChanged );
	
	on_showInstrumentEditorBtnClicked();	// show the instrument editor as default
}



InstrumentRack::~InstrumentRack()
{
	INFOLOG( "DESTROY" );
}



void InstrumentRack::on_showSoundLibraryBtnClicked()
{
	m_pShowSoundLibraryBtn->setPressed( true );
	m_pShowInstrumentEditorBtn->setPressed( false );

	m_pSoundLibraryPanel->show();
	InstrumentEditorPanel::get_instance()->hide();
}



void InstrumentRack::on_showInstrumentEditorBtnClicked()
{
	m_pShowInstrumentEditorBtn->setPressed( true );
	m_pShowSoundLibraryBtn->setPressed( false );

	InstrumentEditorPanel::get_instance()->show();
	m_pSoundLibraryPanel->hide();
}

void InstrumentRack::onPreferencesChanged( bool bAppearanceOnly ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( m_pShowInstrumentEditorBtn->font().family() != pPref->getApplicationFontFamily() ||
		 m_lastUsedFontSize != pPref->getFontSize() ) {
		m_lastUsedFontSize = H2Core::Preferences::get_instance()->getFontSize();
		
		QFont fontButtons( pPref->getApplicationFontFamily(), getPointSize( m_lastUsedFontSize ) );
		m_pShowInstrumentEditorBtn->setFont( fontButtons );
		m_pShowSoundLibraryBtn->setFont( fontButtons );
	}
}
