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
	
	ui->sysDataDirLineEdit->setText( H2Core::Filesystem::sys_drumkits_dir() );
	ui->sysDataDirLineEdit->setToolTip( H2Core::Filesystem::sys_drumkits_dir() );
	
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
