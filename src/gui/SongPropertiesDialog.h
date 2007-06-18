/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: SongPropertiesDialog.h,v 1.6 2005/05/01 19:50:59 comix Exp $
 *
 */

#ifndef SONG_PROPERTIES_DIALOG_H
#define SONG_PROPERTIES_DIALOG_H

#include "config.h"
#include "qlineedit.h"
#include "qpixmap.h"

#include "UI/SongPropertiesDialog_UI.h"
#include "HydrogenApp.h"

/**
 * Song Properties Dialog
 */
class SongPropertiesDialog : public SongPropertiesDialog_UI 
{
	Q_OBJECT

	public:
		/** Constructor */
		SongPropertiesDialog(QWidget* parent);

		/** Destructor */
		~SongPropertiesDialog();

		void cancelBtnClicked();
		void okBtnClicked();


	public slots:

};

#endif


