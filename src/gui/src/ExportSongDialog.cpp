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

#include <hydrogen/basics/note.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/sampler/Sampler.h>
#include <hydrogen/event_queue.h>

#include <memory>

using namespace H2Core;

const char* ExportSongDialog::__class_name = "ExportSongDialog";

ExportSongDialog::ExportSongDialog(QWidget* parent)
 : QDialog(parent)
 , Object( __class_name )
 , m_bExporting( false )
{
	setupUi( this );
	setModal( true );
	setWindowTitle( trUtf8( "Export song" ) );

        exportTypeCombo->addItem(trUtf8("Export to a single track"));
        exportTypeCombo->addItem(trUtf8("Export to seperate tracks"));
        exportTypeCombo->addItem(trUtf8("Both"));

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
	m_bExportTrackouts = false;
        m_nInstrument = 0;
        m_sExtension = ".wav";
        m_bOverwriteFiles = false;

        // use of rubberband batch
        if(checkUseOfRubberband()){
                b_oldRubberbandBatchMode = Preferences::get_instance()->getRubberBandBatchMode();
                toggleRubberbandCheckBox->setChecked(Preferences::get_instance()->getRubberBandBatchMode());
                connect(toggleRubberbandCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleRubberbandBatchMode( bool )));
        }else
        {
                b_oldRubberbandBatchMode = Preferences::get_instance()->getRubberBandBatchMode();
                toggleRubberbandCheckBox->setEnabled( false );
        }


        // use of timeline
        if( Hydrogen::get_instance()->m_timelinevector.size() > 0 ){
                toggleTimeLineBPMCheckBox->setChecked(Preferences::get_instance()->getUseTimelineBpm());
                b_oldTimeLineBPMMode = Preferences::get_instance()->getUseTimelineBpm();
                connect(toggleTimeLineBPMCheckBox, SIGNAL(toggled(bool)), this, SLOT(togglTimeLineBPMMode( bool )));
        }else
        {
                b_oldTimeLineBPMMode = Preferences::get_instance()->getUseTimelineBpm();
                toggleTimeLineBPMCheckBox->setEnabled( false );
        }


        // use of interpolation mode
        m_oldInterpolation = AudioEngine::get_instance()->get_sampler()->getInterpolateMode();
        resampleComboBox->setCurrentIndex( m_oldInterpolation );
        connect(resampleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(resampleComboBoIndexChanged(int)));

        // if rubberbandBatch calculate time needed by lib rubberband to resample samples
        if(b_oldRubberbandBatchMode){
                calculateRubberbandTime();
        }
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

                //this second extension check is mostly important if you leave a dot
                //without a regular extionsion in a filename
                if( !filename.endsWith( m_sExtension ) ){
                    filename.append(m_sExtension);
                }

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
        if ( m_bExporting ) {
		return;
        }

        m_bOverwriteFiles = false;

	/* If the song has a tempo change, notify the user 
	 *  that hydrogen is unable export songs with 
	 *  tempo changes correctly 
	 **/ 
	Hydrogen* engine = Hydrogen::get_instance(); 
	
	bool warn =  Preferences::get_instance()->getShowExportWarning();
	
	std::vector<Hydrogen::HTimelineVector> timelineVector = engine->m_timelinevector; 
/*	if( timelineVector.size() > 0 ){
		int res = QMessageBox::information( this, "Hydrogen", tr( "This version of hydrogen is not able to export songs with tempo changes. If you proceed, the song will be exported without tempo changes."), QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll); 
		if (res == QMessageBox::No ) return; 
		if (res == QMessageBox::YesToAll ) Preferences::get_instance()->setShowExportWarning(true);
	} 
*/
        /* 0: Export to single track
        *  1: Export to multiple tracks
        *  2: Export to both
        */

        if( exportTypeCombo->currentIndex() == 0 || exportTypeCombo->currentIndex() == 2 ){
                m_bExportTrackouts = false;

                QString filename = exportNameTxt->text();
                if ( QFile( filename ).exists() == true && b_QfileDialog == false ) {

                    int res;
                    if( exportTypeCombo->currentIndex() == 0 ){
                        res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(filename), QMessageBox::Yes | QMessageBox::No );
                    } else {
                        res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(filename), QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll);
                    }

                    if (res == QMessageBox::YesToAll ) m_bOverwriteFiles = true;
                    if (res == QMessageBox::No ) return;

                }

                if( exportTypeCombo->currentIndex() == 2 ) m_bExportTrackouts = true;
                Hydrogen::get_instance()->startExportSong( filename, sampleRateCombo->currentText().toInt(), sampleDepthCombo->currentText().toInt() );

                return;
        }

        if( exportTypeCombo->currentIndex() == 1 ){
                m_bExportTrackouts = true;
                exportTracks();
                return;
	}

}

