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

#include <QDesktopServices>

#include "FilesystemInfoForm.h"
#include "ui_FilesystemInfoForm_UI.h"
#include "Skin.h"

#include "core/Helpers/Filesystem.h"
#include "core/Preferences/Preferences.h"

FilesystemInfoForm::FilesystemInfoForm( QWidget *parent ) :
	QWidget( parent ),
	H2Core::Object<FilesystemInfoForm>(),
	ui(new Ui::FilesystemInfoForm)
{
	ui->setupUi(this);

	const auto theme = H2Core::Preferences::get_instance()->getTheme();
	QString sIconPath( Skin::getSvgImagePath() );
	if ( theme.m_interface.m_iconColor ==
		 H2Core::InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	QColor windowColor = theme.m_color.m_windowColor;
	QColor windowTextColor = theme.m_color.m_windowTextColor;

	ui->tmpDirWarningButton->setIcon( QIcon( sIconPath + "warning.svg" ) );
	ui->tmpDirWarningButton->setToolTip( tr( "Filesystem is not writable!" ) );
	ui->tmpDirWarningButton->setType( Button::Type::Icon );
	ui->tmpDirWarningButton->setSize( QSize( 16, 14 ) );
	
	ui->tmpDirLineEdit->setReadOnly( true );
	
	ui->usrDataDirWarningButton->setIcon( QIcon( sIconPath + "warning.svg" ) );
	ui->usrDataDirWarningButton->setToolTip( tr( "User data folder is not writable!" ) );
	ui->usrDataDirWarningButton->setType( Button::Type::Icon );
	ui->usrDataDirWarningButton->setSize( QSize( 16, 14 ) );
	
	ui->usrDataDirLineEdit->setReadOnly( true );
	ui->sysDataDirLineEdit->setReadOnly( true );
	
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
		ui->tmpDirWarningButton->show();
	} else {
		ui->tmpDirWarningButton->hide();
	}
	
	ui->usrDataDirLineEdit->setText( usrDataDir );
	ui->usrDataDirLineEdit->setToolTip( usrDataDir );
	
	if(!H2Core::Filesystem::dir_writable( usrDataDir, true)) {
		ui->usrDataDirWarningButton->show();
	} else {
		ui->usrDataDirWarningButton->hide();
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
