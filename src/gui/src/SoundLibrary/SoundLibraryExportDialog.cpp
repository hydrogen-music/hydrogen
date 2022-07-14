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

#include "SoundLibraryExportDialog.h"
#include "SoundLibraryPanel.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "../InstrumentRack.h"

#include <core/Hydrogen.h>
#include <core/Helpers/Filesystem.h>
#include <core/Preferences/Preferences.h>
#include <core/H2Exception.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QFileDialog>
#include <QtGui>
#include <QtWidgets>

#include <memory>

#if defined(H2CORE_HAVE_LIBARCHIVE)
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <cstdio>
#endif

using namespace H2Core;

SoundLibraryExportDialog::SoundLibraryExportDialog( QWidget* pParent,
													const QString& sSelectedDrumkitPath )
	: QDialog( pParent )
	, m_sSelectedDrumkitPath( sSelectedDrumkitPath )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	setupUi( this );

	updateDrumkitList();

	exportBtn->setText( tr( "&Export" ) );
	cancelBtn->setText( pCommonStrings->getButtonCancel() );
	
	setWindowTitle( tr( "Export Sound Library" ) );
	adjustSize();
	setFixedSize( width(), height() );
	drumkitPathTxt->setText( Preferences::get_instance()->getLastExportDrumkitDirectory() );
}




SoundLibraryExportDialog::~SoundLibraryExportDialog()
{
}



void SoundLibraryExportDialog::on_exportBtn_clicked()
{
	bool bRecentVersion = versionList->currentIndex() == 1 ? false : true;

	QString sDrumkitLabel = drumkitListComboBox->currentText();
	QString sDrumkitPath = HydrogenApp::get_instance()->getInstrumentRack()
		->getSoundLibraryPanel()->getDrumkitPath( sDrumkitLabel );

	auto pDrumkit = Hydrogen::get_instance()->getSoundLibraryDatabase()
		->getDrumkit( sDrumkitPath );
	if ( pDrumkit == nullptr ) {
		QMessageBox::critical( this, "Hydrogen",
							   tr("Unable to retrieve drumkit from sound library" ) );
		return;
	}

	QString sTargetComponent;
	if ( componentList->currentIndex() == 0 && bRecentVersion ) {
		// Exporting all components
		sTargetComponent = "";
	} else {
		sTargetComponent = componentList->currentText();
	}

	// Check whether the resulting file does already exist and ask the
	// user if it should be overwritten.
	QString sTargetName = drumkitPathTxt->text() + "/" +
		pDrumkit->getExportName( sTargetComponent, bRecentVersion ) +
		Filesystem::drumkit_ext;
	
	if ( Filesystem::file_exists( sTargetName, true ) ) {
		auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
		QMessageBox msgBox;
		msgBox.setWindowTitle("Hydrogen");
		msgBox.setIcon( QMessageBox::Warning );
		msgBox.setText( tr( "The file [%1] does already exist and will be overwritten.")
						.arg( sTargetName ) );

		msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
		msgBox.setButtonText(QMessageBox::Ok,
							 pCommonStrings->getButtonOk() );
		msgBox.setButtonText(QMessageBox::Cancel,
							 pCommonStrings->getButtonCancel());
		msgBox.setDefaultButton(QMessageBox::Ok);

		if ( msgBox.exec() == QMessageBox::Cancel ) {
			return;
		}
	}
	
	QApplication::setOverrideCursor(Qt::WaitCursor);
	
	if ( ! pDrumkit->exportTo( drumkitPathTxt->text(), // Target folder
							   sTargetComponent, // Selected component
							   bRecentVersion ) ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, "Hydrogen", tr("Unable to export drumkit") );
		return;
	}

	QApplication::restoreOverrideCursor();
	QMessageBox::information( this, "Hydrogen",
							  tr("Drumkit exported to") + "\n" +
							  sTargetName );
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
	QString sPath = Preferences::get_instance()->getLastExportDrumkitDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = QDir::homePath();
	}

	QString filename = QFileDialog::getExistingDirectory( this, tr("Directory"), sPath );
	if ( filename.isEmpty() ) {
		drumkitPathTxt->setText( sPath );
	} else {
		drumkitPathTxt->setText( filename );
		Preferences::get_instance()->setLastExportDrumkitDirectory( filename );
	}
}

void SoundLibraryExportDialog::on_cancelBtn_clicked()
{
	accept();
}

void SoundLibraryExportDialog::on_drumkitListComboBox_currentIndexChanged( QString str )
{
	componentList->clear();

	if ( versionList->currentIndex() == 0 ) {
		// Only kit version 0.9.7 or newer support components. For
		// them we can support to export all or individual ones. For
		// older versions one component must pretend to be the whole
		// kit.
		componentList->addItem( tr( "All" ) );
		componentList->insertSeparator( 1 );
	}

	QStringList p_compoList = m_kit_components[str];

	for (QStringList::iterator it = p_compoList.begin() ; it != p_compoList.end(); ++it) {
		QString p_compoName = *it;

		componentList->addItem( p_compoName );
	}
}

void SoundLibraryExportDialog::on_versionList_currentIndexChanged( int index )
{
	on_drumkitListComboBox_currentIndexChanged( drumkitListComboBox->currentText() );
}

void SoundLibraryExportDialog::updateDrumkitList()
{
	auto pSoundLibraryDatabase = Hydrogen::get_instance()->getSoundLibraryDatabase();
	auto pSoundLibraryPanel = HydrogenApp::get_instance()->getInstrumentRack()
		->getSoundLibraryPanel();

	drumkitListComboBox->clear();

	for ( const auto& pDrumkitEntry : pSoundLibraryDatabase->getDrumkitDatabase() ) {
		auto pDrumkit = pDrumkitEntry.second;
		QString sDrumkitLabel = pSoundLibraryPanel->getDrumkitLabel( pDrumkitEntry.first );

		DEBUGLOG( QString( "first: %1, label: %2" ).arg( pDrumkitEntry.first ).arg( sDrumkitLabel ) );
		if ( pDrumkit != nullptr && ! sDrumkitLabel.isEmpty() ) {
			drumkitListComboBox->addItem( sDrumkitLabel );
			QStringList components;
			for ( auto pComponent : *(pDrumkit->get_components() ) ) {
				components.append( pComponent->get_name() );
			}
			m_kit_components[ sDrumkitLabel ] = components;
		}
	}

	int nIndex = drumkitListComboBox->findText( m_sSelectedDrumkitPath );
	if ( nIndex >= 0 ) {
		drumkitListComboBox->setCurrentIndex( nIndex );
	}
	else {
		drumkitListComboBox->setCurrentIndex( 0 );
	}

	on_drumkitListComboBox_currentIndexChanged( drumkitListComboBox->currentText() );
	on_versionList_currentIndexChanged( 0 );
}
