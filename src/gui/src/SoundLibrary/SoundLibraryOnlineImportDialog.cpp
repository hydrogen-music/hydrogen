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

#include "SoundLibraryOnlineImportDialog.h"
#include "SoundLibraryRepositoryDialog.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"
#include "../Widgets/DownloadWidget.h"

#include <core/Basics/Drumkit.h>
#include <core/H2Exception.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QTreeWidget>
#include <QDomDocument>
#include <QMessageBox>
#include <QHeaderView>
#include <QCryptographicHash>

#include <memory>

const int max_redirects = 30;

SoundLibraryOnlineImportDialog::SoundLibraryOnlineImportDialog( QWidget* pParent )
 : QDialog( pParent )
{
	setupUi( this );

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	setWindowTitle( tr( "Sound Library import" ) );

	m_sLabelInstalled = tr( "Installed" );
	m_sLabelNew = pCommonStrings->getMenuActionNew();

	QStringList headers;
	headers << tr( "Sound library" ) << tr( "Status" );
	QTreeWidgetItem* header = new QTreeWidgetItem( headers );
	m_pDrumkitTree->setHeaderItem( header );
	m_pDrumkitTree->header()->resizeSection( 0, 200 );

	connect( m_pDrumkitTree,
			 SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem* ) ),
			 this,
			 SLOT( soundLibraryItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );
	connect( repositoryCombo, SIGNAL(currentIndexChanged(int)),
			 this, SLOT( onRepositoryComboBoxIndexChanged(int) ));

	SoundLibraryNameLbl->setText( "" );
	SoundLibraryInfoLbl->setText( "" );
	DownloadBtn->setIsActive( false );

	UpdateListBtn->setSize( QSize( 105, 24 ) );
	UpdateListBtn->setType( Button::Type::Push );
	EditListBtn->setSize( QSize( 130, 24 ) );
	EditListBtn->setType( Button::Type::Push );
	DownloadBtn->setSize( QSize( 215, 24 ) );
	DownloadBtn->setType( Button::Type::Push );
	close_btn->setSize( QSize( 80, 24 ) );
	close_btn->setType( Button::Type::Push );

	m_sDownloadBtnBase = DownloadBtn->text();
	connect( m_pDrumkitTree, &QTreeWidget::itemSelectionChanged,
			 this, &SoundLibraryOnlineImportDialog::selectionChanged );

	updateRepositoryCombo();
}




SoundLibraryOnlineImportDialog::~SoundLibraryOnlineImportDialog()
{
	if ( auto pH2App = HydrogenApp::get_instance() ) {
		pH2App->removeEventListener( this );
	}
}

//update combo box
void SoundLibraryOnlineImportDialog::updateRepositoryCombo()
{
	auto pPref = H2Core::Preferences::get_instance();

	/*
		Read serverList from config and put servers into the comboBox
	*/

	if ( pPref->m_serverList.size() == 0 ) {
		pPref->m_serverList.push_back(
			"http://hydrogen-music.org/feeds/drumkit_list.php" );
	}

	repositoryCombo->clear();

	for ( const auto& ssServer : pPref->m_serverList ) {
		repositoryCombo->insertItem( 0, ssServer );
	}
	reloadRepositoryData();
}

void SoundLibraryOnlineImportDialog::onRepositoryComboBoxIndexChanged(int i)
{
	UNUSED(i);

	if(!repositoryCombo->currentText().isEmpty())
	{
		QString cacheFile = getCachedFileName();
		if( !H2Core::Filesystem::file_exists( cacheFile, true ) )
		{
			SoundLibraryOnlineImportDialog::on_UpdateListBtn_clicked();
		}
		reloadRepositoryData();
	}
}

///
/// Edit the server list
///
void SoundLibraryOnlineImportDialog::on_EditListBtn_clicked()
{
	SoundLibraryRepositoryDialog repoDialog( this );
	repoDialog.exec();
	updateRepositoryCombo();
}

