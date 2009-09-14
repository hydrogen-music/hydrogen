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

#include "SoundLibraryExportDialog.h"
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/SoundLibrary.h>

#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>

#include <hydrogen/adsr.h>
#include <hydrogen/sample.h>
#include <hydrogen/instrument.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/SoundLibrary.h>
#include <hydrogen/data_path.h>

#include <memory>
#include <QtGui>

using namespace H2Core;

SoundLibraryExportDialog::SoundLibraryExportDialog( QWidget* pParent )
 : QDialog( pParent )
 , Object( "SoundLibraryExportDialog" )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( trUtf8( "Export Sound Library" ) );
	setFixedSize( width(), height() );
	updateDrumkitList();
}




SoundLibraryExportDialog::~SoundLibraryExportDialog()
{
	INFOLOG( "DESTROY" );

	for (uint i = 0; i < drumkitInfoList.size(); i++ ) {
		Drumkit* info = drumkitInfoList[i];
		delete info;
	}
	drumkitInfoList.clear();
}



void SoundLibraryExportDialog::on_exportBtn_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	QString drumkitName = drumkitList->currentText();

	H2Core::LocalFileMng fileMng;
	QString drumkitDir = fileMng.getDrumkitDirectory( drumkitName );

	QString saveDir = drumkitPathTxt->text();
	QString cmd = QString( "cd " ) + drumkitDir + "; tar czf \"" + saveDir + "/" + drumkitName + ".h2drumkit\" \"" + drumkitName + "\"";

	INFOLOG( "cmd: " + cmd );
	system( cmd.toLocal8Bit() );

	QApplication::restoreOverrideCursor();
	QMessageBox::information( this, "Hydrogen", "Drumkit exported." );
}

void SoundLibraryExportDialog::on_drumkitPathTxt_textChanged( QString str )
{
	QString path = drumkitPathTxt->text();
	if (path.isEmpty()) {
		exportBtn->setEnabled( false );
	}
	else {
		exportBtn->setEnabled( true );
	}
}

void SoundLibraryExportDialog::on_browseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();

	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode(QFileDialog::Directory);
	fd->setFilter( "Hydrogen drumkit (*.h2drumkit)" );
	fd->setDirectory( lastUsedDir );
	fd->setAcceptMode( QFileDialog::AcceptSave );
	fd->setWindowTitle( trUtf8( "Export drumkit" ) );

	QString filename;
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}

	if ( !filename.isEmpty() ) {
		drumkitPathTxt->setText( filename );
		lastUsedDir = fd->directory().absolutePath();
	}
	INFOLOG( "Filename: " + filename );
}

void SoundLibraryExportDialog::updateDrumkitList()
{
	INFOLOG( "[updateDrumkitList]" );

	drumkitList->clear();

	for (uint i = 0; i < drumkitInfoList.size(); i++ ) {
		Drumkit* info = drumkitInfoList[i];
		delete info;
	}
	drumkitInfoList.clear();

	//LocalFileMng mng;
	std::vector<QString> userList = Drumkit::getUserDrumkitList();
	for (uint i = 0; i < userList.size(); i++) {
		QString absPath =  userList[i];

		Drumkit *info = Drumkit::load( absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			drumkitList->addItem( info->getName() );
		}
	}


	std::vector<QString> systemList = Drumkit::getSystemDrumkitList();
	for (uint i = 0; i < systemList.size(); i++) {
		QString absPath = systemList[i];
		Drumkit *info = Drumkit::load( absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			drumkitList->addItem( info->getName() );
		}
	}

	
	/// \todo sort in exportTab_drumkitList
//	drumkitList->sort();

	drumkitList->setCurrentIndex( 0 );
}
