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
 * $Id: ExportSongDialog.cpp,v 1.12 2005/05/24 12:57:56 comix Exp $
 *
 */

#include <qprogressbar.h>
#include <qlabel.h>

#include "config.h"
#include "ExportSongDialog.h"
#include "Skin.h"
#include "HydrogenApp.h"
#include "lib/Song.h"
#include "lib/Hydrogen.h"
#include "lib/drivers/GenericDriver.h"


ExportSongDialog::ExportSongDialog(QWidget* parent)
 : ExportSongDialog_UI(parent, 0, true)
 , Object( "ExportSongDialog" )
 , m_bExporting( false )
{
	setCaption( trUtf8( "Export song" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	HydrogenApp::getInstance()->addEventListener( this );

	m_pSamplerateLbl->setText( trUtf8( "Sample rate: %1" ).arg( Hydrogen::getInstance()->getAudioDriver()->getSampleRate() ) );
}



ExportSongDialog::~ExportSongDialog()
{
	HydrogenApp::getInstance()->removeEventListener( this );
}



/// \todo: memorizzare l'ultima directory usata
void ExportSongDialog::browseBtnClicked()
{
	static QString lastUsedDir = "";

	QFileDialog *fd = new QFileDialog(this, "File Dialog", TRUE);
	fd->setMode(QFileDialog::AnyFile);
	fd->setFilter( trUtf8("Wave file (*.wav)") );
	fd->setDir( lastUsedDir );

	fd->setCaption( trUtf8( "Export song" ) );
	fd->setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	QString defaultFilename( Hydrogen::getInstance()->getSong()->m_sName.c_str() );
	defaultFilename.replace( '*', "_" );
	defaultFilename += ".wav";

	fd->setSelection(defaultFilename);

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFile();
	}

	if (filename != "") {
		lastUsedDir = fd->dirPath();
		QString sNewFilename = filename;
		if ( sNewFilename.endsWith( ".wav" ) == false ) {
			filename += ".wav";
		}

		exportNameTxt->setText(filename);
	}
}



void ExportSongDialog::okBtnClicked()
{
	if (m_bExporting) {
		return;
	}

	QString filename = exportNameTxt->text();
	m_bExporting = true;
	Hydrogen::getInstance()->startExportSong( filename.ascii() );
}



void ExportSongDialog::closeBtnClicked()
{
	Hydrogen::getInstance()->stopExportSong();
	accept();
	m_bExporting = false;
}


void ExportSongDialog::filenameTxtChanged()
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
	m_pProgressBar->setProgress( nValue );
}

