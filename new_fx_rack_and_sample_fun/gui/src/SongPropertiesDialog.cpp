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
#include <hydrogen/Song.h>
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
//	setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	Song *song = Hydrogen::get_instance()->getSong();
	songNameTxt->setText( song->__name );

	authorTxt->setText( song->__author );
	notesTxt->append( song->get_notes() );
	licenseTxt->setText( song->get_license() );
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
	Song *song = Hydrogen::get_instance()->getSong();

	song->__name = songNameTxt->text();
	song->__author = authorTxt->text();
	song->set_notes( notesTxt->toPlainText() );
	song->set_license( licenseTxt->text() );

	accept();
}
