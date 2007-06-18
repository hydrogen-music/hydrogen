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
 * $Id: ExportSongDialog.h,v 1.6 2005/05/01 19:50:57 comix Exp $
 *
 */


#ifndef EXPORT_SONG_DIALOG_H
#define EXPORT_SONG_DIALOG_H


#include "qfiledialog.h"
#include "qpixmap.h"
#include "qlineedit.h"
#include "qmessagebox.h"
#include "qpushbutton.h"

#include "UI/ExportSongDialog_UI.h"
#include "EventListener.h"
#include "lib/Object.h"

///
/// Dialog for exporting song
///
class ExportSongDialog : public ExportSongDialog_UI, public EventListener, public Object
{
	Q_OBJECT

	public:
		ExportSongDialog(QWidget* parent);
		~ExportSongDialog();
		void browseBtnClicked();
		void filenameTxtChanged();

		virtual void closeBtnClicked();
		virtual void okBtnClicked();

		virtual void progressEvent( int nValue );
	private:
		bool m_bExporting;
};


#endif

