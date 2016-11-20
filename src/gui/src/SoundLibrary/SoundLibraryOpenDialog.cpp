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
	p_soundLib = new SoundLibraryPanel( NULL, true );
	pVBox->addWidget( p_soundLib, 0, 0 );


	// Buttons
	QHBoxLayout *pButtonsBox = new QHBoxLayout();

	pButtonsBox->addStretch();

	p_btnOk = new QPushButton( "Load" );
	pButtonsBox->addWidget( p_btnOk );

	p_btnCancel = new QPushButton( "Cancel" );
	pButtonsBox->addWidget( p_btnCancel );

	pButtonsBox->addStretch();

	pVBox->addLayout( pButtonsBox );


	this->setLayout( pVBox );

	connect( p_soundLib, SIGNAL( item_changed ( bool ) ), this, SLOT( on_soundLib_item_changed( bool ) ) );
	connect( p_btnOk, SIGNAL( clicked ( ) ), this, SLOT( on_open_btn_clicked( ) ) );
	connect( p_btnCancel, SIGNAL( clicked ( ) ), this, SLOT( on_cancel_btn_clicked( ) ) );
}


SoundLibraryOpenDialog::~SoundLibraryOpenDialog()
{

}


void SoundLibraryOpenDialog::on_soundLib_item_changed( bool bDrumkitSelected)
{
	p_btnOk->setEnabled( bDrumkitSelected );
}


void SoundLibraryOpenDialog::on_open_btn_clicked()
{
	p_soundLib->on_drumkitLoadAction();
	accept();
}


void SoundLibraryOpenDialog::on_cancel_btn_clicked()
{
	accept();
}
