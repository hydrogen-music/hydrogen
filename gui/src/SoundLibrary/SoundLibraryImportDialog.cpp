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

#include "SoundLibraryImportDialog.h"
#include "../widgets/DownloadWidget.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"

#include <hydrogen/LocalFileMng.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/SoundLibrary.h>
#include <hydrogen/Preferences.h>


#include <QTreeWidget>
#include <QDomDocument>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>



SoundLibraryImportDialog::SoundLibraryImportDialog( QWidget* pParent )
 : QDialog( pParent )
 , Object( "SoundLibraryImportDialog" )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( trUtf8( "Sound Library import" ) );
	setFixedSize( width(), height() );

	QStringList headers;
	headers << trUtf8( "Sound library" ) << trUtf8( "Status" );
	QTreeWidgetItem* header = new QTreeWidgetItem( headers );
	m_pDrumkitTree->setHeaderItem( header );
	m_pDrumkitTree->header()->resizeSection( 0, 200 );

	connect( m_pDrumkitTree, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem* ) ), this, SLOT( soundLibraryItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );

	H2Core::Preferences *pPref = H2Core::Preferences::getInstance();


	/*
		Read serverList from config and put servers into the comboBox
	*/
	std::list<std::string>::const_iterator cur_Server;
	
	if(pPref->sServerList.size() == 0){ 	
		pPref->sServerList.push_back("http://www.hydrogen-music.org/feeds/drumkit_list.php");
	}

	for( cur_Server = pPref->sServerList.begin(); cur_Server != pPref->sServerList.end(); ++cur_Server )
	{
		repositoryCombo->insertItem(0,QString(cur_Server->c_str()));
	}	

	
	SoundLibraryNameLbl->setText( "" );
	SoundLibraryInfoLbl->setText( "" );
	DownloadBtn->setEnabled( false );

	InstallBtn->setEnabled (false );



	// force a new update
	//on_UpdateListBtn_clicked();
}




SoundLibraryImportDialog::~SoundLibraryImportDialog()
{
	INFOLOG( "DESTROY" );

}




///
/// Download and update the drumkit list
///
void SoundLibraryImportDialog::on_UpdateListBtn_clicked()
{
	DownloadWidget drumkitList( this, trUtf8( "Updating SoundLibrary list..." ), repositoryCombo->currentText() );
	drumkitList.exec();

	m_soundLibraryList.clear();

	// analizzo l'xml scaricato
	QString sDrumkitXML = drumkitList.getXMLContent();

	QDomDocument dom;
	dom.setContent( sDrumkitXML );
	QDomNode drumkitNode = dom.documentElement().firstChild();
	while ( !drumkitNode.isNull() ) {
		if( !drumkitNode.toElement().isNull() ) {
			if ( drumkitNode.toElement().tagName() == "drumkit" || drumkitNode.toElement().tagName() == "song" ) {

				SoundLibraryInfo soundLibInfo;
			
				if ( drumkitNode.toElement().tagName() =="song" ) {
					soundLibInfo.m_sType = "song";
				}

				if ( drumkitNode.toElement().tagName() =="drumkit" ) {
					soundLibInfo.m_sType = "drumkit";
				}


				QDomElement nameNode = drumkitNode.firstChildElement( "name" );
				if ( !nameNode.isNull() ) {
					soundLibInfo.m_sName = nameNode.text();
				}

				QDomElement urlNode = drumkitNode.firstChildElement( "url" );
				if ( !urlNode.isNull() ) {
					soundLibInfo.m_sURL = urlNode.text();
				}

				QDomElement infoNode = drumkitNode.firstChildElement( "info" );
				if ( !infoNode.isNull() ) {
					soundLibInfo.m_sInfo = infoNode.text();
				}

				QDomElement authorNode = drumkitNode.firstChildElement( "author" );
				if ( !authorNode.isNull() ) {
					soundLibInfo.m_sAuthor = authorNode.text();
				}

				m_soundLibraryList.push_back( soundLibInfo );
			}
		}
		drumkitNode = drumkitNode.nextSibling();
	}

	updateSoundLibraryList();
}




