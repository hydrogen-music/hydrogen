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

#include "../Widgets/DownloadWidget.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"

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
const int max_redirects = 30;

SoundLibraryImportDialog::SoundLibraryImportDialog( QWidget* pParent, bool bOnlineImport )
 : QDialog( pParent )
 , Object( __class_name )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( tr( "Sound Library import" ) );
	setFixedSize( width(), height() );

	QStringList headers;
	headers << tr( "Sound library" ) << tr( "Status" );
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

	if( bOnlineImport){
		 tabWidget->setCurrentIndex( 0 );
	} else {
		 tabWidget->setCurrentIndex( 1 );
	}
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
		pref->sServerList.push_back( "http://hydrogen-music.org/feeds/drumkit_list.php" );
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

	if(!repositoryCombo->currentText().isEmpty())
	{
		QString cacheFile = getCachedFilename();
		if( !H2Core::Filesystem::file_exists( cacheFile, true ) )
		{
			SoundLibraryImportDialog::on_UpdateListBtn_clicked();
		}
		reloadRepositoryData();
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

void SoundLibraryImportDialog::clearImageCache()
{
	// Note: After a kit is installed the list refreshes and this gets called to
	// clear the image cache - maybe we want to keep the cache in this case?
	QString cacheDir = H2Core::Filesystem::repositories_cache_dir() ;
	INFOLOG("Deleting cached image files from " + cacheDir.toLocal8Bit() );

	QDir dir( cacheDir );
	dir.setNameFilters(QStringList() << "*.png");
	dir.setFilter(QDir::Files);
	foreach(QString dirFile, dir.entryList())
	{
		if ( !dir.remove(dirFile) )
		{
			WARNINGLOG("Error removing image file(s) from cache.");
		}
	}
}

QString SoundLibraryImportDialog::getCachedFilename()
{
	QString cacheDir = H2Core::Filesystem::repositories_cache_dir();
	QString serverMd5 = QString(QCryptographicHash::hash(( repositoryCombo->currentText().toLatin1() ),QCryptographicHash::Md5).toHex());
	QString cacheFile = cacheDir + "/" + serverMd5;
	return cacheFile;
}

QString SoundLibraryImportDialog::getCachedImageFilename()
{
	QString cacheDir = H2Core::Filesystem::repositories_cache_dir();
	QString kitNameMd5 = QString(QCryptographicHash::hash(( SoundLibraryNameLbl->text().toLatin1() ),QCryptographicHash::Md5).toHex());
	QString cacheFile = cacheDir + "/" + kitNameMd5 + ".png";	
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
		ERRORLOG( QString("Failed to open file for writing repository cache: %1").arg( fileName ) );
		return;
	}

	QTextStream stream( &outFile );
	stream << data;

	outFile.close();
}

void SoundLibraryImportDialog::writeCachedImage( const QString& imageFile, QPixmap& pixmap )
{
	QString cacheFile = getCachedImageFilename() ;

	QFile outFile( cacheFile );
	if( !outFile.open( QIODevice::WriteOnly ) )
	{
		ERRORLOG( QString("Failed to open file for writing repository image cache: %1").arg( imageFile ) );
		return;
	}
	
	pixmap.save(&outFile);

	outFile.close();
}

