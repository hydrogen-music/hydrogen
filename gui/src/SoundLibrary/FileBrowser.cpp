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

#include "FileBrowser.h"

#include <QVBoxLayout>
#include <QDir>

#include "math.h"
#include "string.h"

#include <hydrogen/hydrogen.h>
#include <hydrogen/sample.h>
#include <hydrogen/audio_engine.h>
using namespace H2Core;

FileBrowser::FileBrowser( QWidget* pParent )
 : QWidget( pParent )
 , Object( "FileBrowser" )
{
	INFOLOG( "[FileBrowser]" );

	m_pDirectoryLabel = new QLabel( NULL );
	m_pUpBtn = new QPushButton( "..", NULL );
	m_pUpBtn->setMaximumWidth( 30 );
	connect( m_pUpBtn, SIGNAL( clicked() ), this, SLOT( on_upBtnClicked() ) );

	QWidget *pDirectoryPanel = new QWidget( NULL );
	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->setSpacing( 0 );
	hbox->setMargin( 0 );
	hbox->addWidget( m_pDirectoryLabel );
	hbox->addWidget( m_pUpBtn );
	pDirectoryPanel->setLayout( hbox );

	QWidget *pInfoPanel = new QWidget( NULL );
	m_pFileInfo = new QLabel( NULL );
	QPushButton *pPlayBtn = new QPushButton( "Play", NULL );
	connect( pPlayBtn, SIGNAL( clicked() ), this, SLOT( on_playBtnClicked() ) );
	pPlayBtn->setMaximumWidth( 40 );
	QHBoxLayout *pInfoHBox = new QHBoxLayout();
	pInfoHBox->addWidget( m_pFileInfo );
	pInfoHBox->addWidget( pPlayBtn );
	pInfoPanel->setLayout( pInfoHBox );

	m_pDirList = new QListWidget( NULL );
	m_pFileList = new QListWidget( NULL );

	connect( m_pFileList, SIGNAL( currentItemChanged( QListWidgetItem*, QListWidgetItem*) ), this, SLOT( on_fileList_ItemChanged( QListWidgetItem*, QListWidgetItem* ) ) );
	connect( m_pFileList, SIGNAL( itemActivated( QListWidgetItem* ) ), this, SLOT( on_fileList_ItemActivated( QListWidgetItem* ) ) );
	connect( m_pDirList, SIGNAL( itemActivated( QListWidgetItem* ) ), this, SLOT( on_dirList_ItemActivated( QListWidgetItem* ) ) );

	// LAYOUT
	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->setSpacing( 0 );
	vbox->setMargin( 0 );

	vbox->addWidget( pDirectoryPanel );
	vbox->addWidget( m_pDirList );
	vbox->addWidget( m_pFileList );
	vbox->addWidget( pInfoPanel );

	this->setLayout( vbox );


	updateFileInfo( "", 0, 0 );
	loadDirectoryTree( QDir::homePath() );

}



FileBrowser::~FileBrowser()
{
	INFOLOG( "[~FileBrowser]" );
}



void FileBrowser::loadDirectoryTree( const QString& sBasedir )
{
	INFOLOG( "[loadDirectoryTree]" );
	m_pDirList->clear();
	m_pFileList->clear();

	m_directory.setPath( sBasedir );
	m_pDirectoryLabel->setText( sBasedir );

	QFileInfoList list = m_directory.entryInfoList();
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);

		QListWidgetItem *pItem = new QListWidgetItem();
		if ( fileInfo.isDir() ) {
			if ( fileInfo.fileName().startsWith( "." ) ) {
				continue;
			}

			pItem->setText( fileInfo.fileName() );
			m_pDirList->insertItem( 0, pItem);
		}
	}
	m_pDirList->sortItems( Qt::AscendingOrder );


	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);

		QListWidgetItem *pItem = new QListWidgetItem();
		if ( !fileInfo.isDir() ) {
			bool bOk = false;
			if ( fileInfo.fileName().endsWith( ".wav", Qt::CaseInsensitive ) ) {
				bOk = true;
			}
			else if ( fileInfo.fileName().endsWith( ".flac", Qt::CaseInsensitive ) ) {
				bOk = true;
			}
			else if ( fileInfo.fileName().endsWith( ".au", Qt::CaseInsensitive ) ) {
				bOk = true;
			}
			else if ( fileInfo.fileName().endsWith( ".aiff", Qt::CaseInsensitive ) ) {
				bOk = true;
			}

			if ( bOk ) {
				pItem->setText( fileInfo.fileName() );
				m_pFileList->insertItem( 0, pItem);
			}
		}
	}
	m_pFileList->sortItems( Qt::AscendingOrder );

}



