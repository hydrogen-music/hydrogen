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

#ifndef SONG_EDITOR_PANEL_TAG_WIDGET_H
#define SONG_EDITOR_PANEL_TAG_WIDGET_H

#include <QDialog>
#include "ui_SongEditorPanelTagWidget_UI.h"
#include <core/Object.h>

#include <memory>

///
///
namespace H2Core
{


/** \ingroup docGUI*/
class SongEditorPanelTagWidget :  public QDialog, public Ui_SongEditorPanelTagWidget_UI,  public H2Core::Object<SongEditorPanelTagWidget>
{
    H2_OBJECT(SongEditorPanelTagWidget)

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
		int m_nMaxRows;
		int m_nTimelinePosition;
		void createTheTagTableWidget();
		std::vector<QString> m_oldTags;
};

}
#endif