QString SoundLibraryImportDialog::readCachedData(const QString& fileName)
{
	QString content;
	QFile inFile( fileName );
	if( !inFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
	{
		ERRORLOG( QString("Failed to open file for reading: %1").arg( fileName ) );
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

QString SoundLibraryImportDialog::readCachedImage( const QString& imageFile )
{
	QString cacheFile = getCachedImageFilename() ;

	QFile file( cacheFile );
	if( !file.exists() )
	{
		// no image in cache, just return NULL
		return nullptr;
	}
	
	return cacheFile;
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

				QDomElement imageNode = drumkitNode.firstChildElement( "image" );
				if ( !imageNode.isNull() ) {
					soundLibInfo.setImage( imageNode.text() );
				}

				QDomElement imageLicenseNode = drumkitNode.firstChildElement( "imageLicense" );
				if ( !imageLicenseNode.isNull() ) {
					soundLibInfo.setImageLicense( imageLicenseNode.text() );
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
	QString downloadUrl = repositoryCombo->currentText();
	QString sDrumkitXML;

	for (int i=0; i < max_redirects; i++) {
		DownloadWidget drumkitList( this, tr( "Updating SoundLibrary list..." ), downloadUrl);
		drumkitList.exec();

		if (!drumkitList.get_redirect_url().isEmpty()) {
			downloadUrl = drumkitList.get_redirect_url().toEncoded();
		} else if (!drumkitList.get_error()) {
			sDrumkitXML = drumkitList.get_xml_content();
			break;
		}
	}

	/*
	 * Hydrogen creates the following cache hierarchy to cache
	 * the content of server lists:
	 *
	 * CACHE_DIR
	 *     +-----repositories
	 *	   +-----serverlist_$(md5(SERVER_NAME))
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
	m_pDrumkitsItem->setText( 0, tr( "Drumkits" ) );
	m_pDrumkitsItem->setExpanded( true );


	m_pSongItem = new QTreeWidgetItem( m_pDrumkitTree );
	m_pSongItem->setText( 0, tr( "Songs" ) );
	m_pSongItem->setExpanded( true );

	m_pPatternItem = new QTreeWidgetItem( m_pDrumkitTree );
	m_pPatternItem->setText( 0, tr( "Patterns" ) );
	m_pPatternItem->setExpanded( true );

	for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
		QString sLibraryName = m_soundLibraryList[ i ].getName();

		QTreeWidgetItem* pDrumkitItem = nullptr;

		if ( m_soundLibraryList[ i ].getType() == "song" ) {
			pDrumkitItem = new QTreeWidgetItem( m_pSongItem );
		} else if ( m_soundLibraryList[ i ].getType() == "drumkit" ) {
			pDrumkitItem = new QTreeWidgetItem( m_pDrumkitsItem );
		} else if ( m_soundLibraryList[ i ].getType() == "pattern" ) {
			pDrumkitItem = new QTreeWidgetItem( m_pPatternItem );
		}

		if( pDrumkitItem ) {
			if ( isSoundLibraryItemAlreadyInstalled( m_soundLibraryList[ i ]  ) ) {
				pDrumkitItem->setText( 0, sLibraryName );
				pDrumkitItem->setText( 1, tr( "Installed" ) );
			}
			else {
				pDrumkitItem->setText( 0, sLibraryName );
				pDrumkitItem->setText( 1, tr( "New" ) );
			}
		}
	}

	// Also clear out the image cache
	clearImageCache();

}




/// Is the SoundLibrary already installed?
bool SoundLibraryImportDialog::isSoundLibraryItemAlreadyInstalled( SoundLibraryInfo sInfo )
{
	// check if the filename matches with an already installed soundlibrary directory.
	// The filename used in the Soundlibrary URL must be the same of the unpacked directory.
	// E.g: V-Synth_VariBreaks.h2drumkit must contain the V-Synth_VariBreaks directory once unpacked.
	// Many drumkit are broken now (wrong filenames) and MUST be fixed!

	QString sName = QFileInfo( sInfo.getUrl() ).fileName();
	sName = sName.left( sName.lastIndexOf( "." ) );

	if ( sInfo.getType() == "drumkit" ) {
		if ( H2Core::Filesystem::drumkit_exists(sName) ) {
			return true;
		}
	}

	if ( sInfo.getType() == "pattern" ) {
		return SoundLibraryDatabase::get_instance()->isPatternInstalled( sInfo.getName() );
	}

	if ( sInfo.getType() == "song" ) {
		if ( H2Core::Filesystem::song_exists(sName) ) {
			return true;
		}
	}

	return false;
}

void SoundLibraryImportDialog::loadImage(QString img )
{
	QPixmap pixmap;
	pixmap.load( img ) ;

	writeCachedImage( drumkitImageLabel->text(), pixmap );
	showImage( pixmap );
}

void SoundLibraryImportDialog::showImage( QPixmap pixmap )
{
	int x = (int) drumkitImageLabel->size().width();
	int y = drumkitImageLabel->size().height();
	float labelAspect = (float) x / y;
	float imageAspect = (float) pixmap.width() / pixmap.height();

	if ( ( x < pixmap.width() ) || ( y < pixmap.height() ) )
	{
		if ( labelAspect >= imageAspect )
		{
			// image is taller or the same as label frame
			pixmap = pixmap.scaledToHeight( y );
		}
		else
		{
			// image is wider than label frame
			pixmap = pixmap.scaledToWidth( x );
		}
	}
	drumkitImageLabel->setPixmap( pixmap ); // TODO: Check if valid!

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

				AuthorLbl->setText( tr( "Author: %1" ).arg( info.getAuthor() ) );

				LicenseLbl->setText( tr( "Drumkit License: %1" ).arg( info.getLicense()) );

				ImageLicenseLbl->setText( tr("Image License: %1" ).arg( info.getImageLicense() ) );

				// Load the drumkit image
				// Clear any image first
				drumkitImageLabel->setPixmap( QPixmap() );
				drumkitImageLabel->setText( info.getImage() );

				if ( info.getImage().length() > 0 )
				{
					if ( isSoundLibraryItemAlreadyInstalled( info ) )
					{
						// get image file from local disk
						QString sName = QFileInfo( info.getUrl() ).fileName();
						sName = sName.left( sName.lastIndexOf( "." ) );

						H2Core::Drumkit* drumkitInfo = H2Core::Drumkit::load_by_name( sName, false );
						if ( drumkitInfo )
						{
							// get the image from the local filesystem
							QPixmap pixmap ( drumkitInfo->get_path() + "/" + drumkitInfo->get_image() );
							INFOLOG("Loaded image " + drumkitInfo->get_image().toLocal8Bit() + " from local filesystem");
							showImage( pixmap );
						}
						else
						{
							___ERRORLOG ( "Error loading the drumkit" );
						}

					}
					else
					{
						// Try from the cache
						QString cachedFile = readCachedImage( info.getImage() );
						
						if ( cachedFile.length() > 0 )
						{
							QPixmap pixmap ( cachedFile );
							showImage( pixmap );
							INFOLOG( "Loaded image " + info.getImage().toLocal8Bit() + " from cache (" + cachedFile + ")" );
						}
						else
						{
							// Get the drumkit's directory name from URL
							//
							// Example: if the server repo URL is: http://www.hydrogen-music.org/feeds/drumkit_list.php
							// and the image name from the XML is Roland_TR-808_drum_machine.jpg
							// the URL for the image will be: http://www.hydrogen-music.org/feeds/images/Roland_TR-808_drum_machine.jpg

							if ( info.getImage().length() > 0 )
							{
								QString sImageUrl;
								QString sLocalFile;
								
								sImageUrl = repositoryCombo->currentText().left( repositoryCombo->currentText().lastIndexOf( QString( "/" )) + 1 ) + info.getImage() ;
								sLocalFile = QDir::tempPath() + "/" + QFileInfo( sImageUrl ).fileName();

								DownloadWidget dl( this, tr( "" ), sImageUrl, sLocalFile );
								dl.exec();

								loadImage( sLocalFile );
								// Delete the temporary file
								QFile::remove( sLocalFile );
							}
						}
					}
				}
				else
				{
					// no image file specified in drumkit.xml
					INFOLOG( "No image for this kit specified in drumkit.xml on remote server" );
				}
				
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

			if( sType == "drumkit") {
				sLocalFile = QDir::tempPath() + "/" + QFileInfo( sURL ).fileName();
			}

			if( sType == "song") {
				sLocalFile = H2Core::Filesystem::songs_dir() + QFileInfo( sURL ).fileName();
			}

			if( sType == "pattern") {
				sLocalFile = H2Core::Filesystem::patterns_dir() + QFileInfo( sURL ).fileName();
			}

			bool Error = false;

			for ( int i = 0; i < max_redirects; ++i ) {
				DownloadWidget dl( this, tr( "Downloading SoundLibrary..." ), sURL, sLocalFile );
				dl.exec();


				QUrl redirect_url = dl.get_redirect_url();
				if (redirect_url.isEmpty() ) {
					// ok, we have all data
					Error = dl.get_error();
					break;
				}
				else {
					sURL = redirect_url.toEncoded();
					Error = dl.get_error();
				}
			}


			//No 'else', error message has been already displayed by DL widget
			if(!Error)
			{
				// install the new soundlibrary
				try {
					if ( sType == "drumkit" ) {
						H2Core::Drumkit::install( sLocalFile );
						QApplication::restoreOverrideCursor();
						QMessageBox::information( this, "Hydrogen", QString( tr( "SoundLibrary imported in %1" ) ).arg( H2Core::Filesystem::usr_data_path() ) );
					}

					if ( sType == "song" || sType == "pattern") {
						QApplication::restoreOverrideCursor();
					}
				}
				catch( H2Core::H2Exception ex ) {
					QApplication::restoreOverrideCursor();
					QMessageBox::warning( this, "Hydrogen", tr( "An error occurred importing the SoundLibrary."  ) );
				}
			}
			else
			{
				QApplication::restoreOverrideCursor();
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

	QFileDialog fd(this);
	fd.setFileMode(QFileDialog::ExistingFile);
	fd.setNameFilter( "Hydrogen drumkit (*.h2drumkit)" );
	fd.setDirectory( lastUsedDir );

	fd.setWindowTitle( tr( "Import drumkit" ) );

	QString filename = "";
	if (fd.exec() == QDialog::Accepted) {
		filename = fd.selectedFiles().first();
	}

	if (filename != "") {
		SoundLibraryPathTxt->setText( filename );
		lastUsedDir = fd.directory().absolutePath();
		InstallBtn->setEnabled ( true );
	}
}




void SoundLibraryImportDialog::on_InstallBtn_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	try {
		H2Core::Drumkit::install( SoundLibraryPathTxt->text() );
		QMessageBox::information( this, "Hydrogen", QString( tr( "SoundLibrary imported in %1" ).arg( H2Core::Filesystem::usr_data_path() )  ) );
		// update the drumkit list
		SoundLibraryDatabase::get_instance()->update();
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
		QApplication::restoreOverrideCursor();
	}
	catch( H2Core::H2Exception ex ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::warning( this, "Hydrogen", tr( "An error occurred importing the SoundLibrary."  ) );
	}
}



void SoundLibraryImportDialog::on_close_btn_clicked()
{
	accept();
}