void SoundLibraryOnlineImportDialog::clearImageCache()
{
	// Note: After a kit is installed the list refreshes and this gets called to
	// clear the image cache - maybe we want to keep the cache in this case?
	QString cacheDir = H2Core::Filesystem::repositories_cache_dir() ;
	INFOLOG("Deleting cached image files from " + cacheDir );

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

QString SoundLibraryOnlineImportDialog::getCachedFileName()
{
	const QString sCacheDir = H2Core::Filesystem::repositories_cache_dir();
	const QString sServerMd5 = QString(
		QCryptographicHash::hash( repositoryCombo->currentText().toLatin1(),
								  QCryptographicHash::Md5 ).toHex() );
	return sCacheDir + "/" + sServerMd5;
}

QString SoundLibraryOnlineImportDialog::getCachedImageFileName()
{
	const QString sCacheDir = H2Core::Filesystem::repositories_cache_dir();
	const QString sKitNameMd5 = QString(
		QCryptographicHash::hash( SoundLibraryNameLbl->text().toLatin1(),
								  QCryptographicHash::Md5 ).toHex() );
	return sCacheDir + "/" + sKitNameMd5 + ".png";
}


void SoundLibraryOnlineImportDialog::writeCachedData(const QString& sFileName, const QString& data)
{
	if( data.isEmpty() )
	{
		return;
	}

	QFile outFile( sFileName );
	if( !outFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		ERRORLOG( QString("Failed to open file for writing repository cache: %1").arg( sFileName ) );
		return;
	}

	QTextStream stream( &outFile );
	stream << data;

	outFile.close();
}

void SoundLibraryOnlineImportDialog::writeCachedImage( const QString& imageFile, const QPixmap& pixmap )
{
	QString cacheFile = getCachedImageFileName() ;

	QFile outFile( cacheFile );
	if( !outFile.open( QIODevice::WriteOnly ) )
	{
		ERRORLOG( QString("Failed to open file for writing repository image cache: %1").arg( imageFile ) );
		return;
	}
	
	pixmap.save(&outFile);

	outFile.close();
}

QString SoundLibraryOnlineImportDialog::readCachedData( const QString& sFileName ) {
	QFile inFile( sFileName );
	if ( ! inFile.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
		ERRORLOG( QString("Failed to open file for reading: %1").arg( sFileName ) );
		return "";
	}

	QDomDocument document;
	if ( ! document.setContent( &inFile ) ) {
		inFile.close();
		return "";
	}
	inFile.close();

	return document.toString();
}

QString SoundLibraryOnlineImportDialog::readCachedImage( const QString& imageFile )
{
	QString cacheFile = getCachedImageFileName() ;

	QFile file( cacheFile );
	if( !file.exists() )
	{
		// no image in cache, just return NULL
		return nullptr;
	}
	
	return cacheFile;
}

void SoundLibraryOnlineImportDialog::reloadRepositoryData()
{
	QString sDrumkitXML;
	const QString sCacheFile = getCachedFileName();

	if ( H2Core::Filesystem::file_exists( sCacheFile, true ) ) {
		sDrumkitXML = readCachedData( sCacheFile );
	}

	m_soundLibraryList.clear();

	QDomDocument dom;
	dom.setContent( sDrumkitXML );

	auto setIfPresent = []( H2Core::SoundLibraryInfo& info,
							const QDomNode& node, const QString& sLabel ) {
		const QDomElement childNode = node.firstChildElement( sLabel );
		if ( ! childNode.isNull() ) {
			info.setName( childNode.text() );
		}
	};

	QDomNode drumkitNode = dom.documentElement().firstChild();
	while ( ! drumkitNode.isNull() ) {
		if ( ! drumkitNode.toElement().isNull() &&
			 ( drumkitNode.toElement().tagName() == "drumkit" ||
			   drumkitNode.toElement().tagName() == "song" ||
			   drumkitNode.toElement().tagName() == "pattern" ) ) {

			m_soundLibraryList.push_back( H2Core::SoundLibraryInfo(
				drumkitNode.firstChildElement( "name" ).text(),
				drumkitNode.firstChildElement( "url" ).text(),
				drumkitNode.firstChildElement( "info" ).text(),
				drumkitNode.firstChildElement( "author" ).text(),
				drumkitNode.firstChildElement( "category" ).text(),
				drumkitNode.toElement().tagName(),
				H2Core::License(
					drumkitNode.firstChildElement( "license" ).text() ),
				drumkitNode.firstChildElement( "image" ).text(),
				H2Core::License(
					drumkitNode.firstChildElement( "imageLicense" ).text() ),
				"" ) );
		}
		drumkitNode = drumkitNode.nextSibling();
	}

	updateSoundLibraryList();
}

///
/// Download and update the drumkit list
///
void SoundLibraryOnlineImportDialog::on_UpdateListBtn_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QString downloadUrl = repositoryCombo->currentText();
	QString sDrumkitXML;

	for (int ii=1; ii <= max_redirects; ii++) {
		DownloadWidget drumkitList( this, tr( "Updating SoundLibrary list..." ), downloadUrl);
		drumkitList.exec();

		if (!drumkitList.get_redirect_url().isEmpty()) {
			downloadUrl = drumkitList.get_redirect_url().toEncoded();
		} else if (drumkitList.get_error().isEmpty()) {
			sDrumkitXML = drumkitList.get_xml_content();
			break;
		}

		// Only show a popup with the error messages once after
		// attempting all redirects. We assume in here that the error
		// does stay the same for all redirects.
		if ( ii == max_redirects ) {
			QMessageBox::warning( this, "Hydrogen", drumkitList.get_error() );
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


	QString cacheFile = getCachedFileName();


	writeCachedData(cacheFile, sDrumkitXML);

	reloadRepositoryData();
	QApplication::restoreOverrideCursor();
}




void SoundLibraryOnlineImportDialog::updateSoundLibraryList()
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
				pDrumkitItem->setText( 1, m_sLabelInstalled );
			}
			else {
				pDrumkitItem->setText( 0, sLibraryName );
				pDrumkitItem->setText( 1, m_sLabelNew );
			}
		}
	}

	// Also clear out the image cache
	clearImageCache();

}

