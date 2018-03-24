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

#include "SoundLibraryOpenDialog.h"

#include "SoundLibrary/SoundLibraryPanel.h"

using namespace H2Core;

const char* SoundLibraryOpenDialog::__class_name = "SoundLibraryOpenDialog";

SoundLibraryOpenDialog::SoundLibraryOpenDialog( QWidget* pParent )
	: QDialog( pParent )
	, Object( __class_name )
{
	INFOLOG( "INIT" );
	setWindowTitle( trUtf8( "Open Sound Library" ) );
	setFixedSize( 280, 380 );

	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 6 );
	pVBox->setMargin( 9 );


	// Sound Library Panel
	m_pSoundLibraryPanel = new SoundLibraryPanel( nullptr, true );
	pVBox->addWidget( m_pSoundLibraryPanel, 0, 0 );


	// Buttons
	QHBoxLayout *pButtonsBox = new QHBoxLayout();

	pButtonsBox->addStretch();

	m_pOkBtn = new QPushButton( trUtf8("Load") );
	pButtonsBox->addWidget( m_pOkBtn );

	m_pCancelBtn = new QPushButton( trUtf8("Cancel") );
	pButtonsBox->addWidget( m_pCancelBtn );

	pButtonsBox->addStretch();

	pVBox->addLayout( pButtonsBox );


	this->setLayout( pVBox );

	connect( m_pSoundLibraryPanel, SIGNAL( item_changed ( bool ) ), this, SLOT( on_soundLib_item_changed( bool ) ) );
	connect( m_pOkBtn, SIGNAL( clicked ( ) ), this, SLOT( on_open_btn_clicked( ) ) );
	connect( m_pCancelBtn, SIGNAL( clicked ( ) ), this, SLOT( on_cancel_btn_clicked( ) ) );
}


SoundLibraryOpenDialog::~SoundLibraryOpenDialog()
{

}


void SoundLibraryOpenDialog::on_soundLib_item_changed( bool bDrumkitSelected)
{
	m_pOkBtn->setEnabled( bDrumkitSelected );
}


void SoundLibraryOpenDialog::on_open_btn_clicked()
{
	m_pSoundLibraryPanel->on_drumkitLoadAction();
	accept();
}


void SoundLibraryOpenDialog::on_cancel_btn_clicked()
{
	accept();
}
