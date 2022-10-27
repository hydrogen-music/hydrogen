/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <QFileDialog>
#include <QProgressBar>
#include <QLabel>

#include "CommonStrings.h"
#include "ExportSongDialog.h"
#include "HydrogenApp.h"
#include "Mixer/Mixer.h"

#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Timeline.h>
#include <core/IO/AudioOutput.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Sampler/Sampler.h>
#include <core/EventQueue.h>

#include <memory>

#ifdef WIN32
#include <time.h>
#endif

using namespace H2Core;

enum ExportModes { EXPORT_TO_SINGLE_TRACK, EXPORT_TO_SEPARATE_TRACKS, EXPORT_TO_BOTH };

static int interpolateModeToComboBoxIndex(Interpolation::InterpolateMode interpolateMode)
{
	int Index = 0;
	
	switch ( interpolateMode ) {
		case Interpolation::InterpolateMode::Linear:
			Index = 0;
			break;
		case Interpolation::InterpolateMode::Cosine:
			Index = 1;
			break;
		case Interpolation::InterpolateMode::Third:
			Index = 2;
			break;
		case Interpolation::InterpolateMode::Cubic:
			Index = 3;
			break;
		case Interpolation::InterpolateMode::Hermite:
			Index = 4;
			break;
	}
	
	return Index;
}

// Here we are going to store export filename 
QString ExportSongDialog::sLastFilename = "";

ExportSongDialog::ExportSongDialog(QWidget* parent)
	: QDialog(parent)
	, m_bExporting( false )
	, m_pHydrogen( Hydrogen::get_instance() )
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
	m_bOldRubberbandBatchMode = m_pPreferences->getRubberBandBatchMode();

	// use of rubberband batch
	if( checkUseOfRubberband() ) {
		toggleRubberbandCheckBox->setChecked( m_bOldRubberbandBatchMode );
		connect(toggleRubberbandCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleRubberbandBatchMode( bool )));
	} else {
		toggleRubberbandCheckBox->setEnabled( false );
		toggleRubberbandCheckBox->setToolTip( tr( "No sample in the current song uses Rubberband" ) );
	}

	// use of timeline
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();
	toggleTimeLineBPMCheckBox->setChecked( pSong->getIsTimelineActivated());
	m_bOldTimeLineBPMMode = pSong->getIsTimelineActivated();
	connect(toggleTimeLineBPMCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleTimeLineBPMMode( bool )));

	// use of interpolation mode
	m_OldInterpolationMode = m_pHydrogen->getAudioEngine()->getSampler()->getInterpolateMode();
	resampleComboBox->setCurrentIndex( interpolateModeToComboBoxIndex( m_OldInterpolationMode ) );
	connect(resampleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(resampleComboBoIndexChanged(int)));
	
	//Load the other settings..
	restoreSettingsFromPreferences();

	// Have the dialog find the best size
	adjustSize();
}

ExportSongDialog::~ExportSongDialog()
{
	if ( auto pH2App = HydrogenApp::get_instance() ) {
		pH2App->removeEventListener( this );
	}
}

