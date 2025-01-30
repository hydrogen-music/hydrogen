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

#ifndef SOUND_LIBRARY_PROPERTIES_DIALOG_H
#define SOUND_LIBRARY_PROPERTIES_DIALOG_H

#include "ui_SoundLibraryPropertiesDialog_UI.h"
#include "../Widgets/WidgetWithLicenseProperty.h"

#include <core/Basics/Drumkit.h>
#include <core/Object.h>

///
///
namespace H2Core
{

/** \ingroup docGUI*/
class SoundLibraryPropertiesDialog :  public QDialog,
									  protected WidgetWithLicenseProperty,
									  public Ui_SoundLibraryPropertiesDialog_UI,
									  public H2Core::Object<SoundLibraryPropertiesDialog>
{
	H2_OBJECT(SoundLibraryPropertiesDialog)
	Q_OBJECT
	public:
		SoundLibraryPropertiesDialog( QWidget* pParent,
									  std::shared_ptr<Drumkit> pDrumkit,
									  bool bDrumkitNameLocked );
		~SoundLibraryPropertiesDialog();
		void showEvent( QShowEvent *e ) override;

	private slots:
		void on_saveBtn_clicked();
		void on_imageBrowsePushButton_clicked();
	void licenseComboBoxChanged( int );
	void imageLicenseComboBoxChanged( int );

	private:
	void updateLicenseTable();
	void updateImage( QString& filename );

	std::shared_ptr<Drumkit> m_pDrumkit;
	/**
	 * This dialog can be accessed both via SoundLibrary/MainForm >
	 * Drumkits -> Properties and MainForm > Drumkits -> Save
	 * As. Historically they were two distinct dialogs featuring
	 * pretty much exactly the same fields.
	 *
	 * In order to keep the general menu structure both choices are
	 * still supported but a small tweak was introduced to make them
	 * work slightly differently. When accessed via "Properties" this
	 * variable is set to true and it is not possible to create new
	 * drumkits by altering the name of an existing one. If, on the
	 * other hand, it's opened via "Save As" anything goes.
	 */
	bool m_bDrumkitNameLocked;

	QString m_sNewImagePath;
	
};

}
#endif
