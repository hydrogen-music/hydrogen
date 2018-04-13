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

#include "ExportSongDialog.h"
#include "Skin.h"
#include "HydrogenApp.h"
#include "Mixer/Mixer.h"

#include <hydrogen/basics/note.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/timeline.h>
#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/sampler/Sampler.h>
#include <hydrogen/event_queue.h>

#include <memory>

#ifdef WIN32
#include <time.h>
#endif

using namespace H2Core;

const char* ExportSongDialog::__class_name = "ExportSongDialog";

enum ExportModes { EXPORT_TO_SINGLE_TRACK, EXPORT_TO_SEPARATE_TRACKS, EXPORT_TO_BOTH};

ExportSongDialog::ExportSongDialog(QWidget* parent)
	: QDialog(parent)
	, Object( __class_name )
	, m_bExporting( false )
{
	setupUi( this );
	setModal( true );
	setWindowTitle( trUtf8( "Export song" ) );

	exportTypeCombo->addItem(trUtf8("Export to a single track"));
	exportTypeCombo->addItem(trUtf8("Export to separate tracks"));
	exportTypeCombo->addItem(trUtf8("Both"));

	HydrogenApp::get_instance()->addEventListener( this );
	Hydrogen * pHydrogen = Hydrogen::get_instance();
	Preferences *pPref = Preferences::get_instance();

	m_pProgressBar->setValue( 0 );
	sampleRateCombo->setCurrentIndex(1);
	sampleDepthCombo->setCurrentIndex(1);

	QString defaultFilename( pHydrogen->getSong()->get_filename() );
	
	if( pHydrogen->getSong()->get_filename().isEmpty() ){
		defaultFilename = pHydrogen->getSong()->__name;
	}
	
	defaultFilename.replace( '*', "_" );
	defaultFilename.replace( Filesystem::song_ext, "" );
	defaultFilename += ".wav";
	
	exportNameTxt->setText(defaultFilename);
	b_QfileDialog = false;
	m_bExportTrackouts = false;
	m_nInstrument = 0;
	m_sExtension = ".wav";
	m_bOverwriteFiles = false;

	// use of rubberband batch
	if(checkUseOfRubberband()){
		b_oldRubberbandBatchMode = pPref->getRubberBandBatchMode();
		toggleRubberbandCheckBox->setChecked(pPref->getRubberBandBatchMode());
		connect(toggleRubberbandCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleRubberbandBatchMode( bool )));
	}else
	{
		b_oldRubberbandBatchMode = pPref->getRubberBandBatchMode();
		toggleRubberbandCheckBox->setEnabled( false );
	}


	// use of timeline
	if( pHydrogen->getTimeline()->m_timelinevector.size() > 0 ){
		toggleTimeLineBPMCheckBox->setChecked(pPref->getUseTimelineBpm());
		b_oldTimeLineBPMMode = pPref->getUseTimelineBpm();
		connect(toggleTimeLineBPMCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleTimeLineBPMMode( bool )));
	}else
	{
		b_oldTimeLineBPMMode = pPref->getUseTimelineBpm();
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

	// Have the dialog find the best size
	adjustSize();
}



ExportSongDialog::~ExportSongDialog()
{
	HydrogenApp::get_instance()->removeEventListener( this );
}


void ExportSongDialog::on_browseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();


	QFileDialog fd(this);
	fd.setFileMode(QFileDialog::AnyFile);


	if( templateCombo->currentIndex() <= 4 ) fd.setNameFilter("Microsoft WAV (*.wav *.WAV)");
	if( templateCombo->currentIndex() > 4 && templateCombo->currentIndex() < 8  ) fd.setNameFilter( "Apple AIFF (*.aiff *.AIFF)");
	if( templateCombo->currentIndex() == 8) fd.setNameFilter( "Lossless  Flac (*.flac *.FLAC)");
	if( templateCombo->currentIndex() == 9) fd.setNameFilter( "Compressed Ogg (*.ogg *.OGG)");

	fd.setDirectory( lastUsedDir );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setWindowTitle( trUtf8( "Export song" ) );


	QString defaultFilename = exportNameTxt->text();

	fd.selectFile(defaultFilename);

	QString filename = "";
	if (fd.exec()) {
		filename = fd.selectedFiles().first();
		b_QfileDialog = true;
	}

	if ( ! filename.isEmpty() ) {
		lastUsedDir = fd.directory().absolutePath();

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
		sampleRateLable->hide();
		sampleDepthLable->hide();
	}

}



