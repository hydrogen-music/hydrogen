/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef DRUMKIT_EXPORT_DIALOG_H
#define DRUMKIT_EXPORT_DIALOG_H

#include <QtGui>
#include <QtWidgets>

#include "ui_DrumkitExportDialog_UI.h"

#include <core/Object.h>
#include <core/Basics/Drumkit.h>

///
///
///
/** \ingroup docGUI*/
class DrumkitExportDialog :  public QDialog, public Ui_DrumkitExportDialog_UI,  public H2Core::Object<DrumkitExportDialog>
{
	H2_OBJECT(DrumkitExportDialog)
	Q_OBJECT
	public:
	DrumkitExportDialog( QWidget* pParent, std::shared_ptr<H2Core::Drumkit> pDrumkit );
		~DrumkitExportDialog();

private slots:
	void on_exportBtn_clicked();
	void on_browseBtn_clicked();
	void on_cancelBtn_clicked();
	void on_versionList_currentIndexChanged( int index );
	void on_drumkitPathTxt_textChanged( QString str );
	
private:
	void updateComponentList();
	std::shared_ptr<H2Core::Drumkit> m_pDrumkit;
	QStringList m_components;
};


#endif

