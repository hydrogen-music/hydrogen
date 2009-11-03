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

#ifndef SONG_EDITOR_PANEL_TAG_WIDGET_H
#define SONG_EDITOR_PANEL_TAG_WIDGET_H

#include <QDialog>
#include "config.h"
#include "ui_SongEditorPanelTagWidget_UI.h"
#include <hydrogen/Object.h>

///
///
namespace H2Core
{


class SongEditorPanelTagWidget : public QDialog, public Ui_SongEditorPanelTagWidget_UI, public Object
{

//lineEditBEAT
//lineEditBPM
//deleteBtn

	Q_OBJECT
	public:
		SongEditorPanelTagWidget( QWidget* pParent, int beat );
		~SongEditorPanelTagWidget();

	private slots:

		void on_CancelBtn_clicked();
		void on_okBtn_clicked();
		void on_tagTableWidget_currentItemChanged( QTableWidgetItem * current, QTableWidgetItem * previous );

	private:
		int m_stimelineposition;
		void createTheTagTableWidget();
};

}
#endif
