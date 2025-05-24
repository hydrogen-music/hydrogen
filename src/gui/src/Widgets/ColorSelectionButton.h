/*
 * Hydrogen
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef COLORSELECTIONBUTTON_H
#define COLORSELECTIONBUTTON_H


#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtGui>
#include <QPushButton>
#include <QColor>

/** QPushButton opening a QColorDialog when clicked and displaying the
 * selected color as background - with neither text nor an icon present.
 * 
 */
/** \ingroup docGUI docWidgets*/
class ColorSelectionButton : public QPushButton, public H2Core::Object<ColorSelectionButton>
{
    H2_OBJECT(ColorSelectionButton)
	Q_OBJECT

public:
	ColorSelectionButton( QWidget *pParent, QColor sInitialColor = Qt::black, int nSize = 0 );
	~ColorSelectionButton();

	QColor getColor() const;
	void setColor( const QColor& color );
	
signals:
	void colorChanged();

private:
	bool m_bMouseOver;

	virtual void mousePressEvent(QMouseEvent *ev) override;
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
	virtual void leaveEvent(QEvent *ev) override;
	virtual void paintEvent( QPaintEvent* ev) override;

	QColor m_sColor;
};

inline QColor ColorSelectionButton::getColor() const {
	return m_sColor;
}

#endif
