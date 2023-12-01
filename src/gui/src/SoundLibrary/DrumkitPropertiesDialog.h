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

#ifndef DRUMKIT_PROPERTIES_DIALOG_H
#define DRUMKIT_PROPERTIES_DIALOG_H

#include "ui_DrumkitPropertiesDialog_UI.h"
#include "../Widgets/WidgetWithLicenseProperty.h"

#include <core/Basics/Drumkit.h>
#include <core/Object.h>

///
///
namespace H2Core
{

/** \ingroup docGUI*/
class DrumkitPropertiesDialog :  public QDialog,
								 protected WidgetWithLicenseProperty,
								 public Ui_DrumkitPropertiesDialog_UI,
								 public H2Core::Object<DrumkitPropertiesDialog>
{
	H2_OBJECT(DrumkitPropertiesDialog)
	Q_OBJECT
	public:
		DrumkitPropertiesDialog( QWidget* pParent,
									  std::shared_ptr<Drumkit> pDrumkit,
									  bool bEditingNotSaving );
		~DrumkitPropertiesDialog();
		void showEvent( QShowEvent *e ) override;

	private slots:
		void on_saveBtn_clicked();
		void on_imageBrowsePushButton_clicked();
	void licenseComboBoxChanged( int );
	void imageLicenseComboBoxChanged( int );

  private:
	void updateMappingTable();
	void updateLicensesTable();
	void updateImage( QString& filename );
	void saveDrumkitMap();

	std::shared_ptr<Drumkit> m_pDrumkit;
	/**
	 * This dialog can be used to both alter the properties of a drumkit as well
	 * as to save it as a new kit.
	 */
	bool m_bEditingNotSaving;

	QString m_sNewImagePath;
	
};

}
#endif
