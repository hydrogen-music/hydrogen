/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "InstrumentRack.h"
#include "Skin.h"
#include "Widgets/Button.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "SoundLibrary/SoundLibraryPanel.h"

#include <QGridLayout>

const char* InstrumentRack::__class_name = "InstrumentRack";

InstrumentRack::InstrumentRack( QWidget *pParent )
 : QWidget( pParent )
 , Object( __class_name )
{
	INFOLOG( "INIT" );

	resize( 290, 405 );
	setMinimumSize( width(), height() );
	setFixedWidth( width() );


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

