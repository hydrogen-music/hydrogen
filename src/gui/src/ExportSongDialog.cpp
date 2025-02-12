/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <QProgressBar>
#include <QLabel>

#include "CommonStrings.h"
#include "ExportSongDialog.h"
#include "HydrogenApp.h"
#include "Mixer/Mixer.h"
#include "Widgets/FileDialog.h"

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
#include <core/IO/DiskWriterDriver.h>
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
	m_sExtension = Filesystem::AudioFormatToSuffix( Filesystem::AudioFormat::Flac );
	m_bOverwriteFiles = false;
	m_bOldRubberbandBatchMode = m_pPreferences->getRubberBandBatchMode();

	// Format combo box
	formatCombo->addItem( "FLAC (Free Lossless Audio Codec)" );
	m_formatMap[ 0 ] = Filesystem::AudioFormat::Flac;
	formatCombo->addItem( "OPUS (Opus Compressed Audio)" );
	m_formatMap[ 1 ] = Filesystem::AudioFormat::Opus;
	formatCombo->addItem( "OGG (Vorbis Compressed Audio)" );
	m_formatMap[ 2 ] = Filesystem::AudioFormat::Ogg;
	formatCombo->addItem( "MP3 (MP3 Compressed Audio)" );
	m_formatMap[ 3 ] = Filesystem::AudioFormat::Mp3;
	formatCombo->addItem( "WAV (Waveform Audio)" );
	m_formatMap[ 4 ] = Filesystem::AudioFormat::Wav;
	formatCombo->addItem( "AIFF (Audio Interchange File Format)" );
	m_formatMap[ 5 ] = Filesystem::AudioFormat::Aiff;
	formatCombo->addItem( "AU (Sun/NeXT Audio)" );
	m_formatMap[ 6 ] = Filesystem::AudioFormat::Au;
	formatCombo->addItem( "CAF (Core Audio Format)" );
	m_formatMap[ 7 ] = Filesystem::AudioFormat::Caf;
	formatCombo->addItem( "VOC (Creative Voice)" );
	m_formatMap[ 8 ] = Filesystem::AudioFormat::Voc;
	formatCombo->addItem( "W64 (Sonic Foundry’s 64 bit RIFF/WAV)" );
	m_formatMap[ 9 ] = Filesystem::AudioFormat::W64;

	// Per-default FLAC format is chosen, for which we only provide compression
	// level but no sample rate and depth settings (IMHO providing them and
	// setting sample rates smaller than 48k does only make sense when encoding
	// speech and not for music yet alone drumkits.)
	formatCombo->setCurrentIndex( 0 );
	connect( formatCombo, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(formatComboIndexChanged(int) ) );

	sampleRateCombo->hide();
	sampleRateLabel->hide();
	sampleDepthCombo->hide();
	sampleDepthLabel->hide();
	// Best-possible quality (MP3, Vorbis, Opus) / fastest encoding (FLAC) as
	// default.
	compressionLevelSpinBox->setValue( 0.0 );

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
	return QString( "%1.%2" ).arg( sDefaultFilename ).arg( m_sExtension );
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
	m_pPreferences->setExportFormat( m_formatMap[ formatCombo->currentIndex() ] );
	m_pPreferences->setExportCompressionLevel(compressionLevelSpinBox->value() );
	m_pPreferences->setExportSampleRateIdx( sampleRateCombo->currentIndex() );
	m_pPreferences->setExportSampleDepthIdx( sampleDepthCombo->currentIndex() );
}

