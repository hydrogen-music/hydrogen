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

#include <QDesktopServices>

#include "FilesystemInfoForm.h"
#include "ui_FilesystemInfoForm_UI.h"
#include "Skin.h"

#include "core/Helpers/Filesystem.h"

FilesystemInfoForm::FilesystemInfoForm( QWidget *parent ) :
	QWidget( parent ),
	H2Core::Object<FilesystemInfoForm>(),
	ui(new Ui::FilesystemInfoForm)
{
	ui->setupUi(this);
	
	updateInfo();
}

FilesystemInfoForm::~FilesystemInfoForm()
{
	delete ui;
}

void FilesystemInfoForm::showEvent ( QShowEvent* )
{
	updateInfo();
}
void FilesystemInfoForm::updateInfo()
{
	const QString tmpDir = H2Core::Filesystem::tmp_dir();
	const QString usrDataDir = H2Core::Filesystem::usr_data_path();
	const QString sysDataDir = H2Core::Filesystem::sys_data_path();
	
	ui->tmpDirLineEdit->setText( tmpDir);
	ui->tmpDirLineEdit->setToolTip( tmpDir );
	
	if(!H2Core::Filesystem::dir_writable( tmpDir, true)) {
		QPixmap warningIcon (Skin::getImagePath() + "/patternEditor/icn_warning.png" );
		ui->tmpDirWarningLabel->setPixmap(warningIcon);
		ui->tmpDirWarningLabel->setToolTip("Temporary directory is not writable");
	} else {
		ui->tmpDirWarningLabel->setText("");
	}
	
	ui->usrDataDirLineEdit->setText( usrDataDir );
	ui->usrDataDirLineEdit->setToolTip( usrDataDir );
	
	if(!H2Core::Filesystem::dir_writable( usrDataDir, true)) {
		QPixmap warningIcon (Skin::getImagePath() + "/patternEditor/icn_warning.png" );
		ui->usrDataDirWarningLabel->setPixmap(warningIcon);
		ui->usrDataDirWarningLabel->setToolTip("User data directory is not writable");
	} else {
		ui->usrDataDirWarningLabel->setText("");
	}
	
	ui->sysDataDirLineEdit->setText( sysDataDir );
	ui->sysDataDirLineEdit->setToolTip( sysDataDir );
	
	//System data dir is not writable for the user, so no warning here...
}

void 
FilesystemInfoForm::on_openTmpButton_clicked()
{
	QDesktopServices::openUrl( QUrl::fromLocalFile( H2Core::Filesystem::tmp_dir() ) );
}

void 
FilesystemInfoForm::on_openUsrButton_clicked()
{
	QDesktopServices::openUrl( QUrl::fromLocalFile( H2Core::Filesystem::usr_data_path() ) );
}

void 
FilesystemInfoForm::on_openSysButton_clicked()
{
	QDesktopServices::openUrl( QUrl::fromLocalFile( H2Core::Filesystem::sys_data_path() ) );
}