void SoundLibraryOnlineImportDialog::soundLibraryChangedEvent() {
	updateSoundLibraryList();
}


/// Is the SoundLibrary already installed?
bool SoundLibraryOnlineImportDialog::isSoundLibraryItemAlreadyInstalled( const H2Core::SoundLibraryInfo& sInfo )
{
	// check if the filename matches with an already installed soundlibrary directory.
	// The filename used in the Soundlibrary URL must be the same of the unpacked directory.
	// E.g: V-Synth_VariBreaks.h2drumkit must contain the V-Synth_VariBreaks directory once unpacked.
	// Many drumkit are broken now (wrong filenames) and MUST be fixed!

	QString sName = QFileInfo( sInfo.getUrl() ).fileName();
	sName = sName.left( sName.lastIndexOf( "." ) );

	if ( sInfo.getType() == "drumkit" ) {
		if ( H2Core::Filesystem::drumkit_exists( sName ) ||
			H2Core::Filesystem::drumkit_exists( sInfo.getName() ) ) {
			return true;
		}
	}

	if ( sInfo.getType() == "pattern" ) {
		return H2Core::Hydrogen::get_instance()->getSoundLibraryDatabase()
			->isPatternInstalled( sInfo.getName() );
	}

	if ( sInfo.getType() == "song" ) {
		if ( H2Core::Filesystem::song_exists(sName) ) {
			return true;
		}
	}

	return false;
}

void SoundLibraryOnlineImportDialog::loadImage( const QString& img )
{
	QPixmap pixmap;
	pixmap.load( img ) ;

	writeCachedImage( drumkitImageLabel->text(), pixmap );
	showImage( pixmap );
}

void SoundLibraryOnlineImportDialog::showImage( const QPixmap& pixmap )
{
	int x = (int) drumkitImageLabel->size().width();
	int y = drumkitImageLabel->size().height();
	float labelAspect = (float) x / y;
	float imageAspect = (float) pixmap.width() / pixmap.height();

	if ( x < pixmap.width() || y < pixmap.height() ) {
		if ( labelAspect >= imageAspect ) {
			// image is taller or the same as label frame
			drumkitImageLabel->setPixmap( pixmap.scaledToHeight( y ) );
		}
		else {
			// image is wider than label frame
			drumkitImageLabel->setPixmap( pixmap.scaledToWidth( x ) );
		}
	}
	else {
		drumkitImageLabel->setPixmap( pixmap ); // TODO: Check if valid!
	}
}