void SoundLibraryImportDialog::updateSoundLibraryList()
{
	// build the sound library tree
	m_pDrumkitTree->clear();

	m_pDrumkitsItem = new QTreeWidgetItem( m_pDrumkitTree );
	m_pDrumkitsItem->setText( 0, trUtf8( "Drumkits" ) );
	m_pDrumkitTree->setItemExpanded( m_pDrumkitsItem, true );

	
	m_pSongItem = new QTreeWidgetItem( m_pDrumkitTree );
	m_pSongItem->setText( 0, trUtf8( "Songs" ) );
	m_pDrumkitTree->setItemExpanded( m_pSongItem, true );


	for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
		QString sLibraryName = m_soundLibraryList[ i ].m_sName;

		QTreeWidgetItem* pDrumkitItem;

		if ( m_soundLibraryList[ i ].m_sType == "song" ) {
			pDrumkitItem = new QTreeWidgetItem(  m_pSongItem );
		}

		
		if ( m_soundLibraryList[ i ].m_sType == "drumkit" ) {
			pDrumkitItem = new QTreeWidgetItem(  m_pDrumkitsItem );
		}

	

		if ( isSoundLibraryItemAlreadyInstalled( m_soundLibraryList[ i ]  ) ) {
			pDrumkitItem->setText( 0, sLibraryName );
			pDrumkitItem->setText( 1, trUtf8( "Installed" ) );
		}
		else {
			pDrumkitItem->setText( 0, sLibraryName );
			pDrumkitItem->setText( 1, trUtf8( "New" ) );
		}
	}
}




/// Is the SoundLibrary already installed?
bool SoundLibraryImportDialog::isSoundLibraryItemAlreadyInstalled( SoundLibraryInfo sInfo )
{
	// check if the filename matchs with an already installed soundlibrary directory.
	// The filename used in the Soundlibrary URL must be the same of the unpacked directory.
	// E.g: V-Synth_VariBreaks.h2drumkit must contain the V-Synth_VariBreaks directory once unpacked.
	// Many drumkit are broken now (wrong filenames) and MUST be fixed!

	std::string soundLibraryItemName = QFileInfo( sInfo.m_sURL ).fileName().toStdString();
	soundLibraryItemName = soundLibraryItemName.substr( 0, soundLibraryItemName.rfind( "." ) );

	if ( sInfo.m_sType == "drumkit" )
	{
		std::vector<std::string> systemList = H2Core::Drumkit::getSystemDrumkitList();
		for ( uint i = 0; i < systemList.size(); ++i ) {
			if ( systemList[ i ] == soundLibraryItemName ) {
				return true;
			}
		}

		std::vector<std::string> userList = H2Core::Drumkit::getUserDrumkitList();
		for ( uint i = 0; i < userList.size(); ++i ) {
			if ( userList[ i ] == soundLibraryItemName ) {
				return true;
			}
		}
	}

	if ( sInfo.m_sType == "song" )
	{
		H2Core::LocalFileMng mng;
		std::vector<std::string> songList = mng.getSongList();
		for ( uint i = 0; i < songList.size(); ++i ) {
			if ( songList[ i ] == soundLibraryItemName ) {
				return true;
			}
		}
	}

	return false;
}




void SoundLibraryImportDialog::soundLibraryItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous  )
{
	UNUSED( previous );
	if ( current ) {

		QString selected = current->text(0);
		for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
			if ( m_soundLibraryList[ i ].m_sName == selected ) {
				SoundLibraryInfo info = m_soundLibraryList[ i ];

				//bool alreadyInstalled = isSoundLibraryAlreadyInstalled( info.m_sURL );

				SoundLibraryNameLbl->setText( info.m_sName );
				SoundLibraryInfoLbl->setText( info.m_sInfo );
				AuthorLbl->setText( trUtf8( "Author: %1" ).arg( info.m_sAuthor ) );
				DownloadBtn->setEnabled( true );
				return;
			}
		}
	}

	SoundLibraryNameLbl->setText( "" );
	SoundLibraryInfoLbl->setText( "" );
	AuthorLbl->setText( "" );
	DownloadBtn->setEnabled( false );
}




