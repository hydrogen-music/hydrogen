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

#ifndef ABOUT__DIALOG_H
#define ABOUT__DIALOG_H


#include <QtGui>
#include <QtWidgets>
#include <QTextBrowser>

#include "ui_AboutDialog_UI.h"

/** \ingroup docGUI*/
class AboutDialog : public QDialog, public Ui_AboutDialog_UI
{
Q_OBJECT
public:
	explicit AboutDialog(QWidget* parent);
	~AboutDialog();

private slots:
	void on_okBtn_clicked();

private:
	class Author {
		public:
			QString m_sName;
			QString m_sEmail;
			QString m_sInfo;

			Author( const QString& sName, const QString& sEmail,
					const QString& sInfo )
				: m_sName( sName ), m_sEmail( sEmail ), m_sInfo( sInfo ) {}
	};
};

#endif
