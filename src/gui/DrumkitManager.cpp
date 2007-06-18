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
 * $Id: DrumkitManager.cpp,v 1.16 2005/05/09 18:10:53 comix Exp $
 *
 */
#include <qcursor.h>

#include "config.h"
#include "Skin.h"
#include "DrumkitManager.h"
#include "HydrogenApp.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "PatternEditor/PatternEditor.h"
#include "lib/Hydrogen.h"
#include "lib/LocalFileMng.h"
#include "lib/Preferences.h"
#include "lib/ADSR.h"
#include "lib/Sample.h"


DrumkitManager::DrumkitManager( QWidget* parent )
 : DrumkitManager_UI( parent )
 , Object("DrumkitManager")
{
	infoLog( "INIT" );
	setMinimumSize( width(), height() );	// not resizable
	setMaximumSize( width(), height() );	// not resizable

	setCaption( trUtf8( "Drumkit manager" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	// Load drumkit tab
	loadTab_loadDrumkitBtn->setEnabled( false );
	// disabled until is ready
	loadTab_deleteDrumkitBtn->setEnabled( false );

	// save tab
	saveTab_saveBtn->setEnabled( false );

	// import tab
	Preferences *pref = Preferences::getInstance();
	importTab_infoLbl->setText( trUtf8( "The drumkit will be installed in %1/.hydrogen/data/" ).arg( QDir::homeDirPath() ) );
	importTab_importBtn->setEnabled( false );

	updateDrumkitList();
}



DrumkitManager::~DrumkitManager()
{
	infoLog( "DESTROY" );

	for (uint i = 0; i < drumkitInfoList.size(); i++ ) {
		DrumkitInfo* info = drumkitInfoList[i];
		delete info;
	}
	drumkitInfoList.clear();
}



void DrumkitManager::updateDrumkitList()
{
	infoLog( "[updateDrumkitList]" );

	loadTab_drumkitListBox->clear();
	exportTab_drumkitList->clear();

	for (uint i = 0; i < drumkitInfoList.size(); i++ ) {
		DrumkitInfo* info = drumkitInfoList[i];
		delete info;
	}
	drumkitInfoList.clear();

	Preferences *pref = Preferences::getInstance();
	LocalFileMng mng;
	vector<string> userList = mng.listUserDrumkits();
	for (uint i = 0; i < userList.size(); i++) {
		string absPath = QDir::homeDirPath().append("/.hydrogen/data/").ascii() + userList[i];
		DrumkitInfo *info = mng.loadDrumkit( absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			loadTab_drumkitListBox->insertItem( (info->getName()).c_str() );
			exportTab_drumkitList->insertItem( (info->getName()).c_str() );
		}
	}


	vector<string> systemList = mng.listSystemDrumkits();
	for (uint i = 0; i < systemList.size(); i++) {
		string absPath = DataPath::getDataPath() + "/drumkits/" + systemList[i];
		DrumkitInfo *info = mng.loadDrumkit( absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			loadTab_drumkitListBox->insertItem( (info->getName()).c_str() );
			exportTab_drumkitList->insertItem( (info->getName()).c_str() );
		}
	}

	loadTab_drumkitListBox->sort();
	/// \todo sort in exportTab_drumkitList
//	exportTab_drumkitList->sort();

	loadTab_drumkitListBox->setSelected( 0, true );
	exportTab_drumkitList->setCurrentItem( 0 );
}



void DrumkitManager::okBtnClicked()
{
	hide();
}



void DrumkitManager::loadTab_selectionChanged()
{
	QString sSelectedDrumkitName = loadTab_drumkitListBox->currentText();
	// find the drumkit in the list
	for ( uint i = 0; i < drumkitInfoList.size(); i++ ) {
		DrumkitInfo *drumkitInfo = drumkitInfoList[i];
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



void DrumkitManager::loadTab_loadDrumkitBtnClicked()
{
	QString sSelectedDrumkitName = loadTab_drumkitListBox->currentText();
	// find the drumkit in the list
	for ( uint i = 0; i < drumkitInfoList.size(); i++ ) {
		DrumkitInfo *drumkitInfo = drumkitInfoList[i];
		if ( QString( drumkitInfo->getName().c_str() ) == sSelectedDrumkitName ) {
			setCursor( QCursor( Qt::WaitCursor ) );

			( Hydrogen::getInstance() )->loadDrumkit( drumkitInfo );
			( Hydrogen::getInstance() )->getSong()->m_bIsModified = true;
			( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Drumkit loaded: [%1]" ).arg( drumkitInfo->getName().c_str() ), 2000 );

			setCursor( QCursor( Qt::ArrowCursor ) );

			// update drumkit info in save tab
			saveTab_nameTxt ->setText( QString( drumkitInfo->getName().c_str() ) );
			saveTab_authorTxt->setText( QString( drumkitInfo->getAuthor().c_str() ) );
			saveTab_infoTxt->setText( QString( drumkitInfo->getInfo().c_str() ) );

			HydrogenApp::getInstance()->getPatternEditorPanel()->getInstrumentList()->updateEditor();
			HydrogenApp::getInstance()->getPatternEditorPanel()->getPatternEditor()->updateEditor( true );

//			QMessageBox::information( this, "Hydrogen", QString( "Drumkit loaded: %1").arg( QString( drumkitInfo->getName().c_str() ) ) );
			break;
		}
	}
}



void DrumkitManager::importTab_browseBtnClicked() {
	static QString lastUsedDir = "";

	QFileDialog *fd = new QFileDialog(this, "File Dialog", TRUE);
	fd->setMode(QFileDialog::ExistingFile);
	fd->setFilter( "Hydrogen drumkit (*.h2drumkit)" );
	fd->setDir( lastUsedDir );

	fd->setCaption( trUtf8( "Import drumkit" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFile();
	}

	if (filename != "") {
		importTab_drumkitPathTxt->setText( filename );
		lastUsedDir = fd->dirPath();
	}
}



void DrumkitManager::importTab_importBtnClicked() {
	setCursor( QCursor( Qt::WaitCursor ) );

	string dataDir = QDir::homeDirPath().append("/.hydrogen/data/").ascii();
	LocalFileMng fileMng;
	fileMng.installDrumkit( string( importTab_drumkitPathTxt->text().latin1() ) );

	QMessageBox::information( this, "Hydrogen", "Drumkit imported in " + QString( dataDir.c_str() )  );

	updateDrumkitList();
	setCursor( QCursor( Qt::ArrowCursor ) );
}



/// \todo da rifare saveTab_saveBtnClicked
void DrumkitManager::saveTab_saveBtnClicked()
{
	setCursor( QCursor( Qt::WaitCursor ) );

	DrumkitInfo *pDrumkitInfo = new DrumkitInfo();
	pDrumkitInfo->setName( ( saveTab_nameTxt->text() ).latin1() );
	pDrumkitInfo->setAuthor( ( saveTab_authorTxt->text() ).latin1() );
	pDrumkitInfo->setInfo( ( saveTab_infoTxt->text() ).latin1() );

	LocalFileMng fileMng;
	Song *pSong = Hydrogen::getInstance()->getSong();
	InstrumentList *pSongInstrList = pSong->getInstrumentList();
	InstrumentList *pInstrumentList = new InstrumentList();

	for ( uint nInstrument = 0; nInstrument < pSongInstrList->getSize(); nInstrument++ ) {
		Instrument *pOldInstr = pSongInstrList->get( nInstrument );
		Instrument *pNewInstr = new Instrument();
		pNewInstr->m_sId = pOldInstr->m_sId;
		pNewInstr->m_fVolume = pOldInstr->m_fVolume;
		pNewInstr->m_sName = pOldInstr->m_sName;
		pNewInstr->m_fPan_L = pOldInstr->m_fPan_L;
		pNewInstr->m_fPan_R = pOldInstr->m_fPan_R;
		pNewInstr->m_bIsMuted = pOldInstr->m_bIsMuted;
		pNewInstr->m_bIsLocked = false; // saved instruments are unlocked
		pNewInstr->m_fRandomPitchFactor = pOldInstr->m_fRandomPitchFactor;

		pNewInstr->m_pADSR = new ADSR( *(pOldInstr->m_pADSR) );

		pNewInstr->m_bFilterActive = pOldInstr->m_bFilterActive;
		pNewInstr->m_fCutoff = pOldInstr->m_fCutoff;
		pNewInstr->m_fResonance = pOldInstr->m_fResonance;

		pNewInstr->m_excludeVectId = pOldInstr->m_excludeVectId;

		string sInstrDrumkit = pOldInstr->m_sDrumkitName;

		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer *pOldLayer = pOldInstr->getLayer( nLayer );
			if ( pOldLayer ) {
				Sample *pSample = pOldLayer->m_pSample;

				Sample *pNewSample = new Sample( 0, pSample->m_sFilename );	// is not a real sample, it contains only the filename information
				InstrumentLayer *pLayer = new InstrumentLayer( pNewSample );
				pLayer->m_fGain = pOldLayer->m_fGain;
				pLayer->m_fPitch = pOldLayer->m_fPitch;
				pLayer->m_fStartVelocity = pOldLayer->m_fStartVelocity;
				pLayer->m_fEndVelocity = pOldLayer->m_fEndVelocity;

				pNewInstr->setLayer( pLayer, nLayer );
			}
			else {
				pNewInstr->setLayer( NULL, nLayer );
			}
		}
		pInstrumentList->add ( pNewInstr );
	}

	pDrumkitInfo->setInstrumentList( pInstrumentList );

	int err = fileMng.saveDrumkit( pDrumkitInfo );
	if (err != 0) {
		QMessageBox::information( this, "Hydrogen", "Error saving drumkit" );
	}

	// delete the drumkit info
	delete pDrumkitInfo;
	pDrumkitInfo = NULL;

	updateDrumkitList();
	setCursor( QCursor( Qt::ArrowCursor ) );
	QMessageBox::information( this, "Hydrogen", "Drumkit saved." );
}



void DrumkitManager::tabChanged() {
/*
	QWidget *page = tabWidget->currentPage();
	string pageStr = ( tabWidget->tabLabel( page ) ).latin1();
*/
}



void DrumkitManager::exportTab_browseBtnClicked() {
	static QString lastUsedDir = "";

	QFileDialog *fd = new QFileDialog(this, "File Dialog", TRUE);
//	fd->setMode(QFileDialog::DirectoryOnly);
	fd->setMode(QFileDialog::Directory);
	fd->setFilter( "Hydrogen drumkit (*.h2drumkit)" );
	fd->setDir( lastUsedDir );

	fd->setCaption( trUtf8( "Export drumkit" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFile();
	}

	if (filename != "") {
		exportTab_drumkitPathTxt->setText( filename );
		lastUsedDir = fd->dirPath();
	}


}



void DrumkitManager::exportTab_exportBtnClicked() {
	setCursor( QCursor( Qt::WaitCursor ) );

	string drumkitName = exportTab_drumkitList->currentText().latin1();

	LocalFileMng fileMng;
	string drumkitDir = fileMng.getDrumkitDirectory( drumkitName );

	string saveDir = exportTab_drumkitPathTxt->text().latin1();
	string cmd = string( "cd " ) + drumkitDir + string( "; tar czf " ) + saveDir + drumkitName + string( ".h2drumkit " ) + drumkitName;

	system( cmd.c_str() );

	setCursor( QCursor( Qt::ArrowCursor ) );
	QMessageBox::information( this, "Hydrogen", "Drumkit exported." );
}



void DrumkitManager::exportTab_drumkitPathChanged() {
	string path = exportTab_drumkitPathTxt->text().latin1();
	if (path == "") {
		exportTab_exportBtn->setEnabled( false );
	}
	else {
		exportTab_exportBtn->setEnabled( true );
	}
}



void DrumkitManager::importTab_drumkitPathChanged() {
	string path = importTab_drumkitPathTxt->text().latin1();
	if (path == "") {
		importTab_importBtn->setEnabled( false );
	}
	else {
		importTab_importBtn->setEnabled( true );
	}
}



void DrumkitManager::saveTab_nameChanged() {
	string name = saveTab_nameTxt->text().latin1();
	if (name == "") {
		saveTab_saveBtn->setEnabled( false );
	}
	else {
		saveTab_saveBtn->setEnabled( true );
	}
}



void DrumkitManager::loadTab_deleteDrumkitBtnClicked() {
	QMessageBox::information( this, "Hydrogen", "Not implemented yet" );

	// verificare che nessun suono del drumkit sia utilizzato correntemente
}


