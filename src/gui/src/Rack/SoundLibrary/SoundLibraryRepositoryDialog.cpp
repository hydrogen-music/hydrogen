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

#include "SoundLibraryRepositoryDialog.h"

#include <core/Preferences/Preferences.h>

#include <QList>
#include <QInputDialog>
#include <QListWidgetItem>

SoundLibraryRepositoryDialog::SoundLibraryRepositoryDialog( QWidget* pParent )
 : QDialog( pParent )
 {
	setupUi( this );
	
	setWindowTitle( tr( "Edit repository settings" ) );
	adjustSize();
	setMinimumSize( width(), height() );

	updateDialog();

}


//update all values
void SoundLibraryRepositoryDialog::updateDialog(){
	
	const auto pPref = H2Core::Preferences::get_instance();

	/*
		Read serverList from config and put servers into the serverList
	*/
	
	ServerListWidget->clear();
	for ( const auto& ssServer : pPref->m_serverList ) {
		ServerListWidget->addItem( ssServer );
	}
}



///
/// Add new server url

void SoundLibraryRepositoryDialog::on_AddBtn_clicked()
{
	auto pPref = H2Core::Preferences::get_instance();
	bool ok;

	QString text = QInputDialog::getText(this, tr("Edit server list"), tr("URL"), QLineEdit::Normal,QString(""), &ok);
	
	if( ok && !text.isEmpty() ){
		pPref->m_serverList.push_back( text );
	}

	updateDialog();
}

///
/// Delete serverList entry
///
void SoundLibraryRepositoryDialog::on_DeleteBtn_clicked()
{
	QList<QListWidgetItem *> selectedItems;
	selectedItems = ServerListWidget->selectedItems();

	auto  pPref = H2Core::Preferences::get_instance();

	while ( ! selectedItems.isEmpty() ){

		QString selText;
	
		selText = selectedItems.takeFirst()->text();

		pPref->m_serverList.removeAll(selText);

	}
	updateDialog();
}

void SoundLibraryRepositoryDialog::on_CloseBtn_clicked()
{
	accept();
}

SoundLibraryRepositoryDialog::~SoundLibraryRepositoryDialog()
{
	INFOLOG( "DESTROY" );

}
