#ifndef FILESYSTEMINFOFORM_H
#define FILESYSTEMINFOFORM_H

#include <QWidget>
#include "core/Object.h"

namespace Ui {
class FilesystemInfoForm;
}

class FilesystemInfoForm : public QWidget, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT
	
public:
	explicit FilesystemInfoForm(QWidget *parent = nullptr);
	~FilesystemInfoForm();
	
private:
	Ui::FilesystemInfoForm *ui;
	
	void updateInfo();

private slots:
	void	on_openTmpButton_clicked();
	void	on_openUsrButton_clicked();
	void	on_openSysButton_clicked();
	
	void	showEvent ( QShowEvent* );
};

#endif // FILESYSTEMINFOFORM_H