void ExportSongDialog::exportTracks()
{
        Song *pSong = Hydrogen::get_instance()->getSong();
        if( m_nInstrument <= pSong->get_instrument_list()->size() -1 ){

                bool instrumentexists = false;
                //if a instrument contains no notes we jump to the next instrument
                unsigned nPatterns = pSong->get_pattern_list()->size();
                for ( unsigned i = 0; i < nPatterns; i++ ) {
                        Pattern *pat = pSong->get_pattern_list()->get( i );
                        const Pattern::notes_t* notes = pat->get_notes();
                        FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
                                Note *pNote = it->second;
                                assert( pNote );

                                if( pNote->get_instrument()->get_name() == Hydrogen::get_instance()->getSong()->get_instrument_list()->get(m_nInstrument)->get_name() ){
                                        instrumentexists = true;
                                        break;
                                }

                        }
                }

                if( !instrumentexists ){
                        if( m_nInstrument == Hydrogen::get_instance()->getSong()->get_instrument_list()->size() -1 ){
                                m_bExportTrackouts = false;
                                HydrogenApp::get_instance()->getMixer()->unmuteAll( true );
                                m_nInstrument = 0;
                                return;
                        }else {
                                m_nInstrument++;
                                exportTracks();
                                return;
                        }
                }

                QStringList filenameList =  exportNameTxt->text().split( m_sExtension );

                QString firstItem;
                if( !filenameList.isEmpty() ){
                        firstItem = filenameList.first();
                }
                QString newItem =  firstItem + "-" + Hydrogen::get_instance()->getSong()->get_instrument_list()->get(m_nInstrument)->get_name();

                QString filename =  newItem.append(m_sExtension);

                if ( QFile( filename ).exists() == true && b_QfileDialog == false && !m_bOverwriteFiles) {
                        int res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(filename), QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll );
                        if (res == QMessageBox::No ) return;
                        if (res == QMessageBox::YesToAll ) m_bOverwriteFiles = true;
                }

                //Hydrogen::get_instance()->stopExportSong();
                Hydrogen::get_instance()->stopTempExportSong();
                m_bExporting = false;
                HydrogenApp::get_instance()->getMixer()->soloClicked( m_nInstrument );

                Hydrogen::get_instance()->startExportSong( filename, sampleRateCombo->currentText().toInt(), sampleDepthCombo->currentText().toInt() );

                if(! (m_nInstrument == Hydrogen::get_instance()->getSong()->get_instrument_list()->size() -1 )){
                        m_nInstrument++;
                }
        }

}

void ExportSongDialog::on_closeBtn_clicked()
{
	Hydrogen::get_instance()->stopExportSong();
	m_bExporting = false;
        if(Preferences::get_instance()->getRubberBandBatchMode()){
                EventQueue::get_instance()->push_event( EVENT_RECALCULATERUBBERBAND, -1);
        }
        Preferences::get_instance()->setRubberBandBatchMode( b_oldRubberbandBatchMode );
        Preferences::get_instance()->setUseTimelineBpm( b_oldTimeLineBPMMode );
        setResamplerMode(m_oldInterpolation);
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

	filename = exportNameTxt->text();
	splitty = filename.split(".");
	splitty.removeLast();
	filename = splitty.join( "." );

	switch ( index ) {
	case 0:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 1 ); //44100hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename += ".wav";
                m_sExtension = ".wav";
		break;
	case 1:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename += ".wav";
                m_sExtension = ".wav";
		break;
	case 2:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 2 ); //24bit
		filename += ".wav";
                m_sExtension = ".wav";
		break;
	case 3:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 0 ); //22050hz
		sampleDepthCombo->setCurrentIndex ( 0 ); //8bit
		filename += ".wav";
                m_sExtension = ".wav";
		break;
	case 4:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 3 ); //96000hz
		sampleDepthCombo->setCurrentIndex ( 3 ); //32bit
		filename += ".wav";
                m_sExtension = ".wav";
		break;
	case 5:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 1 ); //44100hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename += ".aiff";
                m_sExtension = ".aiff";
		break;
	case 6:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename += ".aiff";
                m_sExtension = ".aiff";
		break;
	case 7:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 2 ); //24bit
		filename += ".aiff";
                m_sExtension = ".aiff";
		break;
	case 8:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 2 ); //48000hz
		sampleDepthCombo->setCurrentIndex ( 2 ); //24bit
		filename += ".flac";
                m_sExtension = ".flac";
		break;
	case 9:
		sampleRateCombo->hide();
		sampleDepthCombo->hide();
		label->hide();
		label_2->hide();
		filename += ".ogg";
                m_sExtension = ".ogg";
		break;

	default:
		sampleRateCombo->show();
		sampleDepthCombo->show();
		sampleRateCombo->setCurrentIndex ( 1 ); //44100hz
		sampleDepthCombo->setCurrentIndex ( 1 ); //16bit
		filename += ".wav";
                m_sExtension = ".wav";
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

                if( m_nInstrument == Hydrogen::get_instance()->getSong()->get_instrument_list()->size() -1 ){
                        HydrogenApp::get_instance()->getMixer()->unmuteAll( false );
                        m_nInstrument = 0;
                        m_bExportTrackouts = false;

                }

                if( m_bExportTrackouts ){
                        exportTracks();
                }
        }

        if ( nValue < 100 ) {
                closeBtn->setEnabled(false);
                resampleComboBox->setEnabled(false);

        }else
        {
                closeBtn->setEnabled(true);
                resampleComboBox->setEnabled(true);
        }
}

