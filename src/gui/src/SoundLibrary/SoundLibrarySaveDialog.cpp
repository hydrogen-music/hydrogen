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

#include "SoundLibrarySaveDialog.h"
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/drumkit.h>
#include <QMessageBox>

const char* SoundLibrarySaveDialog::__class_name = "SoundLibrarySaveDialog";

SoundLibrarySaveDialog::SoundLibrarySaveDialog( QWidget* pParent )
 : QDialog( pParent )
 , Object( __class_name )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( trUtf8( "Save Sound Library" ) );
	setFixedSize( width(), height() );
}

SoundLibrarySaveDialog::~SoundLibrarySaveDialog()
{
	INFOLOG( "DESTROY" );

}

void SoundLibrarySaveDialog::on_saveBtn_clicked()
{
	if( nameTxt->text().isEmpty() ){
		QMessageBox::information( this, "Hydrogen", trUtf8 ( "Please supply at least a valid name"));
		return;
	}

	bool Overwrite = false;

	if(H2Core::Drumkit::user_drumkit_exists( nameTxt->text() )){
		QMessageBox msgBox;
		msgBox.setText(trUtf8("A library with the same name already exists. Do you want to overwrite the existing library?"));
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);

		int ret = msgBox.exec();

		if(ret == QMessageBox::Yes){
			Overwrite = true;
		} else {
			return;
		}
	}

	if( !H2Core::Drumkit::save( nameTxt->text(),
								authorTxt->text(),
								infoTxt->toHtml(),
								licenseTxt->text(),
								H2Core::Hydrogen::get_instance()->getSong()->get_instrument_list(),
								H2Core::Hydrogen::get_instance()->getSong()->get_components(),
								Overwrite ) ) {
		QMessageBox::information( this, "Hydrogen", trUtf8 ( "Saving of this library failed."));
		return;
	}
	accept();
}