void ExportSongDialog::on_okBtn_clicked()
{
	if ( m_bExporting ) {
		return;
	}
	
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrumentList = pSong->get_instrument_list();

	m_bOverwriteFiles = false;

	if( exportTypeCombo->currentIndex() == EXPORT_TO_SINGLE_TRACK || exportTypeCombo->currentIndex() == EXPORT_TO_BOTH ){
		m_bExportTrackouts = false;

		QString filename = exportNameTxt->text();
		if ( QFile( filename ).exists() == true && b_QfileDialog == false ) {

			int res;
			if( exportTypeCombo->currentIndex() == EXPORT_TO_SINGLE_TRACK ){
				res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(filename), QMessageBox::Yes | QMessageBox::No );
			} else {
				res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(filename), QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll);
			}

			if (res == QMessageBox::YesToAll ){
				m_bOverwriteFiles = true;
			}
			
			if (res == QMessageBox::No ) {
				return;
			}
		}

		if( exportTypeCombo->currentIndex() == EXPORT_TO_BOTH ){
			m_bExportTrackouts = true;
		}
		
		/* arm all tracks for export */
		for (auto i = 0; i < pInstrumentList->size(); i++) {
			pInstrumentList->get(i)->set_currently_exported( true );
		}
		
		pEngine->startExportSession( sampleRateCombo->currentText().toInt(), sampleDepthCombo->currentText().toInt());
		pEngine->startExportSong( filename );

		return;
	}

	if( exportTypeCombo->currentIndex() == EXPORT_TO_SEPARATE_TRACKS ){
		m_bExportTrackouts = true;
		pEngine->startExportSession(sampleRateCombo->currentText().toInt(), sampleDepthCombo->currentText().toInt());
		exportTracks();
		return;
	}

}

bool ExportSongDialog::currentInstrumentHasNotes()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	unsigned nPatterns = pSong->get_pattern_list()->size();
	
	bool bInstrumentHasNotes = false;
	
	for ( unsigned i = 0; i < nPatterns; i++ ) {
		Pattern *pPattern = pSong->get_pattern_list()->get( i );
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );

			if( pNote->get_instrument()->get_id() == pSong->get_instrument_list()->get(m_nInstrument)->get_id() ){
				bInstrumentHasNotes = true;
				break;
			}
		}
	}
	
	return bInstrumentHasNotes;
}

QString ExportSongDialog::findUniqueExportFilenameForInstrument(Instrument* pInstrument)
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	QString uniqueInstrumentName;
	
	int instrumentOccurence = 0;
	for(int i=0; i  < pSong->get_instrument_list()->size(); i++ ){
		if( pSong->get_instrument_list()->get(m_nInstrument)->get_name() == pInstrument->get_name()){
			instrumentOccurence++;
		}
	}
	
	if(instrumentOccurence >= 2){
		uniqueInstrumentName = pInstrument->get_name() + QString("_") + QString::number( pInstrument->get_id() );
	} else {
		uniqueInstrumentName = pInstrument->get_name();
	}
	
	return uniqueInstrumentName;
}

void ExportSongDialog::exportTracks()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrumentList = pSong->get_instrument_list();
	
	if( m_nInstrument < pInstrumentList->size() ){
		
		//if a instrument contains no notes we jump to the next instrument
		bool bInstrumentHasNotes = currentInstrumentHasNotes();

		if( !bInstrumentHasNotes ){
			if( m_nInstrument == pInstrumentList->size() -1 ){
				m_bExportTrackouts = false;
				m_nInstrument = 0;
				return;
			} else {
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
		QString newItem = firstItem + "-" + findUniqueExportFilenameForInstrument( pInstrumentList->get( m_nInstrument ) );

		QString filename = newItem.append( m_sExtension );

		if ( QFile( filename ).exists() == true && b_QfileDialog == false && !m_bOverwriteFiles) {
			int res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(filename), QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll );
			if (res == QMessageBox::No ) return;
			if (res == QMessageBox::YesToAll ) m_bOverwriteFiles = true;
		}
		
		if( m_nInstrument > 0 ){
			pEngine->stopExportSong();
			m_bExporting = false;
		}
		
		for (auto i = 0; i < pInstrumentList->size(); i++) {
			pInstrumentList->get(i)->set_currently_exported( false );
		}
		
		pSong->get_instrument_list()->get(m_nInstrument)->set_currently_exported( true );
		
		pEngine->startExportSong( filename );

		if(! (m_nInstrument == pInstrumentList->size()) ){
			m_nInstrument++;
		}
	}

}

