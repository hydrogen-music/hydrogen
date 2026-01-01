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

#ifndef DRUMKIT_PROPERTIES_DIALOG_H
#define DRUMKIT_PROPERTIES_DIALOG_H

#include "ui_DrumkitPropertiesDialog_UI.h"
#include "../../Widgets/WidgetWithLicenseProperty.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
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
		/** @param nInstrumentID If set to a value different than
		 *   #Instrument::EmptyId, the corresponding line in the type tab will be
		 *   selected on startup. */
		DrumkitPropertiesDialog( QWidget* pParent,
								 std::shared_ptr<Drumkit> pDrumkit,
								 bool bEditingNotSaving,
								 bool bSaveToNsmSession,
								 Instrument::Id id = Instrument::EmptyId );
		~DrumkitPropertiesDialog();
		void showEvent( QShowEvent *e ) override;

	private slots:
		void on_saveBtn_clicked();
		void on_imageBrowsePushButton_clicked();
	void licenseComboBoxChanged( int );
	void imageLicenseComboBoxChanged( int );

  private:
	void updateTypesTable( bool bDrumkitWritable );
	void updateLicensesTable();
		void highlightDuplicates();
	void updateImage( const QString& sFilePath );
	void saveDrumkitMap();

	std::shared_ptr<Drumkit> m_pDrumkit;
	/**
	 * This dialog can be used to both alter the properties of a drumkit as well
	 * as to save it as a new kit.
	 */
	bool m_bEditingNotSaving;

		/** Whether the kit should be stored in the users' drumkit folder or in
		 * the NSM session folder (only available when Hydrogen is under session
		 * management). */
		bool m_bSaveToNsmSession;

	QString m_sNewImagePath;

		/** used to selected a specific instrument type row on opening based on
		 * the provided instrument ID. */
		std::map<Instrument::Id, LCDCombo*> m_idToTypeMap;
};

}
#endif