QString ExportSongDialog::createDefaultFilename()
{
	QString sDefaultFilename = m_pHydrogen->getSong()->getFilename();

	// If song is not saved then use song name otherwise use the song filename
	if( sDefaultFilename.isEmpty() ){
		sDefaultFilename = m_pHydrogen->getSong()->getName();
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
	Preferences::get_instance()->setLastExportSongDirectory( dir.absolutePath() );
	
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

	QString sDirPath = m_pPreferences->getLastExportSongDirectory();
	QDir qd = QDir( sDirPath );
	
	// joining filepath with dirname
	QString sFullPath = qd.absoluteFilePath( sLastFilename );
	exportNameTxt->setText( sFullPath );
	
	// loading rest of the options
	templateCombo->setCurrentIndex( m_pPreferences->getExportTemplateIdx() );
	exportTypeCombo->setCurrentIndex( m_pPreferences->getExportModeIdx() );
	
	int nExportSampleRateIdx = m_pPreferences->getExportSampleRateIdx();
	if( nExportSampleRateIdx > 0 ) {
		sampleRateCombo->setCurrentIndex( nExportSampleRateIdx );
	} else {
		sampleRateCombo->setCurrentIndex( 0 );
	}
	
	
	int nExportBithDepthIdx = m_pPreferences->getExportSampleDepthIdx();
	if( nExportBithDepthIdx > 0 ) {
		sampleDepthCombo->setCurrentIndex( nExportBithDepthIdx );
	} else {
		sampleDepthCombo->setCurrentIndex( 0 );
	}
}

void ExportSongDialog::on_browseBtn_clicked()
{
	QString sPath = Preferences::get_instance()->getLastExportSongDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = QDir::homePath();
	}

	QFileDialog fd(this);
	fd.setFileMode(QFileDialog::AnyFile);

	if( templateCombo->currentIndex() <= 4 ) {
		fd.setNameFilter("Microsoft WAV (*.wav *.WAV)");
	} else if ( templateCombo->currentIndex() > 4 && templateCombo->currentIndex() < 8  ) {
		fd.setNameFilter( "Apple AIFF (*.aiff *.AIFF)");
	} else if ( templateCombo->currentIndex() == 8 ) {
		fd.setNameFilter( "Lossless  Flac (*.flac *.FLAC)");
	} else if ( templateCombo->currentIndex() == 9 ) {
		fd.setNameFilter( "Compressed Ogg (*.ogg *.OGG)");
	}

	fd.setDirectory( sPath );
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

	auto pPref = Preferences::get_instance();
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	auto pInstrumentList = pSong->getInstrumentList();

	// License related export warnings
	if ( pPref->m_bShowExportSongLicenseWarning ) {
		auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
		
		QMessageBox licenseWarning( this );

		auto drumkitContent =
			pSong->getInstrumentList()->summarizeContent( pSong->getComponents() );

		bool bHasAttribution = false;
		bool bIsCopyleft = false;
		QStringList licenses;
		QString sLicense;
		for ( const auto& ccontent : drumkitContent ) {
			if ( ccontent->m_license.hasAttribution() ) {
				sLicense = QString( "%1 (by %2)" )
					.arg( ccontent->m_license.getLicenseString() )
					.arg( ccontent->m_license.getCopyrightHolder() );
				bHasAttribution = true;
			}
			else {
				sLicense = ccontent->m_license.getLicenseString();
			}
			
			if ( ! licenses.contains( sLicense ) ) {
				licenses << sLicense;

				if ( ccontent->m_license.isCopyleft() ) {
					bIsCopyleft = true;
				}
			}
		}
		QString sMsg = QString( tr( "Your song uses samples of the following license:" ) )
			.append( "<ul>" );
		for ( const auto& llicense : licenses ) {
			sMsg.append( QString(  "<li>%1</li>" ).arg( llicense ) );
		}
		sMsg.append( "</ul>" );

		if ( bIsCopyleft ) {
			sMsg.append( QString( "<p>%1</p>" )
						 .arg( pCommonStrings->getLicenseCopyleftWarning() ) );
		}

		if ( bHasAttribution ) {
			sMsg.append( QString( "<p>%1</p>" )
						 .arg( pCommonStrings->getLicenseAttributionWarning() ) );
		}
		
		sMsg.append( "\n" ).append( tr( "Be sure you satisfy all license conditions and give the required attribution." ) );

		licenseWarning.setWindowTitle( pCommonStrings->getLicenseWarningWindowTitle() );
		licenseWarning.setText( sMsg );
		licenseWarning.setTextFormat( Qt::RichText );

		licenseWarning.addButton( pCommonStrings->getButtonOk(),
								  QMessageBox::AcceptRole );
		auto pMuteButton =
			licenseWarning.addButton( pCommonStrings->getMutableDialog(),
									  QMessageBox::YesRole );
		auto pRejectButton =
			licenseWarning.addButton( pCommonStrings->getButtonCancel(),
									  QMessageBox::RejectRole );
		licenseWarning.exec();

		if ( licenseWarning.clickedButton() == pMuteButton ) {
			pPref->m_bShowExportSongLicenseWarning = false;
		}
		else if ( licenseWarning.clickedButton() == pRejectButton ) {
			return;
		}
	}

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

		if ( ! m_pHydrogen->startExportSession( sampleRateCombo->currentText().toInt(),
												sampleDepthCombo->currentText().toInt()) ) {
			QMessageBox::critical( this, "Hydrogen", tr( "Unable to export song" ) );
			return;
		}
		m_pHydrogen->startExportSong( filename );
		return;
	}

	if( exportTypeCombo->currentIndex() == EXPORT_TO_SEPARATE_TRACKS ){
		m_bExportTrackouts = true;
		m_pHydrogen->startExportSession(sampleRateCombo->currentText().toInt(), sampleDepthCombo->currentText().toInt());
		exportTracks();
		return;
	}

}

