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


#ifndef EXPORT_SONG_DIALOG_H
#define EXPORT_SONG_DIALOG_H

#include "config.h"

#include "ui_ExportSongDialog_UI.h"
#include "EventListener.h"
#include <hydrogen/Object.h>

///
/// Dialog for exporting song
///
class ExportSongDialog : public QDialog, public Ui_ExportSongDialog_UI, public EventListener, public Object
{
	Q_OBJECT

	public:
		ExportSongDialog(QWidget* parent);
		~ExportSongDialog();


		virtual void progressEvent( int nValue );

		bool b_QfileDialog;

	private slots:
		void on_browseBtn_clicked();
		void on_closeBtn_clicked();
		void on_okBtn_clicked();
		void on_exportNameTxt_textChanged(const QString& text);
		void on_templateCombo_currentIndexChanged(int index );

	private:
		bool m_bExporting;
		void exportTracks();
		bool m_bExportTrackouts;
                bool m_bOverwriteFiles;
                uint m_nInstrument;
                QString m_sExtension;
};


#endif

