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

#include <memory>

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

	HydrogenApp::get_instance()->addEventListener( this );

	m_pProgressBar->setValue( 0 );
	srComboBox->setCurrentIndex(1);
	sdComboBox->setCurrentIndex(1);
	m_pSamplerateLbl->setText( trUtf8( "Sample rate: %1" ).arg( sdComboBox->currentText() ) );

	QString defaultFilename( Hydrogen::get_instance()->getSong()->get_filename() );
	if( Hydrogen::get_instance()->getSong()->get_filename().isEmpty() )
		defaultFilename = Hydrogen::get_instance()->getSong()->__name;
	defaultFilename.replace( '*', "_" );
	defaultFilename.replace( ".h2song", "" );	
	defaultFilename += ".wav";
	exportNameTxt->setText(defaultFilename);
	b_QfileDialog = false;

}



ExportSongDialog::~ExportSongDialog()
{
	HydrogenApp::get_instance()->removeEventListener( this );
}



/// \todo: memorizzare l'ultima directory usata
void ExportSongDialog::on_browseBtn_clicked()
{
//	static QString lastUsedDir = "";
	static QString lastUsedDir = QDir::homePath();


	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode(QFileDialog::AnyFile);
	//fd->setFilter( trUtf8("Wave file (*.wav)") );
	fd->setNameFilters( QStringList() << "Microsoft WAV (*.wav *.WAv)"
         				  << "Apple AIFF (*.aiff *.AIFF)"
         				  << "Lossless  Flac (*.flac *.FLAC)"
					  << "Compressed Ogg (*.ogg *.OGG)" );
         				 // << "Any files (*)");

	fd->setDirectory( lastUsedDir );
	fd->setAcceptMode( QFileDialog::AcceptSave );
	fd->setWindowTitle( trUtf8( "Export song" ) );
//	fd->setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	QString defaultFilename = exportNameTxt->text();

	fd->selectFile(defaultFilename);

	QString filename = "";
	if (fd->exec()) {
		filename = fd->selectedFiles().first();
		b_QfileDialog = true;
	}

	if ( ! filename.isEmpty() ) {
		lastUsedDir = fd->directory().absolutePath();
		QString sNewFilename = filename;
//		if ( sNewFilename.endsWith( ".wav" ) == false ) {
//			filename += ".wav";
//		}

		exportNameTxt->setText(filename);
	}

	if( filename.endsWith( ".ogg" ) || filename.endsWith( ".OGG" ) ){
		srComboBox->hide();
		sdComboBox->hide();
		label->hide();
		label_2->hide();
	}
		
}



void ExportSongDialog::on_okBtn_clicked()
{
	if (m_bExporting) {
		return;
	}

	QString filename = exportNameTxt->text();
	if ( QFile( filename ).exists() == true && b_QfileDialog == false ) {
		int res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(filename), tr("&Ok"), tr("&Cancel"), 0, 1 );
		if (res == 1 ) return;
	}
	m_bExporting = true;
	
	Hydrogen::get_instance()->startExportSong( filename, srComboBox->currentText().toInt(), sdComboBox->currentText().toInt() );
}



void ExportSongDialog::on_closeBtn_clicked()
{
	Hydrogen::get_instance()->stopExportSong();
	m_bExporting = false;
	accept();

}


void ExportSongDialog::on_exportNameTxt_textChanged( const QString& )
{
	QString filename = exportNameTxt->text();
	if ( ! filename.isEmpty() ) {
		okBtn->setEnabled(true);
	}
	else {
		okBtn->setEnabled(false);
	}

	if( filename.endsWith( ".ogg" ) || filename.endsWith( ".OGG" ) ){
		srComboBox->hide();
		sdComboBox->hide();		
	}else
	{
		srComboBox->show();
		sdComboBox->show();
		label->show();
		label_2->show();
	}
}


void ExportSongDialog::progressEvent( int nValue )
{
        m_pProgressBar->setValue( nValue );
	if ( nValue == 100 ) {
	  	//INFOLOG("SONO A 100");
		
//		Hydrogen::get_instance()->stopExportSong();
		m_bExporting = false;
		QFile check( exportNameTxt->text() );
		if ( ! check.exists() ) {
			QMessageBox::information( this, "Hydrogen", trUtf8("Export failed!") );
		}
		accept();
	}
}
