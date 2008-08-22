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

#ifndef ABOUT__DIALOG_H
#define ABOUT__DIALOG_H

#include "config.h"

#include <QtGui>

#include "ui_about_dialog.h"

class AboutDialog : public QDialog, public Ui_AboutDialog_UI
{
Q_OBJECT
public:
	AboutDialog(QWidget* parent);
	~AboutDialog();

private slots:
	void on_okBtn_clicked();

private:
	class Author {
		public:
			QString m_sName;
			QString m_sEmail;
			QString m_sInfo;

			Author( QString sName, QString sEmail, QString sInfo ) : m_sName( sName ), m_sEmail( sEmail ), m_sInfo( sInfo ) {}
	};
};

#endif
