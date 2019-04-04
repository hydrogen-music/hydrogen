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

#include "SongPropertiesDialog.h"
#include "Skin.h"
#include <hydrogen/basics/song.h>
#include <hydrogen/hydrogen.h>

#include <QPixmap>

using namespace H2Core;

SongPropertiesDialog::SongPropertiesDialog(QWidget* parent)
 : QDialog(parent)
{
	setupUi( this );

	setMaximumSize( width(), height() );
	setMinimumSize( width(), height() );

	setWindowTitle( trUtf8( "Song properties" ) );

	Song *pSong = Hydrogen::get_instance()->getSong();
	songNameTxt->setText( pSong->__name );

	authorTxt->setText( pSong->__author );
	notesTxt->append( pSong->get_notes() );
	licenseTxt->setText( pSong->get_license() );
}



SongPropertiesDialog::~SongPropertiesDialog()
{
}


void SongPropertiesDialog::on_cancelBtn_clicked()
{
	reject();
}

void SongPropertiesDialog::on_okBtn_clicked()
{
	Song *pSong = Hydrogen::get_instance()->getSong();

	pSong->__name = songNameTxt->text();
	pSong->__author = authorTxt->text();
	pSong->set_notes( notesTxt->toPlainText() );
	pSong->set_license( licenseTxt->text() );

	accept();
}
