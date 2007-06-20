/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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


#ifndef FILE_PREVIEW_H
#define FILE_PREVIEW_H

#include <hydrogen/Object.h>

#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <Q3FileDialog>

#include <string>
using std::string;

class FilePreview : public QWidget, public Q3FilePreview, public Object
{
	Q_OBJECT
	public:
		FilePreview( QWidget *parent=0 );
		~FilePreview();

		void previewUrl( const Q3Url &u );

	public slots:
		void playClicked();


	private:
		QPushButton *m_pPlayBtn;
		QLabel *m_pNBytes;
		QLabel *m_pSamplerate;
		string m_sFilename;
};

#endif
