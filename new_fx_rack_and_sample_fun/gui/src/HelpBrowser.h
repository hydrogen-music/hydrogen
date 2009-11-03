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


#ifndef HELPBROWSER_H
#define HELPBROWSER_H

#include "config.h"

#include <iostream>
#include <string>
#include <QtGui>

#include <hydrogen/Object.h>

class SimpleHTMLBrowser : public QDialog, public Object
{
	Q_OBJECT
	public:
		enum SimpleHTMLBrowserType {
			WELCOME,
			MANUAL
		};

		SimpleHTMLBrowser( QWidget *pParent, const QString& sDataPath, const QString& sFilename, SimpleHTMLBrowserType type );
		~SimpleHTMLBrowser();

	public slots:
		void dontShowAnymoreBtnClicked();
		void closeWindow();
		void docIndex();

	private:
		SimpleHTMLBrowserType m_type;
		QTextBrowser *m_pBrowser;
		QPushButton *m_pDontShowAnymoreBtn;
		QPushButton *m_pCloseWindowBtn;
		QPushButton *m_pDocHomeBtn;

		QString m_sDataPath;
		QString m_sFilename;

		virtual void showEvent ( QShowEvent *ev );
		virtual void resizeEvent( QResizeEvent *ev );
};


#endif
