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
#include "Mixer/Mixer.h"

#include <hydrogen/note.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/instrument.h>
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
	sampleRateCombo->setCurrentIndex(1);
	sampleDepthCombo->setCurrentIndex(1);

	QString defaultFilename( Hydrogen::get_instance()->getSong()->get_filename() );
	if( Hydrogen::get_instance()->getSong()->get_filename().isEmpty() )
		defaultFilename = Hydrogen::get_instance()->getSong()->__name;
	defaultFilename.replace( '*', "_" );
	defaultFilename.replace( ".h2song", "" );	
	defaultFilename += ".wav";
	exportNameTxt->setText(defaultFilename);
	b_QfileDialog = false;
	m_bExportTrackOuts = true;
	m_bTrackoutIsExporting = false;
	m_ninstrument = 0;

}



ExportSongDialog::~ExportSongDialog()
{
	HydrogenApp::get_instance()->removeEventListener( this );
}



/// \todo: memorizzare l'ultima directory usata
void ExportSongDialog::on_browseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();


	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode(QFileDialog::AnyFile);


	if( templateCombo->currentIndex() <= 4 ) fd->setNameFilter("Microsoft WAV (*.wav *.WAV)");
	if( templateCombo->currentIndex() > 4 && templateCombo->currentIndex() < 8  ) fd->setNameFilter( "Apple AIFF (*.aiff *.AIFF)");
	if( templateCombo->currentIndex() == 8) fd->setNameFilter( "Lossless  Flac (*.flac *.FLAC)");
	if( templateCombo->currentIndex() == 9) fd->setNameFilter( "Compressed Ogg (*.ogg *.OGG)");

	fd->setDirectory( lastUsedDir );
	fd->setAcceptMode( QFileDialog::AcceptSave );
	fd->setWindowTitle( trUtf8( "Export song" ) );


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
		exportNameTxt->setText(filename);
	}

	if( filename.endsWith( ".ogg" ) || filename.endsWith( ".OGG" ) ){
		sampleRateCombo->hide();
		sampleDepthCombo->hide();
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

	m_bExporting = tocheckBox->isChecked();//only for testing
	if( m_bExporting ){
		m_bTrackoutIsExporting = true;
	}
	Hydrogen::get_instance()->startExportSong( filename, sampleRateCombo->currentText().toInt(), sampleDepthCombo->currentText().toInt() );

}

void ExportSongDialog::exportTracks()
{
	Song *pSong = Hydrogen::get_instance()->getSong();
	if( m_ninstrument <= pSong->get_instrument_list()->get_size() -1 ){

		bool instrumentexists = false;
		//if a instrument contains no notes we jump to the next instrument
		unsigned nPatterns = pSong->get_pattern_list()->get_size();
		for ( unsigned i = 0; i < nPatterns; i++ ) {
			Pattern *pat = pSong->get_pattern_list()->get( i );

			std::multimap <int, Note*>::iterator pos;
			for ( pos = pat->note_map.begin(); pos != pat->note_map.end(); ++pos ) {
				Note *pNote = pos->second;
				assert( pNote );
				if( pNote->get_instrument()->get_name() == Hydrogen::get_instance()->getSong()->get_instrument_list()->get(m_ninstrument)->get_name() ){
					instrumentexists = true;
					break;
				}
			}
		}

		if( !instrumentexists ){
			if( m_ninstrument == Hydrogen::get_instance()->getSong()->get_instrument_list()->get_size() -1 ){
				m_bTrackoutIsExporting = false;//
				HydrogenApp::get_instance()->getMixer()->soloClicked( m_ninstrument );//solo instrument. this will disable all other instrument-solos
				HydrogenApp::get_instance()->getMixer()->soloClicked( m_ninstrument );//unsolo this instrument because exporting is finished
				m_ninstrument = 0;
				return;
			}else
			{
				m_ninstrument++;
				exportTracks();
			return;
			}
		}
		
		QStringList filenamelist =  exportNameTxt->text().split(".");
		
		QString firstitem = "";
		if( !filenamelist.isEmpty() ){
			firstitem = filenamelist.first();
		}
		QString newitem =  firstitem + "-" + Hydrogen::get_instance()->getSong()->get_instrument_list()->get(m_ninstrument)->get_name();
		int listsize = filenamelist.size();
		filenamelist.replace ( listsize -2, newitem );
		QString filename = filenamelist.join(".");
		
		if ( QFile( filename ).exists() == true && b_QfileDialog == false ) {
			int res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(filename), tr("&Ok"), tr("&Cancel"), 0, 1 );
			if (res == 1 ) return;
		}

		Hydrogen::get_instance()->stopExportSong();
		m_bExporting = false;
		HydrogenApp::get_instance()->getMixer()->soloClicked( m_ninstrument );
		Hydrogen::get_instance()->startExportSong( filename, sampleRateCombo->currentText().toInt(), sampleDepthCombo->currentText().toInt() );
		if( m_ninstrument == Hydrogen::get_instance()->getSong()->get_instrument_list()->get_size() -1 ){
			m_bTrackoutIsExporting = false;//
			HydrogenApp::get_instance()->getMixer()->soloClicked( m_ninstrument );
			m_ninstrument = 0;
		}else
		{
			m_ninstrument++;
		}
	}
}

