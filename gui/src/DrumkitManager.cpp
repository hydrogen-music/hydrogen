/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "Skin.h"
#include "DrumkitManager.h"
#include "HydrogenApp.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "PatternEditor/DrumPatternEditor.h"
#include "PatternEditor/PatternEditorInstrumentList.h"
#include <hydrogen/Hydrogen.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/ADSR.h>
#include <hydrogen/Sample.h>
#include <hydrogen/Instrument.h>
#include <hydrogen/H2Exception.h>

#include <QFileDialog>

#include <QCursor>
#include <QPixmap>

using namespace H2Core;

OldDrumkitManager::OldDrumkitManager( QWidget* parent )
 : QWidget( parent )
 , Object( "OldDrumkitManager" )
{
	INFOLOG( "INIT" );

	setupUi( this );

	setMinimumSize( width(), height() );	// not resizable
	setMaximumSize( width(), height() );	// not resizable

	setWindowTitle( trUtf8( "Drumkit manager" ) );
//	setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	// Load drumkit tab
	loadTab_loadDrumkitBtn->setEnabled( false );
	// disabled until is ready
	loadTab_deleteDrumkitBtn->setEnabled( false );

	// save tab
	saveTab_saveBtn->setEnabled( false );

	// import tab
	importTab_infoLbl->setText( trUtf8( "The drumkit will be installed in %1" ).arg( Preferences::getInstance()->getDataDirectory().c_str() ) );
	importTab_importBtn->setEnabled( false );

	updateDrumkitList();
}



OldDrumkitManager::~OldDrumkitManager()
{
	INFOLOG( "DESTROY" );

	for (uint i = 0; i < drumkitInfoList.size(); i++ ) {
		Drumkit* info = drumkitInfoList[i];
		delete info;
	}
	drumkitInfoList.clear();
}



void OldDrumkitManager::updateDrumkitList()
{
	INFOLOG( "[updateDrumkitList]" );

	loadTabDrumkitListBox->clear();
	exportTab_drumkitList->clear();

	for (uint i = 0; i < drumkitInfoList.size(); i++ ) {
		Drumkit* info = drumkitInfoList[i];
		delete info;
	}
	drumkitInfoList.clear();

	//LocalFileMng mng;
	vector<string> userList = Drumkit::getUserDrumkitList();
	for (uint i = 0; i < userList.size(); i++) {
	  string absPath = Preferences::getInstance()->getDataDirectory() + userList[i];
		Drumkit *info = Drumkit::load( absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			loadTabDrumkitListBox->addItem( QString( info->getName().c_str() ) );
			exportTab_drumkitList->addItem( info->getName().c_str() );
		}
	}


	vector<string> systemList = Drumkit::getSystemDrumkitList();
	for (uint i = 0; i < systemList.size(); i++) {
		string absPath = DataPath::getDataPath() + "/drumkits/" + systemList[i];
		Drumkit *info = Drumkit::load( absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			loadTabDrumkitListBox->addItem( (info->getName()).c_str() );
			exportTab_drumkitList->addItem( (info->getName()).c_str() );
		}
	}

	loadTabDrumkitListBox->sortItems();
	/// \todo sort in exportTab_drumkitList
//	exportTab_drumkitList->sort();

	loadTabDrumkitListBox->setCurrentRow( 0 );
	exportTab_drumkitList->setCurrentIndex( 0 );
}



void OldDrumkitManager::okBtnClicked()
{
	hide();
}



void OldDrumkitManager::on_loadTabDrumkitListBox_currentRowChanged(int row)
{
//	INFOLOG( "[loadTab_drumkitListBox_currentRowChanged]" );
	if (row == -1) return;

	QString sSelectedDrumkitName = loadTabDrumkitListBox->currentItem()->text();
	// find the drumkit in the list
	for ( uint i = 0; i < drumkitInfoList.size(); i++ ) {
		Drumkit *drumkitInfo = drumkitInfoList[i];
		if ( QString( drumkitInfo->getName().c_str() ) == sSelectedDrumkitName ) {
			loadTab_drumkitNameLbl->setText( trUtf8( "Name: <b>%1</b>").arg( drumkitInfo->getName().c_str() ) );
			loadTab_drumkitAuthorLbl->setText( trUtf8( "Author: %1" ).arg( drumkitInfo->getAuthor().c_str() ) );
			loadTab_drumkitInfoLbl->setText( trUtf8( "Info: <br>%1").arg( drumkitInfo->getInfo().c_str() ) );

			loadTab_loadDrumkitBtn->setEnabled( true );

			// disabled!!
//			loadTab_deleteDrumkitBtn->setEnabled( true );
			break;
		}
	}
}



void OldDrumkitManager::on_loadTab_loadDrumkitBtn_clicked()
{
	QString sSelectedDrumkitName = loadTabDrumkitListBox->currentItem()->text();
	// find the drumkit in the list
	for ( uint i = 0; i < drumkitInfoList.size(); i++ ) {
		Drumkit *drumkitInfo = drumkitInfoList[i];
		if ( QString( drumkitInfo->getName().c_str() ) == sSelectedDrumkitName ) {
			setCursor( QCursor( Qt::WaitCursor ) );

			try {
				Hydrogen::getInstance()->loadDrumkit( drumkitInfo );
				Hydrogen::getInstance()->getSong()->m_bIsModified = true;
				HydrogenApp::getInstance()->setStatusBarMessage( trUtf8( "Drumkit loaded: [%1]" ).arg( drumkitInfo->getName().c_str() ), 2000 );

				setCursor( QCursor( Qt::ArrowCursor ) );

				// update drumkit info in save tab
				saveTab_nameTxt ->setText( QString( drumkitInfo->getName().c_str() ) );
				saveTab_authorTxt->setText( QString( drumkitInfo->getAuthor().c_str() ) );
				saveTab_infoTxt->append( QString( drumkitInfo->getInfo().c_str() ) );
			}
			catch ( H2Exception ex ) {
				setCursor( QCursor( Qt::ArrowCursor ) );
				QMessageBox::warning( this, "Hydrogen", QString( "An error occurred loading the Drumkit") );
			}
			break;
		}
	}
}



