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

#ifndef SOUND_LIBRARY_ONLINE_IMPORT_DIALOG_H
#define SOUND_LIBRARY_ONLINE_IMPORT_DIALOG_H


#include "ui_SoundLibraryOnlineImportDialog_UI.h"
#include <QNetworkReply>
#include "../../EventListener.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

///
/// This dialog is used to import a SoundLibrary file from a local file or via HTTP.
///
/** \ingroup docGUI*/
class SoundLibraryOnlineImportDialog :  public QDialog,
								  public Ui_SoundLibraryOnlineImportDialog_UI,
								  public H2Core::Object<SoundLibraryOnlineImportDialog>,
								  public EventListener
{
    H2_OBJECT(SoundLibraryOnlineImportDialog)
	Q_OBJECT
	public:
		SoundLibraryOnlineImportDialog( QWidget* pParent );
		~SoundLibraryOnlineImportDialog();

	virtual void soundLibraryChangedEvent() override;

	signals:

	private slots:
		void on_EditListBtn_clicked();
		void on_UpdateListBtn_clicked();
		void on_DownloadBtn_clicked();

		void on_close_btn_clicked();

		void soundLibraryItemChanged( QTreeWidgetItem*, QTreeWidgetItem* );
		void onRepositoryComboBoxIndexChanged(int);

		// Indicate the number of selected items in the download button and only
		// active it in case at least one was selected.
		void selectionChanged();

	private:
	std::vector<H2Core::SoundLibraryInfo> m_soundLibraryList;

		QTreeWidgetItem* m_pDrumkitsItem;
		QTreeWidgetItem* m_pSongItem;
		QTreeWidgetItem* m_pPatternItem;

		QString m_sDownloadBtnBase;
		QString m_sLabelInstalled;
		QString m_sLabelNew;

		bool isSoundLibraryItemAlreadyInstalled( const H2Core::SoundLibraryInfo& sInfo );
		void writeCachedData(const QString& fileName, const QString& data);
		void writeCachedImage( const QString& imageFile, const QPixmap& pixmap );
		void clearImageCache();
		QString readCachedImage( const QString& imageFile );
		QString readCachedData(const QString& fileName);
		QString getCachedFileName();
		QString getCachedImageFileName();
		void reloadRepositoryData();
		void updateSoundLibraryList();
		void updateRepositoryCombo();
		void showImage( const QPixmap& pixmap );
		void loadImage( const QString& img );
		void updateDownloadBtn();
};

#endif