void SoundLibraryOnlineImportDialog::soundLibraryItemChanged( QTreeWidgetItem* pCurrentItem,
															  QTreeWidgetItem*  )
{
	auto resetLabels = [=](){
		SoundLibraryNameLbl->setText( "" );
		SoundLibraryInfoLbl->setText( "" );
		AuthorLbl->setText( "" );
		LicenseLbl->setText( "" );
	};

	if ( pCurrentItem == nullptr ) {
		resetLabels();
		return;
	}

	if ( pCurrentItem == m_pDrumkitsItem || pCurrentItem == m_pSongItem ||
		 pCurrentItem == m_pPatternItem ) {
		resetLabels();
		pCurrentItem->setSelected( false );
		return;
	}

	QString selected = pCurrentItem->text(0);
	for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
		if ( m_soundLibraryList[ i ].getName() == selected ) {
			H2Core::SoundLibraryInfo info = m_soundLibraryList[ i ];

			//bool alreadyInstalled = isSoundLibraryAlreadyInstalled( info.m_sURL );

			SoundLibraryNameLbl->setText( info.getName() );

			if( info.getType() == "pattern" ){
				SoundLibraryInfoLbl->setText("");
			} else {
				SoundLibraryInfoLbl->setText( info.getInfo() );
			}

			AuthorLbl->setText( tr( "Author: %1" ).arg( info.getAuthor() ) );

			LicenseLbl->setText( tr( "Drumkit License: %1" )
								 .arg( info.getLicense().getLicenseString() ) );

			ImageLicenseLbl->setText( tr("Image License: %1" )
									  .arg( info.getImageLicense().getLicenseString() ) );

			// Load the drumkit image
			// Clear any image first
			drumkitImageLabel->setPixmap( QPixmap() );
			drumkitImageLabel->setText( info.getImage() );

			if ( info.getImage().length() > 0 ) {
				if ( isSoundLibraryItemAlreadyInstalled( info ) ) {
					// get image file from local disk
					QString sName = QFileInfo( info.getUrl() ).fileName();
					sName = sName.left( sName.lastIndexOf( "." ) );

					auto pDrumkit = H2Core::Hydrogen::get_instance()
						->getSoundLibraryDatabase()->getDrumkit( info.getPath() );
					if ( pDrumkit != nullptr ) {
						// get the image from the local filesystem
						QPixmap pixmap ( pDrumkit->getPath() + "/" + pDrumkit->getImage() );
						INFOLOG("Loaded image " + pDrumkit->getImage() + " from local filesystem");
						showImage( pixmap );
					}
					else {
						___ERRORLOG ( "Error loading the drumkit" );
					}

				}
				else {
					// Try from the cache
					QString cachedFile = readCachedImage( info.getImage() );

					if ( cachedFile.length() > 0 ) {
						QPixmap pixmap ( cachedFile );
						showImage( pixmap );
						INFOLOG( "Loaded image " + info.getImage() + " from cache (" + cachedFile + ")" );
					}
					else {
						// Get the drumkit's directory name from URL
						//
						// Example: if the server repo URL is: http://www.hydrogen-music.org/feeds/drumkit_list.php
						// and the image name from the XML is Roland_TR-808_drum_machine.jpg
						// the URL for the image will be: http://www.hydrogen-music.org/feeds/images/Roland_TR-808_drum_machine.jpg

						if ( info.getImage().length() > 0 ) {
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

			return;
		}
	}

	SoundLibraryNameLbl->setText( "" );
	SoundLibraryInfoLbl->setText( "" );
	AuthorLbl->setText( "" );
}

void SoundLibraryOnlineImportDialog::on_DownloadBtn_clicked()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QApplication::setOverrideCursor( Qt::WaitCursor );

	bool bUpdateDrumkits = false;
	bool bUpdatePatterns = false;

	QStringList installedDrumkits;
	// Associate a download error message (key) with all drumkits receiving it.
	std::map<QString, QStringList> failedDrumkitMap;

	for ( const auto& ppItem : m_pDrumkitTree->selectedItems() ) {
		if ( ppItem == nullptr ) {
			continue;
		}

		const QString sSelected = ppItem->text( 0 );
		if ( ppItem->text( 1 ) == m_sLabelInstalled ) {
			// Item already installed. Skipping...
			continue;
		}

		for ( int ii = 0; ii < m_soundLibraryList.size(); ++ii ) {
			if ( m_soundLibraryList[ii].getName() != sSelected ) {
				continue;
			}
			// Download the sound library
			const QString sName = m_soundLibraryList[ii].getName();
			const QString sType = m_soundLibraryList[ii].getType();
			QString sURL = m_soundLibraryList[ii].getUrl();
			QString sLocalFile;

			if ( sType == "drumkit" ) {
				sLocalFile =
					QDir::tempPath() + "/" + QFileInfo( sURL ).fileName();
			}
			else if ( sType == "song" ) {
				sLocalFile = H2Core::Filesystem::songs_dir() +
							 QFileInfo( sURL ).fileName();
			}
			else if ( sType == "pattern" ) {
				sLocalFile = H2Core::Filesystem::patterns_dir() +
							 QFileInfo( sURL ).fileName();
				bUpdatePatterns = true;
			}
			else {
				ERRORLOG( QString( "Unknown type [%1]" ).arg( sType ) );
				continue;
			}

			// We store the (potential) error message of the last download
			// attempt.
			QString sError;
			for ( int jj = 0; jj < max_redirects; ++jj ) {
				DownloadWidget dl(
					this, tr( "Downloading SoundLibrary..." ), sURL, sLocalFile
				);
				dl.exec();

				sError = dl.get_error();

				QUrl redirect_url = dl.get_redirect_url();
				if ( redirect_url.isEmpty() ) {
					// ok, we have all data
					break;
				}
				else {
					sURL = redirect_url.toEncoded();
				}
			}

			if ( !sError.isEmpty() ) {
				failedDrumkitMap[sError] << sName;
			}
			else {
				// Success
				ppItem->setText( 1, m_sLabelInstalled );
				updateDownloadBtn();

				if ( sType == "drumkit" ) {
					QString sImportedPath;
					bool bEncodingIssues;
					if ( H2Core::Drumkit::install(
							 sLocalFile, "", &sImportedPath, &bEncodingIssues
						 ) ) {
						QDir dir;
						dir.remove( sLocalFile );
						bUpdateDrumkits = true;

						installedDrumkits << sName;
						if ( bEncodingIssues ) {
							QApplication::restoreOverrideCursor();
							QMessageBox::warning(
								this, "Hydrogen",
								QString( "%1: %2 [%3]%4" )
									.arg( sName )
									.arg( pCommonStrings
											  ->getImportDrumkitSuccess() )
									.arg( sImportedPath )
									.arg( pCommonStrings
											  ->getImportDrumkitEncodingFailure(
											  ) )
							);
							QApplication::setOverrideCursor( Qt::WaitCursor );
						}
					}
					else {
						QApplication::restoreOverrideCursor();
						if ( MainForm::checkDrumkitPathEncoding(
								 sLocalFile,
								 pCommonStrings->getImportDrumkitFailure()
							 ) ) {
							// In case it was not an encoding error, we have
							// to create and show an error dialog ourselves.
							QMessageBox::critical(
								nullptr, "Hydrogen",
								QString( "%1\n\n%2" )
									.arg( pCommonStrings
											  ->getImportDrumkitFailure() )
									.arg( sName )
							);
						}
						QApplication::setOverrideCursor( Qt::WaitCursor );
					}
				}
			}

			break;
		}
	}

	auto pDB = H2Core::Hydrogen::get_instance()->getSoundLibraryDatabase();
	if ( bUpdatePatterns ) {
		pDB->updatePatterns();
	}

	if ( bUpdateDrumkits ) {
		pDB->updateDrumkits();
	}

	QApplication::restoreOverrideCursor();

	if ( failedDrumkitMap.size() > 0 ) {
		/*: Shown in a dialog in case a download of an online resource did fail.
		 * A list of names does follow in a new line. */
		QString sMsg( tr( "Download failed for" ) + "\n" );
		for ( const auto& [ssError, ddrumkits] : failedDrumkitMap ) {
			sMsg.append( QString( "\n- %1\n[%2]" )
							 .arg( ddrumkits.join( "\n- " ) )
							 .arg( ssError ) );
		}

		QMessageBox::warning( this, "Hydrogen", sMsg );
	}
	if ( installedDrumkits.size() > 0 ) {
		QMessageBox::information(
			this, "Hydrogen",
			QString( tr( "Drumkits\n\n- %1\n\nimported into %2" ) )
				.arg( installedDrumkits.join( "\n- " ) )
				.arg( H2Core::Filesystem::usr_data_path() )
		);
	}

	return;
}

void SoundLibraryOnlineImportDialog::on_close_btn_clicked() {
	accept();
}

void SoundLibraryOnlineImportDialog::selectionChanged() {

	// We do not take the root nodes into account.
	auto pCurrentItem = m_pDrumkitTree->currentItem();
	if ( pCurrentItem != nullptr && (
			 pCurrentItem == m_pDrumkitsItem || pCurrentItem == m_pSongItem ||
			 pCurrentItem == m_pPatternItem ) ) {
		pCurrentItem->setSelected( false );
	}

	updateDownloadBtn();
}

void SoundLibraryOnlineImportDialog::updateDownloadBtn() {
	// Determine how many of the sSelected kits are not installed yet.
	int nInstalledKits = 0;
	for ( const auto& ppItem : m_pDrumkitTree->selectedItems() ) {
		if ( ppItem != nullptr && ppItem->text( 1 ) == m_sLabelNew ) {
			++nInstalledKits;
		}
	}

	if ( nInstalledKits == 0 ) {
		DownloadBtn->setIsActive( false );
		DownloadBtn->setText( m_sDownloadBtnBase );
	}
	else {
		DownloadBtn->setIsActive( true );
		DownloadBtn->setText(
			QString( "%1 (%2)" ).arg( m_sDownloadBtnBase )
			.arg( nInstalledKits ) );
	}
}
