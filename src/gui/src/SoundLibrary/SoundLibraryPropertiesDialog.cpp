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

#include <QtGui>

#include "../HydrogenApp.h"
#include "../Skin.h"

#include "SoundLibraryPropertiesDialog.h"
#include "../InstrumentRack.h"
#include "SoundLibraryPanel.h"
#include <hydrogen/hydrogen.h>

namespace H2Core
{

//globals
Drumkit *drumkitinfo = NULL ;
Drumkit *predrumkit = NULL;
QString oldName;

const char* SoundLibraryPropertiesDialog::__class_name = "SoundLibraryPropertiesDialog";

SoundLibraryPropertiesDialog::SoundLibraryPropertiesDialog( QWidget* pParent, Drumkit *drumkitInfo, Drumkit *preDrumKit )
 : QDialog( pParent )
 , Object( __class_name )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( trUtf8( "SoundLibrary Properties" ) );
	setFixedSize( width(), height() );
	predrumkit = preDrumKit;

	//display the current drumkit infos into the qlineedit
	if ( drumkitInfo != NULL ){
		drumkitinfo = drumkitInfo;
		nameTxt->setText( QString( drumkitInfo->get_name() ) );
		oldName = drumkitInfo->get_name();
		authorTxt->setText( QString( drumkitInfo->get_author() ) );
		infoTxt->append( QString( drumkitInfo->get_info() ) );
		licenseTxt->setText( QString( drumkitInfo->get_license() ) );
		imageText->setText( QString ( drumkitInfo->get_image() ) );
		imageLicenseText->setText( QString ( drumkitInfo->get_image_license() ) );

		QPixmap *pixmap = new QPixmap (drumkitInfo->get_path() + "/" + drumkitInfo->get_image());
		// scale the image down to fit if required
		int x = (int) drumkitImageLabel->size().width();
		int y = drumkitImageLabel->size().height();
		float labelAspect = (float) x / y;
		float imageAspect = (float) pixmap->width() / pixmap->height();

		if ( ( x < pixmap->width() ) || ( y < pixmap->height() ) )
		{
			if ( labelAspect >= imageAspect )
			{
				// image is taller or the same as label frame
				*pixmap = pixmap->scaledToHeight( y );
			}
			else
			{
				// image is wider than label frame
				*pixmap = pixmap->scaledToWidth( x );
			}
		}
		drumkitImageLabel->setPixmap(*pixmap);
		drumkitImageLabel->show();

	}

}


SoundLibraryPropertiesDialog::~SoundLibraryPropertiesDialog()
{
	INFOLOG( "DESTROY" );

}

void SoundLibraryPropertiesDialog::updateImage( QString& filename )
{
	QPixmap *pixmap = new QPixmap ( filename );
	// scale the image down to fit if required
	int x = (int) drumkitImageLabel->size().width();
	int y = drumkitImageLabel->size().height();
	float labelAspect = (float) x / y;
	float imageAspect = (float) pixmap->width() / pixmap->height();

	if ( ( x < pixmap->width() ) || ( y < pixmap->height() ) )
	{
		if ( labelAspect >= imageAspect )
		{
			// image is taller or the same as label frame
			*pixmap = pixmap->scaledToHeight( y );
		}
		else
		{
			// image is wider than label frame
			*pixmap = pixmap->scaledToWidth( x );
		}
	}
	drumkitImageLabel->setPixmap(*pixmap);
	drumkitImageLabel->show();

}

void SoundLibraryPropertiesDialog::on_imageBrowsePushButton_clicked()
{
	// Try to get the drumkit directory and open file browser
	QString drumkitDir = Filesystem::drumkit_dir_search( nameTxt->text() ) + "/" + nameTxt->text();

	QString fileName = QFileDialog::getOpenFileName(this, trUtf8("Open Image"), drumkitDir, trUtf8("Image Files (*.png *.jpg *.jpeg)"));

	// If cancel was clicked just abort
	if ( fileName == NULL )
	{
		return;
	}

	// If this file is in different directory copy it here
	
	QFile file( fileName );
	QFileInfo fileInfo(file.fileName());
	//ERRORLOG(fileInfo.dir().path().toLocal8Bit() + drumkitDir);
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
	drumkitinfo->set_image( filename );
	updateImage( fileName );
}

void SoundLibraryPropertiesDialog::on_saveBtn_clicked()
{

	bool reload = false;

	if ( saveChanges_checkBox->isChecked() ){
		//test if the drumkit is loaded
		if ( Hydrogen::get_instance()->getCurrentDrumkitname() != drumkitinfo->get_name() ){
			QMessageBox::information( this, "Hydrogen", trUtf8 ( "This is not possible, you can only save changes inside instruments to the current loaded sound library"));
			saveChanges_checkBox->setChecked( false );
			return;
		}
		reload = true;
	}

	//load the selected drumkit to save it correct.... later the old drumkit will be reloaded
	if ( drumkitinfo != NULL && ( !saveChanges_checkBox->isChecked() ) ){
		if ( Hydrogen::get_instance()->getCurrentDrumkitname() != drumkitinfo->get_name() ){
			Hydrogen::get_instance()->loadDrumkit( drumkitinfo );
			Hydrogen::get_instance()->getSong()->set_is_modified( true );
		}
	}

	//check the drumkit name. if the name is a new one, one qmessagebox with question "are you sure" will displayed.
	if ( nameTxt->text() != oldName  ){
		int res = QMessageBox::information( this, "Hydrogen", tr( "Warning! Changing the drumkit name will result in creating a new drumkit with this name.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
		if ( res == 1 ) {
			return;
		}
		else
		{
			reload = true;
		}
	}
	
	//save the drumkit
	// Note: The full path of the image is passed to make copying to a new drumkit easy
	if( !H2Core::Drumkit::save( nameTxt->text(), authorTxt->text(), infoTxt->toHtml(), licenseTxt->text(), drumkitinfo->get_path() + "/" + drumkitinfo->get_image(), drumkitinfo->get_image_license(), H2Core::Hydrogen::get_instance()->getSong()->get_instrument_list(), H2Core::Hydrogen::get_instance()->getSong()->get_components(), true ) ) {
		QMessageBox::information( this, "Hydrogen", trUtf8 ( "Saving of this drumkit failed."));
	}

	//check the name and set the drumkitinfo to current drumkit
	if ( drumkitinfo != NULL && !nameTxt->text().isEmpty() ){
		drumkitinfo->set_name( nameTxt->text() );
		drumkitinfo->set_author( authorTxt->text() );
		drumkitinfo->set_info( infoTxt->toHtml() );
		drumkitinfo->set_license( licenseTxt->text() );
		drumkitinfo->set_image( imageText->text() );
		drumkitinfo->set_image_license( imageLicenseText->text() );
	}


	//check pre loaded drumkit name  and reload the old drumkit
	if ( predrumkit != NULL ){
		if ( predrumkit->get_name() !=  Hydrogen::get_instance()->getCurrentDrumkitname() ){
			Hydrogen::get_instance()->loadDrumkit( predrumkit );
			Hydrogen::get_instance()->getSong()->set_is_modified( true );
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
