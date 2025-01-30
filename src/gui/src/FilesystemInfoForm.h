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

#ifndef FILESYSTEMINFOFORM_H
#define FILESYSTEMINFOFORM_H

#include <QWidget>
#include "core/Object.h"

namespace Ui {
class FilesystemInfoForm;
}

/** \ingroup docGUI docDebugging*/
class FilesystemInfoForm :  public QWidget,  public H2Core::Object<FilesystemInfoForm>
{
	H2_OBJECT(FilesystemInfoForm)
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
	
	virtual void showEvent ( QShowEvent* ) override;
};

#endif // FILESYSTEMINFOFORM_H
