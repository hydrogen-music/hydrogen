/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include "HydrogenApp.h"
#include "Skin.h"

#include "core/Helpers/Filesystem.h"
#include "core/Preferences/Preferences.h"

FilesystemInfoForm::FilesystemInfoForm( QWidget *parent ) :
	QWidget( parent )
{
	setupUi( this );

	const auto pPref = HydrogenApp::pPreferences();
	QString sIconPath( Skin::getSvgImagePath() );
	if ( pPref->getInterfaceTheme()->m_iconColor ==
		 H2Core::InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	const QColor windowColor = pPref->getColorTheme()->m_windowColor;
	const QColor windowTextColor = pPref->getColorTheme()->m_windowTextColor;

	tmpDirWarningButton->setIcon( QIcon( sIconPath + "warning.svg" ) );
	tmpDirWarningButton->setToolTip( tr( "Filesystem is not writable!" ) );
	tmpDirWarningButton->setType( Button::Type::Icon );
	tmpDirWarningButton->setSize( QSize( 16, 14 ) );
	
	tmpDirLineEdit->setReadOnly( true );
	
	usrDataDirWarningButton->setIcon( QIcon( sIconPath + "warning.svg" ) );
	usrDataDirWarningButton->setToolTip( tr( "User data folder is not writable!" ) );
	usrDataDirWarningButton->setType( Button::Type::Icon );
	usrDataDirWarningButton->setSize( QSize( 16, 14 ) );
	
	usrDataDirLineEdit->setReadOnly( true );
	sysDataDirLineEdit->setReadOnly( true );
	
	updateInfo();
}

FilesystemInfoForm::~FilesystemInfoForm()
{
}

void FilesystemInfoForm::showEvent ( QShowEvent* )
{
	updateInfo();
}
void FilesystemInfoForm::updateInfo()
{
	const QString tmpDir = H2Core::Filesystem::tmpDir();
	const QString usrDataDir = H2Core::Filesystem::userDataPath();
	const QString sysDataDir = H2Core::Filesystem::systemDataPath();
	
	tmpDirLineEdit->setText( tmpDir);
	tmpDirLineEdit->setToolTip( tmpDir );
	
	if(!H2Core::Filesystem::dirWritable( tmpDir, true)) {
		tmpDirWarningButton->show();
	} else {
		tmpDirWarningButton->hide();
	}
	
	usrDataDirLineEdit->setText( usrDataDir );
	usrDataDirLineEdit->setToolTip( usrDataDir );
	
	if(!H2Core::Filesystem::dirWritable( usrDataDir, true)) {
		usrDataDirWarningButton->show();
	} else {
		usrDataDirWarningButton->hide();
	}
	
	sysDataDirLineEdit->setText( sysDataDir );
	sysDataDirLineEdit->setToolTip( sysDataDir );
	
	//System data dir is not writable for the user, so no warning here...
}

void 
FilesystemInfoForm::on_openTmpButton_clicked()
{
	QDesktopServices::openUrl( QUrl::fromLocalFile( H2Core::Filesystem::tmpDir() ) );
}

void 
FilesystemInfoForm::on_openUsrButton_clicked()
{
	QDesktopServices::openUrl( QUrl::fromLocalFile( H2Core::Filesystem::userDataPath() ) );
}

void 
FilesystemInfoForm::on_openSysButton_clicked()
{
	QDesktopServices::openUrl( QUrl::fromLocalFile( H2Core::Filesystem::systemDataPath() ) );
}