void ExportSongDialog::restoreSettingsFromPreferences()
{
	// In case the filename used in the last session is invalid, we discard it.
	if ( ! sLastFilename.isEmpty() ) {
		QFileInfo info( sLastFilename );
		if ( info.suffix().isEmpty() ||
			 Filesystem::AudioFormatFromSuffix( info.suffix() ) ==
			 Filesystem::AudioFormat::Unknown ) {
			sLastFilename = "";
		}
	}

	if ( sLastFilename.isEmpty() ) {
		sLastFilename = createDefaultFilename();
	}

	// loading previous directory
	QString sDirPath = m_pPreferences->getLastExportSongDirectory();
	QDir qd = QDir( sDirPath );
	// joining filepath with dirname
	QString sFullPath = qd.absoluteFilePath( sLastFilename );

	exportNameTxt->setText( sFullPath );
	
	// loading rest of the options
	const auto previousFormat = m_pPreferences->getExportFormat();
	if ( previousFormat != Filesystem::AudioFormat::Unknown ) {
		for ( const auto [ nnIndex, fformat ] : m_formatMap ) {
			if ( fformat == previousFormat ) {
				formatCombo->setCurrentIndex( nnIndex );
				break;
			}
		}
	}
	compressionLevelSpinBox->setValue(
		m_pPreferences->getExportCompressionLevel() );
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

	FileDialog fd(this);
	fd.setFileMode(QFileDialog::AnyFile);

	const auto format = m_formatMap[ formatCombo->currentIndex() ];

	switch ( format ) {
	case Filesystem::AudioFormat::Wav:
		fd.setNameFilter( "Microsoft WAV (*.wav *.WAV)" );
		break;
	case Filesystem::AudioFormat::Aiff:
		fd.setNameFilter( "Apple AIFF (*.aiff *.AIFF)" );
		break;
	case Filesystem::AudioFormat::Flac:
		fd.setNameFilter( "Lossless Flac (*.flac *.FLAC)" );
		break;
	case Filesystem::AudioFormat::Opus:
		fd.setNameFilter( "Compressed Ogg/Opus (*.opus *.OPUS)" );
		break;
	case Filesystem::AudioFormat::Ogg:
		fd.setNameFilter( "Compressed Ogg/Vorbis (*.ogg *.OGG)" );
		break;
	case Filesystem::AudioFormat::Mp3:
		fd.setNameFilter( "Compressed MPEG Layer 3 (*.mp3 *.MP3)" );
		break;
	case Filesystem::AudioFormat::Au:
		fd.setNameFilter( "Sun/NeXT AU (*.au *.AU)" );
		break;
	case Filesystem::AudioFormat::Caf:
		fd.setNameFilter( "Core Audio Format (*.caf *.CAF)" );
		break;
	case Filesystem::AudioFormat::Voc:
		fd.setNameFilter( "Creative Voice File (*.voc *.VOC)" );
		break;
	case Filesystem::AudioFormat::W64:
		fd.setNameFilter( "Sonic Foundrys' 64 bit RIFF/WAV (*.w64 *.W64)" );
		break;
	case Filesystem::AudioFormat::Unknown:
	default:
		ERRORLOG( QString( "Unhandle combo index [%1]" )
				  .arg( formatCombo->currentIndex() ) );
		return;
	}

	fd.setDirectory( sPath );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setWindowTitle( tr( "Export song" ) );

	QString defaultFilename = exportNameTxt->text();

	fd.selectFile(defaultFilename);

	QString sFilename = "";
	if ( fd.exec() ) {
		sFilename = fd.selectedFiles().first();
		m_bQfileDialog = true;
	}

	if ( !sFilename.isEmpty() ) {
		// this second extension check is mostly important if you leave a dot
		// without a regular extionsion in a sFilename
		if( ! sFilename.endsWith( m_sExtension ) ){
			sFilename.append( QString( ".%1" ).arg( m_sExtension ) );
		}

		exportNameTxt->setText( sFilename );
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

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QFileInfo fileInfo( exportNameTxt->text() );
	if ( ! Filesystem::dir_writable( fileInfo.absoluteDir().absolutePath(), false ) ) {
		QMessageBox::warning( this, "Hydrogen",
							  pCommonStrings->getFileDialogMissingWritePermissions(),
							  QMessageBox::Ok );
		return;
	}

	int nSampleRate = sampleRateCombo->currentText().toInt();
	int nSampleDepth = sampleDepthCombo->currentText().toInt();
	const float fCompressionLevel = compressionLevelSpinBox->value();

	// Some formats only support a certain set of parameters and need special
	// treatment.
	const auto format = m_formatMap[ formatCombo->currentIndex() ];
	if ( format == Filesystem::AudioFormat::Ogg ||
		 format == Filesystem::AudioFormat::Opus ) {
		nSampleDepth = 32;
		nSampleRate = 48000;
	}
	else if ( format == Filesystem::AudioFormat::Voc ) {
		nSampleDepth = std::min( 16, nSampleDepth );
	}
	else if ( format == Filesystem::AudioFormat::Flac ||
			  format == Filesystem::AudioFormat::Mp3 ) {
		nSampleDepth = 16;
		nSampleRate = 48000;
	}

	auto pPref = Preferences::get_instance();
	std::shared_ptr<Song> pSong = m_pHydrogen->getSong();
	auto pInstrumentList = pSong->getInstrumentList();

	// License related export warnings
	if ( pPref->m_bShowExportSongLicenseWarning ) {
		
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
		if ( fileInfo.exists() == true && m_bQfileDialog == false ) {

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

		if ( ! m_pHydrogen->startExportSession(
				 nSampleRate, nSampleDepth, fCompressionLevel ) ) {
			QMessageBox::critical( this, "Hydrogen", tr( "Unable to export song" ) );
			return;
		}
		m_pHydrogen->startExportSong( filename );
		return;
	}

	if ( exportTypeCombo->currentIndex() == EXPORT_TO_SEPARATE_TRACKS ){
		m_bExportTrackouts = true;
		if ( ! m_pHydrogen->startExportSession(
				 nSampleRate, nSampleDepth, fCompressionLevel ) ) {
			QMessageBox::critical( this, "Hydrogen", tr( "Unable to export tracks" ) );
			return;
		}
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
		FOREACH_NOTE_CST_IT_BEGIN_LENGTH(notes,it,pPattern) {
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

		// Ensure we use the right extension.
		const QString sSuffix = QString( ".%1" ).arg( m_sExtension );
		const QString sTemplateName = exportNameTxt->text();
		QString sBaseName = sTemplateName;
		if ( sTemplateName.endsWith( sSuffix, Qt::CaseInsensitive ) ) {
			sBaseName.chop( sSuffix.size() );
		}

		const QString sInstrumentName = findUniqueExportFilenameForInstrument(
			pInstrumentList->get( m_nInstrument ) );
		QString sExportName;
		if ( sBaseName.isEmpty() || sBaseName.endsWith( "/" ) ||
			 sBaseName.endsWith( "\\" ) ) {
			// Allow to use just the instrument names when leaving the song name
			// blank.
			sExportName = QString( "%1%2" ).arg( sBaseName )
				.arg( sInstrumentName );
		}
		else {
			sExportName = QString( "%1-%2" ).arg( sBaseName )
				.arg( sInstrumentName );
		}

		const QString sFileName = QString( "%1%2" ).arg( sExportName )
			.arg( sSuffix );

		if ( QFile( sFileName ).exists() == true && m_bQfileDialog == false &&
			 ! m_bOverwriteFiles ) {
			const int nRes = QMessageBox::information(
				this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?")
				.arg( sFileName ),
				QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll );
			if ( nRes == QMessageBox::No ) {
				return;
			}
			if ( nRes == QMessageBox::YesToAll ) {
				m_bOverwriteFiles = true;
			}
		}
		
		if( m_nInstrument > 0 ){
			m_pHydrogen->stopExportSong();
			m_bExporting = false;
		}
		
		for (auto i = 0; i < pInstrumentList->size(); i++) {
			pInstrumentList->get(i)->set_currently_exported( false );
		}
		
		pSong->getInstrumentList()->get(m_nInstrument)->set_currently_exported( true );
		
		m_pHydrogen->startExportSong( sFileName );

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


void ExportSongDialog::formatComboIndexChanged( int nIndex )
{
	const auto format = m_formatMap[ nIndex ];
	if ( format == Filesystem::AudioFormat::Unknown ) {
		ERRORLOG( QString( "Invalid index [%1]" ).arg( nIndex ) );
		return;
	}

	switch( format ) {
	case Filesystem::AudioFormat::Wav:
	case Filesystem::AudioFormat::Aiff:
	case Filesystem::AudioFormat::Aif:
	case Filesystem::AudioFormat::Aifc:
	case Filesystem::AudioFormat::Au:
	case Filesystem::AudioFormat::Caf:
	case Filesystem::AudioFormat::Voc:
	case Filesystem::AudioFormat::W64:
		sampleRateCombo->show();
		sampleRateLabel->show();
		sampleDepthCombo->show();
		sampleDepthLabel->show();
		compressionLevelSpinBox->hide();
		compressionLevelLabel->hide();
		break;
	case Filesystem::AudioFormat::Mp3:
	case Filesystem::AudioFormat::Ogg:
	case Filesystem::AudioFormat::Flac:
	case Filesystem::AudioFormat::Opus:
	default:
		sampleRateCombo->hide();
		sampleRateLabel->hide();
		sampleDepthCombo->hide();
		sampleDepthLabel->hide();
		compressionLevelSpinBox->show();
		compressionLevelLabel->show();
		break;
	}

	if ( format == Filesystem::AudioFormat::Voc ) {
		// Voc files do only support sample rates up to 16 bits
		if ( sampleDepthCombo->count() == 4 ) {
			sampleDepthCombo->removeItem( 3 );
			sampleDepthCombo->removeItem( 2 );
		}
	}
	else {
		if ( sampleDepthCombo->count() == 2 ) {
			sampleDepthCombo->addItems( QStringList() << "24" << "32" );
		}
	}

	m_sExtension = Filesystem::AudioFormatToSuffix( format );

	if ( ! exportNameTxt->text().isEmpty() ) {
		const QString sPreviousFilename = exportNameTxt->text();
		auto splitty = sPreviousFilename.split(".");
		splitty.removeLast();
		exportNameTxt->setText( QString( "%1.%2" )
								.arg( splitty.join( "." ) ).arg( m_sExtension ) );
	}
}

void ExportSongDialog::on_exportNameTxt_textChanged( const QString& )
{
	const QString sFilenameLower = exportNameTxt->text().toLower();
	okBtn->setEnabled( ! sFilenameLower.isEmpty() );

	const auto splittedFilename = exportNameTxt->text().split(".");

	const auto format = Filesystem::AudioFormatFromSuffix(
		splittedFilename.last() );

	if ( format == Filesystem::AudioFormat::Unknown ) {
		ERRORLOG( QString( "Unknown file format in filename [%1]" )
				  .arg( exportNameTxt->text() ) );
		okBtn->setEnabled( false );
		return;
	}
	const auto previousFormat = m_formatMap[ formatCombo->currentIndex() ];

	if ( previousFormat != format ) {
		for ( const auto [ nnIndex, fformat ] : m_formatMap ) {
			if ( fformat == format ) {
				formatCombo->setCurrentIndex( nnIndex );
				break;
			}
		}
	}
}

void ExportSongDialog::progressEvent( int nValue )
{
	m_pProgressBar->setValue( nValue );
	if ( nValue == 100 ) {

		m_bExporting = false;

		// Check whether an error occured during export.
		const auto pDriver = static_cast<DiskWriterDriver*>(
			Hydrogen::get_instance()->getAudioEngine()->getAudioDriver());
		if ( pDriver != nullptr && pDriver->m_bWritingFailed ) {
			m_nInstrument = 0;
			m_bExportTrackouts = false;
			QMessageBox::critical( this, "Hydrogen",
								   tr( "Unable to export song" ),
								   QMessageBox::Ok );
			m_pProgressBar->setValue( 0 );
		}
		else {
			if ( m_nInstrument ==
				 Hydrogen::get_instance()->getSong()->getInstrumentList()->size() ) {
				m_nInstrument = 0;
				m_bExportTrackouts = false;
			}

			if ( m_bExportTrackouts ) {
				exportTracks();
			}
		}
	}

	if ( nValue < 100 ) {
		closeBtn->setEnabled( false );
		resampleComboBox->setEnabled( false );
	}
	else {
		closeBtn->setEnabled( true );
		resampleComboBox->setEnabled( true );
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
					if ( pCompo != nullptr ) {
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
	}
	return false;
}
