/*
 * Hydrogen
 * Copyright(c) 2002-2016 by the Hydrogen Team 
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

#ifndef INFOBAR_H
#define INFOBAR_H

#include <QWidget>
#include <vector>

class QHBoxLayout;
class QLabel;
class QPushButton;

class InfoBar : public QWidget
{
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

	private:
	void setBackgroundColor();
	void createLayout();
	void createIcon();
	void createLabel();
	void createCloseButton();
	void updateText();
};

#endif
