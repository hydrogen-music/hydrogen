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
#include "SoundLibraryRepositoryDialog.h"
#include "SoundLibraryPanel.h"

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

#include <memory>

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


	SoundLibraryNameLbl->setText( "" );
	SoundLibraryInfoLbl->setText( "" );
	DownloadBtn->setEnabled( false );

	InstallBtn->setEnabled (false );

	updateRepositoryCombo();

	// force a new update
	//on_UpdateListBtn_clicked();
}




SoundLibraryImportDialog::~SoundLibraryImportDialog()
{
	INFOLOG( "DESTROY" );

}

//update combo box
void SoundLibraryImportDialog::updateRepositoryCombo()
{
	H2Core::Preferences* pref = H2Core::Preferences::get_instance();

	/*
		Read serverList from config and put servers into the comboBox
	*/
	
	if( pref->sServerList.size() == 0 ) {
		pref->sServerList.push_back( "http://www.hydrogen-music.org/feeds/drumkit_list.php" );
	}

	repositoryCombo->clear();

	std::list<QString>::const_iterator cur_Server;
	for( cur_Server = pref->sServerList.begin(); cur_Server != pref->sServerList.end(); ++cur_Server ) {
		repositoryCombo->insertItem( 0, *cur_Server );
	}
}

///
/// Edit the server list
///
void SoundLibraryImportDialog::on_EditListBtn_clicked()
{
	SoundLibraryRepositoryDialog repoDialog( this );
	repoDialog.exec();
	updateRepositoryCombo();
}


