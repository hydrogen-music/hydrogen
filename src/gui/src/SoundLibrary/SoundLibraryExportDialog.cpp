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

SoundLibraryExportDialog::SoundLibraryExportDialog( QWidget* pParent,  const QString& sSelectedKit, H2Core::Filesystem::Lookup lookup )
	: QDialog( pParent )
	, m_sPreselectedKit( sSelectedKit )
	, m_preselectedKitLookup( lookup )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	// updating the drumkit list might take a while. Therefore, we
	// show the user that this is expected behavior by showing a wait cursor.
	QApplication::setOverrideCursor(Qt::WaitCursor);
	
	setupUi( this );

	exportBtn->setText( tr( "&Export" ) );
	cancelBtn->setText( pCommonStrings->getButtonCancel() );
	
	setWindowTitle( tr( "Export Sound Library" ) );
	m_sSysDrumkitSuffix = " (system)";
	updateDrumkitList();
	adjustSize();
	setFixedSize( width(), height() );
	drumkitPathTxt->setText( Preferences::get_instance()->getLastExportDrumkitDirectory() );
	
	QApplication::restoreOverrideCursor();
}




SoundLibraryExportDialog::~SoundLibraryExportDialog()
{
	INFOLOG( "DESTROY" );

	for (uint i = 0; i < m_pDrumkitInfoList.size(); i++ ) {
		Drumkit* info = m_pDrumkitInfoList[i];
		delete info;
	}
	m_pDrumkitInfoList.clear();
}



void SoundLibraryExportDialog::on_exportBtn_clicked()
{
	bool bRecentVersion = versionList->currentIndex() == 1 ? false : true;

	// The name of the drumkit is not something well defined within
	// Hydrogen. Drumkit::load_by_name is expecting the name of the
	// drumkit folder containing both the samples and drumkit.xml
	// files. However, the name property stored in the latter, which
	// is also used as Drumkit::__name can differ (and they actually
	// do for a number of kits we host at SourceForge). Therefore,
	// it's important to retrieve the kit from the list of drumkits
	// used to create the different choices presented in the GUI.
	Drumkit* pDrumkit = nullptr;
	for ( const auto& ppKit : m_pDrumkitInfoList ) {
		if ( ppKit->isUserDrumkit() ) {
			if ( ppKit->get_name().compare( drumkitList->currentText() ) == 0 ) {
				pDrumkit = ppKit;
				break;
			}
		} else {
			QString	sChosenName = drumkitList->currentText();
			if ( sChosenName.contains( m_sSysDrumkitSuffix ) ) {
				sChosenName.replace( m_sSysDrumkitSuffix, "" );
				if ( ppKit->get_name().compare( sChosenName ) == 0 ) {
					pDrumkit = ppKit;
					break;
				}
			}
		}
	}

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

void SoundLibraryExportDialog::on_drumkitList_currentIndexChanged( QString str )
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
	on_drumkitList_currentIndexChanged( drumkitList->currentText() );
}

void SoundLibraryExportDialog::updateDrumkitList()
{
	INFOLOG( "[updateDrumkitList]" );

	drumkitList->clear();

	for ( auto pDrumkitInfo : m_pDrumkitInfoList ) {
		delete pDrumkitInfo;
	}
	m_pDrumkitInfoList.clear();

	QStringList sysDrumkits = Filesystem::sys_drumkit_list();
	QString sDrumkitName;
	for (int i = 0; i < sysDrumkits.size(); ++i) {
		QString absPath = Filesystem::sys_drumkits_dir() + sysDrumkits.at(i);
		Drumkit *info = Drumkit::load( absPath, false );
		if (info) {
			m_pDrumkitInfoList.push_back( info );
			sDrumkitName = info->get_name() + m_sSysDrumkitSuffix;
			drumkitList->addItem( sDrumkitName );
			QStringList p_components;
			for ( auto pComponent : *(info->get_components() ) ) {
				p_components.append( pComponent->get_name() );
			}
			m_kit_components[ sDrumkitName ] = p_components;
		}
	}

	drumkitList->insertSeparator( drumkitList->count() );

	QStringList userDrumkits = Filesystem::usr_drumkit_list();
	for (int i = 0; i < userDrumkits.size(); ++i) {
		QString absPath = Filesystem::usr_drumkits_dir() + userDrumkits.at(i);
		Drumkit *info = Drumkit::load( absPath, false );
		if (info) {
			m_pDrumkitInfoList.push_back( info );
			drumkitList->addItem( info->get_name() );
			QStringList p_components;
			for ( auto pComponent : *(info->get_components() ) ) {
				p_components.append(pComponent->get_name());
			}
			m_kit_components[info->get_name()] = p_components;
		}
	}

	/*
	 * If the export dialog was called from the soundlibrary panel via right click on
	 * a soundlibrary, the variable preselectedKit holds the name of the selected drumkit
	 */
	if ( m_preselectedKitLookup == Filesystem::Lookup::system ) {
		m_sPreselectedKit.append( m_sSysDrumkitSuffix );
	}

	int index = drumkitList->findText( m_sPreselectedKit );
	if ( index >= 0) {
		drumkitList->setCurrentIndex( index );
	}
	else {
		drumkitList->setCurrentIndex( 0 );
	}

	on_drumkitList_currentIndexChanged( drumkitList->currentText() );
	on_versionList_currentIndexChanged( 0 );
}
