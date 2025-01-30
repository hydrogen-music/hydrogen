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

#include "DrumkitOpenDialog.h"

#include "SoundLibrary/SoundLibraryPanel.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"

using namespace H2Core;

DrumkitOpenDialog::DrumkitOpenDialog( QWidget* pParent )
	: QDialog( pParent )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	setWindowTitle( tr( "Open Sound Library" ) );
	setFixedSize( 280, 380 );

	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 6 );
	pVBox->setMargin( 9 );


	// Sound Library Panel
	m_pSoundLibraryPanel = new SoundLibraryPanel( nullptr, true );
	pVBox->addWidget( m_pSoundLibraryPanel, 0 );


	// Buttons
	QHBoxLayout *pButtonsBox = new QHBoxLayout();

	pButtonsBox->addStretch();

	m_pOkBtn = new QPushButton( pCommonStrings->getMenuActionLoad() );
	pButtonsBox->addWidget( m_pOkBtn );

	m_pCancelBtn = new QPushButton( pCommonStrings->getButtonCancel() );
	pButtonsBox->addWidget( m_pCancelBtn );

	pButtonsBox->addStretch();

	pVBox->addLayout( pButtonsBox );


	this->setLayout( pVBox );

	connect( m_pSoundLibraryPanel, SIGNAL( item_changed ( bool ) ), this, SLOT( on_soundLib_item_changed( bool ) ) );
	connect( m_pOkBtn, SIGNAL( clicked ( ) ), this, SLOT( on_open_btn_clicked( ) ) );
	connect( m_pCancelBtn, SIGNAL( clicked ( ) ), this, SLOT( on_cancel_btn_clicked( ) ) );
}


DrumkitOpenDialog::~DrumkitOpenDialog()
{

}


void DrumkitOpenDialog::on_soundLib_item_changed( bool bDrumkitSelected)
{
	m_pOkBtn->setEnabled( bDrumkitSelected );
}


void DrumkitOpenDialog::on_open_btn_clicked()
{
	m_pSoundLibraryPanel->on_drumkitLoadAction();
	accept();
}


void DrumkitOpenDialog::on_cancel_btn_clicked()
{
	accept();
}
