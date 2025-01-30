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

#ifndef INFOBAR_H
#define INFOBAR_H

#include <QWidget>
#include <vector>
#include <core/Preferences/Preferences.h>
#include <core/Object.h>

class QHBoxLayout;
class QLabel;
class QPushButton;

/** \ingroup docGUI docWidgets*/
class InfoBar : public QWidget, public H2Core::Object<InfoBar>
{
	H2_OBJECT(InfoBar)
	Q_OBJECT
	
	QHBoxLayout *m_pLayout;
	QLabel *m_pLabel;

	QString m_sTitle;
	QString m_sText;

	std::vector<QPushButton *> m_buttons;

	public:
	InfoBar(QWidget *parent = Q_NULLPTR);
	void setTitle(const QString &text);
	void setText(const QString &text);
	QPushButton *addButton( const QString &label );
	void reset();

public slots:
	void onPreferencesChanged( H2Core::Preferences::Changes changes );


	private:
	void updateStyleSheet();
	void createLayout();
	void createIcon();
	void createLabel();
	void createCloseButton();
	void updateText();
};

#endif