void ExportSongDialog::on_closeBtn_clicked()
{
	
	Hydrogen::get_instance()->stopExportSong();
	Hydrogen::get_instance()->stopExportSession();
	
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
		sampleRateLable->hide();
		sampleDepthLable->hide();
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
		if( templateCombo->currentIndex() != 9 ){
			templateCombo->setCurrentIndex( 9 );//ogg
		}
	}
	else if( filename.endsWith( ".flac" ) || filename.endsWith( ".FLAC" ) ){
		sampleRateLable->show();
		sampleDepthLable->show();
		if( templateCombo->currentIndex() != 8 ){
			templateCombo->setCurrentIndex( 8 );//flac
		}
	}
	else if( filename.endsWith( ".aiff" ) || filename.endsWith( ".AIFF" ) ){
		sampleRateLable->show();
		sampleDepthLable->show();
		if( templateCombo->currentIndex() < 5 || templateCombo->currentIndex() > 7 ){
			templateCombo->setCurrentIndex( 5 );//aiff
		}
	}
	else if( filename.endsWith( ".wav" ) || filename.endsWith( ".WAV" ) ){
		sampleRateLable->show();
		sampleDepthLable->show();
		if( templateCombo->currentIndex() > 4 ){
			templateCombo->setCurrentIndex( 0 );//wav

		}
	}
}

void ExportSongDialog::progressEvent( int nValue )
{
	m_pProgressBar->setValue( nValue );
	if ( nValue == 100 ) {

		m_bExporting = false;

		if( m_nInstrument == Hydrogen::get_instance()->getSong()->get_instrument_list()->size()){
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

void ExportSongDialog::toggleTimeLineBPMMode(bool toggled)
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
	
	Hydrogen *pEngine = Hydrogen::get_instance();
	Timeline* pTimeline = pEngine->getTimeline();

	float oldBPM = pEngine->getSong()->__bpm;
	float lowBPM = oldBPM;

	if( pTimeline->m_timelinevector.size() >= 1 ){
		for ( int t = 0; t < pTimeline->m_timelinevector.size(); t++){
			if(pTimeline->m_timelinevector[t].m_htimelinebpm < lowBPM){
				lowBPM =  pTimeline->m_timelinevector[t].m_htimelinebpm;
			}

		}
	}

	pEngine->setBPM(lowBPM);
	time_t sTime = time(NULL);

	Song *pSong = pEngine->getSong();
	assert(pSong);
	
	if(pSong){
		InstrumentList *songInstrList = pSong->get_instrument_list();
		assert(songInstrList);
		for ( unsigned nInstr = 0; nInstr < songInstrList->size(); ++nInstr ) {
			Instrument *pInstr = songInstrList->get( nInstr );
			assert( pInstr );
			if ( pInstr ){
				for (std::vector<InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it != pInstr->get_components()->end(); ++it) {
					InstrumentComponent* pCompo = *it;
					for ( int nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); nLayer++ ) {
						InstrumentLayer *pLayer = pCompo->get_layer( nLayer );
						if ( pLayer ) {
							Sample *pSample = pLayer->get_sample();
							if ( pSample ) {
								if( pSample->get_rubberband().use ) {
									Sample *pNewSample = Sample::load(
												pSample->get_filepath(),
												pSample->get_loops(),
												pSample->get_rubberband(),
												*pSample->get_velocity_envelope(),
												*pSample->get_pan_envelope()
												);
									if( !pNewSample ){
										continue;
									}
									delete pSample;
									// insert new sample from newInstrument
									AudioEngine::get_instance()->lock( RIGHT_HERE );
									pLayer->set_sample( pNewSample );
									AudioEngine::get_instance()->unlock();
									
								}
							}
						}
					}
				}
			}
		}
	}
	
	Preferences::get_instance()->setRubberBandCalcTime(time(NULL) - sTime);
	
	pEngine->setBPM(oldBPM);
	
	closeBtn->setEnabled(true);
	resampleComboBox->setEnabled(true);
	okBtn->setEnabled(true);
	QApplication::restoreOverrideCursor();
}

bool ExportSongDialog::checkUseOfRubberband()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	assert(pSong);
	
	if(pSong){
		InstrumentList *pSongInstrList = pSong->get_instrument_list();
		assert(pSongInstrList);
		for ( unsigned nInstr = 0; nInstr < pSongInstrList->size(); ++nInstr ) {
			Instrument *pInstr = pSongInstrList->get( nInstr );
			assert( pInstr );
			if ( pInstr ){
				for (std::vector<InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it != pInstr->get_components()->end(); ++it) {
					InstrumentComponent* pCompo = *it;
					for ( int nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); nLayer++ ) {
						InstrumentLayer *pLayer = pCompo->get_layer( nLayer );
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
	}
	return false;
}
