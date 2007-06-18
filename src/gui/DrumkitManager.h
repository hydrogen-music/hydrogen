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
 * $Id: DrumkitManager.h,v 1.6 2005/05/01 19:50:57 comix Exp $
 *
 */


#ifndef DRUMKIT_MANAGER_H
#define DRUMKIT_MANAGER_H

#include <qapplication.h>
#include <qcombobox.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qtextbrowser.h>
#include <qtextedit.h>
#include <qtimer.h>
#include <qwidget.h>

#include <vector>

#include "UI/DrumkitManager_UI.h"
#include "lib/Object.h"
#include "lib/Song.h"

///
/// Drumkit manager
///
class DrumkitManager : public DrumkitManager_UI, public Object
{
	Q_OBJECT

	public:
		DrumkitManager( QWidget* parent );
		~DrumkitManager();

	private:
		std::vector<DrumkitInfo*> drumkitInfoList;

		void updateDrumkitList();

		void okBtnClicked();

		void loadTab_selectionChanged();
		void loadTab_loadDrumkitBtnClicked();
		void loadTab_deleteDrumkitBtnClicked();

		void importTab_browseBtnClicked();
		void importTab_importBtnClicked();
		void importTab_drumkitPathChanged();

		void saveTab_saveBtnClicked();
		void saveTab_nameChanged();

		void tabChanged();

		void exportTab_browseBtnClicked();
		void exportTab_exportBtnClicked();
		void exportTab_drumkitPathChanged();
};


#endif