///
/// Download and update the drumkit list
///
void SoundLibraryImportDialog::on_UpdateListBtn_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	DownloadWidget drumkitList( this, trUtf8( "Updating SoundLibrary list..." ), repositoryCombo->currentText() );
	drumkitList.exec();

	m_soundLibraryList.clear();

	QString sDrumkitXML = drumkitList.get_xml_content();

	QDomDocument dom;
	dom.setContent( sDrumkitXML );
	QDomNode drumkitNode = dom.documentElement().firstChild();
	while ( !drumkitNode.isNull() ) {
		if( !drumkitNode.toElement().isNull() ) {

			if ( drumkitNode.toElement().tagName() == "drumkit" || drumkitNode.toElement().tagName() == "song" || drumkitNode.toElement().tagName() == "pattern" ) {

				SoundLibraryInfo soundLibInfo;
			
				if ( drumkitNode.toElement().tagName() =="song" ) {
					soundLibInfo.m_sType = "song";
				}

				if ( drumkitNode.toElement().tagName() =="drumkit" ) {
					soundLibInfo.m_sType = "drumkit";
				}

				if ( drumkitNode.toElement().tagName() =="pattern" ) {
					soundLibInfo.m_sType = "pattern";
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

				QDomElement licenseNode = drumkitNode.firstChildElement( "license" );
				if ( !licenseNode.isNull() ) {
					soundLibInfo.m_sLicense = licenseNode.text();
				}

				m_soundLibraryList.push_back( soundLibInfo );
			}
		}
		drumkitNode = drumkitNode.nextSibling();
	}

	updateSoundLibraryList();
	QApplication::restoreOverrideCursor();
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

	m_pPatternItem = new QTreeWidgetItem( m_pDrumkitTree );
	m_pPatternItem->setText( 0, trUtf8( "Patterns" ) );
	m_pDrumkitTree->setItemExpanded( m_pPatternItem, true );

	for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
		QString sLibraryName = m_soundLibraryList[ i ].m_sName;

		QTreeWidgetItem* pDrumkitItem = NULL;

		if ( m_soundLibraryList[ i ].m_sType == "song" ) {
			pDrumkitItem = new QTreeWidgetItem(  m_pSongItem );
		}

		if ( m_soundLibraryList[ i ].m_sType == "drumkit" ) {
			pDrumkitItem = new QTreeWidgetItem(  m_pDrumkitsItem );
		}

		if ( m_soundLibraryList[ i ].m_sType == "pattern" ) {
			pDrumkitItem = new QTreeWidgetItem(  m_pPatternItem );
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

	QString soundLibraryItemName = QFileInfo( sInfo.m_sURL ).fileName();
	soundLibraryItemName = soundLibraryItemName.left( soundLibraryItemName.lastIndexOf( "." ) );

	if ( sInfo.m_sType == "drumkit" ) {
		std::vector<QString> systemList = H2Core::Drumkit::getSystemDrumkitList();
		for ( uint i = 0; i < systemList.size(); ++i ) {
			if ( systemList[ i ].endsWith(soundLibraryItemName) ) {
				return true;
			}
		}

		std::vector<QString> userList = H2Core::Drumkit::getUserDrumkitList();
		for ( uint i = 0; i < userList.size(); ++i ) {
			if ( userList[ i ].endsWith(soundLibraryItemName) ) {
				return true;
			}
		}
	}

	if ( sInfo.m_sType == "pattern" ) {
		H2Core::LocalFileMng mng;
		std::vector<QString> patternList = mng.getAllPatternName();
		for ( uint i = 0; i < patternList.size(); ++i ) {
			if ( patternList[ i ] == soundLibraryItemName ) {
				return true;
			}
		}
	}

	if ( sInfo.m_sType == "song" ) {
		H2Core::LocalFileMng mng;
		std::vector<QString> songList = mng.getSongList();
		for ( uint i = 0; i < songList.size(); ++i ) {
			if ( songList[ i ] == soundLibraryItemName ) {
				return true;
			}
		}
	}

	return false;
}




void SoundLibraryImportDialog::soundLibraryItemChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous  )
{
	UNUSED( previous );
	if ( current ) {

		QString selected = current->text(0);
		for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
			if ( m_soundLibraryList[ i ].m_sName == selected ) {
				SoundLibraryInfo info = m_soundLibraryList[ i ];

				//bool alreadyInstalled = isSoundLibraryAlreadyInstalled( info.m_sURL );

				SoundLibraryNameLbl->setText( info.m_sName );
				
				if( info.m_sType == "pattern" ){
					SoundLibraryInfoLbl->setText("");
				} else {
					SoundLibraryInfoLbl->setText( info.m_sInfo );
				}

				AuthorLbl->setText( trUtf8( "Author: %1" ).arg( info.m_sAuthor ) );

				LicenseLbl->setText( trUtf8( "License: %1" ).arg( info.m_sLicense ) );


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
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QString selected = m_pDrumkitTree->currentItem()->text(0);

	for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
		if ( m_soundLibraryList[ i ].m_sName == selected ) {
			// Download the sound library
			QString sURL = m_soundLibraryList[ i ].m_sURL;
			QString sType = m_soundLibraryList[ i ].m_sType;
			QString sLocalFile;

			QString dataDir = H2Core::Preferences::get_instance()->getDataDirectory();

			if( sType == "drumkit") {
				sLocalFile = QDir::tempPath() + "/" + QFileInfo( sURL ).fileName();
			}

			if( sType == "song") {
				sLocalFile = dataDir + "songs/" + QFileInfo( sURL ).fileName();
			}

			if( sType == "pattern") {
				sLocalFile = dataDir + "patterns/" + QFileInfo( sURL ).fileName();
			}

			for ( int i = 0; i < 30; ++i ) {
				DownloadWidget dl( this, trUtf8( "Downloading SoundLibrary..." ), sURL, sLocalFile );
				dl.exec();

				QString redirect_url = dl.get_redirect_url();
				if (redirect_url == "" ) {
					// ok, we have all data
					break;
				}
				else {
					sURL = redirect_url;
				}
			}


			// install the new soundlibrary

			try {
				if ( sType == "drumkit" ) {
					H2Core::Drumkit::install( sLocalFile );
					QApplication::restoreOverrideCursor();
					QMessageBox::information( this, "Hydrogen", QString( trUtf8( "SoundLibrary imported in %1" ) ).arg( dataDir ) );
				}

				if ( sType == "song" || sType == "pattern") {
					QApplication::restoreOverrideCursor();
				}
			}
			catch( H2Core::H2Exception ex ) {
				QApplication::restoreOverrideCursor();
				QMessageBox::warning( this, "Hydrogen", trUtf8( "An error occurred importing the SoundLibrary."  ) );
			}

			QApplication::setOverrideCursor(Qt::WaitCursor);
			// remove the downloaded files..
			if( sType == "drumkit" ) {
				QDir dir;
				dir.remove( sLocalFile );
			}

			// update the drumkit list
			HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
			HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
			updateSoundLibraryList();
			QApplication::restoreOverrideCursor();
			return;
		}
	}
}




void SoundLibraryImportDialog::on_BrowseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();

	std::auto_ptr<QFileDialog> fd( new QFileDialog );
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
	QApplication::setOverrideCursor(Qt::WaitCursor);

	QString dataDir = H2Core::Preferences::get_instance()->getDataDirectory();
	try {
		H2Core::Drumkit::install( SoundLibraryPathTxt->text() );
		QMessageBox::information( this, "Hydrogen", QString( trUtf8( "SoundLibrary imported in %1" ).arg( dataDir )  ) );
		// update the drumkit list
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
		QApplication::restoreOverrideCursor();
	}
	catch( H2Core::H2Exception ex ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::warning( this, "Hydrogen", trUtf8( "An error occurred importing the SoundLibrary."  ) );
	}
}



void SoundLibraryImportDialog::on_close_btn_clicked()
{
	accept();
}



