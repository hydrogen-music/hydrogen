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

#include <QtGui>
#include <QtWidgets>

#include "../HydrogenApp.h"
#include "../Skin.h"

#include "SoundLibraryPropertiesDialog.h"
#include "../InstrumentRack.h"
#include "SoundLibraryPanel.h"
#include <core/Hydrogen.h>

namespace H2Core
{

const char* SoundLibraryPropertiesDialog::__class_name = "SoundLibraryPropertiesDialog";

SoundLibraryPropertiesDialog::SoundLibraryPropertiesDialog( QWidget* pParent, Drumkit *pDrumkitInfo, Drumkit *pPreDrumkit )
 : QDialog( pParent )
 , Object( __class_name )
 , m_pDrumkitInfo( pDrumkitInfo )
 , m_pPreDrumkitInfo( pPreDrumkit )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( tr( "SoundLibrary Properties" ) );
	setFixedSize( width(), height() );

	//display the current drumkit infos into the qlineedit
	if ( pDrumkitInfo != nullptr ){
		nameTxt->setText( QString( pDrumkitInfo->get_name() ) );
		authorTxt->setText( QString( pDrumkitInfo->get_author() ) );
		infoTxt->append( QString( pDrumkitInfo->get_info() ) );
		licenseTxt->setText( QString( pDrumkitInfo->get_license() ) );
		imageText->setText( QString ( pDrumkitInfo->get_image() ) );
		imageLicenseText->setText( QString ( pDrumkitInfo->get_image_license() ) );
		// Licence with attribution is often too long...
		imageLicenseText->setToolTip( QString( pDrumkitInfo->get_image_license() ) );
	}

}

/// On showing the dialog (after layout sizes have been applied), load the drumkit image if any.
void SoundLibraryPropertiesDialog::showEvent( QShowEvent *e )
{
	if ( m_pDrumkitInfo != nullptr ) {
		QString sImage = m_pDrumkitInfo->get_path() + "/" + m_pDrumkitInfo->get_image();
		updateImage( sImage );
	}
}


SoundLibraryPropertiesDialog::~SoundLibraryPropertiesDialog()
{
	INFOLOG( "DESTROY" );

}

void SoundLibraryPropertiesDialog::updateImage( QString& filename )
{
	QPixmap *pPixmap = new QPixmap ( filename );
	// scale the image down to fit if required
	int x = (int) drumkitImageLabel->size().width();
	int y = drumkitImageLabel->size().height();
	float labelAspect = (float) x / y;
	float imageAspect = (float) pPixmap->width() / pPixmap->height();

	if ( ( x < pPixmap->width() ) || ( y < pPixmap->height() ) )
	{
		if ( labelAspect >= imageAspect )
		{
			// image is taller or the same as label frame
			*pPixmap = pPixmap->scaledToHeight( y );
		}
		else
		{
			// image is wider than label frame
			*pPixmap = pPixmap->scaledToWidth( x );
		}
	}
	drumkitImageLabel->setPixmap(*pPixmap);
	drumkitImageLabel->show();

}

void SoundLibraryPropertiesDialog::on_imageBrowsePushButton_clicked()
{
	// Try to get the drumkit directory and open file browser
	QString drumkitDir = m_pDrumkitInfo->get_path();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), drumkitDir, tr("Image Files (*.png *.jpg *.jpeg)"));

	// If cancel was clicked just abort
	if ( fileName == nullptr )
	{
		return;
	}

	// If this file is in different directory copy it here
	
	QFile file( fileName );
	QFileInfo fileInfo(file.fileName());

	if ( fileInfo.dir().path() != drumkitDir )
	{
		INFOLOG("Copying " + fileName + " to " + drumkitDir.toLocal8Bit() );
		if ( !QFile::copy( fileName, drumkitDir + "/" + fileInfo.fileName() ))
		{
			WARNINGLOG( "Could not copy " + fileInfo.fileName() + " to " + drumkitDir );
		}

	}
	QString filename(fileInfo.fileName());
	imageText->setText( filename );
	m_pDrumkitInfo->set_image( filename );
	updateImage( fileName );
}

