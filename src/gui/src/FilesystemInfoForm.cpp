#include <QDesktopServices>

#include "FilesystemInfoForm.h"
#include "ui_FilesystemInfoForm_UI.h"
#include "Skin.h"

#include "core/Helpers/Filesystem.h"

const char* FilesystemInfoForm::__class_name = "FilesystemInfoForm";

FilesystemInfoForm::FilesystemInfoForm( QWidget *parent ) :
	QWidget( parent ),
	H2Core::Object( __class_name ),
	ui(new Ui::FilesystemInfoForm)
{
	ui->setupUi(this);

	ui->tmpDirWarningButton->setIcon( QIcon( Skin::getSvgImagePath() + "/icons/warning.svg" ) );
	ui->tmpDirWarningButton->setStyleSheet( QString( "	\
width: 16px; \
height: 16px; \
color: %1; \
background-color: %2; \
border-color: %2;" ).arg( Skin::getWindowTextColor().name() ).arg( Skin::getWindowColor().name() ) );
	ui->tmpDirWarningButton->setToolTip( tr( "Filesystem is not writable!" ) );
	ui->tmpDirWarningButton->setFlat( true );
	
	ui->usrDataDirWarningButton->setIcon( QIcon( Skin::getSvgImagePath() + "/icons/warning.svg" ) );
	ui->usrDataDirWarningButton->setStyleSheet( QString( "	\
width: 16px; \
height: 16px; \
color: %1; \
background-color: %2; \
border-color: %2;" ).arg( Skin::getWindowTextColor().name() ).arg( Skin::getWindowColor().name() ) );
	ui->usrDataDirWarningButton->setToolTip( tr( "User data folder is not writable!" ) );
	ui->usrDataDirWarningButton->setFlat( true );
	
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