void ExportSongDialog::on_closeBtn_clicked()
{
	Hydrogen::get_instance()->stopExportSong();
	m_bExporting = false;
	accept();

}


void ExportSongDialog::on_templateCombo_currentIndexChanged(int index )
{
	/**index
	 * 0 = wav 44100 | 16
	 * 1 = wav 48000 | 16
	 * 2 = wav 48000 | 24
	 * 3 = wav 22050 | 8
	 * 4 = wav 96000 | 32
	 * 5 = aiff 44100 | 16
	 * 6 = aiff 48000 | 16
	 * 7 = aiff 48000 | 24
	 * 8 = flac 48000 
	 * 9 = ogg VBR , disable comboboxes
	 **/

	QString filename;
	QStringList splitty;

	switch ( index ) {
	case 0:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 1 ); //44100hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".wav";
		break;
	case 1:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".wav";
		break;
	case 2:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 2 ); //24bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".wav";
		break;
	case 3:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 0 ); //22050hz
		sampleDepthCombo->setCurrentIndex ( 0 ); //8bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".wav";
		break;
	case 4:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 3 ); //96000hz
		sampleDepthCombo->setCurrentIndex ( 3 ); //32bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".wav";
		break;
	case 5:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 1 ); //44100hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".aiff";
		break;
	case 6:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".aiff";
		break;
	case 7:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 2 ); //24bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".aiff";
		break;
	case 8:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 2 ); //24bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".flac";
		break;
	case 9:
		sampleRateCombo->hide();
		sampleDepthCombo->hide();
		label->hide();
		label_2->hide();
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".ogg";
		break;

	default:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 1 ); //44100hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename = exportNameTxt->text();
		splitty = filename.split(".");
		splitty.removeLast();
		filename = splitty.join( "." );
		filename += ".wav";
	}

	exportNameTxt->setText(filename);


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
		templateCombo->setCurrentIndex( 9 );//ogg
	}
	else if( filename.endsWith( ".flac" ) || filename.endsWith( ".FLAC" ) ){
		label->show();
		label_2->show();
		templateCombo->setCurrentIndex( 8 );//flac
	}
	else if( filename.endsWith( ".aiff" ) || filename.endsWith( ".AIFF" ) ){
		label->show();
		label_2->show();
		templateCombo->setCurrentIndex( 5 );//aiff
	}
	else if( filename.endsWith( ".wav" ) || filename.endsWith( ".WAV" ) ){
		label->show();
		label_2->show();
		templateCombo->setCurrentIndex( 0 );//wav
	}
}


void ExportSongDialog::progressEvent( int nValue )
{
        m_pProgressBar->setValue( nValue );
	if ( nValue == 100 ) {
		m_bExporting = false;
		QFile check( exportNameTxt->text() );
		if ( ! check.exists() ) {
			QMessageBox::information( this, "Hydrogen", trUtf8("Export failed!") );
		}
		if( m_bExportTrackOuts && m_bTrackoutIsExporting ){
			exportTracks();
		}
	}
}
