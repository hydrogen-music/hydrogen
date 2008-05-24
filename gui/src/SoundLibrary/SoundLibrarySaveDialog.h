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

#ifndef SOUND_LIBRARY_SAVE_DIALOG_H
#define SOUND_LIBRARY_SAVE_DIALOG_H

#include "config.h"
#include "ui_SoundLibrarySaveDialog_UI.h"
#include <hydrogen/Object.h>

///
///
///
class SoundLibrarySaveDialog : public QDialog, public Ui_SoundLibrarySaveDialog_UI, public Object
{
	Q_OBJECT
	public:
		SoundLibrarySaveDialog( QWidget* pParent );
		~SoundLibrarySaveDialog();

	private slots:
			void on_saveBtn_clicked();


	private:
};


#endif

