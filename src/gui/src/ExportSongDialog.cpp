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

enum ExportModes { EXPORT_TO_SINGLE_TRACK, EXPORT_TO_SEPARATE_TRACKS, EXPORT_TO_BOTH };

// Here we are going to store export filename 
QString ExportSongDialog::sLastFilename = "";

ExportSongDialog::ExportSongDialog(QWidget* parent)
	: QDialog(parent)
	, Object( __class_name )
	, m_bExporting( false )
	, m_pEngine( Hydrogen::get_instance() )
	, m_pPreferences( Preferences::get_instance() )
{
	setupUi( this );
	setModal( true );
	setWindowTitle( tr( "Export song" ) );

	exportTypeCombo->addItem(tr("Export to a single track"));
	exportTypeCombo->addItem(tr("Export to separate tracks"));
	exportTypeCombo->addItem(tr("Both"));

	HydrogenApp::get_instance()->addEventListener( this );

	m_pProgressBar->setValue( 0 );
	
	m_bQfileDialog = false;
	m_bExportTrackouts = false;
	m_nInstrument = 0;
	m_sExtension = ".wav";
	m_bOverwriteFiles = false;

	// use of rubberband batch
	if( checkUseOfRubberband() ) {
		m_bOldRubberbandBatchMode = m_pPreferences->getRubberBandBatchMode();
		toggleRubberbandCheckBox->setChecked(m_pPreferences->getRubberBandBatchMode());
		connect(toggleRubberbandCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleRubberbandBatchMode( bool )));
	} else {
		m_bOldRubberbandBatchMode = m_pPreferences->getRubberBandBatchMode();
		toggleRubberbandCheckBox->setEnabled( false );
	}

	// use of timeline
	if( m_pEngine->getTimeline()->getAllTempoMarkers().size() > 0 ){
		toggleTimeLineBPMCheckBox->setChecked(m_pPreferences->getUseTimelineBpm());
		m_bOldTimeLineBPMMode = m_pPreferences->getUseTimelineBpm();
		connect(toggleTimeLineBPMCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleTimeLineBPMMode( bool )));
	} else {
		m_bOldTimeLineBPMMode = m_pPreferences->getUseTimelineBpm();
		toggleTimeLineBPMCheckBox->setEnabled( false );
	}

	// use of interpolation mode
	m_nOldInterpolation = AudioEngine::get_instance()->get_sampler()->getInterpolateMode();
	resampleComboBox->setCurrentIndex( m_nOldInterpolation );
	connect(resampleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(resampleComboBoIndexChanged(int)));

	// if rubberbandBatch calculate time needed by lib rubberband to resample samples
	if( m_bOldRubberbandBatchMode ) {
		calculateRubberbandTime();
	}
	
	//Load the other settings..
	restoreSettingsFromPreferences();

	// Have the dialog find the best size
	adjustSize();
}

ExportSongDialog::~ExportSongDialog()
{
	HydrogenApp::get_instance()->removeEventListener( this );
}

QString ExportSongDialog::createDefaultFilename()
{
	QString sDefaultFilename = m_pEngine->getSong()->get_filename();

	// If song is not saved then use song name otherwise use the song filename
	if( sDefaultFilename.isEmpty() ){
		sDefaultFilename = m_pEngine->getSong()->__name;
	} else {
		// extracting filename from full path
		QFileInfo qDefaultFile( sDefaultFilename ); 
		sDefaultFilename = qDefaultFile.fileName();
	}

	sDefaultFilename.replace( '*', "_" );
	sDefaultFilename.replace( Filesystem::songs_ext, "" );
	sDefaultFilename += m_sExtension;
	return sDefaultFilename;
}

