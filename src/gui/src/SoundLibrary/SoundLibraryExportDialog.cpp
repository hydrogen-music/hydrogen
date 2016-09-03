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

#include "SoundLibraryExportDialog.h"

#include <hydrogen/hydrogen.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/h2_exception.h>

#include <hydrogen/basics/adsr.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/drumkit_component.h>
#include <QFileDialog>
#include <memory>
#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif
#if defined(H2CORE_HAVE_LIBARCHIVE)
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <cstdio>
#endif

using namespace H2Core;

const char* SoundLibraryExportDialog::__class_name = "SoundLibraryExportDialog";

SoundLibraryExportDialog::SoundLibraryExportDialog( QWidget* pParent,  const QString& selectedKit )
	: QDialog( pParent )
	, Object( __class_name )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( trUtf8( "Export Sound Library" ) );
	setFixedSize( width(), height() );
	preselectedKit = selectedKit;
	updateDrumkitList();
	drumkitPathTxt->setText( QDir::homePath() );
}




SoundLibraryExportDialog::~SoundLibraryExportDialog()
{
	INFOLOG( "DESTROY" );

	for (uint i = 0; i < drumkitInfoList.size(); i++ ) {
		Drumkit* info = drumkitInfoList[i];
		delete info;
	}
	drumkitInfoList.clear();
}



void SoundLibraryExportDialog::on_exportBtn_clicked()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	QString drumkitName = drumkitList->currentText();
	QString drumkitDir = Filesystem::drumkit_dir_search( drumkitName );
	QString saveDir = drumkitPathTxt->text();

	Preferences *pref = Preferences::get_instance();
	QDir qdTempFolder( pref->getTmpDirectory() );
	bool TmpFileCreated = false;


	int componentID = -1;
	Drumkit* info;
	if( versionList->currentIndex() == 1 ) {
		for (uint i = 0; i < drumkitInfoList.size(); i++ ) {
			info = drumkitInfoList[i];
			if( info->get_name().compare( drumkitName ) == 0 ) {
				QString temporaryDrumkitXML = qdTempFolder.filePath( "drumkit.xml" );
				INFOLOG( "[ExportSoundLibrary]" );
				INFOLOG( "Saving temporary file into: " + temporaryDrumkitXML );
				TmpFileCreated = true;
				for (std::vector<DrumkitComponent*>::iterator it = info->get_components()->begin() ; it != info->get_components()->end(); ++it) {
					DrumkitComponent* pComponent = *it;
					if( pComponent->get_name().compare( componentList->currentText() ) == 0) {
						componentID = pComponent->get_id();
						break;
					}
				}
				info->save_file( temporaryDrumkitXML, true, componentID );
				break;
			}
		}
	}

#if defined(H2CORE_HAVE_LIBARCHIVE)
	QString fullDir = drumkitDir + "/" + drumkitName;
	QDir sourceDir(fullDir);

	sourceDir.setFilter(QDir::Files);
	QStringList filesList = sourceDir.entryList();

	QString outname = saveDir + "/" + drumkitName + ".h2drumkit";

	struct archive *a;
	struct archive_entry *entry;
	struct stat st;
	char buff[8192];
	int len;
	int fd;


	a = archive_write_new();

	#if ARCHIVE_VERSION_NUMBER < 3000000
		archive_write_set_compression_gzip(a);
	#else
		archive_write_add_filter_gzip(a);
	#endif

	archive_write_set_format_pax_restricted(a);
	archive_write_open_filename(a, outname.toUtf8().constData());
	for (int i = 0; i < filesList.size(); i++) {
		QString filename = fullDir + "/" + filesList.at(i);
		QString targetFilename = drumkitName + "/" + filesList.at(i);

		if( versionList->currentIndex() == 1 ) {
			if( filesList.at(i).compare( QString("drumkit.xml") ) == 0 ) {
				filename = qdTempFolder.filePath( "drumkit.xml" );
			}
			else {
				bool bFoundFileInRightComponent = false;
				for( int j = 0; j < info->get_instruments()->size() ; j++){
					InstrumentList instrList = info->get_instruments();
					Instrument* instr = instrList[j];
					for (std::vector<InstrumentComponent*>::iterator it = instr->get_components()->begin() ; it != instr->get_components()->end(); ++it) {
						InstrumentComponent* component = *it;
						if( component->get_drumkit_componentID() == componentID ){
							for( int n = 0; n < MAX_LAYERS; n++ ) {
								InstrumentLayer* layer = component->get_layer( n );
								if( layer ) {
									 if( layer->get_sample()->get_filename().compare(filesList.at(i)) == 0 ) {
										 bFoundFileInRightComponent = true;
										 break;
									 }
								}
							}
						}
					}
				}
				if( !bFoundFileInRightComponent )
					continue;
			}
		}


		stat(filename.toUtf8().constData(), &st);
		entry = archive_entry_new();
		archive_entry_set_pathname(entry, targetFilename.toUtf8().constData());
		archive_entry_set_size(entry, st.st_size);
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, 0644);
		archive_write_header(a, entry);
		fd = ::open(filename.toUtf8().constData(), O_RDONLY);
		len = read(fd, buff, sizeof(buff));
		while ( len > 0 ) {
				archive_write_data(a, buff, len);
				len = read(fd, buff, sizeof(buff));
		}
		::close(fd);
		archive_entry_free(entry);
	}
	archive_write_close(a);

	#if ARCHIVE_VERSION_NUMBER < 3000000
		archive_write_finish(a);
	#else
		archive_write_free(a);
	#endif

	filesList.clear();

	QApplication::restoreOverrideCursor();
	QMessageBox::information( this, "Hydrogen", "Drumkit exported." );
