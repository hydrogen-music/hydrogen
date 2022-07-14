/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
									  std::shared_ptr<Drumkit> pPreDrumKit,
									  bool bCurrentDrumkit );
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
		/** The one selected by the user */
		std::shared_ptr<Drumkit> m_pDrumkitInfo;
		/** The one currently loaded in Hydrogen.
		 *
		 * Since changes to a drumkit can only the saved correctly
		 * when first loading it, we need to keep a pointer to the
		 * current one in order to restore it.
		 */
		std::shared_ptr<Drumkit> m_pPreDrumkitInfo;

	/**
	 * Specifies whether the dialog was invoked for drumkit that
	 * can be found on disk (via the SoundLibrary tree) or for the one
	 * currently loaded in the Song (via the main menu). For the
	 * latter #m_pDrumkitInfo holds all the associated
	 * DrumkitComponent but the instrument list is stored in the
	 * current #Song.
	 */
	bool m_bCurrentDrumkit;
};

}
#endif