void ExportSongDialog::saveSettingsToPreferences()
{
	// extracting dirname from export box	
	QString sFilename = exportNameTxt->text();
	QFileInfo info( sFilename );
	QDir dir = info.absoluteDir();
	if ( !dir.exists() ) {
		// very strange if it happens but better to check for it anyway
		return;
	}
	
	// saving filename for this session	
	sLastFilename = info.fileName();
	QString sSelectedDirname = dir.absolutePath();
	m_pPreferences->setExportDirectory( sSelectedDirname );
	
	// saving other options
	m_pPreferences->setExportModeIdx( exportTypeCombo->currentIndex() );
	m_pPreferences->setExportTemplateIdx( templateCombo->currentIndex() );
	m_pPreferences->setExportSampleRateIdx( sampleRateCombo->currentIndex() );
	m_pPreferences->setExportSampleDepthIdx( sampleDepthCombo->currentIndex() );
}

void ExportSongDialog::restoreSettingsFromPreferences()
{
	// loading previous directory and filling filename text field
	
	// loading default filename on a first run and storing it in static field
	if( sLastFilename.isEmpty() ) {
		sLastFilename = createDefaultFilename();
	}

	QString sDirPath = m_pPreferences->getExportDirectory();
	QDir qd = QDir( sDirPath );
	
	// joining filepath with dirname
	QString sFullPath = qd.absoluteFilePath( sLastFilename );
	exportNameTxt->setText( sFullPath );
	
	// loading rest of the options
	templateCombo->setCurrentIndex( m_pPreferences->getExportTemplateIdx() );
	exportTypeCombo->setCurrentIndex( m_pPreferences->getExportModeIdx() );
	
	int nExportSampleRateIdx = m_pPreferences->getExportSampleRateIdx();
	if( nExportSampleRateIdx >= 0 ) {
		sampleRateCombo->setCurrentIndex( nExportSampleRateIdx );
	}
	
	
	int nExportBithDepthIdx = m_pPreferences->getExportSampleDepthIdx();
	if( nExportBithDepthIdx >= 0 ) {
		sampleDepthCombo->setCurrentIndex( nExportBithDepthIdx );
	}
}

