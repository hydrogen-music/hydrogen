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

#include "SoundLibraryImportDialog.h"
#include "../widgets/DownloadWidget.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"

#include <hydrogen/LocalFileMng.h>
#include <hydrogen/H2Exception.h>
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
	DownloadWidget drumkitList( this, trUtf8( "Updating SoundLibrary list..." ), "http://www.hydrogen-music.org/feeds/drumkit_list.php" );
	drumkitList.exec();

	m_soundLibraryList.clear();

	// analizzo l'xml scaricato
	QString sDrumkitXML = drumkitList.getXMLContent();

	QDomDocument dom;
	dom.setContent( sDrumkitXML );
	QDomNode drumkitNode = dom.documentElement().firstChild();
	while ( !drumkitNode.isNull() ) {
		if( !drumkitNode.toElement().isNull() ) {
			if ( drumkitNode.toElement().tagName() == "drumkit" ) {

				SoundLibraryInfo soundLibInfo;

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

	for ( uint i = 0; i < m_soundLibraryList.size(); ++i ) {
		QString sLibraryName = m_soundLibraryList[ i ].m_sName;

		QTreeWidgetItem* pDrumkitItem = new QTreeWidgetItem( m_pDrumkitTree );

		if ( isSoundLibraryAlreadyInstalled( m_soundLibraryList[ i ].m_sURL ) ) {
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
bool SoundLibraryImportDialog::isSoundLibraryAlreadyInstalled( QString sURL )
{
	// check if the filename matchs with an already installed soundlibrary directory.
	// The filename used in the Soundlibrary URL must be the same of the unpacked directory.
	// E.g: V-Synth_VariBreaks.h2drumkit must contain the V-Synth_VariBreaks directory once unpacked.
	// Many drumkit are broken now (wrong filenames) and MUST be fixed!

	string soundLibraryName = QFileInfo( sURL ).fileName().toStdString();
	soundLibraryName = soundLibraryName.substr( 0, soundLibraryName.rfind( "." ) );

	vector<string> systemList = H2Core::Drumkit::getSystemDrumkitList();
	for ( uint i = 0; i < systemList.size(); ++i ) {
		if ( systemList[ i ] == soundLibraryName ) {
			return true;
		}
	}

	vector<string> userList = H2Core::Drumkit::getUserDrumkitList();
	for ( uint i = 0; i < userList.size(); ++i ) {
		if ( userList[ i ] == soundLibraryName ) {
			return true;
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
			QString sLocalFile = QDir::tempPath () + "/" + QFileInfo( sURL ).fileName();
			DownloadWidget drumkit( this, trUtf8( "Downloading SoundLibrary..." ), sURL, sLocalFile );
			drumkit.exec();

			// install the new soundlibrary
			setCursor( QCursor( Qt::WaitCursor ) );

			std::string dataDir = H2Core::Preferences::getInstance()->getDataDirectory();
			try {
				H2Core::Drumkit::install( sLocalFile.toStdString() );
				QMessageBox::information( this, "Hydrogen", QString( trUtf8( "SoundLibrary imported in %1" ) ).arg( dataDir.c_str() ) );
				setCursor( QCursor( Qt::ArrowCursor ) );
			}
			catch( H2Core::H2Exception ex ) {
				setCursor( QCursor( Qt::ArrowCursor ) );
				QMessageBox::warning( this, "Hydrogen", trUtf8( "An error occurred importing the SoundLibrary."  ) );
			}

			// remove the downloaded files..
			QDir dir;
			dir.remove( sLocalFile );

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

	string dataDir = H2Core::Preferences::getInstance()->getDataDirectory();
	try {
		H2Core::Drumkit::install( string( SoundLibraryPathTxt->text().toStdString() ) );
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