void SoundLibraryImportDialog::on_DownloadBtn_clicked()
{
	QString selected = m_pDrumkitTree->currentItem()->text(0);

	for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
		if ( m_soundLibraryList[ i ].m_sName == selected ) {
			// Download the sound library
			QString sURL = m_soundLibraryList[ i ].m_sURL;
			QString sType = m_soundLibraryList[ i ].m_sType;
			QString sLocalFile;

			std::string dataDir = H2Core::Preferences::getInstance()->getDataDirectory();


			if( sType == "drumkit")
			{
				sLocalFile = QDir::tempPath () + "/" + QFileInfo( sURL ).fileName();
			}

			if( sType == "song")
			{
				sLocalFile = QString(dataDir.c_str()) + "songs/" + QFileInfo( sURL ).fileName();
			}

		

			DownloadWidget drumkit( this, trUtf8( "Downloading SoundLibrary..." ), sURL, sLocalFile );
			drumkit.exec();

			// install the new soundlibrary
			setCursor( QCursor( Qt::WaitCursor ) );

			
			try {
				if ( sType == "drumkit" )
				{
					H2Core::Drumkit::install( sLocalFile.toStdString() );
					QMessageBox::information( this, "Hydrogen", QString( trUtf8( "SoundLibrary imported in %1" ) ).arg( dataDir.c_str() ) );
					setCursor( QCursor( Qt::ArrowCursor ) );
				}

				
				if ( sType == "song" )
				{
					//H2Core::Drumkit::install( sLocalFile.toStdString() );
					//QMessageBox::information( this, "Hydrogen", QString( trUtf8( "SoundLibrary imported in %1" ) ).arg( dataDir.c_str() ) );
					setCursor( QCursor( Qt::ArrowCursor ) );
				}
				
					

			}
			catch( H2Core::H2Exception ex ) {
				setCursor( QCursor( Qt::ArrowCursor ) );
				QMessageBox::warning( this, "Hydrogen", trUtf8( "An error occurred importing the SoundLibrary."  ) );
			}

			// remove the downloaded files..
			if( sType == "drumkit" )
			{
				QDir dir;
				dir.remove( sLocalFile );
			}

			// update the drumkit list
			HydrogenApp::getInstance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
			updateSoundLibraryList();
			return;
		}
	}
}




void SoundLibraryImportDialog::on_BrowseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();

	QFileDialog *fd = new QFileDialog(this);
	fd->setFileMode(QFileDialog::ExistingFile);
	fd->setFilter( "Hydrogen drumkit (*.h2drumkit)" );
	fd->setDirectory( lastUsedDir );

	fd->setWindowTitle( trUtf8( "Import drumkit" ) );

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}

	if (filename != "") {
		SoundLibraryPathTxt->setText( filename );
		lastUsedDir = fd->directory().absolutePath();
		InstallBtn->setEnabled ( true );
	}
}




void SoundLibraryImportDialog::on_InstallBtn_clicked()
{
	setCursor( QCursor( Qt::WaitCursor ) );

	std::string dataDir = H2Core::Preferences::getInstance()->getDataDirectory();
	try {
		H2Core::Drumkit::install( SoundLibraryPathTxt->text().toStdString() );
		QMessageBox::information( this, "Hydrogen", QString( trUtf8( "SoundLibrary imported in %1" ).arg( dataDir.c_str() )  ) );
		// update the drumkit list
		HydrogenApp::getInstance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
		setCursor( QCursor( Qt::ArrowCursor ) );
	}
	catch( H2Core::H2Exception ex ) {
		setCursor( QCursor( Qt::ArrowCursor ) );
		QMessageBox::warning( this, "Hydrogen", trUtf8( "An error occurred importing the SoundLibrary."  ) );
	}
}



void SoundLibraryImportDialog::on_close_btn_clicked()
{
	accept();
}