void ExportSongDialog::on_browseBtn_clicked()
{
	QString sPrevDir = m_pPreferences->getExportDirectory();

	QFileDialog fd(this);
	fd.setFileMode(QFileDialog::AnyFile);

	if( templateCombo->currentIndex() <= 4 ) fd.setNameFilter("Microsoft WAV (*.wav *.WAV)");
	if( templateCombo->currentIndex() > 4 && templateCombo->currentIndex() < 8  ) fd.setNameFilter( "Apple AIFF (*.aiff *.AIFF)");
	if( templateCombo->currentIndex() == 8) fd.setNameFilter( "Lossless  Flac (*.flac *.FLAC)");
	if( templateCombo->currentIndex() == 9) fd.setNameFilter( "Compressed Ogg (*.ogg *.OGG)");

	fd.setDirectory( sPrevDir );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setWindowTitle( tr( "Export song" ) );

	QString defaultFilename = exportNameTxt->text();

	fd.selectFile(defaultFilename);

	QString filename = "";
	if (fd.exec()) {
		filename = fd.selectedFiles().first();
		m_bQfileDialog = true;
	}

	if ( !filename.isEmpty() ) {
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

bool ExportSongDialog::validateUserInput() 
{
    // check if directory exists otherwise error
	QString filename = exportNameTxt->text();
	QFileInfo file( filename );
	QDir dir = file.dir();
	if( !dir.exists() ) {
		QMessageBox::warning(
			this, "Hydrogen",
			tr( "Directory %1 does not exist").arg( dir.absolutePath() ),
			QMessageBox::Ok
		);
		return false;
	}
	
	return true;
}

void ExportSongDialog::on_okBtn_clicked()
{
	if ( m_bExporting ) {
		return;
	}
	
	if( !validateUserInput() ) {
		return;
	}
	
	saveSettingsToPreferences();
	
	Song *pSong = m_pEngine->getSong();
	InstrumentList *pInstrumentList = pSong->get_instrument_list();

	m_bOverwriteFiles = false;

	if( exportTypeCombo->currentIndex() == EXPORT_TO_SINGLE_TRACK || exportTypeCombo->currentIndex() == EXPORT_TO_BOTH ){
		m_bExportTrackouts = false;

		QString filename = exportNameTxt->text();
		if ( QFileInfo( filename ).exists() == true && m_bQfileDialog == false ) {

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
		
		m_pEngine->startExportSession( sampleRateCombo->currentText().toInt(), sampleDepthCombo->currentText().toInt());
		m_pEngine->startExportSong( filename );

		return;
	}

	if( exportTypeCombo->currentIndex() == EXPORT_TO_SEPARATE_TRACKS ){
		m_bExportTrackouts = true;
		m_pEngine->startExportSession(sampleRateCombo->currentText().toInt(), sampleDepthCombo->currentText().toInt());
		exportTracks();
		return;
	}

}

bool ExportSongDialog::currentInstrumentHasNotes()
{
	Song *pSong = m_pEngine->getSong();
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
	Song *pSong = m_pEngine->getSong();
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
	Song *pSong = m_pEngine->getSong();
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

		if ( QFile( filename ).exists() == true && m_bQfileDialog == false && !m_bOverwriteFiles) {
			int res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(filename), QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll );
			if (res == QMessageBox::No ) return;
			if (res == QMessageBox::YesToAll ) m_bOverwriteFiles = true;
		}
		
		if( m_nInstrument > 0 ){
			m_pEngine->stopExportSong();
			m_bExporting = false;
		}
		
		for (auto i = 0; i < pInstrumentList->size(); i++) {
			pInstrumentList->get(i)->set_currently_exported( false );
		}
		
		pSong->get_instrument_list()->get(m_nInstrument)->set_currently_exported( true );
		
		m_pEngine->startExportSong( filename );

		if(! (m_nInstrument == pInstrumentList->size()) ){
			m_nInstrument++;
		}
	}
    
}

void ExportSongDialog::on_closeBtn_clicked()
{
	
	m_pEngine->stopExportSong();
	m_pEngine->stopExportSession();
	
	m_bExporting = false;
	
	if(m_pPreferences->getRubberBandBatchMode()){
		EventQueue::get_instance()->push_event( EVENT_RECALCULATERUBBERBAND, -1);
	}
	m_pPreferences->setRubberBandBatchMode( m_bOldRubberbandBatchMode );
	m_pPreferences->setUseTimelineBpm( m_bOldTimeLineBPMMode );
	setResamplerMode(m_nOldInterpolation);
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
		sampleRateCombo->setCurrentIndex ( 4 ); //96000hz
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
	m_pPreferences->setRubberBandBatchMode(toggled);
	if(toggled){
		calculateRubberbandTime();
	}
}

void ExportSongDialog::toggleTimeLineBPMMode(bool toggled)
{
	m_pPreferences->setUseTimelineBpm(toggled);
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
	
	Timeline* pTimeline = m_pEngine->getTimeline();
	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();

	float oldBPM = m_pEngine->getSong()->__bpm;
	float lowBPM = oldBPM;

	if ( tempoMarkerVector.size() >= 1 ){
		for ( int t = 0; t < tempoMarkerVector.size(); t++){
			if(tempoMarkerVector[t]->m_htimelinebpm < lowBPM){
				lowBPM =  tempoMarkerVector[t]->m_htimelinebpm;
			}

		}
	}

	m_pEngine->setBPM(lowBPM);
	time_t sTime = time(nullptr);

	Song *pSong = m_pEngine->getSong();
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
	
	Preferences::get_instance()->setRubberBandCalcTime(time(nullptr) - sTime);
	
	m_pEngine->setBPM(oldBPM);
	
	closeBtn->setEnabled(true);
	resampleComboBox->setEnabled(true);
	okBtn->setEnabled(true);
	QApplication::restoreOverrideCursor();
}

bool ExportSongDialog::checkUseOfRubberband()
{
	Song *pSong = m_pEngine->getSong();
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
