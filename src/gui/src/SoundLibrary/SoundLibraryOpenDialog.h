/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef SOUND_LIBRARY_OPEN_DIALOG_H
#define SOUND_LIBRARY_OPEN_DIALOG_H

#include <QtGui>

#include "SoundLibraryDatastructures.h"

class SoundLibraryPanel;

class SoundLibraryOpenDialog : public QDialog, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT
	public:
		SoundLibraryOpenDialog( QWidget* pParent );
		~SoundLibraryOpenDialog();

	private slots:
		void on_soundLib_item_changed( bool bDrumkitSelected );

		void on_cancel_btn_clicked();
		void on_open_btn_clicked();

	private:
		SoundLibraryPanel* p_soundLib;
		QPushButton* p_btnOk;
		QPushButton* p_btnCancel;
};

#endif
