/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <QFileDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPixmap>

#include "ExportSongDialog.h"
#include "Skin.h"
#include "HydrogenApp.h"
#include <hydrogen/Song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/IO/AudioOutput.h>

using namespace H2Core;

ExportSongDialog::ExportSongDialog(QWidget* parent)
 : QDialog(parent)
 , Object( "ExportSongDialog" )
 , m_bExporting( false )
{
	setupUi( this );
	setModal( true );
	setWindowTitle( trUtf8( "Export song" ) );
//	setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	HydrogenApp::getInstance()->addEventListener( this );

	m_pSamplerateLbl->setText( trUtf8( "Sample rate: %1" ).arg( Hydrogen::getInstance()->getAudioOutput()->getSampleRate() ) );
	m_pProgressBar->setValue( 0 );
}



ExportSongDialog::~ExportSongDialog()
{
	HydrogenApp::getInstance()->removeEventListener( this );
}



/// \todo: memorizzare l'ultima directory usata
void ExportSongDialog::on_browseBtn_clicked()
{
//	static QString lastUsedDir = "";
	static QString lastUsedDir = QDir::homePath();

	QFileDialog *fd = new QFileDialog(this);
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setFilter( trUtf8("Wave file (*.wav)") );
	fd->setDirectory( lastUsedDir );
	fd->setAcceptMode( QFileDialog::AcceptSave );
	fd->setWindowTitle( trUtf8( "Export song" ) );
//	fd->setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	QString defaultFilename( Hydrogen::getInstance()->getSong()->m_sName.c_str() );
	defaultFilename.replace( '*', "_" );
	defaultFilename += ".wav";

	fd->selectFile(defaultFilename);

	QString filename = "";
	if (fd->exec()) {
		filename = fd->selectedFiles().first();
	}

	if (filename != "") {
		lastUsedDir = fd->directory().absolutePath();
		QString sNewFilename = filename;
		if ( sNewFilename.endsWith( ".wav" ) == false ) {
			filename += ".wav";
		}

		exportNameTxt->setText(filename);
	}
}



void ExportSongDialog::on_okBtn_clicked()
{
	if (m_bExporting) {
		return;
	}

	QString filename = exportNameTxt->text();
	m_bExporting = true;
	Hydrogen::getInstance()->startExportSong( filename.toStdString() );
}



void ExportSongDialog::on_closeBtn_clicked()
{
	Hydrogen::getInstance()->stopExportSong();
	accept();
	m_bExporting = false;
}


void ExportSongDialog::on_exportNameTxt_textChanged( const QString& )
{
	QString filename = exportNameTxt->text();
	if (filename != "") {
		okBtn->setEnabled(true);
	}
	else {
		okBtn->setEnabled(false);
	}
}


void ExportSongDialog::progressEvent( int nValue )
{
        m_pProgressBar->setValue( nValue );
	if ( nValue == 100 ) {
	  //INFOLOG("SONO A 100");
	  accept();
	}
}