void ExportSongDialog::toggleRubberbandBatchMode(bool toggled)
{
        Preferences::get_instance()->setRubberBandBatchMode(toggled);
        if(toggled){
                calculateRubberbandTime();
        }
}

void ExportSongDialog::togglTimeLineBPMMode(bool toggled)
{
        Preferences::get_instance()->setUseTimelineBpm(toggled);
}

void ExportSongDialog::resampleComboBoIndexChanged(int index )
{
        setResamplerMode(index);
}

void ExportSongDialog::setResamplerMode(int index)
{
        switch ( index ){
        case 0:
               AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::LINEAR );
               break;
        case 1:
               AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::COSINE );
               break;
        case 2:
               AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::THIRD );
               break;
        case 3:
               AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::CUBIC );
               break;
        case 4:
               AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::HERMITE );
               break;
        }
}


void ExportSongDialog::calculateRubberbandTime()
{
        QApplication::setOverrideCursor(Qt::WaitCursor);
        closeBtn->setEnabled(false);
        resampleComboBox->setEnabled(false);
        okBtn->setEnabled(false);
        Hydrogen* engine = Hydrogen::get_instance();
        float oldBPM = engine->getSong()->__bpm;
        float lowBPM = oldBPM;

        if( engine->m_timelinevector.size() >= 1 ){
                for ( int t = 0; t < engine->m_timelinevector.size(); t++){
                        if(engine->m_timelinevector[t].m_htimelinebpm < lowBPM){
                                lowBPM =  engine->m_timelinevector[t].m_htimelinebpm;
                        }

                }
        }

        engine->setBPM(lowBPM);
        time_t sTime = time(NULL);
        Hydrogen *pEngine = Hydrogen::get_instance();
        Song *song = pEngine->getSong();
        assert(song);
        if(song){
                InstrumentList *songInstrList = song->get_instrument_list();
                assert(songInstrList);
                for ( unsigned nInstr = 0; nInstr < songInstrList->size(); ++nInstr ) {
                        Instrument *pInstr = songInstrList->get( nInstr );
                        assert( pInstr );
                        if ( pInstr ){
                                for ( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
                                        InstrumentLayer *pLayer = pInstr->get_layer( nLayer );
                                        if ( pLayer ) {
                                                Sample *pSample = pLayer->get_sample();
                                                if ( pSample ) {
                            if( pSample->get_rubberband().use ) {
                                Sample *newSample = Sample::load(
                                        pSample->get_filepath(),
                                        pSample->get_loops(),
                                        pSample->get_rubberband(),
                                        *pSample->get_velocity_envelope(),
                                        *pSample->get_pan_envelope()
                                        );
                                                                if( !newSample  ){
                                                                        continue;
                                                                }
                                                                delete pSample;
                                                                // insert new sample from newInstrument
                                                                AudioEngine::get_instance()->lock( RIGHT_HERE );
                                                                pLayer->set_sample( newSample );
                                                                AudioEngine::get_instance()->unlock();

                                                        }
                                                }
                                        }
                                }
                        }
                }
        }
        Preferences::get_instance()->setRubberBandCalcTime(time(NULL) - sTime);
        engine->setBPM(oldBPM);
        closeBtn->setEnabled(true);
        resampleComboBox->setEnabled(true);
        okBtn->setEnabled(true);
        QApplication::restoreOverrideCursor();

}

bool ExportSongDialog::checkUseOfRubberband()
{
        Hydrogen *pEngine = Hydrogen::get_instance();
        Song *song = pEngine->getSong();
        assert(song);
        if(song){
                InstrumentList *songInstrList = song->get_instrument_list();
                assert(songInstrList);
                for ( unsigned nInstr = 0; nInstr < songInstrList->size(); ++nInstr ) {
                        Instrument *pInstr = songInstrList->get( nInstr );
                        assert( pInstr );
                        if ( pInstr ){
                                for ( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
                                        InstrumentLayer *pLayer = pInstr->get_layer( nLayer );
                                        if ( pLayer ) {
                                                Sample *pSample = pLayer->get_sample();
                                                if ( pSample ) {
                            if( pSample->get_rubberband().use ) {
                                    return true;

                                                        }
                                                }
                                        }
                                }
                        }
                }
        }
        return false;
}
