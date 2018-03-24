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

#ifndef SOUND_LIBRARY_IMPORT_DIALOG_H
#define SOUND_LIBRARY_IMPORT_DIALOG_H


#include "ui_SoundLibraryImportDialog_UI.h"
#include <hydrogen/object.h>
#include <QNetworkReply>

#include <hydrogen/Preferences.h>
#include "SoundLibraryDatastructures.h"

///
/// This dialog is used to import a SoundLibrary file from a local file or via HTTP.
///
class SoundLibraryImportDialog : public QDialog, public Ui_SoundLibraryImportDialog_UI, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT
	public:
		SoundLibraryImportDialog( QWidget* pParent, bool bOnlineImport );
		~SoundLibraryImportDialog();

	signals:

	private slots:
		void on_EditListBtn_clicked();
		void on_UpdateListBtn_clicked();
		void on_DownloadBtn_clicked();
		void on_BrowseBtn_clicked();
		void on_InstallBtn_clicked();

		void on_close_btn_clicked();

		void soundLibraryItemChanged( QTreeWidgetItem*, QTreeWidgetItem* );
		void onRepositoryComboBoxIndexChanged(int);



	private:
		std::vector<SoundLibraryInfo> m_soundLibraryList;

		QTreeWidgetItem* m_pDrumkitsItem;
		QTreeWidgetItem* m_pSongItem;
		QTreeWidgetItem* m_pPatternItem;

		bool isSoundLibraryItemAlreadyInstalled( SoundLibraryInfo sInfo );
		void writeCachedData(const QString& fileName, const QString& data);
		void writeCachedImage( const QString& imageFile, QPixmap& pixmap );
		void clearImageCache();
		QString readCachedImage( const QString& imageFile );
		QString readCachedData(const QString& fileName);
		QString getCachedFilename();
		QString getCachedImageFilename();
		void reloadRepositoryData();
		void updateSoundLibraryList();
		void updateRepositoryCombo();
		void showImage( QPixmap pixmap );
		void loadImage( QString img );

};

#endif
