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

#include "SoundLibraryDatastructures.h"
#include "SoundLibraryImportDialog.h"
#include "SoundLibraryRepositoryDialog.h"
#include "SoundLibraryPanel.h"

#include "../widgets/DownloadWidget.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"

#include <hydrogen/LocalFileMng.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/helpers/filesystem.h>


#include <QTreeWidget>
#include <QDomDocument>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QCryptographicHash>

#include <memory>

const char* SoundLibraryImportDialog::__class_name = "SoundLibraryImportDialog";

SoundLibraryImportDialog::SoundLibraryImportDialog( QWidget* pParent )
 : QDialog( pParent )
 , Object( __class_name )
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
	connect( repositoryCombo, SIGNAL(currentIndexChanged(int)), this, SLOT( onRepositoryComboBoxIndexChanged(int) ));

	SoundLibraryNameLbl->setText( "" );
	SoundLibraryInfoLbl->setText( "" );
	DownloadBtn->setEnabled( false );

	InstallBtn->setEnabled (false );

	updateRepositoryCombo();
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
	reloadRepositoryData();
}

void SoundLibraryImportDialog::onRepositoryComboBoxIndexChanged(int i)
{
	UNUSED(i);
	QString cacheFile = getCachedFilename();
	if( !H2Core::Filesystem::file_exists( cacheFile ) )
	{
		SoundLibraryImportDialog::on_UpdateListBtn_clicked();
	}
	reloadRepositoryData();
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

QString SoundLibraryImportDialog::getCachedFilename()
{
	QString cacheDir = H2Core::Filesystem::repositories_cache_dir();
	QString serverMd5 = QString(QCryptographicHash::hash(( repositoryCombo->currentText().toLatin1() ),QCryptographicHash::Md5).toHex());
	QString cacheFile = cacheDir + "/" + serverMd5;
	return cacheFile;
}

void SoundLibraryImportDialog::writeCachedData(const QString& fileName, const QString& data)
{
	if( data.isEmpty() )
	{
		return;
	}

	QFile outFile( fileName );
	if( !outFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		ERRORLOG( "Failed to open file for writing repository cache." );
		return;
	}

	QTextStream stream( &outFile );
	stream << data;

	outFile.close();
}

QString SoundLibraryImportDialog::readCachedData(const QString& fileName)
{
	QString content;
	QFile inFile( fileName );
	if( !inFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
	{
		ERRORLOG( "Failed to open file for reading." );
		return content;
	}

	QDomDocument document;
	if( !document.setContent( &inFile ) )
	{
		inFile.close();
		return content;
	}
	inFile.close();

	content = document.toString();

	return content;
}

void SoundLibraryImportDialog::reloadRepositoryData()
{
	QString sDrumkitXML;
	QString cacheFile = getCachedFilename();

	if(H2Core::Filesystem::file_exists(cacheFile,true))
	{
		sDrumkitXML = readCachedData(cacheFile);
	}

	m_soundLibraryList.clear();
	QDomDocument dom;
	dom.setContent( sDrumkitXML );
	QDomNode drumkitNode = dom.documentElement().firstChild();
	while ( !drumkitNode.isNull() ) {
		if( !drumkitNode.toElement().isNull() ) {

			if ( drumkitNode.toElement().tagName() == "drumkit" || drumkitNode.toElement().tagName() == "song" || drumkitNode.toElement().tagName() == "pattern" ) {

				SoundLibraryInfo soundLibInfo;

				if ( drumkitNode.toElement().tagName() =="song" ) {
					soundLibInfo.setType( "song" );
				}

				if ( drumkitNode.toElement().tagName() =="drumkit" ) {
					soundLibInfo.setType( "drumkit" );
				}

				if ( drumkitNode.toElement().tagName() =="pattern" ) {
					soundLibInfo.setType( "pattern" );
				}

				QDomElement nameNode = drumkitNode.firstChildElement( "name" );
				if ( !nameNode.isNull() ) {
					soundLibInfo.setName( nameNode.text() );
				}

				QDomElement urlNode = drumkitNode.firstChildElement( "url" );
				if ( !urlNode.isNull() ) {
					soundLibInfo.setUrl( urlNode.text() );
				}

				QDomElement infoNode = drumkitNode.firstChildElement( "info" );
				if ( !infoNode.isNull() ) {
					soundLibInfo.setInfo( infoNode.text() );
				}

				QDomElement authorNode = drumkitNode.firstChildElement( "author" );
				if ( !authorNode.isNull() ) {
					soundLibInfo.setAuthor( authorNode.text() );
				}

				QDomElement licenseNode = drumkitNode.firstChildElement( "license" );
				if ( !licenseNode.isNull() ) {
					soundLibInfo.setLicense( licenseNode.text() );
				}

				m_soundLibraryList.push_back( soundLibInfo );
			}
		}
		drumkitNode = drumkitNode.nextSibling();
	}

	updateSoundLibraryList();

}

///
/// Download and update the drumkit list
///
void SoundLibraryImportDialog::on_UpdateListBtn_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	DownloadWidget drumkitList( this, trUtf8( "Updating SoundLibrary list..." ), repositoryCombo->currentText() );
	drumkitList.exec();

	QString sDrumkitXML = drumkitList.get_xml_content();


	/*
	 * Hydrogen creates the following cache hierarchy to cache
	 * the content of server lists:
	 *
	 * CACHE_DIR
	 *     +-----repositories
	 *           +-----serverlist_$(md5(SERVER_NAME))
	 */


	QString cacheFile = getCachedFilename();


	writeCachedData(cacheFile, sDrumkitXML);

	reloadRepositoryData();
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
		QString sLibraryName = m_soundLibraryList[ i ].getName();

		QTreeWidgetItem* pDrumkitItem = NULL;

		if ( m_soundLibraryList[ i ].getType() == "song" ) {
			pDrumkitItem = new QTreeWidgetItem(  m_pSongItem );
		}

		if ( m_soundLibraryList[ i ].getType() == "drumkit" ) {
			pDrumkitItem = new QTreeWidgetItem(  m_pDrumkitsItem );
		}

		if ( m_soundLibraryList[ i ].getType() == "pattern" ) {
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

	QString sName = QFileInfo( sInfo.getUrl() ).fileName();
	sName = sName.left( sName.lastIndexOf( "." ) );

	if ( sInfo.getType() == "drumkit" ) {
		if ( H2Core::Filesystem::drumkit_exists(sName) )
			return true;
	}

	if ( sInfo.getType() == "pattern" ) {
		return SoundLibraryDatabase::get_instance()->isPatternInstalled( sInfo.getName() );
	}

	if ( sInfo.getType() == "song" ) {
		if ( H2Core::Filesystem::song_exists(sName) )
			return true;
	}

	return false;
}




void SoundLibraryImportDialog::soundLibraryItemChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous  )
{
	UNUSED( previous );
	if ( current ) {

		QString selected = current->text(0);
		for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
			if ( m_soundLibraryList[ i ].getName() == selected ) {
				SoundLibraryInfo info = m_soundLibraryList[ i ];

				//bool alreadyInstalled = isSoundLibraryAlreadyInstalled( info.m_sURL );

				SoundLibraryNameLbl->setText( info.getName() );
				
				if( info.getType() == "pattern" ){
					SoundLibraryInfoLbl->setText("");
				} else {
					SoundLibraryInfoLbl->setText( info.getInfo() );
				}

				AuthorLbl->setText( trUtf8( "Author: %1" ).arg( info.getAuthor() ) );

				LicenseLbl->setText( trUtf8( "License: %1" ).arg( info.getLicense()) );


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
		if ( m_soundLibraryList[ i ].getName() == selected ) {
			// Download the sound library
			QString sURL = m_soundLibraryList[ i ].getUrl();
			QString sType = m_soundLibraryList[ i ].getType();
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
			SoundLibraryDatabase::get_instance()->update();
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
		SoundLibraryDatabase::get_instance()->update();
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