bool ExportSongDialog::currentInstrumentHasNotes()
{
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	unsigned nPatterns = pSong->getPatternList()->size();
	
	bool bInstrumentHasNotes = false;
	
	for ( unsigned i = 0; i < nPatterns; i++ ) {
		Pattern *pPattern = pSong->getPatternList()->get( i );
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );

			if( pNote->get_instrument()->get_id() == pSong->getInstrumentList()->get(m_nInstrument)->get_id() ){
				bInstrumentHasNotes = true;
				break;
			}
		}
	}
	
	return bInstrumentHasNotes;
}

QString ExportSongDialog::findUniqueExportFilenameForInstrument( std::shared_ptr<Instrument> pInstrument )
{
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	QString uniqueInstrumentName;
	
	int instrumentOccurence = 0;
	for(int i=0; i  < pSong->getInstrumentList()->size(); i++ ){
		if( pSong->getInstrumentList()->get(m_nInstrument)->get_name() == pInstrument->get_name()){
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
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	auto pInstrumentList = pSong->getInstrumentList();
	
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
			m_pHydrogen->stopExportSong();
			m_bExporting = false;
		}
		
		for (auto i = 0; i < pInstrumentList->size(); i++) {
			pInstrumentList->get(i)->set_currently_exported( false );
		}
		
		pSong->getInstrumentList()->get(m_nInstrument)->set_currently_exported( true );
		
		m_pHydrogen->startExportSong( filename );

		if(! (m_nInstrument == pInstrumentList->size()) ){
			m_nInstrument++;
		}
	}
    
}

void ExportSongDialog::closeEvent( QCloseEvent *event ) {
	UNUSED( event );
	closeExport();
}
void ExportSongDialog::on_closeBtn_clicked()
{
	closeExport();
}
void ExportSongDialog::closeExport() {
	
	m_pHydrogen->stopExportSong();
	m_pHydrogen->stopExportSession();
	
	m_bExporting = false;
	
	if( m_pPreferences->getRubberBandBatchMode() ){
		m_pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
		m_pHydrogen->recalculateRubberband( m_pHydrogen->getAudioEngine()->getTransportPosition()->getBpm() );
		m_pHydrogen->getAudioEngine()->unlock();
	}
	m_pPreferences->setRubberBandBatchMode( m_bOldRubberbandBatchMode );
	m_pHydrogen->setIsTimelineActivated( m_bOldTimeLineBPMMode );
	
	m_pHydrogen->getAudioEngine()->getSampler()->setInterpolateMode( m_OldInterpolationMode );
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

		if( m_nInstrument == Hydrogen::get_instance()->getSong()->getInstrumentList()->size()){
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
}

void ExportSongDialog::toggleTimeLineBPMMode(bool toggled)
{
	m_pHydrogen->setIsTimelineActivated( toggled );
}

void ExportSongDialog::resampleComboBoIndexChanged(int index )
{
	setResamplerMode(index);
}

void ExportSongDialog::setResamplerMode(int index)
{
	switch ( index ){
	case 0:
		m_pHydrogen->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Linear );
		break;
	case 1:
		m_pHydrogen->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Cosine );
		break;
	case 2:
		m_pHydrogen->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Third );
		break;
	case 3:
		m_pHydrogen->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Cubic );
		break;
	case 4:
		m_pHydrogen->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Hermite );
		break;
	}
}

bool ExportSongDialog::checkUseOfRubberband()
{
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	assert(pSong);
	
	if(pSong){
		auto pSongInstrList = pSong->getInstrumentList();
		assert(pSongInstrList);
		for ( unsigned nInstr = 0; nInstr < pSongInstrList->size(); ++nInstr ) {
			auto pInstr = pSongInstrList->get( nInstr );
			assert( pInstr );
			if ( pInstr ){
				for ( const auto& pCompo : *pInstr->get_components() ) {
					for ( int nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); nLayer++ ) {
						auto pLayer = pCompo->get_layer( nLayer );
						if ( pLayer ) {
							auto pSample = pLayer->get_sample();
							if ( pSample != nullptr ) {
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
