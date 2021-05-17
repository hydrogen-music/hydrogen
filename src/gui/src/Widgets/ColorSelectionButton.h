/*
 * Hydrogen
 * Copyright (C) 2021 The hydrogen development team <hydrogen-devel@lists.sourceforge.net>
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


#ifndef COLORSELECTIONBUTTON_H
#define COLORSELECTIONBUTTON_H


#include <core/Object.h>
#include <core/Preferences.h>

#include <QtGui>
#include <QPushButton>
#include <QColor>

class ColorSelectionButton : public QPushButton, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT

public:
	ColorSelectionButton( QWidget *pParent, QColor sInitialColor, int nSize );
	~ColorSelectionButton();

	QColor getColor() const;

signals:
	void colorChanged();

private:
	bool m_bMouseOver;

	void mousePressEvent(QMouseEvent *ev);
	void enterEvent(QEvent *ev);
	void leaveEvent(QEvent *ev);
	void paintEvent( QPaintEvent* ev);

	QColor m_sColor;

};

inline QColor ColorSelectionButton::getColor() const {
	return m_sColor;
}

#endif