void OldDrumkitManager::on_importTab_browseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();

	QFileDialog *fd = new QFileDialog(this);
	fd->setFileMode(QFileDialog::ExistingFile);
	fd->setFilter( "Hydrogen drumkit (*.h2drumkit)" );
	fd->setDirectory( lastUsedDir );

	fd->setWindowTitle( trUtf8( "Import drumkit" ) );
//	setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}

	if (filename != "") {
		importTab_drumkitPathTxt->setText( filename );
		lastUsedDir = fd->directory().absolutePath();
	}
}



void OldDrumkitManager::on_importTab_importBtn_clicked()
{
	setCursor( QCursor( Qt::WaitCursor ) );

	string dataDir = Preferences::getInstance()->getDataDirectory();
	LocalFileMng fileMng;
	try {
		H2Core::Drumkit::install( string( importTab_drumkitPathTxt->text().toStdString() ) );
		QMessageBox::information( this, "Hydrogen", "Drumkit imported in " + QString( dataDir.c_str() )  );
		updateDrumkitList();
		setCursor( QCursor( Qt::ArrowCursor ) );
	}
	catch( H2Exception ex ) {
		setCursor( QCursor( Qt::ArrowCursor ) );
		QMessageBox::warning( this, "Hydrogen", "An error occurred importing the Drumkit."  );
	}
}



/// \todo da rifare saveTab_saveBtnClicked
void OldDrumkitManager::on_saveTab_saveBtn_clicked()
{
	setCursor( QCursor( Qt::WaitCursor ) );

	H2Core::Drumkit::save( 
			saveTab_nameTxt->text().toStdString(),
			saveTab_authorTxt->text().toStdString(),
			saveTab_infoTxt->toPlainText().toStdString()
	);
	updateDrumkitList();
	setCursor( QCursor( Qt::ArrowCursor ) );
	QMessageBox::information( this, "Hydrogen", "Drumkit saved." );
}



void OldDrumkitManager::on_exportTab_browseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();

	QFileDialog *fd = new QFileDialog(this);
	fd->setFileMode(QFileDialog::Directory);
	fd->setFilter( "Hydrogen drumkit (*.h2drumkit)" );
	fd->setDirectory( lastUsedDir );
	fd->setAcceptMode( QFileDialog::AcceptSave );
	fd->setWindowTitle( trUtf8( "Export drumkit" ) );

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}

	if (filename != "") {
		exportTab_drumkitPathTxt->setText( filename );
		lastUsedDir = fd->directory().absolutePath();
	}
	INFOLOG( "Filename: " + filename.toStdString() );
}



void OldDrumkitManager::on_exportTab_exportBtn_clicked()
{
	setCursor( QCursor( Qt::WaitCursor ) );

	string drumkitName = exportTab_drumkitList->currentText().toStdString();

	LocalFileMng fileMng;
	string drumkitDir = fileMng.getDrumkitDirectory( drumkitName );

	string saveDir = exportTab_drumkitPathTxt->text().toStdString();
	string cmd = string( "cd " ) + drumkitDir + string( "; tar czf \"" ) + saveDir + "/" + drumkitName + string( ".h2drumkit\" \"" ) + drumkitName + "\"";

	INFOLOG( "cmd: " + cmd );
	system( cmd.c_str() );

	setCursor( QCursor( Qt::ArrowCursor ) );
	QMessageBox::information( this, "Hydrogen", "Drumkit exported." );
}



void OldDrumkitManager::on_exportTab_drumkitPathTxt_textChanged( QString str )
{
	UNUSED( str );
	string path = exportTab_drumkitPathTxt->text().toStdString();
	if (path == "") {
		exportTab_exportBtn->setEnabled( false );
	}
	else {
		exportTab_exportBtn->setEnabled( true );
	}
}


void OldDrumkitManager::on_importTab_drumkitPathTxt_textChanged(QString str)
{
	UNUSED( str );
	string path = importTab_drumkitPathTxt->text().toStdString();
	if (path == "") {
		importTab_importBtn->setEnabled( false );
	}
	else {
		importTab_importBtn->setEnabled( true );
	}
}


void OldDrumkitManager::on_saveTab_nameTxt_textChanged(QString str)
{
	UNUSED( str );
	string name = saveTab_nameTxt->text().toStdString();
	if (name == "") {
		saveTab_saveBtn->setEnabled( false );
	}
	else {
		saveTab_saveBtn->setEnabled( true );
	}
}



void OldDrumkitManager::on_loadTab_deleteDrumkitBtn_clicked()
{
	QMessageBox::information( this, "Hydrogen", "Not implemented yet" );

	// verificare che nessun suono del drumkit sia utilizzato correntemente
}
