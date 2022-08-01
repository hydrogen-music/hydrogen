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
#include "../HydrogenApp.h"
#include "../CommonStrings.h"

#include <core/Helpers/Filesystem.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/DrumkitComponent.h>

using namespace H2Core;

SoundLibraryExportDialog::SoundLibraryExportDialog( QWidget* pParent,
													std::shared_ptr<Drumkit> pDrumkit )
	: QDialog( pParent ),
	  m_pDrumkit( pDrumkit )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	setupUi( this );

	exportBtn->setText( tr( "&Export" ) );
	cancelBtn->setText( pCommonStrings->getButtonCancel() );
	
	setWindowTitle( QString( "%1 [%2]" )
					.arg( tr( "Export Drumkit" ) )
					.arg( pDrumkit != nullptr ? pDrumkit->get_name() : tr( "invalid drumkit" ) ) );
	adjustSize();
	setFixedSize( width(), height() );
	drumkitPathTxt->setText( Preferences::get_instance()->getLastExportDrumkitDirectory() );

	if ( pDrumkit != nullptr ) {
		for ( const auto& pComponent : *pDrumkit->get_components() ) {
			m_components.append( pComponent->get_name() );
		}

		updateComponentList();
	}
}

SoundLibraryExportDialog::~SoundLibraryExportDialog()
{
}



void SoundLibraryExportDialog::on_exportBtn_clicked()
{
	if ( m_pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	if ( ! HydrogenApp::checkDrumkitLicense( m_pDrumkit ) ) {
		ERRORLOG( "User cancelled dialog due to licensing issues." );
		return;
	}
		
	bool bRecentVersion = versionList->currentIndex() == 1 ? false : true;

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
		m_pDrumkit->getExportName( sTargetComponent, bRecentVersion ) +
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
	
	if ( ! m_pDrumkit->exportTo( drumkitPathTxt->text(), // Target folder
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

void SoundLibraryExportDialog::on_versionList_currentIndexChanged( int index )
{
	updateComponentList();
}

void SoundLibraryExportDialog::updateComponentList( )
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

	for ( const auto& sComponentName : m_components ) {
		componentList->addItem( sComponentName );
	}
}