void SoundLibraryPropertiesDialog::on_saveBtn_clicked()
{

	bool reload = false;

	if ( saveChanges_checkBox->isChecked() ){
		//test if the drumkit is loaded
		if ( m_pDrumkitInfo->get_name() != m_pPreDrumkitInfo->get_name() ||
			 m_pDrumkitInfo->isUserDrumkit() != m_pPreDrumkitInfo->isUserDrumkit() ){
			QMessageBox::information( this, "Hydrogen", tr ( "This is not possible, you can only save changes inside instruments to the current loaded sound library"));
			saveChanges_checkBox->setChecked( false );
			return;
		}
		reload = true;
	}

	//load the selected drumkit to save it correct.... later the old drumkit will be reloaded
	if ( m_pDrumkitInfo != nullptr && ( !saveChanges_checkBox->isChecked() ) ){
		if ( m_pPreDrumkitInfo->get_name() != m_pDrumkitInfo->get_name() ||
			 m_pPreDrumkitInfo->isUserDrumkit() != m_pDrumkitInfo->isUserDrumkit() ){
			Hydrogen::get_instance()->loadDrumkit( m_pDrumkitInfo );
			Hydrogen::get_instance()->getSong()->setIsModified( true );
		}
	}

	// Check the drumkit name. if the name is a new one, one qmessagebox with question "are you sure" will displayed.
	if ( nameTxt->text() != m_pDrumkitInfo->get_name()  ){
		int res = QMessageBox::information( this, "Hydrogen", tr( "Warning! Changing the drumkit name will result in creating a new drumkit with this name.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 );
		if ( res == 1 ) {
			return;
		}
		else
		{
			reload = true;
		}
	}
	
	//check the name and set the drumkitinfo to current drumkit
	if ( m_pDrumkitInfo != nullptr && !nameTxt->text().isEmpty() ){
		m_pDrumkitInfo->set_name( nameTxt->text() );
		m_pDrumkitInfo->set_author( authorTxt->text() );
		m_pDrumkitInfo->set_info( infoTxt->toHtml() );
		m_pDrumkitInfo->set_license( licenseTxt->text() );
		m_pDrumkitInfo->set_image( imageText->text() );
		m_pDrumkitInfo->set_image_license( imageLicenseText->text() );
	}

	//save the drumkit
	// Note: The full path of the image is passed to make copying to a new drumkit easy
	if( m_pDrumkitInfo != nullptr ) {
		if( !H2Core::Drumkit::save( nameTxt->text(),
									authorTxt->text(),
									infoTxt->toHtml(),
									licenseTxt->text(),
									m_pDrumkitInfo->get_path() + "/" + m_pDrumkitInfo->get_image(),
									m_pDrumkitInfo->get_image_license(),
									H2Core::Hydrogen::get_instance()->getSong()->getInstrumentList(),
									H2Core::Hydrogen::get_instance()->getSong()->getComponents(),
									true ) ) {
			QMessageBox::information( this, "Hydrogen", tr ( "Saving of this drumkit failed."));
		}
	}

	//check pre loaded drumkit name  and reload the old drumkit
	if ( m_pPreDrumkitInfo != nullptr ){
		if ( m_pPreDrumkitInfo->get_name() != Hydrogen::get_instance()->getCurrentDrumkitName() ||
			 m_pPreDrumkitInfo->isUserDrumkit() != m_pDrumkitInfo->isUserDrumkit() ) {
			Hydrogen::get_instance()->loadDrumkit( m_pPreDrumkitInfo );
			Hydrogen::get_instance()->getSong()->setIsModified( true );
		}
	}

	//reload if necessary
	if ( reload == true ){
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
	}

	accept();

}

}
