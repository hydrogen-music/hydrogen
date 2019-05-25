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

#ifndef SOUND_LIBRARY_EXPORT_DIALOG_H
#define SOUND_LIBRARY_EXPORT_DIALOG_H

#include "ui_SoundLibraryExportDialog_UI.h"

#include <hydrogen/object.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/drumkit.h>

#include <vector>

///
///
///
class SoundLibraryExportDialog : public QDialog, public Ui_SoundLibraryExportDialog_UI, public H2Core::Object
{
	Q_OBJECT
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	SoundLibraryExportDialog( QWidget* pParent, const QString&);
	~SoundLibraryExportDialog();

private slots:
	void on_exportBtn_clicked();
	void on_browseBtn_clicked();
	void on_cancelBtn_clicked();
	void on_versionList_currentIndexChanged( int index );
	void on_drumkitList_currentIndexChanged( QString str );
	void on_drumkitPathTxt_textChanged( QString str );
	void updateDrumkitList();
private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
	std::vector<H2Core::Drumkit*> drumkitInfoList;
	QString preselectedKit;
	QHash<QString, QStringList> kit_components;
};


#endif