void FileBrowser::updateFileInfo( QString sFilename, unsigned nSampleRate, unsigned nBytes )
{

	char sFileSizeUnit[6];
	char sFileSize[32];

	if( nBytes >= 1073741824 ){

		sprintf( sFileSize, "%#.3f", (float)nBytes / 1073741824.0 );
		strcpy( sFileSizeUnit, "GByte" );

	} else if( nBytes >= 1048576 ){

		sprintf( sFileSize, "%#.2f", (float)nBytes / 1048576.0 );
		strcpy( sFileSizeUnit, "MByte" );

	} else if(nBytes >= 1024) {

		sprintf( sFileSize, "%#.1f", (float)nBytes / 1024.0 );
		strcpy( sFileSizeUnit, "KByte" );

	} else {

		sprintf( sFileSize, "%#.0f", (double)nBytes );
		strcpy( sFileSizeUnit, "Byte" );

	}

	// get sample rate of the file

	//Sample* mySample = Sample::load(sFilename.toLocal8Bit().constData());
	//int nSamplerate = mySample->m_nSampleRate;

	m_pFileInfo->setText( QString( trUtf8( "%1<br>%2 KHz<br>%3 %4" ) )
			      .arg( sFilename )
			      .arg( nSampleRate )
			      .arg( sFileSize )
			      .arg( sFileSizeUnit )
	    );

}



void FileBrowser::on_fileList_ItemChanged( QListWidgetItem * current, QListWidgetItem * previous )
{
	UNUSED( previous );
	INFOLOG( "[on_fileList_ItemChanged]" );
	if ( current ) {
		QString sFileName = current->text();
		QFileInfoList list = m_directory.entryInfoList();
		for (int i = 0; i < list.size(); ++i) {
			QFileInfo fileInfo = list.at(i);
			if ( fileInfo.fileName() == current->text() ) {
				updateFileInfo( sFileName, 0, fileInfo.size() );
			}
		}
	}
}



void FileBrowser::on_fileList_ItemActivated( QListWidgetItem* item )
{
	if ( !item ) {
		return;
	}
	QString sFileName = m_directory.absolutePath() + "/" + ( item->text() );

	QFileInfoList list = m_directory.entryInfoList();
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		if ( fileInfo.fileName() == item->text() ) {
			INFOLOG( "[on_fileList_ItemActivated] " + fileInfo.absoluteFilePath() );
			if ( !fileInfo.isDir() ) {

				// FIXME: evitare di caricare il sample, visualizzare solo le info del file
				Sample *pNewSample = Sample::load( fileInfo.absoluteFilePath() );
				if (pNewSample) {
					updateFileInfo( fileInfo.absoluteFilePath(), pNewSample->get_sample_rate(), pNewSample->get_size() );
					AudioEngine::get_instance()->get_sampler()->preview_sample(pNewSample, 192);
				}
			}
		}
	}
}



void FileBrowser::on_dirList_ItemActivated( QListWidgetItem* pItem )
{
	INFOLOG( "[on_dirList_ItemActivated]" );

	if ( !pItem ) {
		return;
	}
	QString sFileName = m_directory.absolutePath() + "/" + ( pItem->text() );

	QFileInfoList list = m_directory.entryInfoList();
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		if ( fileInfo.fileName() == pItem->text() ) {
			if ( fileInfo.isDir() ) {
				// change directory
				loadDirectoryTree( fileInfo.absoluteFilePath() );
				return;
			}
		}
	}

}



void FileBrowser::on_upBtnClicked()
{
	INFOLOG( "[on_upBtnClicked]" );
	m_directory.cdUp();
	loadDirectoryTree( m_directory.absolutePath() );
}


void FileBrowser::on_playBtnClicked()
{
	INFOLOG( "[on_playBtnClicked]" );
}