#elif !defined(WIN32)


	if(TmpFileCreated)
	{
		/*
		 * If a temporary drumkit.xml has been created:
		 * 1. move the original drumkit.xml to drumkit_backup.xml
		 * 2. copy the temporary file to drumkitDir/drumkit.xml
		 * 3. export the drumkit
		 * 4. move the drumkit_backup.xml to drumkit.xml
		 */ 

		int ret = 0;
		
		//1.
		QString cmd = QString( "cd " ) + drumkitDir + "; " + "cp " + drumkitName + "/drumkit.xml " + drumkitName + "/drumkit_097.xml"; 
		ret = system( cmd.toLocal8Bit() );
		
		
		//2.
		cmd = QString( "cd " ) + drumkitDir + "; " + "mv " + qdTempFolder.filePath( "drumkit.xml" ) + " " + drumkitName + "/drumkit.xml"; 
		ret = system( cmd.toLocal8Bit() );
		
		//3.
		cmd =  QString( "cd " ) + drumkitDir + ";" + "tar czf \"" + saveDir + "/" + drumkitName + ".h2drumkit\" -- \"" + drumkitName + "\"";
		ret = system( cmd.toLocal8Bit() );

		//4.
		cmd = QString( "cd " ) + drumkitDir + "; " + "mv " + drumkitName + "/drumkit_097.xml " + drumkitName + "/drumkit.xml"; 
		ret = system( cmd.toLocal8Bit() );

	} else {
		QString cmd =  QString( "cd " ) + drumkitDir + ";" + "tar czf \"" + saveDir + "/" + drumkitName + ".h2drumkit\" -- \"" + drumkitName + "\"";
		int ret = system( cmd.toLocal8Bit() );
	}



	QApplication::restoreOverrideCursor();
	QMessageBox::information( this, "Hydrogen", "Drumkit exported." );
#else
	QApplication::restoreOverrideCursor();
	QMessageBox::information( this, "Hydrogen", "Drumkit not exported. Operation not supported." );
#endif
}

void SoundLibraryExportDialog::on_drumkitPathTxt_textChanged( QString str )
{
	QString path = drumkitPathTxt->text();
	if (path.isEmpty()) {
		exportBtn->setEnabled( false );
	}
	else {
		exportBtn->setEnabled( true );
	}
}

void SoundLibraryExportDialog::on_browseBtn_clicked()
{
	static QString lastUsedDir = QDir::homePath();
	QString filename = QFileDialog::getExistingDirectory (this, tr("Directory"), lastUsedDir);
	if ( filename.isEmpty() ) {
		drumkitPathTxt->setText( QDir::homePath() );
	}
	else
	{
		drumkitPathTxt->setText( filename );
		lastUsedDir = filename;
	}
}

void SoundLibraryExportDialog::on_cancelBtn_clicked()
{
	accept();
}

void SoundLibraryExportDialog::on_drumkitList_currentIndexChanged( QString str )
{
	componentList->clear();

	QStringList p_compoList = kit_components[str];

	for (QStringList::iterator it = p_compoList.begin() ; it != p_compoList.end(); ++it) {
		QString p_compoName = *it;

		componentList->addItem( p_compoName );
	}
}

void SoundLibraryExportDialog::on_versionList_currentIndexChanged( int index )
{
	if( index == 0 )
		componentList->setEnabled( false );
	else if( index == 1 )
		componentList->setEnabled(  true );
}

void SoundLibraryExportDialog::updateDrumkitList()
{
	INFOLOG( "[updateDrumkitList]" );

	drumkitList->clear();

	for (uint i = 0; i < drumkitInfoList.size(); i++ ) {
		Drumkit* info = drumkitInfoList[i];
		delete info;
	}
	drumkitInfoList.clear();

	QStringList sysDrumkits = Filesystem::sys_drumkits_list();
	for (int i = 0; i < sysDrumkits.size(); ++i) {
		QString absPath = Filesystem::sys_drumkits_dir() + "/" + sysDrumkits.at(i);
		Drumkit *info = Drumkit::load( absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			drumkitList->addItem( info->get_name() );
			QStringList p_components;
			for (std::vector<DrumkitComponent*>::iterator it = info->get_components()->begin() ; it != info->get_components()->end(); ++it) {
				DrumkitComponent* p_compo = *it;
				p_components.append(p_compo->get_name());
			}
			kit_components[info->get_name()] = p_components;
		}
	}

	QStringList userDrumkits = Filesystem::usr_drumkits_list();
	for (int i = 0; i < userDrumkits.size(); ++i) {
		QString absPath = Filesystem::usr_drumkits_dir() + "/" + userDrumkits.at(i);
		Drumkit *info = Drumkit::load( absPath );
		if (info) {
			drumkitInfoList.push_back( info );
			drumkitList->addItem( info->get_name() );
			QStringList p_components;
			for (std::vector<DrumkitComponent*>::iterator it = info->get_components()->begin() ; it != info->get_components()->end(); ++it) {
				DrumkitComponent* p_compo = *it;
				p_components.append(p_compo->get_name());
			}
			kit_components[info->get_name()] = p_components;
		}
	}

	/*
	 * If the export dialog was called from the soundlibrary panel via right click on
	 * a soundlibrary, the variable preselectedKit holds the name of the selected drumkit
	 */

	int index = drumkitList->findText( preselectedKit );
	if ( index >= 0) {
		drumkitList->setCurrentIndex( index );
	}
	else {
		drumkitList->setCurrentIndex( 0 );
	}

	on_drumkitList_currentIndexChanged( drumkitList->currentText() );
	on_versionList_currentIndexChanged( 0 );
}
