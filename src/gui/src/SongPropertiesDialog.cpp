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

#include "SongPropertiesDialog.h"
#include "Skin.h"
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>

#include <QPixmap>

using namespace H2Core;

SongPropertiesDialog::SongPropertiesDialog(QWidget* parent)
 : QDialog(parent)
{
	setupUi( this );

	adjustSize();
	setMaximumSize( width(), height() );
	setMinimumSize( width(), height() );

	setWindowTitle( tr( "Song properties" ) );

	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	songNameTxt->setText( pSong->getName() );

	authorTxt->setText( pSong->getAuthor() );
	notesTxt->append( pSong->getNotes() );
	licenseTxt->setText( pSong->getLicense() );
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
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();

	pSong->setName( songNameTxt->text() );
	pSong->setAuthor( authorTxt->text() );
	pSong->setNotes( notesTxt->toPlainText() );
	pSong->setLicense( licenseTxt->text() );

	accept();
}
