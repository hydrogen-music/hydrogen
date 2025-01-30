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

#ifndef SONG_PROPERTIES_DIALOG_H
#define SONG_PROPERTIES_DIALOG_H


#include "ui_SongPropertiesDialog_UI.h"
#include "HydrogenApp.h"
#include "Widgets/WidgetWithLicenseProperty.h"

/**
 * Song Properties Dialog
 */
/** \ingroup docGUI*/
class SongPropertiesDialog : public QDialog,
							 protected WidgetWithLicenseProperty,
							 private Ui_SongPropertiesDialog_UI
{
	Q_OBJECT

	public:
		explicit SongPropertiesDialog(QWidget* parent);
		~SongPropertiesDialog();

	private slots:
		void on_cancelBtn_clicked();
		void on_okBtn_clicked();
	void licenseComboBoxChanged( int );

};

#endif


