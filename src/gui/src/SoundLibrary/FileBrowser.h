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

#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H


#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

/** \ingroup docGUI*/
class FileBrowser : public QWidget, private H2Core::Object<FileBrowser>
{
    H2_OBJECT(FileBrowser)
	Q_OBJECT
	public:
		explicit FileBrowser( QWidget* pParent );
		~FileBrowser();

	private slots:
		void on_fileList_ItemChanged( QListWidgetItem * current, QListWidgetItem * previous );
		void on_fileList_ItemActivated( QListWidgetItem* );

		void on_dirList_ItemActivated( QListWidgetItem* );
		void on_upBtnClicked();
		void on_playBtnClicked();

	private:
		QLabel *m_pDirectoryLabel;
		QPushButton* m_pUpBtn;
		QLabel *m_pFileInfo;
		QListWidget *m_pDirList;
		QListWidget *m_pFileList;
		QDir m_directory;


		void loadDirectoryTree( const QString& basedir );
		void updateFileInfo( QString sFilename, unsigned nSampleRate, unsigned nBytes );

};


#endif
