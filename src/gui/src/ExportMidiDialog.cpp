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

#include <QLabel>

#include "ExportMidiDialog.h"

#include "CommonStrings.h"
#include "HydrogenApp.h"
#include "Widgets/FileDialog.h"

#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/SMF/SMF.h>

using namespace H2Core;

enum ExportModes { EXPORT_SMF1_SINGLE, EXPORT_SMF1_MULTI, EXPORT_SMF0 };

// Here we are going to store export filename 
QString ExportMidiDialog::sLastFilename = "";

ExportMidiDialog::ExportMidiDialog( QWidget* parent )
	: QDialog( parent )
	, Object()
	, m_bFileSelected( false )
	, m_sExtension( ".mid" )
{
	setupUi( this );
	setModal( true );
	setWindowTitle( tr( "Export midi" ) );

	exportTypeCombo->addItem( tr("SMF1 single: export all instruments to a single track") );
	exportTypeCombo->addItem( tr("SMF1 multi: export each instrument to separate track") );
	exportTypeCombo->addItem( tr("SMF0: export all events to one track") );

	// loading previous directory and filling filename text field
	// loading default filename on a first run
	if ( sLastFilename.isEmpty() ) {
		sLastFilename = createDefaultFilename();
	}

	const auto pPref = Preferences::get_instance();

	QDir lastExportDir = QDir( pPref->getLastExportMidiDirectory() );

	// joining filepath with dirname
	const QString sFullPath = lastExportDir.absoluteFilePath( sLastFilename );
	exportNameTxt->setText( sFullPath );

	// loading rest of the options
	exportTypeCombo->setCurrentIndex( pPref->getMidiExportMode() );

	humanizationCheckBox->setChecked( pPref->getMidiExportUseHumanization() );

	adjustSize();
}

ExportMidiDialog::~ExportMidiDialog()
{
}

QString ExportMidiDialog::createDefaultFilename()
{
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return "";
	}
	QString sDefaultFilename = pSong->getFilename();

	if( sDefaultFilename.isEmpty() ){
		sDefaultFilename = pSong->getName();
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

void ExportMidiDialog::on_browseBtn_clicked()
{
	QString sPath = Preferences::get_instance()->getLastExportMidiDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::usr_data_path();
	}
	
	FileDialog fd( this );

	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( tr("Midi file (*%1)").arg( m_sExtension ) );
	fd.setDirectory( sPath );
	fd.setWindowTitle( tr( "Export MIDI file" ) );
	fd.setAcceptMode( QFileDialog::AcceptSave );

	QString sDefaultFilename = exportNameTxt->text();
	fd.selectFile( sDefaultFilename );

	QString sFilename;
	if ( fd.exec() == QDialog::Accepted ) {
		m_bFileSelected = true;
		sFilename = fd.selectedFiles().first();
	}

	if ( sFilename.isEmpty() ) {
		return;
	}

	if ( sFilename.endsWith( m_sExtension ) == false ) {
		sFilename += m_sExtension;
	}

	exportNameTxt->setText( sFilename );
}

bool ExportMidiDialog::validateUserInput( ) 
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

void ExportMidiDialog::on_okBtn_clicked()
{
	auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	if ( !validateUserInput() ) {
		return;
	}
	
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	auto pPref = Preferences::get_instance();
	pPref->setMidiExportMode( exportTypeCombo->currentIndex() );

	// extracting dirname from export box
	const QString sFilename = exportNameTxt->text();
	QFileInfo info( sFilename );
	QDir dir = info.absoluteDir();
	if ( !dir.exists() ) {
		// very strange if it happens but better to check for it anyway
		return;
	}

	sLastFilename = info.fileName();
	pPref->setLastExportMidiDirectory( dir.absolutePath() );

	if ( ! Filesystem::dir_writable(  info.absoluteDir().absolutePath(), false ) ) {
		QMessageBox::warning( this, "Hydrogen",
							  pCommonStrings->getFileDialogMissingWritePermissions(),
							  QMessageBox::Ok );
		return;
	}
	
	if ( info.exists() == true && m_bFileSelected == false ) {
		int res = QMessageBox::information( this, "Hydrogen", tr( "The file %1 exists. \nOverwrite the existing file?").arg(sFilename), QMessageBox::Yes | QMessageBox::No );
		if ( res == QMessageBox::No ) {
			return;
		}
	}

	// choosing writer
	std::shared_ptr<SMFWriter> pSmfWriter = nullptr;
	if ( exportTypeCombo->currentIndex() == EXPORT_SMF1_SINGLE ){
		pSmfWriter = std::make_shared<SMF1WriterSingle>();
	}
	else if ( exportTypeCombo->currentIndex() == EXPORT_SMF1_MULTI ){
		pSmfWriter = std::make_shared<SMF1WriterMulti>();
	}
	else if ( exportTypeCombo->currentIndex() == EXPORT_SMF0 ){
		pSmfWriter = std::make_shared<SMF0Writer>();
	}
	else {
		ERRORLOG( QString( "Unknown index [%1]" )
				  .arg( exportTypeCombo->currentIndex() ) );
		return;
	}

	pPref->setMidiExportUseHumanization( humanizationCheckBox->isChecked() );
	
	pSmfWriter->save( sFilename, pSong );

	// Check whether same time signature were off.
	const auto timeSignatureFailures = pSmfWriter->getTimeSignatureFailures();
	if ( timeSignatureFailures.size() > 0 ) {
		QStringList informative;
		for ( const auto& ffailure : timeSignatureFailures ) {
			informative << QString( "[%1]: %2/%3 -> %4/%5" )
				.arg( ffailure.nColumn )
				.arg( ffailure.fOldNumerator ).arg( ffailure.nOldDenominator )
				.arg( ffailure.nNewNumerator ).arg( ffailure.nNewDenominator );
		}

		QMessageBox msgBox;
		msgBox.setText( tr( "Time signature of some columns needed to be change.\nPlease make the numerator an integers and the denominator a power of two (like 1, 2, 4, 8, 16...)." ) );
		msgBox.setInformativeText( informative.join( "\n" ) );
		msgBox.setStandardButtons( QMessageBox::Ok );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Warning );
		msgBox.exec();
	}

	accept();
}

void ExportMidiDialog::on_closeBtn_clicked()
{
	accept();
}

void ExportMidiDialog::on_exportNameTxt_textChanged( const QString& )
{
	QString filename = exportNameTxt->text();
	if ( !filename.isEmpty() ) {
		okBtn->setEnabled( true );
	}
	else {
		okBtn->setEnabled( false );
	}
}
