/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: SongPropertiesDialog.cpp,v 1.8 2005/05/11 16:49:32 comix Exp $
 *
 */

#include "SongPropertiesDialog.h"
#include "Skin.h"
#include "lib/Song.h"

#include "qlineedit.h"
#include "qtextedit.h"

SongPropertiesDialog::SongPropertiesDialog(QWidget* parent) : SongPropertiesDialog_UI(parent) 
{
	setMaximumSize( width(), height() );
	setMinimumSize( width(), height() );
	
	setCaption( trUtf8( "Song properties" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	Song *song = (HydrogenApp::getInstance())->getSong();
	songNameTxt->setText( trUtf8( song->m_sName.c_str() ) );

	authorTxt->setText( trUtf8( song->m_sAuthor.c_str() ) );
	notesTxt->setText( trUtf8( (song->getNotes()).c_str() ) );

}



SongPropertiesDialog::~SongPropertiesDialog() {
}



void SongPropertiesDialog::cancelBtnClicked() {
	reject();
}



void SongPropertiesDialog::okBtnClicked() {
	Song *song = (HydrogenApp::getInstance())->getSong();

	song->m_sName = songNameTxt->text().ascii();
	song->m_sAuthor = authorTxt->text().ascii();
	song->setNotes(notesTxt->text().latin1());
	accept();
}


